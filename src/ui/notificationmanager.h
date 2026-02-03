#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QTimer>
#include <QSystemTrayIcon>

// Forward declarations for KDE Frameworks
// In a real KDE environment, these would be actual KNotification includes
// #include <KNotification>
// #include <KNotificationJobUiDelegate>

namespace NFSShareManager {

class ConfigurationManager;
struct NFSShare;
struct NFSMount;
struct RemoteNFSShare;

/**
 * @brief Notification types for different system events
 */
enum class NotificationType {
    // Share management notifications
    ShareCreated,
    ShareRemoved,
    ShareUpdated,
    ShareError,
    SharePermissionChanged,
    
    // Mount management notifications
    MountCompleted,
    MountFailed,
    UnmountCompleted,
    UnmountFailed,
    MountConnectionLost,
    
    // Network discovery notifications
    ShareDiscovered,
    ShareUnavailable,
    DiscoveryCompleted,
    DiscoveryError,
    NetworkChanged,
    
    // System notifications
    ServiceStarted,
    ServiceStopped,
    ServiceError,
    ConfigurationChanged,
    BackupCreated,
    
    // Operation notifications
    OperationStarted,
    OperationCompleted,
    OperationFailed,
    OperationCancelled,
    
    // Security notifications
    AuthenticationRequired,
    AuthenticationFailed,
    PermissionDenied,
    
    // General notifications
    Information,
    Warning,
    Error,
    Success
};

/**
 * @brief Notification urgency levels
 */
enum class NotificationUrgency {
    Low,        // Background information
    Normal,     // Standard notifications
    High,       // Important events requiring attention
    Critical    // Critical errors or security issues
};

/**
 * @brief Notification preferences structure
 */
struct NotificationPreferences {
    bool enableNotifications = true;
    bool enableSystemTrayNotifications = true;
    bool enableKNotifications = true;
    bool enableSoundNotifications = false;
    bool enablePersistentNotifications = false;
    
    // Per-category preferences
    bool enableShareNotifications = true;
    bool enableMountNotifications = true;
    bool enableDiscoveryNotifications = true;
    bool enableSystemNotifications = true;
    bool enableOperationNotifications = true;
    bool enableSecurityNotifications = true;
    
    // Urgency level filtering
    NotificationUrgency minimumUrgency = NotificationUrgency::Normal;
    
    // Timing preferences
    int defaultTimeout = 5000;  // milliseconds
    int errorTimeout = 10000;   // milliseconds
    int successTimeout = 3000;  // milliseconds
    
    // Grouping preferences
    bool groupSimilarNotifications = true;
    int groupingTimeWindow = 2000;  // milliseconds
};

/**
 * @brief Comprehensive notification manager for NFS Share Manager
 * 
 * This class provides a unified interface for all application notifications,
 * integrating with KDE's KNotifications framework while maintaining fallback
 * support for system tray notifications and message boxes.
 */
class NotificationManager : public QObject
{
    Q_OBJECT

public:
    explicit NotificationManager(ConfigurationManager *configManager, QObject *parent = nullptr);
    ~NotificationManager();

    /**
     * @brief Get current notification preferences
     * @return Current notification preferences
     */
    NotificationPreferences preferences() const { return m_preferences; }

    /**
     * @brief Set notification preferences
     * @param preferences New notification preferences
     */
    void setPreferences(const NotificationPreferences &preferences);

    /**
     * @brief Check if notifications are enabled for a specific type
     * @param type Notification type to check
     * @return True if notifications are enabled for this type
     */
    bool isNotificationEnabled(NotificationType type) const;

    /**
     * @brief Show a notification with automatic type detection
     * @param type Notification type
     * @param title Notification title
     * @param message Notification message
     * @param urgency Notification urgency level
     * @param timeout Custom timeout (0 = use default)
     * @param actions Optional actions for the notification
     */
    void showNotification(NotificationType type, const QString &title, const QString &message,
                         NotificationUrgency urgency = NotificationUrgency::Normal,
                         int timeout = 0, const QStringList &actions = QStringList());

    /**
     * @brief Show a simple information notification
     * @param message Notification message
     * @param timeout Custom timeout (0 = use default)
     */
    void showInfo(const QString &message, int timeout = 0);

    /**
     * @brief Show a warning notification
     * @param title Warning title
     * @param message Warning message
     * @param timeout Custom timeout (0 = use default)
     */
    void showWarning(const QString &title, const QString &message, int timeout = 0);

    /**
     * @brief Show an error notification
     * @param title Error title
     * @param message Error message
     * @param timeout Custom timeout (0 = use default)
     */
    void showError(const QString &title, const QString &message, int timeout = 0);

    /**
     * @brief Show a success notification
     * @param message Success message
     * @param timeout Custom timeout (0 = use default)
     */
    void showSuccess(const QString &message, int timeout = 0);

    // Specialized notification methods for different system events

    /**
     * @brief Show share-related notifications
     */
    void notifyShareCreated(const NFSShare &share);
    void notifyShareRemoved(const QString &sharePath);
    void notifyShareUpdated(const NFSShare &share);
    void notifyShareError(const QString &sharePath, const QString &error);

    /**
     * @brief Show mount-related notifications
     */
    void notifyMountCompleted(const NFSMount &mount);
    void notifyMountFailed(const RemoteNFSShare &remoteShare, const QString &mountPoint, const QString &error);
    void notifyUnmountCompleted(const QString &mountPoint);
    void notifyUnmountFailed(const QString &mountPoint, const QString &error);
    void notifyMountConnectionLost(const NFSMount &mount);

