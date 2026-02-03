#include "notificationmanager.h"
#include "../core/configurationmanager.h"
#include "../core/nfsshare.h"
#include "../core/nfsmount.h"
#include "../core/remotenfsshare.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>

// In a real KDE environment, these would be actual KNotification includes
// #include <KNotification>
// #include <KNotificationJobUiDelegate>

namespace NFSShareManager {

NotificationManager::NotificationManager(ConfigurationManager *configManager, QObject *parent)
    : QObject(parent)
    , m_configurationManager(configManager)
    , m_systemTrayIcon(nullptr)
    , m_knotificationsAvailable(false)
    , m_groupingTimer(new QTimer(this))
    , m_notificationCounter(0)
{
    initializeNotifications();
    loadPreferences();
    
    // Set up grouping timer
    m_groupingTimer->setSingleShot(true);
    connect(m_groupingTimer, &QTimer::timeout, this, &NotificationManager::onGroupingTimerTimeout);
}

NotificationManager::~NotificationManager()
{
    savePreferences();
}

void NotificationManager::initializeNotifications()
{
    // Check if KNotifications is available
    // In a real KDE environment, this would check for KNotification availability
    m_knotificationsAvailable = false; // Set to false for now since KF6 is not available
    
    qDebug() << "NotificationManager initialized. KNotifications available:" << m_knotificationsAvailable;
}

void NotificationManager::loadPreferences()
{
    if (!m_configurationManager) {
        qWarning() << "No configuration manager available for loading notification preferences";
        return;
    }

    // Load preferences from configuration using individual preference methods
    m_preferences.enableNotifications = m_configurationManager->getPreference("notifications/enabled", true).toBool();
    m_preferences.enableSystemTrayNotifications = m_configurationManager->getPreference("notifications/systemTray", true).toBool();
    m_preferences.enableKNotifications = m_configurationManager->getPreference("notifications/knotifications", true).toBool();
    m_preferences.enableSoundNotifications = m_configurationManager->getPreference("notifications/sound", false).toBool();
    m_preferences.enablePersistentNotifications = m_configurationManager->getPreference("notifications/persistent", false).toBool();
    
    // Category preferences
    m_preferences.enableShareNotifications = m_configurationManager->getPreference("notifications/shares", true).toBool();
    m_preferences.enableMountNotifications = m_configurationManager->getPreference("notifications/mounts", true).toBool();
    m_preferences.enableDiscoveryNotifications = m_configurationManager->getPreference("notifications/discovery", true).toBool();
    m_preferences.enableSystemNotifications = m_configurationManager->getPreference("notifications/system", true).toBool();
    m_preferences.enableOperationNotifications = m_configurationManager->getPreference("notifications/operations", true).toBool();
    m_preferences.enableSecurityNotifications = m_configurationManager->getPreference("notifications/security", true).toBool();
    
    // Urgency and timing
    int urgencyValue = m_configurationManager->getPreference("notifications/minimumUrgency", static_cast<int>(NotificationUrgency::Normal)).toInt();
    m_preferences.minimumUrgency = static_cast<NotificationUrgency>(urgencyValue);
    
    m_preferences.defaultTimeout = m_configurationManager->getPreference("notifications/defaultTimeout", 5000).toInt();
    m_preferences.errorTimeout = m_configurationManager->getPreference("notifications/errorTimeout", 10000).toInt();
    m_preferences.successTimeout = m_configurationManager->getPreference("notifications/successTimeout", 3000).toInt();
    
    // Grouping preferences
    m_preferences.groupSimilarNotifications = m_configurationManager->getPreference("notifications/groupSimilar", true).toBool();
    m_preferences.groupingTimeWindow = m_configurationManager->getPreference("notifications/groupingWindow", 2000).toInt();
    
    qDebug() << "Loaded notification preferences. Notifications enabled:" << m_preferences.enableNotifications;
}

void NotificationManager::savePreferences()
{
    if (!m_configurationManager) {
        qWarning() << "No configuration manager available for saving notification preferences";
        return;
    }

    // Save preferences using individual preference methods
    m_configurationManager->setPreference("notifications/enabled", m_preferences.enableNotifications);
    m_configurationManager->setPreference("notifications/systemTray", m_preferences.enableSystemTrayNotifications);
    m_configurationManager->setPreference("notifications/knotifications", m_preferences.enableKNotifications);
    m_configurationManager->setPreference("notifications/sound", m_preferences.enableSoundNotifications);
    m_configurationManager->setPreference("notifications/persistent", m_preferences.enablePersistentNotifications);
    
    // Category preferences
    m_configurationManager->setPreference("notifications/shares", m_preferences.enableShareNotifications);
    m_configurationManager->setPreference("notifications/mounts", m_preferences.enableMountNotifications);
    m_configurationManager->setPreference("notifications/discovery", m_preferences.enableDiscoveryNotifications);
    m_configurationManager->setPreference("notifications/system", m_preferences.enableSystemNotifications);
    m_configurationManager->setPreference("notifications/operations", m_preferences.enableOperationNotifications);
    m_configurationManager->setPreference("notifications/security", m_preferences.enableSecurityNotifications);
    
    // Urgency and timing
    m_configurationManager->setPreference("notifications/minimumUrgency", static_cast<int>(m_preferences.minimumUrgency));
    m_configurationManager->setPreference("notifications/defaultTimeout", m_preferences.defaultTimeout);
    m_configurationManager->setPreference("notifications/errorTimeout", m_preferences.errorTimeout);
    m_configurationManager->setPreference("notifications/successTimeout", m_preferences.successTimeout);
    
    // Grouping preferences
    m_configurationManager->setPreference("notifications/groupSimilar", m_preferences.groupSimilarNotifications);
    m_configurationManager->setPreference("notifications/groupingWindow", m_preferences.groupingTimeWindow);
    
    // Save configuration to persistent storage
    m_configurationManager->saveConfiguration();
    
    qDebug() << "Saved notification preferences";
}

void NotificationManager::setPreferences(const NotificationPreferences &preferences)
{
    m_preferences = preferences;
    savePreferences();
    emit preferencesChanged(m_preferences);
}

bool NotificationManager::isNotificationEnabled(NotificationType type) const
{
    if (!m_preferences.enableNotifications) {
        return false;
    }
    
    // Check category-specific preferences
    switch (type) {
        case NotificationType::ShareCreated:
        case NotificationType::ShareRemoved:
        case NotificationType::ShareUpdated:
        case NotificationType::ShareError:
        case NotificationType::SharePermissionChanged:
            return m_preferences.enableShareNotifications;
            
        case NotificationType::MountCompleted:
        case NotificationType::MountFailed:
        case NotificationType::UnmountCompleted:
        case NotificationType::UnmountFailed:
        case NotificationType::MountConnectionLost:
            return m_preferences.enableMountNotifications;
            
        case NotificationType::ShareDiscovered:
        case NotificationType::ShareUnavailable:
        case NotificationType::DiscoveryCompleted:
        case NotificationType::DiscoveryError:
        case NotificationType::NetworkChanged:
            return m_preferences.enableDiscoveryNotifications;
            
        case NotificationType::ServiceStarted:
        case NotificationType::ServiceStopped:
        case NotificationType::ServiceError:
        case NotificationType::ConfigurationChanged:
        case NotificationType::BackupCreated:
            return m_preferences.enableSystemNotifications;
            
        case NotificationType::OperationStarted:
        case NotificationType::OperationCompleted:
        case NotificationType::OperationFailed:
        case NotificationType::OperationCancelled:
            return m_preferences.enableOperationNotifications;
            
        case NotificationType::AuthenticationRequired:
        case NotificationType::AuthenticationFailed:
        case NotificationType::PermissionDenied:
            return m_preferences.enableSecurityNotifications;
            
        default:
            return true; // Enable general notifications by default
    }
}

void NotificationManager::showNotification(NotificationType type, const QString &title, const QString &message,
                                         NotificationUrgency urgency, int timeout, const QStringList &actions)
{
    if (!isNotificationEnabled(type)) {
        return;
    }
    
    // Check urgency filtering
    if (urgency < m_preferences.minimumUrgency) {
        return;
    }
    
    // Use default timeout if not specified
    if (timeout == 0) {
        timeout = getDefaultTimeout(type, urgency);
    }
    
    // Check if notification should be grouped
    if (m_preferences.groupSimilarNotifications && shouldGroupNotification(type, title)) {
        QString groupKey = QString("%1:%2").arg(static_cast<int>(type)).arg(title);
        m_pendingGroupedNotifications[groupKey].append(message);
        m_lastNotificationTime[groupKey] = QDateTime::currentDateTime();
        
        // Start or restart grouping timer
        m_groupingTimer->start(m_preferences.groupingTimeWindow);
        return;
    }
    
    // Show notification immediately
    QString notificationId;
    
    if (m_knotificationsAvailable && m_preferences.enableKNotifications) {
        notificationId = showKNotification(type, title, message, urgency, timeout, actions);
    } else if (m_systemTrayIcon && m_preferences.enableSystemTrayNotifications) {
        showSystemTrayNotification(type, title, message, timeout);
    } else {
        // Fallback to message box for critical notifications
        if (urgency >= NotificationUrgency::High) {
            showMessageBoxNotification(type, title, message);
        }
    }
    
    // Track active notification
    if (!notificationId.isEmpty()) {
        QVariantMap notificationData;
        notificationData["type"] = static_cast<int>(type);
        notificationData["title"] = title;
        notificationData["message"] = message;
        notificationData["urgency"] = static_cast<int>(urgency);
        notificationData["timestamp"] = QDateTime::currentDateTime();
        m_activeNotifications[notificationId] = notificationData;
    }
    
    qDebug() << "Showed notification:" << title << "-" << message;
}

void NotificationManager::showInfo(const QString &message, int timeout)
{
    showNotification(NotificationType::Information, tr("Information"), message, 
                    NotificationUrgency::Normal, timeout);
}

void NotificationManager::showWarning(const QString &title, const QString &message, int timeout)
{
    showNotification(NotificationType::Warning, title, message, 
                    NotificationUrgency::High, timeout);
}

void NotificationManager::showError(const QString &title, const QString &message, int timeout)
{
    showNotification(NotificationType::Error, title, message, 
                    NotificationUrgency::Critical, timeout);
}

void NotificationManager::showSuccess(const QString &message, int timeout)
{
    showNotification(NotificationType::Success, tr("Success"), message, 
                    NotificationUrgency::Normal, timeout);
}

// Specialized notification methods

void NotificationManager::notifyShareCreated(const NFSShare &share)
{
    QString message = tr("NFS share created successfully: %1").arg(share.path());
    showNotification(NotificationType::ShareCreated, tr("Share Created"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyShareRemoved(const QString &sharePath)
{
    QString message = tr("NFS share removed: %1").arg(sharePath);
    showNotification(NotificationType::ShareRemoved, tr("Share Removed"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyShareUpdated(const NFSShare &share)
{
    QString message = tr("NFS share updated: %1").arg(share.path());
    showNotification(NotificationType::ShareUpdated, tr("Share Updated"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyShareError(const QString &sharePath, const QString &error)
{
    QString message = tr("Error with NFS share %1: %2").arg(sharePath, error);
    showNotification(NotificationType::ShareError, tr("Share Error"), message, 
                    NotificationUrgency::High);
}

void NotificationManager::notifyMountCompleted(const NFSMount &mount)
{
    QString message = tr("Successfully mounted %1:%2 at %3")
                     .arg(mount.remoteShare().hostName(), mount.remoteShare().exportPath(), mount.localMountPoint());
    showNotification(NotificationType::MountCompleted, tr("Mount Completed"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyMountFailed(const RemoteNFSShare &remoteShare, const QString &mountPoint, const QString &error)
{
    QString message = tr("Failed to mount %1:%2 at %3: %4")
                     .arg(remoteShare.hostName(), remoteShare.exportPath(), mountPoint, error);
    showNotification(NotificationType::MountFailed, tr("Mount Failed"), message, 
                    NotificationUrgency::High);
}

void NotificationManager::notifyUnmountCompleted(const QString &mountPoint)
{
    QString message = tr("Successfully unmounted %1").arg(mountPoint);
    showNotification(NotificationType::UnmountCompleted, tr("Unmount Completed"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyUnmountFailed(const QString &mountPoint, const QString &error)
{
    QString message = tr("Failed to unmount %1: %2").arg(mountPoint, error);
    showNotification(NotificationType::UnmountFailed, tr("Unmount Failed"), message, 
                    NotificationUrgency::High);
}

void NotificationManager::notifyMountConnectionLost(const NFSMount &mount)
{
    QString message = tr("Connection lost to mounted share %1:%2")
                     .arg(mount.remoteShare().hostName(), mount.remoteShare().exportPath());
    showNotification(NotificationType::MountConnectionLost, tr("Connection Lost"), message, 
                    NotificationUrgency::High);
}

void NotificationManager::notifyShareDiscovered(const RemoteNFSShare &share)
{
    QString message = tr("Discovered NFS share: %1:%2").arg(share.hostName(), share.exportPath());
    showNotification(NotificationType::ShareDiscovered, tr("Share Discovered"), message, 
                    NotificationUrgency::Low);
}

void NotificationManager::notifyShareUnavailable(const QString &hostAddress, const QString &exportPath)
{
    QString message = tr("NFS share no longer available: %1:%2").arg(hostAddress, exportPath);
    showNotification(NotificationType::ShareUnavailable, tr("Share Unavailable"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyDiscoveryCompleted(int sharesFound, int hostsScanned)
{
    QString message = tr("Network discovery completed. Found %1 shares on %2 hosts.")
                     .arg(sharesFound).arg(hostsScanned);
    showNotification(NotificationType::DiscoveryCompleted, tr("Discovery Completed"), message, 
                    NotificationUrgency::Low);
}

void NotificationManager::notifyDiscoveryError(const QString &error)
{
    QString message = tr("Network discovery error: %1").arg(error);
    showNotification(NotificationType::DiscoveryError, tr("Discovery Error"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyNetworkChanged(const QString &description)
{
    QString message = tr("Network configuration changed: %1").arg(description);
    showNotification(NotificationType::NetworkChanged, tr("Network Changed"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyServiceStarted(const QString &serviceName)
{
    QString message = tr("Service started: %1").arg(serviceName);
    showNotification(NotificationType::ServiceStarted, tr("Service Started"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyServiceStopped(const QString &serviceName)
{
    QString message = tr("Service stopped: %1").arg(serviceName);
    showNotification(NotificationType::ServiceStopped, tr("Service Stopped"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyServiceError(const QString &serviceName, const QString &error)
{
    QString message = tr("Service error in %1: %2").arg(serviceName, error);
    showNotification(NotificationType::ServiceError, tr("Service Error"), message, 
                    NotificationUrgency::High);
}

void NotificationManager::notifyConfigurationChanged(const QString &description)
{
    QString message = tr("Configuration changed: %1").arg(description);
    showNotification(NotificationType::ConfigurationChanged, tr("Configuration Changed"), message, 
                    NotificationUrgency::Low);
}

void NotificationManager::notifyBackupCreated(const QString &backupPath)
{
    QString message = tr("Configuration backup created: %1").arg(backupPath);
    showNotification(NotificationType::BackupCreated, tr("Backup Created"), message, 
                    NotificationUrgency::Low);
}

void NotificationManager::notifyOperationStarted(const QString &operationName)
{
    QString message = tr("Operation started: %1").arg(operationName);
    showNotification(NotificationType::OperationStarted, tr("Operation Started"), message, 
                    NotificationUrgency::Low);
}

void NotificationManager::notifyOperationCompleted(const QString &operationName, const QString &result)
{
    QString message = result.isEmpty() ? 
                     tr("Operation completed: %1").arg(operationName) :
                     tr("Operation completed: %1 - %2").arg(operationName, result);
    showNotification(NotificationType::OperationCompleted, tr("Operation Completed"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyOperationFailed(const QString &operationName, const QString &error)
{
    QString message = tr("Operation failed: %1 - %2").arg(operationName, error);
    showNotification(NotificationType::OperationFailed, tr("Operation Failed"), message, 
                    NotificationUrgency::High);
}

void NotificationManager::notifyOperationCancelled(const QString &operationName)
{
    QString message = tr("Operation cancelled: %1").arg(operationName);
    showNotification(NotificationType::OperationCancelled, tr("Operation Cancelled"), message, 
                    NotificationUrgency::Normal);
}

void NotificationManager::notifyAuthenticationRequired(const QString &action)
{
    QString message = tr("Authentication required for: %1").arg(action);
    showNotification(NotificationType::AuthenticationRequired, tr("Authentication Required"), message, 
                    NotificationUrgency::High);
}

void NotificationManager::notifyAuthenticationFailed(const QString &action)
{
    QString message = tr("Authentication failed for: %1").arg(action);
    showNotification(NotificationType::AuthenticationFailed, tr("Authentication Failed"), message, 
                    NotificationUrgency::Critical);
}

void NotificationManager::notifyPermissionDenied(const QString &action)
{
    QString message = tr("Permission denied for: %1").arg(action);
    showNotification(NotificationType::PermissionDenied, tr("Permission Denied"), message, 
                    NotificationUrgency::Critical);
}

void NotificationManager::setSystemTrayIcon(QSystemTrayIcon *trayIcon)
{
    m_systemTrayIcon = trayIcon;
}

bool NotificationManager::isKNotificationsAvailable() const
{
    return m_knotificationsAvailable;
}

// Private helper methods

QString NotificationManager::getNotificationCategory(NotificationType type) const
{
    switch (type) {
        case NotificationType::ShareCreated:
        case NotificationType::ShareRemoved:
        case NotificationType::ShareUpdated:
        case NotificationType::ShareError:
        case NotificationType::SharePermissionChanged:
            return "shares";
            
        case NotificationType::MountCompleted:
        case NotificationType::MountFailed:
        case NotificationType::UnmountCompleted:
        case NotificationType::UnmountFailed:
        case NotificationType::MountConnectionLost:
            return "mounts";
            
        case NotificationType::ShareDiscovered:
        case NotificationType::ShareUnavailable:
        case NotificationType::DiscoveryCompleted:
        case NotificationType::DiscoveryError:
        case NotificationType::NetworkChanged:
            return "discovery";
            
        case NotificationType::ServiceStarted:
        case NotificationType::ServiceStopped:
        case NotificationType::ServiceError:
        case NotificationType::ConfigurationChanged:
        case NotificationType::BackupCreated:
            return "system";
            
        case NotificationType::OperationStarted:
        case NotificationType::OperationCompleted:
        case NotificationType::OperationFailed:
        case NotificationType::OperationCancelled:
            return "operations";
            
        case NotificationType::AuthenticationRequired:
        case NotificationType::AuthenticationFailed:
        case NotificationType::PermissionDenied:
            return "security";
            
        default:
            return "general";
    }
}

QString NotificationManager::getNotificationIcon(NotificationType type) const
{
    switch (type) {
        case NotificationType::ShareCreated:
        case NotificationType::ShareUpdated:
            return "folder-network";
            
        case NotificationType::ShareRemoved:
            return "edit-delete";
            
        case NotificationType::ShareError:
        case NotificationType::MountFailed:
        case NotificationType::UnmountFailed:
        case NotificationType::ServiceError:
        case NotificationType::DiscoveryError:
        case NotificationType::OperationFailed:
        case NotificationType::Error:
            return "dialog-error";
            
        case NotificationType::MountCompleted:
        case NotificationType::UnmountCompleted:
            return "drive-harddisk";
            
        case NotificationType::MountConnectionLost:
            return "network-disconnect";
            
        case NotificationType::ShareDiscovered:
            return "network-server";
            
        case NotificationType::ShareUnavailable:
            return "network-offline";
            
        case NotificationType::DiscoveryCompleted:
            return "view-refresh";
            
        case NotificationType::NetworkChanged:
            return "network-wired";
            
        case NotificationType::ServiceStarted:
        case NotificationType::ServiceStopped:
            return "system-run";
            
        case NotificationType::ConfigurationChanged:
        case NotificationType::BackupCreated:
            return "configure";
            
        case NotificationType::OperationStarted:
        case NotificationType::OperationCompleted:
        case NotificationType::Success:
            return "dialog-ok";
            
        case NotificationType::OperationCancelled:
            return "dialog-cancel";
            
        case NotificationType::AuthenticationRequired:
        case NotificationType::AuthenticationFailed:
        case NotificationType::PermissionDenied:
            return "dialog-password";
            
        case NotificationType::Warning:
            return "dialog-warning";
            
        default:
            return "dialog-information";
    }
}

int NotificationManager::getDefaultTimeout(NotificationType type, NotificationUrgency urgency) const
{
    // Use urgency-based timeouts first
    switch (urgency) {
        case NotificationUrgency::Low:
            return m_preferences.successTimeout;
        case NotificationUrgency::Normal:
            return m_preferences.defaultTimeout;
        case NotificationUrgency::High:
        case NotificationUrgency::Critical:
            return m_preferences.errorTimeout;
    }
    
    // Fallback to type-based timeouts
    switch (type) {
        case NotificationType::ShareError:
        case NotificationType::MountFailed:
        case NotificationType::UnmountFailed:
        case NotificationType::ServiceError:
        case NotificationType::DiscoveryError:
        case NotificationType::OperationFailed:
        case NotificationType::AuthenticationFailed:
        case NotificationType::PermissionDenied:
        case NotificationType::Error:
            return m_preferences.errorTimeout;
            
        case NotificationType::ShareCreated:
        case NotificationType::ShareRemoved:
        case NotificationType::MountCompleted:
        case NotificationType::UnmountCompleted:
        case NotificationType::OperationCompleted:
        case NotificationType::Success:
            return m_preferences.successTimeout;
            
        default:
            return m_preferences.defaultTimeout;
    }
}

bool NotificationManager::shouldGroupNotification(NotificationType type, const QString &title) const
{
    if (!m_preferences.groupSimilarNotifications) {
        return false;
    }
    
    // Group similar discovery and operation notifications
    switch (type) {
        case NotificationType::ShareDiscovered:
        case NotificationType::ShareUnavailable:
        case NotificationType::OperationStarted:
        case NotificationType::OperationCompleted:
            return true;
        default:
            return false;
    }
}

QString NotificationManager::showKNotification(NotificationType type, const QString &title, const QString &message,
                                              NotificationUrgency urgency, int timeout, const QStringList &actions)
{
    // In a real KDE environment, this would use KNotification
    // For now, we'll simulate the behavior and return a notification ID
    
    QString notificationId = QString("notification_%1").arg(++m_notificationCounter);
    
    qDebug() << "KNotification (simulated):" << title << "-" << message 
             << "Category:" << getNotificationCategory(type)
             << "Icon:" << getNotificationIcon(type)
             << "Urgency:" << static_cast<int>(urgency)
             << "Timeout:" << timeout;
    
    // In a real implementation:
    /*
    KNotification *notification = new KNotification(getNotificationCategory(type));
    notification->setTitle(title);
    notification->setText(message);
    notification->setIconName(getNotificationIcon(type));
    
    // Set urgency
    switch (urgency) {
        case NotificationUrgency::Low:
            notification->setUrgency(KNotification::LowUrgency);
            break;
        case NotificationUrgency::Normal:
            notification->setUrgency(KNotification::NormalUrgency);
            break;
        case NotificationUrgency::High:
            notification->setUrgency(KNotification::HighUrgency);
            break;
        case NotificationUrgency::Critical:
            notification->setUrgency(KNotification::CriticalUrgency);
            break;
    }
    
    // Add actions
    for (const QString &action : actions) {
        notification->setActions(QStringList() << action);
    }
    
    // Set timeout
    if (timeout > 0) {
        notification->setTimeout(timeout);
    }
    
    // Connect signals
    connect(notification, &KNotification::action1Activated, this, &NotificationManager::onNotificationActionActivated);
    connect(notification, &KNotification::closed, this, &NotificationManager::onNotificationClosed);
    
    // Send notification
    notification->sendEvent();
    
    return QString::number(reinterpret_cast<quintptr>(notification));
    */
    
    return notificationId;
}

void NotificationManager::showSystemTrayNotification(NotificationType type, const QString &title, 
                                                    const QString &message, int timeout)
{
    if (!m_systemTrayIcon || !m_systemTrayIcon->isVisible()) {
        return;
    }
    
    QSystemTrayIcon::MessageIcon icon;
    switch (type) {
        case NotificationType::Error:
        case NotificationType::ShareError:
        case NotificationType::MountFailed:
        case NotificationType::UnmountFailed:
        case NotificationType::ServiceError:
        case NotificationType::DiscoveryError:
        case NotificationType::OperationFailed:
        case NotificationType::AuthenticationFailed:
        case NotificationType::PermissionDenied:
            icon = QSystemTrayIcon::Critical;
            break;
            
        case NotificationType::Warning:
        case NotificationType::MountConnectionLost:
        case NotificationType::ShareUnavailable:
        case NotificationType::AuthenticationRequired:
            icon = QSystemTrayIcon::Warning;
            break;
            
        default:
            icon = QSystemTrayIcon::Information;
            break;
    }
    
    m_systemTrayIcon->showMessage(title, message, icon, timeout);
    
    qDebug() << "System tray notification:" << title << "-" << message;
}

void NotificationManager::showMessageBoxNotification(NotificationType type, const QString &title, const QString &message)
{
    QMessageBox::Icon icon;
    switch (type) {
        case NotificationType::Error:
        case NotificationType::ShareError:
        case NotificationType::MountFailed:
        case NotificationType::UnmountFailed:
        case NotificationType::ServiceError:
        case NotificationType::DiscoveryError:
        case NotificationType::OperationFailed:
        case NotificationType::AuthenticationFailed:
        case NotificationType::PermissionDenied:
            icon = QMessageBox::Critical;
            break;
            
        case NotificationType::Warning:
        case NotificationType::MountConnectionLost:
        case NotificationType::ShareUnavailable:
        case NotificationType::AuthenticationRequired:
            icon = QMessageBox::Warning;
            break;
            
        default:
            icon = QMessageBox::Information;
            break;
    }
    
    QMessageBox msgBox;
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.exec();
    
    qDebug() << "Message box notification:" << title << "-" << message;
}

QString NotificationManager::formatNotificationMessage(const QString &baseMessage, const QVariantMap &context) const
{
    QString formattedMessage = baseMessage;
    
    // Add timestamp if requested
    if (context.value("includeTimestamp", false).toBool()) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
        formattedMessage = QString("[%1] %2").arg(timestamp, formattedMessage);
    }
    
    // Add context information
    if (context.contains("host")) {
        formattedMessage += QString(" (Host: %1)").arg(context.value("host").toString());
    }
    
    if (context.contains("path")) {
        formattedMessage += QString(" (Path: %1)").arg(context.value("path").toString());
    }
    
    return formattedMessage;
}

void NotificationManager::onNotificationActionActivated(const QString &actionId)
{
    // Find the notification that triggered this action
    QString notificationId;
    for (auto it = m_activeNotifications.begin(); it != m_activeNotifications.end(); ++it) {
        // In a real implementation, we would track which notification triggered the action
        notificationId = it.key();
        break;
    }
    
    emit notificationActionTriggered(notificationId, actionId);
    
    qDebug() << "Notification action activated:" << actionId;
}

void NotificationManager::onNotificationClosed()
{
    // In a real implementation, we would identify which notification was closed
    QString notificationId = QString("notification_%1").arg(m_notificationCounter);
    
    m_activeNotifications.remove(notificationId);
    emit notificationClosed(notificationId);
    
    qDebug() << "Notification closed:" << notificationId;
}

void NotificationManager::onGroupingTimerTimeout()
{
    // Process all pending grouped notifications
    for (auto it = m_pendingGroupedNotifications.begin(); it != m_pendingGroupedNotifications.end(); ++it) {
        const QString &groupKey = it.key();
        const QStringList &messages = it.value();
        
        if (messages.isEmpty()) {
            continue;
        }
        
        // Parse group key
        QStringList keyParts = groupKey.split(':');
        if (keyParts.size() != 2) {
            continue;
        }
        
        NotificationType type = static_cast<NotificationType>(keyParts[0].toInt());
        QString title = keyParts[1];
        
        // Create grouped message
        QString groupedMessage;
        if (messages.size() == 1) {
            groupedMessage = messages.first();
        } else {
            groupedMessage = tr("%1 similar events occurred").arg(messages.size());
            if (messages.size() <= 5) {
                groupedMessage += ":\n" + messages.join("\n");
            }
        }
        
        // Show the grouped notification
        showNotification(type, title, groupedMessage, NotificationUrgency::Normal, 0, QStringList());
    }
    
    // Clear pending notifications
    m_pendingGroupedNotifications.clear();
}

} // namespace NFSShareManager