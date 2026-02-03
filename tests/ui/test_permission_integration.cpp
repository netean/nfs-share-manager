#include <QtTest/QtTest>
#include <QApplication>

#include "../../src/core/permissionset.h"
#include "../../src/core/types.h"

using namespace NFSShareManager;

class TestPermissionIntegration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Test core permission functionality without GUI
    void testPermissionSetBasics();
    void testHostPermissions();
    void testUserPermissions();
    void testExportOptions();
    void testValidation();

private:
    QApplication *m_app;
};

void TestPermissionIntegration::initTestCase()
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

void TestPermissionIntegration::cleanupTestCase()
{
    delete m_app;
}

void TestPermissionIntegration::testPermissionSetBasics()
{
    PermissionSet permissions;
    
    // Test default values
    QCOMPARE(permissions.defaultAccess(), AccessMode::ReadOnly);
    QCOMPARE(permissions.enableRootSquash(), true);
    QCOMPARE(permissions.anonymousUser(), QString("nobody"));
    
    // Test setting values
    permissions.setDefaultAccess(AccessMode::ReadWrite);
    permissions.setEnableRootSquash(false);
    permissions.setAnonymousUser("testuser");
    
    QCOMPARE(permissions.defaultAccess(), AccessMode::ReadWrite);
    QCOMPARE(permissions.enableRootSquash(), false);
    QCOMPARE(permissions.anonymousUser(), QString("testuser"));
}

void TestPermissionIntegration::testHostPermissions()
{
    PermissionSet permissions;
    
    // Add host permissions
    permissions.setHostPermission("192.168.1.100", AccessMode::ReadWrite);
    permissions.setHostPermission("10.0.0.0/24", AccessMode::ReadOnly);
    permissions.setHostPermission("*.example.com", AccessMode::ReadWrite);
    
    // Test effective access
    QCOMPARE(permissions.getEffectiveAccessForHost("192.168.1.100"), AccessMode::ReadWrite);
    QCOMPARE(permissions.getEffectiveAccessForHost("10.0.0.0/24"), AccessMode::ReadOnly);
    QCOMPARE(permissions.getEffectiveAccessForHost("*.example.com"), AccessMode::ReadWrite);
    QCOMPARE(permissions.getEffectiveAccessForHost("unknown.host"), AccessMode::ReadOnly); // Default
    
    // Test removal
    permissions.removeHostPermission("192.168.1.100");
    QCOMPARE(permissions.getEffectiveAccessForHost("192.168.1.100"), AccessMode::ReadOnly); // Back to default
}

void TestPermissionIntegration::testUserPermissions()
{
    PermissionSet permissions;
    
    // Add user permissions
    permissions.setUserPermission("alice", AccessMode::ReadWrite);
    permissions.setUserPermission("bob", AccessMode::ReadOnly);
    permissions.setUserPermission("admin", AccessMode::ReadWrite);
    
    // Test effective access
    QCOMPARE(permissions.getEffectiveAccessForUser("alice"), AccessMode::ReadWrite);
    QCOMPARE(permissions.getEffectiveAccessForUser("bob"), AccessMode::ReadOnly);
    QCOMPARE(permissions.getEffectiveAccessForUser("admin"), AccessMode::ReadWrite);
    QCOMPARE(permissions.getEffectiveAccessForUser("unknown"), AccessMode::ReadOnly); // Default
    
    // Test removal
    permissions.removeUserPermission("alice");
    QCOMPARE(permissions.getEffectiveAccessForUser("alice"), AccessMode::ReadOnly); // Back to default
}

void TestPermissionIntegration::testExportOptions()
{
    PermissionSet permissions(AccessMode::ReadWrite);
    permissions.setEnableRootSquash(true);
    permissions.setAnonymousUser("nobody");
    
    QString exportOptions = permissions.toExportOptions();
    
    // Should contain expected options
    QVERIFY(exportOptions.contains("rw"));
    QVERIFY(exportOptions.contains("root_squash"));
    QVERIFY(exportOptions.contains("sync"));
    
    // Test parsing back
    PermissionSet parsedPermissions;
    QVERIFY(parsedPermissions.fromExportOptions(exportOptions));
    
    QCOMPARE(parsedPermissions.defaultAccess(), AccessMode::ReadWrite);
    QCOMPARE(parsedPermissions.enableRootSquash(), true);
}

void TestPermissionIntegration::testValidation()
{
    PermissionSet permissions;
    
    // Valid configuration
    QVERIFY(permissions.isValid());
    QVERIFY(permissions.validationErrors().isEmpty());
    
    // Test validation by creating a PermissionSet with invalid data
    // Since setAnonymousUser validates input, we need to test the validation
    // logic directly by checking what isValidUser returns
    
    // Test that empty user is considered invalid
    PermissionSet testPermissions;
    // We can't set invalid data through the public API, so let's test
    // the validation methods work correctly with edge cases
    
    // Test with a username that starts with a number (invalid)
    testPermissions.setAnonymousUser("123invalid"); // Should be rejected
    QCOMPARE(testPermissions.anonymousUser(), QString("nobody")); // Should remain default
    
    // Test with valid username
    testPermissions.setAnonymousUser("validuser");
    QCOMPARE(testPermissions.anonymousUser(), QString("validuser")); // Should be set
    
    QVERIFY(testPermissions.isValid());
    QVERIFY(testPermissions.validationErrors().isEmpty());
}

QTEST_MAIN(TestPermissionIntegration)
#include "test_permission_integration.moc"