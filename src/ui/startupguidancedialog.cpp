#include "startupguidancedialog.h"

#include <QApplication>
#include <QClipboard>
#include <QScrollArea>
#include <QSplitter>
#include <QMessageBox>
#include <QStyle>
#include <QIcon>
#include <QFont>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

namespace NFSShareManager {

StartupGuidanceDialog::StartupGuidanceDialog(const StartupValidationResult &result, 
                                           StartupManager *startupManager,
                                           QWidget *parent)
    : QDialog(parent)
    , m_validationResult(result)
    , m_startupManager(startupManager)
    , m_result(Result::Exit)
    , m_tabWidget(nullptr)
    , m_showingDetails(false)
    , m_operationInProgress(false)
    , m_progressTimer(new QTimer(this))
{
    setupUI();
    updateValidationResult(result);
    
    // Connect startup manager signals
    if (m_startupManager) {
        connect(m_startupManager, &StartupManager::dependencyInstallationNeeded,
                this, &StartupGuidanceDialog::onDependencyInstallationStarted);
        connect(m_startupManager, &StartupManager::serviceStartupNeeded,
                this, &StartupGuidanceDialog::onServiceStartupStarted);
    }
    
    // Setup progress timer
    m_progressTimer->setInterval(100);
    connect(m_progressTimer, &QTimer::timeout, this, &StartupGuidanceDialog::updateProgress);
}

StartupGuidanceDialog::~StartupGuidanceDialog()
{
}

void StartupGuidanceDialog::setupUI()
{
    setWindowTitle(tr("NFS Share Manager - Startup Guidance"));
    setWindowIcon(QIcon::fromTheme("network-server"));
    setModal(true);
    resize(800, 600);
    setMinimumSize(600, 400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    setupOverviewTab();
    setupDependenciesTab();
    setupServicesTab();
    setupSystemInfoTab();
    
    // Progress bar (initially hidden)
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    m_progressLabel = new QLabel();
    m_progressLabel->setVisible(false);
    mainLayout->addWidget(m_progressLabel);
    
    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_detailsButton = new QPushButton(tr("Show Details"));
    m_detailsButton->setCheckable(true);
    connect(m_detailsButton, &QPushButton::toggled, this, &StartupGuidanceDialog::onShowDetailsToggled);
    buttonLayout->addWidget(m_detailsButton);
    
    buttonLayout->addStretch();
    
    m_installDepsButton = new QPushButton(QIcon::fromTheme("system-software-install"), tr("Install Dependencies"));
    connect(m_installDepsButton, &QPushButton::clicked, this, &StartupGuidanceDialog::onInstallDependenciesClicked);
    buttonLayout->addWidget(m_installDepsButton);
    
    m_startServicesButton = new QPushButton(QIcon::fromTheme("system-run"), tr("Start Services"));
    connect(m_startServicesButton, &QPushButton::clicked, this, &StartupGuidanceDialog::onStartServicesClicked);
    buttonLayout->addWidget(m_startServicesButton);
    
    m_tryAgainButton = new QPushButton(QIcon::fromTheme("view-refresh"), tr("Try Again"));
    connect(m_tryAgainButton, &QPushButton::clicked, this, &StartupGuidanceDialog::onTryAgainClicked);
    buttonLayout->addWidget(m_tryAgainButton);
    
    m_continueButton = new QPushButton(QIcon::fromTheme("dialog-ok"), tr("Continue Anyway"));
    connect(m_continueButton, &QPushButton::clicked, this, &StartupGuidanceDialog::onContinueAnywayClicked);
    buttonLayout->addWidget(m_continueButton);
    
    m_exitButton = new QPushButton(QIcon::fromTheme("application-exit"), tr("Exit"));
    connect(m_exitButton, &QPushButton::clicked, this, &StartupGuidanceDialog::onExitClicked);
    buttonLayout->addWidget(m_exitButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Initially show only overview tab
    onShowDetailsToggled(false);
}

void StartupGuidanceDialog::setupOverviewTab()
{
    m_overviewTab = new QWidget();
    m_tabWidget->addTab(m_overviewTab, QIcon::fromTheme("dialog-information"), tr("Overview"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_overviewTab);
    
    // Status section
    QHBoxLayout *statusLayout = new QHBoxLayout();
    
    m_statusIcon = new QLabel();
    m_statusIcon->setFixedSize(48, 48);
    m_statusIcon->setScaledContents(true);
    statusLayout->addWidget(m_statusIcon);
    
    QVBoxLayout *statusTextLayout = new QVBoxLayout();
    
    m_statusText = new QLabel();
    QFont statusFont = m_statusText->font();
    statusFont.setPointSize(statusFont.pointSize() + 2);
    statusFont.setBold(true);
    m_statusText->setFont(statusFont);
    statusTextLayout->addWidget(m_statusText);
    
    m_summaryText = new QLabel();
    m_summaryText->setWordWrap(true);
    statusTextLayout->addWidget(m_summaryText);
    
    statusLayout->addLayout(statusTextLayout);
    statusLayout->addStretch();
    
    layout->addLayout(statusLayout);
    
    // Separator
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);
    
    // Suggestions section
    QLabel *suggestionsLabel = new QLabel(tr("Recommendations:"));
    QFont suggestionsFont = suggestionsLabel->font();
    suggestionsFont.setBold(true);
    suggestionsLabel->setFont(suggestionsFont);
    layout->addWidget(suggestionsLabel);
    
    m_suggestionsList = new QListWidget();
    m_suggestionsList->setMaximumHeight(150);
    layout->addWidget(m_suggestionsList);
    
    layout->addStretch();
}

void StartupGuidanceDialog::setupDependenciesTab()
{
    m_dependenciesTab = new QWidget();
    m_tabWidget->addTab(m_dependenciesTab, QIcon::fromTheme("system-software-install"), tr("Dependencies"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_dependenciesTab);
    
    QLabel *headerLabel = new QLabel(tr("System Dependencies"));
    QFont headerFont = headerLabel->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 1);
    headerLabel->setFont(headerFont);
    layout->addWidget(headerLabel);
    
    QLabel *descLabel = new QLabel(tr("The following system dependencies are required for NFS Share Manager to function properly:"));
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
    
    m_dependenciesScrollArea = new QScrollArea();
    m_dependenciesScrollArea->setWidgetResizable(true);
    m_dependenciesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_dependenciesContent = new QWidget();
    m_dependenciesLayout = new QVBoxLayout(m_dependenciesContent);
    m_dependenciesLayout->addStretch();
    
    m_dependenciesScrollArea->setWidget(m_dependenciesContent);
    layout->addWidget(m_dependenciesScrollArea);
}

void StartupGuidanceDialog::setupServicesTab()
{
    m_servicesTab = new QWidget();
    m_tabWidget->addTab(m_servicesTab, QIcon::fromTheme("system-run"), tr("Services"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_servicesTab);
    
    QLabel *headerLabel = new QLabel(tr("System Services"));
    QFont headerFont = headerLabel->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 1);
    headerLabel->setFont(headerFont);
    layout->addWidget(headerLabel);
    
    QLabel *descLabel = new QLabel(tr("The following system services are needed for full NFS functionality:"));
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
    
    m_servicesScrollArea = new QScrollArea();
    m_servicesScrollArea->setWidgetResizable(true);
    m_servicesScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_servicesContent = new QWidget();
    m_servicesLayout = new QVBoxLayout(m_servicesContent);
    m_servicesLayout->addStretch();
    
    m_servicesScrollArea->setWidget(m_servicesContent);
    layout->addWidget(m_servicesScrollArea);
}

void StartupGuidanceDialog::setupSystemInfoTab()
{
    m_systemInfoTab = new QWidget();
    m_tabWidget->addTab(m_systemInfoTab, QIcon::fromTheme("help-about"), tr("System Info"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_systemInfoTab);
    
    QLabel *headerLabel = new QLabel(tr("System Information"));
    QFont headerFont = headerLabel->font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() + 1);
    headerLabel->setFont(headerFont);
    layout->addWidget(headerLabel);
    
    QLabel *descLabel = new QLabel(tr("This information can help with troubleshooting:"));
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);
    
    m_systemInfoText = new QTextEdit();
    m_systemInfoText->setReadOnly(true);
    m_systemInfoText->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_systemInfoText);
    
    m_copySystemInfoButton = new QPushButton(QIcon::fromTheme("edit-copy"), tr("Copy to Clipboard"));
    connect(m_copySystemInfoButton, &QPushButton::clicked, this, &StartupGuidanceDialog::onCopySystemInfoClicked);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_copySystemInfoButton);
    layout->addLayout(buttonLayout);
}

