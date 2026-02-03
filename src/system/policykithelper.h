#pragma once

#include <QObject>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariantMap>
#include <QString>
#include <QStringList>

namespace NFSShareManager {

/**
 * @brief PolicyKit helper class for privilege escalation
 * 
 * This class provides a secure interface for performing privileged operations
 * through PolicyKit authentication. It handles D-Bus communication with the
 * PolicyKit daemon and manages authentication for NFS-related system operations.
 */
class PolicyKitHelper : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief PolicyKit action identifiers for NFS operations
     */
    enum class Action {
        CreateShare,        ///< Create new NFS share (modify /etc/exports)
        RemoveShare,        ///< Remove NFS share (modify /etc/exports)
        ModifyShare,        ///< Modify existing NFS share configuration
        MountRemoteShare,   ///< Mount remote NFS share
        UnmountShare,       ///< Unmount NFS share
        ModifyFstab,        ///< Modify /etc/fstab for persistent mounts
        RestartNFSService,  ///< Restart NFS server service
        ModifySystemFiles   ///< General system file modifications
    };
    Q_ENUM(Action)

    /**
     * @brief Authentication result status
     */
    enum class AuthResult {
        Authorized,         ///< User is authorized to perform the action
        NotAuthorized,      ///< User is not authorized
        AuthenticationFailed, ///< Authentication process failed
        Cancelled,          ///< User cancelled authentication
        Error               ///< System error during authentication
    };
    Q_ENUM(AuthResult)

    explicit PolicyKitHelper(QObject *parent = nullptr);
    ~PolicyKitHelper();

    /**
     * @brief Check if the current user is authorized for a specific action
     * @param action The action to check authorization for
     * @return AuthResult indicating the authorization status
     */
    AuthResult checkAuthorization(Action action);

    /**
     * @brief Execute a privileged action with PolicyKit authentication
     * @param action The action to execute
     * @param parameters Action-specific parameters
     * @return true if the action was executed successfully, false otherwise
     */
    bool executePrivilegedAction(Action action, const QVariantMap &parameters = QVariantMap());

    /**
     * @brief Get human-readable description for an action
     * @param action The action to get description for
     * @return Localized description string
     */
    static QString getActionDescription(Action action);

    /**
     * @brief Get PolicyKit action ID string for an action
     * @param action The action to get ID for
     * @return PolicyKit action ID string
     */
    static QString getActionId(Action action);

    /**
     * @brief Check if PolicyKit is available on the system
     * @return true if PolicyKit is available, false otherwise
     */
    bool isPolicyKitAvailable() const;

signals:
    /**
     * @brief Emitted when an authorization check completes
     * @param action The action that was checked
     * @param result The authorization result
     */
    void authorizationResult(Action action, AuthResult result);

    /**
     * @brief Emitted when a privileged action completes
     * @param action The action that was executed
     * @param success Whether the action succeeded
     * @param errorMessage Error message if the action failed
     */
    void actionCompleted(Action action, bool success, const QString &errorMessage = QString());

    /**
     * @brief Emitted when authentication is required
     * @param action The action requiring authentication
     * @param message User-friendly message explaining why authentication is needed
     */
    void authenticationRequired(Action action, const QString &message);

private slots:
    /**
     * @brief Handle D-Bus service availability changes
     */
    void onServiceAvailabilityChanged();

private:
    /**
     * @brief Initialize PolicyKit D-Bus interface
     * @return true if initialization succeeded, false otherwise
     */
    bool initializePolicyKit();

    /**
     * @brief Perform actual privileged operation after authentication
     * @param action The action to perform
     * @param parameters Action parameters
     * @return true if successful, false otherwise
     */
    bool performPrivilegedOperation(Action action, const QVariantMap &parameters);

    /**
     * @brief Execute system command with proper error handling
     * @param command The command to execute
     * @param arguments Command arguments
     * @return true if command succeeded, false otherwise
     */
    bool executeSystemCommand(const QString &command, const QStringList &arguments);

    /**
     * @brief Create backup of system file before modification
     * @param filePath Path to the file to backup
     * @return true if backup was created successfully
     */
    bool createBackup(const QString &filePath);

    /**
     * @brief Validate parameters for a specific action
     * @param action The action to validate parameters for
     * @param parameters The parameters to validate
     * @return true if parameters are valid, false otherwise
     */
    bool validateParameters(Action action, const QVariantMap &parameters);

    QDBusInterface *m_policyKitInterface;   ///< D-Bus interface to PolicyKit
    bool m_isAvailable;                     ///< Whether PolicyKit is available
    QString m_lastError;                    ///< Last error message
};

} // namespace NFSShareManager