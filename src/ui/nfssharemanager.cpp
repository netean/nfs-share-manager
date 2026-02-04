#include "nfssharemanager.h"
#include "../core/configurationmanager.h"
#include "../core/nfsshare.h"
#include "../core/nfsmount.h"
#include "../core/remotenfsshare.h"
#include "../core/types.h"
#include "../business/sharemanager.h"
#include "../business/mountmanager.h"
#include "../business/networkdiscovery.h"
#include "../business/permissionmanager.h"
#include "notificationmanager.h"
#include "operationmanager.h"
#include "sharecreatedialog.h"
#include "shareconfigdialog.h"
#include "mountdialog.h"
#include "notificationpreferencesdialog.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QCloseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QBrush>
#include <QDateTime>
#include <QDebug>

namespace NFSShareManager {

NFSShareManagerApp::NFSShareManagerApp(QWidget *parent)
    : QMainWindow(parent)
    , m_configurationManager(new ConfigurationManager(this))
    , m_shareManager(new ShareManager(this))
    , m_mountManager(new MountManager(this))
    , m_networkDiscovery(new NetworkDiscovery(this))
    , m_permissionManager(new PermissionManager(this))
    , m_notificationManager(new NotificationManager(m_configurationManager, this))
    , m_tabWidget(nullptr)
    , m_statusUpdateTimer(new QTimer(this))
    , m_discoveryTimeoutTimer(new QTimer(this))
    , m_systemTrayAvailable(false)
    , m_explicitQuit(false)
    , m_operationManager(new OperationManager(this))
    , m_globalProgressBar(nullptr)
    , m_globalProgressLabel(nullptr)
    , m_cancelOperationsButton(nullptr)
    , m_cancelDiscoveryButton(nullptr)
{
    qDebug() << "NFSShareManagerApp: Initializing application";
    
    // Set window properties
    setWindowTitle(tr("NFS Share Manager"));
    setMinimumSize(800, 600);
    resize(1000, 700);
    
    // Initialize UI
    setupUI();
    
    // Initialize components
    initializeComponents();
    
    // Connect signals
    connectSignals();
    
    // Load configuration
    loadConfiguration();
    
    // Setup system tray if available
    setupSystemTray();
    
    // Start status update timer
    m_statusUpdateTimer->setInterval(5000); // 5 seconds
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &NFSShareManagerApp::onStatusUpdateTimer);
    m_statusUpdateTimer->start();
    
    qDebug() << "NFSShareManagerApp: Initialization complete";
}

NFSShareManagerApp::~NFSShareManagerApp()
{
    qDebug() << "NFSShareManagerApp: Shutting down";
}

void NFSShareManagerApp::setupUI()
{
    // Create central widget
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);
    
    // Setup tabs
    setupLocalSharesTab();
    setupRemoteSharesTab();
    setupMountedSharesTab();
    
    // Setup menu bar
    setupMenuBar();
    
    // Setup status bar
    setupStatusBar();
    
    // Setup global progress bar
    m_globalProgressBar = new QProgressBar(this);
    m_globalProgressBar->setVisible(false);
    m_globalProgressLabel = new QLabel(this);
    m_globalProgressLabel->setVisible(false);
    m_cancelOperationsButton = new QPushButton(tr("Cancel"), this);
    m_cancelOperationsButton->setVisible(false);
    
    statusBar()->addPermanentWidget(m_globalProgressLabel);
    statusBar()->addPermanentWidget(m_globalProgressBar);
    statusBar()->addPermanentWidget(m_cancelOperationsButton);
    
    // Initialize discovery timeout timer
    m_discoveryTimeoutTimer->setSingleShot(true);
    
    // Load timeout from preferences (default 2 minutes)
    int discoveryTimeout = m_configurationManager->getPreference("discovery/timeout", 120000).toInt();
    m_discoveryTimeoutTimer->setInterval(discoveryTimeout);
    
    connect(m_discoveryTimeoutTimer, &QTimer::timeout, this, &NFSShareManagerApp::onDiscoveryTimeout);
}

void NFSShareManagerApp::setupLocalSharesTab()
{
    m_localSharesTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_localSharesTab);
    
    // Status label
    m_localSharesStatus = new QLabel(tr("Local NFS Shares"));
    layout->addWidget(m_localSharesStatus);
    
    // Shares list
    m_localSharesList = new QListWidget();
    layout->addWidget(m_localSharesList);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_createShareButton = new QPushButton(tr("Create Share"));
    m_removeShareButton = new QPushButton(tr("Remove Share"));
    m_editShareButton = new QPushButton(tr("Edit Share"));
    
    buttonLayout->addWidget(m_createShareButton);
    buttonLayout->addWidget(m_removeShareButton);
    buttonLayout->addWidget(m_editShareButton);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    m_tabWidget->addTab(m_localSharesTab, tr("Local Shares"));
    
    // Connect buttons
    connect(m_createShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onCreateShareClicked);
    connect(m_removeShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onRemoveShareClicked);
    connect(m_editShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onEditShareClicked);
}

void NFSShareManagerApp::setupRemoteSharesTab()
{
    m_remoteSharesTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_remoteSharesTab);
    
    // Status label
    m_remoteSharesStatus = new QLabel(tr("Remote NFS Shares"));
    layout->addWidget(m_remoteSharesStatus);
    
    // Discovery progress
    m_discoveryProgress = new QProgressBar();
    m_discoveryProgress->setVisible(false);
    layout->addWidget(m_discoveryProgress);
    
    // Cancel discovery button
    m_cancelDiscoveryButton = new QPushButton(tr("Cancel Discovery"));
    m_cancelDiscoveryButton->setVisible(false);
    layout->addWidget(m_cancelDiscoveryButton);
    
    // Shares list
    m_remoteSharesList = new QListWidget();
    layout->addWidget(m_remoteSharesList);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_mountShareButton = new QPushButton(tr("Mount Share"));
    m_refreshDiscoveryButton = new QPushButton(tr("Refresh"));
    m_discoveryModeButton = new QPushButton(tr("Discovery Mode"));
    m_autoDiscoveryToggle = new QPushButton(tr("Auto Discovery"));
    
    buttonLayout->addWidget(m_mountShareButton);
    buttonLayout->addWidget(m_refreshDiscoveryButton);
    buttonLayout->addWidget(m_discoveryModeButton);
    buttonLayout->addWidget(m_autoDiscoveryToggle);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    // Status labels
    m_autoDiscoveryStatus = new QLabel(tr("Auto Discovery: Disabled"));
    m_lastScanTimeLabel = new QLabel(tr("Last Scan: Never"));
    m_discoveryStatsLabel = new QLabel(tr("Shares Found: 0"));
    m_discoveryModeLabel = new QLabel(tr("Mode: Network Scan"));
    
    layout->addWidget(m_autoDiscoveryStatus);
    layout->addWidget(m_lastScanTimeLabel);
    layout->addWidget(m_discoveryStatsLabel);
    layout->addWidget(m_discoveryModeLabel);
    
    m_tabWidget->addTab(m_remoteSharesTab, tr("Remote Shares"));
    
    // Connect buttons
    connect(m_mountShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onMountShareClicked);
    connect(m_refreshDiscoveryButton, &QPushButton::clicked, this, &NFSShareManagerApp::onRefreshDiscoveryClicked);
    connect(m_discoveryModeButton, &QPushButton::clicked, this, &NFSShareManagerApp::onDiscoveryModeClicked);
    connect(m_autoDiscoveryToggle, &QPushButton::clicked, this, [this]() {
        bool currentlyActive = m_networkDiscovery->isDiscoveryActive();
        onAutoDiscoveryToggled(!currentlyActive);
    });
    connect(m_cancelDiscoveryButton, &QPushButton::clicked, this, &NFSShareManagerApp::onCancelDiscoveryClicked);
}

