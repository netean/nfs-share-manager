#include "permissionmanagerdialog.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QInputDialog>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>

namespace NFSShareManager {

PermissionManagerDialog::PermissionManagerDialog(QWidget *parent)
    : QDialog(parent)
    , m_permissions()
    , m_isValid(false)
    , m_tabWidget(nullptr)
    , m_buttonBox(nullptr)
    , m_testButton(nullptr)
    , m_resetButton(nullptr)
    , m_importButton(nullptr)
    , m_exportButton(nullptr)
    , m_basicTab(nullptr)
    , m_defaultAccessCombo(nullptr)
    , m_rootSquashCheck(nullptr)
    , m_anonymousUserEdit(nullptr)
    , m_anonymousUserValidation(nullptr)
    , m_presetsCombo(nullptr)
    , m_exportPreviewEdit(nullptr)
    , m_hostTab(nullptr)
    , m_hostTable(nullptr)
    , m_newHostEdit(nullptr)
    , m_newHostAccessCombo(nullptr)
    , m_addHostButton(nullptr)
    , m_removeHostButton(nullptr)
    , m_hostValidationLabel(nullptr)
    , m_hostCompleter(nullptr)
    , m_userTab(nullptr)
    , m_userTable(nullptr)
    , m_newUserEdit(nullptr)
    , m_newUserAccessCombo(nullptr)
    , m_addUserButton(nullptr)
    , m_removeUserButton(nullptr)
    , m_userValidationLabel(nullptr)
    , m_userCompleter(nullptr)
    , m_advancedTab(nullptr)
    , m_advancedOptionsEdit(nullptr)
    , m_advancedValidationLabel(nullptr)
    , m_fullExportPreviewEdit(nullptr)
    , m_statusLabel(nullptr)
    , m_validationProgress(nullptr)
    , m_validationTimer(nullptr)
    , m_hostValidator(nullptr)
    , m_userValidator(nullptr)
    , m_hostSuggestionsModel(nullptr)
    , m_userSuggestionsModel(nullptr)
{
    setupUI();
    connectSignals();
    loadPresets();
    updateUIFromPermissions();
    
    // Start validation timer
    m_validationTimer = new QTimer(this);
    m_validationTimer->setSingleShot(true);
    m_validationTimer->setInterval(500); // 500ms delay for real-time validation
    connect(m_validationTimer, &QTimer::timeout, this, &PermissionManagerDialog::onValidationTimer);
    
    validateConfiguration();
}

PermissionManagerDialog::PermissionManagerDialog(const PermissionSet &permissions, QWidget *parent)
    : PermissionManagerDialog(parent)
{
    setPermissions(permissions);
}

PermissionManagerDialog::~PermissionManagerDialog()
{
}

PermissionSet PermissionManagerDialog::getPermissions() const
{
    return m_permissions;
}

void PermissionManagerDialog::setPermissions(const PermissionSet &permissions)
{
    m_permissions = permissions;
    updateUIFromPermissions();
    validateConfiguration();
}

bool PermissionManagerDialog::isValid() const
{
    return m_isValid;
}

QStringList PermissionManagerDialog::getValidationErrors() const
{
    return m_validationErrors;
}

void PermissionManagerDialog::accept()
{
    updatePermissionsFromUI();
    validateConfiguration();
    
    if (!m_isValid) {
        QString message = tr("Please fix the following issues before continuing:\n\n");
        for (const QString &error : m_validationErrors) {
            message += "• " + error + "\n";
        }
        
        QMessageBox::warning(this, tr("Configuration Errors"), message);
        return;
    }
    
    QDialog::accept();
}

void PermissionManagerDialog::resetToDefaults()
{
    int result = QMessageBox::question(this, tr("Reset to Defaults"),
                                     tr("This will reset all permission settings to their default values.\n\n"
                                        "Are you sure you want to continue?"),
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        m_permissions = PermissionSet();
        updateUIFromPermissions();
        validateConfiguration();
    }
}

void PermissionManagerDialog::importFromExportString()
{
    bool ok;
    QString exportString = QInputDialog::getMultiLineText(
        this,
        tr("Import from Export String"),
        tr("Enter the NFS export options string:"),
        QString(),
        &ok
    );
    
    if (ok && !exportString.isEmpty()) {
        PermissionSet newPermissions;
        if (newPermissions.fromExportOptions(exportString)) {
            m_permissions = newPermissions;
            updateUIFromPermissions();
            validateConfiguration();
            QMessageBox::information(this, tr("Import Successful"),
                                   tr("Permissions have been imported successfully."));
        } else {
            QMessageBox::warning(this, tr("Import Failed"),
                               tr("Failed to parse the export options string.\n\n"
                                  "Please check the format and try again."));
        }
    }
}

void PermissionManagerDialog::exportToExportString()
{
    updatePermissionsFromUI();
    QString exportString = m_permissions.toExportOptions();
    
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(exportString);
    
    QMessageBox::information(this, tr("Export Successful"),
                           tr("Export options have been copied to the clipboard:\n\n%1")
                           .arg(exportString));
}

void PermissionManagerDialog::onDefaultAccessChanged()
{
    if (m_validationTimer) {
        m_validationTimer->start();
    }
    updateExportPreview();
}

void PermissionManagerDialog::onRootSquashChanged()
{
    if (m_validationTimer) {
        m_validationTimer->start();
    }
    updateExportPreview();
}

void PermissionManagerDialog::onAnonymousUserChanged()
{
    if (m_validationTimer) {
        m_validationTimer->start();
    }
    
    // Real-time validation for anonymous user
    if (m_anonymousUserEdit && m_anonymousUserValidation) {
        QString user = m_anonymousUserEdit->text().trimmed();
        auto validation = validateUserSpecification(user);
        
        if (validation.first) {
            m_anonymousUserValidation->setText(tr("✓ Valid username"));
            m_anonymousUserValidation->setStyleSheet("color: green;");
        } else {
            m_anonymousUserValidation->setText(validation.second);
            m_anonymousUserValidation->setStyleSheet("color: red;");
        }
    }
    
    updateExportPreview();
}

void PermissionManagerDialog::onHostPermissionChanged()
{
    if (m_validationTimer) {
        m_validationTimer->start();
    }
    updateExportPreview();
}

void PermissionManagerDialog::onUserPermissionChanged()
{
    if (m_validationTimer) {
        m_validationTimer->start();
    }
    updateExportPreview();
}

void PermissionManagerDialog::onAddHostPermission()
{
    if (!m_newHostEdit || !m_newHostAccessCombo || !m_hostTable) {
        return;
    }
    
    QString host = m_newHostEdit->text().trimmed();
    if (host.isEmpty()) {
        return;
    }
    
    // Validate host specification
    auto validation = validateHostSpecification(host);
    if (!validation.first) {
        QMessageBox::warning(this, tr("Invalid Host"), validation.second);
        return;
    }
    
    // Check for duplicates
    for (int row = 0; row < m_hostTable->rowCount(); ++row) {
        QTableWidgetItem *item = m_hostTable->item(row, 0);
        if (item && item->text() == host) {
            QMessageBox::information(this, tr("Duplicate Host"),
                                   tr("The host '%1' is already in the permissions list.").arg(host));
            return;
        }
    }
    
    // Add to table
    int row = m_hostTable->rowCount();
    m_hostTable->insertRow(row);
    
    QTableWidgetItem *hostItem = new QTableWidgetItem(host);
    m_hostTable->setItem(row, 0, hostItem);
    
    QComboBox *accessCombo = new QComboBox();
    accessCombo->addItem(tr("No Access"), static_cast<int>(AccessMode::NoAccess));
    accessCombo->addItem(tr("Read Only"), static_cast<int>(AccessMode::ReadOnly));
    accessCombo->addItem(tr("Read Write"), static_cast<int>(AccessMode::ReadWrite));
    accessCombo->setCurrentIndex(m_newHostAccessCombo->currentIndex());
    
    connect(accessCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PermissionManagerDialog::onHostPermissionChanged);
    
    m_hostTable->setCellWidget(row, 1, accessCombo);
    
    // Clear input
    m_newHostEdit->clear();
    
    onHostPermissionChanged();
}

void PermissionManagerDialog::onRemoveHostPermission()
{
    if (!m_hostTable) {
        return;
    }
    
    int currentRow = m_hostTable->currentRow();
    if (currentRow >= 0) {
        m_hostTable->removeRow(currentRow);
        onHostPermissionChanged();
    }
}

void PermissionManagerDialog::onAddUserPermission()
{
    if (!m_newUserEdit || !m_newUserAccessCombo || !m_userTable) {
        return;
    }
    
    QString user = m_newUserEdit->text().trimmed();
    if (user.isEmpty()) {
        return;
    }
    
    // Validate user specification
    auto validation = validateUserSpecification(user);
    if (!validation.first) {
        QMessageBox::warning(this, tr("Invalid User"), validation.second);
        return;
    }
    
    // Check for duplicates
    for (int row = 0; row < m_userTable->rowCount(); ++row) {
        QTableWidgetItem *item = m_userTable->item(row, 0);
        if (item && item->text() == user) {
            QMessageBox::information(this, tr("Duplicate User"),
                                   tr("The user '%1' is already in the permissions list.").arg(user));
            return;
        }
    }
    
    // Add to table
    int row = m_userTable->rowCount();
    m_userTable->insertRow(row);
    
    QTableWidgetItem *userItem = new QTableWidgetItem(user);
    m_userTable->setItem(row, 0, userItem);
    
    QComboBox *accessCombo = new QComboBox();
    accessCombo->addItem(tr("No Access"), static_cast<int>(AccessMode::NoAccess));
    accessCombo->addItem(tr("Read Only"), static_cast<int>(AccessMode::ReadOnly));
    accessCombo->addItem(tr("Read Write"), static_cast<int>(AccessMode::ReadWrite));
    accessCombo->setCurrentIndex(m_newUserAccessCombo->currentIndex());
    
    connect(accessCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PermissionManagerDialog::onUserPermissionChanged);
    
    m_userTable->setCellWidget(row, 1, accessCombo);
    
    // Clear input
    m_newUserEdit->clear();
    
    onUserPermissionChanged();
}

void PermissionManagerDialog::onRemoveUserPermission()
{
    if (!m_userTable) {
        return;
    }
    
    int currentRow = m_userTable->currentRow();
    if (currentRow >= 0) {
        m_userTable->removeRow(currentRow);
        onUserPermissionChanged();
    }
}

void PermissionManagerDialog::onHostTableSelectionChanged()
{
    if (m_removeHostButton) {
        bool hasSelection = m_hostTable && m_hostTable->currentRow() >= 0;
        m_removeHostButton->setEnabled(hasSelection);
    }
}

void PermissionManagerDialog::onUserTableSelectionChanged()
{
    if (m_removeUserButton) {
        bool hasSelection = m_userTable && m_userTable->currentRow() >= 0;
        m_removeUserButton->setEnabled(hasSelection);
    }
}

void PermissionManagerDialog::onAdvancedOptionsChanged()
{
    if (m_validationTimer) {
        m_validationTimer->start();
    }
    
    // Real-time validation for advanced options
    if (m_advancedOptionsEdit && m_advancedValidationLabel) {
        QString options = m_advancedOptionsEdit->toPlainText().trimmed();
        
        if (options.isEmpty()) {
            m_advancedValidationLabel->setText(tr("No custom options"));
            m_advancedValidationLabel->setStyleSheet("color: gray;");
        } else {
            // Basic syntax validation
            QStringList optionList = options.split('\n', Qt::SkipEmptyParts);
            bool allValid = true;
            QString errorMsg;
            
            for (const QString &option : optionList) {
                QString trimmed = option.trimmed();
                if (trimmed.isEmpty()) continue;
                
                // Basic NFS option validation
                if (!trimmed.contains('=') && !QRegularExpression("^[a-zA-Z_][a-zA-Z0-9_]*$").match(trimmed).hasMatch()) {
                    allValid = false;
                    errorMsg = tr("Invalid option format: %1").arg(trimmed);
                    break;
                }
            }
            
            if (allValid) {
                m_advancedValidationLabel->setText(tr("✓ Valid options"));
                m_advancedValidationLabel->setStyleSheet("color: green;");
            } else {
                m_advancedValidationLabel->setText(errorMsg);
                m_advancedValidationLabel->setStyleSheet("color: red;");
            }
        }
    }
    
    updateExportPreview();
}

void PermissionManagerDialog::onPresetChanged()
{
    if (!m_presetsCombo) {
        return;
    }
    
    QString presetName = m_presetsCombo->currentText();
    if (presetName != tr("Custom") && m_presets.contains(presetName)) {
        applyPreset(presetName);
    }
}

void PermissionManagerDialog::onTestConfiguration()
{
    updatePermissionsFromUI();
    validateConfiguration();
    
    if (m_isValid) {
        QString exportOptions = m_permissions.toExportOptions();
        QMessageBox::information(this, tr("Configuration Test"),
                               tr("✓ Configuration is valid!\n\n"
                                  "Generated export options:\n%1").arg(exportOptions));
    } else {
        QString message = tr("Configuration has the following issues:\n\n");
        for (const QString &error : m_validationErrors) {
            message += "• " + error + "\n";
        }
        QMessageBox::warning(this, tr("Configuration Test"), message);
    }
}

void PermissionManagerDialog::onValidationTimer()
{
    validateConfiguration();
}

void PermissionManagerDialog::onHostInputChanged()
{
    if (!m_newHostEdit || !m_hostValidationLabel) {
        return;
    }
    
    QString host = m_newHostEdit->text().trimmed();
    if (host.isEmpty()) {
        m_hostValidationLabel->setText(tr("Enter hostname, IP, or network"));
        m_hostValidationLabel->setStyleSheet("color: gray;");
        if (m_addHostButton) {
            m_addHostButton->setEnabled(false);
        }
        return;
    }
    
    auto validation = validateHostSpecification(host);
    if (validation.first) {
        m_hostValidationLabel->setText(tr("✓ Valid host specification"));
        m_hostValidationLabel->setStyleSheet("color: green;");
        if (m_addHostButton) {
            m_addHostButton->setEnabled(true);
        }
    } else {
        m_hostValidationLabel->setText(validation.second);
        m_hostValidationLabel->setStyleSheet("color: red;");
        if (m_addHostButton) {
            m_addHostButton->setEnabled(false);
        }
    }
}

void PermissionManagerDialog::onUserInputChanged()
{
    if (!m_newUserEdit || !m_userValidationLabel) {
        return;
    }
    
    QString user = m_newUserEdit->text().trimmed();
    if (user.isEmpty()) {
        m_userValidationLabel->setText(tr("Enter username"));
        m_userValidationLabel->setStyleSheet("color: gray;");
        if (m_addUserButton) {
            m_addUserButton->setEnabled(false);
        }
        return;
    }
    
    auto validation = validateUserSpecification(user);
    if (validation.first) {
        m_userValidationLabel->setText(tr("✓ Valid username"));
        m_userValidationLabel->setStyleSheet("color: green;");
        if (m_addUserButton) {
            m_addUserButton->setEnabled(true);
        }
    } else {
        m_userValidationLabel->setText(validation.second);
        m_userValidationLabel->setStyleSheet("color: red;");
        if (m_addUserButton) {
            m_addUserButton->setEnabled(false);
        }
    }
}

void PermissionManagerDialog::setupUI()
{
    setWindowTitle(tr("Permission Manager"));
    setModal(true);
    resize(800, 600);
    setMinimumSize(600, 500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    setupBasicTab();
    setupHostPermissionsTab();
    setupUserPermissionsTab();
    setupAdvancedTab();
    
    // Status label
    m_statusLabel = new QLabel();
    mainLayout->addWidget(m_statusLabel);
    
    // Button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    
    m_testButton = new QPushButton(tr("Test Configuration"));
    m_resetButton = new QPushButton(tr("Reset to Defaults"));
    m_importButton = new QPushButton(tr("Import..."));
    m_exportButton = new QPushButton(tr("Export..."));
    
    m_buttonBox->addButton(m_testButton, QDialogButtonBox::ActionRole);
    m_buttonBox->addButton(m_resetButton, QDialogButtonBox::ResetRole);
    m_buttonBox->addButton(m_importButton, QDialogButtonBox::ActionRole);
    m_buttonBox->addButton(m_exportButton, QDialogButtonBox::ActionRole);
    
    mainLayout->addWidget(m_buttonBox);
}

void PermissionManagerDialog::setupBasicTab()
{
    m_basicTab = new QWidget();
    m_tabWidget->addTab(m_basicTab, tr("Basic Permissions"));
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_basicTab);
    
    // Presets section
    QGroupBox *presetsGroup = new QGroupBox(tr("Permission Presets"));
    QFormLayout *presetsLayout = new QFormLayout(presetsGroup);
    
    m_presetsCombo = new QComboBox();
    presetsLayout->addRow(tr("Apply Preset:"), m_presetsCombo);
    
    mainLayout->addWidget(presetsGroup);
    
    // Basic settings section
    QGroupBox *basicGroup = new QGroupBox(tr("Default Access Settings"));
    QFormLayout *basicLayout = new QFormLayout(basicGroup);
    
    m_defaultAccessCombo = new QComboBox();
    m_defaultAccessCombo->addItem(tr("No Access"), static_cast<int>(AccessMode::NoAccess));
    m_defaultAccessCombo->addItem(tr("Read Only"), static_cast<int>(AccessMode::ReadOnly));
    m_defaultAccessCombo->addItem(tr("Read Write"), static_cast<int>(AccessMode::ReadWrite));
    basicLayout->addRow(tr("Default Access:"), m_defaultAccessCombo);
    
    m_rootSquashCheck = new QCheckBox(tr("Enable root squashing (recommended)"));
    m_rootSquashCheck->setToolTip(tr("Maps root user to anonymous user for security"));
    basicLayout->addRow("", m_rootSquashCheck);
    
    QHBoxLayout *anonUserLayout = new QHBoxLayout();
    m_anonymousUserEdit = new QLineEdit();
    m_anonymousUserEdit->setPlaceholderText(tr("nobody"));
    m_anonymousUserValidation = new QLabel();
    anonUserLayout->addWidget(m_anonymousUserEdit);
    anonUserLayout->addWidget(m_anonymousUserValidation);
    basicLayout->addRow(tr("Anonymous User:"), anonUserLayout);
    
    mainLayout->addWidget(basicGroup);
    
    // Export preview section
    QGroupBox *previewGroup = new QGroupBox(tr("Export Options Preview"));
    QVBoxLayout *previewLayout = new QVBoxLayout(previewGroup);
    
    m_exportPreviewEdit = new QTextEdit();
    m_exportPreviewEdit->setMaximumHeight(80);
    m_exportPreviewEdit->setReadOnly(true);
    m_exportPreviewEdit->setStyleSheet("background-color: #f8f8f8; font-family: monospace;");
    previewLayout->addWidget(m_exportPreviewEdit);
    
    mainLayout->addWidget(previewGroup);
    
    mainLayout->addStretch();
}

void PermissionManagerDialog::setupHostPermissionsTab()
{
    m_hostTab = new QWidget();
    m_tabWidget->addTab(m_hostTab, tr("Host Permissions"));
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_hostTab);
    
