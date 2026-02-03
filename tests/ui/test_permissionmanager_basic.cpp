#include <QtTest/QtTest>
#include <QApplication>

#include "../../src/ui/permissionmanagerdialog.h"
#include "../../src/core/permissionset.h"
#include "../../src/core/types.h"

using namespace NFSShareManager;

class TestPermissionManagerBasic : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Basic functionality tests without GUI interaction
    void testPermissionSetAndGet();
    void testValidation();
    void testHostValidationFunction();
    void testUserValidationFunction();

private:
    QApplication *m_app;
};

void TestPermissionManagerBasic::initTestCase()
{
    // QApplication is needed for GUI tests
    if (!QApplication::instance()) {
        int argc = 0;
        char **argv = nullptr;
        m_app = new QApplication(argc, argv);
    } else {
        m_app = nullptr;
    }
}

void TestPermissionManagerBasic::cleanupTestCase()
{
    delete m_app;
}

void TestPermissionManagerBasic::testPermissionSetAndGet()
{
    PermissionManagerDialog dialog;
    
    // Create a test permission set
    PermissionSet testPermissions(AccessMode::ReadWrite);
    testPermissions.setEnableRootSquash(false);
    testPermissions.setAnonymousUser("testuser");
    testPermissions.setHostPermission("192.168.1.100", AccessMode::ReadOnly);
    testPermissions.setUserPermission("alice", AccessMode::ReadWrite);
    
    // Set permissions in dialog
    dialog.setPermissions(testPermissions);
    
    // Get permissions back
    PermissionSet retrievedPermissions = dialog.getPermissions();
    
    // Verify they match
    QCOMPARE(retrievedPermissions.defaultAccess(), AccessMode::ReadWrite);
    QCOMPARE(retrievedPermissions.enableRootSquash(), false);
    QCOMPARE(retrievedPermissions.anonymousUser(), QString("testuser"));
    QCOMPARE(retrievedPermissions.getEffectiveAccessForHost("192.168.1.100"), AccessMode::ReadOnly);
    QCOMPARE(retrievedPermissions.getEffectiveAccessForUser("alice"), AccessMode::ReadWrite);
}

void TestPermissionManagerBasic::testValidation()
{
    PermissionManagerDialog dialog;
    
    // Test with valid default configuration
    QVERIFY(dialog.isValid());
    QVERIFY(dialog.getValidationErrors().isEmpty());
    
    // Test with invalid configuration
    PermissionSet invalidPermissions;
    invalidPermissions.setAnonymousUser(""); // Invalid empty user
    dialog.setPermissions(invalidPermissions);
    
    // Should be invalid now
    QVERIFY(!dialog.isValid());
    QVERIFY(!dialog.getValidationErrors().isEmpty());
}

void TestPermissionManagerBasic::testHostValidationFunction()
{
    PermissionManagerDialog dialog;
    
    // Test valid IP address
    auto result = dialog.validateHostSpecification("192.168.1.100");
    QVERIFY(result.first);
    QVERIFY(result.second.isEmpty());
    
    // Test valid hostname
    result = dialog.validateHostSpecification("example.com");
    QVERIFY(result.first);
    
    // Test valid network range
    result = dialog.validateHostSpecification("192.168.1.0/24");
    QVERIFY(result.first);
    
    // Test invalid host
    result = dialog.validateHostSpecification("invalid@host");
    QVERIFY(!result.first);
    QVERIFY(!result.second.isEmpty());
    
    // Test empty host
    result = dialog.validateHostSpecification("");
    QVERIFY(!result.first);
}

void TestPermissionManagerBasic::testUserValidationFunction()
{
    PermissionManagerDialog dialog;
    
    // Test valid username
    auto result = dialog.validateUserSpecification("alice");
    QVERIFY(result.first);
    QVERIFY(result.second.isEmpty());
    
    // Test valid username with underscore
    result = dialog.validateUserSpecification("test_user");
    QVERIFY(result.first);
    
    // Test invalid username (starts with number)
    result = dialog.validateUserSpecification("123invalid");
    QVERIFY(!result.first);
    QVERIFY(!result.second.isEmpty());
    
    // Test invalid username (contains @)
    result = dialog.validateUserSpecification("user@invalid");
    QVERIFY(!result.first);
    
    // Test empty username
    result = dialog.validateUserSpecification("");
    QVERIFY(!result.first);
}

QTEST_MAIN(TestPermissionManagerBasic)
#include "test_permissionmanager_basic.moc"