void NFSShareManagerApp::setupMountedSharesTab()
{
    m_mountedSharesTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(m_mountedSharesTab);
    
    // Status label
    m_mountedSharesStatus = new QLabel(tr("Mounted NFS Shares"));
    layout->addWidget(m_mountedSharesStatus);
    
    // Shares list
    m_mountedSharesList = new QListWidget();
    layout->addWidget(m_mountedSharesList);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_unmountShareButton = new QPushButton(tr("Unmount Share"));
    
    buttonLayout->addWidget(m_unmountShareButton);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    m_tabWidget->addTab(m_mountedSharesTab, tr("Mounted Shares"));
    
    // Connect buttons
    connect(m_unmountShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onUnmountShareClicked);
}

void NFSShareManagerApp::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    m_exportConfigAction = new QAction(tr("&Export Configuration..."), this);
    m_importConfigAction = new QAction(tr("&Import Configuration..."), this);
    connect(m_exportConfigAction, &QAction::triggered, this, &NFSShareManagerApp::onExportConfigurationClicked);
    connect(m_importConfigAction, &QAction::triggered, this, &NFSShareManagerApp::onImportConfigurationClicked);
    fileMenu->addAction(m_exportConfigAction);
    fileMenu->addAction(m_importConfigAction);
    fileMenu->addSeparator();
    
    QAction *quitAction = new QAction(tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &NFSShareManagerApp::quitApplication);
    fileMenu->addAction(quitAction);
    
    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    m_refreshAction = new QAction(tr("&Refresh All"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    connect(m_refreshAction, &QAction::triggered, this, &NFSShareManagerApp::refreshAll);
    viewMenu->addAction(m_refreshAction);
    
    // Settings menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    m_preferencesAction = new QAction(tr("&Preferences..."), this);
    connect(m_preferencesAction, &QAction::triggered, this, &NFSShareManagerApp::showPreferences);
    settingsMenu->addAction(m_preferencesAction);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    m_aboutAction = new QAction(tr("&About"), this);
    connect(m_aboutAction, &QAction::triggered, this, &NFSShareManagerApp::showAbout);
    helpMenu->addAction(m_aboutAction);
}

void NFSShareManagerApp::setupStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void NFSShareManagerApp::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qDebug() << "System tray not available";
        return;
    }
    
    m_systemTrayAvailable = true;
    
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon::fromTheme("folder-network"));
    
    m_trayMenu = new QMenu(this);
    m_showAction = new QAction(tr("Show"), this);
    m_hideAction = new QAction(tr("Hide"), this);
    m_quitAction = new QAction(tr("Quit"), this);
    
    connect(m_showAction, &QAction::triggered, this, &NFSShareManagerApp::showMainWindow);
    connect(m_hideAction, &QAction::triggered, this, &NFSShareManagerApp::hideToTray);
    connect(m_quitAction, &QAction::triggered, this, &NFSShareManagerApp::quitApplication);
    
    m_trayMenu->addAction(m_showAction);
    m_trayMenu->addAction(m_hideAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &NFSShareManagerApp::onTrayIconActivated);
    
    m_trayIcon->show();
}

void NFSShareManagerApp::initializeComponents()
{
    qDebug() << "Initializing components";
    // Components are already created in constructor
    // This is where we would do additional initialization
}

void NFSShareManagerApp::loadConfiguration()
{
    qDebug() << "Loading configuration";
    
    // Load configuration from ConfigurationManager
    if (m_configurationManager->loadConfiguration()) {
        qDebug() << "Configuration loaded successfully";
        
        // Load saved shares into ShareManager
        QList<NFSShare> savedShares = m_configurationManager->getLocalShares();
        for (const NFSShare &share : savedShares) {
            // Add share to ShareManager without triggering creation signals
            m_shareManager->addExistingShare(share);
        }
        
        // Load discovery preferences
        int discoveryInterval = m_configurationManager->getPreference("discovery/interval", 30000).toInt();
        bool autoDiscoveryEnabled = m_configurationManager->getPreference("discovery/auto_enabled", false).toBool();
        int scanMode = m_configurationManager->getPreference("discovery/scan_mode", static_cast<int>(NetworkDiscovery::ScanMode::Quick)).toInt();
        
        // Apply discovery settings
        m_networkDiscovery->setScanMode(static_cast<NetworkDiscovery::ScanMode>(scanMode));
        if (autoDiscoveryEnabled) {
            m_networkDiscovery->startDiscovery(discoveryInterval);
            m_autoDiscoveryStatus->setText(tr("Auto Discovery: Enabled (every %1s)").arg(discoveryInterval / 1000));
            m_autoDiscoveryToggle->setText(tr("Disable Auto Discovery"));
        }
        
        // Update UI with loaded data
        updateLocalSharesList();
        updateMountedSharesList();
        
    } else {
        qDebug() << "Failed to load configuration, using defaults";
    }
}

