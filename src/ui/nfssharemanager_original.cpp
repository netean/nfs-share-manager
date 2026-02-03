#include "nfssharemanager.h"
#include "sharecreatedialog.h"
#include "shareconfigdialog.h"
#include "mountdialog.h"
#include "operationmanager.h"
#include "notificationmanager.h"
#include "notificationpreferencesdialog.h"
#include "startupguidancedialog.h"
#include "../core/configurationmanager.h"
#include "../core/startupmanager.h"
#include "../business/sharemanager.h"
#include "../business/mountmanager.h"
#include "../business/networkdiscovery.h"
#include "../business/permissionmanager.h"
#include "../core/nfsshare.h"
#include "../core/nfsmount.h"
#include "../core/remotenfsshare.h"
#include "../core/types.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QProgressBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QToolBar>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QCloseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QIcon>
#include <QStyle>
#include <QColor>
#include <QDebug>
#include <QFontDatabase>
#include <QUuid>
#include <algorithm>

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
    , m_mainSplitter(nullptr)
    , m_localSharesTab(nullptr)
    , m_localSharesList(nullptr)
    , m_createShareButton(nullptr)
    , m_removeShareButton(nullptr)
    , m_editShareButton(nullptr)
    , m_localSharesStatus(nullptr)
    , m_remoteSharesTab(nullptr)
    , m_remoteSharesList(nullptr)
    , m_mountShareButton(nullptr)
    , m_refreshDiscoveryButton(nullptr)
    , m_discoveryModeButton(nullptr)
    , m_manageTargetsButton(nullptr)
    , m_autoDiscoveryToggle(nullptr)
    , m_remoteSharesStatus(nullptr)
    , m_autoDiscoveryStatus(nullptr)
    , m_lastScanTimeLabel(nullptr)
    , m_discoveryStatsLabel(nullptr)
    , m_discoveryModeLabel(nullptr)
    , m_discoveryProgress(nullptr)
    , m_mountedSharesTab(nullptr)
    , m_mountedSharesList(nullptr)
    , m_unmountShareButton(nullptr)
    , m_mountedSharesStatus(nullptr)
    , m_trayIcon(nullptr)
    , m_trayMenu(nullptr)
    , m_showAction(nullptr)
    , m_hideAction(nullptr)
    , m_quitAction(nullptr)
    , m_refreshAction(nullptr)
    , m_preferencesAction(nullptr)
    , m_aboutAction(nullptr)
    , m_exportConfigAction(nullptr)
    , m_importConfigAction(nullptr)
    , m_statusUpdateTimer(new QTimer(this))
    , m_systemTrayAvailable(false)
    , m_operationManager(nullptr)
    , m_globalProgressBar(nullptr)
    , m_globalProgressLabel(nullptr)
    , m_cancelOperationsButton(nullptr)
{
    // Perform startup validation before initializing UI
    StartupManager startupManager(this);
    StartupValidationResult startupResult = startupManager.validateStartup();
    
    // Handle startup issues if any
    if (!startupResult.canStart || startupResult.hasWarnings) {
        StartupGuidanceDialog guidanceDialog(startupResult, &startupManager, this);
        
        // Show guidance dialog
        int dialogResult = guidanceDialog.exec();
        
        if (dialogResult == QDialog::Rejected || guidanceDialog.getResult() == StartupGuidanceDialog::Result::Exit) {
            // User chose to exit
            QTimer::singleShot(0, qApp, &QApplication::quit);
            return;
        }
        
        if (guidanceDialog.getResult() == StartupGuidanceDialog::Result::TryAgain) {
            // Re-validate after user attempted fixes
            startupResult = startupManager.validateStartup();
            
            if (!startupResult.canStart) {
                QMessageBox::critical(this, tr("Startup Failed"), 
                                    tr("Critical issues still prevent NFS Share Manager from starting. "
                                       "Please resolve the dependency and service issues manually."));
                QTimer::singleShot(0, qApp, &QApplication::quit);
                return;
            }
        }
        
        if (guidanceDialog.getResult() == StartupGuidanceDialog::Result::InstallDependencies) {
            // Handle dependency installation
            if (!startupManager.handleMissingDependencies(startupResult.missingDependencies)) {
                QMessageBox::warning(this, tr("Dependency Installation"), 
                                    tr("Some dependencies could not be installed. "
                                       "The application will start but some features may not work properly."));
            }
        }
        
        if (guidanceDialog.getResult() == StartupGuidanceDialog::Result::StartServices) {
            // Handle service startup
            if (!startupManager.handleUnavailableServices(startupResult.unavailableServices)) {
                QMessageBox::warning(this, tr("Service Startup"), 
                                    tr("Some services could not be started. "
                                       "The application will start but some features may not work properly."));
            }
        }
    }
    
    // Validate and restore configuration
    if (!startupManager.validateAndRestoreConfiguration(m_configurationManager)) {
        QMessageBox::critical(this, tr("Configuration Error"), 
                             tr("Failed to load or restore configuration. "
                                "The application may not work properly."));
    }
    
    // Initialize operation manager
    m_operationManager = new OperationManager(this, this);
    
    initializeComponents();
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupSystemTray();
    connectSignals();
    loadConfiguration();
    
    // Validate component integration
    if (!validateComponentIntegration()) {
        qWarning() << "Component integration validation failed - some features may not work properly";
    }
    
    // Start status update timer
    m_statusUpdateTimer->setInterval(5000); // Update every 5 seconds
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &NFSShareManagerApp::onStatusUpdateTimer);
    m_statusUpdateTimer->start();
    
    // Start network discovery
    m_networkDiscovery->startDiscovery();
    
    // Initialize discovery UI state
    updateDiscoveryModeLabel();
    updateDiscoveryStatistics();
    
    showStatusMessage(tr("NFS Share Manager started successfully"));
}

NFSShareManagerApp::~NFSShareManagerApp()
{
    qDebug() << "Shutting down NFS Share Manager...";
    
    // Stop network discovery first to prevent new operations
    if (m_networkDiscovery) {
        m_networkDiscovery->stopDiscovery();
    }
    
    // Cancel any ongoing operations
    if (m_operationManager) {
        m_operationManager->cancelAllOperations();
    }
    
    // Save current configuration before shutdown
    if (m_configurationManager) {
        m_configurationManager->saveConfiguration();
        
        // Create shutdown backup if enabled
        bool autoBackup = m_configurationManager->getPreference(QStringLiteral("AutoBackup"), true).toBool();
        if (autoBackup) {
            QString backupPath = m_configurationManager->createBackup("shutdown");
            if (!backupPath.isEmpty()) {
                qDebug() << "Shutdown backup created:" << backupPath;
            }
        }
    }
    
    // Gracefully shutdown components in reverse dependency order
    
    // Shutdown notification manager
    if (m_notificationManager) {
        m_notificationManager->shutdown();
    }
    
    // Shutdown operation manager
    if (m_operationManager) {
        m_operationManager->shutdown();
    }
    
    // Shutdown UI-related managers
    if (m_permissionManager) {
        m_permissionManager->shutdown();
    }
    
    // Shutdown business logic managers
    if (m_mountManager) {
        m_mountManager->shutdown();
    }
    
    if (m_shareManager) {
        m_shareManager->shutdown();
    }
    
    if (m_networkDiscovery) {
        m_networkDiscovery->shutdown();
    }
    
    // Shutdown core configuration manager last
    if (m_configurationManager) {
        m_configurationManager->shutdown();
    }
    
    qDebug() << "NFS Share Manager shutdown completed";
}

void NFSShareManagerApp::setupUI()
{
    setWindowTitle(tr("NFS Share Manager"));
    setWindowIcon(QIcon::fromTheme("network-server", style()->standardIcon(QStyle::SP_ComputerIcon)));
    resize(1000, 700);
    setMinimumSize(800, 600);
    
    setupCentralWidget();
}

void NFSShareManagerApp::setupMenuBar()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    m_exportConfigAction = new QAction(QIcon::fromTheme("document-export"), tr("&Export Configuration..."), this);
    m_exportConfigAction->setShortcut(QKeySequence::SaveAs);
    m_exportConfigAction->setStatusTip(tr("Export current configuration to file"));
    connect(m_exportConfigAction, &QAction::triggered, this, [this]() {
        // TODO: Implement export dialog
        showStatusMessage(tr("Export configuration not yet implemented"));
    });
    fileMenu->addAction(m_exportConfigAction);
    
    m_importConfigAction = new QAction(QIcon::fromTheme("document-import"), tr("&Import Configuration..."), this);
    m_importConfigAction->setShortcut(QKeySequence::Open);
    m_importConfigAction->setStatusTip(tr("Import configuration from file"));
    connect(m_importConfigAction, &QAction::triggered, this, [this]() {
        // TODO: Implement import dialog
        showStatusMessage(tr("Import configuration not yet implemented"));
    });
    fileMenu->addAction(m_importConfigAction);
    
    fileMenu->addSeparator();
    
    QAction *quitAction = new QAction(QIcon::fromTheme("application-exit"), tr("&Quit"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    quitAction->setStatusTip(tr("Quit the application"));
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(quitAction);
    
    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    
    m_refreshAction = new QAction(QIcon::fromTheme("view-refresh"), tr("&Refresh All"), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh);
    m_refreshAction->setStatusTip(tr("Refresh all shares and mounts"));
    connect(m_refreshAction, &QAction::triggered, this, &NFSShareManagerApp::refreshAll);
    viewMenu->addAction(m_refreshAction);
    
    // Settings menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    
    m_preferencesAction = new QAction(QIcon::fromTheme("configure"), tr("&Preferences..."), this);
    m_preferencesAction->setShortcut(QKeySequence::Preferences);
    m_preferencesAction->setStatusTip(tr("Configure application preferences"));
    connect(m_preferencesAction, &QAction::triggered, this, &NFSShareManagerApp::showPreferences);
    settingsMenu->addAction(m_preferencesAction);
    
    // Add notification preferences
    QAction *notificationPrefsAction = new QAction(QIcon::fromTheme("preferences-desktop-notification"), 
                                                  tr("&Notification Preferences..."), this);
    notificationPrefsAction->setStatusTip(tr("Configure notification settings"));
    connect(notificationPrefsAction, &QAction::triggered, this, &NFSShareManagerApp::showNotificationPreferences);
    settingsMenu->addAction(notificationPrefsAction);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    m_aboutAction = new QAction(QIcon::fromTheme("help-about"), tr("&About"), this);
    m_aboutAction->setStatusTip(tr("Show information about this application"));
    connect(m_aboutAction, &QAction::triggered, this, &NFSShareManagerApp::showAbout);
    helpMenu->addAction(m_aboutAction);
}

void NFSShareManagerApp::setupToolBar()
{
    QToolBar *mainToolBar = addToolBar(tr("Main"));
    mainToolBar->setObjectName("MainToolBar");
    
    mainToolBar->addAction(m_refreshAction);
    mainToolBar->addSeparator();
    
    // Add quick action buttons that will be connected later
    QAction *createShareAction = new QAction(QIcon::fromTheme("folder-new"), tr("Create Share"), this);
    createShareAction->setStatusTip(tr("Create a new NFS share"));
    connect(createShareAction, &QAction::triggered, this, &NFSShareManagerApp::onCreateShareClicked);
    mainToolBar->addAction(createShareAction);
    
    QAction *mountShareAction = new QAction(QIcon::fromTheme("drive-harddisk"), tr("Mount Share"), this);
    mountShareAction->setStatusTip(tr("Mount a remote NFS share"));
    connect(mountShareAction, &QAction::triggered, this, &NFSShareManagerApp::onMountShareClicked);
    mainToolBar->addAction(mountShareAction);
}

void NFSShareManagerApp::setupStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
    statusBar()->setSizeGripEnabled(true);
    
    // Add global progress indication to status bar
    m_globalProgressBar = new QProgressBar();
    m_globalProgressBar->setMaximumWidth(200);
    m_globalProgressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_globalProgressBar);
    
    m_globalProgressLabel = new QLabel();
    m_globalProgressLabel->setVisible(false);
    statusBar()->addPermanentWidget(m_globalProgressLabel);
    
    m_cancelOperationsButton = new QPushButton(tr("Cancel"));
    m_cancelOperationsButton->setMaximumWidth(60);
    m_cancelOperationsButton->setVisible(false);
    connect(m_cancelOperationsButton, &QPushButton::clicked, this, [this]() {
        if (m_operationManager) {
            m_operationManager->cancelAllOperations();
        }
    });
    statusBar()->addPermanentWidget(m_cancelOperationsButton);
}

void NFSShareManagerApp::setupCentralWidget()
{
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    setupLocalSharesTab();
    setupRemoteSharesTab();
    setupMountedSharesTab();
}