void StartupGuidanceDialog::updateValidationResult(const StartupValidationResult &result)
{
    m_validationResult = result;
    
    updateOverview();
    updateDependencies();
    updateServices();
    updateSystemInfo();
    updateButtons();
}

void StartupGuidanceDialog::updateOverview()
{
    // Update status icon and text
    if (m_validationResult.canStart && !m_validationResult.hasWarnings) {
        m_statusIcon->setPixmap(QIcon::fromTheme("dialog-ok").pixmap(48, 48));
        m_statusText->setText(tr("System Ready"));
        m_summaryText->setText(tr("All dependencies and services are available. NFS Share Manager is ready to start."));
    } else if (m_validationResult.canStart && m_validationResult.hasWarnings) {
        m_statusIcon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(48, 48));
        m_statusText->setText(tr("Ready with Warnings"));
        m_summaryText->setText(tr("NFS Share Manager can start but some features may not work properly due to missing dependencies or services."));
    } else {
        m_statusIcon->setPixmap(QIcon::fromTheme("dialog-error").pixmap(48, 48));
        m_statusText->setText(tr("Cannot Start"));
        m_summaryText->setText(tr("Critical issues prevent NFS Share Manager from starting properly. Please resolve the issues below."));
    }
    
    // Update suggestions list
    m_suggestionsList->clear();
    for (const QString &suggestion : m_validationResult.suggestions) {
        QListWidgetItem *item = new QListWidgetItem(suggestion);
        item->setIcon(QIcon::fromTheme("dialog-information"));
        m_suggestionsList->addItem(item);
    }
}

