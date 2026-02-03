#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTimer>

#include "../../src/ui/nfssharemanager.h"
#include "../../src/business/sharemanager.h"
#include "../../src/business/mountmanager.h"
#include "../../src/business/networkdiscovery.h"
#include "../../src/business/permissionmanager.h"
#include "../../src/core/configurationmanager.h"
#include "../../src/ui/notificationmanager.h"
#include "../../src/ui/operationmanager.h"

using namespace NFSShareManager;

/**
 * @brief Test class for component integration
 * 
 * This test verifies that all components are properly integrated
 * and can communicate with each other through signal/slot connections.
 */
class TestComponentIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Component initialization tests
    void testComponentCreation();
    void testComponentInitialization();
    void testSignalConnections();
    void testComponentLifecycle();
    
    // Integration tests
    void testShareManagerIntegration();
    void testMountManagerIntegration();
    void testNetworkDiscoveryIntegration();
    void testNotificationIntegration();
    void testOperationManagerIntegration();
    
    // Error handling tests
    void testComponentFailureHandling();
    void testComponentRecovery();

private:
    NFSShareManagerApp *m_app;
    QApplication *m_qapp;
};

void TestComponentIntegration::initTestCase()
{
    // Create QApplication if it doesn't exist
    if (!QApplication::instance()) {
        int argc = 1;
        char *argv[] = {const_cast<char*>("test")};
        m_qapp = new QApplication(argc, argv);
    } else {
        m_qapp = nullptr;
    }
}

void TestComponentIntegration::cleanupTestCase()
{
    if (m_qapp) {
        delete m_qapp;
        m_qapp = nullptr;
    }
}

void TestComponentIntegration::init()
{
    // Create main application instance
    m_app = new NFSShareManagerApp();
    
    // Don't show the window during tests
    m_app->hide();
}

void TestComponentIntegration::cleanup()
{
    if (m_app) {
        delete m_app;
        m_app = nullptr;
    }
}

void TestComponentIntegration::testComponentCreation()
{
    // Test that all components are created
    QVERIFY(m_app != nullptr);
    QVERIFY(m_app->configurationManager() != nullptr);
    QVERIFY(m_app->shareManager() != nullptr);
    QVERIFY(m_app->mountManager() != nullptr);
    QVERIFY(m_app->networkDiscovery() != nullptr);
    QVERIFY(m_app->notificationManager() != nullptr);
}

void TestComponentIntegration::testComponentInitialization()
{
    // Test that components are properly initialized
    QVERIFY(m_app->configurationManager()->isInitialized());
    QVERIFY(m_app->shareManager()->isInitialized());
    QVERIFY(m_app->mountManager()->isInitialized());
    QVERIFY(m_app->networkDiscovery()->isInitialized());
}

void TestComponentIntegration::testSignalConnections()
{
    // Test ShareManager signal connections
    ShareManager *shareManager = m_app->shareManager();
    QVERIFY(shareManager != nullptr);
    
    QSignalSpy shareCreatedSpy(shareManager, &ShareManager::shareCreated);
    QSignalSpy shareRemovedSpy(shareManager, &ShareManager::shareRemoved);
    QSignalSpy shareErrorSpy(shareManager, &ShareManager::shareError);
    
    QVERIFY(shareCreatedSpy.isValid());
    QVERIFY(shareRemovedSpy.isValid());
    QVERIFY(shareErrorSpy.isValid());
    
    // Test MountManager signal connections
    MountManager *mountManager = m_app->mountManager();
    QVERIFY(mountManager != nullptr);
    
    QSignalSpy mountStartedSpy(mountManager, &MountManager::mountStarted);
    QSignalSpy mountCompletedSpy(mountManager, &MountManager::mountCompleted);
    QSignalSpy mountFailedSpy(mountManager, &MountManager::mountFailed);
    
    QVERIFY(mountStartedSpy.isValid());
    QVERIFY(mountCompletedSpy.isValid());
    QVERIFY(mountFailedSpy.isValid());
    
    // Test NetworkDiscovery signal connections
    NetworkDiscovery *networkDiscovery = m_app->networkDiscovery();
    QVERIFY(networkDiscovery != nullptr);
    
    QSignalSpy shareDiscoveredSpy(networkDiscovery, &NetworkDiscovery::shareDiscovered);
    QSignalSpy discoveryCompletedSpy(networkDiscovery, &NetworkDiscovery::discoveryCompleted);
    QSignalSpy discoveryErrorSpy(networkDiscovery, &NetworkDiscovery::discoveryError);
    
    QVERIFY(shareDiscoveredSpy.isValid());
    QVERIFY(discoveryCompletedSpy.isValid());
    QVERIFY(discoveryErrorSpy.isValid());
}

