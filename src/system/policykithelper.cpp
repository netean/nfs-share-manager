#include "policykithelper.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusError>
#include <QDBusArgument>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QCoreApplication>

Q_LOGGING_CATEGORY(policyKitLog, "nfs.policykit")

namespace NFSShareManager {

PolicyKitHelper::PolicyKitHelper(QObject *parent)
    : QObject(parent)
    , m_policyKitInterface(nullptr)
    , m_isAvailable(false)
{
    initializePolicyKit();
}

PolicyKitHelper::~PolicyKitHelper()
{
    delete m_policyKitInterface;
}

bool PolicyKitHelper::initializePolicyKit()
{
    // Check if PolicyKit service is available
    QDBusConnection systemBus = QDBusConnection::systemBus();
    if (!systemBus.isConnected()) {
        qCWarning(policyKitLog) << "Cannot connect to system D-Bus";
        m_lastError = tr("Cannot connect to system D-Bus");
        return false;
    }

    // Check if PolicyKit service is running
    QDBusConnectionInterface *interface = systemBus.interface();
    if (!interface->isServiceRegistered("org.freedesktop.PolicyKit1")) {
        qCWarning(policyKitLog) << "PolicyKit service is not available";
        m_lastError = tr("PolicyKit service is not available");
        return false;
    }

    // Create D-Bus interface to PolicyKit
    m_policyKitInterface = new QDBusInterface(
        "org.freedesktop.PolicyKit1",
        "/org/freedesktop/PolicyKit1/Authority",
        "org.freedesktop.PolicyKit1.Authority",
        systemBus,
        this
    );

    if (!m_policyKitInterface->isValid()) {
        qCWarning(policyKitLog) << "Failed to create PolicyKit D-Bus interface:" 
                                << m_policyKitInterface->lastError().message();
        m_lastError = tr("Failed to connect to PolicyKit: %1")
                         .arg(m_policyKitInterface->lastError().message());
        delete m_policyKitInterface;
        m_policyKitInterface = nullptr;
        return false;
    }

    m_isAvailable = true;
    qCInfo(policyKitLog) << "PolicyKit interface initialized successfully";

    // Monitor service availability
    connect(interface, &QDBusConnectionInterface::serviceOwnerChanged,
            this, &PolicyKitHelper::onServiceAvailabilityChanged);

    return true;
}

bool PolicyKitHelper::isPolicyKitAvailable() const
{
    return m_isAvailable;
}

PolicyKitHelper::AuthResult PolicyKitHelper::checkAuthorization(Action action)
{
    if (!m_isAvailable || !m_policyKitInterface) {
        qCWarning(policyKitLog) << "PolicyKit not available for authorization check";
        return AuthResult::Error;
    }

    QString actionId = getActionId(action);
    if (actionId.isEmpty()) {
        qCWarning(policyKitLog) << "Invalid action for authorization check";
        return AuthResult::Error;
    }

    // For now, we'll use a simplified approach that checks if the user is in the admin group
    // In a real implementation, you would use the full PolicyKit D-Bus API
    // This is a placeholder that demonstrates the interface
    
    // Check if user is in admin/sudo group as a basic authorization check
    QProcess process;
    process.start("groups");
    process.waitForFinished(5000);
    
    if (process.exitCode() == 0) {
        QString groups = process.readAllStandardOutput();
        if (groups.contains("sudo") || groups.contains("admin") || groups.contains("wheel")) {
            qCInfo(policyKitLog) << "User has admin privileges for action:" << actionId;
            return AuthResult::Authorized;
        } else {
            qCWarning(policyKitLog) << "User does not have admin privileges for action:" << actionId;
            emit authenticationRequired(action, getActionDescription(action));
            return AuthResult::NotAuthorized;
        }
    } else {
        qCWarning(policyKitLog) << "Failed to check user groups";
        return AuthResult::Error;
    }
}

bool PolicyKitHelper::executePrivilegedAction(Action action, const QVariantMap &parameters)
{
    if (!m_isAvailable || !m_policyKitInterface) {
        qCWarning(policyKitLog) << "PolicyKit not available for privileged action";
        m_lastError = tr("PolicyKit is not available");
        emit actionCompleted(action, false, m_lastError);
        return false;
    }

    // Validate parameters
    if (!validateParameters(action, parameters)) {
        qCWarning(policyKitLog) << "Invalid parameters for action:" << static_cast<int>(action);
        m_lastError = tr("Invalid parameters for the requested action");
        emit actionCompleted(action, false, m_lastError);
        return false;
    }

    // Check authorization first
    AuthResult authResult = checkAuthorization(action);
    if (authResult != AuthResult::Authorized) {
        QString errorMsg;
        switch (authResult) {
        case AuthResult::NotAuthorized:
            errorMsg = tr("You are not authorized to perform this action");
            break;
        case AuthResult::AuthenticationFailed:
            errorMsg = tr("Authentication failed");
            break;
        case AuthResult::Cancelled:
            errorMsg = tr("Authentication was cancelled");
            break;
        case AuthResult::Error:
        default:
            errorMsg = m_lastError.isEmpty() ? tr("Authorization error") : m_lastError;
            break;
        }
        
        qCWarning(policyKitLog) << "Authorization failed for action:" << static_cast<int>(action) << errorMsg;
        emit actionCompleted(action, false, errorMsg);
        return false;
    }

    // Perform the privileged operation
    bool success = performPrivilegedOperation(action, parameters);
    
    if (success) {
        qCInfo(policyKitLog) << "Privileged action completed successfully:" << static_cast<int>(action);
    } else {
        qCWarning(policyKitLog) << "Privileged action failed:" << static_cast<int>(action) << m_lastError;
    }

    emit actionCompleted(action, success, success ? QString() : m_lastError);
    return success;
}

bool PolicyKitHelper::performPrivilegedOperation(Action action, const QVariantMap &parameters)
{
    switch (action) {
    case Action::CreateShare:
    case Action::ModifyShare:
    case Action::RemoveShare: {
        QString exportsFile = parameters.value("exportsFile", "/etc/exports").toString();
        QString shareEntry = parameters.value("shareEntry").toString();
        
        if (!createBackup(exportsFile)) {
            m_lastError = tr("Failed to create backup of exports file");
            return false;
        }

        // For this implementation, we'll use a simple approach
        // In a real implementation, you'd want more sophisticated exports file management
        if (action == Action::CreateShare || action == Action::ModifyShare) {
            QFile file(exportsFile);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                m_lastError = tr("Failed to open exports file for writing: %1").arg(file.errorString());
                return false;
            }
            
            QTextStream stream(&file);
            stream << shareEntry << "\n";
            file.close();
        }

        // Reload NFS exports
        return executeSystemCommand("exportfs", {"-ra"});
    }

    case Action::MountRemoteShare: {
        QString source = parameters.value("source").toString();
        QString target = parameters.value("target").toString();
        QStringList options = parameters.value("options").toStringList();
        
        // Create mount point if it doesn't exist
        QDir dir;
        if (!dir.mkpath(target)) {
            m_lastError = tr("Failed to create mount point: %1").arg(target);
            return false;
        }

        QStringList args = {"-t", "nfs"};
        if (!options.isEmpty()) {
            args << "-o" << options.join(",");
        }
        args << source << target;

        return executeSystemCommand("mount", args);
    }

    case Action::UnmountShare: {
        QString target = parameters.value("target").toString();
        return executeSystemCommand("umount", {target});
    }

    case Action::ModifyFstab: {
        QString fstabFile = "/etc/fstab";
        QString fstabEntry = parameters.value("fstabEntry").toString();
        
        if (!createBackup(fstabFile)) {
            m_lastError = tr("Failed to create backup of fstab");
            return false;
        }

        QFile file(fstabFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
            m_lastError = tr("Failed to open fstab for writing: %1").arg(file.errorString());
            return false;
        }
        
        QTextStream stream(&file);
        stream << fstabEntry << "\n";
        file.close();
        return true;
    }

    case Action::RestartNFSService: {
        // Try systemctl first, then fall back to service command
        if (executeSystemCommand("systemctl", {"restart", "nfs-server"})) {
            return true;
        }
        return executeSystemCommand("service", {"nfs-kernel-server", "restart"});
    }

    case Action::ModifySystemFiles: {
        QString filePath = parameters.value("filePath").toString();
        QString content = parameters.value("content").toString();
        
        if (!createBackup(filePath)) {
            m_lastError = tr("Failed to create backup of system file");
            return false;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            m_lastError = tr("Failed to open system file for writing: %1").arg(file.errorString());
            return false;
        }
        
        QTextStream stream(&file);
        stream << content;
        file.close();
        return true;
    }

    default:
        m_lastError = tr("Unknown action requested");
        return false;
    }
}