void NFSShareManagerApp::connectSignals()
{
    qDebug() << "Connecting signals";
    
    // Connect ShareManager signals
    connect(m_shareManager, &ShareManager::shareCreated, this, &NFSShareManagerApp::onShareCreated);
    connect(m_shareManager, &ShareManager::shareRemoved, this, &NFSShareManagerApp::onShareRemoved);
    connect(m_shareManager, &ShareManager::shareUpdated, this, &NFSShareManagerApp::onShareUpdated);
    connect(m_shareManager, &ShareManager::shareError, this, &NFSShareManagerApp::onShareError);
    connect(m_shareManager, &ShareManager::sharesRefreshed, this, &NFSShareManagerApp::onSharesRefreshed);
    connect(m_shareManager, &ShareManager::sharesPersistenceRequested, this, &NFSShareManagerApp::onSharesPersistenceRequested);
    
    // Connect MountManager signals
    connect(m_mountManager, &MountManager::mountStarted, this, &NFSShareManagerApp::onMountStarted);
    connect(m_mountManager, &MountManager::mountCompleted, this, &NFSShareManagerApp::onMountCompleted);
    connect(m_mountManager, &MountManager::unmountStarted, this, &NFSShareManagerApp::onUnmountStarted);
    connect(m_mountManager, &MountManager::unmountCompleted, this, &NFSShareManagerApp::onUnmountCompleted);
    // Note: mountFailed and unmountFailed signals need signature fixes
    
    // Connect NetworkDiscovery signals
    connect(m_networkDiscovery, &NetworkDiscovery::discoveryCompleted, this, &NFSShareManagerApp::onDiscoveryCompleted);
    connect(m_networkDiscovery, &NetworkDiscovery::discoveryStarted, this, &NFSShareManagerApp::onDiscoveryStarted);
    connect(m_networkDiscovery, &NetworkDiscovery::discoveryError, this, &NFSShareManagerApp::onDiscoveryError);
    connect(m_networkDiscovery, &NetworkDiscovery::scanProgress, this, &NFSShareManagerApp::onScanProgress);
    connect(m_networkDiscovery, &NetworkDiscovery::shareDiscovered, this, &NFSShareManagerApp::onShareDiscovered);
    connect(m_networkDiscovery, &NetworkDiscovery::shareUnavailable, this, &NFSShareManagerApp::onShareUnavailable);
    connect(m_networkDiscovery, &NetworkDiscovery::discoveryStatusChanged, this, &NFSShareManagerApp::onDiscoveryStatusChanged);
    // Note: Other NetworkDiscovery signals need to be checked for correct signatures
}

// Slot implementations
void NFSShareManagerApp::onCreateShareClicked()
{
    ShareCreateDialog dialog(m_shareManager, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString path = dialog.getSharePath();
        ShareConfiguration config = dialog.getShareConfiguration();
        
        // Create the share using ShareManager
        bool result = m_shareManager->createShare(path, config);
        if (!result) {
            // Only show error if creation failed - success notification comes from signal
            m_notificationManager->showError(tr("Create Share Failed"), tr("Failed to create NFS share at %1").arg(path));
        }
        // Note: Success notification and UI update will come from the shareCreated signal
    }
}

void NFSShareManagerApp::onRemoveShareClicked()
{
    QListWidgetItem *currentItem = m_localSharesList->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, tr("Remove Share"), tr("Please select a share to remove."));
        return;
    }
    
    QString sharePath = currentItem->data(Qt::UserRole).toString();
    
    int ret = QMessageBox::question(this, tr("Remove Share"), 
                                   tr("Are you sure you want to remove the NFS share at:\n%1").arg(sharePath),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        bool result = m_shareManager->removeShare(sharePath);
        if (!result) {
            // Only show error if removal failed - success notification comes from signal
            m_notificationManager->showError(tr("Remove Share Failed"), tr("Failed to remove NFS share at %1").arg(sharePath));
        }
        // Note: Success notification and UI update will come from the shareRemoved signal
    }
}

void NFSShareManagerApp::onEditShareClicked()
{
    QListWidgetItem *currentItem = m_localSharesList->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, tr("Edit Share"), tr("Please select a share to edit."));
        return;
    }
    
    QString sharePath = currentItem->data(Qt::UserRole).toString();
    NFSShare share = m_shareManager->getShare(sharePath);
    
    if (share.path().isEmpty()) {
        QMessageBox::warning(this, tr("Edit Share"), tr("Could not load share configuration for: %1").arg(sharePath));
        return;
    }
    
    ShareConfigDialog dialog(share, m_shareManager, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        ShareConfiguration newConfig = dialog.getShareConfiguration();
        bool result = m_shareManager->updateShareConfiguration(sharePath, newConfig);
        
        if (!result) {
            // Only show error if update failed - success notification comes from signal
            m_notificationManager->showError(tr("Update Share Failed"), tr("Failed to update NFS share at %1").arg(sharePath));
        }
        // Note: Success notification and UI update will come from the shareUpdated signal
    }
}

void NFSShareManagerApp::onMountShareClicked()
{
    QListWidgetItem *currentItem = m_remoteSharesList->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, tr("Mount Share"), tr("Please select a remote share to mount."));
        return;
    }
    
    RemoteNFSShare remoteShare = currentItem->data(Qt::UserRole).value<RemoteNFSShare>();
    
    MountDialog dialog(remoteShare, m_mountManager, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString mountPoint = dialog.getMountPoint();
        MountOptions options = dialog.getMountOptions();
        bool isPersistent = dialog.isPersistent();
        
        // Start mount operation
        auto result = m_mountManager->mountShare(remoteShare, mountPoint, options, isPersistent);
        
        if (result == MountManager::MountResult::Success) {
            m_notificationManager->showSuccess(tr("Successfully mounted %1:%2 at %3")
                                             .arg(remoteShare.hostAddress().toString())
                                             .arg(remoteShare.exportPath())
                                             .arg(mountPoint));
            updateMountedSharesList();
        } else {
            QString errorMsg = tr("Mount failed with error code: %1").arg(static_cast<int>(result));
            m_notificationManager->showError(tr("Mount Failed"), errorMsg);
        }
    }
}

void NFSShareManagerApp::onUnmountShareClicked()
{
    QListWidgetItem *currentItem = m_mountedSharesList->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, tr("Unmount Share"), tr("Please select a mounted share to unmount."));
        return;
    }
    
    QString mountPoint = currentItem->data(Qt::UserRole).toString();
    
    int ret = QMessageBox::question(this, tr("Unmount Share"), 
                                   tr("Are you sure you want to unmount:\n%1").arg(mountPoint),
                                   QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        bool result = m_mountManager->unmountShare(mountPoint);
        
        if (result) {
            m_notificationManager->showSuccess(tr("Successfully unmounted %1").arg(mountPoint));
            updateMountedSharesList();
        } else {
            m_notificationManager->showError(tr("Unmount Failed"), tr("Failed to unmount %1").arg(mountPoint));
        }
    }
}

void NFSShareManagerApp::onRefreshDiscoveryClicked()
{
    m_discoveryProgress->setVisible(true);
    m_discoveryProgress->setRange(0, 0); // Indeterminate progress
    m_cancelDiscoveryButton->setVisible(true);
    
    // Start discovery timeout timer
    m_discoveryTimeoutTimer->start();
    
    // Start network discovery
    m_networkDiscovery->refreshDiscovery(NetworkDiscovery::ScanMode::Quick);
    
    // Update UI
    m_refreshDiscoveryButton->setEnabled(false);
    m_remoteSharesStatus->setText(tr("Discovering NFS shares..."));
}

