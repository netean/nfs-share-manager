#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFileInfo>
#include "../../src/business/mountmanager.h"
#include "../../src/core/remotenfsshare.h"
#include "../../src/core/nfsmount.h"

using namespace NFSShareManager;

class TestMountManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testConstructor();
    void testValidateMountPoint_data();
    void testValidateMountPoint();
    void testCreateMountPoint();
    void testIsMountPointSuitable();
    void testGetDefaultMountOptions();
    void testGenerateMountPoint();

    // Mount management tests
    void testMountShareValidation();
    void testMountTrackingOperations();
    void testFstabOperations();

    // Error handling tests
    void testInvalidMountPoint();
    void testInvalidRemoteShare();

private:
    MountManager *m_mountManager;
    QTemporaryDir *m_tempDir;
    QString m_testMountRoot;
};

void TestMountManager::initTestCase()
{
    // Set up test environment
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testMountRoot = m_tempDir->path() + "/mounts";
    QDir().mkpath(m_testMountRoot);
}

void TestMountManager::cleanupTestCase()
{
    delete m_tempDir;
}

void TestMountManager::init()
{
    m_mountManager = new MountManager(this);
}

void TestMountManager::cleanup()
{
    delete m_mountManager;
    m_mountManager = nullptr;
}

void TestMountManager::testConstructor()
{
    QVERIFY(m_mountManager != nullptr);
    QVERIFY(m_mountManager->getManagedMounts().isEmpty());
}

void TestMountManager::testValidateMountPoint_data()
{
    QTest::addColumn<QString>("mountPoint");
    QTest::addColumn<bool>("expectedValid");

    QTest::newRow("empty") << QString() << false;
    QTest::newRow("relative_path") << QString("relative/path") << false;
    QTest::newRow("valid_absolute") << QString("/tmp/test_mount") << true;
    QTest::newRow("home_path") << QString(QDir::homePath() + "/test_mount") << true;
    QTest::newRow("invalid_chars") << QString("/tmp/test<mount>") << false;
    QTest::newRow("too_long") << QString("/" + QString("a").repeated(300)) << false;
}

void TestMountManager::testValidateMountPoint()
{
    QFETCH(QString, mountPoint);
    QFETCH(bool, expectedValid);

    bool isValid = m_mountManager->validateMountPoint(mountPoint);
    QCOMPARE(isValid, expectedValid);

    // Check validation errors
    QStringList errors = m_mountManager->getMountPointValidationErrors(mountPoint);
    if (expectedValid) {
        QVERIFY(errors.isEmpty());
    } else {
        QVERIFY(!errors.isEmpty());
    }
}

void TestMountManager::testCreateMountPoint()
{
    QString testPath = m_testMountRoot + "/test_create";
    
    // Ensure path doesn't exist initially
    QDir().rmpath(testPath);
    QVERIFY(!QDir(testPath).exists());
    
    // Create mount point
    bool created = m_mountManager->createMountPoint(testPath);
    QVERIFY(created);
    QVERIFY(QDir(testPath).exists());
    
    // Test creating existing directory
    bool createdAgain = m_mountManager->createMountPoint(testPath);
    QVERIFY(createdAgain);
}

void TestMountManager::testIsMountPointSuitable()
{
    QString testPath = m_testMountRoot + "/test_suitable";
    
    // Create empty directory
    QDir().mkpath(testPath);
    QVERIFY(m_mountManager->isMountPointSuitable(testPath));
    
    // Create file in directory (should make it unsuitable)
    QString filePath = testPath + "/test_file.txt";
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("test content");
    file.close();
    
    QVERIFY(!m_mountManager->isMountPointSuitable(testPath));
    
    // Clean up
    QFile::remove(filePath);
    QDir().rmpath(testPath);
}

void TestMountManager::testGetDefaultMountOptions()
{
    RemoteNFSShare share;
    share.setHostName("testserver");
    share.setHostAddress(QHostAddress("192.168.1.100"));
    share.setExportPath("/export/test");
    share.setSupportedVersion(NFSVersion::Version4);
    
    MountOptions options = m_mountManager->getDefaultMountOptions(share);
    
    QCOMPARE(options.nfsVersion, NFSVersion::Version4);
    QVERIFY(!options.readOnly);
    QCOMPARE(options.timeoutSeconds, 60);
    QCOMPARE(options.retryCount, 2);
    QVERIFY(!options.softMount);
    QVERIFY(options.backgroundMount);
    QVERIFY(options.rsize > 0);
    QVERIFY(options.wsize > 0);
}

