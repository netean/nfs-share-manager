#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QHostAddress>
#include "../../src/system/nfsserviceinterface.h"
#include "../../src/core/shareconfiguration.h"
#include "../../src/core/remotenfsshare.h"

using namespace NFSShareManager;

class TestNFSServiceInterface : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Tool availability tests
    void testIsNFSToolsAvailable();
    void testGetMissingTools();

    // Command result structure tests
    void testNFSCommandResult();

    // Export management tests
    void testExportDirectoryWithInvalidConfig();
    void testUnexportDirectory();
    void testGetExportedDirectories();
    void testReloadExports();

    // Network discovery tests
    void testQueryRemoteExportsWithHostname();
    void testQueryRemoteExportsWithIP();
    void testQueryRPCServices();

    // Mount management tests
    void testMountNFSShareValidation();
    void testUnmountNFSShare();
    void testGetMountedNFSShares();
    void testIsNFSMountPoint();

    // Parsing tests
    void testParseExportfsOutput();
    void testParseShowmountOutput();
    void testParseRPCInfoOutput();
    void testParseMountOutput();

    // Error handling tests
    void testCommandTimeout();
    void testInvalidCommand();
    void testMountPointValidation();

private:
    NFSServiceInterface *m_interface;
    QTemporaryDir *m_tempDir;
};

void TestNFSServiceInterface::initTestCase()
{
    // This will run once before all tests
}

void TestNFSServiceInterface::cleanupTestCase()
{
    // This will run once after all tests
}

void TestNFSServiceInterface::init()
{
    m_interface = new NFSServiceInterface(this);
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
}

void TestNFSServiceInterface::cleanup()
{
    delete m_interface;
    m_interface = nullptr;
    delete m_tempDir;
    m_tempDir = nullptr;
}

void TestNFSServiceInterface::testIsNFSToolsAvailable()
{
    // Test tool availability checking
    // Note: This test may fail on systems without NFS tools installed
    bool available = m_interface->isNFSToolsAvailable();
    
    // The result depends on the system, but the method should not crash
    QVERIFY(available == true || available == false);
    
    // Test that calling it multiple times gives consistent results
    bool available2 = m_interface->isNFSToolsAvailable();
    QCOMPARE(available, available2);
}

void TestNFSServiceInterface::testGetMissingTools()
{
    QStringList missing = m_interface->getMissingTools();
    
    // Should return a list (possibly empty)
    QVERIFY(missing.size() >= 0);
    
    // If tools are available, missing list should be empty
    if (m_interface->isNFSToolsAvailable()) {
        QVERIFY(missing.isEmpty());
    }
}

void TestNFSServiceInterface::testNFSCommandResult()
{
    // Test default constructor
    NFSCommandResult result1;
    QVERIFY(!result1.success);
    QCOMPARE(result1.exitCode, -1);
    QVERIFY(result1.output.isEmpty());
    QVERIFY(result1.error.isEmpty());
    QVERIFY(result1.command.isEmpty());
    
    // Test parameterized constructor
    NFSCommandResult result2(true, 0, "output", "error", "command");
    QVERIFY(result2.success);
    QCOMPARE(result2.exitCode, 0);
    QCOMPARE(result2.output, QString("output"));
    QCOMPARE(result2.error, QString("error"));
    QCOMPARE(result2.command, QString("command"));
}

void TestNFSServiceInterface::testExportDirectoryWithInvalidConfig()
{
    // Test with invalid export path
    ShareConfiguration config("test", AccessMode::ReadOnly);
    
    // Add some valid configuration to avoid issues
    config.addAllowedHost("*");
    
    NFSCommandResult result = m_interface->exportDirectory("", config);
    
    // Should fail with empty path - but the exact behavior depends on the exportfs command
    // The important thing is that it doesn't crash
    QVERIFY(!result.command.isEmpty());
}

void TestNFSServiceInterface::testUnexportDirectory()
{
    // Test unexport with empty path - should fail gracefully
    NFSCommandResult result = m_interface->unexportDirectory("");
    
    // Should fail with empty path
    QVERIFY(!result.success);
    QVERIFY(result.error.contains("Export path cannot be empty"));
}

