#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QHostAddress>
#include <QTimer>
#include <QHash>
#include "../core/types.h"

namespace NFSShareManager {

class RemoteNFSShare;
class ShareConfiguration;

/**
 * @brief Result structure for NFS command operations
 */
struct NFSCommandResult {
    bool success;           ///< Whether the command succeeded
    int exitCode;          ///< Process exit code
    QString output;        ///< Standard output from the command
    QString error;         ///< Standard error from the command
    QString command;       ///< The command that was executed
    
    NFSCommandResult() : success(false), exitCode(-1) {}
    NFSCommandResult(bool s, int code, const QString &out, const QString &err, const QString &cmd)
        : success(s), exitCode(code), output(out), error(err), command(cmd) {}
};

/**
 * @brief Mount information structure
 */
struct MountInfo {
    QString device;        ///< Source device/share
    QString mountPoint;    ///< Local mount point
    QString fileSystem;    ///< File system type
    QString options;       ///< Mount options
    bool isNFS;           ///< Whether this is an NFS mount
    
    MountInfo() : isNFS(false) {}
};

/**
 * @brief NFS service interface wrapper
 * 
 * This class provides a high-level interface to system NFS commands,
 * abstracting the complexity of command execution, output parsing,
 * and error handling.
 */
class NFSServiceInterface : public QObject
{
    Q_OBJECT

public:
    explicit NFSServiceInterface(QObject *parent = nullptr);
    ~NFSServiceInterface();

    /**
     * @brief Check if NFS tools are available on the system
     * @return True if all required NFS tools are found
     */
    bool isNFSToolsAvailable() const;

    /**
     * @brief Get list of missing NFS tools
     * @return List of missing tool names
     */
    QStringList getMissingTools() const;

    // Export management methods
    
    /**
     * @brief Export a directory using exportfs
     * @param exportPath The directory path to export
     * @param config The share configuration
     * @return Command result with success status and output
     */
    NFSCommandResult exportDirectory(const QString &exportPath, const ShareConfiguration &config);

    /**
     * @brief Unexport a directory using exportfs
     * @param exportPath The directory path to unexport
     * @return Command result with success status and output
     */
    NFSCommandResult unexportDirectory(const QString &exportPath);

    /**
     * @brief Get list of currently exported directories
     * @return Command result containing export list
     */
    NFSCommandResult getExportedDirectories();

    /**
     * @brief Reload NFS exports from /etc/exports
     * @return Command result with success status
     */
    NFSCommandResult reloadExports();

    // Network discovery methods
    
    /**
     * @brief Query exports from a remote NFS server using showmount
     * @param hostAddress The server address to query
     * @param timeout Timeout in milliseconds (default: 5000)
     * @return Command result containing export list
     */
    NFSCommandResult queryRemoteExports(const QHostAddress &hostAddress, int timeout = 5000);

    /**
     * @brief Query exports from a remote NFS server using hostname
     * @param hostname The server hostname to query
     * @param timeout Timeout in milliseconds (default: 5000)
     * @return Command result containing export list
     */
    NFSCommandResult queryRemoteExports(const QString &hostname, int timeout = 5000);

    /**
     * @brief Check if RPC services are running on a host using rpcinfo
     * @param hostAddress The server address to check
     * @param timeout Timeout in milliseconds (default: 3000)
     * @return Command result containing RPC service information
     */
    NFSCommandResult queryRPCServices(const QHostAddress &hostAddress, int timeout = 3000);

    /**
     * @brief Check if RPC services are running on a host using hostname
     * @param hostname The server hostname to check
     * @param timeout Timeout in milliseconds (default: 3000)
     * @return Command result containing RPC service information
     */
    NFSCommandResult queryRPCServices(const QString &hostname, int timeout = 3000);

    // Mount management methods
    
