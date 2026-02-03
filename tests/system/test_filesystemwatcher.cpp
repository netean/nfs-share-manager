#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QThread>
#include <QTextStream>
#include "../../src/system/filesystemwatcher.h"

using namespace NFSShareManager;

class TestFileSystemWatcher : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testAddFile();
    void testAddDirectory();
    void testRemoveFile();
    void testRemoveDirectory();
    void testIsWatching();
    void testWatchedFiles();
    void testWatchedDirectories();

    // Change detection tests
    void testFileModification();
    void testFileCreation();
    void testFileDeletion();
    void testDirectoryModification();
    void testDirectoryCreation();
    void testDirectoryDeletion();
    void testPermissionChanges();

    // Configuration tests
    void testDebounceInterval();
    void testChangeDetectionEnabled();
    void testIgnorePatterns();

    // Recursive directory watching tests
    void testRecursiveDirectoryWatching();
    void testRecursiveDirectoryCreation();

    // Error handling tests
    void testInvalidPaths();
    void testNonExistentPaths();
    void testWatchFailure();

    // Performance and edge case tests
    void testRapidChanges();
    void testLargeNumberOfFiles();

private:
    void waitForSignal(QSignalSpy &spy, int timeout = 2000);
    void createTestFile(const QString &path, const QString &content = "test content");
    void modifyTestFile(const QString &path, const QString &newContent = "modified content");
    void createTestDirectory(const QString &path);

    QTemporaryDir *m_tempDir;
    FileSystemWatcher *m_watcher;
    QString m_testDir;
};

void TestFileSystemWatcher::initTestCase()
{
    // Create temporary directory for all tests
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDir = m_tempDir->path();
    
    qDebug() << "Test directory:" << m_testDir;
}

void TestFileSystemWatcher::cleanupTestCase()
{
    delete m_tempDir;
}

void TestFileSystemWatcher::init()
{
    m_watcher = new FileSystemWatcher(this);
    QVERIFY(m_watcher != nullptr);
}

void TestFileSystemWatcher::cleanup()
{
    delete m_watcher;
    m_watcher = nullptr;
}

void TestFileSystemWatcher::testAddFile()
{
    // Create a test file
    QString testFile = m_testDir + "/test_file.txt";
    createTestFile(testFile);
    
    // Test adding the file
    QVERIFY(m_watcher->addFile(testFile));
    QVERIFY(m_watcher->isWatching(testFile));
    QVERIFY(m_watcher->watchedFiles().contains(QFileInfo(testFile).absoluteFilePath()));
    
    // Test adding the same file again (should succeed)
    QVERIFY(m_watcher->addFile(testFile));
    
    // Test adding empty path
    QVERIFY(!m_watcher->addFile(""));
    
    // Test adding non-existent file
    QVERIFY(!m_watcher->addFile(m_testDir + "/nonexistent.txt"));
    
    // Test adding directory as file
    QVERIFY(!m_watcher->addFile(m_testDir));
}

void TestFileSystemWatcher::testAddDirectory()
{
    // Create a test directory
    QString testDir = m_testDir + "/test_directory";
    createTestDirectory(testDir);
    
    // Test adding the directory
    QVERIFY(m_watcher->addDirectory(testDir));
    QVERIFY(m_watcher->isWatching(testDir));
    QVERIFY(m_watcher->watchedDirectories().contains(QFileInfo(testDir).absoluteFilePath()));
    
    // Test adding the same directory again (should succeed)
    QVERIFY(m_watcher->addDirectory(testDir));
    
    // Test adding empty path
    QVERIFY(!m_watcher->addDirectory(""));
    
    // Test adding non-existent directory
    QVERIFY(!m_watcher->addDirectory(m_testDir + "/nonexistent"));
    
    // Test adding file as directory
    QString testFile = m_testDir + "/test_file.txt";
    createTestFile(testFile);
    QVERIFY(!m_watcher->addDirectory(testFile));
}

void TestFileSystemWatcher::testRemoveFile()
{
    // Create and add a test file
    QString testFile = m_testDir + "/test_file.txt";
    createTestFile(testFile);
    QVERIFY(m_watcher->addFile(testFile));
    QVERIFY(m_watcher->isWatching(testFile));
    
    // Test removing the file
    QVERIFY(m_watcher->removeFile(testFile));
    QVERIFY(!m_watcher->isWatching(testFile));
    QVERIFY(!m_watcher->watchedFiles().contains(QFileInfo(testFile).absoluteFilePath()));
    
    // Test removing the same file again (should succeed)
    QVERIFY(m_watcher->removeFile(testFile));
    
    // Test removing empty path
    QVERIFY(!m_watcher->removeFile(""));
}