void StartupGuidanceDialog::updateDependencies()
{
    // Clear existing dependency widgets
    QLayoutItem *item;
    while ((item = m_dependenciesLayout->takeAt(0)) != nullptr) {
        if (item->widget() && item->widget() != m_dependenciesContent) {
            delete item->widget();
        }
        delete item;
    }
    
    // Add dependency widgets
    for (const StartupDependency &dep : m_validationResult.missingDependencies) {
        QWidget *depWidget = createDependencyWidget(dep);
        m_dependenciesLayout->insertWidget(m_dependenciesLayout->count() - 1, depWidget);
    }
    
    // Add all dependencies for completeness
    if (m_startupManager) {
        QList<StartupDependency> allDeps = m_startupManager->checkDependencies();
        for (const StartupDependency &dep : allDeps) {
            if (dep.isAvailable) {
                QWidget *depWidget = createDependencyWidget(dep);
                m_dependenciesLayout->insertWidget(m_dependenciesLayout->count() - 1, depWidget);
            }
        }
    }
}

void StartupGuidanceDialog::updateServices()
{
    // Clear existing service widgets
    QLayoutItem *item;
    while ((item = m_servicesLayout->takeAt(0)) != nullptr) {
        if (item->widget() && item->widget() != m_servicesContent) {
            delete item->widget();
        }
        delete item;
    }
    
    // Add service widgets
    for (const ServiceInfo &service : m_validationResult.unavailableServices) {
        QWidget *serviceWidget = createServiceWidget(service);
        m_servicesLayout->insertWidget(m_servicesLayout->count() - 1, serviceWidget);
    }
    
    // Add all services for completeness
    if (m_startupManager) {
        QList<ServiceInfo> allServices = m_startupManager->checkServices();
        for (const ServiceInfo &service : allServices) {
            if (service.isRunning) {
                QWidget *serviceWidget = createServiceWidget(service);
                m_servicesLayout->insertWidget(m_servicesLayout->count() - 1, serviceWidget);
            }
        }
    }
}

