#include "shareconfigdialog.h"
#include "../business/sharemanager.h"

#include <QMessageBox>
#include <QDebug>

namespace NFSShareManager {

ShareConfigDialog::ShareConfigDialog(const NFSShare &share, ShareManager *shareManager, QWidget *parent)
    : ShareCreateDialog(shareManager, parent)
    , m_originalShare(share)
    , m_isModified(false)
{
    setupEditMode();
    loadShareConfiguration();
    connectChangeTracking();
}

ShareConfigDialog::~ShareConfigDialog()
{
}

const NFSShare &ShareConfigDialog::originalShare() const
{
    return m_originalShare;
}

bool ShareConfigDialog::isModified() const
{
    return m_isModified;
}

void ShareConfigDialog::accept()
{
    // Check if configuration has actually changed
    if (!isModified()) {
        QDialog::accept();
        return;
    }
    
    // Perform validation by calling the parent's validation
    if (!validatePath() || !validateShareConfig()) {
        showValidationErrors(m_validationErrors);
        return;
    }
    
    // Confirm the changes with the user
    QString message = tr("Are you sure you want to update the share configuration?\n\n"
                        "Path: %1\n"
                        "This will modify the NFS exports and may temporarily interrupt access.")
                     .arg(m_originalShare.path());
    
    int result = QMessageBox::question(this, tr("Confirm Share Update"), message,
                                     QMessageBox::Yes | QMessageBox::No,
                                     QMessageBox::No);
    
    if (result != QMessageBox::Yes) {
        return;
    }
    
    QDialog::accept();
}

void ShareConfigDialog::onConfigurationChanged()
{
    m_isModified = true;
    
    // Update window title to indicate changes
    QString title = tr("Edit NFS Share - %1").arg(m_originalShare.displayName());
    if (m_isModified) {
        title += " *";
    }
    setWindowTitle(title);
}

void ShareConfigDialog::loadShareConfiguration()
{
    // Load path (read-only)
    if (m_pathEdit) {
        m_pathEdit->setText(m_originalShare.path());
    }
    
    // Load share configuration
    const ShareConfiguration &config = m_originalShare.config();
    
    if (m_shareNameEdit) {
        m_shareNameEdit->setText(config.name());
    }
    
    if (m_accessModeCombo) {
        int index = m_accessModeCombo->findData(static_cast<int>(config.accessMode()));
        if (index >= 0) {
            m_accessModeCombo->setCurrentIndex(index);
        }
    }
    
    if (m_nfsVersionCombo) {
        int index = m_nfsVersionCombo->findData(static_cast<int>(config.nfsVersion()));
        if (index >= 0) {
            m_nfsVersionCombo->setCurrentIndex(index);
        }
    }
    
    if (m_allowRootAccessCheck) {
        m_allowRootAccessCheck->setChecked(config.allowRootAccess());
    }
    
    // Load allowed hosts
    if (m_allowedHostsList) {
        m_allowedHostsList->clear();
        const QStringList &hosts = config.allowedHosts();
        for (const QString &host : hosts) {
            m_allowedHostsList->addItem(host);
        }
    }
    
    // Load allowed users
    if (m_allowedUsersList) {
        m_allowedUsersList->clear();
        const QStringList &users = config.allowedUsers();
        for (const QString &user : users) {
            m_allowedUsersList->addItem(user);
        }
    }
    
    // Load permissions
    const PermissionSet &permissions = m_originalShare.permissions();
    
    if (m_rootSquashCheck) {
        m_rootSquashCheck->setChecked(permissions.enableRootSquash());
    }
    
    if (m_anonymousUserEdit) {
        m_anonymousUserEdit->setText(permissions.anonymousUser());
    }
    
    // Update UI state
    updateUIState();
    
    // Reset modification flag after loading
    m_isModified = false;
}

void ShareConfigDialog::setupEditMode()
{
    // Change window title
    setWindowTitle(tr("Edit NFS Share - %1").arg(m_originalShare.displayName()));
    
    // Disable path editing
    if (m_pathEdit) {
        m_pathEdit->setReadOnly(true);
        m_pathEdit->setStyleSheet("background-color: #f0f0f0;");
    }
    
    if (m_browseButton) {
        m_browseButton->setEnabled(false);
        m_browseButton->setVisible(false);
    }
    
    // Update button text
    if (m_buttonBox) {
        QPushButton *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
        if (okButton) {
            okButton->setText(tr("Update Share"));
        }
    }
    
    // Add information about the current share status
    if (m_statusLabel) {
        QString statusText;
        if (m_originalShare.isActive()) {
            statusText = tr("Share is currently active and exported");
        } else {
            statusText = tr("Share is currently inactive");
        }
        
        if (m_originalShare.hasError()) {
            statusText += tr(" (Error: %1)").arg(m_originalShare.errorMessage());
        }
        
        m_statusLabel->setText(statusText);
    }
}

void ShareConfigDialog::connectChangeTracking()
{
    // Connect all input widgets to change tracking
    if (m_shareNameEdit) {
        connect(m_shareNameEdit, &QLineEdit::textChanged, this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_accessModeCombo) {
        connect(m_accessModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_nfsVersionCombo) {
        connect(m_nfsVersionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_allowRootAccessCheck) {
        connect(m_allowRootAccessCheck, &QCheckBox::toggled, this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_allowedHostsList) {
        connect(m_allowedHostsList, &QListWidget::itemChanged, this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_allowedUsersList) {
        connect(m_allowedUsersList, &QListWidget::itemChanged, this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_rootSquashCheck) {
        connect(m_rootSquashCheck, &QCheckBox::toggled, this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_anonymousUserEdit) {
        connect(m_anonymousUserEdit, &QLineEdit::textChanged, this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_customOptionsEdit) {
        connect(m_customOptionsEdit, &QTextEdit::textChanged, this, &ShareConfigDialog::onConfigurationChanged);
    }
    
    if (m_enableAdvancedCheck) {
        connect(m_enableAdvancedCheck, &QCheckBox::toggled, this, &ShareConfigDialog::onConfigurationChanged);
    }
}

} // namespace NFSShareManager