void NFSShareManagerApp::onDiscoveryModeClicked()
{
    QStringList modes;
    modes << tr("Quick Scan") << tr("Full Scan") << tr("Complete Scan") << tr("Targeted Scan");
    
    bool ok;
    QString selectedMode = QInputDialog::getItem(this, tr("Discovery Mode"), 
                                               tr("Select discovery mode:"), modes, 0, false, &ok);
    
    if (ok && !selectedMode.isEmpty()) {
        NetworkDiscovery::ScanMode mode = NetworkDiscovery::ScanMode::Quick;
        
        if (selectedMode == tr("Full Scan")) {
            mode = NetworkDiscovery::ScanMode::Full;
        } else if (selectedMode == tr("Complete Scan")) {
            mode = NetworkDiscovery::ScanMode::Complete;
        } else if (selectedMode == tr("Targeted Scan")) {
            // Show manual entry dialog
            QString hostAddress = QInputDialog::getText(this, tr("Targeted Scan"), 
                                                      tr("Enter NFS server address:"), 
                                                      QLineEdit::Normal, QString(), &ok);
            if (ok && !hostAddress.isEmpty()) {
                mode = NetworkDiscovery::ScanMode::Targeted;
            } else {
                return;
            }
        }
        
        m_discoveryModeLabel->setText(tr("Mode: %1").arg(selectedMode));
        
        // Auto-refresh with new mode
        m_networkDiscovery->refreshDiscovery(mode);
        onRefreshDiscoveryClicked();
    }
}

void NFSShareManagerApp::onAutoDiscoveryToggled(bool enabled)
{
    if (enabled) {
        // Get discovery interval from configuration or use default
        int interval = m_configurationManager->getPreference("discovery/interval", 30000).toInt();
        m_networkDiscovery->startDiscovery(interval);
        m_autoDiscoveryStatus->setText(tr("Auto Discovery: Enabled (every %1s)").arg(interval / 1000));
        m_autoDiscoveryToggle->setText(tr("Disable Auto Discovery"));
        // Start initial discovery
        onRefreshDiscoveryClicked();
    } else {
        m_networkDiscovery->stopDiscovery();
        m_autoDiscoveryStatus->setText(tr("Auto Discovery: Disabled"));
        m_autoDiscoveryToggle->setText(tr("Enable Auto Discovery"));
        
        // Stop any ongoing discovery and hide UI elements
        onCancelDiscoveryClicked();
    }
}

void NFSShareManagerApp::quitApplication()
{
    // Set flag to indicate explicit quit (bypass system tray dialog)
    m_explicitQuit = true;
    
    // Save configuration before quitting
    m_configurationManager->saveConfiguration();
    
    // Stop any ongoing operations
    if (m_networkDiscovery) {
        m_networkDiscovery->stopDiscovery();
    }
    
    // Stop timers
    if (m_statusUpdateTimer) {
        m_statusUpdateTimer->stop();
    }
    
    // Quit the application completely
    QApplication::quit();
}

void NFSShareManagerApp::onStatusUpdateTimer()
{
    // Update status periodically
    updateStatusBar();
}

void NFSShareManagerApp::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showMainWindow();
    }
}

void NFSShareManagerApp::showMainWindow()
{
    show();
    raise();
    activateWindow();
}

void NFSShareManagerApp::hideToTray()
{
    if (m_systemTrayAvailable) {
        hide();
    }
}

void NFSShareManagerApp::refreshAll()
{
    updateLocalSharesList();
    updateMountedSharesList();
    onRefreshDiscoveryClicked();
    updateStatusBar();
}

void NFSShareManagerApp::showAbout()
{
    QMessageBox::about(this, tr("About NFS Share Manager"),
                      tr("NFS Share Manager v1.0.0\n\n"
                         "A KDE application for managing NFS shares and mounts.\n\n"
                         "Built with Qt6 and KDE Frameworks 6.\n\n"
                         "Features:\n"
                         "• Local NFS share management\n"
                         "• Remote share discovery and mounting\n"
                         "• System tray integration\n"
                         "• PolicyKit integration for security\n"
                         "• Comprehensive configuration management"));
}