void NFSShareManagerApp::setupLocalSharesTab()
{
    m_localSharesTab = new QWidget();
    m_tabWidget->addTab(m_localSharesTab, QIcon::fromTheme("folder-open"), tr("Local Shares"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_localSharesTab);
    
    // Status label
    m_localSharesStatus = new QLabel(tr("Loading local shares..."));
    layout->addWidget(m_localSharesStatus);
    
    // Shares list
    m_localSharesList = new QListWidget();
    m_localSharesList->setAlternatingRowColors(true);
    m_localSharesList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_localSharesList);
    
    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_createShareButton = new QPushButton(QIcon::fromTheme("folder-new"), tr("Create Share"));
    m_createShareButton->setToolTip(tr("Create a new NFS share from a local directory"));
    connect(m_createShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onCreateShareClicked);
    buttonLayout->addWidget(m_createShareButton);
    
    m_editShareButton = new QPushButton(QIcon::fromTheme("document-edit"), tr("Edit Share"));
    m_editShareButton->setToolTip(tr("Edit the selected share's configuration"));
    m_editShareButton->setEnabled(false);
    connect(m_editShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onEditShareClicked);
    buttonLayout->addWidget(m_editShareButton);
    
    m_removeShareButton = new QPushButton(QIcon::fromTheme("edit-delete"), tr("Remove Share"));
    m_removeShareButton->setToolTip(tr("Remove the selected NFS share"));
    m_removeShareButton->setEnabled(false);
    connect(m_removeShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onRemoveShareClicked);
    buttonLayout->addWidget(m_removeShareButton);
    
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    // Connect list selection changes
    connect(m_localSharesList, &QListWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = !m_localSharesList->selectedItems().isEmpty();
        m_editShareButton->setEnabled(hasSelection);
        m_removeShareButton->setEnabled(hasSelection);
    });
}

void NFSShareManagerApp::setupRemoteSharesTab()
{
    m_remoteSharesTab = new QWidget();
    m_tabWidget->addTab(m_remoteSharesTab, QIcon::fromTheme("network-server"), tr("Remote Shares"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_remoteSharesTab);
    
    // Discovery status and controls layout
    QHBoxLayout *discoveryControlsLayout = new QHBoxLayout();
    
    // Discovery status label
    m_remoteSharesStatus = new QLabel(tr("Discovering network shares..."));
    m_remoteSharesStatus->setStyleSheet("QLabel { font-weight: bold; }");
    discoveryControlsLayout->addWidget(m_remoteSharesStatus);
    
    // Discovery progress bar
    m_discoveryProgress = new QProgressBar();
    m_discoveryProgress->setRange(0, 0); // Indeterminate progress
    m_discoveryProgress->setMaximumWidth(200);
    m_discoveryProgress->setVisible(false); // Initially hidden
    discoveryControlsLayout->addWidget(m_discoveryProgress);
    
    // Auto-discovery status indicator
    m_autoDiscoveryStatus = new QLabel();
    m_autoDiscoveryStatus->setPixmap(QIcon::fromTheme("dialog-information").pixmap(16, 16));
    m_autoDiscoveryStatus->setToolTip(tr("Automatic discovery is active"));
    discoveryControlsLayout->addWidget(m_autoDiscoveryStatus);
    
    discoveryControlsLayout->addStretch();
    
    // Last scan time label
    m_lastScanTimeLabel = new QLabel(tr("Last scan: Never"));
    m_lastScanTimeLabel->setStyleSheet("QLabel { color: gray; font-size: 10pt; }");
    discoveryControlsLayout->addWidget(m_lastScanTimeLabel);
    
    layout->addLayout(discoveryControlsLayout);
    
    // Discovery statistics layout
    QHBoxLayout *statsLayout = new QHBoxLayout();
    
    m_discoveryStatsLabel = new QLabel(tr("Hosts scanned: 0 | Shares found: 0"));
    m_discoveryStatsLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    statsLayout->addWidget(m_discoveryStatsLabel);
    
    statsLayout->addStretch();
    
    // Discovery mode indicator
    m_discoveryModeLabel = new QLabel(tr("Mode: Quick"));
    m_discoveryModeLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    statsLayout->addWidget(m_discoveryModeLabel);
    
    layout->addLayout(statsLayout);
    
    // Remote shares list with enhanced display
    m_remoteSharesList = new QListWidget();
    m_remoteSharesList->setAlternatingRowColors(true);
    m_remoteSharesList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_remoteSharesList->setIconSize(QSize(24, 24));
    m_remoteSharesList->setSpacing(2);
    layout->addWidget(m_remoteSharesList);
    
    // Enhanced button layout with more options
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_mountShareButton = new QPushButton(QIcon::fromTheme("drive-harddisk"), tr("Mount Share"));
    m_mountShareButton->setToolTip(tr("Mount the selected remote NFS share"));
    m_mountShareButton->setEnabled(false);
    connect(m_mountShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onMountShareClicked);
    buttonLayout->addWidget(m_mountShareButton);
    
    m_refreshDiscoveryButton = new QPushButton(QIcon::fromTheme("view-refresh"), tr("Refresh Discovery"));
    m_refreshDiscoveryButton->setToolTip(tr("Refresh the list of available network shares"));
    connect(m_refreshDiscoveryButton, &QPushButton::clicked, this, &NFSShareManagerApp::onRefreshDiscoveryClicked);
    buttonLayout->addWidget(m_refreshDiscoveryButton);
    
    // Add discovery mode selection button
    m_discoveryModeButton = new QPushButton(QIcon::fromTheme("configure"), tr("Discovery Mode"));
    m_discoveryModeButton->setToolTip(tr("Change network discovery scan mode"));
    connect(m_discoveryModeButton, &QPushButton::clicked, this, &NFSShareManagerApp::onDiscoveryModeClicked);
    buttonLayout->addWidget(m_discoveryModeButton);
    
    // Add target hosts management button
    m_manageTargetsButton = new QPushButton(QIcon::fromTheme("list-add"), tr("Manage Targets"));
    m_manageTargetsButton->setToolTip(tr("Add or remove specific hosts to scan"));
    connect(m_manageTargetsButton, &QPushButton::clicked, this, &NFSShareManagerApp::onManageTargetsClicked);
    buttonLayout->addWidget(m_manageTargetsButton);
    
    buttonLayout->addStretch();
    
    // Auto-discovery toggle
    m_autoDiscoveryToggle = new QPushButton(QIcon::fromTheme("media-playback-pause"), tr("Pause Auto-Discovery"));
    m_autoDiscoveryToggle->setToolTip(tr("Pause or resume automatic network discovery"));
    m_autoDiscoveryToggle->setCheckable(true);
    connect(m_autoDiscoveryToggle, &QPushButton::toggled, this, &NFSShareManagerApp::onAutoDiscoveryToggled);
    buttonLayout->addWidget(m_autoDiscoveryToggle);
    
    layout->addLayout(buttonLayout);
    
    // Connect list selection changes
    connect(m_remoteSharesList, &QListWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = !m_remoteSharesList->selectedItems().isEmpty();
        m_mountShareButton->setEnabled(hasSelection);
    });
    
    // Connect double-click to mount
    connect(m_remoteSharesList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if (item && m_mountShareButton->isEnabled()) {
            onMountShareClicked();
        }
    });
}

void NFSShareManagerApp::setupMountedSharesTab()
{
    m_mountedSharesTab = new QWidget();
    m_tabWidget->addTab(m_mountedSharesTab, QIcon::fromTheme("drive-harddisk-mounted"), tr("Mounted Shares"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_mountedSharesTab);
    
    // Status label
    m_mountedSharesStatus = new QLabel(tr("Loading mounted shares..."));
    layout->addWidget(m_mountedSharesStatus);
    
    // Mounted shares list
    m_mountedSharesList = new QListWidget();
    m_mountedSharesList->setAlternatingRowColors(true);
    m_mountedSharesList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_mountedSharesList);
    
    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_unmountShareButton = new QPushButton(QIcon::fromTheme("drive-harddisk-unmounted"), tr("Unmount Share"));
    m_unmountShareButton->setToolTip(tr("Unmount the selected NFS share"));
    m_unmountShareButton->setEnabled(false);
    connect(m_unmountShareButton, &QPushButton::clicked, this, &NFSShareManagerApp::onUnmountShareClicked);
    buttonLayout->addWidget(m_unmountShareButton);
    
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
    
    // Connect list selection changes
    connect(m_mountedSharesList, &QListWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = !m_mountedSharesList->selectedItems().isEmpty();
        m_unmountShareButton->setEnabled(hasSelection);
    });
}

void NFSShareManagerApp::initializeComponents()
{
    // Set up component interdependencies
    
    // Configure ShareManager with dependencies
    if (m_shareManager && m_configurationManager) {
        // Share manager needs configuration for default settings
        m_shareManager->setConfigurationManager(m_configurationManager);
    }
    
    // Configure MountManager with dependencies
    if (m_mountManager && m_configurationManager) {
        // Mount manager needs configuration for mount options and persistent mounts
        m_mountManager->setConfigurationManager(m_configurationManager);
    }
    
    // Configure NetworkDiscovery with dependencies
    if (m_networkDiscovery && m_configurationManager) {
        // Network discovery needs configuration for scan settings and target hosts
        m_networkDiscovery->setConfigurationManager(m_configurationManager);
    }
    
    // Configure PermissionManager with dependencies
    if (m_permissionManager && m_configurationManager) {
        // Permission manager needs configuration for default permissions
        m_permissionManager->setConfigurationManager(m_configurationManager);
    }
    
    // Set up cross-component relationships
    
    // ShareManager and PermissionManager integration
    if (m_shareManager && m_permissionManager) {
        // Connect share manager to permission manager for permission validation
        connect(m_shareManager, &ShareManager::permissionValidationRequested,
                m_permissionManager, &PermissionManager::validatePermissions);
        
        connect(m_permissionManager, &PermissionManager::permissionValidated,
                m_shareManager, &ShareManager::onPermissionValidated);
    }
    
    // MountManager and NetworkDiscovery integration
    if (m_mountManager && m_networkDiscovery) {
        // Mount manager can use network discovery to validate remote shares
        connect(m_mountManager, &MountManager::remoteShareValidationRequested,
                m_networkDiscovery, &NetworkDiscovery::validateRemoteShare);
        
        connect(m_networkDiscovery, &NetworkDiscovery::remoteShareValidated,
                m_mountManager, &MountManager::onRemoteShareValidated);
    }
    
    // NotificationManager integration with all components
    if (m_notificationManager) {
        // Set notification preferences from configuration
        if (m_configurationManager) {
            NotificationPreferences prefs = m_configurationManager->getNotificationPreferences();
            m_notificationManager->setPreferences(prefs);
        }
        
        // Connect notification manager to configuration changes
        if (m_configurationManager) {
            connect(m_configurationManager, &ConfigurationManager::notificationPreferencesChanged,
                    m_notificationManager, &NotificationManager::setPreferences);
        }
    }
    
    // Initialize component states
    
    // Initialize share manager
    if (m_shareManager) {
        m_shareManager->initialize();
    }
    
    // Initialize mount manager
    if (m_mountManager) {
        m_mountManager->initialize();
    }
    
    // Initialize network discovery (but don't start scanning yet)
    if (m_networkDiscovery) {
        m_networkDiscovery->initialize();
    }
    
    // Initialize permission manager
    if (m_permissionManager) {
        m_permissionManager->initialize();
    }
    
    qDebug() << "Component initialization completed";
}

bool NFSShareManagerApp::validateComponentIntegration()
{
    bool allValid = true;
    QStringList issues;
    
    // Validate core components
    if (!m_configurationManager) {
        issues << tr("Configuration manager is not available");
        allValid = false;
    }
    
    if (!m_shareManager) {
        issues << tr("Share manager is not available");
        allValid = false;
    }
    
    if (!m_mountManager) {
        issues << tr("Mount manager is not available");
        allValid = false;
    }
    
    if (!m_networkDiscovery) {
        issues << tr("Network discovery is not available");
        allValid = false;
    }
    
    if (!m_permissionManager) {
        issues << tr("Permission manager is not available");
        allValid = false;
    }
    
    if (!m_notificationManager) {
        issues << tr("Notification manager is not available");
        allValid = false;
    }
    
    if (!m_operationManager) {
        issues << tr("Operation manager is not available");
        allValid = false;
    }
    
    // Validate component states
    if (m_shareManager && !m_shareManager->isInitialized()) {
        issues << tr("Share manager failed to initialize");
        allValid = false;
    }
    
    if (m_mountManager && !m_mountManager->isInitialized()) {
        issues << tr("Mount manager failed to initialize");
        allValid = false;
    }
    
    if (m_networkDiscovery && !m_networkDiscovery->isInitialized()) {
        issues << tr("Network discovery failed to initialize");
        allValid = false;
    }
    
    if (m_permissionManager && !m_permissionManager->isInitialized()) {
        issues << tr("Permission manager failed to initialize");
        allValid = false;
    }
    
    // Log validation results
    if (allValid) {
        qDebug() << "Component integration validation passed";
    } else {
        qWarning() << "Component integration validation failed:";
        for (const QString &issue : issues) {
            qWarning() << " -" << issue;
        }
        
        // Show user notification about integration issues
        if (m_notificationManager) {
            QString message = tr("Some components failed to initialize properly:\n%1\n\n"
                               "The application may not function correctly.")
                             .arg(issues.join("\n"));
            m_notificationManager->showError(tr("Component Integration Issues"), message);
        }
    }
    
    return allValid;
}

