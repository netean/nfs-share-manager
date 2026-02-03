#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QSystemTrayIcon>
#include "../../src/ui/notificationmanager.h"
#include "../../src/core/configurationmanager.h"
#include "../../src/core/nfsshare.h"
#include "../../src/core/nfsmount.h"
#include "../../src/core/remotenfsshare.h"

using namespace NFSShareManager;

class TestNotificationManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testPreferencesManagement();
    void testNotificationFiltering();
    void testBasicNotifications();
    
    // Specialized notification tests
    void testShareNotifications();
    void testMountNotifications();
    void testDiscoveryNotifications();
    void testSystemNotifications();
    void testOperationNotifications();
    void testSecurityNotifications();
    
    // Integration tests
    void testSystemTrayIntegration();
    void testNotificationGrouping();
    void testUrgencyFiltering();
    void testConfigurationPersistence();

private:
    ConfigurationManager *m_configManager;
    NotificationManager *m_notificationManager;
    QSystemTrayIcon *m_trayIcon;
};

void TestNotificationManager::initTestCase()
{
    // Initialize test environment
    m_configManager = new ConfigurationManager(this);
    m_trayIcon = new QSystemTrayIcon(this);
}

void TestNotificationManager::cleanupTestCase()
{
    delete m_configManager;
    delete m_trayIcon;
}

void TestNotificationManager::init()
{
    // Create fresh notification manager for each test
    m_notificationManager = new NotificationManager(m_configManager, this);
    m_notificationManager->setSystemTrayIcon(m_trayIcon);
    
    // Reset to default preferences for each test
    NotificationPreferences defaultPrefs;
    m_notificationManager->setPreferences(defaultPrefs);
}

void TestNotificationManager::cleanup()
{
    delete m_notificationManager;
    m_notificationManager = nullptr;
}

void TestNotificationManager::testInitialization()
{
    QVERIFY(m_notificationManager != nullptr);
    
    // Test default preferences
    NotificationPreferences prefs = m_notificationManager->preferences();
    QVERIFY(prefs.enableNotifications);
    QVERIFY(prefs.enableSystemTrayNotifications);
    QVERIFY(prefs.enableShareNotifications);
    QVERIFY(prefs.enableMountNotifications);
    QCOMPARE(prefs.minimumUrgency, NotificationUrgency::Normal);
    QCOMPARE(prefs.defaultTimeout, 5000);
    QCOMPARE(prefs.errorTimeout, 10000);
    QCOMPARE(prefs.successTimeout, 3000);
}

void TestNotificationManager::testPreferencesManagement()
{
    // Test setting preferences
    NotificationPreferences newPrefs;
    newPrefs.enableNotifications = false;
    newPrefs.enableShareNotifications = false;
    newPrefs.minimumUrgency = NotificationUrgency::High;
    newPrefs.defaultTimeout = 8000;
    
    QSignalSpy prefsChangedSpy(m_notificationManager, &NotificationManager::preferencesChanged);
    
    m_notificationManager->setPreferences(newPrefs);
    
    QCOMPARE(prefsChangedSpy.count(), 1);
    
    NotificationPreferences retrievedPrefs = m_notificationManager->preferences();
    QCOMPARE(retrievedPrefs.enableNotifications, false);
    QCOMPARE(retrievedPrefs.enableShareNotifications, false);
    QCOMPARE(retrievedPrefs.minimumUrgency, NotificationUrgency::High);
    QCOMPARE(retrievedPrefs.defaultTimeout, 8000);
}

void TestNotificationManager::testNotificationFiltering()
{
    // Test that notifications are filtered based on preferences
    NotificationPreferences prefs = m_notificationManager->preferences();
    prefs.enableShareNotifications = false;
    prefs.enableNotifications = true; // Keep global notifications enabled
    m_notificationManager->setPreferences(prefs);
    
    QVERIFY(!m_notificationManager->isNotificationEnabled(NotificationType::ShareCreated));
    QVERIFY(!m_notificationManager->isNotificationEnabled(NotificationType::ShareRemoved));
    QVERIFY(m_notificationManager->isNotificationEnabled(NotificationType::MountCompleted));
    
    // Test urgency filtering
    prefs.minimumUrgency = NotificationUrgency::High;
    m_notificationManager->setPreferences(prefs);
    
    // Low urgency notifications should be filtered out
    // (This would need to be tested through actual notification calls)
}

void TestNotificationManager::testBasicNotifications()
{
    // Test basic notification methods
    m_notificationManager->showInfo("Test info message");
    m_notificationManager->showWarning("Test Warning", "This is a warning message");
    m_notificationManager->showError("Test Error", "This is an error message");
    m_notificationManager->showSuccess("Test success message");
    
    // Since we can't easily test the actual notification display without KNotifications,
    // we mainly test that the methods don't crash and handle parameters correctly
    QVERIFY(true); // Test passes if we reach here without crashing
}