void NFSShareManagerApp::showPreferences()
{
    // Create a simple preferences dialog
    QDialog prefsDialog(this);
    prefsDialog.setWindowTitle(tr("Preferences"));
    prefsDialog.setModal(true);
    prefsDialog.resize(400, 300);
    
    QVBoxLayout *layout = new QVBoxLayout(&prefsDialog);
    
    // Auto discovery settings
    QGroupBox *discoveryGroup = new QGroupBox(tr("Network Discovery"), &prefsDialog);
    QVBoxLayout *discoveryLayout = new QVBoxLayout(discoveryGroup);
    
    QCheckBox *autoDiscoveryCheck = new QCheckBox(tr("Enable automatic discovery"), discoveryGroup);
    autoDiscoveryCheck->setChecked(m_networkDiscovery->isDiscoveryActive());
    discoveryLayout->addWidget(autoDiscoveryCheck);
    
    QLabel *intervalLabel = new QLabel(tr("Discovery interval (seconds):"), discoveryGroup);
    QSpinBox *intervalSpin = new QSpinBox(discoveryGroup);
    intervalSpin->setRange(30, 3600);
    intervalSpin->setValue(m_configurationManager->getPreference("discovery/interval", 30000).toInt() / 1000);
    discoveryLayout->addWidget(intervalLabel);
    discoveryLayout->addWidget(intervalSpin);
    
    QLabel *timeoutLabel = new QLabel(tr("Discovery timeout (seconds):"), discoveryGroup);
    QSpinBox *timeoutSpin = new QSpinBox(discoveryGroup);
    timeoutSpin->setRange(30, 300);  // 30 seconds to 5 minutes
    timeoutSpin->setValue(m_configurationManager->getPreference("discovery/timeout", 120000).toInt() / 1000);  // Default 2 minutes
    timeoutSpin->setToolTip(tr("Maximum time to wait for network discovery to complete"));
    discoveryLayout->addWidget(timeoutLabel);
    discoveryLayout->addWidget(timeoutSpin);
    
    // Default scan mode
    QLabel *scanModeLabel = new QLabel(tr("Default scan mode:"), discoveryGroup);
    QComboBox *scanModeCombo = new QComboBox(discoveryGroup);
    scanModeCombo->addItem(tr("Quick Scan"), static_cast<int>(NetworkDiscovery::ScanMode::Quick));
    scanModeCombo->addItem(tr("Full Scan"), static_cast<int>(NetworkDiscovery::ScanMode::Full));
    scanModeCombo->addItem(tr("Complete Scan"), static_cast<int>(NetworkDiscovery::ScanMode::Complete));
    scanModeCombo->addItem(tr("Targeted Scan"), static_cast<int>(NetworkDiscovery::ScanMode::Targeted));
    
    int currentMode = m_configurationManager->getPreference("discovery/scan_mode", static_cast<int>(NetworkDiscovery::ScanMode::Quick)).toInt();
    int modeIndex = scanModeCombo->findData(currentMode);
    if (modeIndex >= 0) {
        scanModeCombo->setCurrentIndex(modeIndex);
    }
    scanModeCombo->setToolTip(tr("Default scan mode for automatic discovery"));
    discoveryLayout->addWidget(scanModeLabel);
    discoveryLayout->addWidget(scanModeCombo);
    
    layout->addWidget(discoveryGroup);
    
    // Notification settings
    QGroupBox *notificationGroup = new QGroupBox(tr("Notifications"), &prefsDialog);
    QVBoxLayout *notificationLayout = new QVBoxLayout(notificationGroup);
    
    QPushButton *notificationPrefsButton = new QPushButton(tr("Configure Notifications..."), notificationGroup);
    connect(notificationPrefsButton, &QPushButton::clicked, this, &NFSShareManagerApp::showNotificationPreferences);
    notificationLayout->addWidget(notificationPrefsButton);
    
    layout->addWidget(notificationGroup);
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &prefsDialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &prefsDialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &prefsDialog, &QDialog::reject);
    layout->addWidget(buttonBox);
    
    if (prefsDialog.exec() == QDialog::Accepted) {
        // Apply settings
        bool autoDiscovery = autoDiscoveryCheck->isChecked();
        int intervalSeconds = intervalSpin->value();
        int intervalMs = intervalSeconds * 1000;
        int timeoutSeconds = timeoutSpin->value();
        int timeoutMs = timeoutSeconds * 1000;
        int scanMode = scanModeCombo->currentData().toInt();
        
        // Save preferences
        m_configurationManager->setPreference("discovery/auto_enabled", autoDiscovery);
        m_configurationManager->setPreference("discovery/interval", intervalMs);
        m_configurationManager->setPreference("discovery/timeout", timeoutMs);
        m_configurationManager->setPreference("discovery/scan_mode", scanMode);
        m_configurationManager->saveConfiguration();
        
        // Update discovery timeout timer
        m_discoveryTimeoutTimer->setInterval(timeoutMs);
        
        // Update network discovery scan mode
        m_networkDiscovery->setScanMode(static_cast<NetworkDiscovery::ScanMode>(scanMode));
        
        if (autoDiscovery != m_networkDiscovery->isDiscoveryActive()) {
            if (autoDiscovery) {
                m_networkDiscovery->startDiscovery(intervalMs);
                m_autoDiscoveryStatus->setText(tr("Auto Discovery: Enabled (every %1s)").arg(intervalSeconds));
                m_autoDiscoveryToggle->setText(tr("Disable Auto Discovery"));
            } else {
                m_networkDiscovery->stopDiscovery();
                m_autoDiscoveryStatus->setText(tr("Auto Discovery: Disabled"));
                m_autoDiscoveryToggle->setText(tr("Enable Auto Discovery"));
            }
        } else if (autoDiscovery) {
            // Update interval if auto discovery is already enabled
            m_networkDiscovery->setScanInterval(intervalMs);
            m_autoDiscoveryStatus->setText(tr("Auto Discovery: Enabled (every %1s)").arg(intervalSeconds));
        }
        
        // Save configuration
        m_configurationManager->saveConfiguration();
    }
}

void NFSShareManagerApp::showNotificationPreferences()
{
    NotificationPreferencesDialog dialog(m_notificationManager->preferences(), this);
    if (dialog.exec() == QDialog::Accepted) {
        m_notificationManager->setPreferences(dialog.getPreferences());
    }
}

bool NFSShareManagerApp::exportConfiguration(const QString &filePath, const QString &profileName, const QString &description)
{
    try {
        bool success = m_configurationManager->exportConfiguration(filePath, profileName, description);
        if (success) {
            m_notificationManager->showSuccess(tr("Configuration exported successfully to %1").arg(filePath));
        } else {
            m_notificationManager->showError(tr("Export Failed"), tr("Failed to export configuration to %1").arg(filePath));
        }
        return success;
    } catch (const std::exception &e) {
        m_notificationManager->showError(tr("Export Error"), tr("Error exporting configuration: %1").arg(e.what()));
        return false;
    }
}

bool NFSShareManagerApp::importConfiguration(const QString &filePath, bool mergeMode)
{
    try {
        bool success = m_configurationManager->importConfiguration(filePath, mergeMode);
        if (success) {
            m_notificationManager->showSuccess(tr("Configuration imported successfully from %1").arg(filePath));
            // Refresh all UI components
            refreshAll();
        } else {
            m_notificationManager->showError(tr("Import Failed"), tr("Failed to import configuration from %1").arg(filePath));
        }
        return success;
    } catch (const std::exception &e) {
        m_notificationManager->showError(tr("Import Error"), tr("Error importing configuration: %1").arg(e.what()));
        return false;
    }
}

void NFSShareManagerApp::closeEvent(QCloseEvent *event)
{
    // Only show system tray dialog if we're not explicitly quitting
    // and system tray is available
    if (m_systemTrayAvailable && m_trayIcon->isVisible() && !m_explicitQuit) {
        QMessageBox::information(this, tr("NFS Share Manager"),
                                tr("The application will keep running in the "
                                   "system tray. To terminate the application, "
                                   "choose <b>Quit</b> from the context menu "
                                   "of the system tray entry."));
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void NFSShareManagerApp::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_systemTrayAvailable) {
            hide();
        }
    }
}

} // namespace NFSShareManager