bool PolicyKitHelper::executeSystemCommand(const QString &command, const QStringList &arguments)
{
    QProcess process;
    process.start(command, arguments);
    
    if (!process.waitForStarted(5000)) {
        m_lastError = tr("Failed to start command: %1").arg(command);
        return false;
    }

    if (!process.waitForFinished(30000)) {
        m_lastError = tr("Command timed out: %1").arg(command);
        process.kill();
        return false;
    }

    if (process.exitCode() != 0) {
        QString errorOutput = process.readAllStandardError();
        m_lastError = tr("Command failed: %1\nError: %2").arg(command, errorOutput);
        return false;
    }

    return true;
}

bool PolicyKitHelper::createBackup(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        // File doesn't exist, no backup needed
        return true;
    }

    QString backupPath = filePath + ".backup." + 
                        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    
    if (!QFile::copy(filePath, backupPath)) {
        qCWarning(policyKitLog) << "Failed to create backup:" << filePath << "to" << backupPath;
        return false;
    }

    qCInfo(policyKitLog) << "Created backup:" << backupPath;
    return true;
}

bool PolicyKitHelper::validateParameters(Action action, const QVariantMap &parameters)
{
    switch (action) {
    case Action::CreateShare:
    case Action::ModifyShare:
        return parameters.contains("shareEntry") && 
               !parameters.value("shareEntry").toString().isEmpty();

    case Action::RemoveShare:
        return parameters.contains("sharePath") && 
               !parameters.value("sharePath").toString().isEmpty();

    case Action::MountRemoteShare:
        return parameters.contains("source") && parameters.contains("target") &&
               !parameters.value("source").toString().isEmpty() &&
               !parameters.value("target").toString().isEmpty();

    case Action::UnmountShare:
        return parameters.contains("target") && 
               !parameters.value("target").toString().isEmpty();

    case Action::ModifyFstab:
        return parameters.contains("fstabEntry") && 
               !parameters.value("fstabEntry").toString().isEmpty();

    case Action::ModifySystemFiles:
        return parameters.contains("filePath") && parameters.contains("content") &&
               !parameters.value("filePath").toString().isEmpty();

    case Action::RestartNFSService:
        return true; // No parameters needed

    default:
        return false;
    }
}