void NFSShareManagerApp::handleComponentFailure(const QString &componentName, const QString &error)
{
    qCritical() << "Component failure:" << componentName << "-" << error;
    
    // Log the failure
    QString logMessage = tr("Component '%1' failed: %2").arg(componentName, error);
    
    // Show user notification
    if (m_notificationManager) {
        m_notificationManager->showError(tr("Component Failure"), logMessage);
    } else {
        // Fallback to message box if notification manager is not available
        QMessageBox::critical(this, tr("Component Failure"), logMessage);
    }
    
    // Update status
    showStatusMessage(tr("Component failure: %1").arg(componentName), 10000);
    
    // Disable related UI elements based on which component failed
    if (componentName == "ShareManager") {
        if (m_createShareButton) m_createShareButton->setEnabled(false);
        if (m_editShareButton) m_editShareButton->setEnabled(false);
        if (m_removeShareButton) m_removeShareButton->setEnabled(false);
        if (m_localSharesStatus) {
            m_localSharesStatus->setText(tr("Share management unavailable"));
            m_localSharesStatus->setStyleSheet("QLabel { color: red; }");
        }
    } else if (componentName == "MountManager") {
        if (m_mountShareButton) m_mountShareButton->setEnabled(false);
        if (m_unmountShareButton) m_unmountShareButton->setEnabled(false);
        if (m_mountedSharesStatus) {
            m_mountedSharesStatus->setText(tr("Mount management unavailable"));
            m_mountedSharesStatus->setStyleSheet("QLabel { color: red; }");
        }
    } else if (componentName == "NetworkDiscovery") {
        if (m_refreshDiscoveryButton) m_refreshDiscoveryButton->setEnabled(false);
        if (m_discoveryModeButton) m_discoveryModeButton->setEnabled(false);
        if (m_manageTargetsButton) m_manageTargetsButton->setEnabled(false);
        if (m_autoDiscoveryToggle) m_autoDiscoveryToggle->setEnabled(false);
        if (m_remoteSharesStatus) {
            m_remoteSharesStatus->setText(tr("Network discovery unavailable"));
            m_remoteSharesStatus->setStyleSheet("QLabel { color: red; }");
        }
    }
}

void NFSShareManagerApp::loadConfiguration()
{
    qDebug() << "Loading application configuration...";
    
    // Load configuration using the configuration manager
    if (m_configurationManager->loadConfiguration()) {
        qDebug() << "Configuration loaded successfully";
        
        // Validate configuration integrity
        ValidationResult validation = m_configurationManager->validateConfiguration();
        if (!validation.isValid) {
            qWarning() << "Configuration validation failed:" << validation.errors;
            
            if (validation.canAutoRepair) {
                qDebug() << "Attempting to repair configuration...";
                if (m_configurationManager->repairConfiguration(validation)) {
                    qDebug() << "Configuration repaired successfully";
                } else {
                    qWarning() << "Failed to repair configuration";
                }
            }
        }
        
        // Create automatic backup if enabled
        bool autoBackup = m_configurationManager->getPreference(QStringLiteral("AutoBackup"), true).toBool();
        if (autoBackup) {
            QString backupPath = m_configurationManager->createBackup();
            if (!backupPath.isEmpty()) {
                qDebug() << "Automatic backup created:" << backupPath;
            }
        }
        
    } else {
        qWarning() << "Failed to load configuration, using defaults";
    }
}

bool NFSShareManagerApp::exportConfiguration(const QString &filePath, const QString &profileName, 
                                            const QString &description)
{
    return m_configurationManager->exportConfiguration(filePath, profileName, description);
}

bool NFSShareManagerApp::importConfiguration(const QString &filePath, bool mergeMode)
{
    return m_configurationManager->importConfiguration(filePath, mergeMode);
}

void NFSShareManagerApp::onConfigurationValidationFailed(const ValidationResult &result)
{
    qWarning() << "Configuration validation failed with" << result.errors.size() << "errors";
    
    // Use notification manager for configuration validation failure
    if (m_notificationManager) {
        QString message = tr("Configuration validation failed with %1 errors").arg(result.errors.size());
        if (!result.errors.isEmpty()) {
            message += "\n" + result.errors.first(); // Show first error
        }
        m_notificationManager->showError(tr("Configuration Error"), message);
    }
    
    // Log all issues for debugging
    for (const QString &error : result.errors) {
        qWarning() << "Configuration error:" << error;
    }
    
    for (const QString &warning : result.warnings) {
        qWarning() << "Configuration warning:" << warning;
    }
    
    if (result.canAutoRepair && !result.repairActions.isEmpty()) {
        qDebug() << "Suggested repair actions:";
        for (const QString &action : result.repairActions) {
            qDebug() << " -" << action;
        }
    }
}

void NFSShareManagerApp::onConfigurationRepaired(const QStringList &repairActions)
{
    qDebug() << "Configuration repaired with" << repairActions.size() << "actions:";
    for (const QString &action : repairActions) {
        qDebug() << " -" << action;
    }
    
    // Use notification manager for configuration repair notification
    if (m_notificationManager) {
        QString message = tr("Configuration automatically repaired with %1 actions").arg(repairActions.size());
        m_notificationManager->notifyConfigurationChanged(message);
    }
}

void NFSShareManagerApp::onBackupCreated(const QString &backupPath)
{
    qDebug() << "Configuration backup created:" << backupPath;
    
    // Use notification manager for backup creation notification
    if (m_notificationManager) {
        m_notificationManager->notifyBackupCreated(backupPath);
    }
}

void NFSShareManagerApp::connectConfigurationSignals()
{
    connect(m_configurationManager, &ConfigurationManager::configurationValidationFailed,
            this, &NFSShareManagerApp::onConfigurationValidationFailed);
    
    connect(m_configurationManager, &ConfigurationManager::configurationRepaired,
            this, &NFSShareManagerApp::onConfigurationRepaired);
    
    connect(m_configurationManager, &ConfigurationManager::backupCreated,
            this, &NFSShareManagerApp::onBackupCreated);
}

} // namespace NFSShareManager
void NFSShareManagerApp::setupSystemTray()
{
    m_systemTrayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    
    if (!m_systemTrayAvailable) {
        qDebug() << "System tray is not available on this system";
        return;
    }
    
    // Create system tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon::fromTheme("network-server", style()->standardIcon(QStyle::SP_ComputerIcon)));
    m_trayIcon->setToolTip(tr("NFS Share Manager"));
    
    // Create tray menu
    m_trayMenu = new QMenu(this);
    
    m_showAction = new QAction(QIcon::fromTheme("window-restore"), tr("&Show"), this);
    connect(m_showAction, &QAction::triggered, this, &NFSShareManagerApp::showMainWindow);
    m_trayMenu->addAction(m_showAction);
    
    m_hideAction = new QAction(QIcon::fromTheme("window-minimize"), tr("&Hide"), this);
    connect(m_hideAction, &QAction::triggered, this, &NFSShareManagerApp::hideToTray);
    m_trayMenu->addAction(m_hideAction);
    
    m_trayMenu->addSeparator();
    
    // Add quick status items
    QAction *statusAction = new QAction(tr("Status: Initializing..."), this);
    statusAction->setEnabled(false);
    m_trayMenu->addAction(statusAction);
    
    m_trayMenu->addSeparator();
    
    m_quitAction = new QAction(QIcon::fromTheme("application-exit"), tr("&Quit"), this);
    connect(m_quitAction, &QAction::triggered, this, &QWidget::close);
    m_trayMenu->addAction(m_quitAction);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    
    // Connect tray icon signals
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &NFSShareManagerApp::onTrayIconActivated);
    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, &NFSShareManagerApp::onTrayIconMessageClicked);
    
    // Show the tray icon
    m_trayIcon->show();
    
    // Set up notification manager integration with system tray
    if (m_notificationManager) {
        m_notificationManager->setSystemTrayIcon(m_trayIcon);
    }
    
    qDebug() << "System tray icon initialized successfully";
}

void NFSShareManagerApp::connectSignals()
{
    connectConfigurationSignals();
    connectShareManagerSignals();
    connectMountManagerSignals();
    connectNetworkDiscoverySignals();
    connectOperationManagerSignals();
    connectComponentLifecycleSignals();
}

void NFSShareManagerApp::connectComponentLifecycleSignals()
{
    // Connect component failure signals
    if (m_shareManager) {
        connect(m_shareManager, &ShareManager::componentFailed,
                this, [this](const QString &error) {
                    handleComponentFailure("ShareManager", error);
                });
    }
    
    if (m_mountManager) {
        connect(m_mountManager, &MountManager::componentFailed,
                this, [this](const QString &error) {
                    handleComponentFailure("MountManager", error);
                });
    }
    
    if (m_networkDiscovery) {
        connect(m_networkDiscovery, &NetworkDiscovery::componentFailed,
                this, [this](const QString &error) {
                    handleComponentFailure("NetworkDiscovery", error);
                });
    }
    
    if (m_permissionManager) {
        connect(m_permissionManager, &PermissionManager::componentFailed,
                this, [this](const QString &error) {
                    handleComponentFailure("PermissionManager", error);
                });
    }
    
    if (m_configurationManager) {
        connect(m_configurationManager, &ConfigurationManager::componentFailed,
                this, [this](const QString &error) {
                    handleComponentFailure("ConfigurationManager", error);
                });
    }
    
    if (m_notificationManager) {
        connect(m_notificationManager, &NotificationManager::componentFailed,
                this, [this](const QString &error) {
                    handleComponentFailure("NotificationManager", error);
                });
    }
    
    if (m_operationManager) {
        connect(m_operationManager, &OperationManager::componentFailed,
                this, [this](const QString &error) {
                    handleComponentFailure("OperationManager", error);
                });
    }
}

void NFSShareManagerApp::connectShareManagerSignals()
{
    connect(m_shareManager, &ShareManager::shareCreated,
            this, &NFSShareManagerApp::onShareCreated);
    
    connect(m_shareManager, &ShareManager::shareRemoved,
            this, &NFSShareManagerApp::onShareRemoved);
    
    connect(m_shareManager, &ShareManager::shareUpdated,
            this, &NFSShareManagerApp::onShareUpdated);
    
    connect(m_shareManager, &ShareManager::shareError,
            this, &NFSShareManagerApp::onShareError);
    
    connect(m_shareManager, &ShareManager::sharesRefreshed,
            this, &NFSShareManagerApp::onSharesRefreshed);
    
    connect(m_shareManager, &ShareManager::nfsServerStatusChanged,
            this, &NFSShareManagerApp::onNFSServerStatusChanged);
}

void NFSShareManagerApp::connectMountManagerSignals()
{
    connect(m_mountManager, &MountManager::mountStarted,
            this, &NFSShareManagerApp::onMountStarted);
    
    connect(m_mountManager, &MountManager::mountCompleted,
            this, &NFSShareManagerApp::onMountCompleted);
    
    connect(m_mountManager, &MountManager::mountFailed,
            this, &NFSShareManagerApp::onMountFailed);
    
    connect(m_mountManager, &MountManager::unmountStarted,
            this, &NFSShareManagerApp::onUnmountStarted);
    
    connect(m_mountManager, &MountManager::unmountCompleted,
            this, &NFSShareManagerApp::onUnmountCompleted);
    
    connect(m_mountManager, &MountManager::unmountFailed,
            this, &NFSShareManagerApp::onUnmountFailed);
    
    connect(m_mountManager, &MountManager::mountStatusChanged,
            this, &NFSShareManagerApp::onMountStatusChanged);
}