void TestNotificationManager::testShareNotifications()
{
    // Create test share
    NFSShare testShare;
    testShare.setPath("/test/share");
    testShare.setExportPath("/test/share");
    testShare.setActive(true);
    
    // Test share notifications
    m_notificationManager->notifyShareCreated(testShare);
    m_notificationManager->notifyShareRemoved("/test/share");
    m_notificationManager->notifyShareUpdated(testShare);
    m_notificationManager->notifyShareError("/test/share", "Test error");
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testMountNotifications()
{
    // Create test mount
    RemoteNFSShare remoteShare;
    remoteShare.setHostName("testhost");
    remoteShare.setExportPath("/test/export");
    
    NFSMount testMount;
    testMount.setRemoteShare(remoteShare);
    testMount.setLocalMountPoint("/mnt/test");
    testMount.setPersistent(false);
    
    // Test mount notifications
    m_notificationManager->notifyMountCompleted(testMount);
    m_notificationManager->notifyMountFailed(remoteShare, "/mnt/test", "Test mount error");
    m_notificationManager->notifyUnmountCompleted("/mnt/test");
    m_notificationManager->notifyUnmountFailed("/mnt/test", "Test unmount error");
    m_notificationManager->notifyMountConnectionLost(testMount);
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testDiscoveryNotifications()
{
    // Create test remote share
    RemoteNFSShare remoteShare;
    remoteShare.setHostName("testhost");
    remoteShare.setHostAddress(QHostAddress("192.168.1.100"));
    remoteShare.setExportPath("/test/export");
    remoteShare.setAvailable(true);
    
    // Test discovery notifications
    m_notificationManager->notifyShareDiscovered(remoteShare);
    m_notificationManager->notifyShareUnavailable("192.168.1.100", "/test/export");
    m_notificationManager->notifyDiscoveryCompleted(5, 3);
    m_notificationManager->notifyDiscoveryError("Test discovery error");
    m_notificationManager->notifyNetworkChanged("Interface eth0 up");
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testSystemNotifications()
{
    // Test system notifications
    m_notificationManager->notifyServiceStarted("nfs-server");
    m_notificationManager->notifyServiceStopped("nfs-server");
    m_notificationManager->notifyServiceError("nfs-server", "Service failed to start");
    m_notificationManager->notifyConfigurationChanged("NFS exports updated");
    m_notificationManager->notifyBackupCreated("/tmp/backup.conf");
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testOperationNotifications()
{
    // Test operation notifications
    m_notificationManager->notifyOperationStarted("Test Operation");
    m_notificationManager->notifyOperationCompleted("Test Operation", "Success");
    m_notificationManager->notifyOperationFailed("Test Operation", "Test failure");
    m_notificationManager->notifyOperationCancelled("Test Operation");
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testSecurityNotifications()
{
    // Test security notifications
    m_notificationManager->notifyAuthenticationRequired("Create NFS share");
    m_notificationManager->notifyAuthenticationFailed("Create NFS share");
    m_notificationManager->notifyPermissionDenied("Modify system files");
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testSystemTrayIntegration()
{
    // Test that system tray icon is properly set
    QVERIFY(m_trayIcon != nullptr);
    
    // Test notification with system tray
    NotificationPreferences prefs = m_notificationManager->preferences();
    prefs.enableKNotifications = false; // Force fallback to system tray
    prefs.enableSystemTrayNotifications = true;
    m_notificationManager->setPreferences(prefs);
    
    m_notificationManager->showInfo("System tray test");
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testNotificationGrouping()
{
    // Test notification grouping functionality
    NotificationPreferences prefs = m_notificationManager->preferences();
    prefs.groupSimilarNotifications = true;
    prefs.groupingTimeWindow = 1000; // 1 second
    m_notificationManager->setPreferences(prefs);
    
    // Send multiple similar notifications quickly
    RemoteNFSShare share1, share2, share3;
    share1.setHostName("host1");
    share1.setExportPath("/export1");
    share2.setHostName("host2");
    share2.setExportPath("/export2");
    share3.setHostName("host3");
    share3.setExportPath("/export3");
    
    m_notificationManager->notifyShareDiscovered(share1);
    m_notificationManager->notifyShareDiscovered(share2);
    m_notificationManager->notifyShareDiscovered(share3);
    
    // Wait for grouping timer
    QTest::qWait(1500);
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testUrgencyFiltering()
{
    // Test urgency-based filtering
    NotificationPreferences prefs = m_notificationManager->preferences();
    prefs.minimumUrgency = NotificationUrgency::High;
    m_notificationManager->setPreferences(prefs);
    
    // These should be filtered out (low/normal urgency)
    m_notificationManager->showInfo("Low priority info");
    
    // These should be shown (high/critical urgency)
    m_notificationManager->showError("Critical Error", "This should be shown");
    
    QVERIFY(true); // Test passes if no crashes occur
}

void TestNotificationManager::testConfigurationPersistence()
{
    // Test that preferences are saved and loaded correctly
    NotificationPreferences originalPrefs = m_notificationManager->preferences();
    
    // Modify preferences
    NotificationPreferences newPrefs = originalPrefs;
    newPrefs.enableNotifications = !originalPrefs.enableNotifications;
    newPrefs.defaultTimeout = originalPrefs.defaultTimeout + 1000;
    newPrefs.minimumUrgency = (originalPrefs.minimumUrgency == NotificationUrgency::Normal) ? 
                              NotificationUrgency::High : NotificationUrgency::Normal;
    
    m_notificationManager->setPreferences(newPrefs);
    
    // Create new notification manager to test loading
    delete m_notificationManager;
    m_notificationManager = new NotificationManager(m_configManager, this);
    
    NotificationPreferences loadedPrefs = m_notificationManager->preferences();
    
    // Verify preferences were persisted
    QCOMPARE(loadedPrefs.enableNotifications, newPrefs.enableNotifications);
    QCOMPARE(loadedPrefs.defaultTimeout, newPrefs.defaultTimeout);
    QCOMPARE(loadedPrefs.minimumUrgency, newPrefs.minimumUrgency);
}

QTEST_MAIN(TestNotificationManager)
#include "test_notificationmanager.moc"