QString PolicyKitHelper::getActionId(Action action)
{
    switch (action) {
    case Action::CreateShare:
        return "org.kde.nfs-share-manager.create-share";
    case Action::RemoveShare:
        return "org.kde.nfs-share-manager.remove-share";
    case Action::ModifyShare:
        return "org.kde.nfs-share-manager.modify-share";
    case Action::MountRemoteShare:
        return "org.kde.nfs-share-manager.mount-share";
    case Action::UnmountShare:
        return "org.kde.nfs-share-manager.unmount-share";
    case Action::ModifyFstab:
        return "org.kde.nfs-share-manager.modify-fstab";
    case Action::RestartNFSService:
        return "org.kde.nfs-share-manager.restart-service";
    case Action::ModifySystemFiles:
        return "org.kde.nfs-share-manager.modify-system-files";
    default:
        return QString();
    }
}

QString PolicyKitHelper::getActionDescription(Action action)
{
    switch (action) {
    case Action::CreateShare:
        return tr("Create a new NFS share");
    case Action::RemoveShare:
        return tr("Remove an existing NFS share");
    case Action::ModifyShare:
        return tr("Modify NFS share configuration");
    case Action::MountRemoteShare:
        return tr("Mount a remote NFS share");
    case Action::UnmountShare:
        return tr("Unmount an NFS share");
    case Action::ModifyFstab:
        return tr("Modify system mount configuration");
    case Action::RestartNFSService:
        return tr("Restart the NFS service");
    case Action::ModifySystemFiles:
        return tr("Modify system configuration files");
    default:
        return tr("Unknown action");
    }
}

void PolicyKitHelper::onServiceAvailabilityChanged()
{
    QDBusConnection systemBus = QDBusConnection::systemBus();
    QDBusConnectionInterface *interface = systemBus.interface();
    
    bool wasAvailable = m_isAvailable;
    m_isAvailable = interface->isServiceRegistered("org.freedesktop.PolicyKit1");
    
    if (wasAvailable != m_isAvailable) {
        qCInfo(policyKitLog) << "PolicyKit availability changed:" << m_isAvailable;
        
        if (!m_isAvailable) {
            delete m_policyKitInterface;
            m_policyKitInterface = nullptr;
        } else {
            initializePolicyKit();
        }
    }
}

} // namespace NFSShareManager