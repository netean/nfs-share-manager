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
#include <QDialogButtonBox>
#include <QProgressBar>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QSplitter>
#include <QTimer>
#include <QRegularExpressionValidator>
#include <QCompleter>
#include <QStringListModel>

#include "../core/permissionset.h"
#include "../core/types.h"

namespace NFSShareManager {

/**
 * @brief Advanced permission management dialog for NFS shares
 * 
 * This dialog provides comprehensive permission configuration capabilities
 * including host-based and user-based access controls, real-time validation,
 * and syntax checking for NFS export options.
 */
class PermissionManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PermissionManagerDialog(QWidget *parent = nullptr);
    explicit PermissionManagerDialog(const PermissionSet &permissions, QWidget *parent = nullptr);
    ~PermissionManagerDialog();

    /**
     * @brief Get the configured permission set
     * @return The complete permission configuration
     */
    PermissionSet getPermissions() const;

    /**
     * @brief Set the permission set to edit
     * @param permissions The permission set to configure
     */
    void setPermissions(const PermissionSet &permissions);

    /**
     * @brief Check if the current configuration is valid
     * @return True if all settings are valid
     */
    bool isValid() const;

    /**
     * @brief Get validation error messages
     * @return List of current validation errors
     */
    QStringList getValidationErrors() const;

    /**
     * @brief Validate host specification (public for testing)
     * @param host The host string to validate
     * @return Validation result with error message if invalid
     */
    QPair<bool, QString> validateHostSpecification(const QString &host) const;

    /**
     * @brief Validate user specification (public for testing)
     * @param user The username to validate
     * @return Validation result with error message if invalid
     */
    QPair<bool, QString> validateUserSpecification(const QString &user) const;

public slots:
    /**
     * @brief Accept the dialog with validation
     */
    void accept() override;

    /**
     * @brief Reset to default permissions
     */
    void resetToDefaults();

    /**
     * @brief Import permissions from export string
     */
    void importFromExportString();

    /**
     * @brief Export permissions to export string
     */
    void exportToExportString();

private slots:
    /**
     * @brief Handle default access mode changes
     */
    void onDefaultAccessChanged();

    /**
     * @brief Handle root squash setting changes
     */
    void onRootSquashChanged();

    /**
     * @brief Handle anonymous user changes with validation
     */
    void onAnonymousUserChanged();

    /**
     * @brief Handle host permission table changes
     */
    void onHostPermissionChanged();

    /**
     * @brief Handle user permission table changes
     */
    void onUserPermissionChanged();

    /**
     * @brief Add new host permission entry
     */
    void onAddHostPermission();

    /**
     * @brief Remove selected host permission entry
     */
    void onRemoveHostPermission();

    /**
     * @brief Add new user permission entry
     */
    void onAddUserPermission();

    /**
     * @brief Remove selected user permission entry
     */
    void onRemoveUserPermission();

    /**
     * @brief Handle host table selection changes
     */
    void onHostTableSelectionChanged();

    /**
     * @brief Handle user table selection changes
     */
    void onUserTableSelectionChanged();

    /**
     * @brief Handle advanced options text changes
     */
    void onAdvancedOptionsChanged();

    /**
     * @brief Handle preset selection changes
     */
    void onPresetChanged();

    /**
     * @brief Test current configuration
     */
    void onTestConfiguration();

    /**
     * @brief Real-time validation timer timeout
     */
    void onValidationTimer();

    /**
     * @brief Handle host input validation
     */
    void onHostInputChanged();

    /**
     * @brief Handle user input validation
     */
    void onUserInputChanged();

protected:
    /**
     * @brief Set up the user interface
     */
    void setupUI();

    /**
     * @brief Set up the basic permissions tab
     */
    void setupBasicTab();

    /**
     * @brief Set up the host permissions tab
     */
    void setupHostPermissionsTab();

    /**
     * @brief Set up the user permissions tab
     */
    void setupUserPermissionsTab();

    /**
     * @brief Set up the advanced options tab
     */
    void setupAdvancedTab();

    /**
     * @brief Connect all signals and slots
     */
    void connectSignals();

    /**
     * @brief Update UI from current permission set
     */
    void updateUIFromPermissions();

    /**
     * @brief Update permission set from UI
     */
    void updatePermissionsFromUI();

    /**
     * @brief Validate current configuration
     */
    void validateConfiguration();

    /**
     * @brief Update validation status display
     */
    void updateValidationDisplay();

    /**
     * @brief Set up host input validation
     */
    void setupHostValidation();

    /**
     * @brief Set up user input validation
     */
    void setupUserValidation();

    /**
     * @brief Get host input suggestions
     * @return List of common host patterns for auto-completion
     */
    QStringList getHostSuggestions() const;

    /**
     * @brief Get user input suggestions
     * @return List of common usernames for auto-completion
     */
    QStringList getUserSuggestions() const;

    /**
     * @brief Load permission presets
     */
    void loadPresets();

    /**
     * @brief Apply selected preset
     * @param presetName Name of the preset to apply
     */
    void applyPreset(const QString &presetName);

    /**
     * @brief Update export preview
     */
    void updateExportPreview();

    // Core data
    PermissionSet m_permissions;
    bool m_isValid;
    QStringList m_validationErrors;

    // Main layout
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_testButton;
    QPushButton *m_resetButton;
    QPushButton *m_importButton;
    QPushButton *m_exportButton;

    // Basic permissions tab
    QWidget *m_basicTab;
    QComboBox *m_defaultAccessCombo;
    QCheckBox *m_rootSquashCheck;
    QLineEdit *m_anonymousUserEdit;
    QLabel *m_anonymousUserValidation;
    QComboBox *m_presetsCombo;
    QTextEdit *m_exportPreviewEdit;

    // Host permissions tab
    QWidget *m_hostTab;
    QTableWidget *m_hostTable;
    QLineEdit *m_newHostEdit;
    QComboBox *m_newHostAccessCombo;
    QPushButton *m_addHostButton;
    QPushButton *m_removeHostButton;
    QLabel *m_hostValidationLabel;
    QCompleter *m_hostCompleter;

    // User permissions tab
    QWidget *m_userTab;
    QTableWidget *m_userTable;
    QLineEdit *m_newUserEdit;
    QComboBox *m_newUserAccessCombo;
    QPushButton *m_addUserButton;
    QPushButton *m_removeUserButton;
    QLabel *m_userValidationLabel;
    QCompleter *m_userCompleter;

    // Advanced options tab
    QWidget *m_advancedTab;
    QTextEdit *m_advancedOptionsEdit;
    QLabel *m_advancedValidationLabel;
    QTextEdit *m_fullExportPreviewEdit;

    // Validation and status
    QLabel *m_statusLabel;
    QProgressBar *m_validationProgress;
    QTimer *m_validationTimer;

    // Validation helpers
    QRegularExpressionValidator *m_hostValidator;
    QRegularExpressionValidator *m_userValidator;
    QStringListModel *m_hostSuggestionsModel;
    QStringListModel *m_userSuggestionsModel;

    // Presets
    QMap<QString, PermissionSet> m_presets;
};

} // namespace NFSShareManager