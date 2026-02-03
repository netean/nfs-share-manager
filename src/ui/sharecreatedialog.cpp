#include "sharecreatedialog.h"
#include "permissionmanagerdialog.h"
#include "../business/sharemanager.h"
#include "../core/nfsshare.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QTimer>
#include <QDebug>

namespace NFSShareManager {

ShareCreateDialog::ShareCreateDialog(ShareManager *shareManager, QWidget *parent)
    : QDialog(parent)
    , m_shareManager(shareManager)
    , m_tabWidget(nullptr)
    , m_buttonBox(nullptr)
    , m_testButton(nullptr)
    , m_basicTab(nullptr)
    , m_pathEdit(nullptr)
    , m_browseButton(nullptr)
    , m_shareNameEdit(nullptr)
    , m_accessModeCombo(nullptr)
    , m_nfsVersionCombo(nullptr)
    , m_allowRootAccessCheck(nullptr)
    , m_pathValidationLabel(nullptr)
    , m_shareNameValidationLabel(nullptr)
    , m_permissionsTab(nullptr)
    , m_allowedHostsList(nullptr)
    , m_newHostEdit(nullptr)
    , m_addHostButton(nullptr)
    , m_removeHostButton(nullptr)
    , m_allowedUsersList(nullptr)
    , m_newUserEdit(nullptr)
    , m_addUserButton(nullptr)
    , m_removeUserButton(nullptr)
    , m_rootSquashCheck(nullptr)
    , m_anonymousUserEdit(nullptr)
    , m_advancedPermissionsButton(nullptr)
    , m_advancedTab(nullptr)
    , m_customOptionsEdit(nullptr)
    , m_configPreviewEdit(nullptr)
    , m_enableAdvancedCheck(nullptr)
    , m_validationProgress(nullptr)
    , m_statusLabel(nullptr)
    , m_isValid(false)
{
    setupUI();
    connectSignals();
    updateUIState();
    
    // Set initial focus
    if (m_pathEdit) {
        m_pathEdit->setFocus();
    }
}

ShareCreateDialog::~ShareCreateDialog()
{
}

QString ShareCreateDialog::getSharePath() const
{
    return m_pathEdit ? m_pathEdit->text().trimmed() : QString();
}

ShareConfiguration ShareCreateDialog::getShareConfiguration() const
{
    ShareConfiguration config;
    
    if (m_shareNameEdit) {
        config.setName(m_shareNameEdit->text().trimmed());
    }
    
    if (m_accessModeCombo) {
        AccessMode mode = static_cast<AccessMode>(m_accessModeCombo->currentData().toInt());
        config.setAccessMode(mode);
    }
    
    if (m_nfsVersionCombo) {
        NFSVersion version = static_cast<NFSVersion>(m_nfsVersionCombo->currentData().toInt());
        config.setNfsVersion(version);
    }
    
    if (m_allowRootAccessCheck) {
        config.setAllowRootAccess(m_allowRootAccessCheck->isChecked());
    }
    
    // Set allowed hosts
    if (m_allowedHostsList) {
        QStringList hosts;
        for (int i = 0; i < m_allowedHostsList->count(); ++i) {
            hosts.append(m_allowedHostsList->item(i)->text());
        }
        config.setAllowedHosts(hosts);
    }
    
    // Set allowed users
    if (m_allowedUsersList) {
        QStringList users;
        for (int i = 0; i < m_allowedUsersList->count(); ++i) {
            users.append(m_allowedUsersList->item(i)->text());
        }
        config.setAllowedUsers(users);
    }
    
    return config;
}

PermissionSet ShareCreateDialog::getPermissions() const
{
    // If detailed permissions have been configured, use those
    if (!m_detailedPermissions.hostPermissions().isEmpty() || 
        !m_detailedPermissions.userPermissions().isEmpty() ||
        m_detailedPermissions.anonymousUser() != "nobody") {
        return m_detailedPermissions;
    }
    
    // Otherwise, build from basic UI
    PermissionSet permissions;
    
    if (m_accessModeCombo) {
        AccessMode mode = static_cast<AccessMode>(m_accessModeCombo->currentData().toInt());
        permissions.setDefaultAccess(mode);
    }
    
    if (m_rootSquashCheck) {
        permissions.setEnableRootSquash(m_rootSquashCheck->isChecked());
    }
    
    if (m_anonymousUserEdit) {
        permissions.setAnonymousUser(m_anonymousUserEdit->text().trimmed());
    }
    
    return permissions;
}

void ShareCreateDialog::accept()
{
    // Final validation before accepting
    validateConfiguration();
    
    if (!m_isValid) {
        showValidationErrors(m_validationErrors);
        return;
    }
    
    // Check if path is already shared
    QString path = getSharePath();
    if (m_shareManager && m_shareManager->isShared(path)) {
        QMessageBox::warning(this, tr("Path Already Shared"),
                           tr("The directory '%1' is already being shared.\n\n"
                              "Please choose a different directory or remove the existing share first.")
                           .arg(path));
        return;
    }
    
    QDialog::accept();
}

void ShareCreateDialog::onBrowseClicked()
{
    QString currentPath = m_pathEdit ? m_pathEdit->text().trimmed() : QString();
    if (currentPath.isEmpty()) {
        currentPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    
    QString selectedPath = QFileDialog::getExistingDirectory(
        this,
        tr("Select Directory to Share"),
        currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!selectedPath.isEmpty() && m_pathEdit) {
        m_pathEdit->setText(selectedPath);
        onPathChanged();
    }
}

void ShareCreateDialog::onPathChanged()
{
    QString path = getSharePath();
    
    // Auto-generate share name if empty
    if (m_shareNameEdit && m_shareNameEdit->text().isEmpty() && !path.isEmpty()) {
        QFileInfo info(path);
        QString suggestedName = info.baseName();
        if (suggestedName.isEmpty()) {
            suggestedName = info.fileName();
        }
        if (suggestedName.isEmpty()) {
            suggestedName = tr("Root");
        }
        m_shareNameEdit->setText(suggestedName);
    }
    
    validateConfiguration();
}

void ShareCreateDialog::onShareNameChanged()
{
    validateConfiguration();
}

void ShareCreateDialog::onAccessModeChanged()
{
    updateUIState();
    validateConfiguration();
    updateConfigurationPreview();
}

void ShareCreateDialog::onNFSVersionChanged()
{
    updateConfigurationPreview();
}

void ShareCreateDialog::onRootAccessChanged()
{
    updateConfigurationPreview();
}

void ShareCreateDialog::onAddAllowedHostClicked()
{
    if (!m_newHostEdit || !m_allowedHostsList) {
        return;
    }
    
    QString host = m_newHostEdit->text().trimmed();
    if (host.isEmpty()) {
        return;
    }
    
    // Validate host format
    QRegularExpression hostRegex(R"(^(?:(?:\d{1,3}\.){3}\d{1,3}(?:/\d{1,2})?|[a-zA-Z0-9.-]+|\*)$)");
    if (!hostRegex.match(host).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Host"),
                           tr("Please enter a valid hostname, IP address, or network range.\n\n"
                              "Examples:\n"
                              "• hostname.example.com\n"
                              "• 192.168.1.100\n"
                              "• 192.168.1.0/24\n"
                              "• * (all hosts)"));
        return;
    }
    
    // Check for duplicates
    for (int i = 0; i < m_allowedHostsList->count(); ++i) {
        if (m_allowedHostsList->item(i)->text() == host) {
            QMessageBox::information(this, tr("Duplicate Host"),
                                   tr("The host '%1' is already in the allowed hosts list.").arg(host));
            return;
        }
    }
    
    m_allowedHostsList->addItem(host);
    m_newHostEdit->clear();
    updateConfigurationPreview();
}

void ShareCreateDialog::onRemoveAllowedHostClicked()
{
    if (!m_allowedHostsList) {
        return;
    }
    
    QListWidgetItem *currentItem = m_allowedHostsList->currentItem();
    if (currentItem) {
        delete currentItem;
        updateConfigurationPreview();
    }
}

void ShareCreateDialog::onAddAllowedUserClicked()
{
    if (!m_newUserEdit || !m_allowedUsersList) {
        return;
    }
    
    QString user = m_newUserEdit->text().trimmed();
    if (user.isEmpty()) {
        return;
    }
    
    // Validate username format
    QRegularExpression userRegex(R"(^[a-zA-Z0-9._-]+$)");
    if (!userRegex.match(user).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid Username"),
                           tr("Please enter a valid username.\n\n"
                              "Usernames can contain letters, numbers, dots, underscores, and hyphens."));
        return;
    }
    