void NFSShareManagerApp::connectNetworkDiscoverySignals()
{
    connect(m_networkDiscovery, &NetworkDiscovery::shareDiscovered,
            this, &NFSShareManagerApp::onShareDiscovered);
    
    connect(m_networkDiscovery, &NetworkDiscovery::shareUnavailable,
            this, &NFSShareManagerApp::onShareUnavailable);
    
    connect(m_networkDiscovery, &NetworkDiscovery::discoveryCompleted,
            this, &NFSShareManagerApp::onDiscoveryCompleted);
    
    connect(m_networkDiscovery, &NetworkDiscovery::discoveryError,
            this, &NFSShareManagerApp::onDiscoveryError);
    
    connect(m_networkDiscovery, &NetworkDiscovery::discoveryStarted,
            this, &NFSShareManagerApp::onDiscoveryStarted);
    
    connect(m_networkDiscovery, &NetworkDiscovery::discoveryStatusChanged,
            this, &NFSShareManagerApp::onDiscoveryStatusChanged);
    
    connect(m_networkDiscovery, &NetworkDiscovery::scanProgress,
            this, &NFSShareManagerApp::onScanProgress);
}

void NFSShareManagerApp::connectOperationManagerSignals()
{
    if (!m_operationManager) {
        return;
    }
    
    connect(m_operationManager, &OperationManager::operationStarted,
            this, &NFSShareManagerApp::onOperationStarted);
    
    connect(m_operationManager, &OperationManager::operationProgressUpdated,
            this, &NFSShareManagerApp::onOperationProgressUpdated);
    
    connect(m_operationManager, &OperationManager::operationCompleted,
            this, &NFSShareManagerApp::onOperationCompleted);
    
    connect(m_operationManager, &OperationManager::operationFailed,
            this, &NFSShareManagerApp::onOperationFailed);
    
    connect(m_operationManager, &OperationManager::operationCancelled,
            this, &NFSShareManagerApp::onOperationCancelled);
}

// Public slots implementation
void NFSShareManagerApp::showMainWindow()
{
    show();
    raise();
    activateWindow();
    
    if (isMinimized()) {
        showNormal();
    }
}

void NFSShareManagerApp::hideToTray()
{
    if (m_systemTrayAvailable && m_trayIcon) {
        hide();
        showStatusMessage(tr("Application was minimized to tray"));
    } else {
        showMinimized();
    }
}

void NFSShareManagerApp::refreshAll()
{
    showStatusMessage(tr("Refreshing all data..."));
    
    // Refresh shares
    if (m_shareManager) {
        m_shareManager->refreshShares();
    }
    
    // Refresh mounts
    if (m_mountManager) {
        m_mountManager->refreshMountStatus();
    }
    
    // Refresh network discovery
    if (m_networkDiscovery) {
        m_networkDiscovery->refreshDiscovery();
    }
    
    updateLocalSharesList();
    updateRemoteSharesList();
    updateMountedSharesList();
}

void NFSShareManagerApp::showAbout()
{
    QMessageBox::about(this, tr("About NFS Share Manager"),
                      tr("<h3>NFS Share Manager</h3>"
                         "<p>Version 1.0.0</p>"
                         "<p>A comprehensive KDE application for managing NFS shares and mounts.</p>"
                         "<p>Built with Qt6 and KDE Frameworks 6</p>"
                         "<p>Copyright © 2024 KDE Community</p>"));
}

void NFSShareManagerApp::showPreferences()
{
    // TODO: Implement general preferences dialog
    showStatusMessage(tr("General preferences dialog not yet implemented"));
}

void NFSShareManagerApp::showNotificationPreferences()
{
    if (!m_notificationManager) {
        showErrorMessage(tr("Error"), tr("Notification manager is not available"));
        return;
    }

    NotificationPreferencesDialog dialog(m_notificationManager->preferences(), this);
    if (dialog.exec() == QDialog::Accepted) {
        NotificationPreferences newPreferences = dialog.getPreferences();
        m_notificationManager->setPreferences(newPreferences);
        
        showSuccessMessage(tr("Notification preferences updated successfully"));
    }
}

// Event handlers
void NFSShareManagerApp::closeEvent(QCloseEvent *event)
{
    if (m_systemTrayAvailable && m_trayIcon && m_trayIcon->isVisible()) {
        // Hide to tray instead of closing
        hide();
        event->ignore();
        
        if (m_trayIcon) {
            m_trayIcon->showMessage(tr("NFS Share Manager"),
                                   tr("Application was minimized to the system tray. "
                                      "Click the tray icon to show the window."),
                                   QSystemTrayIcon::Information, 2000);
        }
    } else {
        event->accept();
    }
}

void NFSShareManagerApp::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_systemTrayAvailable && m_trayIcon) {
            hide();
        }
    }
}

// Signal handlers for business logic components
void NFSShareManagerApp::onShareCreated(const NFSShare &share)
{
    qDebug() << "Share created:" << share.path;
    updateLocalSharesList();
    
    // Use notification manager for share creation notification
    if (m_notificationManager) {
        m_notificationManager->notifyShareCreated(share);
    } else {
        showSuccessMessage(tr("Share created successfully: %1").arg(share.path));
    }
    
    updateSystemTrayTooltip();
}

void NFSShareManagerApp::onShareRemoved(const QString &sharePath)
{
    qDebug() << "Share removed:" << sharePath;
    updateLocalSharesList();
    
    // Use notification manager for share removal notification
    if (m_notificationManager) {
        m_notificationManager->notifyShareRemoved(sharePath);
    } else {
        showSuccessMessage(tr("Share removed successfully: %1").arg(sharePath));
    }
    
    updateSystemTrayTooltip();
}

void NFSShareManagerApp::onShareUpdated(const NFSShare &share)
{
    qDebug() << "Share updated:" << share.path;
    updateLocalSharesList();
    
    // Use notification manager for share update notification
    if (m_notificationManager) {
        m_notificationManager->notifyShareUpdated(share);
    } else {
        showSuccessMessage(tr("Share updated successfully: %1").arg(share.path));
    }
}

void NFSShareManagerApp::onShareError(const QString &path, const QString &error)
{
    qWarning() << "Share error for" << path << ":" << error;
    
    // Use notification manager for share error notification
    if (m_notificationManager) {
        m_notificationManager->notifyShareError(path, error);
    } else {
        showErrorMessage(tr("Share Error"), tr("Error with share %1: %2").arg(path, error));
    }
}

void NFSShareManagerApp::onSharesRefreshed()
{
    qDebug() << "Shares refreshed";
    updateLocalSharesList();
    showStatusMessage(tr("Local shares refreshed"));
}

void NFSShareManagerApp::onNFSServerStatusChanged(bool running)
{
    qDebug() << "NFS server status changed:" << (running ? "running" : "stopped");
    QString status = running ? tr("running") : tr("stopped");
    showStatusMessage(tr("NFS server is %1").arg(status));
    updateSystemTrayTooltip();
}

void NFSShareManagerApp::onMountStarted(const RemoteNFSShare &remoteShare, const QString &mountPoint)
{
    qDebug() << "Mount started:" << remoteShare.hostName() << remoteShare.exportPath() << "to" << mountPoint;
    
    // Use notification manager for mount start notification
    if (m_notificationManager) {
        m_notificationManager->notifyMountStarted(remoteShare, mountPoint);
    }
    
    showStatusMessage(tr("Starting mount: %1:%2 to %3")
                     .arg(remoteShare.hostName(), remoteShare.exportPath(), mountPoint));
}

void NFSShareManagerApp::onMountCompleted(const NFSMount &mount)
{
    qDebug() << "Mount completed:" << mount.localMountPoint();
    updateMountedSharesList();
    
    // Use notification manager for mount completion notification
    if (m_notificationManager) {
        m_notificationManager->notifyMountCompleted(mount);
    } else {
        showSuccessMessage(tr("Mount completed: %1").arg(mount.localMountPoint()));
    }
    
    updateSystemTrayTooltip();
}

void NFSShareManagerApp::onMountFailed(const RemoteNFSShare &remoteShare, const QString &mountPoint, 
                                      int result, const QString &errorMessage)
{
    qWarning() << "Mount failed:" << remoteShare.hostName() << remoteShare.exportPath() << "to" << mountPoint << ":" << errorMessage;
    
    // Use notification manager for mount failure notification
    if (m_notificationManager) {
        m_notificationManager->notifyMountFailed(remoteShare, mountPoint, errorMessage);
    } else {
        showErrorMessage(tr("Mount Failed"), 
                        tr("Failed to mount %1:%2 to %3\n\nError: %4")
                        .arg(remoteShare.hostName(), remoteShare.exportPath(), mountPoint, errorMessage));
    }
}

void NFSShareManagerApp::onUnmountStarted(const QString &mountPoint)
{
    qDebug() << "Unmount started:" << mountPoint;
    
    // Use notification manager for unmount start notification
    if (m_notificationManager) {
        m_notificationManager->notifyUnmountStarted(mountPoint);
    }
    
    showStatusMessage(tr("Starting unmount: %1").arg(mountPoint));
}

void NFSShareManagerApp::onUnmountCompleted(const QString &mountPoint)
{
    qDebug() << "Unmount completed:" << mountPoint;
    updateMountedSharesList();
    
    // Use notification manager for unmount completion notification
    if (m_notificationManager) {
        m_notificationManager->notifyUnmountCompleted(mountPoint);
    } else {
        showSuccessMessage(tr("Unmount completed: %1").arg(mountPoint));
    }
    
    updateSystemTrayTooltip();
}

void NFSShareManagerApp::onUnmountFailed(const QString &mountPoint, const QString &errorMessage)
{
    qWarning() << "Unmount failed:" << mountPoint << ":" << errorMessage;
    
    // Use notification manager for unmount failure notification
    if (m_notificationManager) {
        m_notificationManager->notifyUnmountFailed(mountPoint, errorMessage);
    } else {
        showErrorMessage(tr("Unmount Failed"), 
                        tr("Failed to unmount %1\n\nError: %2").arg(mountPoint, errorMessage));
    }
}

void NFSShareManagerApp::onMountStatusChanged(const NFSMount &mount)
{
    qDebug() << "Mount status changed:" << mount.localMountPoint();
    updateMountedSharesList();
}

void NFSShareManagerApp::onShareDiscovered(const RemoteNFSShare &share)
{
    qDebug() << "Share discovered:" << share.hostName() << share.exportPath();
    updateRemoteSharesList();
    
    // Use notification manager for share discovery notification (low priority)
    if (m_notificationManager) {
        m_notificationManager->notifyShareDiscovered(share);
    }
}

void NFSShareManagerApp::onShareUnavailable(const QString &hostAddress, const QString &exportPath)
{
    qDebug() << "Share unavailable:" << hostAddress << exportPath;
    updateRemoteSharesList();
    
    // Use notification manager for share unavailable notification
    if (m_notificationManager) {
        m_notificationManager->notifyShareUnavailable(hostAddress, exportPath);
    }
}

void NFSShareManagerApp::onDiscoveryCompleted(int sharesFound, int hostsScanned)
{
    qDebug() << "Discovery completed:" << sharesFound << "shares found," << hostsScanned << "hosts scanned";
    updateRemoteSharesList();
    
    // Use notification manager for discovery completion notification
    if (m_notificationManager) {
        m_notificationManager->notifyDiscoveryCompleted(sharesFound, hostsScanned);
    } else {
        showStatusMessage(tr("Discovery completed: %1 shares found on %2 hosts").arg(sharesFound).arg(hostsScanned));
    }
    
    // Complete operation if we have one
    if (!m_currentDiscoveryOperationId.isNull() && m_operationManager->hasOperation(m_currentDiscoveryOperationId)) {
        m_operationManager->completeOperation(m_currentDiscoveryOperationId,
            tr("Discovery completed: %1 shares found on %2 hosts").arg(sharesFound).arg(hostsScanned));
        m_currentDiscoveryOperationId = QUuid();
    }
    
    // Hide progress bar
    if (m_discoveryProgress) {
        m_discoveryProgress->setVisible(false);
    }
    
    // Update discovery mode label (remove "scanning..." text)
    updateDiscoveryModeLabel();
    
    // Update statistics
    updateDiscoveryStatistics();
}

void NFSShareManagerApp::onDiscoveryError(const QString &error)
{
    qWarning() << "Discovery error:" << error;
    
    // Use notification manager for discovery error notification
    if (m_notificationManager) {
        m_notificationManager->notifyDiscoveryError(error);
    } else {
        showErrorMessage(tr("Discovery Error"), tr("Network discovery error: %1").arg(error));
    }
    
    // Fail operation if we have one
    if (!m_currentDiscoveryOperationId.isNull() && m_operationManager->hasOperation(m_currentDiscoveryOperationId)) {
        m_operationManager->failOperation(m_currentDiscoveryOperationId, error);
        m_currentDiscoveryOperationId = QUuid();
    }
    
    // Hide progress bar
    if (m_discoveryProgress) {
        m_discoveryProgress->setVisible(false);
    }
    
    // Update discovery mode label (remove "scanning..." text)
    updateDiscoveryModeLabel();
}

