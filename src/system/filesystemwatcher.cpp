#include "filesystemwatcher.h"
#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QDebug>

namespace NFSShareManager {

FileSystemWatcher::FileSystemWatcher(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_debounceTimer(new QTimer(this))
    , m_debounceInterval(DEFAULT_DEBOUNCE_INTERVAL)
    , m_changeDetectionEnabled(true)
{
    // Configure debounce timer
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(m_debounceInterval);
    
    // Connect signals
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &FileSystemWatcher::onFileChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FileSystemWatcher::onDirectoryChanged);
    connect(m_debounceTimer, &QTimer::timeout,
            this, &FileSystemWatcher::onDebounceTimer);
    
    qDebug() << "FileSystemWatcher: Initialized with debounce interval" << m_debounceInterval << "ms";
}

FileSystemWatcher::~FileSystemWatcher()
{
    // Cleanup is handled automatically by Qt parent-child relationships
    qDebug() << "FileSystemWatcher: Destroyed";
}

bool FileSystemWatcher::addFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        qWarning() << "FileSystemWatcher: Cannot add empty file path";
        return false;
    }
    
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qWarning() << "FileSystemWatcher: File does not exist:" << filePath;
        emit watchFailed(filePath, "File does not exist");
        return false;
    }
    
    if (!fileInfo.isFile()) {
        qWarning() << "FileSystemWatcher: Path is not a file:" << filePath;
        emit watchFailed(filePath, "Path is not a file");
        return false;
    }
    
    QString absolutePath = fileInfo.absoluteFilePath();
    
    if (m_watcher->files().contains(absolutePath)) {
        qDebug() << "FileSystemWatcher: File already being watched:" << absolutePath;
        return true;
    }
    
    bool success = m_watcher->addPath(absolutePath);
    if (success) {
        updateFileInfo(absolutePath);
        qDebug() << "FileSystemWatcher: Added file:" << absolutePath;
    } else {
        qWarning() << "FileSystemWatcher: Failed to add file:" << absolutePath;
        emit watchFailed(absolutePath, "Failed to add file to watcher");
    }
    
    return success;
}

bool FileSystemWatcher::addDirectory(const QString &dirPath, bool recursive)
{
    if (dirPath.isEmpty()) {
        qWarning() << "FileSystemWatcher: Cannot add empty directory path";
        return false;
    }
    
    QFileInfo dirInfo(dirPath);
    if (!dirInfo.exists()) {
        qWarning() << "FileSystemWatcher: Directory does not exist:" << dirPath;
        return false;
    }
    
    if (!dirInfo.isDir()) {
        qWarning() << "FileSystemWatcher: Path is not a directory:" << dirPath;
        return false;
    }
    
    QString absolutePath = dirInfo.absoluteFilePath();
    
    if (m_watcher->directories().contains(absolutePath)) {
        qDebug() << "FileSystemWatcher: Directory already being watched:" << absolutePath;
        return true;
    }
    
    bool success = m_watcher->addPath(absolutePath);
    if (success) {
        updateFileInfo(absolutePath);
        m_recursiveDirectories[absolutePath] = recursive;
        
        if (recursive) {
            addRecursiveDirectory(absolutePath);
        }
        
        qDebug() << "FileSystemWatcher: Added directory:" << absolutePath 
                 << (recursive ? "(recursive)" : "(non-recursive)");
    } else {
        qWarning() << "FileSystemWatcher: Failed to add directory:" << absolutePath;
        emit watchFailed(absolutePath, "Failed to add directory to watcher");
    }
    
    return success;
}

bool FileSystemWatcher::removeFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }
    
    QString absolutePath = QFileInfo(filePath).absoluteFilePath();
    
    if (!m_watcher->files().contains(absolutePath)) {
        return true; // Already not watching
    }
    
    bool success = m_watcher->removePath(absolutePath);
    if (success) {
        m_fileInfoCache.remove(absolutePath);
        m_pendingChanges.remove(absolutePath);
        m_lastChangeTime.remove(absolutePath);
        qDebug() << "FileSystemWatcher: Removed file:" << absolutePath;
    } else {
        qWarning() << "FileSystemWatcher: Failed to remove file:" << absolutePath;
    }
    
    return success;
}