    // Check for duplicates
    for (int i = 0; i < m_allowedUsersList->count(); ++i) {
        if (m_allowedUsersList->item(i)->text() == user) {
            QMessageBox::information(this, tr("Duplicate User"),
                                   tr("The user '%1' is already in the allowed users list.").arg(user));
            return;
        }
    }
    
    m_allowedUsersList->addItem(user);
    m_newUserEdit->clear();
    updateConfigurationPreview();
}

void ShareCreateDialog::onRemoveAllowedUserClicked()
{
    if (!m_allowedUsersList) {
        return;
    }
    
    QListWidgetItem *currentItem = m_allowedUsersList->currentItem();
    if (currentItem) {
        delete currentItem;
        updateConfigurationPreview();
    }
}

void ShareCreateDialog::onAllowedHostsSelectionChanged()
{
    if (m_removeHostButton) {
        bool hasSelection = m_allowedHostsList && m_allowedHostsList->currentItem() != nullptr;
        m_removeHostButton->setEnabled(hasSelection);
    }
}

void ShareCreateDialog::onAllowedUsersSelectionChanged()
{
    if (m_removeUserButton) {
        bool hasSelection = m_allowedUsersList && m_allowedUsersList->currentItem() != nullptr;
        m_removeUserButton->setEnabled(hasSelection);
    }
}