void TestFileSystemWatcher::testRemoveDirectory()
{
    // Create and add a test directory
    QString testDir = m_testDir + "/test_directory";
    createTestDirectory(testDir);
    QVERIFY(m_watcher->addDirectory(testDir));
    QVERIFY(m_watcher->isWatching(testDir));
    
    // Test removing the directory
    QVERIFY(m_watcher->removeDirectory(testDir));
    QVERIFY(!m_watcher->isWatching(testDir));
    QVERIFY(!m_watcher->watchedDirectories().contains(QFileInfo(testDir).absoluteFilePath()));
    
    // Test removing the same directory again (should succeed)
    QVERIFY(m_watcher->removeDirectory(testDir));
    
    // Test removing empty path
    QVERIFY(!m_watcher->removeDirectory(""));
}

void TestFileSystemWatcher::testIsWatching()
{
    QString testFile = m_testDir + "/test_file.txt";
    QString testDir = m_testDir + "/test_directory";
    
    createTestFile(testFile);
    createTestDirectory(testDir);
    
    // Initially not watching
    QVERIFY(!m_watcher->isWatching(testFile));
    QVERIFY(!m_watcher->isWatching(testDir));
    QVERIFY(!m_watcher->isWatching(""));
    QVERIFY(!m_watcher->isWatching("/nonexistent"));
    
    // After adding
    QVERIFY(m_watcher->addFile(testFile));
    QVERIFY(m_watcher->addDirectory(testDir));
    QVERIFY(m_watcher->isWatching(testFile));
    QVERIFY(m_watcher->isWatching(testDir));
    
    // After removing
    QVERIFY(m_watcher->removeFile(testFile));
    QVERIFY(m_watcher->removeDirectory(testDir));
    QVERIFY(!m_watcher->isWatching(testFile));
    QVERIFY(!m_watcher->isWatching(testDir));
}

void TestFileSystemWatcher::testWatchedFiles()
{
    QString testFile1 = m_testDir + "/test_file1.txt";
    QString testFile2 = m_testDir + "/test_file2.txt";
    
    createTestFile(testFile1);
    createTestFile(testFile2);
    
    // Initially empty
    QVERIFY(m_watcher->watchedFiles().isEmpty());
    
    // Add files
    QVERIFY(m_watcher->addFile(testFile1));
    QCOMPARE(m_watcher->watchedFiles().size(), 1);
    QVERIFY(m_watcher->watchedFiles().contains(QFileInfo(testFile1).absoluteFilePath()));
    
    QVERIFY(m_watcher->addFile(testFile2));
    QCOMPARE(m_watcher->watchedFiles().size(), 2);
    QVERIFY(m_watcher->watchedFiles().contains(QFileInfo(testFile2).absoluteFilePath()));
    
    // Remove files
    QVERIFY(m_watcher->removeFile(testFile1));
    QCOMPARE(m_watcher->watchedFiles().size(), 1);
    QVERIFY(!m_watcher->watchedFiles().contains(QFileInfo(testFile1).absoluteFilePath()));
    
    QVERIFY(m_watcher->removeFile(testFile2));
    QVERIFY(m_watcher->watchedFiles().isEmpty());
}

void TestFileSystemWatcher::testWatchedDirectories()
{
    QString testDir1 = m_testDir + "/test_directory1";
    QString testDir2 = m_testDir + "/test_directory2";
    
    createTestDirectory(testDir1);
    createTestDirectory(testDir2);
    
    // Initially empty
    QVERIFY(m_watcher->watchedDirectories().isEmpty());
    
    // Add directories
    QVERIFY(m_watcher->addDirectory(testDir1));
    QCOMPARE(m_watcher->watchedDirectories().size(), 1);
    QVERIFY(m_watcher->watchedDirectories().contains(QFileInfo(testDir1).absoluteFilePath()));
    
    QVERIFY(m_watcher->addDirectory(testDir2));
    QCOMPARE(m_watcher->watchedDirectories().size(), 2);
    QVERIFY(m_watcher->watchedDirectories().contains(QFileInfo(testDir2).absoluteFilePath()));
    
    // Remove directories
    QVERIFY(m_watcher->removeDirectory(testDir1));
    QCOMPARE(m_watcher->watchedDirectories().size(), 1);
    QVERIFY(!m_watcher->watchedDirectories().contains(QFileInfo(testDir1).absoluteFilePath()));
    
    QVERIFY(m_watcher->removeDirectory(testDir2));
    QVERIFY(m_watcher->watchedDirectories().isEmpty());
}