bool FileSystemWatcher::removeDirectory(const QString &dirPath)
{
    if (dirPath.isEmpty()) {
        return false;
    }
    
    QString absolutePath = QFileInfo(dirPath).absoluteFilePath();
    
    if (!m_watcher->directories().contains(absolutePath)) {
        return true; // Already not watching
    }
    
    // Remove recursive watching if enabled
    if (m_recursiveDirectories.value(absolutePath, false)) {
        removeRecursiveDirectory(absolutePath);
    }
    
    bool success = m_watcher->removePath(absolutePath);
    if (success) {
        m_fileInfoCache.remove(absolutePath);
        m_pendingChanges.remove(absolutePath);
        m_lastChangeTime.remove(absolutePath);
        m_recursiveDirectories.remove(absolutePath);
        qDebug() << "FileSystemWatcher: Removed directory:" << absolutePath;
    } else {
        qWarning() << "FileSystemWatcher: Failed to remove directory:" << absolutePath;
    }
    
    return success;
}

QStringList FileSystemWatcher::watchedFiles() const
{
    return m_watcher->files();
}

QStringList FileSystemWatcher::watchedDirectories() const
{
    return m_watcher->directories();
}

bool FileSystemWatcher::isWatching(const QString &path) const
{
    if (path.isEmpty()) {
        return false;
    }
    
    QString absolutePath = QFileInfo(path).absoluteFilePath();
    return m_watcher->files().contains(absolutePath) || 
           m_watcher->directories().contains(absolutePath);
}

void FileSystemWatcher::setDebounceInterval(int intervalMs)
{
    if (intervalMs < 0) {
        intervalMs = 0;
    }
    
    m_debounceInterval = intervalMs;
    m_debounceTimer->setInterval(intervalMs);
    
    qDebug() << "FileSystemWatcher: Set debounce interval to" << intervalMs << "ms";
}

int FileSystemWatcher::debounceInterval() const
{
    return m_debounceInterval;
}

void FileSystemWatcher::setChangeDetectionEnabled(bool enabled)
{
    m_changeDetectionEnabled = enabled;
    qDebug() << "FileSystemWatcher: Change detection" << (enabled ? "enabled" : "disabled");
}

bool FileSystemWatcher::isChangeDetectionEnabled() const
{
    return m_changeDetectionEnabled;
}

void FileSystemWatcher::addIgnorePattern(const QString &pattern)
{
    if (!pattern.isEmpty() && !m_ignorePatterns.contains(pattern)) {
        m_ignorePatterns.append(pattern);
        qDebug() << "FileSystemWatcher: Added ignore pattern:" << pattern;
    }
}

void FileSystemWatcher::removeIgnorePattern(const QString &pattern)
{
    if (m_ignorePatterns.removeAll(pattern) > 0) {
        qDebug() << "FileSystemWatcher: Removed ignore pattern:" << pattern;
    }
}

QStringList FileSystemWatcher::ignorePatterns() const
{
    return m_ignorePatterns;
}

void FileSystemWatcher::clearIgnorePatterns()
{
    m_ignorePatterns.clear();
    qDebug() << "FileSystemWatcher: Cleared all ignore patterns";
}

void FileSystemWatcher::onFileChanged(const QString &filePath)
{
    if (!m_changeDetectionEnabled || shouldIgnorePath(filePath)) {
        return;
    }
    
    ChangeType changeType = detectChangeType(filePath);
    
    qDebug() << "FileSystemWatcher: File changed:" << filePath << "type:" << (int)changeType;
    
    // Emit immediate signal
    emit fileChanged(filePath, changeType);
    
    // Queue for debounced signal
    emitChangeSignal(filePath, changeType);
}

void FileSystemWatcher::onDirectoryChanged(const QString &dirPath)
{
    if (!m_changeDetectionEnabled || shouldIgnorePath(dirPath)) {
        return;
    }
    
    ChangeType changeType = detectChangeType(dirPath);
    
    qDebug() << "FileSystemWatcher: Directory changed:" << dirPath << "type:" << (int)changeType;
    
    // Handle recursive directory watching
    if (m_recursiveDirectories.value(dirPath, false)) {
        // Check for new subdirectories to watch
        QDir dir(dirPath);
        if (dir.exists()) {
            QDirIterator it(dirPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString subDir = it.next();
                if (!m_watcher->directories().contains(subDir)) {
                    m_watcher->addPath(subDir);
                    updateFileInfo(subDir);
                    qDebug() << "FileSystemWatcher: Added new subdirectory:" << subDir;
                }
            }
        }
    }
    
    // Emit immediate signal
    emit directoryChanged(dirPath, changeType);
    
    // Queue for debounced signal
    emitChangeSignal(dirPath, changeType);
}