void ShareCreateDialog::onTestConfigurationClicked()
{
    validateConfiguration();
    
    if (m_isValid) {
        QString exportLine = getExportLinePreview();
        QMessageBox::information(this, tr("Configuration Test"),
                               tr("Configuration is valid!\n\n"
                                  "Generated export line:\n%1").arg(exportLine));
    } else {
        showValidationErrors(m_validationErrors);
    }
}

void ShareCreateDialog::onAdvancedPermissionsClicked()
{
    // Create the permission manager dialog
    PermissionManagerDialog dialog(getPermissions(), this);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Store the detailed permissions
        m_detailedPermissions = dialog.getPermissions();
        
        // Update the basic UI to reflect the changes
        if (m_accessModeCombo) {
            int index = m_accessModeCombo->findData(static_cast<int>(m_detailedPermissions.defaultAccess()));
            if (index >= 0) {
                m_accessModeCombo->setCurrentIndex(index);
            }
        }
        
        if (m_rootSquashCheck) {
            m_rootSquashCheck->setChecked(m_detailedPermissions.enableRootSquash());
        }
        
        if (m_anonymousUserEdit) {
            m_anonymousUserEdit->setText(m_detailedPermissions.anonymousUser());
        }
        
        // Update the configuration preview
        updateConfigurationPreview();
        
        // Update button text to indicate advanced settings are configured
        if (m_advancedPermissionsButton) {
            bool hasAdvanced = !m_detailedPermissions.hostPermissions().isEmpty() || 
                              !m_detailedPermissions.userPermissions().isEmpty();
            if (hasAdvanced) {
                m_advancedPermissionsButton->setText(tr("Advanced Permissions... (Configured)"));
                m_advancedPermissionsButton->setStyleSheet("font-weight: bold; color: blue;");
            } else {
                m_advancedPermissionsButton->setText(tr("Advanced Permissions..."));
                m_advancedPermissionsButton->setStyleSheet("");
            }
        }
        
        validateConfiguration();
    }
}

void ShareCreateDialog::onAdvancedOptionsToggled(bool enabled)
{
    if (m_customOptionsEdit) {
        m_customOptionsEdit->setEnabled(enabled);
    }
    updateConfigurationPreview();
}