void StartupGuidanceDialog::updateSystemInfo()
{
    if (m_startupManager) {
        QString systemInfo = m_startupManager->getSystemInfo();
        
        // Add validation result summary
        systemInfo += tr("\nStartup Validation Result:\n");
        systemInfo += tr("Can Start: %1\n").arg(m_validationResult.canStart ? tr("Yes") : tr("No"));
        systemInfo += tr("Has Warnings: %1\n").arg(m_validationResult.hasWarnings ? tr("Yes") : tr("No"));
        systemInfo += tr("Critical Errors: %1\n").arg(m_validationResult.criticalErrors.size());
        systemInfo += tr("Warnings: %1\n").arg(m_validationResult.warnings.size());
        systemInfo += tr("Missing Dependencies: %1\n").arg(m_validationResult.missingDependencies.size());
        systemInfo += tr("Unavailable Services: %1\n").arg(m_validationResult.unavailableServices.size());
        
        if (!m_validationResult.criticalErrors.isEmpty()) {
            systemInfo += tr("\nCritical Errors:\n");
            for (const QString &error : m_validationResult.criticalErrors) {
                systemInfo += tr("- %1\n").arg(error);
            }
        }
        
        if (!m_validationResult.warnings.isEmpty()) {
            systemInfo += tr("\nWarnings:\n");
            for (const QString &warning : m_validationResult.warnings) {
                systemInfo += tr("- %1\n").arg(warning);
            }
        }
        
        m_systemInfoText->setPlainText(systemInfo);
    }
}

void StartupGuidanceDialog::updateButtons()
{
    bool hasMissingDeps = !m_validationResult.missingDependencies.isEmpty();
    bool hasUnavailableServices = !m_validationResult.unavailableServices.isEmpty();
    
    m_installDepsButton->setVisible(hasMissingDeps);
    m_installDepsButton->setEnabled(!m_operationInProgress);
    
    m_startServicesButton->setVisible(hasUnavailableServices);
    m_startServicesButton->setEnabled(!m_operationInProgress);
    
    m_tryAgainButton->setEnabled(!m_operationInProgress);
    
    m_continueButton->setEnabled(!m_operationInProgress);
    if (m_validationResult.canStart) {
        m_continueButton->setText(tr("Continue"));
        m_continueButton->setIcon(QIcon::fromTheme("dialog-ok"));
    } else {
        m_continueButton->setText(tr("Continue Anyway"));
        m_continueButton->setIcon(QIcon::fromTheme("dialog-warning"));
    }
    
    m_exitButton->setEnabled(!m_operationInProgress);
}

QWidget* StartupGuidanceDialog::createDependencyWidget(const StartupDependency &dependency)
{
    QGroupBox *groupBox = new QGroupBox(dependency.name);
    QVBoxLayout *layout = new QVBoxLayout(groupBox);
    
    // Status line
    QHBoxLayout *statusLayout = new QHBoxLayout();
    
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(getDependencyIcon(dependency).pixmap(16, 16));
    statusLayout->addWidget(iconLabel);
    
    QLabel *statusLabel = new QLabel(formatDependencyStatus(dependency));
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    if (dependency.isRequired) {
        QLabel *requiredLabel = new QLabel(tr("Required"));
        requiredLabel->setStyleSheet("color: red; font-weight: bold;");
        statusLayout->addWidget(requiredLabel);
    } else {
        QLabel *optionalLabel = new QLabel(tr("Optional"));
        optionalLabel->setStyleSheet("color: gray;");
        statusLayout->addWidget(optionalLabel);
    }
    
    layout->addLayout(statusLayout);
    
    // Description
    if (!dependency.description.isEmpty()) {
        QLabel *descLabel = new QLabel(dependency.description);
        descLabel->setWordWrap(true);
        descLabel->setStyleSheet("color: gray; font-size: 10pt;");
        layout->addWidget(descLabel);
    }
    
    // Install command (if available and not installed)
    if (!dependency.isAvailable && !dependency.installCommand.isEmpty()) {
        QLabel *installLabel = new QLabel(tr("Install with: %1").arg(dependency.installCommand));
        installLabel->setWordWrap(true);
        installLabel->setStyleSheet("font-family: monospace; background-color: #f0f0f0; padding: 4px; border: 1px solid #ccc;");
        layout->addWidget(installLabel);
    }
    
    return groupBox;
}