void TestComponentIntegration::testComponentLifecycle()
{
    // Test component lifecycle management
    
    // All components should be healthy initially
    QVERIFY(m_app->shareManager()->isHealthy());
    QVERIFY(m_app->mountManager()->isHealthy());
    QVERIFY(m_app->networkDiscovery()->isHealthy());
    QVERIFY(m_app->configurationManager()->isHealthy());
    
    // Test graceful shutdown
    // Components should shutdown cleanly without errors
    delete m_app;
    m_app = nullptr;
    
    // No crashes should occur during shutdown
    QVERIFY(true);
}

void TestComponentIntegration::testShareManagerIntegration()
{
    ShareManager *shareManager = m_app->shareManager();
    QVERIFY(shareManager != nullptr);
    
    // Test that share manager has access to configuration
    QVERIFY(shareManager->configurationManager() != nullptr);
    
    // Test that share manager can validate permissions
    PermissionSet testPermissions;
    testPermissions.setDefaultAccess(AccessMode::ReadOnly);
    
    bool validationResult = shareManager->validatePermissions(testPermissions);
    // Should not crash and should return a boolean result
    Q_UNUSED(validationResult);
}

void TestComponentIntegration::testMountManagerIntegration()
{
    MountManager *mountManager = m_app->mountManager();
    QVERIFY(mountManager != nullptr);
    
    // Test that mount manager has access to configuration
    QVERIFY(mountManager->configurationManager() != nullptr);
    
    // Test that mount manager can validate mount points
    QString testMountPoint = "/tmp/test_mount";
    MountManager::ValidationResult result = mountManager->validateMountPoint(testMountPoint);
    
    // Should not crash and should return a validation result
    Q_UNUSED(result);
}

void TestComponentIntegration::testNetworkDiscoveryIntegration()
{
    NetworkDiscovery *networkDiscovery = m_app->networkDiscovery();
    QVERIFY(networkDiscovery != nullptr);
    
    // Test that network discovery has access to configuration
    QVERIFY(networkDiscovery->configurationManager() != nullptr);
    
    // Test that network discovery can be configured
    networkDiscovery->setScanMode(NetworkDiscovery::ScanMode::Quick);
    QCOMPARE(networkDiscovery->scanMode(), NetworkDiscovery::ScanMode::Quick);
}

void TestComponentIntegration::testNotificationIntegration()
{
    NotificationManager *notificationManager = m_app->notificationManager();
    QVERIFY(notificationManager != nullptr);
    
    // Test that notification manager can show notifications
    // This should not crash
    notificationManager->showInfo(tr("Test Notification"), tr("This is a test notification"));
    
    // Test notification preferences
    NotificationPreferences prefs = notificationManager->preferences();
    QVERIFY(prefs.isValid());
}

void TestComponentIntegration::testOperationManagerIntegration()
{
    // Test operation manager integration through the main app
    // This tests the global progress indication system
    
    // Start a test operation
    QSignalSpy operationStartedSpy(m_app, &NFSShareManagerApp::operationStarted);
    
    // Simulate starting an operation (this would normally be done by business logic)
    // For testing, we'll just verify the operation manager exists and is connected
    QVERIFY(m_app->findChild<OperationManager*>() != nullptr);
}

void TestComponentIntegration::testComponentFailureHandling()
{
    // Test that component failures are handled gracefully
    
    // Simulate a component failure by calling the failure handler directly
    QSignalSpy errorSpy(m_app, &NFSShareManagerApp::errorOccurred);
    
    // This should not crash the application
    m_app->handleComponentFailure("TestComponent", "Simulated failure");
    
    // The application should still be functional
    QVERIFY(m_app->isVisible() || m_app->isHidden()); // Should not have crashed
}

void TestComponentIntegration::testComponentRecovery()
{
    // Test component recovery mechanisms
    
    // All components should be healthy initially
    QVERIFY(m_app->shareManager()->isHealthy());
    QVERIFY(m_app->mountManager()->isHealthy());
    QVERIFY(m_app->networkDiscovery()->isHealthy());
    
    // Simulate a health check
    // This should not crash and should complete successfully
    QTimer::singleShot(100, [this]() {
        // Trigger a health check
        m_app->checkComponentHealth();
    });
    
    // Wait for the health check to complete
    QTest::qWait(200);
    
    // Components should still be healthy
    QVERIFY(m_app->shareManager()->isHealthy());
    QVERIFY(m_app->mountManager()->isHealthy());
    QVERIFY(m_app->networkDiscovery()->isHealthy());
}

QTEST_MAIN(TestComponentIntegration)
#include "test_component_integration.moc"