void ShareCreateDialog::validateConfiguration()
{
    m_validationErrors.clear();
    m_isValid = true;
    
    // Validate path
    if (!validatePath()) {
        m_isValid = false;
    }
    
    // Validate share configuration
    if (!validateShareConfig()) {
        m_isValid = false;
    }
    
    // Update validation labels
    if (m_pathValidationLabel) {
        QString path = getSharePath();
        if (path.isEmpty()) {
            m_pathValidationLabel->setText(tr("Please select a directory to share"));
            m_pathValidationLabel->setStyleSheet("color: red;");
        } else if (!QDir(path).exists()) {
            m_pathValidationLabel->setText(tr("Directory does not exist"));
            m_pathValidationLabel->setStyleSheet("color: red;");
        } else if (!QFileInfo(path).isReadable()) {
            m_pathValidationLabel->setText(tr("Directory is not accessible"));
            m_pathValidationLabel->setStyleSheet("color: red;");
        } else {
            m_pathValidationLabel->setText(tr("✓ Valid directory"));
            m_pathValidationLabel->setStyleSheet("color: green;");
        }
    }
    
    if (m_shareNameValidationLabel) {
        QString name = m_shareNameEdit ? m_shareNameEdit->text().trimmed() : QString();
        if (name.isEmpty()) {
            m_shareNameValidationLabel->setText(tr("Please enter a share name"));
            m_shareNameValidationLabel->setStyleSheet("color: red;");
        } else {
            m_shareNameValidationLabel->setText(tr("✓ Valid name"));
            m_shareNameValidationLabel->setStyleSheet("color: green;");
        }
    }
    
    // Update button states
    if (m_buttonBox) {
        QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
        if (okButton) {
            okButton->setEnabled(m_isValid);
        }
    }
    
    if (m_testButton) {
        m_testButton->setEnabled(!getSharePath().isEmpty());
    }
    
    // Update status
    if (m_statusLabel) {
        if (m_isValid) {
            m_statusLabel->setText(tr("Configuration is valid"));
            m_statusLabel->setStyleSheet("color: green;");
        } else {
            m_statusLabel->setText(tr("Configuration has errors"));
            m_statusLabel->setStyleSheet("color: red;");
        }
    }
}

void ShareCreateDialog::setupUI()
{
    setWindowTitle(tr("Create NFS Share"));
    setModal(true);
    resize(600, 500);
    setMinimumSize(500, 400);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    setupBasicTab();
    setupPermissionsTab();
    setupAdvancedTab();
    
    // Status label
    m_statusLabel = new QLabel();
    mainLayout->addWidget(m_statusLabel);
    
    // Button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_testButton = new QPushButton(tr("Test Configuration"));
    m_buttonBox->addButton(m_testButton, QDialogButtonBox::ActionRole);
    
    mainLayout->addWidget(m_buttonBox);
    
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ShareCreateDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &ShareCreateDialog::reject);
    connect(m_testButton, &QPushButton::clicked, this, &ShareCreateDialog::onTestConfigurationClicked);
}

void ShareCreateDialog::setupBasicTab()
{
    m_basicTab = new QWidget();
    m_tabWidget->addTab(m_basicTab, tr("Basic Configuration"));
    
    QFormLayout *layout = new QFormLayout(m_basicTab);
    
    // Directory path selection
    QHBoxLayout *pathLayout = new QHBoxLayout();
    m_pathEdit = new QLineEdit();
    m_pathEdit->setPlaceholderText(tr("Select directory to share..."));
    m_browseButton = new QPushButton(tr("Browse..."));
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(m_browseButton);
    
    layout->addRow(tr("Directory Path:"), pathLayout);
    
    m_pathValidationLabel = new QLabel();
    layout->addRow("", m_pathValidationLabel);
    
    // Share name
    m_shareNameEdit = new QLineEdit();
    m_shareNameEdit->setPlaceholderText(tr("Enter a descriptive name for this share"));
    layout->addRow(tr("Share Name:"), m_shareNameEdit);
    
    m_shareNameValidationLabel = new QLabel();
    layout->addRow("", m_shareNameValidationLabel);
    
    // Access mode
    m_accessModeCombo = new QComboBox();
    m_accessModeCombo->addItem(tr("Read Only"), static_cast<int>(AccessMode::ReadOnly));
    m_accessModeCombo->addItem(tr("Read Write"), static_cast<int>(AccessMode::ReadWrite));
    m_accessModeCombo->setCurrentIndex(1); // Default to ReadWrite
    layout->addRow(tr("Access Mode:"), m_accessModeCombo);
    
    // NFS version
    m_nfsVersionCombo = new QComboBox();
    m_nfsVersionCombo->addItem(tr("NFS v3"), static_cast<int>(NFSVersion::Version3));
    m_nfsVersionCombo->addItem(tr("NFS v4"), static_cast<int>(NFSVersion::Version4));
    m_nfsVersionCombo->addItem(tr("NFS v4.1"), static_cast<int>(NFSVersion::Version4_1));
    m_nfsVersionCombo->addItem(tr("NFS v4.2"), static_cast<int>(NFSVersion::Version4_2));
    m_nfsVersionCombo->setCurrentIndex(1); // Default to NFSv4
    layout->addRow(tr("NFS Version:"), m_nfsVersionCombo);
    
    // Root access
    m_allowRootAccessCheck = new QCheckBox(tr("Allow root access (disable root squashing)"));
    m_allowRootAccessCheck->setToolTip(tr("When enabled, root users on client machines will have root privileges on the shared directory"));
    layout->addRow("", m_allowRootAccessCheck);
}

