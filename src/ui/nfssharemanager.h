#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QStatusBar>
#include <QTimer>
#include <QUuid>

#include "../business/networkdiscovery.h"

namespace NFSShareManager {

class ConfigurationManager;
class ShareManager;
class MountManager;
class NetworkDiscovery;
class PermissionManager;
class NotificationManager;
class StartupManager;
struct ValidationResult;
struct NFSShare;
struct NFSMount;
struct RemoteNFSShare;

/**
 * @brief Main application class for NFS Share Manager
 * 
 * This class serves as the main entry point and coordinator for the
 * NFS Share Manager application. It provides a comprehensive GUI for
 * managing local NFS shares and remote mounts following KDE design guidelines.
 */
class NFSShareManagerApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit NFSShareManagerApp(QWidget *parent = nullptr);
    ~NFSShareManagerApp();

    /**
     * @brief Get the configuration manager instance
     * @return Pointer to configuration manager
     */
    ConfigurationManager* configurationManager() const { return m_configurationManager; }

    /**
     * @brief Get the share manager instance
     * @return Pointer to share manager
     */
    ShareManager* shareManager() const { return m_shareManager; }

    /**
     * @brief Get the mount manager instance
     * @return Pointer to mount manager
     */
    MountManager* mountManager() const { return m_mountManager; }

    /**
     * @brief Get the network discovery instance
     * @return Pointer to network discovery
     */
    NetworkDiscovery* networkDiscovery() const { return m_networkDiscovery; }

    /**
     * @brief Get the notification manager instance
     * @return Pointer to notification manager
     */
    NotificationManager* notificationManager() const { return m_notificationManager; }

    /**
     * @brief Show the main window and bring to front
     */
    void showMainWindow();

    /**
     * @brief Hide to system tray if available
     */
    void hideToTray();

public slots:
    /**
     * @brief Export current configuration to file
     * @param filePath Path to export file
     * @param profileName Name for the exported profile
     * @param description Description for the exported profile
     * @return True if export was successful
     */
    bool exportConfiguration(const QString &filePath, const QString &profileName, 
                           const QString &description = QString());

    /**
     * @brief Import configuration from file
     * @param filePath Path to import file
     * @param mergeMode If true, merge with existing config; if false, replace
     * @return True if import was successful
     */
    bool importConfiguration(const QString &filePath, bool mergeMode = false);

    /**
     * @brief Refresh all data (shares, mounts, discovery)
     */
    void refreshAll();

    /**
     * @brief Show application about dialog
     */
    void showAbout();

    /**
     * @brief Show application preferences dialog
     */
    void showPreferences();

    /**
     * @brief Show notification preferences dialog
     */
    void showNotificationPreferences();

    /**
     * @brief Quit the application completely
     */
    void quitApplication();

protected:
    /**
     * @brief Handle close event (minimize to tray if available)
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief Handle window state change events
     */
    void changeEvent(QEvent *event) override;

private slots:
    // Configuration management slots
    void onConfigurationValidationFailed(const ValidationResult &result);
    void onConfigurationRepaired(const QStringList &repairActions);
    void onBackupCreated(const QString &backupPath);

    // Share management slots
    void onShareCreated(const NFSShare &share);
    void onShareRemoved(const QString &sharePath);
    void onShareUpdated(const NFSShare &share);
    void onShareError(const QString &path, const QString &error);
    void onSharesRefreshed();
    void onNFSServerStatusChanged(bool running);

    // Mount management slots
    void onMountStarted(const RemoteNFSShare &remoteShare, const QString &mountPoint);
    void onMountCompleted(const NFSMount &mount);
    void onMountFailed(const RemoteNFSShare &remoteShare, const QString &mountPoint, 
                       int result, const QString &errorMessage);
    void onUnmountStarted(const QString &mountPoint);
    void onUnmountCompleted(const QString &mountPoint);
    void onUnmountFailed(const QString &mountPoint, const QString &errorMessage);
    void onMountStatusChanged(const NFSMount &mount);

    // Network discovery slots
    void onShareDiscovered(const RemoteNFSShare &share);
    void onShareUnavailable(const QString &hostAddress, const QString &exportPath);
    void onDiscoveryCompleted(int sharesFound, int hostsScanned);
    void onDiscoveryError(const QString &error);
    void onDiscoveryStarted(NetworkDiscovery::ScanMode mode);
    void onDiscoveryStatusChanged(NetworkDiscovery::DiscoveryStatus status);
    void onScanProgress(int current, int total, const QString &hostAddress);

    // System tray slots
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayIconMessageClicked();

    // UI action slots
    void onCreateShareClicked();
    void onRemoveShareClicked();
    void onEditShareClicked();
    void onMountShareClicked();
    void onUnmountShareClicked();
    void onRefreshDiscoveryClicked();
    void onDiscoveryModeClicked();
    void onManageTargetsClicked();
    void onAutoDiscoveryToggled(bool enabled);
    void onExportConfigurationClicked();
    void onImportConfigurationClicked();

