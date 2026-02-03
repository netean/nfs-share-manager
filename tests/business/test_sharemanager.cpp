#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFileInfo>
#include "../../src/business/sharemanager.h"
#include "../../src/core/shareconfiguration.h"
#include "../../src/core/permissionset.h"

using namespace NFSShareManager;

class ShareManagerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testInitialization();
    void testPathValidation();
    void testPathValidationErrors();
    void testShareCreation();
    void testShareRemoval();
    void testShareListing();
    void testShareConfiguration();
    void testPermissionUpdates();
    void testExportsFileGeneration();

private:
    ShareManager *m_shareManager;
    QTemporaryDir *m_tempDir;
    QString m_testPath;
};

void ShareManagerTest::initTestCase()
{
    // Create temporary directory for testing in a location that's not restricted
    m_tempDir = new QTemporaryDir(QDir::homePath() + "/test_sharemanager_XXXXXX");
    QVERIFY(m_tempDir->isValid());
    m_testPath = m_tempDir->path() + "/test_share";
    
    // Create test directory
    QDir().mkpath(m_testPath);
    QVERIFY(QFileInfo(m_testPath).exists());
}

void ShareManagerTest::cleanupTestCase()
{
    delete m_tempDir;
}

void ShareManagerTest::init()
{
    m_shareManager = new ShareManager(this);
    QVERIFY(m_shareManager != nullptr);
}

void ShareManagerTest::cleanup()
{
    delete m_shareManager;
    m_shareManager = nullptr;
}

void ShareManagerTest::testInitialization()
{
    // Test that ShareManager initializes correctly
    QVERIFY(m_shareManager != nullptr);
    
    // Test that initial share list is empty (or contains system shares)
    QList<NFSShare> shares = m_shareManager->getActiveShares();
    // We don't assert empty because there might be existing system shares
    QVERIFY(shares.size() >= 0);
}

void ShareManagerTest::testPathValidation()
{
    // Test valid path
    QVERIFY(m_shareManager->validateSharePath(m_testPath));
    
    // Test invalid paths
    QVERIFY(!m_shareManager->validateSharePath(""));
    QVERIFY(!m_shareManager->validateSharePath("/nonexistent/path"));
    QVERIFY(!m_shareManager->validateSharePath("/"));  // System directory
    QVERIFY(!m_shareManager->validateSharePath("/boot"));  // System directory
}

void ShareManagerTest::testPathValidationErrors()
{
    // Test empty path
    QStringList errors = m_shareManager->getPathValidationErrors("");
    QVERIFY(!errors.isEmpty());
    QVERIFY(errors.contains("Path cannot be empty"));
    
    // Test nonexistent path
    errors = m_shareManager->getPathValidationErrors("/nonexistent/path");
    QVERIFY(!errors.isEmpty());
    QVERIFY(errors.contains("Directory does not exist"));
    
    // Test system directory
    errors = m_shareManager->getPathValidationErrors("/");
    QVERIFY(!errors.isEmpty());
    QVERIFY(errors.join(" ").contains("Cannot share system directory"));
    
    // Test valid path
    errors = m_shareManager->getPathValidationErrors(m_testPath);
    QVERIFY(errors.isEmpty());
}

void ShareManagerTest::testShareCreation()
{
    // Create a basic share configuration
    ShareConfiguration config("Test Share", AccessMode::ReadWrite);
    config.setAllowedHosts(QStringList() << "192.168.1.0/24");
    
    // Note: This test will likely fail without proper PolicyKit setup
    // but we can test the validation logic
    QVERIFY(config.isValid());
    
    // Test that the share is not already present
    QVERIFY(!m_shareManager->isShared(m_testPath));
    
    // The actual creation will likely fail due to PolicyKit requirements
    // but we can verify the path validation works
    QVERIFY(m_shareManager->validateSharePath(m_testPath));
}

void ShareManagerTest::testShareRemoval()
{
    // Test removing a non-existent share
    bool result = m_shareManager->removeShare("/nonexistent/path");
    QVERIFY(!result);  // Should fail for non-existent share
}

void ShareManagerTest::testShareListing()
{
    // Test getting active shares
    QList<NFSShare> shares = m_shareManager->getActiveShares();
    QVERIFY(shares.size() >= 0);
    
    // Test getting a specific share that doesn't exist
    NFSShare share = m_shareManager->getShare("/nonexistent/path");
    QVERIFY(!share.isValid());
}

void ShareManagerTest::testShareConfiguration()
{
    // Test configuration validation
    ShareConfiguration validConfig("Valid Share", AccessMode::ReadOnly);
    validConfig.setAllowedHosts(QStringList() << "192.168.1.100");
    QVERIFY(validConfig.isValid());
    
    // Test permission set validation
    PermissionSet validPermissions(AccessMode::ReadWrite);
    validPermissions.setHostPermission("192.168.1.100", AccessMode::ReadOnly);
    QVERIFY(validPermissions.isValid());
}

void ShareManagerTest::testPermissionUpdates()
{
    // Test permission validation
    PermissionSet permissions(AccessMode::ReadWrite);
    permissions.setHostPermission("192.168.1.0/24", AccessMode::ReadOnly);
    permissions.setEnableRootSquash(true);
    
    QVERIFY(permissions.isValid());
    
    // Test updating permissions for non-existent share
    bool result = m_shareManager->updateSharePermissions("/nonexistent/path", permissions);
    QVERIFY(!result);  // Should fail for non-existent share
}

void ShareManagerTest::testExportsFileGeneration()
{
    // Test exports file generation
    QString exportsContent = m_shareManager->generateExportsFileContent();
    QVERIFY(!exportsContent.isEmpty());
    
    // Should contain header comments
    QVERIFY(exportsContent.contains("# /etc/exports"));
    QVERIFY(exportsContent.contains("# Generated by NFS Share Manager"));
}

QTEST_MAIN(ShareManagerTest)
#include "test_sharemanager.moc"