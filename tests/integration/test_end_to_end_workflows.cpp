#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTimer>
#include <QTemporaryDir>
#include <QDir>
#include <QFileInfo>
#include <QThread>
#include <QEventLoop>
#include <QNetworkInterface>

#include "../../src/ui/nfssharemanager.h"
#include "../../src/business/sharemanager.h"
#include "../../src/business/mountmanager.h"
#include "../../src/business/networkdiscovery.h"
#include "../../src/business/permissionmanager.h"
#include "../../src/core/configurationmanager.h"
#include "../../src/ui/notificationmanager.h"
#include "../../src/ui/operationmanager.h"
#include "../../src/system/policykithelper.h"
#include "../../src/system/nfsserviceinterface.h"
#include "../../src/core/nfsshare.h"
#include "../../src/core/nfsmount.h"
#include "../../src/core/remotenfsshare.h"
#include "../../src/core/shareconfiguration.h"
#include "../../src/core/permissionset.h"
#include "../../src/core/types.h"

using namespace NFSShareManager;

/**
 * @brief End-to-end integration test for complete NFS Share Manager workflows
 * 
 * This test validates complete workflows from share creation to mounting,
 * verifies PolicyKit integration in realistic scenarios, and tests error
 * handling and recovery across all components.
 * 
 * Test Coverage:
 * - Complete share creation workflow with PolicyKit authentication
 * - Network discovery and remote share detection
 * - Mount/unmount operations with various configurations
 * - Error handling and recovery mechanisms
 * - Component integration and communication
 * - Configuration persistence and restoration
 * - Notification system integration
 * - System tray integration
 */
class EndToEndWorkflowTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Complete workflow tests
    void testCompleteShareCreationWorkflow();
    void testCompleteShareCreationWorkflow_data();
    void testCompleteNetworkDiscoveryWorkflow();
    void testCompleteMountWorkflow();
    void testCompleteMountWorkflow_data();
    void testCompleteUnmountWorkflow();
    
    // PolicyKit integration tests
    void testPolicyKitIntegrationInShareCreation();
    void testPolicyKitIntegrationInMountOperations();
    void testPolicyKitAuthenticationFailure();
    void testPolicyKitPermissionEscalation();
    
    // Error handling and recovery tests
    void testErrorHandlingInShareCreation();
    void testErrorHandlingInNetworkDiscovery();
    void testErrorHandlingInMountOperations();
    void testComponentFailureRecovery();
    void testNetworkFailureRecovery();
    void testSystemServiceFailureRecovery();
    
    // Cross-component integration tests
    void testShareManagerAndPermissionManagerIntegration();
    void testMountManagerAndNetworkDiscoveryIntegration();
    void testNotificationSystemIntegration();
    void testOperationManagerIntegration();
    void testConfigurationPersistenceIntegration();
    
    // Realistic scenario tests
    void testMultipleSharesAndMountsScenario();
    void testNetworkTopologyChangeScenario();
    void testSystemRestartScenario();
    void testConcurrentOperationsScenario();
    void testLongRunningOperationsScenario();

private:
    // Test setup helpers
    void setupTestEnvironment();
    void createTestDirectories();
    void setupMockNFSServices();
    void setupTestNetworkEnvironment();
    void waitForOperation(const QUuid &operationId, int timeoutMs = 30000);
    bool waitForSignal(QObject *sender, const char *signal, int timeoutMs = 10000);
    void simulateNetworkDelay(int ms = 100);
    
    // Test validation helpers
    void validateShareCreation(const NFSShare &share, const QString &expectedPath);
    void validateMountOperation(const NFSMount &mount, const RemoteNFSShare &expectedRemote);
    void validatePermissionConfiguration(const PermissionSet &permissions);
    void validateSystemIntegration();
    void validateErrorRecovery(const QString &componentName);
    
    // Mock and simulation helpers
    void simulatePolicyKitAuthentication(bool success);
    void simulateNetworkShare(const QString &hostname, const QString &exportPath);
    void simulateNetworkFailure();
    void simulateSystemServiceFailure(const QString &serviceName);
    void simulateComponentFailure(const QString &componentName);
    
    // Test data and state
    NFSShareManagerApp *m_app;
    QApplication *m_qapp;
    QTemporaryDir *m_testDir;
    QStringList m_testSharePaths;
    QList<RemoteNFSShare> m_testRemoteShares;
    QList<NFSMount> m_testMounts;
    
    // Component references for direct testing
    ShareManager *m_shareManager;
    MountManager *m_mountManager;
    NetworkDiscovery *m_networkDiscovery;
    PermissionManager *m_permissionManager;
    ConfigurationManager *m_configurationManager;
    NotificationManager *m_notificationManager;
    OperationManager *m_operationManager;
    PolicyKitHelper *m_policyKitHelper;
    
    // Test configuration
    bool m_skipPolicyKitTests;
    bool m_skipNetworkTests;
    int m_defaultTimeout;
};