    /**
     * @brief Show discovery-related notifications
     */
    void notifyShareDiscovered(const RemoteNFSShare &share);
    void notifyShareUnavailable(const QString &hostAddress, const QString &exportPath);
    void notifyDiscoveryCompleted(int sharesFound, int hostsScanned);
    void notifyDiscoveryError(const QString &error);
    void notifyNetworkChanged(const QString &description);

    /**
     * @brief Show system-related notifications
     */
    void notifyServiceStarted(const QString &serviceName);
    void notifyServiceStopped(const QString &serviceName);
    void notifyServiceError(const QString &serviceName, const QString &error);
    void notifyConfigurationChanged(const QString &description);
    void notifyBackupCreated(const QString &backupPath);

    /**
     * @brief Show operation-related notifications
     */
    void notifyOperationStarted(const QString &operationName);
    void notifyOperationCompleted(const QString &operationName, const QString &result = QString());
    void notifyOperationFailed(const QString &operationName, const QString &error);
    void notifyOperationCancelled(const QString &operationName);

    /**
     * @brief Show security-related notifications
     */
    void notifyAuthenticationRequired(const QString &action);
    void notifyAuthenticationFailed(const QString &action);
    void notifyPermissionDenied(const QString &action);

    /**
     * @brief Set system tray icon for fallback notifications
     * @param trayIcon System tray icon instance
     */
    void setSystemTrayIcon(QSystemTrayIcon *trayIcon);

    /**
     * @brief Check if KNotifications is available
     * @return True if KNotifications is available and functional
     */
    bool isKNotificationsAvailable() const;

signals:
    /**
     * @brief Emitted when notification preferences change
     * @param preferences New notification preferences
     */
    void preferencesChanged(const NotificationPreferences &preferences);

    /**
     * @brief Emitted when a notification action is triggered
     * @param notificationId Notification identifier
     * @param actionId Action identifier
     */
    void notificationActionTriggered(const QString &notificationId, const QString &actionId);

    /**
     * @brief Emitted when a notification is closed
     * @param notificationId Notification identifier
     */
    void notificationClosed(const QString &notificationId);

private slots:
    /**
     * @brief Handle notification action activation
     * @param actionId Action identifier
     */
    void onNotificationActionActivated(const QString &actionId);

    /**
     * @brief Handle notification close events
     */
    void onNotificationClosed();

    /**
     * @brief Handle grouped notification timer
     */
    void onGroupingTimerTimeout();

private:
    /**
     * @brief Initialize notification system
     */
    void initializeNotifications();

    /**
     * @brief Load notification preferences from configuration
     */
    void loadPreferences();

    /**
     * @brief Save notification preferences to configuration
     */
    void savePreferences();

    /**
     * @brief Get notification category for a type
     * @param type Notification type
     * @return Category string for KNotifications
     */
    QString getNotificationCategory(NotificationType type) const;

    /**
     * @brief Get notification icon for a type
     * @param type Notification type
     * @return Icon name for the notification
     */
    QString getNotificationIcon(NotificationType type) const;

    /**
     * @brief Get default timeout for a notification type
     * @param type Notification type
     * @param urgency Notification urgency
     * @return Timeout in milliseconds
     */
    int getDefaultTimeout(NotificationType type, NotificationUrgency urgency) const;

    /**
     * @brief Check if notification should be grouped
     * @param type Notification type
     * @param title Notification title
     * @return True if notification should be grouped
     */
    bool shouldGroupNotification(NotificationType type, const QString &title) const;

    /**
     * @brief Show notification using KNotifications
     * @param type Notification type
     * @param title Notification title
     * @param message Notification message
     * @param urgency Notification urgency
     * @param timeout Notification timeout
     * @param actions Notification actions
     * @return Notification ID
     */
    QString showKNotification(NotificationType type, const QString &title, const QString &message,
                             NotificationUrgency urgency, int timeout, const QStringList &actions);

    /**
     * @brief Show notification using system tray (fallback)
     * @param type Notification type
     * @param title Notification title
     * @param message Notification message
     * @param timeout Notification timeout
     */
    void showSystemTrayNotification(NotificationType type, const QString &title, 
                                   const QString &message, int timeout);

    /**
     * @brief Show notification using message box (fallback)
     * @param type Notification type
     * @param title Notification title
     * @param message Notification message
     */
    void showMessageBoxNotification(NotificationType type, const QString &title, const QString &message);

    /**
     * @brief Format notification message with context
     * @param baseMessage Base message
     * @param context Additional context information
     * @return Formatted message
     */
    QString formatNotificationMessage(const QString &baseMessage, const QVariantMap &context = QVariantMap()) const;

    // Configuration and preferences
    ConfigurationManager *m_configurationManager;
    NotificationPreferences m_preferences;

    // System integration
    QSystemTrayIcon *m_systemTrayIcon;
    bool m_knotificationsAvailable;

    // Notification grouping
    QTimer *m_groupingTimer;
    QMap<QString, QStringList> m_pendingGroupedNotifications;
    QMap<QString, QDateTime> m_lastNotificationTime;

    // Active notifications tracking
    QMap<QString, QVariantMap> m_activeNotifications;
    int m_notificationCounter;
};

} // namespace NFSShareManager