    // Instructions
    QLabel *instructionsLabel = new QLabel(tr("Configure specific access permissions for hosts, IP addresses, or network ranges:"));
    instructionsLabel->setWordWrap(true);
    mainLayout->addWidget(instructionsLabel);
    
    // Host permissions table
    m_hostTable = new QTableWidget(0, 2);
    m_hostTable->setHorizontalHeaderLabels({tr("Host/Network"), tr("Access Level")});
    m_hostTable->horizontalHeader()->setStretchLastSection(true);
    m_hostTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_hostTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_hostTable);
    
    // Add host controls
    QGroupBox *addHostGroup = new QGroupBox(tr("Add Host Permission"));
    QGridLayout *addHostLayout = new QGridLayout(addHostGroup);
    
    QLabel *hostLabel = new QLabel(tr("Host/Network:"));
    m_newHostEdit = new QLineEdit();
    m_newHostEdit->setPlaceholderText(tr("hostname.com, 192.168.1.100, 192.168.1.0/24, *.example.com"));
    
    QLabel *accessLabel = new QLabel(tr("Access Level:"));
    m_newHostAccessCombo = new QComboBox();
    m_newHostAccessCombo->addItem(tr("No Access"), static_cast<int>(AccessMode::NoAccess));
    m_newHostAccessCombo->addItem(tr("Read Only"), static_cast<int>(AccessMode::ReadOnly));
    m_newHostAccessCombo->addItem(tr("Read Write"), static_cast<int>(AccessMode::ReadWrite));
    m_newHostAccessCombo->setCurrentIndex(1); // Default to Read Only
    
    m_addHostButton = new QPushButton(tr("Add"));
    m_addHostButton->setEnabled(false);
    m_removeHostButton = new QPushButton(tr("Remove Selected"));
    m_removeHostButton->setEnabled(false);
    
    m_hostValidationLabel = new QLabel(tr("Enter hostname, IP, or network"));
    m_hostValidationLabel->setStyleSheet("color: gray;");
    
    addHostLayout->addWidget(hostLabel, 0, 0);
    addHostLayout->addWidget(m_newHostEdit, 0, 1);
    addHostLayout->addWidget(accessLabel, 1, 0);
    addHostLayout->addWidget(m_newHostAccessCombo, 1, 1);
    addHostLayout->addWidget(m_addHostButton, 0, 2);
    addHostLayout->addWidget(m_removeHostButton, 1, 2);
    addHostLayout->addWidget(m_hostValidationLabel, 2, 0, 1, 3);
    
    mainLayout->addWidget(addHostGroup);
    
    // Setup host input validation and auto-completion
    setupHostValidation();
}