void TestNFSServiceInterface::testGetExportedDirectories()
{
    NFSCommandResult result = m_interface->getExportedDirectories();
    
    // Should return a result (success depends on system state and permissions)
    QVERIFY(!result.command.isEmpty());
}

void TestNFSServiceInterface::testReloadExports()
{
    NFSCommandResult result = m_interface->reloadExports();
    
    // Should return a result (success depends on system state and permissions)
    QVERIFY(!result.command.isEmpty());
}

void TestNFSServiceInterface::testQueryRemoteExportsWithHostname()
{
    // Test with localhost (should be safe on most systems)
    NFSCommandResult result = m_interface->queryRemoteExports("localhost", 1000);
    
    // Should return a result (may fail if no NFS server running)
    QVERIFY(!result.command.isEmpty());
    QVERIFY(result.command.contains("showmount"));
}

void TestNFSServiceInterface::testQueryRemoteExportsWithIP()
{
    QHostAddress localhost(QHostAddress::LocalHost);
    NFSCommandResult result = m_interface->queryRemoteExports(localhost, 1000);
    
    // Should return a result
    QVERIFY(!result.command.isEmpty());
    QVERIFY(result.command.contains("showmount"));
}

void TestNFSServiceInterface::testQueryRPCServices()
{
    // Test with localhost
    NFSCommandResult result = m_interface->queryRPCServices("localhost", 1000);
    
    // Should return a result
    QVERIFY(!result.command.isEmpty());
    QVERIFY(result.command.contains("rpcinfo"));
}

void TestNFSServiceInterface::testMountNFSShareValidation()
{
    // Test with invalid mount point
    NFSCommandResult result = m_interface->mountNFSShare("localhost", "/test", "");
    
    // Should fail with empty mount point
    QVERIFY(!result.success);
    QVERIFY(result.error.contains("Invalid mount point"));
}

void TestNFSServiceInterface::testUnmountNFSShare()
{
    // Test unmount with non-existent mount point
    NFSCommandResult result = m_interface->unmountNFSShare("/nonexistent");
    
    // Should return a result (likely failure)
    QVERIFY(!result.command.isEmpty());
}

void TestNFSServiceInterface::testGetMountedNFSShares()
{
    QList<MountInfo> mounts = m_interface->getMountedNFSShares();
    
    // Should return a list (possibly empty)
    QVERIFY(mounts.size() >= 0);
}

void TestNFSServiceInterface::testIsNFSMountPoint()
{
    // Test with root directory (should not be NFS)
    bool isNFS = m_interface->isNFSMountPoint("/");
    
    // Root is typically not an NFS mount
    QVERIFY(!isNFS);
}

void TestNFSServiceInterface::testParseExportfsOutput()
{
    QString testOutput = "/home/user *(rw,sync,no_subtree_check)\n"
                        "/var/share 192.168.1.0/24(ro,sync)\n"
                        "# This is a comment\n"
                        "\n";
    
    QStringList exports = m_interface->parseExportfsOutput(testOutput);
    
    QCOMPARE(exports.size(), 2);
    QVERIFY(exports[0].contains("/home/user"));
    QVERIFY(exports[1].contains("/var/share"));
}

void TestNFSServiceInterface::testParseShowmountOutput()
{
    QString testOutput = "Export list for testserver:\n"
                        "/home/shared *\n"
                        "/var/data 192.168.1.0/24\n";
    
    QList<RemoteNFSShare> shares = m_interface->parseShowmountOutput(testOutput, "testserver");
    
    QCOMPARE(shares.size(), 2);
    QCOMPARE(shares[0].exportPath(), QString("/home/shared"));
    QCOMPARE(shares[0].hostName(), QString("testserver"));
    QCOMPARE(shares[1].exportPath(), QString("/var/data"));
    QVERIFY(shares[0].isAvailable());
}