// System tray handlers
void NFSShareManagerApp::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if (isVisible()) {
            hideToTray();
        } else {
            showMainWindow();
        }
        break;
    case QSystemTrayIcon::MiddleClick:
        refreshAll();
        break;
    default:
        break;
    }
}

void NFSShareManagerApp::onTrayIconMessageClicked()
{
    showMainWindow();
}

// UI action handlers
void NFSShareManagerApp::onCreateShareClicked()
{
    ShareCreateDialog dialog(m_shareManager, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString path = dialog.getSharePath();
        ShareConfiguration config = dialog.getShareConfiguration();
        PermissionSet permissions = dialog.getPermissions();
        
        // Start progress indication
        QUuid operationId = m_operationManager->startOperation(
            tr("Creating NFS Share"),
            tr("Creating share for directory: %1").arg(path),
            true, // cancellable
            [this]() {
                // Cancellation callback - could interrupt share creation if supported
                qDebug() << "Share creation cancelled by user";
            }
        );
        
        // Update progress
        m_operationManager->setStatus(operationId, tr("Validating directory permissions..."));
        
        // Simulate progress steps (in real implementation, these would be actual steps)
        QTimer::singleShot(500, this, [this, operationId, path, config, permissions]() {
            m_operationManager->updateProgress(operationId, 25, tr("Generating export configuration..."));
            
            QTimer::singleShot(500, this, [this, operationId, path, config, permissions]() {
                m_operationManager->updateProgress(operationId, 50, tr("Updating system exports file..."));
                
                QTimer::singleShot(500, this, [this, operationId, path, config, permissions]() {
                    m_operationManager->updateProgress(operationId, 75, tr("Restarting NFS service..."));
                    
                    QTimer::singleShot(500, this, [this, operationId, path, config, permissions]() {
                        // Perform actual share creation
                        if (m_shareManager->createShare(path, config)) {
                            // Update permissions if needed
                            if (permissions.isValid()) {
                                m_shareManager->updateSharePermissions(path, permissions);
                            }
                            
                            m_operationManager->completeOperation(operationId, 
                                tr("Share created successfully: %1").arg(path));
                            updateLocalSharesList();
                        } else {
                            m_operationManager->failOperation(operationId,
                                tr("Failed to create share for directory: %1\n\n"
                                   "Please check the directory permissions and try again.").arg(path));
                        }
                    });
                });
            });
        });
    }
}

void NFSShareManagerApp::onRemoveShareClicked()
{
    if (!m_localSharesList || !m_shareManager) {
        return;
    }
    
    QListWidgetItem *currentItem = m_localSharesList->currentItem();
    if (!currentItem) {
        return;
    }
    
    QString sharePath = currentItem->data(Qt::UserRole).toString();
    if (sharePath.isEmpty()) {
        return;
    }
    
    // Get share details for confirmation
    NFSShare share = m_shareManager->getShare(sharePath);
    if (share.path().isEmpty()) {
        showErrorMessage(tr("Share Not Found"), 
                        tr("The selected share could not be found."));
        return;
    }
    
    // Confirm removal
    QString message = tr("Are you sure you want to remove this NFS share?\n\n"
                        "Path: %1\n"
                        "Name: %2\n\n"
                        "This will stop sharing the directory and remove it from the NFS exports. "
                        "The directory itself will not be deleted.")
                     .arg(share.path(), share.config().name());
    
    int result = QMessageBox::question(this, tr("Confirm Share Removal"), message,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        // Start progress indication
        QUuid operationId = m_operationManager->startOperation(
            tr("Removing NFS Share"),
            tr("Removing share: %1").arg(sharePath),
            true, // cancellable
            [this]() {
                qDebug() << "Share removal cancelled by user";
            }
        );
        
        // Update progress with steps
        m_operationManager->setStatus(operationId, tr("Stopping share export..."));
        
        QTimer::singleShot(300, this, [this, operationId, sharePath]() {
            m_operationManager->updateProgress(operationId, 33, tr("Updating exports configuration..."));
            
            QTimer::singleShot(300, this, [this, operationId, sharePath]() {
                m_operationManager->updateProgress(operationId, 66, tr("Reloading NFS service..."));
                
                QTimer::singleShot(300, this, [this, operationId, sharePath]() {
                    // Perform actual share removal
                    if (m_shareManager->removeShare(sharePath)) {
                        m_operationManager->completeOperation(operationId,
                            tr("Share removed successfully: %1").arg(sharePath));
                        updateLocalSharesList();
                    } else {
                        m_operationManager->failOperation(operationId,
                            tr("Failed to remove share: %1\n\n"
                               "The share may still be in use or you may not have sufficient permissions.")
                            .arg(sharePath));
                    }
                });
            });
        });
    }
}

void NFSShareManagerApp::onEditShareClicked()
{
    if (!m_localSharesList || !m_shareManager) {
        return;
    }
    
    QListWidgetItem *currentItem = m_localSharesList->currentItem();
    if (!currentItem) {
        return;
    }
    
    QString sharePath = currentItem->data(Qt::UserRole).toString();
    if (sharePath.isEmpty()) {
        return;
    }
    
    // Get the current share configuration
    NFSShare share = m_shareManager->getShare(sharePath);
    if (share.path().isEmpty()) {
        showErrorMessage(tr("Share Not Found"), 
                        tr("The selected share could not be found."));
        return;
    }
    
    // Open the configuration dialog
    ShareConfigDialog dialog(share, m_shareManager, this);
    
    if (dialog.exec() == QDialog::Accepted && dialog.isModified()) {
        ShareConfiguration newConfig = dialog.getShareConfiguration();
        PermissionSet newPermissions = dialog.getPermissions();
        
        showStatusMessage(tr("Updating share configuration for %1...").arg(sharePath));
        
        bool success = true;
        
        // Update configuration
        if (!m_shareManager->updateShareConfiguration(sharePath, newConfig)) {
            success = false;
        }
        
        // Update permissions
        if (success && !m_shareManager->updateSharePermissions(sharePath, newPermissions)) {
            success = false;
        }
        
        if (success) {
            showSuccessMessage(tr("Share configuration updated successfully: %1").arg(sharePath));
            updateLocalSharesList();
        } else {
            showErrorMessage(tr("Update Share Failed"), 
                           tr("Failed to update share configuration: %1\n\n"
                              "Please check your permissions and try again.")
                           .arg(sharePath));
        }
    }
}

void NFSShareManagerApp::onMountShareClicked()
{
    if (!m_remoteSharesList || !m_mountManager) {
        return;
    }
    
    QListWidgetItem *currentItem = m_remoteSharesList->currentItem();
    if (!currentItem) {
        return;
    }
    
    // Get the remote share from the item data
    QVariant shareData = currentItem->data(Qt::UserRole);
    if (!shareData.canConvert<RemoteNFSShare>()) {
        showErrorMessage(tr("Invalid Share"), 
                        tr("The selected share information is not valid."));
        return;
    }
    
    RemoteNFSShare remoteShare = shareData.value<RemoteNFSShare>();
    
    // Check if share is available
    if (!remoteShare.isAvailable()) {
        int result = QMessageBox::question(this, tr("Share Unavailable"),
                                         tr("The selected share appears to be unavailable.\n\n"
                                            "Do you want to try mounting it anyway?"),
                                         QMessageBox::Yes | QMessageBox::No,
                                         QMessageBox::No);
        if (result != QMessageBox::Yes) {
            return;
        }
    }
    
    // Open mount dialog
    MountDialog dialog(remoteShare, m_mountManager, this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Start progress indication for mount operation
        QString mountPoint = dialog.getMountPoint();
        MountOptions options = dialog.getMountOptions();
        bool isPersistent = dialog.isPersistent();
        
        QUuid operationId = m_operationManager->startOperation(
            tr("Mounting NFS Share"),
            tr("Mounting %1:%2 to %3").arg(remoteShare.hostName(), 
                                          remoteShare.exportPath(), 
                                          mountPoint),
            true, // cancellable
            [this, mountPoint]() {
                // Cancellation callback - could interrupt mount if supported
                qDebug() << "Mount operation cancelled by user for:" << mountPoint;
            }
        );
        
        // Simulate mount progress steps
        m_operationManager->setStatus(operationId, tr("Validating mount point..."));
        
        QTimer::singleShot(400, this, [this, operationId, remoteShare, mountPoint, options, isPersistent]() {
            m_operationManager->updateProgress(operationId, 25, tr("Testing network connectivity..."));
            
            QTimer::singleShot(400, this, [this, operationId, remoteShare, mountPoint, options, isPersistent]() {
                m_operationManager->updateProgress(operationId, 50, tr("Creating mount point directory..."));
                
                QTimer::singleShot(400, this, [this, operationId, remoteShare, mountPoint, options, isPersistent]() {
                    m_operationManager->updateProgress(operationId, 75, tr("Executing mount command..."));
                    
                    QTimer::singleShot(400, this, [this, operationId, remoteShare, mountPoint, options, isPersistent]() {
                        // Perform actual mount operation
                        MountManager::MountResult result = m_mountManager->mountShare(
                            remoteShare, mountPoint, options, isPersistent);
                        
                        if (result == MountManager::MountResult::Success) {
                            QString successMsg = tr("Successfully mounted %1:%2 to %3")
                                                .arg(remoteShare.hostName(), 
                                                     remoteShare.exportPath(), 
                                                     mountPoint);
                            m_operationManager->completeOperation(operationId, successMsg);
                            updateMountedSharesList();
                        } else {
                            QString errorMsg = tr("Mount operation failed");
                            switch (result) {
                            case MountManager::MountResult::InvalidMountPoint:
                                errorMsg = tr("Invalid mount point: The specified directory is not suitable for mounting.");
                                break;
                            case MountManager::MountResult::InvalidRemoteShare:
                                errorMsg = tr("Invalid remote share: The share information is not valid.");
                                break;
                            case MountManager::MountResult::MountPointExists:
                                errorMsg = tr("Mount point in use: Something is already mounted at this location.");
                                break;
                            case MountManager::MountResult::PermissionDenied:
                                errorMsg = tr("Permission denied: You don't have sufficient privileges for this operation.");
                                break;
                            case MountManager::MountResult::NetworkError:
                                errorMsg = tr("Network error: Unable to connect to the remote server.");
                                break;
                            case MountManager::MountResult::NFSServiceError:
                                errorMsg = tr("NFS service error: The NFS service is not available or configured incorrectly.");
                                break;
                            case MountManager::MountResult::SystemError:
                                errorMsg = tr("System error: A system-level error occurred during mounting.");
                                break;
                            case MountManager::MountResult::Cancelled:
                                errorMsg = tr("Operation cancelled by user.");
                                break;
                            default:
                                break;
                            }
                            m_operationManager->failOperation(operationId, errorMsg);
                        }
                    });
                });
            });
        });
    }
}

void NFSShareManagerApp::onUnmountShareClicked()
{
    if (!m_mountedSharesList || !m_mountManager) {
        return;
    }
    
    QListWidgetItem *currentItem = m_mountedSharesList->currentItem();
    if (!currentItem) {
        return;
    }
    
    QString mountPoint = currentItem->data(Qt::UserRole).toString();
    if (mountPoint.isEmpty()) {
        return;
    }
    
    // Get mount details for confirmation
    NFSMount mount = m_mountManager->getMountByPath(mountPoint);
    if (mount.localMountPoint().isEmpty()) {
        showErrorMessage(tr("Mount Not Found"), 
                        tr("The selected mount could not be found."));
        return;
    }
    
    // Confirm unmount operation
    QString message = tr("Are you sure you want to unmount this NFS share?\n\n"
                        "Remote Share: %1:%2\n"
                        "Mount Point: %3\n"
                        "Mount Type: %4\n\n"
                        "Any open files or applications using this mount may be affected.")
                     .arg(mount.remoteShare().hostName())
                     .arg(mount.remoteShare().exportPath())
                     .arg(mount.localMountPoint())
                     .arg(mount.isPersistent() ? tr("Persistent") : tr("Temporary"));
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Confirm Unmount"));
    msgBox.setText(message);
    msgBox.setIcon(QMessageBox::Question);
    
    QPushButton *unmountButton = msgBox.addButton(tr("Unmount"), QMessageBox::AcceptRole);
    QPushButton *forceUnmountButton = msgBox.addButton(tr("Force Unmount"), QMessageBox::DestructiveRole);
    QPushButton *cancelButton = msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
    
    forceUnmountButton->setToolTip(tr("Force unmount even if the share is busy"));
    msgBox.setDefaultButton(cancelButton);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == cancelButton) {
        return;
    }
    
    bool forceUnmount = (msgBox.clickedButton() == forceUnmountButton);
    
    showStatusMessage(tr("Unmounting %1...").arg(mountPoint));
    
    if (m_mountManager->unmountShare(mountPoint, forceUnmount)) {
        showSuccessMessage(tr("Share unmounted successfully: %1").arg(mountPoint));
        updateMountedSharesList();
    } else {
        QString errorMsg = tr("Failed to unmount share: %1\n\n"
                             "The share may be busy or you may not have sufficient permissions.");
        
        if (!forceUnmount) {
            errorMsg += tr("\n\nTry using 'Force Unmount' if the share is busy.");
        }
        
        showErrorMessage(tr("Unmount Failed"), errorMsg.arg(mountPoint));
    }
}