void PermissionManagerDialog::setupUserPermissionsTab()
{
    m_userTab = new QWidget();
    m_tabWidget->addTab(m_userTab, tr("User Permissions"));
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_userTab);
    
    // Instructions
    QLabel *instructionsLabel = new QLabel(tr("Configure specific access permissions for individual users:"));
    instructionsLabel->setWordWrap(true);
    mainLayout->addWidget(instructionsLabel);
    
    // User permissions table
    m_userTable = new QTableWidget(0, 2);
    m_userTable->setHorizontalHeaderLabels({tr("Username"), tr("Access Level")});
    m_userTable->horizontalHeader()->setStretchLastSection(true);
    m_userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_userTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_userTable);
    
    // Add user controls
    QGroupBox *addUserGroup = new QGroupBox(tr("Add User Permission"));
    QGridLayout *addUserLayout = new QGridLayout(addUserGroup);
    
    QLabel *userLabel = new QLabel(tr("Username:"));
    m_newUserEdit = new QLineEdit();
    m_newUserEdit->setPlaceholderText(tr("username"));
    
    QLabel *accessLabel = new QLabel(tr("Access Level:"));
    m_newUserAccessCombo = new QComboBox();
    m_newUserAccessCombo->addItem(tr("No Access"), static_cast<int>(AccessMode::NoAccess));
    m_newUserAccessCombo->addItem(tr("Read Only"), static_cast<int>(AccessMode::ReadOnly));
    m_newUserAccessCombo->addItem(tr("Read Write"), static_cast<int>(AccessMode::ReadWrite));
    m_newUserAccessCombo->setCurrentIndex(1); // Default to Read Only
    
    m_addUserButton = new QPushButton(tr("Add"));
    m_addUserButton->setEnabled(false);
    m_removeUserButton = new QPushButton(tr("Remove Selected"));
    m_removeUserButton->setEnabled(false);
    
    m_userValidationLabel = new QLabel(tr("Enter username"));
    m_userValidationLabel->setStyleSheet("color: gray;");
    
    addUserLayout->addWidget(userLabel, 0, 0);
    addUserLayout->addWidget(m_newUserEdit, 0, 1);
    addUserLayout->addWidget(accessLabel, 1, 0);
    addUserLayout->addWidget(m_newUserAccessCombo, 1, 1);
    addUserLayout->addWidget(m_addUserButton, 0, 2);
    addUserLayout->addWidget(m_removeUserButton, 1, 2);
    addUserLayout->addWidget(m_userValidationLabel, 2, 0, 1, 3);
    
    mainLayout->addWidget(addUserGroup);
    
    // Setup user input validation and auto-completion
    setupUserValidation();
}