QWidget* StartupGuidanceDialog::createServiceWidget(const ServiceInfo &service)
{
    QGroupBox *groupBox = new QGroupBox(service.name);
    QVBoxLayout *layout = new QVBoxLayout(groupBox);
    
    // Status line
    QHBoxLayout *statusLayout = new QHBoxLayout();
    
    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(getServiceIcon(service).pixmap(16, 16));
    statusLayout->addWidget(iconLabel);
    
    QLabel *statusLabel = new QLabel(formatServiceStatus(service));
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    
    if (service.isRequired) {
        QLabel *requiredLabel = new QLabel(tr("Required"));
        requiredLabel->setStyleSheet("color: red; font-weight: bold;");
        statusLayout->addWidget(requiredLabel);
    } else {
        QLabel *optionalLabel = new QLabel(tr("Optional"));
        optionalLabel->setStyleSheet("color: gray;");
        statusLayout->addWidget(optionalLabel);
    }
    
    layout->addLayout(statusLayout);
    
    // Description
    if (!service.description.isEmpty()) {
        QLabel *descLabel = new QLabel(service.description);
        descLabel->setWordWrap(true);
        descLabel->setStyleSheet("color: gray; font-size: 10pt;");
        layout->addWidget(descLabel);
    }
    
    // Status message
    if (!service.statusMessage.isEmpty()) {
        QLabel *statusMsgLabel = new QLabel(service.statusMessage);
        statusMsgLabel->setWordWrap(true);
        statusMsgLabel->setStyleSheet("font-style: italic;");
        layout->addWidget(statusMsgLabel);
    }
    
    return groupBox;
}

QIcon StartupGuidanceDialog::getDependencyIcon(const StartupDependency &dependency) const
{
    if (dependency.isAvailable) {
        return QIcon::fromTheme("dialog-ok");
    } else if (dependency.isRequired) {
        return QIcon::fromTheme("dialog-error");
    } else {
        return QIcon::fromTheme("dialog-warning");
    }
}

QIcon StartupGuidanceDialog::getServiceIcon(const ServiceInfo &service) const
{
    if (service.isRunning) {
        return QIcon::fromTheme("dialog-ok");
    } else if (service.isRequired) {
        return QIcon::fromTheme("dialog-error");
    } else {
        return QIcon::fromTheme("dialog-warning");
    }
}

QString StartupGuidanceDialog::formatDependencyStatus(const StartupDependency &dependency) const
{
    if (dependency.isAvailable) {
        return tr("Available");
    } else {
        return tr("Missing");
    }
}

QString StartupGuidanceDialog::formatServiceStatus(const ServiceInfo &service) const
{
    if (service.isRunning) {
        return tr("Running");
    } else {
        return tr("Not Running");
    }
}

void StartupGuidanceDialog::onContinueAnywayClicked()
{
    m_result = Result::ContinueAnyway;
    accept();
}

void StartupGuidanceDialog::onTryAgainClicked()
{
    m_result = Result::TryAgain;
    accept();
}

void StartupGuidanceDialog::onInstallDependenciesClicked()
{
    if (!m_startupManager) {
        return;
    }
    
    // Show confirmation dialog
    QStringList missingNames;
    for (const StartupDependency &dep : m_validationResult.missingDependencies) {
        missingNames.append(dep.name);
    }
    
    QString message = tr("This will attempt to install the following dependencies:\n\n%1\n\n"
                        "This requires administrator privileges and may take some time.\n\n"
                        "Continue?").arg(missingNames.join(", "));
    
    int result = QMessageBox::question(this, tr("Install Dependencies"), message,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        m_result = Result::InstallDependencies;
        accept();
    }
}