    /**
     * @brief Mount an NFS share
     * @param remoteShare The remote share information
     * @param localMountPoint The local directory to mount to
     * @param options Additional mount options
     * @param nfsVersion NFS version to use
     * @return Command result with success status
     */
    NFSCommandResult mountNFSShare(const QString &serverAddress, const QString &remotePath,
                                   const QString &localMountPoint, const QStringList &options = {},
                                   NFSVersion nfsVersion = NFSVersion::Version4);

    /**
     * @brief Unmount an NFS share
     * @param mountPoint The local mount point to unmount
     * @param force Whether to force unmount
     * @return Command result with success status
     */
    NFSCommandResult unmountNFSShare(const QString &mountPoint, bool force = false);

    /**
     * @brief Get list of currently mounted NFS shares
     * @return List of mount information structures
     */
    QList<MountInfo> getMountedNFSShares();

    /**
     * @brief Check if a path is an NFS mount point
     * @param path The path to check
     * @return True if the path is an NFS mount point
     */
    bool isNFSMountPoint(const QString &path);

    // Parsing methods
    
    /**
     * @brief Parse exportfs output into a list of export entries
     * @param output The raw exportfs output
     * @return List of parsed export entries
     */
    QStringList parseExportfsOutput(const QString &output);

    /**
     * @brief Parse showmount output into a list of remote shares
     * @param output The raw showmount output
     * @param serverAddress The server address that was queried
     * @return List of discovered remote shares
     */
    QList<RemoteNFSShare> parseShowmountOutput(const QString &output, const QString &serverAddress);

    /**
     * @brief Parse rpcinfo output to check for NFS services
     * @param output The raw rpcinfo output
     * @return True if NFS services are available
     */
    bool parseRPCInfoOutput(const QString &output);

    /**
     * @brief Parse mount output to extract mount information
     * @param output The raw mount command output
     * @return List of mount information structures
     */
    QList<MountInfo> parseMountOutput(const QString &output);

signals:
    /**
     * @brief Emitted when a command execution starts
     * @param command The command being executed
     */
    void commandStarted(const QString &command);

    /**
     * @brief Emitted when a command execution finishes
     * @param result The command result
     */
    void commandFinished(const NFSCommandResult &result);

    /**
     * @brief Emitted when a command times out
     * @param command The command that timed out
     */
    void commandTimeout(const QString &command);

private slots:
    /**
     * @brief Handle process finished signal
     */
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /**
     * @brief Handle process error signal
     */
    void onProcessError(QProcess::ProcessError error);

    /**
     * @brief Handle command timeout
     */
    void onCommandTimeout();

private:
    /**
     * @brief Execute a system command with timeout
     * @param program The program to execute
     * @param arguments The command arguments
     * @param timeout Timeout in milliseconds
     * @return Command result structure
     */
    NFSCommandResult executeCommand(const QString &program, const QStringList &arguments, int timeout = 10000);

    /**
     * @brief Check if a command/tool exists in the system PATH
     * @param command The command name to check
     * @return True if the command is available
     */
    bool isCommandAvailable(const QString &command) const;

    /**
     * @brief Generate mount options string from configuration
     * @param options List of mount options
     * @param nfsVersion NFS version to use
     * @return Formatted options string
     */
    QString generateMountOptions(const QStringList &options, NFSVersion nfsVersion) const;

    /**
     * @brief Validate mount point path
     * @param path The path to validate
     * @return True if the path is valid for mounting
     */
    bool validateMountPoint(const QString &path) const;

    /**
     * @brief Clean up any running processes
     */
    void cleanup();

    QProcess *m_currentProcess;        ///< Currently running process
    QTimer *m_timeoutTimer;           ///< Timeout timer for commands
    QString m_currentCommand;         ///< Currently executing command
    int m_defaultTimeout;             ///< Default command timeout in ms

    // Tool availability cache
    mutable QHash<QString, bool> m_toolAvailability;
    mutable bool m_toolsChecked;
};

} // namespace NFSShareManager