void PermissionManagerDialog::setupAdvancedTab()
{
    m_advancedTab = new QWidget();
    m_tabWidget->addTab(m_advancedTab, tr("Advanced Options"));
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_advancedTab);
    
    // Custom options section
    QGroupBox *optionsGroup = new QGroupBox(tr("Custom NFS Export Options"));
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroup);
    
    QLabel *optionsLabel = new QLabel(tr("Enter additional NFS export options (one per line):"));
    optionsLayout->addWidget(optionsLabel);
    
    m_advancedOptionsEdit = new QTextEdit();
    m_advancedOptionsEdit->setMaximumHeight(120);
    m_advancedOptionsEdit->setPlaceholderText(tr("sync\nno_subtree_check\nfsid=1"));
    optionsLayout->addWidget(m_advancedOptionsEdit);
    
    m_advancedValidationLabel = new QLabel(tr("No custom options"));
    m_advancedValidationLabel->setStyleSheet("color: gray;");
    optionsLayout->addWidget(m_advancedValidationLabel);
    
    mainLayout->addWidget(optionsGroup);
    
    // Full export preview section
    QGroupBox *fullPreviewGroup = new QGroupBox(tr("Complete Export Configuration"));
    QVBoxLayout *fullPreviewLayout = new QVBoxLayout(fullPreviewGroup);
    
    QLabel *previewLabel = new QLabel(tr("Complete export line with all options:"));
    fullPreviewLayout->addWidget(previewLabel);
    
    m_fullExportPreviewEdit = new QTextEdit();
    m_fullExportPreviewEdit->setMaximumHeight(100);
    m_fullExportPreviewEdit->setReadOnly(true);
    m_fullExportPreviewEdit->setStyleSheet("background-color: #f0f0f0; font-family: monospace;");
    fullPreviewLayout->addWidget(m_fullExportPreviewEdit);
    
    mainLayout->addWidget(fullPreviewGroup);
    
    mainLayout->addStretch();
}