void FileSystemWatcher::onDebounceTimer()
{
    processPendingChanges();
}

FileSystemWatcher::ChangeType FileSystemWatcher::detectChangeType(const QString &path)
{
    QFileInfo currentInfo(path);
    QFileInfo cachedInfo = getCachedFileInfo(path);
    
    if (!currentInfo.exists()) {
        if (cachedInfo.exists()) {
            // File/directory was deleted
            m_fileInfoCache.remove(path);
            return cachedInfo.isDir() ? ChangeType::DirectoryDeleted : ChangeType::FileDeleted;
        }
        return ChangeType::Unknown;
    }
    
    if (!cachedInfo.exists()) {
        // File/directory was created
        updateFileInfo(path);
        return currentInfo.isDir() ? ChangeType::DirectoryCreated : ChangeType::FileCreated;
    }
    
    // Check for permission changes
    if (currentInfo.permissions() != cachedInfo.permissions()) {
        updateFileInfo(path);
        return ChangeType::PermissionsChanged;
    }
    
    // Check for modification
    if (currentInfo.lastModified() != cachedInfo.lastModified()) {
        updateFileInfo(path);
        return currentInfo.isDir() ? ChangeType::DirectoryModified : ChangeType::FileModified;
    }
    
    // Check for size changes (files only)
    if (currentInfo.isFile() && currentInfo.size() != cachedInfo.size()) {
        updateFileInfo(path);
        return ChangeType::FileModified;
    }
    
    // If we get here, update the cache anyway and return a generic change
    updateFileInfo(path);
    return currentInfo.isDir() ? ChangeType::DirectoryModified : ChangeType::FileModified;
}

bool FileSystemWatcher::shouldIgnorePath(const QString &path) const
{
    if (m_ignorePatterns.isEmpty()) {
        return false;
    }
    
    QString fileName = QFileInfo(path).fileName();
    
    for (const QString &pattern : m_ignorePatterns) {
        QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(pattern));
        if (regex.match(fileName).hasMatch()) {
            return true;
        }
    }
    
    return false;
}

void FileSystemWatcher::updateFileInfo(const QString &path)
{
    QFileInfo info(path);
    m_fileInfoCache[path] = info;
}

QFileInfo FileSystemWatcher::getCachedFileInfo(const QString &path) const
{
    return m_fileInfoCache.value(path, QFileInfo());
}

void FileSystemWatcher::addRecursiveDirectory(const QString &dirPath)
{
    QDirIterator it(dirPath, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString subDir = it.next();
        if (!m_watcher->directories().contains(subDir)) {
            m_watcher->addPath(subDir);
            updateFileInfo(subDir);
            qDebug() << "FileSystemWatcher: Added recursive subdirectory:" << subDir;
        }
    }
}

void FileSystemWatcher::removeRecursiveDirectory(const QString &dirPath)
{
    QStringList watchedDirs = m_watcher->directories();
    for (const QString &watchedDir : watchedDirs) {
        if (watchedDir.startsWith(dirPath + "/")) {
            m_watcher->removePath(watchedDir);
            m_fileInfoCache.remove(watchedDir);
            qDebug() << "FileSystemWatcher: Removed recursive subdirectory:" << watchedDir;
        }
    }
}

void FileSystemWatcher::processPendingChanges()
{
    for (auto it = m_pendingChanges.begin(); it != m_pendingChanges.end(); ++it) {
        const QString &path = it.key();
        const ChangeType &changeType = it.value();
        
        qDebug() << "FileSystemWatcher: Emitting debounced change:" << path << "type:" << (int)changeType;
        emit pathChanged(path, changeType);
    }
    
    m_pendingChanges.clear();
}

void FileSystemWatcher::emitChangeSignal(const QString &path, ChangeType changeType)
{
    if (m_debounceInterval <= 0) {
        // No debouncing, emit immediately
        emit pathChanged(path, changeType);
        return;
    }
    
    // Store the change for debounced emission
    m_pendingChanges[path] = changeType;
    m_lastChangeTime[path] = QDateTime::currentDateTime();
    
    // Start or restart the debounce timer
    m_debounceTimer->start();
}

} // namespace NFSShareManager