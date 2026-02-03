#pragma once

#include <QObject>
#include <QList>
#include <QTimer>
#include <QFileSystemWatcher>
#include "../core/nfsmount.h"
#include "../core/remotenfsshare.h"
#include "../system/nfsserviceinterface.h"
#include "../system/policykithelper.h"

namespace NFSShareManager {

/**
 * @brief Manages mounting and unmounting of remote NFS shares
 * 
 * The MountManager class provides comprehensive functionality for mounting
 * remote NFS shares to local directories, including mount point validation,
 * temporary and persistent mount support, and integration with system
 * configuration files.
 */
class MountManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Mount operation result
     */
    enum class MountResult {
        Success,                ///< Mount operation succeeded
        InvalidMountPoint,      ///< Mount point is invalid or inaccessible
        InvalidRemoteShare,     ///< Remote share information is invalid
        MountPointExists,       ///< Mount point already has something mounted
        PermissionDenied,       ///< Insufficient permissions for mount operation
        NetworkError,           ///< Network connectivity issues
        NFSServiceError,        ///< NFS service or protocol error
        SystemError,            ///< General system error
        Cancelled               ///< Operation was cancelled by user
    };
    Q_ENUM(MountResult)

    explicit MountManager(QObject *parent = nullptr);
    ~MountManager();

    /**
     * @brief Mount a remote NFS share to a local directory
     * @param remoteShare The remote share to mount
     * @param localMountPoint The local directory to mount to
     * @param options Mount options to use
     * @param isPersistent Whether the mount should survive reboots
     * @return Mount operation result
     */
    MountResult mountShare(const RemoteNFSShare &remoteShare, 
                          const QString &localMountPoint,
                          const MountOptions &options = MountOptions(),
                          bool isPersistent = false);

    /**
     * @brief Unmount an NFS share
     * @param mountPoint The local mount point to unmount
     * @param force Whether to force unmount if busy
     * @return true if unmount succeeded, false otherwise
     */
    bool unmountShare(const QString &mountPoint, bool force = false);

    /**
     * @brief Unmount an NFS share by mount object
     * @param mount The mount to unmount
     * @param force Whether to force unmount if busy
     * @return true if unmount succeeded, false otherwise
     */
    bool unmountShare(const NFSMount &mount, bool force = false);

    /**
     * @brief Get list of currently managed mounts
     * @return List of active NFS mounts
     */
    QList<NFSMount> getManagedMounts() const;

    /**
     * @brief Get a specific mount by mount point
     * @param mountPoint The mount point to look for
     * @return The mount object, or invalid mount if not found
     */
    NFSMount getMountByPath(const QString &mountPoint) const;

    /**
     * @brief Check if a path is currently managed as a mount point
     * @param mountPoint The path to check
     * @return true if the path is a managed mount point
     */
    bool isManagedMount(const QString &mountPoint) const;

    /**
     * @brief Validate a mount point path
     * @param mountPoint The path to validate
     * @return true if the path is valid for mounting
     */
    bool validateMountPoint(const QString &mountPoint) const;

    /**
     * @brief Get validation errors for a mount point
     * @param mountPoint The path to validate
     * @return List of validation error messages
     */
    QStringList getMountPointValidationErrors(const QString &mountPoint) const;

    /**
     * @brief Create a mount point directory if it doesn't exist
     * @param mountPoint The directory path to create
     * @return true if directory exists or was created successfully
     */
    bool createMountPoint(const QString &mountPoint);

    /**
     * @brief Check if a mount point directory exists and is empty
     * @param mountPoint The directory path to check
     * @return true if directory exists and is suitable for mounting
     */
    bool isMountPointSuitable(const QString &mountPoint) const;

    /**
     * @brief Refresh the status of all managed mounts
     */
    void refreshMountStatus();

    /**
     * @brief Get the default mount options for a remote share
     * @param remoteShare The remote share to get options for
     * @return Default mount options
     */
    MountOptions getDefaultMountOptions(const RemoteNFSShare &remoteShare) const;

    /**
     * @brief Add a persistent mount entry to fstab
     * @param mount The mount to add to fstab
     * @return true if fstab was updated successfully
     */
    bool addToFstab(const NFSMount &mount);

    /**
     * @brief Remove a persistent mount entry from fstab
     * @param mount The mount to remove from fstab
     * @return true if fstab was updated successfully
     */
    bool removeFromFstab(const NFSMount &mount);

    /**
     * @brief Check if a mount exists in fstab
     * @param mount The mount to check
     * @return true if the mount is in fstab
     */
    bool isInFstab(const NFSMount &mount) const;