void PermissionManagerDialog::connectSignals()
{
    // Basic tab signals
    if (m_defaultAccessCombo) {
        connect(m_defaultAccessCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &PermissionManagerDialog::onDefaultAccessChanged);
    }
    
    if (m_rootSquashCheck) {
        connect(m_rootSquashCheck, &QCheckBox::toggled, this, &PermissionManagerDialog::onRootSquashChanged);
    }
    
    if (m_anonymousUserEdit) {
        connect(m_anonymousUserEdit, &QLineEdit::textChanged, this, &PermissionManagerDialog::onAnonymousUserChanged);
    }
    
    if (m_presetsCombo) {
        connect(m_presetsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &PermissionManagerDialog::onPresetChanged);
    }
    
    // Host tab signals
    if (m_newHostEdit) {
        connect(m_newHostEdit, &QLineEdit::textChanged, this, &PermissionManagerDialog::onHostInputChanged);
        connect(m_newHostEdit, &QLineEdit::returnPressed, this, &PermissionManagerDialog::onAddHostPermission);
    }
    
    if (m_addHostButton) {
        connect(m_addHostButton, &QPushButton::clicked, this, &PermissionManagerDialog::onAddHostPermission);
    }
    
    if (m_removeHostButton) {
        connect(m_removeHostButton, &QPushButton::clicked, this, &PermissionManagerDialog::onRemoveHostPermission);
    }
    
    if (m_hostTable) {
        connect(m_hostTable, &QTableWidget::itemSelectionChanged,
                this, &PermissionManagerDialog::onHostTableSelectionChanged);
    }
    
    // User tab signals
    if (m_newUserEdit) {
        connect(m_newUserEdit, &QLineEdit::textChanged, this, &PermissionManagerDialog::onUserInputChanged);
        connect(m_newUserEdit, &QLineEdit::returnPressed, this, &PermissionManagerDialog::onAddUserPermission);
    }
    
    if (m_addUserButton) {
        connect(m_addUserButton, &QPushButton::clicked, this, &PermissionManagerDialog::onAddUserPermission);
    }
    
    if (m_removeUserButton) {
        connect(m_removeUserButton, &QPushButton::clicked, this, &PermissionManagerDialog::onRemoveUserPermission);
    }
    
    if (m_userTable) {
        connect(m_userTable, &QTableWidget::itemSelectionChanged,
                this, &PermissionManagerDialog::onUserTableSelectionChanged);
    }
    
    // Advanced tab signals
    if (m_advancedOptionsEdit) {
        connect(m_advancedOptionsEdit, &QTextEdit::textChanged, this, &PermissionManagerDialog::onAdvancedOptionsChanged);
    }
    
    // Button signals
    if (m_buttonBox) {
        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &PermissionManagerDialog::accept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &PermissionManagerDialog::reject);
    }
    
    if (m_testButton) {
        connect(m_testButton, &QPushButton::clicked, this, &PermissionManagerDialog::onTestConfiguration);
    }
    
    if (m_resetButton) {
        connect(m_resetButton, &QPushButton::clicked, this, &PermissionManagerDialog::resetToDefaults);
    }
    
    if (m_importButton) {
        connect(m_importButton, &QPushButton::clicked, this, &PermissionManagerDialog::importFromExportString);
    }
    
    if (m_exportButton) {
        connect(m_exportButton, &QPushButton::clicked, this, &PermissionManagerDialog::exportToExportString);
    }
}