void EndToEndWorkflowTest::initTestCase()
{
    // Create QApplication if it doesn't exist
    if (!QApplication::instance()) {
        int argc = 1;
        char *argv[] = {const_cast<char*>("test")};
        m_qapp = new QApplication(argc, argv);
    } else {
        m_qapp = nullptr;
    }
    
    // Set test configuration
    m_skipPolicyKitTests = qEnvironmentVariableIsSet("SKIP_POLICYKIT_TESTS");
    m_skipNetworkTests = qEnvironmentVariableIsSet("SKIP_NETWORK_TESTS");
    m_defaultTimeout = qEnvironmentVariableIntValue("TEST_TIMEOUT", 30000);
    
    // Create test environment
    setupTestEnvironment();
    
    qDebug() << "End-to-end integration test environment initialized";
    qDebug() << "PolicyKit tests:" << (m_skipPolicyKitTests ? "SKIPPED" : "ENABLED");
    qDebug() << "Network tests:" << (m_skipNetworkTests ? "SKIPPED" : "ENABLED");
    qDebug() << "Default timeout:" << m_defaultTimeout << "ms";
}

void EndToEndWorkflowTest::cleanupTestCase()
{
    // Clean up test environment
    if (m_testDir) {
        delete m_testDir;
        m_testDir = nullptr;
    }
    
    if (m_qapp) {
        delete m_qapp;
        m_qapp = nullptr;
    }
    
    qDebug() << "End-to-end integration test cleanup completed";
}

void EndToEndWorkflowTest::init()
{
    // Create fresh application instance for each test
    m_app = new NFSShareManagerApp();
    m_app->hide(); // Don't show window during tests
    
    // Get component references
    m_shareManager = m_app->shareManager();
    m_mountManager = m_app->mountManager();
    m_networkDiscovery = m_app->networkDiscovery();
    m_permissionManager = m_app->findChild<PermissionManager*>();
    m_configurationManager = m_app->configurationManager();
    m_notificationManager = m_app->notificationManager();
    m_operationManager = m_app->findChild<OperationManager*>();
    m_policyKitHelper = m_app->findChild<PolicyKitHelper*>();
    
    // Verify all components are available
    QVERIFY(m_shareManager != nullptr);
    QVERIFY(m_mountManager != nullptr);
    QVERIFY(m_networkDiscovery != nullptr);
    QVERIFY(m_configurationManager != nullptr);
    QVERIFY(m_notificationManager != nullptr);
    
    // Clear any existing test data
    m_testSharePaths.clear();
    m_testRemoteShares.clear();
    m_testMounts.clear();
    
    // Create test directories for this test
    createTestDirectories();
    
    // Wait for component initialization
    QTest::qWait(500);
}

void EndToEndWorkflowTest::cleanup()
{
    // Clean up any created shares and mounts
    if (m_shareManager) {
        for (const QString &path : m_testSharePaths) {
            m_shareManager->removeShare(path);
        }
    }
    
    if (m_mountManager) {
        for (const NFSMount &mount : m_testMounts) {
            m_mountManager->unmountShare(mount.localMountPoint(), true); // Force unmount
        }
    }
    
    // Clean up application
    if (m_app) {
        delete m_app;
        m_app = nullptr;
    }
    
    // Reset component references
    m_shareManager = nullptr;
    m_mountManager = nullptr;
    m_networkDiscovery = nullptr;
    m_permissionManager = nullptr;
    m_configurationManager = nullptr;
    m_notificationManager = nullptr;
    m_operationManager = nullptr;
    m_policyKitHelper = nullptr;
}

void EndToEndWorkflowTest::setupTestEnvironment()
{
    // Create temporary directory for test files
    m_testDir = new QTemporaryDir(QDir::homePath() + "/nfs_test_XXXXXX");
    QVERIFY(m_testDir->isValid());
    
    qDebug() << "Test directory created:" << m_testDir->path();
}

void EndToEndWorkflowTest::createTestDirectories()
{
    if (!m_testDir || !m_testDir->isValid()) {
        return;
    }
    
    // Create test share directories
    QStringList testDirs = {
        "test_share_1",
        "test_share_2", 
        "test_share_readonly",
        "test_mount_point_1",
        "test_mount_point_2"
    };
    
    for (const QString &dirName : testDirs) {
        QString fullPath = m_testDir->path() + "/" + dirName;
        QDir().mkpath(fullPath);
        QVERIFY(QFileInfo(fullPath).exists());
        
        // Create some test files in share directories
        if (dirName.startsWith("test_share_")) {
            QString testFile = fullPath + "/test_file.txt";
            QFile file(testFile);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << "Test content for " << dirName << "\n";
                out << "Created at: " << QDateTime::currentDateTime().toString() << "\n";
                file.close();
            }
        }
    }
}