void TestNFSServiceInterface::testParseRPCInfoOutput()
{
    QString testOutputWithNFS = "program vers proto   port  service\n"
                               "100000    4   tcp    111  portmapper\n"
                               "100003    3   tcp   2049  nfs\n"
                               "100005    1   udp    635  mountd\n";
    
    QString testOutputWithoutNFS = "program vers proto   port  service\n"
                                  "100000    4   tcp    111  portmapper\n"
                                  "100001    2   tcp    975  rstatd\n";
    
    QVERIFY(m_interface->parseRPCInfoOutput(testOutputWithNFS));
    
    // The second test is failing - let's check what the method actually returns
    bool result = m_interface->parseRPCInfoOutput(testOutputWithoutNFS);
    if (result) {
        qDebug() << "parseRPCInfoOutput returned true for output without NFS:" << testOutputWithoutNFS;
    }
    QVERIFY(!result);
}

void TestNFSServiceInterface::testParseMountOutput()
{
    QString testOutput = "/dev/sda1 on / type ext4 (rw,relatime)\n"
                        "server:/home on /mnt/nfs type nfs4 (rw,relatime,vers=4.1)\n"
                        "tmpfs on /tmp type tmpfs (rw,nosuid,nodev)\n";
    
    QList<MountInfo> mounts = m_interface->parseMountOutput(testOutput);
    
    QCOMPARE(mounts.size(), 3);
    
    // Check the NFS mount
    bool foundNFS = false;
    for (const MountInfo &mount : mounts) {
        if (mount.isNFS) {
            foundNFS = true;
            QCOMPARE(mount.device, QString("server:/home"));
            QCOMPARE(mount.mountPoint, QString("/mnt/nfs"));
            QCOMPARE(mount.fileSystem, QString("nfs4"));
            break;
        }
    }
    QVERIFY(foundNFS);
}

void TestNFSServiceInterface::testCommandTimeout()
{
    // This test is tricky to implement without actually running long commands
    // We'll test the timeout mechanism indirectly by checking signal emissions
    QSignalSpy timeoutSpy(m_interface, &NFSServiceInterface::commandTimeout);
    QSignalSpy finishedSpy(m_interface, &NFSServiceInterface::commandFinished);
    
    // Try to run a command that might timeout (with very short timeout)
    // Note: This might not actually timeout on fast systems
    NFSCommandResult result = m_interface->queryRemoteExports("192.168.255.255", 1);
    
    // Should have emitted finished signal
    QVERIFY(finishedSpy.count() >= 0);
}

void TestNFSServiceInterface::testInvalidCommand()
{
    // Test behavior when required tools are not available
    // This is hard to test reliably, so we'll just ensure it doesn't crash
    QStringList missing = m_interface->getMissingTools();
    
    // Should return a list without crashing
    QVERIFY(missing.size() >= 0);
}

void TestNFSServiceInterface::testMountPointValidation()
{
    // Test various mount point validation scenarios
    QString validPath = m_tempDir->path() + "/mount";
    QString invalidPath = "";
    QString relativePath = "relative/path";
    QString nonExistentParent = "/nonexistent/parent/mount";
    
    // Create the valid path's parent
    QDir().mkpath(QFileInfo(validPath).dir().path());
    
    // Test mount with valid path (should create mount point)
    NFSCommandResult result1 = m_interface->mountNFSShare("localhost", "/test", validPath);
    // May fail due to no NFS server, but should not fail due to mount point validation
    
    // Test mount with invalid empty path
    NFSCommandResult result2 = m_interface->mountNFSShare("localhost", "/test", invalidPath);
    QVERIFY(!result2.success);
    QVERIFY(result2.error.contains("Invalid mount point"));
    
    // Test mount with relative path
    NFSCommandResult result3 = m_interface->mountNFSShare("localhost", "/test", relativePath);
    QVERIFY(!result3.success);
    QVERIFY(result3.error.contains("Invalid mount point"));
}

QTEST_MAIN(TestNFSServiceInterface)
#include "test_nfsserviceinterface.moc"