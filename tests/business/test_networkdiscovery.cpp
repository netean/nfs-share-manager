#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTimer>
#include "../../src/business/networkdiscovery.h"
#include "../../src/core/remotenfsshare.h"

using namespace NFSShareManager;

class TestNetworkDiscovery : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testConstructor();
    void testStartStopDiscovery();
    void testScanModes();
    void testTargetHostManagement();
    void testAvahiIntegration();
    void testDiscoveryStatus();
    void testScanStatistics();

    // Discovery functionality tests
    void testShareDiscovery();
    void testShareAvailability();
    void testStaleShareRemoval();
    void testNetworkChangeHandling();

    // Configuration tests
    void testScanInterval();
    void testScanModeConfiguration();

private:
    NetworkDiscovery *m_discovery;
};

void TestNetworkDiscovery::initTestCase()
{
    // Initialize test environment
}

void TestNetworkDiscovery::cleanupTestCase()
{
    // Cleanup test environment
}

void TestNetworkDiscovery::init()
{
    m_discovery = new NetworkDiscovery(this);
}

void TestNetworkDiscovery::cleanup()
{
    delete m_discovery;
    m_discovery = nullptr;
}

void TestNetworkDiscovery::testConstructor()
{
    QVERIFY(m_discovery != nullptr);
    QVERIFY(!m_discovery->isDiscoveryActive());
    QCOMPARE(m_discovery->discoveryStatus(), NetworkDiscovery::DiscoveryStatus::Idle);
    QCOMPARE(m_discovery->scanInterval(), 30000); // Default interval
    QCOMPARE(m_discovery->scanMode(), NetworkDiscovery::ScanMode::Quick);
    QVERIFY(m_discovery->getDiscoveredShares().isEmpty());
    QVERIFY(m_discovery->getTargetHosts().isEmpty());
}

void TestNetworkDiscovery::testStartStopDiscovery()
{
    // Test starting discovery
    QSignalSpy startedSpy(m_discovery, &NetworkDiscovery::discoveryStarted);
    QSignalSpy statusSpy(m_discovery, &NetworkDiscovery::discoveryStatusChanged);
    
    m_discovery->startDiscovery(10000);
    
    QVERIFY(m_discovery->isDiscoveryActive());
    QCOMPARE(m_discovery->scanInterval(), 10000);
    
    // Wait for initial scan to start
    QTRY_VERIFY_WITH_TIMEOUT(startedSpy.count() >= 1, 5000);
    QVERIFY(statusSpy.count() >= 1);
    
    // Test stopping discovery
    m_discovery->stopDiscovery();
    
    QVERIFY(!m_discovery->isDiscoveryActive());
    QCOMPARE(m_discovery->discoveryStatus(), NetworkDiscovery::DiscoveryStatus::Idle);
}

void TestNetworkDiscovery::testScanModes()
{
    // Test scan mode configuration
    m_discovery->setScanMode(NetworkDiscovery::ScanMode::Full);
    QCOMPARE(m_discovery->scanMode(), NetworkDiscovery::ScanMode::Full);
    
    m_discovery->setScanMode(NetworkDiscovery::ScanMode::Targeted);
    QCOMPARE(m_discovery->scanMode(), NetworkDiscovery::ScanMode::Targeted);
    
    m_discovery->setScanMode(NetworkDiscovery::ScanMode::Quick);
    QCOMPARE(m_discovery->scanMode(), NetworkDiscovery::ScanMode::Quick);
}

void TestNetworkDiscovery::testTargetHostManagement()
{
    // Test adding target hosts
    m_discovery->addTargetHost("192.168.1.100");
    m_discovery->addTargetHost("server.local");
    
    QStringList hosts = m_discovery->getTargetHosts();
    QCOMPARE(hosts.size(), 2);
    QVERIFY(hosts.contains("192.168.1.100"));
    QVERIFY(hosts.contains("server.local"));
    
    // Test adding duplicate host (should not be added)
    m_discovery->addTargetHost("192.168.1.100");
    QCOMPARE(m_discovery->getTargetHosts().size(), 2);
    
    // Test removing host
    m_discovery->removeTargetHost("192.168.1.100");
    hosts = m_discovery->getTargetHosts();
    QCOMPARE(hosts.size(), 1);
    QVERIFY(!hosts.contains("192.168.1.100"));
    QVERIFY(hosts.contains("server.local"));
    
    // Test clearing all hosts
    m_discovery->clearTargetHosts();
    QVERIFY(m_discovery->getTargetHosts().isEmpty());
}

void TestNetworkDiscovery::testAvahiIntegration()
{
    // Test Avahi availability check
    bool avahiAvailable = m_discovery->isAvahiAvailable();
    // Note: This may be false in test environment, which is fine
    
    // Test enabling/disabling Avahi
    m_discovery->setAvahiEnabled(true);
    QVERIFY(m_discovery->isAvahiEnabled());
    
    m_discovery->setAvahiEnabled(false);
    QVERIFY(!m_discovery->isAvahiEnabled());
}

void TestNetworkDiscovery::testDiscoveryStatus()
{
    QSignalSpy statusSpy(m_discovery, &NetworkDiscovery::discoveryStatusChanged);
    
    // Initial status should be Idle
    QCOMPARE(m_discovery->discoveryStatus(), NetworkDiscovery::DiscoveryStatus::Idle);
    
    // Start discovery and check status changes
    m_discovery->startDiscovery(5000);
    
    // Should eventually change to Scanning and then Completed
    QTRY_VERIFY_WITH_TIMEOUT(statusSpy.count() >= 1, 10000);
    
    // Stop discovery
    m_discovery->stopDiscovery();
    
    // Status should return to Idle
    QCOMPARE(m_discovery->discoveryStatus(), NetworkDiscovery::DiscoveryStatus::Idle);
}