void ShareCreateDialog::setupPermissionsTab()
{
    m_permissionsTab = new QWidget();
    m_tabWidget->addTab(m_permissionsTab, tr("Permissions"));
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_permissionsTab);
    
    // Allowed hosts section
    QGroupBox *hostsGroup = new QGroupBox(tr("Allowed Hosts"));
    QVBoxLayout *hostsLayout = new QVBoxLayout(hostsGroup);
    
    QLabel *hostsLabel = new QLabel(tr("Specify which hosts can access this share:"));
    hostsLayout->addWidget(hostsLabel);
    
    m_allowedHostsList = new QListWidget();
    m_allowedHostsList->setMaximumHeight(100);
    hostsLayout->addWidget(m_allowedHostsList);
    
    QHBoxLayout *hostsButtonLayout = new QHBoxLayout();
    m_newHostEdit = new QLineEdit();
    m_newHostEdit->setPlaceholderText(tr("hostname, IP address, or network range"));
    m_addHostButton = new QPushButton(tr("Add"));
    m_removeHostButton = new QPushButton(tr("Remove"));
    m_removeHostButton->setEnabled(false);
    
    hostsButtonLayout->addWidget(m_newHostEdit);
    hostsButtonLayout->addWidget(m_addHostButton);
    hostsButtonLayout->addWidget(m_removeHostButton);
    hostsLayout->addLayout(hostsButtonLayout);
    
    mainLayout->addWidget(hostsGroup);
    
    // Allowed users section
    QGroupBox *usersGroup = new QGroupBox(tr("Allowed Users"));
    QVBoxLayout *usersLayout = new QVBoxLayout(usersGroup);
    
    QLabel *usersLabel = new QLabel(tr("Specify which users can access this share:"));
    usersLayout->addWidget(usersLabel);
    
    m_allowedUsersList = new QListWidget();
    m_allowedUsersList->setMaximumHeight(100);
    usersLayout->addWidget(m_allowedUsersList);
    
    QHBoxLayout *usersButtonLayout = new QHBoxLayout();
    m_newUserEdit = new QLineEdit();
    m_newUserEdit->setPlaceholderText(tr("username"));
    m_addUserButton = new QPushButton(tr("Add"));
    m_removeUserButton = new QPushButton(tr("Remove"));
    m_removeUserButton->setEnabled(false);
    
    usersButtonLayout->addWidget(m_newUserEdit);
    usersButtonLayout->addWidget(m_addUserButton);
    usersButtonLayout->addWidget(m_removeUserButton);
    usersLayout->addLayout(usersButtonLayout);
    
    mainLayout->addWidget(usersGroup);
    
    // Security options
    QGroupBox *securityGroup = new QGroupBox(tr("Security Options"));
    QFormLayout *securityLayout = new QFormLayout(securityGroup);
    
    m_rootSquashCheck = new QCheckBox(tr("Enable root squashing"));
    m_rootSquashCheck->setChecked(true);
    m_rootSquashCheck->setToolTip(tr("Map root user to anonymous user for security"));
    securityLayout->addRow("", m_rootSquashCheck);
    
    m_anonymousUserEdit = new QLineEdit();
    m_anonymousUserEdit->setText("nobody");
    m_anonymousUserEdit->setToolTip(tr("User to map anonymous and squashed users to"));
    securityLayout->addRow(tr("Anonymous User:"), m_anonymousUserEdit);
    
    // Advanced permissions button
    m_advancedPermissionsButton = new QPushButton(tr("Advanced Permissions..."));
    m_advancedPermissionsButton->setToolTip(tr("Configure detailed host-based and user-based permissions"));
    securityLayout->addRow("", m_advancedPermissionsButton);
    
    mainLayout->addWidget(securityGroup);
    
    mainLayout->addStretch();
}