    // Operation manager slots
    void onOperationStarted(const QUuid &operationId, const QString &title);
    void onOperationProgressUpdated(const QUuid &operationId, int progress, const QString &statusMessage);
    void onOperationCompleted(const QUuid &operationId, const QString &message);
    void onOperationFailed(const QUuid &operationId, const QString &errorMessage);
    void onOperationCancelled(const QUuid &operationId);

    // Status update timer
    void onStatusUpdateTimer();
    void checkComponentHealth();
    void onDiscoveryTimeout();
    void onCancelDiscoveryClicked();

private:
    // UI setup methods
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupSystemTray();
    void setupCentralWidget();
    void setupLocalSharesTab();
    void setupRemoteSharesTab();
    void setupMountedSharesTab();

    // Configuration and initialization
    void loadConfiguration();
    void initializeComponents();
    bool validateComponentIntegration();
    void handleComponentFailure(const QString &componentName, const QString &error);
    void connectSignals();
    void connectConfigurationSignals();
    void connectShareManagerSignals();
    void connectMountManagerSignals();
    void connectNetworkDiscoverySignals();
    void connectOperationManagerSignals();
    void connectComponentLifecycleSignals();

    // UI update methods
    void updateLocalSharesList();
    void updateRemoteSharesList();
    void updateMountedSharesList();
    void updateStatusBar();
    void updateSystemTrayIcon();
    void updateSystemTrayTooltip();

    // Utility methods
    void showStatusMessage(const QString &message, int timeout = 5000);
    void showErrorMessage(const QString &title, const QString &message);
    void showSuccessMessage(const QString &message);
    bool isSystemTrayAvailable() const;
    QString formatShareStatus(const NFSShare &share) const;
    QString formatMountStatus(const NFSMount &mount) const;
    QString formatTimeAgo(const QDateTime &dateTime) const;
    QString formatRemoteShareTooltip(const RemoteNFSShare &share) const;
    QIcon getShareStatusIcon(const NFSShare &share) const;
    QIcon getMountStatusIcon(const NFSMount &mount) const;
    QListWidgetItem* createRemoteShareItem(const RemoteNFSShare &share, bool available);
    void updateDiscoveryStatistics();
    void updateDiscoveryModeLabel();

    // Core business logic components
    ConfigurationManager *m_configurationManager;
    ShareManager *m_shareManager;
    MountManager *m_mountManager;
    NetworkDiscovery *m_networkDiscovery;
    PermissionManager *m_permissionManager;
    NotificationManager *m_notificationManager;

    // UI components
    QTabWidget *m_tabWidget;
    QSplitter *m_mainSplitter;

    // Local shares tab
    QWidget *m_localSharesTab;
    QListWidget *m_localSharesList;
    QPushButton *m_createShareButton;
    QPushButton *m_removeShareButton;
    QPushButton *m_editShareButton;
    QLabel *m_localSharesStatus;

    // Remote shares tab
    QWidget *m_remoteSharesTab;
    QListWidget *m_remoteSharesList;
    QPushButton *m_mountShareButton;
    QPushButton *m_refreshDiscoveryButton;
    QPushButton *m_discoveryModeButton;
    QPushButton *m_manageTargetsButton;
    QPushButton *m_autoDiscoveryToggle;
    QLabel *m_remoteSharesStatus;
    QLabel *m_autoDiscoveryStatus;
    QLabel *m_lastScanTimeLabel;
    QLabel *m_discoveryStatsLabel;
    QLabel *m_discoveryModeLabel;
    QProgressBar *m_discoveryProgress;

    // Mounted shares tab
    QWidget *m_mountedSharesTab;
    QListWidget *m_mountedSharesList;
    QPushButton *m_unmountShareButton;
    QLabel *m_mountedSharesStatus;

    // System tray
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_showAction;
    QAction *m_hideAction;
    QAction *m_quitAction;

    // Menu and toolbar actions
    QAction *m_refreshAction;
    QAction *m_preferencesAction;
    QAction *m_aboutAction;
    QAction *m_exportConfigAction;
    QAction *m_importConfigAction;

    // Status and timers
    QTimer *m_statusUpdateTimer;
    QTimer *m_discoveryTimeoutTimer;
    QString m_lastStatusMessage;
    bool m_systemTrayAvailable;
    bool m_explicitQuit;
    
    // Operation management
    class OperationManager *m_operationManager;
    QUuid m_currentDiscoveryOperationId;
    
    // Global progress indication
    QProgressBar *m_globalProgressBar;
    QLabel *m_globalProgressLabel;
    QPushButton *m_cancelOperationsButton;
    QPushButton *m_cancelDiscoveryButton;
};

} // namespace NFSShareManager