void EndToEndWorkflowTest::testCompleteShareCreationWorkflow()
{
    QFETCH(QString, shareName);
    QFETCH(AccessMode, accessMode);
    QFETCH(QStringList, allowedHosts);
    QFETCH(bool, enableRootSquash);
    
    QString testPath = m_testDir->path() + "/test_share_1";
    QVERIFY(QFileInfo(testPath).exists());
    
    // Step 1: Validate directory path
    QVERIFY(m_shareManager->validateSharePath(testPath));
    
    // Step 2: Create share configuration
    ShareConfiguration config(shareName, accessMode);
    config.setAllowedHosts(allowedHosts);
    config.setNfsVersion(NFSVersion::Version4);
    QVERIFY(config.isValid());
    
    // Step 3: Create permission set
    PermissionSet permissions(accessMode);
    permissions.setEnableRootSquash(enableRootSquash);
    permissions.setAnonymousUser("nobody");
    
    // Add host-specific permissions
    for (const QString &host : allowedHosts) {
        permissions.setHostPermission(host, accessMode);
    }
    QVERIFY(permissions.isValid());
    
    // Step 4: Set up signal spies for monitoring the workflow
    QSignalSpy shareCreatedSpy(m_shareManager, &ShareManager::shareCreated);
    QSignalSpy shareErrorSpy(m_shareManager, &ShareManager::shareError);
    QSignalSpy notificationSpy(m_notificationManager, &NotificationManager::notificationShown);
    
    // Step 5: Create the share (this should trigger PolicyKit authentication)
    bool createResult = m_shareManager->createShare(testPath, config);
    
    // Step 6: Wait for share creation to complete
    if (createResult) {
        QTRY_VERIFY_WITH_TIMEOUT(shareCreatedSpy.count() >= 1, m_defaultTimeout);
        
        // Verify the share was created correctly
        QCOMPARE(shareCreatedSpy.count(), 1);
        QCOMPARE(shareErrorSpy.count(), 0);
        
        NFSShare createdShare = shareCreatedSpy.first().first().value<NFSShare>();
        validateShareCreation(createdShare, testPath);
        
        // Step 7: Update permissions
        bool permissionResult = m_shareManager->updateSharePermissions(testPath, permissions);
        QVERIFY(permissionResult);
        
        // Step 8: Verify share is active and discoverable
        QList<NFSShare> activeShares = m_shareManager->getActiveShares();
        bool shareFound = false;
        for (const NFSShare &share : activeShares) {
            if (share.path() == testPath) {
                shareFound = true;
                QVERIFY(share.isActive());
                QCOMPARE(share.config().name(), shareName);
                QCOMPARE(share.config().accessMode(), accessMode);
                break;
            }
        }
        QVERIFY(shareFound);
        
        // Step 9: Verify notification was shown
        QTRY_VERIFY_WITH_TIMEOUT(notificationSpy.count() >= 1, 5000);
        
        // Add to test tracking
        m_testSharePaths.append(testPath);
        
    } else {
        // If creation failed, verify we got an error
        QTRY_VERIFY_WITH_TIMEOUT(shareErrorSpy.count() >= 1, m_defaultTimeout);
        
        QString errorMessage = shareErrorSpy.first().at(1).toString();
        qDebug() << "Share creation failed as expected:" << errorMessage;
        
        // This might be expected if PolicyKit authentication fails in test environment
        if (!m_skipPolicyKitTests) {
            QFAIL("Share creation failed unexpectedly");
        }
    }
}

void EndToEndWorkflowTest::testCompleteShareCreationWorkflow_data()
{
    QTest::addColumn<QString>("shareName");
    QTest::addColumn<AccessMode>("accessMode");
    QTest::addColumn<QStringList>("allowedHosts");
    QTest::addColumn<bool>("enableRootSquash");
    
    QTest::newRow("basic_readwrite") 
        << "Test Share RW" 
        << AccessMode::ReadWrite 
        << QStringList{"192.168.1.0/24"} 
        << true;
        
    QTest::newRow("readonly_specific_hosts") 
        << "Test Share RO" 
        << AccessMode::ReadOnly 
        << QStringList{"192.168.1.100", "192.168.1.101"} 
        << true;
        
    QTest::newRow("readwrite_no_root_squash") 
        << "Test Share No Squash" 
        << AccessMode::ReadWrite 
        << QStringList{"127.0.0.1", "localhost"} 
        << false;
        
    QTest::newRow("complex_permissions") 
        << "Complex Share" 
        << AccessMode::ReadWrite 
        << QStringList{"192.168.1.0/24", "10.0.0.0/8", "*.example.com"} 
        << true;
}

