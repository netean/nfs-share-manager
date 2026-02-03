#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QFileSystemWatcher>
#include "../core/nfsshare.h"
#include "../core/shareconfiguration.h"
#include "../core/permissionset.h"
#include "../system/policykithelper.h"

namespace NFSShareManager {

class NFSServiceInterface;
struct NFSCommandResult;

/**
 * @brief Share manager class for local NFS share management
 * 
 * This class handles the creation, removal, and configuration management
 * of local NFS shares. It integrates with PolicyKitHelper for privileged
 * operations and NFSServiceInterface for system NFS command execution.
 */
class ShareManager : public QObject
{
    Q_OBJECT

public:
    explicit ShareManager(QObject *parent = nullptr);
    ~ShareManager();

    /**
     * @brief Create a new NFS share
     * @param path Local directory path to share
     * @param config Share configuration settings
     * @return True if the share was created successfully
     */
    bool createShare(const QString &path, const ShareConfiguration &config);

    /**
     * @brief Remove an existing NFS share
     * @param path Local directory path of the share to remove
     * @return True if the share was removed successfully
     */
    bool removeShare(const QString &path);

    /**
     * @brief Get list of all active shares
     * @return List of currently active NFS shares
     */
    QList<NFSShare> getActiveShares() const;

    /**
     * @brief Update permissions for an existing share
     * @param path Local directory path of the share
     * @param permissions New permission settings
     * @return True if permissions were updated successfully
     */
    bool updateSharePermissions(const QString &path, const PermissionSet &permissions);

    /**
     * @brief Update configuration for an existing share
     * @param path Local directory path of the share
     * @param config New configuration settings
     * @return True if configuration was updated successfully
     */
    bool updateShareConfiguration(const QString &path, const ShareConfiguration &config);

    /**
     * @brief Get a specific share by path
     * @param path Local directory path of the share
     * @return The NFSShare object, or invalid share if not found
     */
    NFSShare getShare(const QString &path) const;

    /**
     * @brief Check if a path is currently shared
     * @param path Local directory path to check
     * @return True if the path is currently shared
     */
    bool isShared(const QString &path) const;

    /**
     * @brief Refresh the list of active shares from system
     * This synchronizes internal state with actual system exports
     */
    void refreshShares();

    /**
     * @brief Validate a directory path for sharing
     * @param path Directory path to validate
     * @return True if the path can be shared
     */
    bool validateSharePath(const QString &path) const;

    /**
     * @brief Add an existing share (for loading from configuration)
     * @param share The NFSShare to add
     * @return True if the share was added successfully
     */
    bool addExistingShare(const NFSShare &share);

    /**
     * @brief Get validation errors for a path
     * @param path Directory path to validate
     * @return List of validation error messages
     */
    QStringList getPathValidationErrors(const QString &path) const;

    /**
     * @brief Generate export file content for all shares
     * @return Complete /etc/exports file content
     */
    QString generateExportsFileContent() const;

    /**
     * @brief Check if NFS server is running
     * @return True if NFS server service is active
     */
    bool isNFSServerRunning() const;

    /**
     * @brief Start NFS server service
     * @return True if service was started successfully
     */
    bool startNFSServer();

    /**
     * @brief Restart NFS server service
     * @return True if service was restarted successfully
     */
    bool restartNFSServer();

signals:
    /**
     * @brief Emitted when a new share is created
     * @param share The newly created share
     */
    void shareCreated(const NFSShare &share);

    /**
     * @brief Emitted when a share is removed
     * @param path The path of the removed share
     */
    void shareRemoved(const QString &path);

    /**
     * @brief Emitted when a share is updated
     * @param share The updated share
     */
    void shareUpdated(const NFSShare &share);

    /**
     * @brief Emitted when a share operation fails
     * @param path The path of the share that failed
     * @param error Error message describing the failure
     */
    void shareError(const QString &path, const QString &error);

    /**
     * @brief Emitted when share list is refreshed
     */
    void sharesRefreshed();

    /**
     * @brief Emitted when NFS server status changes
     * @param running True if server is now running
     */
    void nfsServerStatusChanged(bool running);

    /**
     * @brief Emitted when shares need to be persisted to configuration
     */
    void sharesPersistenceRequested();

private slots:
    /**
     * @brief Handle PolicyKit action completion
     * @param action The completed action
     * @param success Whether the action succeeded
     * @param errorMessage Error message if action failed
     */
    void onPolicyKitActionCompleted(PolicyKitHelper::Action action, bool success, const QString &errorMessage);

    /**
     * @brief Handle NFS command completion
     * @param result The command result
     */
    void onNFSCommandFinished(const NFSCommandResult &result);

    /**
     * @brief Handle file system changes
     * @param path The path that changed
     */
    void onFileSystemChanged(const QString &path);

    /**
     * @brief Periodic refresh of share status
     */
    void onRefreshTimer();

private:
    /**
     * @brief Initialize the share manager
     */
    void initialize();

    /**
     * @brief Load existing shares from system
     */
    void loadExistingShares();

    /**
     * @brief Validate share configuration
     * @param config Configuration to validate
     * @return True if configuration is valid
     */
    bool validateShareConfiguration(const ShareConfiguration &config) const;

    /**
     * @brief Update exports file with current shares
     * @return True if exports file was updated successfully
     */
    bool updateExportsFile();

    /**
     * @brief Create backup of exports file
     * @return True if backup was created successfully
     */
    bool backupExportsFile();

    /**
     * @brief Find share by path
     * @param path Directory path to find
     * @return Iterator to the share, or end() if not found
     */
    QList<NFSShare>::iterator findShare(const QString &path);

    /**
     * @brief Find share by path (const version)
     * @param path Directory path to find
     * @return Const iterator to the share, or end() if not found
     */
    QList<NFSShare>::const_iterator findShare(const QString &path) const;

    /**
     * @brief Normalize directory path
     * @param path Path to normalize
     * @return Normalized absolute path
     */
    QString normalizePath(const QString &path) const;

    /**
     * @brief Check directory permissions
     * @param path Directory path to check
     * @return True if directory is accessible
     */
    bool checkDirectoryPermissions(const QString &path) const;

    /**
     * @brief Generate unique export path
     * @param basePath Base directory path
     * @return Unique export path
     */
    QString generateExportPath(const QString &basePath) const;

    PolicyKitHelper *m_policyKitHelper;     ///< PolicyKit integration
    NFSServiceInterface *m_nfsService;      ///< NFS service interface
    QList<NFSShare> m_activeShares;         ///< List of active shares
    QFileSystemWatcher *m_fileWatcher;      ///< File system watcher
    QTimer *m_refreshTimer;                 ///< Periodic refresh timer
    QString m_lastError;                    ///< Last error message
    bool m_initialized;                     ///< Initialization status
};

} // namespace NFSShareManager