void ShareCreateDialog::setupAdvancedTab()
{
    m_advancedTab = new QWidget();
    m_tabWidget->addTab(m_advancedTab, tr("Advanced"));
    
    QVBoxLayout *layout = new QVBoxLayout(m_advancedTab);
    
    // Enable advanced options
    m_enableAdvancedCheck = new QCheckBox(tr("Enable advanced options"));
    layout->addWidget(m_enableAdvancedCheck);
    
    // Custom options
    QLabel *optionsLabel = new QLabel(tr("Custom NFS Options:"));
    layout->addWidget(optionsLabel);
    
    m_customOptionsEdit = new QTextEdit();
    m_customOptionsEdit->setMaximumHeight(100);
    m_customOptionsEdit->setEnabled(false);
    m_customOptionsEdit->setPlaceholderText(tr("Enter custom NFS export options (one per line)"));
    layout->addWidget(m_customOptionsEdit);
    
    // Configuration preview
    QLabel *previewLabel = new QLabel(tr("Export Line Preview:"));
    layout->addWidget(previewLabel);
    
    m_configPreviewEdit = new QTextEdit();
    m_configPreviewEdit->setMaximumHeight(100);
    m_configPreviewEdit->setReadOnly(true);
    m_configPreviewEdit->setStyleSheet("background-color: #f0f0f0; font-family: monospace;");
    layout->addWidget(m_configPreviewEdit);
    
    layout->addStretch();
}

void ShareCreateDialog::connectSignals()
{
    if (m_browseButton) {
        connect(m_browseButton, &QPushButton::clicked, this, &ShareCreateDialog::onBrowseClicked);
    }
    
    if (m_pathEdit) {
        connect(m_pathEdit, &QLineEdit::textChanged, this, &ShareCreateDialog::onPathChanged);
    }
    
    if (m_shareNameEdit) {
        connect(m_shareNameEdit, &QLineEdit::textChanged, this, &ShareCreateDialog::onShareNameChanged);
    }
    
    if (m_accessModeCombo) {
        connect(m_accessModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ShareCreateDialog::onAccessModeChanged);
    }
    
    if (m_nfsVersionCombo) {
        connect(m_nfsVersionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ShareCreateDialog::onNFSVersionChanged);
    }
    
    if (m_allowRootAccessCheck) {
        connect(m_allowRootAccessCheck, &QCheckBox::toggled, this, &ShareCreateDialog::onRootAccessChanged);
    }
    
    if (m_addHostButton) {
        connect(m_addHostButton, &QPushButton::clicked, this, &ShareCreateDialog::onAddAllowedHostClicked);
    }
    
    if (m_removeHostButton) {
        connect(m_removeHostButton, &QPushButton::clicked, this, &ShareCreateDialog::onRemoveAllowedHostClicked);
    }
    
    if (m_addUserButton) {
        connect(m_addUserButton, &QPushButton::clicked, this, &ShareCreateDialog::onAddAllowedUserClicked);
    }
    
    if (m_removeUserButton) {
        connect(m_removeUserButton, &QPushButton::clicked, this, &ShareCreateDialog::onRemoveAllowedUserClicked);
    }
    
    if (m_allowedHostsList) {
        connect(m_allowedHostsList, &QListWidget::itemSelectionChanged,
                this, &ShareCreateDialog::onAllowedHostsSelectionChanged);
    }
    
    if (m_allowedUsersList) {
        connect(m_allowedUsersList, &QListWidget::itemSelectionChanged,
                this, &ShareCreateDialog::onAllowedUsersSelectionChanged);
    }
    
    if (m_enableAdvancedCheck) {
        connect(m_enableAdvancedCheck, &QCheckBox::toggled, this, &ShareCreateDialog::onAdvancedOptionsToggled);
    }
    
    if (m_advancedPermissionsButton) {
        connect(m_advancedPermissionsButton, &QPushButton::clicked, this, &ShareCreateDialog::onAdvancedPermissionsClicked);
    }
    
    if (m_newHostEdit) {
        connect(m_newHostEdit, &QLineEdit::returnPressed, this, &ShareCreateDialog::onAddAllowedHostClicked);
    }
    
    if (m_newUserEdit) {
        connect(m_newUserEdit, &QLineEdit::returnPressed, this, &ShareCreateDialog::onAddAllowedUserClicked);
    }
}