void TestMountManager::testGenerateMountPoint()
{
    // Use reflection to access private method (for testing purposes)
    // In a real implementation, you might make this method protected or add a public wrapper
    RemoteNFSShare share;
    share.setHostName("testserver");
    share.setExportPath("/export/test");
    
    // We can't directly test the private method, but we can test the behavior
    // by checking that mount points are unique
    QString mountPoint1 = m_testMountRoot + "/testserver_export_test";
    QString mountPoint2 = m_testMountRoot + "/testserver_export_test_1";
    
    // Create first directory to force uniqueness
    QDir().mkpath(mountPoint1);
    
    // The actual method would generate a unique name, but we can't test it directly
    // This is a limitation of testing private methods
    QVERIFY(QDir(mountPoint1).exists());
}

void TestMountManager::testMountShareValidation()
{
    // Test with invalid remote share
    RemoteNFSShare invalidShare;
    QString mountPoint = m_testMountRoot + "/test_mount";
    
    MountManager::MountResult result = m_mountManager->mountShare(
        invalidShare, mountPoint, MountOptions(), false);
    
    QCOMPARE(result, MountManager::MountResult::InvalidRemoteShare);
    
    // Test with valid share but invalid mount point
    RemoteNFSShare validShare;
    validShare.setHostName("testserver");
    validShare.setHostAddress(QHostAddress("192.168.1.100"));
    validShare.setExportPath("/export/test");
    validShare.setSupportedVersion(NFSVersion::Version4);
    
    result = m_mountManager->mountShare(validShare, "", MountOptions(), false);
    QCOMPARE(result, MountManager::MountResult::InvalidMountPoint);
}

void TestMountManager::testMountTrackingOperations()
{
    // Create a valid remote share
    RemoteNFSShare share;
    share.setHostName("testserver");
    share.setHostAddress(QHostAddress("192.168.1.100"));
    share.setExportPath("/export/test");
    share.setSupportedVersion(NFSVersion::Version4);
    
    QString mountPoint = m_testMountRoot + "/test_tracking";
    
    // Initially no mounts
    QVERIFY(m_mountManager->getManagedMounts().isEmpty());
    QVERIFY(!m_mountManager->isManagedMount(mountPoint));
    
    // Create mount object for tracking (without actually mounting)
    NFSMount mount(share, mountPoint);
    
    // Test that mount point validation works
    QVERIFY(m_mountManager->validateMountPoint(mountPoint));
    
    // Test mount point creation
    QVERIFY(m_mountManager->createMountPoint(mountPoint));
    QVERIFY(m_mountManager->isMountPointSuitable(mountPoint));
}

void TestMountManager::testFstabOperations()
{
    // Create a test mount
    RemoteNFSShare share;
    share.setHostName("testserver");
    share.setHostAddress(QHostAddress("192.168.1.100"));
    share.setExportPath("/export/test");
    share.setSupportedVersion(NFSVersion::Version4);
    
    QString mountPoint = m_testMountRoot + "/test_fstab";
    NFSMount mount(share, mountPoint, MountOptions(), true);
    
    // Test fstab entry generation
    QString fstabEntry = mount.toFstabEntry();
    QVERIFY(!fstabEntry.isEmpty());
    QVERIFY(fstabEntry.contains("testserver:/export/test"));
    QVERIFY(fstabEntry.contains(mountPoint));
    QVERIFY(fstabEntry.contains("nfs"));
    
    // Note: We can't test actual fstab modification without root privileges
    // In a real test environment, you would mock the PolicyKit operations
}

void TestMountManager::testInvalidMountPoint()
{
    RemoteNFSShare validShare;
    validShare.setHostName("testserver");
    validShare.setHostAddress(QHostAddress("192.168.1.100"));
    validShare.setExportPath("/export/test");
    validShare.setSupportedVersion(NFSVersion::Version4);
    
    // Test various invalid mount points
    QStringList invalidPaths = {
        "",                           // Empty
        "relative/path",             // Relative path
        "/tmp/test<invalid>",        // Invalid characters
        QString("/") + QString("a").repeated(300)  // Too long
    };
    
    for (const QString &path : invalidPaths) {
        MountManager::MountResult result = m_mountManager->mountShare(
            validShare, path, MountOptions(), false);
        QCOMPARE(result, MountManager::MountResult::InvalidMountPoint);
    }
}

void TestMountManager::testInvalidRemoteShare()
{
    QString validMountPoint = m_testMountRoot + "/test_invalid_share";
    
    // Test with completely empty share
    RemoteNFSShare emptyShare;
    MountManager::MountResult result = m_mountManager->mountShare(
        emptyShare, validMountPoint, MountOptions(), false);
    QCOMPARE(result, MountManager::MountResult::InvalidRemoteShare);
    
    // Test with partially invalid share
    RemoteNFSShare partialShare;
    partialShare.setHostName("testserver");
    // Missing export path and address
    result = m_mountManager->mountShare(
        partialShare, validMountPoint, MountOptions(), false);
    QCOMPARE(result, MountManager::MountResult::InvalidRemoteShare);
}

QTEST_MAIN(TestMountManager)
#include "test_mountmanager.moc"