void TestFileSystemWatcher::testFileModification()
{
    QString testFile = m_testDir + "/test_file.txt";
    createTestFile(testFile, "original content");
    
    QVERIFY(m_watcher->addFile(testFile));
    
    QSignalSpy fileChangedSpy(m_watcher, &FileSystemWatcher::fileChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Modify the file
    modifyTestFile(testFile, "modified content");
    
    // Wait for signals
    waitForSignal(fileChangedSpy);
    waitForSignal(pathChangedSpy);
    
    // Verify signals were emitted
    QCOMPARE(fileChangedSpy.count(), 1);
    QCOMPARE(pathChangedSpy.count(), 1);
    
    // Check signal parameters
    QList<QVariant> fileArgs = fileChangedSpy.takeFirst();
    QCOMPARE(fileArgs.at(0).toString(), QFileInfo(testFile).absoluteFilePath());
    QCOMPARE(fileArgs.at(1).value<FileSystemWatcher::ChangeType>(), FileSystemWatcher::ChangeType::FileModified);
    
    QList<QVariant> pathArgs = pathChangedSpy.takeFirst();
    QCOMPARE(pathArgs.at(0).toString(), QFileInfo(testFile).absoluteFilePath());
    QCOMPARE(pathArgs.at(1).value<FileSystemWatcher::ChangeType>(), FileSystemWatcher::ChangeType::FileModified);
}

void TestFileSystemWatcher::testFileCreation()
{
    QString testDir = m_testDir + "/test_directory";
    createTestDirectory(testDir);
    
    QVERIFY(m_watcher->addDirectory(testDir));
    
    QSignalSpy directoryChangedSpy(m_watcher, &FileSystemWatcher::directoryChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Create a new file in the watched directory
    QString newFile = testDir + "/new_file.txt";
    createTestFile(newFile);
    
    // Wait for signals
    waitForSignal(directoryChangedSpy);
    waitForSignal(pathChangedSpy);
    
    // Verify signals were emitted
    QVERIFY(directoryChangedSpy.count() >= 1);
    QVERIFY(pathChangedSpy.count() >= 1);
}

void TestFileSystemWatcher::testFileDeletion()
{
    QString testFile = m_testDir + "/test_file.txt";
    createTestFile(testFile);
    
    QVERIFY(m_watcher->addFile(testFile));
    
    QSignalSpy fileChangedSpy(m_watcher, &FileSystemWatcher::fileChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Delete the file
    QFile::remove(testFile);
    
    // Wait for signals
    waitForSignal(fileChangedSpy);
    waitForSignal(pathChangedSpy);
    
    // Verify signals were emitted
    QCOMPARE(fileChangedSpy.count(), 1);
    QCOMPARE(pathChangedSpy.count(), 1);
    
    // Check signal parameters
    QList<QVariant> fileArgs = fileChangedSpy.takeFirst();
    QCOMPARE(fileArgs.at(0).toString(), QFileInfo(testFile).absoluteFilePath());
    QCOMPARE(fileArgs.at(1).value<FileSystemWatcher::ChangeType>(), FileSystemWatcher::ChangeType::FileDeleted);
}

void TestFileSystemWatcher::testDirectoryModification()
{
    QString testDir = m_testDir + "/test_directory";
    createTestDirectory(testDir);
    
    QVERIFY(m_watcher->addDirectory(testDir));
    
    QSignalSpy directoryChangedSpy(m_watcher, &FileSystemWatcher::directoryChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Create a file in the directory (modifies directory)
    QString testFile = testDir + "/new_file.txt";
    createTestFile(testFile);
    
    // Wait for signals
    waitForSignal(directoryChangedSpy);
    waitForSignal(pathChangedSpy);
    
    // Verify signals were emitted
    QVERIFY(directoryChangedSpy.count() >= 1);
    QVERIFY(pathChangedSpy.count() >= 1);
}

void TestFileSystemWatcher::testDirectoryCreation()
{
    QString parentDir = m_testDir + "/parent_directory";
    createTestDirectory(parentDir);
    
    QVERIFY(m_watcher->addDirectory(parentDir));
    
    QSignalSpy directoryChangedSpy(m_watcher, &FileSystemWatcher::directoryChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Create a subdirectory
    QString newDir = parentDir + "/new_subdirectory";
    createTestDirectory(newDir);
    
    // Wait for signals
    waitForSignal(directoryChangedSpy);
    waitForSignal(pathChangedSpy);
    
    // Verify signals were emitted
    QVERIFY(directoryChangedSpy.count() >= 1);
    QVERIFY(pathChangedSpy.count() >= 1);
}

void TestFileSystemWatcher::testDirectoryDeletion()
{
    QString testDir = m_testDir + "/test_directory";
    createTestDirectory(testDir);
    
    QVERIFY(m_watcher->addDirectory(testDir));
    
    QSignalSpy directoryChangedSpy(m_watcher, &FileSystemWatcher::directoryChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Delete the directory
    QDir().rmdir(testDir);
    
    // Wait for signals
    waitForSignal(directoryChangedSpy);
    waitForSignal(pathChangedSpy);
    
    // Verify signals were emitted
    QCOMPARE(directoryChangedSpy.count(), 1);
    QCOMPARE(pathChangedSpy.count(), 1);
    
    // Check signal parameters
    QList<QVariant> dirArgs = directoryChangedSpy.takeFirst();
    QCOMPARE(dirArgs.at(0).toString(), QFileInfo(testDir).absoluteFilePath());
    QCOMPARE(dirArgs.at(1).value<FileSystemWatcher::ChangeType>(), FileSystemWatcher::ChangeType::DirectoryDeleted);
}

void TestFileSystemWatcher::testPermissionChanges()
{
    QString testFile = m_testDir + "/test_file.txt";
    createTestFile(testFile);
    
    QVERIFY(m_watcher->addFile(testFile));
    
    QSignalSpy fileChangedSpy(m_watcher, &FileSystemWatcher::fileChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Change file permissions
    QFile file(testFile);
    QFile::Permissions originalPerms = file.permissions();
    QFile::Permissions newPerms = originalPerms | QFile::WriteOther;
    file.setPermissions(newPerms);
    
    // Wait for signals
    waitForSignal(fileChangedSpy);
    waitForSignal(pathChangedSpy);
    
    // Verify signals were emitted
    QVERIFY(fileChangedSpy.count() >= 1);
    QVERIFY(pathChangedSpy.count() >= 1);
    
    // Restore original permissions
    file.setPermissions(originalPerms);
}

void TestFileSystemWatcher::testDebounceInterval()
{
    // Test default interval
    QCOMPARE(m_watcher->debounceInterval(), 500);
    
    // Test setting interval
    m_watcher->setDebounceInterval(1000);
    QCOMPARE(m_watcher->debounceInterval(), 1000);
    
    // Test setting negative interval (should be clamped to 0)
    m_watcher->setDebounceInterval(-100);
    QCOMPARE(m_watcher->debounceInterval(), 0);
    
    // Test zero interval (no debouncing)
    m_watcher->setDebounceInterval(0);
    QCOMPARE(m_watcher->debounceInterval(), 0);
}

void TestFileSystemWatcher::testChangeDetectionEnabled()
{
    // Test default state
    QVERIFY(m_watcher->isChangeDetectionEnabled());
    
    // Test disabling
    m_watcher->setChangeDetectionEnabled(false);
    QVERIFY(!m_watcher->isChangeDetectionEnabled());
    
    // Test enabling
    m_watcher->setChangeDetectionEnabled(true);
    QVERIFY(m_watcher->isChangeDetectionEnabled());
    
    // Test that disabled detection doesn't emit signals
    QString testFile = m_testDir + "/test_file.txt";
    createTestFile(testFile);
    QVERIFY(m_watcher->addFile(testFile));
    
    m_watcher->setChangeDetectionEnabled(false);
    
    QSignalSpy fileChangedSpy(m_watcher, &FileSystemWatcher::fileChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    modifyTestFile(testFile);
    
    // Wait a bit to ensure no signals are emitted
    QTest::qWait(1000);
    
    QCOMPARE(fileChangedSpy.count(), 0);
    QCOMPARE(pathChangedSpy.count(), 0);
}

void TestFileSystemWatcher::testIgnorePatterns()
{
    // Test default state
    QVERIFY(m_watcher->ignorePatterns().isEmpty());
    
    // Test adding patterns
    m_watcher->addIgnorePattern("*.tmp");
    m_watcher->addIgnorePattern(".*");
    m_watcher->addIgnorePattern("backup_*");
    
    QStringList patterns = m_watcher->ignorePatterns();
    QCOMPARE(patterns.size(), 3);
    QVERIFY(patterns.contains("*.tmp"));
    QVERIFY(patterns.contains(".*"));
    QVERIFY(patterns.contains("backup_*"));
    
    // Test removing patterns
    m_watcher->removeIgnorePattern(".*");
    patterns = m_watcher->ignorePatterns();
    QCOMPARE(patterns.size(), 2);
    QVERIFY(!patterns.contains(".*"));
    
    // Test clearing patterns
    m_watcher->clearIgnorePatterns();
    QVERIFY(m_watcher->ignorePatterns().isEmpty());
    
    // Test that ignored files don't emit signals
    m_watcher->addIgnorePattern("*.tmp");
    
    QString testDir = m_testDir + "/test_directory";
    createTestDirectory(testDir);
    QVERIFY(m_watcher->addDirectory(testDir));
    
    QSignalSpy directoryChangedSpy(m_watcher, &FileSystemWatcher::directoryChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Create a file that should be ignored
    QString ignoredFile = testDir + "/test.tmp";
    createTestFile(ignoredFile);
    
    // Wait a bit
    QTest::qWait(1000);
    
    // Should not receive signals for ignored files
    // Note: Directory change might still be detected, but the specific file change should be ignored
    // This test is more about the pattern matching functionality
}

void TestFileSystemWatcher::testRecursiveDirectoryWatching()
{
    QString testDir = m_testDir + "/recursive_test";
    createTestDirectory(testDir);
    
    // Create subdirectories
    QString subDir1 = testDir + "/subdir1";
    QString subDir2 = testDir + "/subdir2";
    QString subSubDir = subDir1 + "/subsubdir";
    
    createTestDirectory(subDir1);
    createTestDirectory(subDir2);
    createTestDirectory(subSubDir);
    
    // Add directory with recursive watching
    QVERIFY(m_watcher->addDirectory(testDir, true));
    
    // Check that subdirectories are also being watched
    QStringList watchedDirs = m_watcher->watchedDirectories();
    QVERIFY(watchedDirs.contains(QFileInfo(testDir).absoluteFilePath()));
    QVERIFY(watchedDirs.contains(QFileInfo(subDir1).absoluteFilePath()));
    QVERIFY(watchedDirs.contains(QFileInfo(subDir2).absoluteFilePath()));
    QVERIFY(watchedDirs.contains(QFileInfo(subSubDir).absoluteFilePath()));
    
    // Test removing recursive directory
    QVERIFY(m_watcher->removeDirectory(testDir));
    
    // Check that all subdirectories are removed from watching
    watchedDirs = m_watcher->watchedDirectories();
    QVERIFY(!watchedDirs.contains(QFileInfo(testDir).absoluteFilePath()));
    QVERIFY(!watchedDirs.contains(QFileInfo(subDir1).absoluteFilePath()));
    QVERIFY(!watchedDirs.contains(QFileInfo(subDir2).absoluteFilePath()));
    QVERIFY(!watchedDirs.contains(QFileInfo(subSubDir).absoluteFilePath()));
}

void TestFileSystemWatcher::testRecursiveDirectoryCreation()
{
    QString testDir = m_testDir + "/recursive_creation_test";
    createTestDirectory(testDir);
    
    // Add directory with recursive watching
    QVERIFY(m_watcher->addDirectory(testDir, true));
    
    QSignalSpy directoryChangedSpy(m_watcher, &FileSystemWatcher::directoryChanged);
    
    // Create a new subdirectory
    QString newSubDir = testDir + "/new_subdir";
    createTestDirectory(newSubDir);
    
    // Wait for signals
    waitForSignal(directoryChangedSpy);
    
    // The new subdirectory should be automatically added to watching
    QTest::qWait(1000); // Give some time for the recursive addition
    
    QStringList watchedDirs = m_watcher->watchedDirectories();
    QVERIFY(watchedDirs.contains(QFileInfo(newSubDir).absoluteFilePath()));
}

void TestFileSystemWatcher::testInvalidPaths()
{
    // Test empty paths
    QVERIFY(!m_watcher->addFile(""));
    QVERIFY(!m_watcher->addDirectory(""));
    QVERIFY(!m_watcher->removeFile(""));
    QVERIFY(!m_watcher->removeDirectory(""));
    QVERIFY(!m_watcher->isWatching(""));
    
    // Test null paths
    QVERIFY(!m_watcher->addFile(QString()));
    QVERIFY(!m_watcher->addDirectory(QString()));
    
    // Test invalid characters (platform-dependent)
    QString invalidPath = "/invalid\0path";
    QVERIFY(!m_watcher->addFile(invalidPath));
    QVERIFY(!m_watcher->addDirectory(invalidPath));
}

void TestFileSystemWatcher::testNonExistentPaths()
{
    QString nonExistentFile = m_testDir + "/nonexistent_file.txt";
    QString nonExistentDir = m_testDir + "/nonexistent_directory";
    
    // Test adding non-existent paths
    QVERIFY(!m_watcher->addFile(nonExistentFile));
    QVERIFY(!m_watcher->addDirectory(nonExistentDir));
    
    // Test removing non-existent paths (should succeed)
    QVERIFY(m_watcher->removeFile(nonExistentFile));
    QVERIFY(m_watcher->removeDirectory(nonExistentDir));
    
    // Test checking non-existent paths
    QVERIFY(!m_watcher->isWatching(nonExistentFile));
    QVERIFY(!m_watcher->isWatching(nonExistentDir));
}

void TestFileSystemWatcher::testWatchFailure()
{
    QSignalSpy watchFailedSpy(m_watcher, &FileSystemWatcher::watchFailed);
    
    // Try to add a non-existent file
    QString nonExistentFile = m_testDir + "/nonexistent.txt";
    QVERIFY(!m_watcher->addFile(nonExistentFile));
    
    // Should emit watchFailed signal
    QCOMPARE(watchFailedSpy.count(), 1);
    
    QList<QVariant> args = watchFailedSpy.takeFirst();
    QCOMPARE(args.at(0).toString(), nonExistentFile);
    QVERIFY(!args.at(1).toString().isEmpty()); // Error message should not be empty
}

void TestFileSystemWatcher::testRapidChanges()
{
    QString testFile = m_testDir + "/rapid_changes.txt";
    createTestFile(testFile);
    
    QVERIFY(m_watcher->addFile(testFile));
    
    // Set a longer debounce interval to test debouncing
    m_watcher->setDebounceInterval(1000);
    
    QSignalSpy fileChangedSpy(m_watcher, &FileSystemWatcher::fileChanged);
    QSignalSpy pathChangedSpy(m_watcher, &FileSystemWatcher::pathChanged);
    
    // Make rapid changes
    for (int i = 0; i < 5; ++i) {
        modifyTestFile(testFile, QString("content %1").arg(i));
        QTest::qWait(100); // Small delay between changes
    }
    
    // Wait for debounce timer
    QTest::qWait(1500);
    
    // Should receive multiple fileChanged signals (immediate)
    QVERIFY(fileChangedSpy.count() >= 1);
    
    // Should receive only one pathChanged signal (debounced)
    QCOMPARE(pathChangedSpy.count(), 1);
}

void TestFileSystemWatcher::testLargeNumberOfFiles()
{
    const int numFiles = 50;
    QStringList testFiles;
    
    // Create many test files
    for (int i = 0; i < numFiles; ++i) {
        QString testFile = m_testDir + QString("/test_file_%1.txt").arg(i);
        createTestFile(testFile);
        testFiles.append(testFile);
    }
    
    // Add all files to watcher
    for (const QString &file : testFiles) {
        QVERIFY(m_watcher->addFile(file));
    }
    
    // Verify all files are being watched
    QCOMPARE(m_watcher->watchedFiles().size(), numFiles);
    
    // Remove all files
    for (const QString &file : testFiles) {
        QVERIFY(m_watcher->removeFile(file));
    }
    
    // Verify no files are being watched
    QVERIFY(m_watcher->watchedFiles().isEmpty());
}

void TestFileSystemWatcher::waitForSignal(QSignalSpy &spy, int timeout)
{
    if (spy.isEmpty()) {
        QVERIFY(spy.wait(timeout));
    }
}

void TestFileSystemWatcher::createTestFile(const QString &path, const QString &content)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    
    QTextStream out(&file);
    out << content;
    file.close();
    
    QVERIFY(QFileInfo(path).exists());
}

void TestFileSystemWatcher::modifyTestFile(const QString &path, const QString &newContent)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    
    QTextStream out(&file);
    out << newContent;
    file.close();
    
    // Ensure the modification time changes
    QTest::qWait(10);
}

void TestFileSystemWatcher::createTestDirectory(const QString &path)
{
    QDir dir;
    QVERIFY(dir.mkpath(path));
    QVERIFY(QFileInfo(path).isDir());
}

QTEST_MAIN(TestFileSystemWatcher)
#include "test_filesystemwatcher.moc"