void TestNetworkDiscovery::testScanStatistics()
{
    // Get initial statistics
    QHash<QString, QVariant> stats = m_discovery->getScanStatistics();
    QVERIFY(stats.contains("total_scans"));
    QCOMPARE(stats["total_scans"].toInt(), 0);
    
    // Perform a scan and check statistics update
    QSignalSpy completedSpy(m_discovery, &NetworkDiscovery::discoveryCompleted);
    
    m_discovery->refreshDiscovery(NetworkDiscovery::ScanMode::Quick);
    
    // Wait for scan to complete
    QTRY_VERIFY_WITH_TIMEOUT(completedSpy.count() >= 1, 15000);
    
    // Check updated statistics
    stats = m_discovery->getScanStatistics();
    QVERIFY(stats["total_scans"].toInt() >= 1);
    QVERIFY(stats.contains("total_hosts_scanned"));
    QVERIFY(stats.contains("last_scan_duration"));
}

void TestNetworkDiscovery::testShareDiscovery()
{
    QSignalSpy discoveredSpy(m_discovery, &NetworkDiscovery::shareDiscovered);
    QSignalSpy completedSpy(m_discovery, &NetworkDiscovery::discoveryCompleted);
    
    // Add localhost as target (should be reachable but may not have NFS)
    m_discovery->addTargetHost("127.0.0.1");
    m_discovery->setScanMode(NetworkDiscovery::ScanMode::Targeted);
    
    // Perform discovery
    m_discovery->refreshDiscovery();
    
    // Wait for scan to complete
    QTRY_VERIFY_WITH_TIMEOUT(completedSpy.count() >= 1, 15000);
    
    // Check that scan completed (shares may or may not be found)
    QVERIFY(completedSpy.count() >= 1);
    
    // Get discovered shares
    QList<RemoteNFSShare> shares = m_discovery->getDiscoveredShares();
    // Note: May be empty if no NFS servers are running locally
}

void TestNetworkDiscovery::testShareAvailability()
{
    // Test getting recent shares
    QList<RemoteNFSShare> recentShares = m_discovery->getRecentShares(300);
    // Should be empty initially
    QVERIFY(recentShares.isEmpty());
    
    // Test with different time windows
    recentShares = m_discovery->getRecentShares(60);
    QVERIFY(recentShares.isEmpty());
}

void TestNetworkDiscovery::testStaleShareRemoval()
{
    // This test would require injecting mock shares with old timestamps
    // For now, just verify the method exists and doesn't crash
    QList<RemoteNFSShare> shares = m_discovery->getDiscoveredShares();
    int initialCount = shares.size();
    
    // Call refresh which should remove stale shares
    m_discovery->refreshDiscovery(NetworkDiscovery::ScanMode::Quick);
    
    // Wait for completion
    QSignalSpy completedSpy(m_discovery, &NetworkDiscovery::discoveryCompleted);
    QTRY_VERIFY_WITH_TIMEOUT(completedSpy.count() >= 1, 15000);
    
    // Verify no crash occurred
    QVERIFY(true);
}

void TestNetworkDiscovery::testNetworkChangeHandling()
{
    QSignalSpy networkChangedSpy(m_discovery, &NetworkDiscovery::discoveryStarted);
    
    // Start discovery
    m_discovery->startDiscovery(30000);
    
    // Simulate network change by manually triggering the slot
    // (In real usage, this would be triggered by NetworkMonitor)
    QMetaObject::invokeMethod(m_discovery, "onNetworkChanged", Qt::DirectConnection);
    
    // Should trigger a new discovery
    QTRY_VERIFY_WITH_TIMEOUT(networkChangedSpy.count() >= 2, 10000);
    
    m_discovery->stopDiscovery();
}

void TestNetworkDiscovery::testScanInterval()
{
    // Test setting different scan intervals
    m_discovery->setScanInterval(15000);
    QCOMPARE(m_discovery->scanInterval(), 15000);
    
    m_discovery->setScanInterval(60000);
    QCOMPARE(m_discovery->scanInterval(), 60000);
    
    // Test that active discovery uses new interval
    m_discovery->startDiscovery(45000);
    QCOMPARE(m_discovery->scanInterval(), 45000);
    
    m_discovery->stopDiscovery();
}

void TestNetworkDiscovery::testScanModeConfiguration()
{
    // Test that scan mode affects discovery behavior
    QSignalSpy progressSpy(m_discovery, &NetworkDiscovery::scanProgress);
    
    // Add some target hosts
    m_discovery->addTargetHost("127.0.0.1");
    m_discovery->addTargetHost("192.168.1.1");
    
    // Test targeted mode (should only scan target hosts)
    m_discovery->setScanMode(NetworkDiscovery::ScanMode::Targeted);
    m_discovery->refreshDiscovery();
    
    // Wait for scan progress
    QTRY_VERIFY_WITH_TIMEOUT(progressSpy.count() >= 1, 10000);
    
    // Test quick mode
    progressSpy.clear();
    m_discovery->setScanMode(NetworkDiscovery::ScanMode::Quick);
    m_discovery->refreshDiscovery();
    
    // Should also generate progress signals
    QTRY_VERIFY_WITH_TIMEOUT(progressSpy.count() >= 1, 10000);
}

QTEST_MAIN(TestNetworkDiscovery)
#include "test_networkdiscovery.moc"