void NFSShareManagerApp::onRefreshDiscoveryClicked()
{
    if (m_networkDiscovery) {
        // Start progress indication for network discovery
        QUuid operationId = m_operationManager->startOperation(
            tr("Network Discovery"),
            tr("Scanning network for available NFS shares"),
            true, // cancellable
            [this]() {
                // Cancellation callback
                if (m_networkDiscovery) {
                    m_networkDiscovery->stopDiscovery();
                    qDebug() << "Network discovery cancelled by user";
                }
            }
        );
        
        // Store operation ID for progress updates
        m_currentDiscoveryOperationId = operationId;
        
        showStatusMessage(tr("Refreshing network discovery..."));
        if (m_discoveryProgress) {
            m_discoveryProgress->setVisible(true);
        }
        m_networkDiscovery->refreshDiscovery();
    }
}

void NFSShareManagerApp::onDiscoveryModeClicked()
{
    if (!m_networkDiscovery) {
        return;
    }
    
    // Create a simple mode selection dialog
    QStringList modes;
    modes << tr("Quick Scan") << tr("Full Scan") << tr("Targeted Scan");
    
    bool ok;
    QString selectedMode = QInputDialog::getItem(this, tr("Discovery Mode"), 
                                               tr("Select network discovery mode:"), 
                                               modes, 0, false, &ok);
    
    if (ok && !selectedMode.isEmpty()) {
        NetworkDiscovery::ScanMode mode = NetworkDiscovery::ScanMode::Quick;
        
        if (selectedMode == tr("Full Scan")) {
            mode = NetworkDiscovery::ScanMode::Full;
        } else if (selectedMode == tr("Targeted Scan")) {
            mode = NetworkDiscovery::ScanMode::Targeted;
        }
        
        m_networkDiscovery->setScanMode(mode);
        updateDiscoveryModeLabel();
        showStatusMessage(tr("Discovery mode changed to: %1").arg(selectedMode));
    }
}

void NFSShareManagerApp::onManageTargetsClicked()
{
    if (!m_networkDiscovery) {
        return;
    }
    
    // Create a simple target management dialog
    QStringList currentTargets = m_networkDiscovery->getTargetHosts();
    
    bool ok;
    QString targetsText = currentTargets.join("\n");
    QString newTargetsText = QInputDialog::getMultiLineText(this, tr("Manage Target Hosts"),
                                                          tr("Enter target hosts (one per line):\n"
                                                             "Examples: 192.168.1.100, server.local"),
                                                          targetsText, &ok);
    
    if (ok) {
        QStringList newTargets = newTargetsText.split('\n', Qt::SkipEmptyParts);
        
        // Clear existing targets and add new ones
        m_networkDiscovery->clearTargetHosts();
        for (const QString &target : newTargets) {
            QString trimmedTarget = target.trimmed();
            if (!trimmedTarget.isEmpty()) {
                m_networkDiscovery->addTargetHost(trimmedTarget);
            }
        }
        
        showStatusMessage(tr("Target hosts updated: %1 hosts configured").arg(newTargets.size()));
    }
}

void NFSShareManagerApp::onAutoDiscoveryToggled(bool paused)
{
    if (!m_networkDiscovery) {
        return;
    }
    
    if (paused) {
        m_networkDiscovery->stopDiscovery();
        m_autoDiscoveryToggle->setText(tr("Resume Auto-Discovery"));
        m_autoDiscoveryToggle->setIcon(QIcon::fromTheme("media-playback-start"));
        m_autoDiscoveryToggle->setToolTip(tr("Resume automatic network discovery"));
        m_autoDiscoveryStatus->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(16, 16));
        m_autoDiscoveryStatus->setToolTip(tr("Automatic discovery is paused"));
        showStatusMessage(tr("Automatic discovery paused"));
    } else {
        m_networkDiscovery->startDiscovery();
        m_autoDiscoveryToggle->setText(tr("Pause Auto-Discovery"));
        m_autoDiscoveryToggle->setIcon(QIcon::fromTheme("media-playback-pause"));
        m_autoDiscoveryToggle->setToolTip(tr("Pause automatic network discovery"));
        m_autoDiscoveryStatus->setPixmap(QIcon::fromTheme("dialog-information").pixmap(16, 16));
        m_autoDiscoveryStatus->setToolTip(tr("Automatic discovery is active"));
        showStatusMessage(tr("Automatic discovery resumed"));
    }
}

void NFSShareManagerApp::onDiscoveryStarted(NetworkDiscovery::ScanMode mode)
{
    if (m_discoveryProgress) {
        m_discoveryProgress->setVisible(true);
        m_discoveryProgress->setRange(0, 0); // Indeterminate
    }
    
    QString modeStr;
    switch (mode) {
    case NetworkDiscovery::ScanMode::Quick:
        modeStr = tr("Quick");
        break;
    case NetworkDiscovery::ScanMode::Full:
        modeStr = tr("Full");
        break;
    case NetworkDiscovery::ScanMode::Targeted:
        modeStr = tr("Targeted");
        break;
    }
    
    if (m_discoveryModeLabel) {
        m_discoveryModeLabel->setText(tr("Mode: %1 (scanning...)").arg(modeStr));
    }
    
    showStatusMessage(tr("Starting %1 network scan...").arg(modeStr.toLower()));
}

void NFSShareManagerApp::onDiscoveryStatusChanged(NetworkDiscovery::DiscoveryStatus status)
{
    QString statusText;
    QString statusColor = "black";
    
    switch (status) {
    case NetworkDiscovery::DiscoveryStatus::Idle:
        statusText = tr("Discovery idle");
        statusColor = "gray";
        break;
    case NetworkDiscovery::DiscoveryStatus::Scanning:
        statusText = tr("Scanning network...");
        statusColor = "blue";
        break;
    case NetworkDiscovery::DiscoveryStatus::Completed:
        statusText = tr("Discovery completed");
        statusColor = "green";
        break;
    case NetworkDiscovery::DiscoveryStatus::Error:
        statusText = tr("Discovery error");
        statusColor = "red";
        break;
    }
    
    if (m_remoteSharesStatus) {
        m_remoteSharesStatus->setText(statusText);
        m_remoteSharesStatus->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }").arg(statusColor));
    }
}

void NFSShareManagerApp::onScanProgress(int current, int total, const QString &hostAddress)
{
    if (m_discoveryProgress) {
        if (total > 0) {
            m_discoveryProgress->setRange(0, total);
            m_discoveryProgress->setValue(current);
        }
    }
    
    // Update operation progress if we have one
    if (!m_currentDiscoveryOperationId.isNull() && m_operationManager->hasOperation(m_currentDiscoveryOperationId)) {
        int progressPercent = total > 0 ? (current * 100) / total : 0;
        m_operationManager->updateProgress(m_currentDiscoveryOperationId, progressPercent,
            tr("Scanning %1 (%2/%3)").arg(hostAddress).arg(current).arg(total));
    }
    
    showStatusMessage(tr("Scanning %1 (%2/%3)").arg(hostAddress).arg(current).arg(total), 1000);
}

QString NFSShareManagerApp::formatTimeAgo(const QDateTime &dateTime) const
{
    if (!dateTime.isValid()) {
        return tr("Unknown");
    }
    
    qint64 secondsAgo = dateTime.secsTo(QDateTime::currentDateTime());
    
    if (secondsAgo < 60) {
        return tr("Just now");
    } else if (secondsAgo < 3600) {
        int minutes = secondsAgo / 60;
        return tr("%1 minute(s) ago").arg(minutes);
    } else if (secondsAgo < 86400) {
        int hours = secondsAgo / 3600;
        return tr("%1 hour(s) ago").arg(hours);
    } else {
        int days = secondsAgo / 86400;
        return tr("%1 day(s) ago").arg(days);
    }
}

QString NFSShareManagerApp::formatRemoteShareTooltip(const RemoteNFSShare &share) const
{
    QString tooltip = tr("Remote NFS Share\n");
    tooltip += tr("Hostname: %1\n").arg(share.hostName());
    tooltip += tr("IP Address: %1\n").arg(share.hostAddress().toString());
    tooltip += tr("Export Path: %1\n").arg(share.exportPath());
    
    if (!share.description().isEmpty()) {
        tooltip += tr("Description: %1\n").arg(share.description());
    }
    
    if (share.supportedVersion() != NFSVersion::Unknown) {
        QString versionStr = NFSShareManager::nfsVersionToString(share.supportedVersion());
        tooltip += tr("NFS Version: %1\n").arg(versionStr);
    }
    
    tooltip += tr("Discovered: %1\n").arg(share.discoveredAt().toString());
    
    if (share.lastSeen().isValid()) {
        tooltip += tr("Last Seen: %1\n").arg(share.lastSeen().toString());
    }
    
    tooltip += tr("Available: %1").arg(share.isAvailable() ? tr("Yes") : tr("No"));
    
    if (!share.isAvailable()) {
        tooltip += tr("\n\nThis share is currently unavailable.\n"
                     "It may be offline or access may be restricted.");
    }
    
    return tooltip;
}

QListWidgetItem* NFSShareManagerApp::createRemoteShareItem(const RemoteNFSShare &share, bool available)
{
    QListWidgetItem *item = new QListWidgetItem();
    
    // Create main display text with hostname and export path
    QString mainText = tr("%1:%2").arg(share.hostName(), share.exportPath());
    
    // Create detailed subtitle with IP address and additional info
    QString subText = tr("IP: %1").arg(share.hostAddress().toString());
    
    // Add NFS version if available
    if (share.supportedVersion() != NFSVersion::Unknown) {
        QString versionStr = NFSShareManager::nfsVersionToString(share.supportedVersion());
        subText += tr(" | NFS: %1").arg(versionStr);
    }
    
    // Add discovery time
    QString timeAgo = formatTimeAgo(share.discoveredAt());
    subText += tr(" | Discovered: %1").arg(timeAgo);
    
    // Add last seen time if different from discovery time
    if (share.lastSeen().isValid() && share.lastSeen() != share.discoveredAt()) {
        QString lastSeenAgo = formatTimeAgo(share.lastSeen());
        subText += tr(" | Last seen: %1").arg(lastSeenAgo);
    }
    
    // Combine main text and subtext
    QString fullText = mainText + "\n" + subText;
    item->setText(fullText);
    
    // Set icon based on availability and share type
    QIcon icon;
    if (available) {
        icon = QIcon::fromTheme("network-server", QIcon::fromTheme("folder-remote"));
    } else {
        icon = QIcon::fromTheme("network-server-offline", QIcon::fromTheme("dialog-error"));
    }
    item->setIcon(icon);
    
    // Create detailed tooltip
    QString tooltip = formatRemoteShareTooltip(share);
    item->setToolTip(tooltip);
    
    // Store share data for actions
    item->setData(Qt::UserRole, QVariant::fromValue(share));
    
    // Set visual styling based on availability
    if (!available) {
        item->setForeground(QColor(Qt::gray));
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled); // Disable unavailable shares
    }
    
    return item;
}

void NFSShareManagerApp::updateDiscoveryStatistics()
{
    if (!m_networkDiscovery || !m_discoveryStatsLabel) {
        return;
    }
    
    QHash<QString, QVariant> stats = m_networkDiscovery->getScanStatistics();
    int hostsScanned = m_networkDiscovery->lastScanHostCount();
    int sharesFound = m_networkDiscovery->getDiscoveredShares().size();
    
    QString statsText = tr("Hosts scanned: %1 | Shares found: %2").arg(hostsScanned).arg(sharesFound);
    
    // Add total statistics if available
    if (stats.contains("total_scans") && stats["total_scans"].toInt() > 0) {
        statsText += tr(" | Total scans: %1").arg(stats["total_scans"].toInt());
    }
    
    m_discoveryStatsLabel->setText(statsText);
    
    // Update last scan time
    if (m_lastScanTimeLabel) {
        QDateTime lastScan = m_networkDiscovery->lastScanTime();
        if (lastScan.isValid()) {
            QString timeAgo = formatTimeAgo(lastScan);
            m_lastScanTimeLabel->setText(tr("Last scan: %1").arg(timeAgo));
        } else {
            m_lastScanTimeLabel->setText(tr("Last scan: Never"));
        }
    }
}