// Stub implementations for missing slot methods
namespace NFSShareManager {

void NFSShareManagerApp::onConfigurationValidationFailed(const ValidationResult &result)
{
    Q_UNUSED(result)
    qDebug() << "Configuration validation failed (stub)";
}

void NFSShareManagerApp::onConfigurationRepaired(const QStringList &repairActions)
{
    Q_UNUSED(repairActions)
    qDebug() << "Configuration repaired (stub)";
}

void NFSShareManagerApp::onBackupCreated(const QString &backupPath)
{
    Q_UNUSED(backupPath)
    qDebug() << "Backup created (stub)";
}

void NFSShareManagerApp::onShareCreated(const NFSShare &share)
{
    qDebug() << "Share created signal received:" << share.path();
    // Update the local shares list to show the new share
    updateLocalSharesList();
    // Show success notification
    m_notificationManager->showSuccess(tr("NFS share created successfully: %1").arg(share.path()));
}

void NFSShareManagerApp::onShareRemoved(const QString &sharePath)
{
    qDebug() << "Share removed signal received:" << sharePath;
    // Update the local shares list to remove the share
    updateLocalSharesList();
    // Show success notification
    m_notificationManager->showSuccess(tr("NFS share removed: %1").arg(sharePath));
}

void NFSShareManagerApp::onShareUpdated(const NFSShare &share)
{
    qDebug() << "Share updated signal received:" << share.path();
    // Update the local shares list to reflect changes
    updateLocalSharesList();
    // Show success notification
    m_notificationManager->showSuccess(tr("NFS share updated: %1").arg(share.path()));
}

void NFSShareManagerApp::onShareError(const QString &path, const QString &error)
{
    qDebug() << "Share error signal received:" << path << error;
    // Show error notification
    m_notificationManager->showError(tr("Share Error"), tr("Error with NFS share %1: %2").arg(path, error));
}

void NFSShareManagerApp::onSharesRefreshed()
{
    qDebug() << "Shares refreshed signal received";
    // Update the local shares list
    updateLocalSharesList();
}

void NFSShareManagerApp::onNFSServerStatusChanged(bool running)
{
    Q_UNUSED(running)
    qDebug() << "NFS server status changed (stub)";
}

void NFSShareManagerApp::onSharesPersistenceRequested()
{
    qDebug() << "Saving shares to configuration";
    
    // Get current shares from ShareManager
    QList<NFSShare> currentShares = m_shareManager->getActiveShares();
    
    // Save to configuration
    m_configurationManager->setLocalShares(currentShares);
    m_configurationManager->saveConfiguration();
    
    qDebug() << "Saved" << currentShares.size() << "shares to configuration";
}

void NFSShareManagerApp::onMountStarted(const RemoteNFSShare &remoteShare, const QString &mountPoint)
{
    Q_UNUSED(remoteShare)
    Q_UNUSED(mountPoint)
    qDebug() << "Mount started (stub)";
}

void NFSShareManagerApp::onMountCompleted(const NFSMount &mount)
{
    Q_UNUSED(mount)
    qDebug() << "Mount completed (stub)";
}

void NFSShareManagerApp::onMountFailed(const RemoteNFSShare &remoteShare, const QString &mountPoint, int result, const QString &errorMessage)
{
    Q_UNUSED(remoteShare)
    Q_UNUSED(mountPoint)
    Q_UNUSED(result)
    Q_UNUSED(errorMessage)
    qDebug() << "Mount failed (stub)";
}

void NFSShareManagerApp::onUnmountStarted(const QString &mountPoint)
{
    Q_UNUSED(mountPoint)
    qDebug() << "Unmount started (stub)";
}

void NFSShareManagerApp::onUnmountCompleted(const QString &mountPoint)
{
    Q_UNUSED(mountPoint)
    qDebug() << "Unmount completed (stub)";
}

void NFSShareManagerApp::onUnmountFailed(const QString &mountPoint, const QString &errorMessage)
{
    Q_UNUSED(mountPoint)
    Q_UNUSED(errorMessage)
    qDebug() << "Unmount failed (stub)";
}

void NFSShareManagerApp::onMountStatusChanged(const NFSMount &mount)
{
    Q_UNUSED(mount)
    qDebug() << "Mount status changed (stub)";
}

void NFSShareManagerApp::onShareDiscovered(const RemoteNFSShare &share)
{
    qDebug() << "Share discovered:" << share.hostAddress().toString() << ":" << share.exportPath();
    
    // Add the share to the remote shares list immediately
    QListWidgetItem *item = createRemoteShareItem(share, true);
    m_remoteSharesList->addItem(item);
    
    // Update discovery statistics
    updateDiscoveryStatistics();
}

void NFSShareManagerApp::onShareUnavailable(const QString &hostAddress, const QString &exportPath)
{
    qDebug() << "Share unavailable:" << hostAddress << ":" << exportPath;
    
    // Find and update the corresponding item in the remote shares list
    for (int i = 0; i < m_remoteSharesList->count(); ++i) {
        QListWidgetItem *item = m_remoteSharesList->item(i);
        RemoteNFSShare share = item->data(Qt::UserRole).value<RemoteNFSShare>();
        
        if (share.hostAddress().toString() == hostAddress && share.exportPath() == exportPath) {
            // Mark as unavailable
            item->setIcon(QIcon::fromTheme("folder-network-offline"));
            item->setToolTip(tr("Share unavailable: %1:%2").arg(hostAddress, exportPath));
            item->setForeground(QBrush(Qt::gray));
            break;
        }
    }
    
    // Update discovery statistics
    updateDiscoveryStatistics();
}

void NFSShareManagerApp::updateDiscoveryStatistics()
{
    auto discoveredShares = m_networkDiscovery->getDiscoveredShares();
    int availableShares = 0;
    
    for (const auto &share : discoveredShares) {
        if (share.isAvailable()) {
            availableShares++;
        }
    }
    
    m_discoveryStatsLabel->setText(tr("Shares Found: %1 (Available: %2)").arg(discoveredShares.size()).arg(availableShares));
}

void NFSShareManagerApp::updateDiscoveryModeLabel()
{
    QString modeText;
    switch (m_networkDiscovery->scanMode()) {
        case NetworkDiscovery::ScanMode::Quick:
            modeText = tr("Quick Scan");
            break;
        case NetworkDiscovery::ScanMode::Full:
            modeText = tr("Full Scan");
            break;
        case NetworkDiscovery::ScanMode::Complete:
            modeText = tr("Complete Scan");
            break;
        case NetworkDiscovery::ScanMode::Targeted:
            modeText = tr("Targeted Scan");
            break;
    }
    m_discoveryModeLabel->setText(tr("Mode: %1").arg(modeText));
}

void NFSShareManagerApp::onDiscoveryCompleted(int sharesFound, int hostsScanned)
{
    qDebug() << "Discovery completed:" << sharesFound << "shares found," << hostsScanned << "hosts scanned";
    
    // Stop the discovery timeout timer
    m_discoveryTimeoutTimer->stop();
    
    // Hide progress indicators and cancel button
    m_discoveryProgress->setVisible(false);
    m_cancelDiscoveryButton->setVisible(false);
    
    // Re-enable refresh button
    m_refreshDiscoveryButton->setEnabled(true);
    
    // Update status
    m_remoteSharesStatus->setText(tr("Discovery completed - %1 shares found").arg(sharesFound));
    
    // Update the remote shares list
    updateRemoteSharesList();
    
    // Update last scan time
    m_lastScanTimeLabel->setText(tr("Last Scan: %1").arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    
    // Show notification for successful discovery
    if (m_notificationManager && sharesFound > 0) {
        m_notificationManager->showSuccess(tr("Discovery completed - found %1 NFS shares").arg(sharesFound));
    }
}

void NFSShareManagerApp::onDiscoveryError(const QString &error)
{
    qDebug() << "Discovery error:" << error;
    
    // Stop the discovery timeout timer
    m_discoveryTimeoutTimer->stop();
    
    // Hide progress indicators and cancel button
    m_discoveryProgress->setVisible(false);
    m_cancelDiscoveryButton->setVisible(false);
    
    // Re-enable refresh button
    m_refreshDiscoveryButton->setEnabled(true);
    
    // Update status
    m_remoteSharesStatus->setText(tr("Discovery error: %1").arg(error));
    
    // Show error notification
    if (m_notificationManager) {
        m_notificationManager->showError(tr("Discovery Error"), 
                                       tr("Network discovery failed: %1").arg(error));
    }
}

void NFSShareManagerApp::onDiscoveryStarted(NetworkDiscovery::ScanMode mode)
{
    qDebug() << "Discovery started with mode:" << static_cast<int>(mode);
    
    // Start the discovery timeout timer
    m_discoveryTimeoutTimer->start();
    
    // Show progress indicators and cancel button
    m_discoveryProgress->setVisible(true);
    m_discoveryProgress->setRange(0, 0); // Indeterminate progress
    m_cancelDiscoveryButton->setVisible(true);
    
    // Disable refresh button during discovery
    m_refreshDiscoveryButton->setEnabled(false);
    
    // Update status
    QString modeText;
    switch (mode) {
        case NetworkDiscovery::ScanMode::Quick:
            modeText = tr("Quick Scan");
            break;
        case NetworkDiscovery::ScanMode::Full:
            modeText = tr("Full Scan");
            break;
        case NetworkDiscovery::ScanMode::Complete:
            modeText = tr("Complete Scan");
            break;
        case NetworkDiscovery::ScanMode::Targeted:
            modeText = tr("Targeted Scan");
            break;
    }
    m_remoteSharesStatus->setText(tr("Starting %1...").arg(modeText));
}

void NFSShareManagerApp::onDiscoveryStatusChanged(NetworkDiscovery::DiscoveryStatus status)
{
    qDebug() << "Discovery status changed:" << static_cast<int>(status);
    
    switch (status) {
    case NetworkDiscovery::DiscoveryStatus::Idle:
        m_remoteSharesStatus->setText(tr("Remote NFS Shares"));
        break;
    case NetworkDiscovery::DiscoveryStatus::Scanning:
        m_remoteSharesStatus->setText(tr("Discovering NFS shares..."));
        break;
    case NetworkDiscovery::DiscoveryStatus::Completed:
        // Status will be updated by onDiscoveryCompleted
        break;
    case NetworkDiscovery::DiscoveryStatus::Error:
        m_remoteSharesStatus->setText(tr("Discovery error occurred"));
        break;
    }
}

void NFSShareManagerApp::onScanProgress(int current, int total, const QString &hostAddress)
{
    qDebug() << "Scan progress:" << current << "/" << total << "- scanning" << hostAddress;
    
    // Update progress bar if we have total count
    if (total > 0) {
        m_discoveryProgress->setRange(0, total);
        m_discoveryProgress->setValue(current);
    }
    
    // Update status with current host being scanned
    m_remoteSharesStatus->setText(tr("Scanning %1 (%2/%3)").arg(hostAddress).arg(current).arg(total));
}

void NFSShareManagerApp::onTrayIconMessageClicked()
{
    qDebug() << "Tray icon message clicked (stub)";
}

void NFSShareManagerApp::onManageTargetsClicked()
{
    QMessageBox::information(this, tr("Manage Targets"), tr("Manage Targets functionality not yet implemented"));
}

void NFSShareManagerApp::onOperationStarted(const QUuid &operationId, const QString &title)
{
    Q_UNUSED(operationId)
    Q_UNUSED(title)
    qDebug() << "Operation started (stub)";
}

void NFSShareManagerApp::onOperationProgressUpdated(const QUuid &operationId, int progress, const QString &statusMessage)
{
    Q_UNUSED(operationId)
    Q_UNUSED(progress)
    Q_UNUSED(statusMessage)
    qDebug() << "Operation progress updated (stub)";
}

void NFSShareManagerApp::onOperationCompleted(const QUuid &operationId, const QString &message)
{
    Q_UNUSED(operationId)
    Q_UNUSED(message)
    qDebug() << "Operation completed (stub)";
}

void NFSShareManagerApp::onOperationFailed(const QUuid &operationId, const QString &errorMessage)
{
    Q_UNUSED(operationId)
    Q_UNUSED(errorMessage)
    qDebug() << "Operation failed (stub)";
}

void NFSShareManagerApp::onOperationCancelled(const QUuid &operationId)
{
    Q_UNUSED(operationId)
    qDebug() << "Operation cancelled (stub)";
}

// UI update methods
void NFSShareManagerApp::updateLocalSharesList()
{
    m_localSharesList->clear();
    
    QList<NFSShare> shares = m_shareManager->getActiveShares();
    
    for (const NFSShare &share : shares) {
        QListWidgetItem *item = new QListWidgetItem(m_localSharesList);
        item->setText(tr("%1").arg(share.path()));
        item->setData(Qt::UserRole, share.path());
        item->setIcon(getShareStatusIcon(share));
        item->setToolTip(formatShareStatus(share));
        m_localSharesList->addItem(item);
    }
    
    m_localSharesStatus->setText(tr("Local NFS Shares (%1)").arg(shares.size()));
}

void NFSShareManagerApp::updateRemoteSharesList()
{
    m_remoteSharesList->clear();
    
    auto discoveredShares = m_networkDiscovery->getDiscoveredShares();
    
    for (const RemoteNFSShare &share : discoveredShares) {
        QListWidgetItem *item = createRemoteShareItem(share, true);
        m_remoteSharesList->addItem(item);
    }
    
    m_discoveryStatsLabel->setText(tr("Shares Found: %1").arg(discoveredShares.size()));
}

void NFSShareManagerApp::updateMountedSharesList()
{
    m_mountedSharesList->clear();
    
    QList<NFSMount> mounts = m_mountManager->getManagedMounts();
    
    for (const NFSMount &mount : mounts) {
        QListWidgetItem *item = new QListWidgetItem(m_mountedSharesList);
        item->setText(tr("%1 at %2").arg(mount.remoteShare().displayName(), mount.localMountPoint()));
        item->setData(Qt::UserRole, mount.localMountPoint());
        item->setIcon(getMountStatusIcon(mount));
        item->setToolTip(formatMountStatus(mount));
        m_mountedSharesList->addItem(item);
    }
    
    m_mountedSharesStatus->setText(tr("Mounted NFS Shares (%1)").arg(mounts.size()));
}

void NFSShareManagerApp::updateStatusBar()
{
    QString status = tr("Ready");
    
    // Check if discovery is running
    if (m_networkDiscovery && m_networkDiscovery->isDiscoveryActive()) {
        status = tr("Discovering NFS shares...");
    }
    
    statusBar()->showMessage(status);
}

void NFSShareManagerApp::checkComponentHealth()
{
    qDebug() << "Checking component health (stub)";
}

void NFSShareManagerApp::onDiscoveryTimeout()
{
    qDebug() << "Discovery timeout reached - stopping discovery";
    
    // Stop the network discovery
    m_networkDiscovery->stopDiscovery();
    
    // Hide progress indicators and cancel button
    m_discoveryProgress->setVisible(false);
    m_cancelDiscoveryButton->setVisible(false);
    
    // Re-enable refresh button
    m_refreshDiscoveryButton->setEnabled(true);
    
    // Update status
    m_remoteSharesStatus->setText(tr("Discovery timed out after 40 seconds"));
    
    // Show notification
    if (m_notificationManager) {
        m_notificationManager->showWarning(tr("Discovery Timeout"), 
                                         tr("Network discovery timed out after 40 seconds. Some shares may not have been found."));
    }
    
    // Update the remote shares list with whatever was found
    updateRemoteSharesList();
}

void NFSShareManagerApp::onCancelDiscoveryClicked()
{
    qDebug() << "Discovery cancelled by user";
    
    // Stop the discovery timeout timer
    m_discoveryTimeoutTimer->stop();
    
    // Stop the network discovery
    m_networkDiscovery->stopDiscovery();
    
    // Hide progress indicators and cancel button
    m_discoveryProgress->setVisible(false);
    m_cancelDiscoveryButton->setVisible(false);
    
    // Re-enable refresh button
    m_refreshDiscoveryButton->setEnabled(true);
    
    // Update status
    m_remoteSharesStatus->setText(tr("Discovery cancelled"));
    
    // Show notification
    if (m_notificationManager) {
        m_notificationManager->showInfo(tr("Network discovery was cancelled by user."), 5000);
    }
    
    // Update the remote shares list with whatever was found
    updateRemoteSharesList();
}

// Utility methods
QString NFSShareManagerApp::formatShareStatus(const NFSShare &share) const
{
    QString tooltip = tr("Share: %1\nPath: %2\nStatus: %3")
                     .arg(share.exportPath())
                     .arg(share.path())
                     .arg(share.isActive() ? tr("Active") : tr("Inactive"));
    
    return tooltip;
}

QString NFSShareManagerApp::formatMountStatus(const NFSMount &mount) const
{
    QString tooltip = tr("Remote: %1\nMount Point: %2\nStatus: %3")
                     .arg(mount.remoteShare().displayName())
                     .arg(mount.localMountPoint())
                     .arg(mount.status() == MountStatus::Mounted ? tr("Mounted") : tr("Unmounted"));
    
    if (mount.status() == MountStatus::Mounted) {
        tooltip += tr("\nMounted: %1").arg(formatTimeAgo(mount.mountedAt()));
    }
    
    return tooltip;
}

QString NFSShareManagerApp::formatTimeAgo(const QDateTime &dateTime) const
{
    qint64 seconds = dateTime.secsTo(QDateTime::currentDateTime());
    
    if (seconds < 60) {
        return tr("%1 seconds ago").arg(seconds);
    } else if (seconds < 3600) {
        return tr("%1 minutes ago").arg(seconds / 60);
    } else if (seconds < 86400) {
        return tr("%1 hours ago").arg(seconds / 3600);
    } else {
        return tr("%1 days ago").arg(seconds / 86400);
    }
}

QIcon NFSShareManagerApp::getShareStatusIcon(const NFSShare &share) const
{
    if (share.isActive()) {
        return QIcon::fromTheme("folder-network", QIcon(":/icons/folder-network.png"));
    } else {
        return QIcon::fromTheme("folder-network-offline", QIcon(":/icons/folder-network-offline.png"));
    }
}

QIcon NFSShareManagerApp::getMountStatusIcon(const NFSMount &mount) const
{
    if (mount.status() == MountStatus::Mounted) {
        return QIcon::fromTheme("drive-harddisk-network", QIcon(":/icons/drive-harddisk-network.png"));
    } else {
        return QIcon::fromTheme("drive-harddisk-network-offline", QIcon(":/icons/drive-harddisk-network-offline.png"));
    }
}

QListWidgetItem* NFSShareManagerApp::createRemoteShareItem(const RemoteNFSShare &share, bool available)
{
    QListWidgetItem *item = new QListWidgetItem();
    item->setText(tr("%1:%2").arg(share.hostAddress().toString(), share.exportPath()));
    item->setData(Qt::UserRole, QVariant::fromValue(share));
    
    if (available) {
        item->setIcon(QIcon::fromTheme("folder-network"));
        item->setToolTip(formatRemoteShareTooltip(share));
    } else {
        item->setIcon(QIcon::fromTheme("folder-network-offline"));
        item->setToolTip(tr("Share unavailable: %1:%2").arg(share.hostAddress().toString(), share.exportPath()));
        item->setForeground(QBrush(Qt::gray));
    }
    
    return item;
}

QString NFSShareManagerApp::formatRemoteShareTooltip(const RemoteNFSShare &share) const
{
    QString tooltip = tr("Host: %1\nExport: %2")
                     .arg(share.hostAddress().toString())
                     .arg(share.exportPath());
    
    if (!share.hostName().isEmpty() && share.hostName() != share.hostAddress().toString()) {
        tooltip = tr("Host: %1 (%2)\nExport: %3")
                 .arg(share.hostName())
                 .arg(share.hostAddress().toString())
                 .arg(share.exportPath());
    }
    
    return tooltip;
}

void NFSShareManagerApp::onExportConfigurationClicked()
{
    QString fileName = QInputDialog::getText(this, tr("Export Configuration"), 
                                           tr("Enter filename for configuration export:"),
                                           QLineEdit::Normal, "nfs-config.json");
    if (!fileName.isEmpty()) {
        QString profileName = QInputDialog::getText(this, tr("Export Configuration"), 
                                                  tr("Enter profile name:"),
                                                  QLineEdit::Normal, "Default Profile");
        if (!profileName.isEmpty()) {
            exportConfiguration(fileName, profileName, tr("Exported configuration"));
        }
    }
}

void NFSShareManagerApp::onImportConfigurationClicked()
{
    QString fileName = QInputDialog::getText(this, tr("Import Configuration"), 
                                           tr("Enter filename to import configuration from:"),
                                           QLineEdit::Normal, "nfs-config.json");
    if (!fileName.isEmpty()) {
        int ret = QMessageBox::question(this, tr("Import Configuration"),
                                       tr("Do you want to merge with existing configuration?\n\n"
                                          "Yes: Merge with current settings\n"
                                          "No: Replace current settings"),
                                       QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        
        if (ret != QMessageBox::Cancel) {
            bool mergeMode = (ret == QMessageBox::Yes);
            importConfiguration(fileName, mergeMode);
        }
    }
}

} // namespace NFSShareManager