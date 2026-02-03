#include "mountdialog.h"
#include "../core/remotenfsshare.h"
#include "../core/nfsmount.h"
#include "../business/mountmanager.h"

#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>
#include <QDebug>

namespace NFSShareManager {

MountDialog::MountDialog(const RemoteNFSShare &remoteShare, 
                        MountManager *mountManager,
                        QWidget *parent)
    : QDialog(parent)
    , m_remoteShare(remoteShare)
    , m_mountManager(mountManager)
    , m_tabWidget(nullptr)
    , m_basicTab(nullptr)
    , m_shareInfoLabel(nullptr)
    , m_mountPointEdit(nullptr)
    , m_browseMountPointButton(nullptr)
    , m_mountPointValidationLabel(nullptr)
    , m_mountTypeGroup(nullptr)
    , m_temporaryMountRadio(nullptr)
    , m_persistentMountRadio(nullptr)
    , m_nfsVersionCombo(nullptr)
    , m_readOnlyCheckBox(nullptr)
    , m_advancedTab(nullptr)
    , m_timeoutSpinBox(nullptr)
    , m_retryCountSpinBox(nullptr)
    , m_softMountCheckBox(nullptr)
    , m_backgroundMountCheckBox(nullptr)
    , m_rsizeSpinBox(nullptr)
    , m_wsizeSpinBox(nullptr)
    , m_securityFlavorEdit(nullptr)
    , m_customOptionsEdit(nullptr)
    , m_mountButton(nullptr)
    , m_cancelButton(nullptr)
    , m_testConnectionButton(nullptr)
    , m_resetDefaultsButton(nullptr)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_isValidConfiguration(false)
    , m_isMounting(false)
{
    setupUI();
    connectSignals();
    loadDefaults();
    updateMountPointValidation();
}

QString MountDialog::getMountPoint() const
{
    return m_mountPointEdit ? m_mountPointEdit->text().trimmed() : QString();
}

MountOptions MountDialog::getMountOptions() const
{
    MountOptions options;
    
    if (m_nfsVersionCombo) {
        QString versionText = m_nfsVersionCombo->currentData().toString();
        options.nfsVersion = stringToNFSVersion(versionText);
    }
    
    if (m_readOnlyCheckBox) {
        options.readOnly = m_readOnlyCheckBox->isChecked();
    }
    
    if (m_timeoutSpinBox) {
        options.timeoutSeconds = m_timeoutSpinBox->value();
    }
    
    if (m_retryCountSpinBox) {
        options.retryCount = m_retryCountSpinBox->value();
    }
    
    if (m_softMountCheckBox) {
        options.softMount = m_softMountCheckBox->isChecked();
    }
    
    if (m_backgroundMountCheckBox) {
        options.backgroundMount = m_backgroundMountCheckBox->isChecked();
    }
    
    if (m_rsizeSpinBox) {
        options.rsize = m_rsizeSpinBox->value();
    }
    
    if (m_wsizeSpinBox) {
        options.wsize = m_wsizeSpinBox->value();
    }
    
    if (m_securityFlavorEdit) {
        options.securityFlavor = m_securityFlavorEdit->text().trimmed();
    }
    
    if (m_customOptionsEdit) {
        QString customText = m_customOptionsEdit->toPlainText().trimmed();
        if (!customText.isEmpty()) {
            // Parse custom options from text
            QStringList lines = customText.split('\n', Qt::SkipEmptyParts);
            for (const QString &line : lines) {
                QStringList parts = line.split('=', Qt::SkipEmptyParts);
                if (parts.size() == 2) {
                    options.customOptions[parts[0].trimmed()] = parts[1].trimmed();
                } else if (parts.size() == 1) {
                    options.customOptions[parts[0].trimmed()] = true;
                }
            }
        }
    }
    
    return options;
}

bool MountDialog::isPersistent() const
{
    return m_persistentMountRadio ? m_persistentMountRadio->isChecked() : false;
}

NFSMount MountDialog::getMount() const
{
    return NFSMount(m_remoteShare, getMountPoint(), getMountOptions(), isPersistent());
}

void MountDialog::accept()
{
    if (!validateConfiguration()) {
        showValidationErrors(getValidationErrors());
        return;
    }
    
    // Start the mount operation
    if (m_mountManager) {
        m_isMounting = true;
        updateDialogState();
        
        MountManager::MountResult result = m_mountManager->mountShare(
            m_remoteShare, getMountPoint(), getMountOptions(), isPersistent());
        
        if (result == MountManager::MountResult::Success) {
            QDialog::accept();
        } else {
            m_isMounting = false;
            updateDialogState();
            
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
            
            QMessageBox::warning(this, tr("Mount Failed"), errorMsg);
        }
    }
}

void MountDialog::onBrowseMountPointClicked()
{
    QString currentPath = getMountPoint();
    if (currentPath.isEmpty()) {
        currentPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    
    QString selectedPath = QFileDialog::getExistingDirectory(
        this, tr("Select Mount Point Directory"), currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!selectedPath.isEmpty()) {
        m_mountPointEdit->setText(selectedPath);
        onMountPointChanged();
    }
}

void MountDialog::onMountPointChanged()
{
    updateMountPointValidation();
    updateDialogState();
}

void MountDialog::onMountTypeChanged()
{
    updateDialogState();
}

void MountDialog::onNFSVersionChanged()
{
    updateDialogState();
}

void MountDialog::onAdvancedOptionsToggled(bool enabled)
{
    if (m_advancedTab) {
        m_advancedTab->setEnabled(enabled);
    }
}

void MountDialog::onTestConnectionClicked()
{
    // TODO: Implement connection test
    if (m_statusLabel) {
        m_statusLabel->setText(tr("Connection test not yet implemented"));
    }
}

void MountDialog::onMountStarted(const RemoteNFSShare &remoteShare, const QString &mountPoint)
{
    if (remoteShare.uniqueId() == m_remoteShare.uniqueId() && mountPoint == getMountPoint()) {
        m_isMounting = true;
        updateDialogState();
        if (m_statusLabel) {
            m_statusLabel->setText(tr("Mounting share..."));
        }
    }
}

void MountDialog::onMountCompleted(const NFSMount &mount)
{
    if (mount.remoteShare().uniqueId() == m_remoteShare.uniqueId() && 
        mount.localMountPoint() == getMountPoint()) {
        m_isMounting = false;
        QDialog::accept();
    }
}

void MountDialog::onMountFailed(const RemoteNFSShare &remoteShare, const QString &mountPoint, 
                               MountManager::MountResult result, const QString &errorMessage)
{
    if (remoteShare.uniqueId() == m_remoteShare.uniqueId() && mountPoint == getMountPoint()) {
        m_isMounting = false;
        updateDialogState();
        
        QString fullErrorMsg = tr("Mount operation failed");
        if (!errorMessage.isEmpty()) {
            fullErrorMsg += tr(": %1").arg(errorMessage);
        }
        
        QMessageBox::warning(this, tr("Mount Failed"), fullErrorMsg);
    }
}

void MountDialog::updateMountPointValidation()
{
    QString mountPoint = getMountPoint();
    m_validationErrors.clear();
    m_isValidConfiguration = false;
    
    if (!m_mountPointValidationLabel) {
        return;
    }
    
    if (mountPoint.isEmpty()) {
        m_mountPointValidationLabel->setText(tr("Please specify a mount point"));
        m_mountPointValidationLabel->setStyleSheet("QLabel { color: orange; }");
        return;
    }
    
    if (m_mountManager) {
        QStringList errors = m_mountManager->getMountPointValidationErrors(mountPoint);
        if (!errors.isEmpty()) {
            m_validationErrors = errors;
            m_mountPointValidationLabel->setText(errors.first());
            m_mountPointValidationLabel->setStyleSheet("QLabel { color: red; }");
            return;
        }
        
        if (m_mountManager->validateMountPoint(mountPoint)) {
            m_mountPointValidationLabel->setText(tr("✓ Mount point is valid"));
            m_mountPointValidationLabel->setStyleSheet("QLabel { color: green; }");
            m_isValidConfiguration = true;
        } else {
            m_mountPointValidationLabel->setText(tr("Mount point validation failed"));
            m_mountPointValidationLabel->setStyleSheet("QLabel { color: red; }");
        }
    }
}

void MountDialog::updateDialogState()
{
    bool canMount = m_isValidConfiguration && !m_isMounting;
    
    if (m_mountButton) {
        m_mountButton->setEnabled(canMount);
        m_mountButton->setText(m_isMounting ? tr("Mounting...") : tr("Mount"));
    }
    
    if (m_progressBar) {
        m_progressBar->setVisible(m_isMounting);
        if (m_isMounting) {
            m_progressBar->setRange(0, 0); // Indeterminate progress
        }
    }
    
    // Update status message
    if (m_statusLabel && !m_isMounting) {
        if (m_isValidConfiguration) {
            m_statusLabel->setText(tr("Ready to mount"));
        } else if (!m_validationErrors.isEmpty()) {
            m_statusLabel->setText(m_validationErrors.first());
        } else {
            m_statusLabel->setText(tr("Please configure mount options"));
        }
    }
}

void MountDialog::resetToDefaults()
{
    loadDefaults();
    updateMountPointValidation();
    updateDialogState();
}

void MountDialog::setupUI()
{
    setWindowTitle(tr("Mount NFS Share"));
    setModal(true);
    resize(600, 500);
    setMinimumSize(500, 400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    setupBasicTab();
    setupAdvancedTab();
    
    // Progress bar (initially hidden)
    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Status label
    m_statusLabel = new QLabel(tr("Configure mount options"));
    m_statusLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    mainLayout->addWidget(m_statusLabel);
    
    setupButtons();
    mainLayout->addLayout(createButtonLayout());
}

void MountDialog::setupBasicTab()
{
    m_basicTab = new QWidget();
    m_tabWidget->addTab(m_basicTab, tr("Basic Options"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_basicTab);
    
    // Share information
    QGroupBox *shareInfoGroup = new QGroupBox(tr("Remote Share Information"));
    QFormLayout *shareInfoLayout = new QFormLayout(shareInfoGroup);
    
    QString shareInfo = tr("Host: %1\nIP: %2\nExport: %3")
                       .arg(m_remoteShare.hostName())
                       .arg(m_remoteShare.hostAddress().toString())
                       .arg(m_remoteShare.exportPath());
    
    m_shareInfoLabel = new QLabel(shareInfo);
    m_shareInfoLabel->setWordWrap(true);
    shareInfoLayout->addRow(m_shareInfoLabel);
    
    layout->addWidget(shareInfoGroup);
    
    // Mount point configuration
    QGroupBox *mountPointGroup = new QGroupBox(tr("Mount Point"));
    QVBoxLayout *mountPointLayout = new QVBoxLayout(mountPointGroup);
    
    QHBoxLayout *mountPointEditLayout = new QHBoxLayout();
    m_mountPointEdit = new QLineEdit();
    m_mountPointEdit->setPlaceholderText(tr("Enter or browse for mount point directory"));
    mountPointEditLayout->addWidget(m_mountPointEdit);
    
    m_browseMountPointButton = new QPushButton(tr("Browse..."));
    mountPointEditLayout->addWidget(m_browseMountPointButton);
    
    mountPointLayout->addLayout(mountPointEditLayout);
    
    m_mountPointValidationLabel = new QLabel(tr("Please specify a mount point"));
    m_mountPointValidationLabel->setStyleSheet("QLabel { color: orange; }");
    mountPointLayout->addWidget(m_mountPointValidationLabel);
    
    layout->addWidget(mountPointGroup);
    
    // Mount type selection
    QGroupBox *mountTypeGroup = new QGroupBox(tr("Mount Type"));
    QVBoxLayout *mountTypeLayout = new QVBoxLayout(mountTypeGroup);
    
    m_mountTypeGroup = new QButtonGroup(this);
    
    m_temporaryMountRadio = new QRadioButton(tr("Temporary mount (until reboot)"));
    m_temporaryMountRadio->setToolTip(tr("The mount will be removed when the system is restarted"));
    m_mountTypeGroup->addButton(m_temporaryMountRadio, 0);
    mountTypeLayout->addWidget(m_temporaryMountRadio);
    
    m_persistentMountRadio = new QRadioButton(tr("Persistent mount (survives reboot)"));
    m_persistentMountRadio->setToolTip(tr("The mount will be automatically restored after system restart"));
    m_mountTypeGroup->addButton(m_persistentMountRadio, 1);
    mountTypeLayout->addWidget(m_persistentMountRadio);
    
    // Default to temporary mount
    m_temporaryMountRadio->setChecked(true);
    
    layout->addWidget(mountTypeGroup);
    
    // Basic mount options
    QGroupBox *basicOptionsGroup = new QGroupBox(tr("Basic Options"));
    QFormLayout *basicOptionsLayout = new QFormLayout(basicOptionsGroup);
    
    // NFS version selection
    m_nfsVersionCombo = new QComboBox();
    m_nfsVersionCombo->addItem(tr("NFS v3"), "Version3");
    m_nfsVersionCombo->addItem(tr("NFS v4"), "Version4");
    m_nfsVersionCombo->addItem(tr("NFS v4.1"), "Version4_1");
    m_nfsVersionCombo->addItem(tr("NFS v4.2"), "Version4_2");
    m_nfsVersionCombo->setCurrentIndex(1); // Default to NFS v4
    basicOptionsLayout->addRow(tr("NFS Version:"), m_nfsVersionCombo);
    
    // Read-only option
    m_readOnlyCheckBox = new QCheckBox(tr("Mount as read-only"));
    m_readOnlyCheckBox->setToolTip(tr("Prevent write operations to the mounted share"));
    basicOptionsLayout->addRow(m_readOnlyCheckBox);
    
    layout->addWidget(basicOptionsGroup);
    
    layout->addStretch();
}

void MountDialog::setupAdvancedTab()
{
    m_advancedTab = new QWidget();
    m_tabWidget->addTab(m_advancedTab, tr("Advanced Options"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_advancedTab);
    
    // Network options
    QGroupBox *networkGroup = new QGroupBox(tr("Network Options"));
    QFormLayout *networkLayout = new QFormLayout(networkGroup);
    
    m_timeoutSpinBox = new QSpinBox();
    m_timeoutSpinBox->setRange(1, 600);
    m_timeoutSpinBox->setValue(60);
    m_timeoutSpinBox->setSuffix(tr(" seconds"));
    m_timeoutSpinBox->setToolTip(tr("Network timeout for NFS operations"));
    networkLayout->addRow(tr("Timeout:"), m_timeoutSpinBox);
    
    m_retryCountSpinBox = new QSpinBox();
    m_retryCountSpinBox->setRange(0, 10);
    m_retryCountSpinBox->setValue(2);
    m_retryCountSpinBox->setToolTip(tr("Number of retry attempts for failed operations"));
    networkLayout->addRow(tr("Retry Count:"), m_retryCountSpinBox);
    
    layout->addWidget(networkGroup);
    
    // Mount behavior options
    QGroupBox *behaviorGroup = new QGroupBox(tr("Mount Behavior"));
    QVBoxLayout *behaviorLayout = new QVBoxLayout(behaviorGroup);
    
    m_softMountCheckBox = new QCheckBox(tr("Soft mount"));
    m_softMountCheckBox->setToolTip(tr("Allow operations to fail and return error instead of hanging"));
    behaviorLayout->addWidget(m_softMountCheckBox);
    
    m_backgroundMountCheckBox = new QCheckBox(tr("Background mount"));
    m_backgroundMountCheckBox->setToolTip(tr("Retry mount operations in the background"));
    behaviorLayout->addWidget(m_backgroundMountCheckBox);
    
    layout->addWidget(behaviorGroup);
    
    // Performance options
    QGroupBox *performanceGroup = new QGroupBox(tr("Performance Options"));
    QFormLayout *performanceLayout = new QFormLayout(performanceGroup);
    
    m_rsizeSpinBox = new QSpinBox();
    m_rsizeSpinBox->setRange(0, 1048576);
    m_rsizeSpinBox->setValue(0);
    m_rsizeSpinBox->setSpecialValueText(tr("Default"));
    m_rsizeSpinBox->setToolTip(tr("Read buffer size in bytes (0 for default)"));
    performanceLayout->addRow(tr("Read Size:"), m_rsizeSpinBox);
    
    m_wsizeSpinBox = new QSpinBox();
    m_wsizeSpinBox->setRange(0, 1048576);
    m_wsizeSpinBox->setValue(0);
    m_wsizeSpinBox->setSpecialValueText(tr("Default"));
    m_wsizeSpinBox->setToolTip(tr("Write buffer size in bytes (0 for default)"));
    performanceLayout->addRow(tr("Write Size:"), m_wsizeSpinBox);
    
    layout->addWidget(performanceGroup);
    
    // Security options
    QGroupBox *securityGroup = new QGroupBox(tr("Security Options"));
    QFormLayout *securityLayout = new QFormLayout(securityGroup);
    
    m_securityFlavorEdit = new QLineEdit();
    m_securityFlavorEdit->setPlaceholderText(tr("e.g., sys, krb5, krb5i, krb5p"));
    m_securityFlavorEdit->setToolTip(tr("Security flavor for authentication"));
    securityLayout->addRow(tr("Security Flavor:"), m_securityFlavorEdit);
    
    layout->addWidget(securityGroup);
    
    // Custom options
    QGroupBox *customGroup = new QGroupBox(tr("Custom Options"));
    QVBoxLayout *customLayout = new QVBoxLayout(customGroup);
    
    QLabel *customLabel = new QLabel(tr("Additional mount options (one per line, format: option=value or option):"));
    customLabel->setWordWrap(true);
    customLayout->addWidget(customLabel);
    
    m_customOptionsEdit = new QTextEdit();
    m_customOptionsEdit->setMaximumHeight(100);
    m_customOptionsEdit->setPlaceholderText(tr("noatime\nnodev\nnosuid"));
    customLayout->addWidget(m_customOptionsEdit);
    
    layout->addWidget(customGroup);
    
    layout->addStretch();
}

void MountDialog::setupButtons()
{
    // This method is called from setupUI, but the actual button layout
    // is created in createButtonLayout() method
}

QHBoxLayout* MountDialog::createButtonLayout()
{
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_testConnectionButton = new QPushButton(tr("Test Connection"));
    m_testConnectionButton->setToolTip(tr("Test connectivity to the remote share"));
    buttonLayout->addWidget(m_testConnectionButton);
    
    m_resetDefaultsButton = new QPushButton(tr("Reset Defaults"));
    m_resetDefaultsButton->setToolTip(tr("Reset all options to default values"));
    buttonLayout->addWidget(m_resetDefaultsButton);
    
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"));
    buttonLayout->addWidget(m_cancelButton);
    
    m_mountButton = new QPushButton(tr("Mount"));
    m_mountButton->setDefault(true);
    buttonLayout->addWidget(m_mountButton);
    
    return buttonLayout;
}

void MountDialog::connectSignals()
{
    // Mount point signals
    if (m_mountPointEdit) {
        connect(m_mountPointEdit, &QLineEdit::textChanged,
                this, &MountDialog::onMountPointChanged);
    }
    
    if (m_browseMountPointButton) {
        connect(m_browseMountPointButton, &QPushButton::clicked,
                this, &MountDialog::onBrowseMountPointClicked);
    }
    
    // Mount type signals
    if (m_mountTypeGroup) {
        connect(m_mountTypeGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
                this, [this](QAbstractButton*) { onMountTypeChanged(); });
    }
    
    // NFS version signals
    if (m_nfsVersionCombo) {
        connect(m_nfsVersionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &MountDialog::onNFSVersionChanged);
    }
    
    // Button signals
    if (m_mountButton) {
        connect(m_mountButton, &QPushButton::clicked, this, &MountDialog::accept);
    }
    
    if (m_cancelButton) {
        connect(m_cancelButton, &QPushButton::clicked, this, &MountDialog::reject);
    }
    
    if (m_testConnectionButton) {
        connect(m_testConnectionButton, &QPushButton::clicked,
                this, &MountDialog::onTestConnectionClicked);
    }
    
    if (m_resetDefaultsButton) {
        connect(m_resetDefaultsButton, &QPushButton::clicked,
                this, &MountDialog::resetToDefaults);
    }
    
    // Mount manager signals
    if (m_mountManager) {
        connect(m_mountManager, &MountManager::mountStarted,
                this, &MountDialog::onMountStarted);
        connect(m_mountManager, &MountManager::mountCompleted,
                this, &MountDialog::onMountCompleted);
        connect(m_mountManager, &MountManager::mountFailed,
                this, &MountDialog::onMountFailed);
    }
}

void MountDialog::loadDefaults()
{
    // Generate suggested mount point
    QString suggestedPath = generateSuggestedMountPoint();
    if (m_mountPointEdit) {
        m_mountPointEdit->setText(suggestedPath);
    }
    
    // Load default mount options from mount manager
    if (m_mountManager) {
        MountOptions defaults = m_mountManager->getDefaultMountOptions(m_remoteShare);
        
        // Set NFS version
        if (m_nfsVersionCombo) {
            QString versionStr = nfsVersionToString(defaults.nfsVersion);
            int index = m_nfsVersionCombo->findData(versionStr);
            if (index >= 0) {
                m_nfsVersionCombo->setCurrentIndex(index);
            }
        }
        
        // Set other options
        if (m_readOnlyCheckBox) {
            m_readOnlyCheckBox->setChecked(defaults.readOnly);
        }
        
        if (m_timeoutSpinBox) {
            m_timeoutSpinBox->setValue(defaults.timeoutSeconds);
        }
        
        if (m_retryCountSpinBox) {
            m_retryCountSpinBox->setValue(defaults.retryCount);
        }
        
        if (m_softMountCheckBox) {
            m_softMountCheckBox->setChecked(defaults.softMount);
        }
        
        if (m_backgroundMountCheckBox) {
            m_backgroundMountCheckBox->setChecked(defaults.backgroundMount);
        }
        
        if (m_rsizeSpinBox) {
            m_rsizeSpinBox->setValue(defaults.rsize);
        }
        
        if (m_wsizeSpinBox) {
            m_wsizeSpinBox->setValue(defaults.wsize);
        }
        
        if (m_securityFlavorEdit) {
            m_securityFlavorEdit->setText(defaults.securityFlavor);
        }
    }
}

bool MountDialog::validateConfiguration()
{
    m_validationErrors = getValidationErrors();
    return m_validationErrors.isEmpty();
}

QStringList MountDialog::getValidationErrors()
{
    QStringList errors;
    
    QString mountPoint = getMountPoint();
    if (mountPoint.isEmpty()) {
        errors << tr("Mount point is required");
    } else if (m_mountManager) {
        QStringList mountPointErrors = m_mountManager->getMountPointValidationErrors(mountPoint);
        errors.append(mountPointErrors);
    }
    
    // Validate mount options
    MountOptions options = getMountOptions();
    if (!options.isValid()) {
        errors.append(options.validationErrors());
    }
    
    return errors;
}

QString MountDialog::generateSuggestedMountPoint()
{
    QString basePath = "/mnt/nfs";
    QString hostName = m_remoteShare.hostName();
    QString exportPath = m_remoteShare.exportPath();
    
    // Clean up export path for use in directory name
    QString cleanExportPath = exportPath;
    cleanExportPath.replace('/', '_');
    if (cleanExportPath.startsWith('_')) {
        cleanExportPath = cleanExportPath.mid(1);
    }
    if (cleanExportPath.isEmpty()) {
        cleanExportPath = "root";
    }
    
    QString suggestedPath = QString("%1/%2_%3").arg(basePath, hostName, cleanExportPath);
    
    // Ensure the path doesn't already exist or is not in use
    int counter = 1;
    QString originalPath = suggestedPath;
    while (QDir(suggestedPath).exists() || 
           (m_mountManager && m_mountManager->isManagedMount(suggestedPath))) {
        suggestedPath = QString("%1_%2").arg(originalPath).arg(counter++);
    }
    
    return suggestedPath;
}

void MountDialog::updateMountPointSuggestions()
{
    // This could be extended to provide a dropdown with suggestions
    // For now, we just update the validation
    updateMountPointValidation();
}

void MountDialog::showValidationErrors(const QStringList &errors)
{
    if (errors.isEmpty()) {
        return;
    }
    
    QString message = tr("Please correct the following errors:\n\n");
    for (int i = 0; i < errors.size(); ++i) {
        message += QString("• %1\n").arg(errors[i]);
    }
    
    QMessageBox::warning(this, tr("Configuration Errors"), message);
}

} // namespace NFSShareManager