#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QListWidget>
#include <QTextEdit>
#include <QGroupBox>
#include <QTabWidget>
#include <QSpinBox>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QMessageBox>

#include "../core/shareconfiguration.h"
#include "../core/permissionset.h"
#include "../core/types.h"

// Forward declarations
namespace NFSShareManager {
    class PermissionManagerDialog;
}

namespace NFSShareManager {

class ShareManager;

/**
 * @brief Dialog for creating new NFS shares
 * 
 * This dialog provides a comprehensive interface for creating new NFS shares
 * with directory selection, configuration options, and permission settings.
 * It follows KDE UI guidelines and provides real-time validation.
 */
class ShareCreateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShareCreateDialog(ShareManager *shareManager, QWidget *parent = nullptr);
    ~ShareCreateDialog();

    /**
     * @brief Get the configured share path
     * @return The selected directory path to share
     */
    QString getSharePath() const;

    /**
     * @brief Get the configured share settings
     * @return The complete share configuration
     */
    ShareConfiguration getShareConfiguration() const;

    /**
     * @brief Get the configured permissions
     * @return The permission set for the share
     */
    PermissionSet getPermissions() const;

public slots:
    /**
     * @brief Accept the dialog and create the share
     */
    void accept() override;

private slots:
    /**
     * @brief Handle browse button click for directory selection
     */
    void onBrowseClicked();

    /**
     * @brief Handle path text changes for validation
     */
    void onPathChanged();

    /**
     * @brief Handle share name changes
     */
    void onShareNameChanged();

    /**
     * @brief Handle access mode changes
     */
    void onAccessModeChanged();

    /**
     * @brief Handle NFS version changes
     */
    void onNFSVersionChanged();

    /**
     * @brief Handle root access checkbox changes
     */
    void onRootAccessChanged();

    /**
     * @brief Handle add allowed host button
     */
    void onAddAllowedHostClicked();

    /**
     * @brief Handle remove allowed host button
     */
    void onRemoveAllowedHostClicked();

    /**
     * @brief Handle add allowed user button
     */
    void onAddAllowedUserClicked();

    /**
     * @brief Handle remove allowed user button
     */
    void onRemoveAllowedUserClicked();

    /**
     * @brief Handle allowed hosts list selection changes
     */
    void onAllowedHostsSelectionChanged();

    /**
     * @brief Handle allowed users list selection changes
     */
    void onAllowedUsersSelectionChanged();

    /**
     * @brief Handle test configuration button
     */
    void onTestConfigurationClicked();

    /**
     * @brief Handle advanced permission manager button
     */
    void onAdvancedPermissionsClicked();

    /**
     * @brief Handle advanced options toggle
     */
    void onAdvancedOptionsToggled(bool enabled);

    /**
     * @brief Handle add local networks button
     */
    void onAddLocalNetworksClicked();

    /**
     * @brief Handle allow all hosts button
     */
    void onAllowAllHostsClicked();

    /**
     * @brief Validate current configuration
     */
    void validateConfiguration();

protected:
    /**
     * @brief Set up the user interface
     */
    void setupUI();

    /**
     * @brief Set up the basic configuration tab
     */
    void setupBasicTab();

    /**
     * @brief Set up the permissions tab
     */
    void setupPermissionsTab();

    /**
     * @brief Set up the advanced options tab
     */
    void setupAdvancedTab();

    /**
     * @brief Connect all signals and slots
     */
    void connectSignals();

    /**
     * @brief Update UI state based on current configuration
     */
    void updateUIState();

    /**
     * @brief Validate the selected directory path
     * @return True if path is valid for sharing
     */
    bool validatePath();

    /**
     * @brief Validate the share configuration
     * @return True if configuration is valid
     */
    bool validateShareConfig();

    /**
     * @brief Show validation errors to the user
     * @param errors List of error messages
     */
    void showValidationErrors(const QStringList &errors);

    /**
     * @brief Update the configuration preview
     */
    void updateConfigurationPreview();

    /**
     * @brief Get the current configuration as export line
     * @return NFS export line string
     */
    QString getExportLinePreview() const;

    /**
     * @brief Populate default network hosts for local access
     */
    void populateDefaultNetworkHosts();

    /**
     * @brief Get local network ranges from system interfaces
     * @return List of network ranges in CIDR notation
     */
    QStringList getLocalNetworkRanges() const;

    // Core components
    ShareManager *m_shareManager;

    // Main layout
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_testButton;

    // Basic configuration tab
    QWidget *m_basicTab;
    QLineEdit *m_pathEdit;
    QPushButton *m_browseButton;
    QLineEdit *m_shareNameEdit;
    QComboBox *m_accessModeCombo;
    QComboBox *m_nfsVersionCombo;
    QCheckBox *m_allowRootAccessCheck;
    QLabel *m_pathValidationLabel;
    QLabel *m_shareNameValidationLabel;

    // Permissions tab
    QWidget *m_permissionsTab;
    QListWidget *m_allowedHostsList;
    QLineEdit *m_newHostEdit;
    QPushButton *m_addHostButton;
    QPushButton *m_removeHostButton;
    QListWidget *m_allowedUsersList;
    QLineEdit *m_newUserEdit;
    QPushButton *m_addUserButton;
    QPushButton *m_removeUserButton;
    QCheckBox *m_rootSquashCheck;
    QLineEdit *m_anonymousUserEdit;
    QPushButton *m_advancedPermissionsButton;

    // Advanced options tab
    QWidget *m_advancedTab;
    QTextEdit *m_customOptionsEdit;
    QTextEdit *m_configPreviewEdit;
    QCheckBox *m_enableAdvancedCheck;

    // Status and validation
    QProgressBar *m_validationProgress;
    QLabel *m_statusLabel;
    bool m_isValid;
    QStringList m_validationErrors;
    
    // Advanced permission management
    PermissionSet m_detailedPermissions;
};

} // namespace NFSShareManager