void PermissionManagerDialog::updateUIFromPermissions()
{
    // Update basic settings
    if (m_defaultAccessCombo) {
        int index = m_defaultAccessCombo->findData(static_cast<int>(m_permissions.defaultAccess()));
        if (index >= 0) {
            m_defaultAccessCombo->setCurrentIndex(index);
        }
    }
    
    if (m_rootSquashCheck) {
        m_rootSquashCheck->setChecked(m_permissions.enableRootSquash());
    }
    
    if (m_anonymousUserEdit) {
        m_anonymousUserEdit->setText(m_permissions.anonymousUser());
    }
    
    // Update host permissions table
    if (m_hostTable) {
        m_hostTable->setRowCount(0);
        const auto &hostPerms = m_permissions.hostPermissions();
        for (auto it = hostPerms.begin(); it != hostPerms.end(); ++it) {
            int row = m_hostTable->rowCount();
            m_hostTable->insertRow(row);
            
            QTableWidgetItem *hostItem = new QTableWidgetItem(it.key());
            m_hostTable->setItem(row, 0, hostItem);
            
            QComboBox *accessCombo = new QComboBox();
            accessCombo->addItem(tr("No Access"), static_cast<int>(AccessMode::NoAccess));
            accessCombo->addItem(tr("Read Only"), static_cast<int>(AccessMode::ReadOnly));
            accessCombo->addItem(tr("Read Write"), static_cast<int>(AccessMode::ReadWrite));
            
            int accessIndex = accessCombo->findData(static_cast<int>(it.value()));
            if (accessIndex >= 0) {
                accessCombo->setCurrentIndex(accessIndex);
            }
            
            connect(accessCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &PermissionManagerDialog::onHostPermissionChanged);
            
            m_hostTable->setCellWidget(row, 1, accessCombo);
        }
    }
    
    // Update user permissions table
    if (m_userTable) {
        m_userTable->setRowCount(0);
        const auto &userPerms = m_permissions.userPermissions();
        for (auto it = userPerms.begin(); it != userPerms.end(); ++it) {
            int row = m_userTable->rowCount();
            m_userTable->insertRow(row);
            
            QTableWidgetItem *userItem = new QTableWidgetItem(it.key());
            m_userTable->setItem(row, 0, userItem);
            
            QComboBox *accessCombo = new QComboBox();
            accessCombo->addItem(tr("No Access"), static_cast<int>(AccessMode::NoAccess));
            accessCombo->addItem(tr("Read Only"), static_cast<int>(AccessMode::ReadOnly));
            accessCombo->addItem(tr("Read Write"), static_cast<int>(AccessMode::ReadWrite));
            
            int accessIndex = accessCombo->findData(static_cast<int>(it.value()));
            if (accessIndex >= 0) {
                accessCombo->setCurrentIndex(accessIndex);
            }
            
            connect(accessCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &PermissionManagerDialog::onUserPermissionChanged);
            
            m_userTable->setCellWidget(row, 1, accessCombo);
        }
    }
    
    updateExportPreview();
}