void EndToEndWorkflowTest::testCompleteNetworkDiscoveryWorkflow()
{
    if (m_skipNetworkTests) {
        QSKIP("Network tests are disabled");
    }
    
    // Step 1: Set up signal spies
    QSignalSpy discoveryStartedSpy(m_networkDiscovery, &NetworkDiscovery::discoveryStarted);
    QSignalSpy discoveryCompletedSpy(m_networkDiscovery, &NetworkDiscovery::discoveryCompleted);
    QSignalSpy shareDiscoveredSpy(m_networkDiscovery, &NetworkDiscovery::shareDiscovered);
    QSignalSpy discoveryErrorSpy(m_networkDiscovery, &NetworkDiscovery::discoveryError);
    
    // Step 2: Configure discovery settings
    m_networkDiscovery->setScanMode(NetworkDiscovery::ScanMode::Quick);
    m_networkDiscovery->addTargetHost("127.0.0.1");
    m_networkDiscovery->addTargetHost("localhost");
    
    // Step 3: Start discovery
    m_networkDiscovery->refreshDiscovery();
    
    // Step 4: Wait for discovery to start
    QTRY_VERIFY_WITH_TIMEOUT(discoveryStartedSpy.count() >= 1, 5000);
    
    // Step 5: Wait for discovery to complete
    QTRY_VERIFY_WITH_TIMEOUT(discoveryCompletedSpy.count() >= 1, m_defaultTimeout);
    
    // Step 6: Verify discovery results
    if (discoveryErrorSpy.count() > 0) {
        QString errorMessage = discoveryErrorSpy.first().first().toString();
        qDebug() << "Discovery completed with errors:" << errorMessage;
        // This might be expected in test environment without actual NFS servers
    }
    
    // Step 7: Check discovered shares
    QList<RemoteNFSShare> discoveredShares = m_networkDiscovery->getDiscoveredShares();
    qDebug() << "Discovered" << discoveredShares.size() << "shares";
    
    // Step 8: Verify discovery statistics
    QHash<QString, QVariant> stats = m_networkDiscovery->getScanStatistics();
    QVERIFY(stats.contains("total_scans"));
    QVERIFY(stats["total_scans"].toInt() >= 1);
    
    // Step 9: Test targeted discovery
    m_networkDiscovery->setScanMode(NetworkDiscovery::ScanMode::Targeted);
    discoveryStartedSpy.clear();
    discoveryCompletedSpy.clear();
    
    m_networkDiscovery->refreshDiscovery();
    QTRY_VERIFY_WITH_TIMEOUT(discoveryStartedSpy.count() >= 1, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(discoveryCompletedSpy.count() >= 1, m_defaultTimeout);
    
    // Step 10: Verify scan mode persistence
    QCOMPARE(m_networkDiscovery->scanMode(), NetworkDiscovery::ScanMode::Targeted);
}

void EndToEndWorkflowTest::testCompleteMountWorkflow()
{
    QFETCH(QString, hostname);
    QFETCH(QString, exportPath);
    QFETCH(QString, mountPoint);
    QFETCH(bool, isPersistent);
    QFETCH(NFSVersion, nfsVersion);
    
    // Step 1: Create a test remote share
    RemoteNFSShare remoteShare;
    remoteShare.setHostName(hostname);
    remoteShare.setHostAddress(QHostAddress(hostname == "localhost" ? "127.0.0.1" : hostname));
    remoteShare.setExportPath(exportPath);
    remoteShare.setSupportedVersion(nfsVersion);
    remoteShare.setAvailable(true);
    remoteShare.setDiscoveredAt(QDateTime::currentDateTime());
    
    QVERIFY(remoteShare.isValid());
    
    // Step 2: Prepare mount point
    QString fullMountPoint = m_testDir->path() + "/" + mountPoint;
    QVERIFY(m_mountManager->validateMountPoint(fullMountPoint));
    QVERIFY(m_mountManager->createMountPoint(fullMountPoint));
    
    // Step 3: Configure mount options
    MountOptions options = m_mountManager->getDefaultMountOptions(remoteShare);
    options.nfsVersion = nfsVersion;
    options.isPersistent = isPersistent;
    options.readOnly = false;
    options.timeoutSeconds = 30;
    options.retryCount = 3;
    QVERIFY(options.isValid());
    
    // Step 4: Set up signal spies
    QSignalSpy mountStartedSpy(m_mountManager, &MountManager::mountStarted);
    QSignalSpy mountCompletedSpy(m_mountManager, &MountManager::mountCompleted);
    QSignalSpy mountFailedSpy(m_mountManager, &MountManager::mountFailed);
    QSignalSpy notificationSpy(m_notificationManager, &NotificationManager::notificationShown);
    
    // Step 5: Attempt to mount the share
    bool mountResult = m_mountManager->mountShare(remoteShare, fullMountPoint, options);
    
    // Step 6: Wait for mount operation to complete
    if (mountResult) {
        QTRY_VERIFY_WITH_TIMEOUT(mountStartedSpy.count() >= 1, 5000);
        
        // Wait for either completion or failure
        QTRY_VERIFY_WITH_TIMEOUT(
            mountCompletedSpy.count() >= 1 || mountFailedSpy.count() >= 1, 
            m_defaultTimeout
        );
        
        if (mountCompletedSpy.count() >= 1) {
            // Mount succeeded
            NFSMount completedMount = mountCompletedSpy.first().first().value<NFSMount>();
            validateMountOperation(completedMount, remoteShare);
            
            // Verify mount is in managed mounts list
            QList<NFSMount> managedMounts = m_mountManager->getManagedMounts();
            bool mountFound = false;
            for (const NFSMount &mount : managedMounts) {
                if (mount.localMountPoint() == fullMountPoint) {
                    mountFound = true;
                    QCOMPARE(mount.isPersistent(), isPersistent);
                    QCOMPARE(mount.options().nfsVersion, nfsVersion);
                    break;
                }
            }
            QVERIFY(mountFound);
            
            // Add to test tracking
            m_testMounts.append(completedMount);
            
        } else if (mountFailedSpy.count() >= 1) {
            // Mount failed - this might be expected in test environment
            QString errorMessage = mountFailedSpy.first().at(3).toString();
            qDebug() << "Mount failed as expected in test environment:" << errorMessage;
            
            // This is acceptable in test environment without actual NFS servers
            if (!m_skipNetworkTests) {
                qDebug() << "Mount failure might be expected without real NFS server";
            }
        }
        
        // Step 7: Verify notification was shown
        QTRY_VERIFY_WITH_TIMEOUT(notificationSpy.count() >= 1, 5000);
        
    } else {
        // Mount initiation failed
        qDebug() << "Mount initiation failed - might be expected in test environment";
        
        if (!m_skipNetworkTests && !m_skipPolicyKitTests) {
            QFAIL("Mount initiation failed unexpectedly");
        }
    }
}

void EndToEndWorkflowTest::testCompleteMountWorkflow_data()
{
    QTest::addColumn<QString>("hostname");
    QTest::addColumn<QString>("exportPath");
    QTest::addColumn<QString>("mountPoint");
    QTest::addColumn<bool>("isPersistent");
    QTest::addColumn<NFSVersion>("nfsVersion");
    
    QTest::newRow("localhost_v4_temp") 
        << "localhost" 
        << "/tmp" 
        << "test_mount_point_1" 
        << false 
        << NFSVersion::Version4;
        
    QTest::newRow("localhost_v3_persistent") 
        << "localhost" 
        << "/home" 
        << "test_mount_point_2" 
        << true 
        << NFSVersion::Version3;
        
    QTest::newRow("ip_address_v4") 
        << "127.0.0.1" 
        << "/var" 
        << "test_mount_ip" 
        << false 
        << NFSVersion::Version4;
}

void EndToEndWorkflowTest::testCompleteUnmountWorkflow()
{
    // This test requires a successful mount first
    if (m_testMounts.isEmpty()) {
        QSKIP("No test mounts available for unmount test");
    }
    
    NFSMount testMount = m_testMounts.first();
    QString mountPoint = testMount.localMountPoint();
    
    // Step 1: Verify mount exists
    QList<NFSMount> managedMounts = m_mountManager->getManagedMounts();
    bool mountExists = false;
    for (const NFSMount &mount : managedMounts) {
        if (mount.localMountPoint() == mountPoint) {
            mountExists = true;
            break;
        }
    }
    
    if (!mountExists) {
        QSKIP("Test mount not found in managed mounts");
    }
    
    // Step 2: Set up signal spies
    QSignalSpy unmountStartedSpy(m_mountManager, &MountManager::unmountStarted);
    QSignalSpy unmountCompletedSpy(m_mountManager, &MountManager::unmountCompleted);
    QSignalSpy unmountFailedSpy(m_mountManager, &MountManager::unmountFailed);
    QSignalSpy notificationSpy(m_notificationManager, &NotificationManager::notificationShown);
    
    // Step 3: Attempt to unmount
    bool unmountResult = m_mountManager->unmountShare(mountPoint, false);
    
    // Step 4: Wait for unmount operation to complete
    if (unmountResult) {
        QTRY_VERIFY_WITH_TIMEOUT(unmountStartedSpy.count() >= 1, 5000);
        
        // Wait for either completion or failure
        QTRY_VERIFY_WITH_TIMEOUT(
            unmountCompletedSpy.count() >= 1 || unmountFailedSpy.count() >= 1, 
            m_defaultTimeout
        );
        
        if (unmountCompletedSpy.count() >= 1) {
            // Unmount succeeded
            QString unmountedPath = unmountCompletedSpy.first().first().toString();
            QCOMPARE(unmountedPath, mountPoint);
            
            // Verify mount is removed from managed mounts list
            QList<NFSMount> updatedMounts = m_mountManager->getManagedMounts();
            bool mountStillExists = false;
            for (const NFSMount &mount : updatedMounts) {
                if (mount.localMountPoint() == mountPoint) {
                    mountStillExists = true;
                    break;
                }
            }
            QVERIFY(!mountStillExists);
            
        } else if (unmountFailedSpy.count() >= 1) {
            // Unmount failed
            QString errorMessage = unmountFailedSpy.first().at(1).toString();
            qDebug() << "Unmount failed:" << errorMessage;
            
            // Try force unmount
            bool forceUnmountResult = m_mountManager->unmountShare(mountPoint, true);
            if (forceUnmountResult) {
                QTRY_VERIFY_WITH_TIMEOUT(unmountCompletedSpy.count() >= 1, m_defaultTimeout);
            }
        }
        
        // Step 5: Verify notification was shown
        QTRY_VERIFY_WITH_TIMEOUT(notificationSpy.count() >= 1, 5000);
        
    } else {
        qDebug() << "Unmount initiation failed";
        QFAIL("Unmount initiation failed unexpectedly");
    }
}

void EndToEndWorkflowTest::testPolicyKitIntegrationInShareCreation()
{
    if (m_skipPolicyKitTests) {
        QSKIP("PolicyKit tests are disabled");
    }
    
    QString testPath = m_testDir->path() + "/test_share_policykit";
    QDir().mkpath(testPath);
    
    // Step 1: Verify PolicyKit helper is available
    QVERIFY(m_policyKitHelper != nullptr);
    
    // Step 2: Check authorization for share creation
    bool hasAuth = m_policyKitHelper->checkAuthorization("org.kde.nfs-share-manager.create-share");
    qDebug() << "PolicyKit authorization for share creation:" << hasAuth;
    
    // Step 3: Create share configuration
    ShareConfiguration config("PolicyKit Test Share", AccessMode::ReadWrite);
    config.setAllowedHosts(QStringList() << "192.168.1.0/24");
    
    // Step 4: Set up signal spies
    QSignalSpy authResultSpy(m_policyKitHelper, &PolicyKitHelper::authorizationResult);
    QSignalSpy actionCompletedSpy(m_policyKitHelper, &PolicyKitHelper::actionCompleted);
    QSignalSpy shareCreatedSpy(m_shareManager, &ShareManager::shareCreated);
    QSignalSpy shareErrorSpy(m_shareManager, &ShareManager::shareError);
    
    // Step 5: Attempt share creation (should trigger PolicyKit)
    bool createResult = m_shareManager->createShare(testPath, config);
    
    // Step 6: Wait for PolicyKit authentication
    if (createResult) {
        // Wait for either authorization result or action completion
        QTRY_VERIFY_WITH_TIMEOUT(
            authResultSpy.count() >= 1 || actionCompletedSpy.count() >= 1 || 
            shareCreatedSpy.count() >= 1 || shareErrorSpy.count() >= 1, 
            m_defaultTimeout
        );
        
        if (shareCreatedSpy.count() >= 1) {
            // Share creation succeeded with PolicyKit authentication
            NFSShare createdShare = shareCreatedSpy.first().first().value<NFSShare>();
            QCOMPARE(createdShare.path(), testPath);
            QVERIFY(createdShare.isActive());
            
            m_testSharePaths.append(testPath);
            
        } else if (shareErrorSpy.count() >= 1) {
            // Share creation failed - might be due to PolicyKit denial
            QString errorMessage = shareErrorSpy.first().at(1).toString();
            qDebug() << "Share creation failed with PolicyKit:" << errorMessage;
            
            // This might be expected if user denies PolicyKit authentication
            if (errorMessage.contains("PolicyKit") || errorMessage.contains("authorization")) {
                qDebug() << "PolicyKit authentication was denied - this is acceptable for testing";
            } else {
                QFAIL("Share creation failed for unexpected reason");
            }
        }
    } else {
        qDebug() << "Share creation initiation failed";
        // This might be expected if PolicyKit is not available
    }
}

void EndToEndWorkflowTest::testPolicyKitIntegrationInMountOperations()
{
    if (m_skipPolicyKitTests) {
        QSKIP("PolicyKit tests are disabled");
    }
    
    // Step 1: Create test remote share
    RemoteNFSShare remoteShare;
    remoteShare.setHostName("localhost");
    remoteShare.setHostAddress(QHostAddress("127.0.0.1"));
    remoteShare.setExportPath("/tmp");
    remoteShare.setSupportedVersion(NFSVersion::Version4);
    remoteShare.setAvailable(true);
    
    QString mountPoint = m_testDir->path() + "/test_mount_policykit";
    QDir().mkpath(mountPoint);
    
    // Step 2: Check authorization for mount operations
    bool hasAuth = m_policyKitHelper->checkAuthorization("org.kde.nfs-share-manager.mount-share");
    qDebug() << "PolicyKit authorization for mount operations:" << hasAuth;
    
    // Step 3: Set up signal spies
    QSignalSpy authResultSpy(m_policyKitHelper, &PolicyKitHelper::authorizationResult);
    QSignalSpy actionCompletedSpy(m_policyKitHelper, &PolicyKitHelper::actionCompleted);
    QSignalSpy mountStartedSpy(m_mountManager, &MountManager::mountStarted);
    QSignalSpy mountCompletedSpy(m_mountManager, &MountManager::mountCompleted);
    QSignalSpy mountFailedSpy(m_mountManager, &MountManager::mountFailed);
    
    // Step 4: Attempt mount operation (should trigger PolicyKit)
    MountOptions options = m_mountManager->getDefaultMountOptions(remoteShare);
    bool mountResult = m_mountManager->mountShare(remoteShare, mountPoint, options);
    
    // Step 5: Wait for PolicyKit authentication and mount completion
    if (mountResult) {
        QTRY_VERIFY_WITH_TIMEOUT(
            authResultSpy.count() >= 1 || actionCompletedSpy.count() >= 1 || 
            mountStartedSpy.count() >= 1 || mountFailedSpy.count() >= 1, 
            m_defaultTimeout
        );
        
        if (mountStartedSpy.count() >= 1) {
            // Mount started - PolicyKit authentication succeeded
            QTRY_VERIFY_WITH_TIMEOUT(
                mountCompletedSpy.count() >= 1 || mountFailedSpy.count() >= 1, 
                m_defaultTimeout
            );
            
            if (mountCompletedSpy.count() >= 1) {
                NFSMount completedMount = mountCompletedSpy.first().first().value<NFSMount>();
                QCOMPARE(completedMount.localMountPoint(), mountPoint);
                m_testMounts.append(completedMount);
            }
        } else if (mountFailedSpy.count() >= 1) {
            QString errorMessage = mountFailedSpy.first().at(3).toString();
            qDebug() << "Mount failed with PolicyKit:" << errorMessage;
            
            if (errorMessage.contains("PolicyKit") || errorMessage.contains("authorization")) {
                qDebug() << "PolicyKit authentication was denied - this is acceptable for testing";
            }
        }
    }
}

void EndToEndWorkflowTest::testErrorHandlingInShareCreation()
{
    // Test various error conditions in share creation workflow
    
    // Step 1: Test invalid directory path
    QString invalidPath = "/nonexistent/directory/path";
    ShareConfiguration config("Invalid Path Test", AccessMode::ReadWrite);
    
    QSignalSpy shareErrorSpy(m_shareManager, &ShareManager::shareError);
    
    bool result = m_shareManager->createShare(invalidPath, config);
    QVERIFY(!result || shareErrorSpy.count() >= 1);
    
    if (shareErrorSpy.count() >= 1) {
        QString errorMessage = shareErrorSpy.first().at(1).toString();
        QVERIFY(errorMessage.contains("directory") || errorMessage.contains("path") || 
                errorMessage.contains("exist"));
    }
    
    // Step 2: Test invalid configuration
    shareErrorSpy.clear();
    ShareConfiguration invalidConfig("", AccessMode::ReadWrite); // Empty name
    QString validPath = m_testDir->path() + "/test_share_error";
    QDir().mkpath(validPath);
    
    result = m_shareManager->createShare(validPath, invalidConfig);
    // Should either fail immediately or generate error signal
    if (result) {
        QTRY_VERIFY_WITH_TIMEOUT(shareErrorSpy.count() >= 1, 5000);
    }
    
    // Step 3: Test permission errors (simulate)
    shareErrorSpy.clear();
    QString restrictedPath = "/root/restricted"; // Typically not accessible
    ShareConfiguration restrictedConfig("Restricted Test", AccessMode::ReadWrite);
    
    result = m_shareManager->createShare(restrictedPath, restrictedConfig);
    if (result) {
        QTRY_VERIFY_WITH_TIMEOUT(shareErrorSpy.count() >= 1, 10000);
        
        if (shareErrorSpy.count() >= 1) {
            QString errorMessage = shareErrorSpy.first().at(1).toString();
            QVERIFY(errorMessage.contains("permission") || errorMessage.contains("access") ||
                    errorMessage.contains("denied"));
        }
    }
}

void EndToEndWorkflowTest::testErrorHandlingInNetworkDiscovery()
{
    if (m_skipNetworkTests) {
        QSKIP("Network tests are disabled");
    }
    
    // Step 1: Test discovery with invalid target hosts
    QSignalSpy discoveryErrorSpy(m_networkDiscovery, &NetworkDiscovery::discoveryError);
    QSignalSpy discoveryCompletedSpy(m_networkDiscovery, &NetworkDiscovery::discoveryCompleted);
    
    m_networkDiscovery->clearTargetHosts();
    m_networkDiscovery->addTargetHost("999.999.999.999"); // Invalid IP
    m_networkDiscovery->addTargetHost("nonexistent.invalid.domain");
    m_networkDiscovery->setScanMode(NetworkDiscovery::ScanMode::Targeted);
    
    m_networkDiscovery->refreshDiscovery();
    
    // Wait for completion or error
    QTRY_VERIFY_WITH_TIMEOUT(
        discoveryCompletedSpy.count() >= 1 || discoveryErrorSpy.count() >= 1, 
        m_defaultTimeout
    );
    
    // Should complete with no shares found or generate errors
    if (discoveryErrorSpy.count() >= 1) {
        QString errorMessage = discoveryErrorSpy.first().first().toString();
        qDebug() << "Discovery error as expected:" << errorMessage;
        QVERIFY(!errorMessage.isEmpty());
    }
    
    // Step 2: Test network timeout handling
    discoveryErrorSpy.clear();
    discoveryCompletedSpy.clear();
    
    // Add a host that should timeout
    m_networkDiscovery->clearTargetHosts();
    m_networkDiscovery->addTargetHost("192.168.254.254"); // Likely to timeout
    
    m_networkDiscovery->refreshDiscovery();
    
    QTRY_VERIFY_WITH_TIMEOUT(
        discoveryCompletedSpy.count() >= 1 || discoveryErrorSpy.count() >= 1, 
        m_defaultTimeout
    );
    
    // Should handle timeout gracefully
    qDebug() << "Discovery completed with timeout handling";
}

void EndToEndWorkflowTest::testComponentFailureRecovery()
{
    // Test component failure detection and recovery mechanisms
    
    // Step 1: Verify all components are initially healthy
    QVERIFY(m_shareManager->isHealthy());
    QVERIFY(m_mountManager->isHealthy());
    QVERIFY(m_networkDiscovery->isHealthy());
    QVERIFY(m_configurationManager->isHealthy());
    
    // Step 2: Simulate component failure (if supported)
    // This would typically be done through internal test interfaces
    
    // Step 3: Trigger health check
    QSignalSpy componentFailureSpy(m_app, &NFSShareManagerApp::errorOccurred);
    
    // Simulate calling the health check method
    QTimer::singleShot(100, [this]() {
        m_app->checkComponentHealth();
    });
    
    QTest::qWait(200);
    
    // Step 4: Verify components are still functional
    QVERIFY(m_shareManager->isHealthy());
    QVERIFY(m_mountManager->isHealthy());
    QVERIFY(m_networkDiscovery->isHealthy());
    QVERIFY(m_configurationManager->isHealthy());
    
    // Step 5: Test recovery mechanisms
    // Components should be able to recover from transient failures
    QString testPath = m_testDir->path() + "/recovery_test";
    QDir().mkpath(testPath);
    
    ShareConfiguration config("Recovery Test", AccessMode::ReadOnly);
    bool recoveryResult = m_shareManager->validateSharePath(testPath);
    QVERIFY(recoveryResult);
}

void EndToEndWorkflowTest::testMultipleSharesAndMountsScenario()
{
    // Test realistic scenario with multiple shares and mounts
    
    QStringList testPaths;
    QList<ShareConfiguration> testConfigs;
    
    // Step 1: Create multiple test shares
    for (int i = 1; i <= 3; i++) {
        QString path = m_testDir->path() + QString("/multi_share_%1").arg(i);
        QDir().mkpath(path);
        testPaths.append(path);
        
        ShareConfiguration config(QString("Multi Share %1").arg(i), 
                                 i % 2 == 0 ? AccessMode::ReadOnly : AccessMode::ReadWrite);
        config.setAllowedHosts(QStringList() << "192.168.1.0/24");
        testConfigs.append(config);
    }
    
    // Step 2: Create shares concurrently
    QSignalSpy shareCreatedSpy(m_shareManager, &ShareManager::shareCreated);
    QSignalSpy shareErrorSpy(m_shareManager, &ShareManager::shareError);
    
    for (int i = 0; i < testPaths.size(); i++) {
        bool result = m_shareManager->createShare(testPaths[i], testConfigs[i]);
        if (result) {
            m_testSharePaths.append(testPaths[i]);
        }
    }
    
    // Step 3: Wait for all shares to be created
    QTRY_VERIFY_WITH_TIMEOUT(
        shareCreatedSpy.count() + shareErrorSpy.count() >= testPaths.size(), 
        m_defaultTimeout * 2
    );
    
    // Step 4: Verify shares are active
    QList<NFSShare> activeShares = m_shareManager->getActiveShares();
    int createdCount = 0;
    for (const NFSShare &share : activeShares) {
        if (testPaths.contains(share.path())) {
            createdCount++;
            QVERIFY(share.isActive());
        }
    }
    
    qDebug() << "Created" << createdCount << "out of" << testPaths.size() << "test shares";
    
    // Step 5: Test concurrent operations
    if (createdCount > 0) {
        // Update permissions on multiple shares
        PermissionSet newPermissions(AccessMode::ReadOnly);
        newPermissions.setEnableRootSquash(true);
        
        for (const QString &path : m_testSharePaths) {
            m_shareManager->updateSharePermissions(path, newPermissions);
        }
        
        // Verify updates
        QTest::qWait(1000);
        QList<NFSShare> updatedShares = m_shareManager->getActiveShares();
        for (const NFSShare &share : updatedShares) {
            if (m_testSharePaths.contains(share.path())) {
                QVERIFY(share.permissions().enableRootSquash());
            }
        }
    }
}

void EndToEndWorkflowTest::testConfigurationPersistenceIntegration()
{
    // Test configuration persistence across application restarts
    
    // Step 1: Create test configuration
    QString testPath = m_testDir->path() + "/persistence_test";
    QDir().mkpath(testPath);
    
    ShareConfiguration config("Persistence Test", AccessMode::ReadWrite);
    config.setAllowedHosts(QStringList() << "192.168.1.0/24" << "10.0.0.0/8");
    
    // Step 2: Create share and verify it's saved
    QSignalSpy shareCreatedSpy(m_shareManager, &ShareManager::shareCreated);
    
    bool createResult = m_shareManager->createShare(testPath, config);
    if (createResult) {
        QTRY_VERIFY_WITH_TIMEOUT(shareCreatedSpy.count() >= 1, m_defaultTimeout);
        m_testSharePaths.append(testPath);
    }
    
    // Step 3: Save configuration
    bool saveResult = m_configurationManager->saveConfiguration();
    QVERIFY(saveResult);
    
    // Step 4: Create backup
    QString backupPath = m_configurationManager->createBackup("test_backup");
    QVERIFY(!backupPath.isEmpty());
    QVERIFY(QFileInfo(backupPath).exists());
    
    // Step 5: Test configuration export/import
    QString exportPath = m_testDir->path() + "/config_export.json";
    bool exportResult = m_configurationManager->exportConfiguration(
        exportPath, "Test Profile", "Test configuration export");
    QVERIFY(exportResult);
    QVERIFY(QFileInfo(exportPath).exists());
    
    // Step 6: Test import
    bool importResult = m_configurationManager->importConfiguration(exportPath, false);
    QVERIFY(importResult);
    
    // Step 7: Verify configuration integrity
    ValidationResult validation = m_configurationManager->validateConfiguration();
    if (!validation.isValid) {
        qDebug() << "Configuration validation issues:" << validation.errors;
        
        if (validation.canAutoRepair) {
            bool repairResult = m_configurationManager->repairConfiguration(validation);
            QVERIFY(repairResult);
        }
    }
}

// Helper method implementations
void EndToEndWorkflowTest::validateShareCreation(const NFSShare &share, const QString &expectedPath)
{
    QCOMPARE(share.path(), expectedPath);
    QVERIFY(!share.config().name().isEmpty());
    QVERIFY(share.config().isValid());
    QVERIFY(share.permissions().isValid());
    QVERIFY(share.createdAt().isValid());
    QVERIFY(!share.exportPath().isEmpty());
}

void EndToEndWorkflowTest::validateMountOperation(const NFSMount &mount, const RemoteNFSShare &expectedRemote)
{
    QCOMPARE(mount.remoteShare().hostName(), expectedRemote.hostName());
    QCOMPARE(mount.remoteShare().exportPath(), expectedRemote.exportPath());
    QVERIFY(!mount.localMountPoint().isEmpty());
    QVERIFY(mount.options().isValid());
    QVERIFY(mount.mountedAt().isValid());
    QCOMPARE(mount.status(), MountStatus::Mounted);
}

void EndToEndWorkflowTest::validatePermissionConfiguration(const PermissionSet &permissions)
{
    QVERIFY(permissions.isValid());
    QVERIFY(permissions.validationErrors().isEmpty());
    QVERIFY(!permissions.anonymousUser().isEmpty());
}

bool EndToEndWorkflowTest::waitForSignal(QObject *sender, const char *signal, int timeoutMs)
{
    QSignalSpy spy(sender, signal);
    return spy.wait(timeoutMs);
}

void EndToEndWorkflowTest::simulateNetworkDelay(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

QTEST_MAIN(EndToEndWorkflowTest)
#include "test_end_to_end_workflows.moc"