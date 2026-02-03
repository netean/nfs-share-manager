#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QStringList>
#include <QHash>
#include <QDateTime>
#include <QFileInfo>

namespace NFSShareManager {

/**
 * @brief Enhanced file system watcher for NFS-related file monitoring
 * 
 * This class extends Qt's QFileSystemWatcher with additional functionality
 * specific to NFS share management, including monitoring of system configuration
 * files, mount points, and shared directories with proper debouncing and
 * change detection.
 */
class FileSystemWatcher : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief File change types
     */
    enum class ChangeType {
        FileCreated,        ///< File was created
        FileModified,       ///< File was modified
        FileDeleted,        ///< File was deleted
        DirectoryCreated,   ///< Directory was created
        DirectoryModified,  ///< Directory contents changed
        DirectoryDeleted,   ///< Directory was deleted
        PermissionsChanged, ///< File/directory permissions changed
        Unknown             ///< Unknown change type
    };
    Q_ENUM(ChangeType)

    explicit FileSystemWatcher(QObject *parent = nullptr);
    ~FileSystemWatcher();

    /**
     * @brief Add a file to watch
     * @param filePath Path to the file to watch
     * @return True if the file was added successfully
     */
    bool addFile(const QString &filePath);

    /**
     * @brief Add a directory to watch
     * @param dirPath Path to the directory to watch
     * @param recursive Whether to watch subdirectories recursively
     * @return True if the directory was added successfully
     */
    bool addDirectory(const QString &dirPath, bool recursive = false);

    /**
     * @brief Remove a file from watching
     * @param filePath Path to the file to stop watching
     * @return True if the file was removed successfully
     */
    bool removeFile(const QString &filePath);

    /**
     * @brief Remove a directory from watching
     * @param dirPath Path to the directory to stop watching
     * @return True if the directory was removed successfully
     */
    bool removeDirectory(const QString &dirPath);

    /**
     * @brief Get list of watched files
     * @return List of file paths being watched
     */
    QStringList watchedFiles() const;

    /**
     * @brief Get list of watched directories
     * @return List of directory paths being watched
     */
    QStringList watchedDirectories() const;

    /**
     * @brief Check if a path is being watched
     * @param path Path to check
     * @return True if the path is being watched
     */
    bool isWatching(const QString &path) const;

    /**
     * @brief Set the debounce interval for change notifications
     * @param intervalMs Interval in milliseconds (default: 500)
     */
    void setDebounceInterval(int intervalMs);

    /**
     * @brief Get the current debounce interval
     * @return Debounce interval in milliseconds
     */
    int debounceInterval() const;

    /**
     * @brief Enable or disable change detection
     * @param enabled True to enable change detection
     */
    void setChangeDetectionEnabled(bool enabled);

    /**
     * @brief Check if change detection is enabled
     * @return True if change detection is enabled
     */
    bool isChangeDetectionEnabled() const;

    /**
     * @brief Add a path pattern to ignore
     * @param pattern Glob pattern to ignore (e.g., "*.tmp", ".*")
     */
    void addIgnorePattern(const QString &pattern);

    /**
     * @brief Remove an ignore pattern
     * @param pattern Pattern to remove
     */
    void removeIgnorePattern(const QString &pattern);

    /**
     * @brief Get list of ignore patterns
     * @return List of ignore patterns
     */
    QStringList ignorePatterns() const;

    /**
     * @brief Clear all ignore patterns
     */
    void clearIgnorePatterns();

signals:
    /**
     * @brief Emitted when a watched file changes
     * @param filePath Path to the changed file
     * @param changeType Type of change that occurred
     */
    void fileChanged(const QString &filePath, ChangeType changeType);

    /**
     * @brief Emitted when a watched directory changes
     * @param dirPath Path to the changed directory
     * @param changeType Type of change that occurred
     */
    void directoryChanged(const QString &dirPath, ChangeType changeType);

    /**
     * @brief Emitted when any watched path changes (debounced)
     * @param path Path that changed
     * @param changeType Type of change
     */
    void pathChanged(const QString &path, ChangeType changeType);

    /**
     * @brief Emitted when a watch fails (file/directory becomes inaccessible)
     * @param path Path that failed
     * @param error Error message
     */
    void watchFailed(const QString &path, const QString &error);

private slots:
    /**
     * @brief Handle file change notifications from QFileSystemWatcher
     * @param filePath Path to the changed file
     */
    void onFileChanged(const QString &filePath);

    /**
     * @brief Handle directory change notifications from QFileSystemWatcher
     * @param dirPath Path to the changed directory
     */
    void onDirectoryChanged(const QString &dirPath);

    /**
     * @brief Handle debounce timer timeout
     */
    void onDebounceTimer();

private:
    /**
     * @brief Determine the type of change for a path
     * @param path Path to analyze
     * @return Detected change type
     */
    ChangeType detectChangeType(const QString &path);

    /**
     * @brief Check if a path should be ignored based on patterns
     * @param path Path to check
     * @return True if the path should be ignored
     */
    bool shouldIgnorePath(const QString &path) const;

    /**
     * @brief Update file information cache
     * @param path Path to update
     */
    void updateFileInfo(const QString &path);

    /**
     * @brief Get cached file information
     * @param path Path to get info for
     * @return Cached file information
     */
    QFileInfo getCachedFileInfo(const QString &path) const;

    /**
     * @brief Add recursive directory watching
     * @param dirPath Directory to watch recursively
     */
    void addRecursiveDirectory(const QString &dirPath);

    /**
     * @brief Remove recursive directory watching
     * @param dirPath Directory to stop watching recursively
     */
    void removeRecursiveDirectory(const QString &dirPath);

    /**
     * @brief Process pending changes (called by debounce timer)
     */
    void processPendingChanges();

    /**
     * @brief Emit change signal with proper debouncing
     * @param path Path that changed
     * @param changeType Type of change
     */
    void emitChangeSignal(const QString &path, ChangeType changeType);

    QFileSystemWatcher *m_watcher;          ///< Qt file system watcher
    QTimer *m_debounceTimer;                ///< Timer for debouncing changes
    
    // Configuration
    int m_debounceInterval;                 ///< Debounce interval in milliseconds
    bool m_changeDetectionEnabled;          ///< Whether change detection is enabled
    QStringList m_ignorePatterns;           ///< Patterns to ignore
    
    // State tracking
    QHash<QString, QFileInfo> m_fileInfoCache;     ///< Cache of file information
    QHash<QString, bool> m_recursiveDirectories;   ///< Directories watched recursively
    QHash<QString, ChangeType> m_pendingChanges;   ///< Changes pending debounce
    QHash<QString, QDateTime> m_lastChangeTime;    ///< Last change time per path
    
    // Constants
    static const int DEFAULT_DEBOUNCE_INTERVAL = 500;  ///< Default debounce interval (500ms)
};

} // namespace NFSShareManager