void ShareCreateDialog::updateUIState()
{
    // Update UI elements based on current state
    updateConfigurationPreview();
}

bool ShareCreateDialog::validatePath()
{
    QString path = getSharePath();
    
    if (path.isEmpty()) {
        m_validationErrors.append(tr("Please select a directory to share"));
        return false;
    }
    
    QDir dir(path);
    if (!dir.exists()) {
        m_validationErrors.append(tr("The selected directory does not exist: %1").arg(path));
        return false;
    }
    
    QFileInfo info(path);
    if (!info.isReadable()) {
        m_validationErrors.append(tr("The selected directory is not accessible: %1").arg(path));
        return false;
    }
    
    if (!info.isDir()) {
        m_validationErrors.append(tr("The selected path is not a directory: %1").arg(path));
        return false;
    }
    
    // Check with ShareManager if available
    if (m_shareManager) {
        QStringList pathErrors = m_shareManager->getPathValidationErrors(path);
        if (!pathErrors.isEmpty()) {
            m_validationErrors.append(pathErrors);
            return false;
        }
    }
    
    return true;
}

bool ShareCreateDialog::validateShareConfig()
{
    QString shareName = m_shareNameEdit ? m_shareNameEdit->text().trimmed() : QString();
    if (shareName.isEmpty()) {
        m_validationErrors.append(tr("Please enter a share name"));
        return false;
    }
    
    // Validate share configuration
    ShareConfiguration config = getShareConfiguration();
    if (!config.isValid()) {
        QStringList configErrors = config.validationErrors();
        m_validationErrors.append(configErrors);
        return false;
    }
    
    // Validate permissions
    PermissionSet permissions = getPermissions();
    if (!permissions.isValid()) {
        QStringList permissionErrors = permissions.validationErrors();
        m_validationErrors.append(permissionErrors);
        return false;
    }
    
    return true;
}

void ShareCreateDialog::showValidationErrors(const QStringList &errors)
{
    if (errors.isEmpty()) {
        return;
    }
    
    QString message = tr("Please fix the following issues:\n\n");
    for (const QString &error : errors) {
        message += "• " + error + "\n";
    }
    
    QMessageBox::warning(this, tr("Configuration Errors"), message);
}

void ShareCreateDialog::updateConfigurationPreview()
{
    if (!m_configPreviewEdit) {
        return;
    }
    
    QString exportLine = getExportLinePreview();
    m_configPreviewEdit->setPlainText(exportLine);
}

QString ShareCreateDialog::getExportLinePreview() const
{
    QString path = getSharePath();
    if (path.isEmpty()) {
        return tr("# Select a directory to see the export line preview");
    }
    
    ShareConfiguration config = getShareConfiguration();
    PermissionSet permissions = getPermissions();
    
    // Build export line with path and permissions
    QString exportLine = path + " ";
    
    // Add host specifications or default to all hosts
    const auto &hostPerms = permissions.hostPermissions();
    if (hostPerms.isEmpty()) {
        exportLine += "*";
    } else {
        QStringList hostSpecs;
        for (auto it = hostPerms.begin(); it != hostPerms.end(); ++it) {
            hostSpecs.append(it.key());
        }
        exportLine += hostSpecs.join(" ");
    }
    
    // Add options
    exportLine += "(" + permissions.toExportOptions() + ")";
    
    return exportLine;
}

} // namespace NFSShareManager