void PermissionManagerDialog::updatePermissionsFromUI()
{
    // Update basic settings
    if (m_defaultAccessCombo) {
        AccessMode mode = static_cast<AccessMode>(m_defaultAccessCombo->currentData().toInt());
        m_permissions.setDefaultAccess(mode);
    }
    
    if (m_rootSquashCheck) {
        m_permissions.setEnableRootSquash(m_rootSquashCheck->isChecked());
    }
    
    if (m_anonymousUserEdit) {
        m_permissions.setAnonymousUser(m_anonymousUserEdit->text().trimmed());
    }
    
    // Update host permissions
    if (m_hostTable) {
        // Clear existing host permissions
        const auto &currentHosts = m_permissions.hostPermissions();
        for (auto it = currentHosts.begin(); it != currentHosts.end(); ++it) {
            m_permissions.removeHostPermission(it.key());
        }
        
        // Add permissions from table
        for (int row = 0; row < m_hostTable->rowCount(); ++row) {
            QTableWidgetItem *hostItem = m_hostTable->item(row, 0);
            QComboBox *accessCombo = qobject_cast<QComboBox*>(m_hostTable->cellWidget(row, 1));
            
            if (hostItem && accessCombo) {
                QString host = hostItem->text();
                AccessMode mode = static_cast<AccessMode>(accessCombo->currentData().toInt());
                m_permissions.setHostPermission(host, mode);
            }
        }
    }
    
    // Update user permissions
    if (m_userTable) {
        // Clear existing user permissions
        const auto &currentUsers = m_permissions.userPermissions();
        for (auto it = currentUsers.begin(); it != currentUsers.end(); ++it) {
            m_permissions.removeUserPermission(it.key());
        }
        
        // Add permissions from table
        for (int row = 0; row < m_userTable->rowCount(); ++row) {
            QTableWidgetItem *userItem = m_userTable->item(row, 0);
            QComboBox *accessCombo = qobject_cast<QComboBox*>(m_userTable->cellWidget(row, 1));
            
            if (userItem && accessCombo) {
                QString user = userItem->text();
                AccessMode mode = static_cast<AccessMode>(accessCombo->currentData().toInt());
                m_permissions.setUserPermission(user, mode);
            }
        }
    }
}

void PermissionManagerDialog::validateConfiguration()
{
    updatePermissionsFromUI();
    
    m_validationErrors.clear();
    m_isValid = m_permissions.isValid();
    
    if (!m_isValid) {
        m_validationErrors = m_permissions.validationErrors();
    }
    
    // Additional UI-specific validation
    if (m_advancedOptionsEdit) {
        QString options = m_advancedOptionsEdit->toPlainText().trimmed();
        if (!options.isEmpty()) {
            QStringList optionList = options.split('\n', Qt::SkipEmptyParts);
            for (const QString &option : optionList) {
                QString trimmed = option.trimmed();
                if (trimmed.isEmpty()) continue;
                
                // Basic NFS option validation
                if (!trimmed.contains('=') && !QRegularExpression("^[a-zA-Z_][a-zA-Z0-9_]*$").match(trimmed).hasMatch()) {
                    m_validationErrors.append(tr("Invalid NFS option format: %1").arg(trimmed));
                    m_isValid = false;
                }
            }
        }
    }
    
    updateValidationDisplay();
}