void NFSShareManagerApp::updateDiscoveryModeLabel()
{
    if (!m_networkDiscovery || !m_discoveryModeLabel) {
        return;
    }
    
    QString modeStr;
    switch (m_networkDiscovery->scanMode()) {
    case NetworkDiscovery::ScanMode::Quick:
        modeStr = tr("Quick");
        break;
    case NetworkDiscovery::ScanMode::Full:
        modeStr = tr("Full");
        break;
    case NetworkDiscovery::ScanMode::Targeted:
        modeStr = tr("Targeted");
        break;
    }
    
    m_discoveryModeLabel->setText(tr("Mode: %1").arg(modeStr));
}

void NFSShareManagerApp::updateLocalSharesList()
{
    if (!m_localSharesList || !m_shareManager) {
        return;
    }
    
    m_localSharesList->clear();
    
    QList<NFSShare> shares = m_shareManager->getActiveShares();
    
    if (shares.isEmpty()) {
        if (m_localSharesStatus) {
            m_localSharesStatus->setText(tr("No local shares configured"));
        }
        return;
    }
    
    for (const NFSShare &share : shares) {
        QListWidgetItem *item = new QListWidgetItem();
        
        // Create main display text
        QString mainText = share.config().name().isEmpty() ? 
                          QFileInfo(share.path()).fileName() : 
                          share.config().name();
        
        // Create subtitle with path and status
        QString subText = tr("Path: %1").arg(share.path());
        
        // Add access mode
        QString accessMode = (share.config().accessMode() == AccessMode::ReadOnly) ? 
                            tr("Read-Only") : tr("Read-Write");
        subText += tr(" | Access: %1").arg(accessMode);
        
        // Add status
        QString status = share.isActive() ? tr("Active") : tr("Inactive");
        subText += tr(" | Status: %1").arg(status);
        
        // Combine text
        QString fullText = mainText + "\n" + subText;
        item->setText(fullText);
        
        // Set icon based on status
        QIcon icon = getShareStatusIcon(share);
        item->setIcon(icon);
        
        // Create tooltip
        QString tooltip = formatShareStatus(share);
        item->setToolTip(tooltip);
        
        // Store share path for actions
        item->setData(Qt::UserRole, share.path());
        
        // Set visual styling based on status
        if (!share.isActive()) {
            item->setForeground(QColor(Qt::gray));
        }
        
        if (!share.errorMessage().isEmpty()) {
            item->setForeground(QColor(Qt::red));
        }
        
        m_localSharesList->addItem(item);
    }
    
    if (m_localSharesStatus) {
        int activeCount = 0;
        for (const NFSShare &share : shares) {
            if (share.isActive()) {
                activeCount++;
            }
        }
        
        m_localSharesStatus->setText(tr("%1 share(s) configured, %2 active")
                                    .arg(shares.size()).arg(activeCount));
    }
}

void NFSShareManagerApp::updateRemoteSharesList()
{
    if (!m_remoteSharesList || !m_networkDiscovery) {
        return;
    }
    
    m_remoteSharesList->clear();
    
    QList<RemoteNFSShare> shares = m_networkDiscovery->getDiscoveredShares();
    
    if (shares.isEmpty()) {
        if (m_remoteSharesStatus) {
            m_remoteSharesStatus->setText(tr("No remote shares discovered"));
        }
        return;
    }
    
    // Sort shares by hostname and export path
    std::sort(shares.begin(), shares.end(), [](const RemoteNFSShare &a, const RemoteNFSShare &b) {
        if (a.hostName() != b.hostName()) {
            return a.hostName() < b.hostName();
        }
        return a.exportPath() < b.exportPath();
    });
    
    for (const RemoteNFSShare &share : shares) {
        QListWidgetItem *item = createRemoteShareItem(share, share.isAvailable());
        m_remoteSharesList->addItem(item);
    }
    
    if (m_remoteSharesStatus) {
        int availableCount = 0;
        for (const RemoteNFSShare &share : shares) {
            if (share.isAvailable()) {
                availableCount++;
            }
        }
        
        m_remoteSharesStatus->setText(tr("%1 share(s) discovered, %2 available")
                                     .arg(shares.size()).arg(availableCount));
    }
}

void NFSShareManagerApp::updateMountedSharesList()
{
    if (!m_mountedSharesList || !m_mountManager) {
        return;
    }
    
    m_mountedSharesList->clear();
    
    QList<NFSMount> mounts = m_mountManager->getManagedMounts();
    
    if (mounts.isEmpty()) {
        if (m_mountedSharesStatus) {
            m_mountedSharesStatus->setText(tr("No NFS shares currently mounted"));
        }
        return;
    }
    
    // Sort mounts by mount point
    std::sort(mounts.begin(), mounts.end(), [](const NFSMount &a, const NFSMount &b) {
        return a.localMountPoint() < b.localMountPoint();
    });
    
    for (const NFSMount &mount : mounts) {
        QListWidgetItem *item = new QListWidgetItem();
        
        // Create main display text with remote share info
        QString mainText = tr("%1:%2").arg(mount.remoteShare().hostName(), 
                                          mount.remoteShare().exportPath());
        
        // Create subtitle with mount point and details
        QString subText = tr("Mounted at: %1").arg(mount.localMountPoint());
        
        // Add mount type
        QString mountType = mount.isPersistent() ? tr("Persistent") : tr("Temporary");
        subText += tr(" | Type: %1").arg(mountType);
        
        // Add NFS version
        QString versionStr = nfsVersionToString(mount.options().nfsVersion);
        subText += tr(" | NFS: %1").arg(versionStr);
        
        // Add access mode
        QString accessMode = mount.options().readOnly ? tr("Read-Only") : tr("Read-Write");
        subText += tr(" | Access: %1").arg(accessMode);
        
        // Add status
        QString status = mountStatusToString(mount.status());
        subText += tr(" | Status: %1").arg(status);
        
        // Combine text
        QString fullText = mainText + "\n" + subText;
        item->setText(fullText);
        
        // Set icon based on status
        QIcon icon = getMountStatusIcon(mount);
        item->setIcon(icon);
        
        // Create detailed tooltip
        QString tooltip = formatMountStatus(mount);
        item->setToolTip(tooltip);
        
        // Store mount point for actions
        item->setData(Qt::UserRole, mount.localMountPoint());
        
        // Set visual styling based on status
        if (mount.status() != MountStatus::Mounted) {
            item->setForeground(QColor(Qt::gray));
        }
        
        if (mount.hasError()) {
            item->setForeground(QColor(Qt::red));
        }
        
        m_mountedSharesList->addItem(item);
    }
    
    if (m_mountedSharesStatus) {
        int activeCount = 0;
        int persistentCount = 0;
        for (const NFSMount &mount : mounts) {
            if (mount.status() == MountStatus::Mounted) {
                activeCount++;
            }
            if (mount.isPersistent()) {
                persistentCount++;
            }
        }
        
        m_mountedSharesStatus->setText(tr("%1 mount(s) total, %2 active, %3 persistent")
                                      .arg(mounts.size()).arg(activeCount).arg(persistentCount));
    }
}

QString NFSShareManagerApp::formatShareStatus(const NFSShare &share) const
{
    QString tooltip = tr("Local NFS Share\n");
    tooltip += tr("Name: %1\n").arg(share.config().name());
    tooltip += tr("Path: %1\n").arg(share.path());
    tooltip += tr("Export Path: %1\n").arg(share.exportPath());
    
    QString accessMode = (share.config().accessMode() == AccessMode::ReadOnly) ? 
                        tr("Read-Only") : tr("Read-Write");
    tooltip += tr("Access Mode: %1\n").arg(accessMode);
    
    tooltip += tr("Status: %1\n").arg(share.isActive() ? tr("Active") : tr("Inactive"));
    tooltip += tr("Created: %1\n").arg(share.createdAt().toString());
    
    if (!share.errorMessage().isEmpty()) {
        tooltip += tr("Error: %1\n").arg(share.errorMessage());
    }
    
    // Add permission details
    if (share.permissions().isValid()) {
        tooltip += tr("\nPermissions:\n");
        tooltip += tr("Default Access: %1\n").arg(accessModeToString(share.permissions().defaultAccess()));
        
        if (share.permissions().enableRootSquash()) {
            tooltip += tr("Root Squash: Enabled\n");
        }
        
        if (!share.permissions().anonymousUser().isEmpty()) {
            tooltip += tr("Anonymous User: %1\n").arg(share.permissions().anonymousUser());
        }
    }
    
    return tooltip;
}

QString NFSShareManagerApp::formatMountStatus(const NFSMount &mount) const
{
    QString tooltip = tr("NFS Mount\n");
    tooltip += tr("Remote Share: %1:%2\n").arg(mount.remoteShare().hostName(), 
                                              mount.remoteShare().exportPath());
    tooltip += tr("Mount Point: %1\n").arg(mount.localMountPoint());
    tooltip += tr("Status: %1\n").arg(mountStatusToString(mount.status()));
    
    QString mountType = mount.isPersistent() ? tr("Persistent") : tr("Temporary");
    tooltip += tr("Mount Type: %1\n").arg(mountType);
    
    // Add mount options
    tooltip += tr("\nMount Options:\n");
    tooltip += tr("NFS Version: %1\n").arg(nfsVersionToString(mount.options().nfsVersion));
    tooltip += tr("Access Mode: %1\n").arg(mount.options().readOnly ? tr("Read-Only") : tr("Read-Write"));
    tooltip += tr("Timeout: %1 seconds\n").arg(mount.options().timeoutSeconds);
    tooltip += tr("Retry Count: %1\n").arg(mount.options().retryCount);
    
    if (mount.options().softMount) {
        tooltip += tr("Soft Mount: Yes\n");
    }
    
    if (!mount.options().securityFlavor.isEmpty()) {
        tooltip += tr("Security: %1\n").arg(mount.options().securityFlavor);
    }
    
    // Add timestamps
    if (mount.mountedAt().isValid()) {
        tooltip += tr("Mounted: %1\n").arg(mount.mountedAt().toString());
    }
    
    if (mount.hasError()) {
        tooltip += tr("Error: %1\n").arg(mount.errorMessage());
    }
    
    return tooltip;
}

// Status message methods implementation
void NFSShareManagerApp::showStatusMessage(const QString &message, int timeout)
{
    if (statusBar()) {
        statusBar()->showMessage(message, timeout);
    }
    
    m_lastStatusMessage = message;
    
    // Also show in system tray if available
    if (m_systemTrayAvailable && m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->setToolTip(tr("NFS Share Manager - %1").arg(message));
    }
    
    qDebug() << "Status:" << message;
}

void NFSShareManagerApp::showErrorMessage(const QString &title, const QString &message)
{
    // Use notification manager for comprehensive error notifications
    if (m_notificationManager) {
        m_notificationManager->showError(title, message);
    } else {
        // Fallback to message box
        QMessageBox::warning(this, title, message);
    }
    
    // Also show in status bar
    showStatusMessage(tr("Error: %1").arg(title), 10000);
    
    qWarning() << "Error:" << title << "-" << message;
}

void NFSShareManagerApp::showSuccessMessage(const QString &message)
{
    // Use notification manager for success notifications
    if (m_notificationManager) {
        m_notificationManager->showSuccess(message);
    }
    
    showStatusMessage(message, 3000);
    
    qDebug() << "Success:" << message;
}

void NFSShareManagerApp::onStatusUpdateTimer()
{
    // Update system tray tooltip with current status
    updateSystemTrayTooltip();
    
    // Update status indicators
    updateStatusBar();
    
    // Update discovery statistics
    updateDiscoveryStatistics();
    
    // Check component health
    checkComponentHealth();
}