    /**
     * @brief Load persistent mounts from fstab
     * @return List of NFS mounts found in fstab
     */
    QList<NFSMount> loadPersistentMounts();

signals:
    /**
     * @brief Emitted when a mount operation starts
     * @param remoteShare The share being mounted
     * @param mountPoint The local mount point
     */
    void mountStarted(const RemoteNFSShare &remoteShare, const QString &mountPoint);

    /**
     * @brief Emitted when a mount operation completes successfully
     * @param mount The completed mount
     */
    void mountCompleted(const NFSMount &mount);

    /**
     * @brief Emitted when a mount operation fails
     * @param remoteShare The share that failed to mount
     * @param mountPoint The local mount point
     * @param result The failure reason
     * @param errorMessage Detailed error message
     */
    void mountFailed(const RemoteNFSShare &remoteShare, const QString &mountPoint, 
                     MountResult result, const QString &errorMessage);

    /**
     * @brief Emitted when an unmount operation starts
     * @param mountPoint The mount point being unmounted
     */
    void unmountStarted(const QString &mountPoint);

    /**
     * @brief Emitted when an unmount operation completes successfully
     * @param mountPoint The unmounted path
     */
    void unmountCompleted(const QString &mountPoint);

    /**
     * @brief Emitted when an unmount operation fails
     * @param mountPoint The mount point that failed to unmount
     * @param errorMessage Detailed error message
     */
    void unmountFailed(const QString &mountPoint, const QString &errorMessage);

    /**
     * @brief Emitted when mount status changes
     * @param mount The mount with updated status
     */
    void mountStatusChanged(const NFSMount &mount);

private slots:
    /**
     * @brief Handle mount status refresh timer
     */
    void onRefreshTimer();

    /**
     * @brief Handle file system changes
     * @param path The path that changed
     */
    void onFileSystemChanged(const QString &path);

    /**
     * @brief Handle PolicyKit action completion
     * @param action The completed action
     * @param success Whether the action succeeded
     * @param errorMessage Error message if failed
     */
    void onPolicyKitActionCompleted(PolicyKitHelper::Action action, bool success, const QString &errorMessage);

private:
    /**
     * @brief Perform the actual mount operation
     * @param mount The mount to perform
     * @return Mount operation result
     */
    MountResult performMount(const NFSMount &mount);

    /**
     * @brief Perform the actual unmount operation
     * @param mountPoint The mount point to unmount
     * @param force Whether to force unmount
     * @return true if unmount succeeded
     */
    bool performUnmount(const QString &mountPoint, bool force);

    /**
     * @brief Update mount status by checking system state
     * @param mount The mount to update
     * @return true if status was updated
     */
    bool updateMountStatus(NFSMount &mount);

    /**
     * @brief Generate a unique mount point path
     * @param remoteShare The remote share to generate path for
     * @return A unique mount point path
     */
    QString generateMountPoint(const RemoteNFSShare &remoteShare) const;

    /**
     * @brief Validate mount options
     * @param options The options to validate
     * @return true if options are valid
     */
    bool validateMountOptions(const MountOptions &options) const;

    /**
     * @brief Parse fstab file to find NFS entries
     * @return List of NFS mounts from fstab
     */
    QList<NFSMount> parseFstabEntries() const;

    /**
     * @brief Create backup of fstab before modification
     * @return true if backup was created successfully
     */
    bool backupFstab() const;

    /**
     * @brief Add mount to internal tracking
     * @param mount The mount to track
     */
    void addMountToTracking(const NFSMount &mount);

    /**
     * @brief Remove mount from internal tracking
     * @param mountPoint The mount point to stop tracking
     */
    void removeMountFromTracking(const QString &mountPoint);

    /**
     * @brief Find mount in tracking list
     * @param mountPoint The mount point to find
     * @return Iterator to the mount, or end() if not found
     */
    QList<NFSMount>::iterator findMount(const QString &mountPoint);

    /**
     * @brief Find mount in tracking list (const version)
     * @param mountPoint The mount point to find
     * @return Const iterator to the mount, or end() if not found
     */
    QList<NFSMount>::const_iterator findMount(const QString &mountPoint) const;

    NFSServiceInterface *m_nfsService;      ///< NFS service interface
    PolicyKitHelper *m_policyKitHelper;     ///< PolicyKit helper for privileged operations
    QFileSystemWatcher *m_fsWatcher;        ///< File system watcher for mount points
    QTimer *m_refreshTimer;                 ///< Timer for periodic status refresh
    QList<NFSMount> m_managedMounts;        ///< List of managed mounts
    QString m_defaultMountRoot;             ///< Default root directory for mounts
    int m_refreshInterval;                  ///< Status refresh interval in seconds
};

} // namespace NFSShareManager