void PermissionManagerDialog::updateValidationDisplay()
{
    if (m_statusLabel) {
        if (m_isValid) {
            m_statusLabel->setText(tr("✓ Configuration is valid"));
            m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            m_statusLabel->setText(tr("⚠ Configuration has errors"));
            m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }
    
    if (m_buttonBox) {
        QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
        if (okButton) {
            okButton->setEnabled(m_isValid);
        }
    }
}

void PermissionManagerDialog::setupHostValidation()
{
    // Set up auto-completion for hosts
    m_hostSuggestionsModel = new QStringListModel(getHostSuggestions(), this);
    m_hostCompleter = new QCompleter(m_hostSuggestionsModel, this);
    m_hostCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    
    if (m_newHostEdit) {
        m_newHostEdit->setCompleter(m_hostCompleter);
    }
}

void PermissionManagerDialog::setupUserValidation()
{
    // Set up auto-completion for users
    m_userSuggestionsModel = new QStringListModel(getUserSuggestions(), this);
    m_userCompleter = new QCompleter(m_userSuggestionsModel, this);
    m_userCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    
    if (m_newUserEdit) {
        m_newUserEdit->setCompleter(m_userCompleter);
    }
}

QPair<bool, QString> PermissionManagerDialog::validateHostSpecification(const QString &host) const
{
    if (host.isEmpty()) {
        return {false, tr("Host specification cannot be empty")};
    }
    
    // Check if it's a valid IP address
    QHostAddress addr(host);
    if (!addr.isNull()) {
        return {true, QString()};
    }
    
    // Check if it's a network specification (e.g., 192.168.1.0/24)
    QRegularExpression networkRegex("^(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3})/(\\d{1,2})$");
    QRegularExpressionMatch networkMatch = networkRegex.match(host);
    if (networkMatch.hasMatch()) {
        QString ip = networkMatch.captured(1);
        int prefix = networkMatch.captured(2).toInt();
        
        QHostAddress networkAddr(ip);
        if (networkAddr.isNull()) {
            return {false, tr("Invalid IP address in network specification: %1").arg(ip)};
        }
        
        if (prefix < 0 || prefix > 32) {
            return {false, tr("Invalid network prefix length: %1 (must be 0-32)").arg(prefix)};
        }
        
        return {true, QString()};
    }
    
    // Check if it's a valid hostname (basic validation)
    QRegularExpression hostnameRegex("^[a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?(\\.[a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?)*$");
    if (hostnameRegex.match(host).hasMatch()) {
        return {true, QString()};
    }
    
    // Check for wildcard patterns (*, ?)
    if (host.contains('*') || host.contains('?')) {
        // Basic wildcard validation
        QRegularExpression wildcardRegex("^[a-zA-Z0-9.*?\\-]+$");
        if (wildcardRegex.match(host).hasMatch()) {
            return {true, QString()};
        } else {
            return {false, tr("Invalid characters in wildcard pattern: %1").arg(host)};
        }
    }
    
    return {false, tr("Invalid host specification: %1\n\nValid formats:\n• hostname.com\n• 192.168.1.100\n• 192.168.1.0/24\n• *.example.com").arg(host)};
}

QPair<bool, QString> PermissionManagerDialog::validateUserSpecification(const QString &user) const
{
    if (user.isEmpty()) {
        return {false, tr("Username cannot be empty")};
    }
    
    // POSIX compliant username validation
    QRegularExpression userRegex("^[a-zA-Z_][a-zA-Z0-9_-]{0,31}$");
    if (userRegex.match(user).hasMatch()) {
        return {true, QString()};
    }
    
    return {false, tr("Invalid username: %1\n\nUsernames must:\n• Start with a letter or underscore\n• Contain only letters, numbers, underscores, and hyphens\n• Be 1-32 characters long").arg(user)};
}

QStringList PermissionManagerDialog::getHostSuggestions() const
{
    return {
        "localhost",
        "127.0.0.1",
        "192.168.1.0/24",
        "192.168.0.0/24",
        "10.0.0.0/24",
        "*.local",
        "*"
    };
}

QStringList PermissionManagerDialog::getUserSuggestions() const
{
    return {
        "nobody",
        "nfsnobody",
        "daemon",
        "www-data",
        "apache",
        "nginx"
    };
}

void PermissionManagerDialog::loadPresets()
{
    if (!m_presetsCombo) {
        return;
    }
    
    // Define common permission presets
    PermissionSet readOnlyPreset(AccessMode::ReadOnly);
    readOnlyPreset.setEnableRootSquash(true);
    readOnlyPreset.setAnonymousUser("nobody");
    m_presets["Read Only (Secure)"] = readOnlyPreset;
    
    PermissionSet readWritePreset(AccessMode::ReadWrite);
    readWritePreset.setEnableRootSquash(true);
    readWritePreset.setAnonymousUser("nobody");
    m_presets["Read Write (Secure)"] = readWritePreset;
    
    PermissionSet openPreset(AccessMode::ReadWrite);
    openPreset.setEnableRootSquash(false);
    m_presets["Open Access"] = openPreset;
    
    PermissionSet restrictedPreset(AccessMode::ReadWrite);
    restrictedPreset.setEnableRootSquash(true);
    restrictedPreset.setAnonymousUser("nobody");
    restrictedPreset.setHostPermission("192.168.1.0/24", AccessMode::ReadWrite);
    restrictedPreset.setHostPermission("10.0.0.0/24", AccessMode::ReadOnly);
    m_presets["Network Restricted"] = restrictedPreset;
    
    // Populate combo box
    m_presetsCombo->addItem(tr("Custom"));
    for (auto it = m_presets.begin(); it != m_presets.end(); ++it) {
        m_presetsCombo->addItem(it.key());
    }
}

void PermissionManagerDialog::applyPreset(const QString &presetName)
{
    if (m_presets.contains(presetName)) {
        m_permissions = m_presets[presetName];
        updateUIFromPermissions();
        validateConfiguration();
    }
}

void PermissionManagerDialog::updateExportPreview()
{
    updatePermissionsFromUI();
    
    if (m_exportPreviewEdit) {
        QString preview = m_permissions.toExportOptions();
        m_exportPreviewEdit->setPlainText(preview);
    }
    
    if (m_fullExportPreviewEdit) {
        QString fullPreview = m_permissions.toExportOptions();
        
        // Add custom options if any
        if (m_advancedOptionsEdit) {
            QString customOptions = m_advancedOptionsEdit->toPlainText().trimmed();
            if (!customOptions.isEmpty()) {
                QStringList optionList = customOptions.split('\n', Qt::SkipEmptyParts);
                for (const QString &option : optionList) {
                    QString trimmed = option.trimmed();
                    if (!trimmed.isEmpty()) {
                        fullPreview += "," + trimmed;
                    }
                }
            }
        }
        
        m_fullExportPreviewEdit->setPlainText(fullPreview);
    }
}

} // namespace NFSShareManager