void NFSShareManagerApp::checkComponentHealth()
{
    // Check if any components have become unhealthy
    QStringList unhealthyComponents;
    
    if (m_shareManager && !m_shareManager->isHealthy()) {
        unhealthyComponents << "ShareManager";
    }
    
    if (m_mountManager && !m_mountManager->isHealthy()) {
        unhealthyComponents << "MountManager";
    }
    
    if (m_networkDiscovery && !m_networkDiscovery->isHealthy()) {
        unhealthyComponents << "NetworkDiscovery";
    }
    
    if (m_permissionManager && !m_permissionManager->isHealthy()) {
        unhealthyComponents << "PermissionManager";
    }
    
    if (m_configurationManager && !m_configurationManager->isHealthy()) {
        unhealthyComponents << "ConfigurationManager";
    }
    
    if (m_notificationManager && !m_notificationManager->isHealthy()) {
        unhealthyComponents << "NotificationManager";
    }
    
    if (m_operationManager && !m_operationManager->isHealthy()) {
        unhealthyComponents << "OperationManager";
    }
    
    // Handle unhealthy components
    for (const QString &component : unhealthyComponents) {
        qWarning() << "Component health check failed:" << component;
        
        // Try to recover the component
        if (component == "ShareManager" && m_shareManager) {
            if (m_shareManager->recover()) {
                qDebug() << "ShareManager recovered successfully";
            } else {
                handleComponentFailure(component, tr("Component became unhealthy and could not be recovered"));
            }
        } else if (component == "MountManager" && m_mountManager) {
            if (m_mountManager->recover()) {
                qDebug() << "MountManager recovered successfully";
            } else {
                handleComponentFailure(component, tr("Component became unhealthy and could not be recovered"));
            }
        } else if (component == "NetworkDiscovery" && m_networkDiscovery) {
            if (m_networkDiscovery->recover()) {
                qDebug() << "NetworkDiscovery recovered successfully";
            } else {
                handleComponentFailure(component, tr("Component became unhealthy and could not be recovered"));
            }
        }
        // Add recovery for other components as needed
    }
}

void NFSShareManagerApp::updateStatusBar()
{
    if (!statusBar()) {
        return;
    }
    
    // If no recent status message, show summary
    if (m_lastStatusMessage.isEmpty() || 
        statusBar()->currentMessage().isEmpty()) {
        
        QString summary;
        
        // Count active shares
        int activeShares = 0;
        if (m_shareManager) {
            QList<NFSShare> shares = m_shareManager->getActiveShares();
            for (const NFSShare &share : shares) {
                if (share.isActive()) {
                    activeShares++;
                }
            }
        }
        
        // Count active mounts
        int activeMounts = 0;
        if (m_mountManager) {
            QList<NFSMount> mounts = m_mountManager->getManagedMounts();
            for (const NFSMount &mount : mounts) {
                if (mount.status() == MountStatus::Mounted) {
                    activeMounts++;
                }
            }
        }
        
        // Count discovered shares
        int discoveredShares = 0;
        if (m_networkDiscovery) {
            discoveredShares = m_networkDiscovery->getDiscoveredShares().size();
        }
        
        summary = tr("Shares: %1 active | Mounts: %2 active | Discovered: %3")
                 .arg(activeShares).arg(activeMounts).arg(discoveredShares);
        
        statusBar()->showMessage(summary);
    }
}

void NFSShareManagerApp::updateSystemTrayTooltip()
{
    if (!m_systemTrayAvailable || !m_trayIcon) {
        return;
    }
    
    QString tooltip = tr("NFS Share Manager");
    
    // Add current status
    if (!m_lastStatusMessage.isEmpty()) {
        tooltip += tr("\nStatus: %1").arg(m_lastStatusMessage);
    }
    
    // Add summary information
    if (m_shareManager) {
        QList<NFSShare> shares = m_shareManager->getActiveShares();
        int activeShares = 0;
        for (const NFSShare &share : shares) {
            if (share.isActive()) {
                activeShares++;
            }
        }
        tooltip += tr("\nLocal Shares: %1 active").arg(activeShares);
    }
    
    if (m_mountManager) {
        QList<NFSMount> mounts = m_mountManager->getManagedMounts();
        int activeMounts = 0;
        for (const NFSMount &mount : mounts) {
            if (mount.status() == MountStatus::Mounted) {
                activeMounts++;
            }
        }
        tooltip += tr("\nMounted Shares: %1 active").arg(activeMounts);
    }
    
    if (m_networkDiscovery) {
        int discoveredShares = m_networkDiscovery->getDiscoveredShares().size();
        tooltip += tr("\nDiscovered Shares: %1").arg(discoveredShares);
        
        if (m_networkDiscovery->isDiscoveryActive()) {
            tooltip += tr(" (scanning...)");
        }
    }
    
    m_trayIcon->setToolTip(tooltip);
}

QIcon NFSShareManagerApp::getShareStatusIcon(const NFSShare &share) const
{
    if (!share.errorMessage().isEmpty()) {
        return QIcon::fromTheme("dialog-error", style()->standardIcon(QStyle::SP_MessageBoxCritical));
    } else if (share.isActive()) {
        return QIcon::fromTheme("folder-network", style()->standardIcon(QStyle::SP_DirIcon));
    } else {
        return QIcon::fromTheme("folder-grey", style()->standardIcon(QStyle::SP_DirIcon));
    }
}

QIcon NFSShareManagerApp::getMountStatusIcon(const NFSMount &mount) const
{
    switch (mount.status()) {
    case MountStatus::Mounted:
        return QIcon::fromTheme("drive-harddisk-mounted", style()->standardIcon(QStyle::SP_DriveHDIcon));
    case MountStatus::Unmounted:
        return QIcon::fromTheme("drive-harddisk-unmounted", style()->standardIcon(QStyle::SP_DriveHDIcon));
    case MountStatus::Error:
        return QIcon::fromTheme("dialog-error", style()->standardIcon(QStyle::SP_MessageBoxCritical));
    case MountStatus::Mounting:
        return QIcon::fromTheme("view-refresh", style()->standardIcon(QStyle::SP_BrowserReload));
    case MountStatus::Unmounting:
        return QIcon::fromTheme("view-refresh", style()->standardIcon(QStyle::SP_BrowserReload));
    default:
        return QIcon::fromTheme("drive-harddisk", style()->standardIcon(QStyle::SP_DriveHDIcon));
    }
}

QString NFSShareManagerApp::formatMountStatus(const NFSMount &mount) const
{
    QString tooltip = tr("NFS Mount\n");
    tooltip += tr("Remote Share: %1:%2\n").arg(mount.remoteShare().hostName(), 
                                              mount.remoteShare().exportPath());
    tooltip += tr("Mount Point: %1\n").arg(mount.localMountPoint());
    tooltip += tr("Status: %1\n").arg(mountStatusToString(mount.status()));
    
    QString mountType = mount.isPersistent() ? tr("Persistent") : tr("Temporary");
    tooltip += tr("Mount Type: %1\n").arg(mountType);
    
    // Add mount options
    tooltip += tr("\nMount Options:\n");
    tooltip += tr("NFS Version: %1\n").arg(nfsVersionToString(mount.options().nfsVersion));
    tooltip += tr("Access Mode: %1\n").arg(mount.options().readOnly ? tr("Read-Only") : tr("Read-Write"));
    tooltip += tr("Timeout: %1 seconds\n").arg(mount.options().timeoutSeconds);
    tooltip += tr("Retry Count: %1\n").arg(mount.options().retryCount);
    
    if (mount.options().softMount) {
        tooltip += tr("Soft Mount: Yes\n");
    }
    
    if (!mount.options().securityFlavor.isEmpty()) {
        tooltip += tr("Security: %1\n").arg(mount.options().securityFlavor);
    }
    
    // Add timestamps
    if (mount.mountedAt().isValid()) {
        tooltip += tr("Mounted: %1\n").arg(mount.mountedAt().toString());
    }
    
    if (mount.lastActivity().isValid()) {
        tooltip += tr("Last Activity: %1\n").arg(mount.lastActivity().toString());
    }
    
    if (mount.hasError()) {
        tooltip += tr("\nError: %1").arg(mount.errorMessage());
    }
    
    return tooltip;
}

QIcon NFSShareManagerApp::getShareStatusIcon(const NFSShare &share) const
{
    if (!share.errorMessage().isEmpty()) {
        return QIcon::fromTheme("dialog-error", style()->standardIcon(QStyle::SP_MessageBoxCritical));
    } else if (share.isActive()) {
        return QIcon::fromTheme("folder-open", style()->standardIcon(QStyle::SP_DirOpenIcon));
    } else {
        return QIcon::fromTheme("folder", style()->standardIcon(QStyle::SP_DirClosedIcon));
    }
}

QIcon NFSShareManagerApp::getMountStatusIcon(const NFSMount &mount) const
{
    switch (mount.status()) {
    case MountStatus::Mounted:
        if (mount.hasError()) {
            return QIcon::fromTheme("dialog-warning", style()->standardIcon(QStyle::SP_MessageBoxWarning));
        } else {
            return QIcon::fromTheme("drive-harddisk-mounted", style()->standardIcon(QStyle::SP_DriveHDIcon));
        }
    case MountStatus::Mounting:
    case MountStatus::Unmounting:
        return QIcon::fromTheme("view-refresh", style()->standardIcon(QStyle::SP_BrowserReload));
    case MountStatus::Failed:
        return QIcon::fromTheme("dialog-error", style()->standardIcon(QStyle::SP_MessageBoxCritical));
    case MountStatus::NotMounted:
    default:
        return QIcon::fromTheme("drive-harddisk-unmounted", style()->standardIcon(QStyle::SP_DriveHDIcon));
    }
}

// Operation manager signal handlers
void NFSShareManagerApp::onOperationStarted(const QUuid &operationId, const QString &title)
{
    qDebug() << "Operation started:" << operationId << title;
    
    // Show global progress indication
    if (m_globalProgressBar && m_globalProgressLabel && m_cancelOperationsButton) {
        m_globalProgressBar->setVisible(true);
        m_globalProgressBar->setRange(0, 0); // Indeterminate initially
        m_globalProgressLabel->setText(title);
        m_globalProgressLabel->setVisible(true);
        m_cancelOperationsButton->setVisible(true);
    }
    
    // Update system tray tooltip
    updateSystemTrayTooltip();
}

void NFSShareManagerApp::onOperationProgressUpdated(const QUuid &operationId, int progress, const QString &statusMessage)
{
    // Update global progress bar
    if (m_globalProgressBar && m_globalProgressLabel) {
        if (m_globalProgressBar->maximum() == 0) {
            // Switch from indeterminate to determinate
            m_globalProgressBar->setRange(0, 100);
        }
        m_globalProgressBar->setValue(progress);
        m_globalProgressLabel->setText(statusMessage);
    }
    
    // Update status bar message
    showStatusMessage(statusMessage, 2000);
}

void NFSShareManagerApp::onOperationCompleted(const QUuid &operationId, const QString &message)
{
    qDebug() << "Operation completed:" << operationId << message;
    
    // Hide global progress indication after a short delay
    QTimer::singleShot(2000, this, [this]() {
        if (m_operationManager && m_operationManager->getActiveOperations().isEmpty()) {
            if (m_globalProgressBar && m_globalProgressLabel && m_cancelOperationsButton) {
                m_globalProgressBar->setVisible(false);
                m_globalProgressLabel->setVisible(false);
                m_cancelOperationsButton->setVisible(false);
            }
        }
    });
    
    // Show success message
    showSuccessMessage(message);
    
    // Update system tray tooltip
    updateSystemTrayTooltip();
}

void NFSShareManagerApp::onOperationFailed(const QUuid &operationId, const QString &errorMessage)
{
    qWarning() << "Operation failed:" << operationId << errorMessage;
    
    // Hide global progress indication
    if (m_operationManager && m_operationManager->getActiveOperations().isEmpty()) {
        if (m_globalProgressBar && m_globalProgressLabel && m_cancelOperationsButton) {
            m_globalProgressBar->setVisible(false);
            m_globalProgressLabel->setVisible(false);
            m_cancelOperationsButton->setVisible(false);
        }
    }
    
    // Show error message
    showErrorMessage(tr("Operation Failed"), errorMessage);
    
    // Update system tray tooltip
    updateSystemTrayTooltip();
}

void NFSShareManagerApp::onOperationCancelled(const QUuid &operationId)
{
    qDebug() << "Operation cancelled:" << operationId;
    
    // Hide global progress indication
    if (m_operationManager && m_operationManager->getActiveOperations().isEmpty()) {
        if (m_globalProgressBar && m_globalProgressLabel && m_cancelOperationsButton) {
            m_globalProgressBar->setVisible(false);
            m_globalProgressLabel->setVisible(false);
            m_cancelOperationsButton->setVisible(false);
        }
    }
    
    // Show cancellation message
    showStatusMessage(tr("Operation cancelled"), 3000);
    
    // Update system tray tooltip
    updateSystemTrayTooltip();
}

} // namespace NFSShareManager