void StartupGuidanceDialog::onStartServicesClicked()
{
    if (!m_startupManager) {
        return;
    }
    
    // Show confirmation dialog
    QStringList serviceNames;
    for (const ServiceInfo &service : m_validationResult.unavailableServices) {
        serviceNames.append(service.name);
    }
    
    QString message = tr("This will attempt to start the following services:\n\n%1\n\n"
                        "This requires administrator privileges.\n\n"
                        "Continue?").arg(serviceNames.join(", "));
    
    int result = QMessageBox::question(this, tr("Start Services"), message,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        m_result = Result::StartServices;
        accept();
    }
}

void StartupGuidanceDialog::onExitClicked()
{
    m_result = Result::Exit;
    reject();
}

void StartupGuidanceDialog::onShowDetailsToggled(bool show)
{
    m_showingDetails = show;
    
    if (show) {
        m_detailsButton->setText(tr("Hide Details"));
        m_tabWidget->setTabVisible(1, true); // Dependencies
        m_tabWidget->setTabVisible(2, true); // Services
        m_tabWidget->setTabVisible(3, true); // System Info
        resize(800, 600);
    } else {
        m_detailsButton->setText(tr("Show Details"));
        m_tabWidget->setCurrentIndex(0); // Overview
        m_tabWidget->setTabVisible(1, false); // Dependencies
        m_tabWidget->setTabVisible(2, false); // Services
        m_tabWidget->setTabVisible(3, false); // System Info
        resize(600, 400);
    }
}

void StartupGuidanceDialog::onCopySystemInfoClicked()
{
    QApplication::clipboard()->setText(m_systemInfoText->toPlainText());
    
    // Show brief confirmation
    m_copySystemInfoButton->setText(tr("Copied!"));
    QTimer::singleShot(2000, this, [this]() {
        m_copySystemInfoButton->setText(tr("Copy to Clipboard"));
    });
}

void StartupGuidanceDialog::onDependencyInstallationStarted()
{
    m_operationInProgress = true;
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressLabel->setText(tr("Installing dependencies..."));
    m_progressLabel->setVisible(true);
    m_progressTimer->start();
    updateButtons();
}

void StartupGuidanceDialog::onDependencyInstallationFinished(bool success)
{
    m_operationInProgress = false;
    m_progressBar->setVisible(false);
    m_progressLabel->setVisible(false);
    m_progressTimer->stop();
    
    if (success) {
        QMessageBox::information(this, tr("Installation Complete"), 
                               tr("Dependencies have been installed successfully."));
    } else {
        QMessageBox::warning(this, tr("Installation Failed"), 
                           tr("Some dependencies could not be installed. Please check the system information for details."));
    }
    
    updateButtons();
}

void StartupGuidanceDialog::onServiceStartupStarted()
{
    m_operationInProgress = true;
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressLabel->setText(tr("Starting services..."));
    m_progressLabel->setVisible(true);
    m_progressTimer->start();
    updateButtons();
}

void StartupGuidanceDialog::onServiceStartupFinished(bool success)
{
    m_operationInProgress = false;
    m_progressBar->setVisible(false);
    m_progressLabel->setVisible(false);
    m_progressTimer->stop();
    
    if (success) {
        QMessageBox::information(this, tr("Services Started"), 
                               tr("Services have been started successfully."));
    } else {
        QMessageBox::warning(this, tr("Service Startup Failed"), 
                           tr("Some services could not be started. Please check the system information for details."));
    }
    
    updateButtons();
}

void StartupGuidanceDialog::updateProgress()
{
    // Simple progress animation
    static int dots = 0;
    dots = (dots + 1) % 4;
    
    QString baseText = m_progressLabel->text().split('.').first();
    m_progressLabel->setText(baseText + QString(".").repeated(dots));
}

} // namespace NFSShareManager