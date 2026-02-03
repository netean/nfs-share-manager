#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QTextEdit>
#include <QTabWidget>
#include <QButtonGroup>
#include <QRadioButton>

#include "../core/remotenfsshare.h"
#include "../core/nfsmount.h"
#include "../business/mountmanager.h"

namespace NFSShareManager {

/**
 * @brief Dialog for mounting remote NFS shares
 * 
 * This dialog provides a comprehensive interface for configuring and mounting
 * remote NFS shares, including mount point selection, mount options configuration,
 * and support for both temporary and persistent mounts.
 */
class MountDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for mounting a specific remote share
     * @param remoteShare The remote share to mount
     * @param mountManager The mount manager instance
     * @param parent Parent widget
     */
    explicit MountDialog(const RemoteNFSShare &remoteShare, 
                        MountManager *mountManager,
                        QWidget *parent = nullptr);

    /**
     * @brief Get the configured mount point
     * @return The selected local mount point path
     */
    QString getMountPoint() const;

    /**
     * @brief Get the configured mount options
     * @return The configured mount options
     */
    MountOptions getMountOptions() const;

    /**
     * @brief Check if the mount should be persistent
     * @return True if mount should survive reboots
     */
    bool isPersistent() const;

    /**
     * @brief Get the configured NFSMount object
     * @return Complete mount configuration
     */
    NFSMount getMount() const;

protected:
    /**
     * @brief Accept the dialog and validate configuration
     */
    void accept() override;

private slots:
    /**
     * @brief Handle browse button click for mount point selection
     */
    void onBrowseMountPointClicked();

    /**
     * @brief Handle mount point text changes
     */
    void onMountPointChanged();

    /**
     * @brief Handle mount type selection changes
     */
    void onMountTypeChanged();

    /**
     * @brief Handle NFS version selection changes
     */
    void onNFSVersionChanged();

    /**
     * @brief Handle advanced options toggle
     */
    void onAdvancedOptionsToggled(bool enabled);

    /**
     * @brief Handle test connection button click
     */
    void onTestConnectionClicked();

    /**
     * @brief Handle mount manager signals
     */
    void onMountStarted(const RemoteNFSShare &remoteShare, const QString &mountPoint);
    void onMountCompleted(const NFSMount &mount);
    void onMountFailed(const RemoteNFSShare &remoteShare, const QString &mountPoint, 
                       MountManager::MountResult result, const QString &errorMessage);

    /**
     * @brief Update mount point validation status
     */
    void updateMountPointValidation();

    /**
     * @brief Update dialog state based on current configuration
     */
    void updateDialogState();

    /**
     * @brief Reset options to defaults
     */
    void resetToDefaults();

private:
    /**
     * @brief Set up the dialog UI
     */
    void setupUI();

    /**
     * @brief Set up the basic mount configuration tab
     */
    void setupBasicTab();

    /**
     * @brief Set up the advanced options tab
     */
    void setupAdvancedTab();

    /**
     * @brief Set up the dialog buttons
     */
    void setupButtons();

    /**
     * @brief Create the button layout
     * @return Button layout with all dialog buttons
     */
    QHBoxLayout* createButtonLayout();

    /**
     * @brief Connect all signals and slots
     */
    void connectSignals();

    /**
     * @brief Load default mount options
     */
    void loadDefaults();

    /**
     * @brief Validate the current configuration
     * @return True if configuration is valid
     */
    bool validateConfiguration();

    /**
     * @brief Get validation error messages
     * @return List of validation errors
     */
    QStringList getValidationErrors();

    /**
     * @brief Generate a suggested mount point path
     * @return Suggested mount point path
     */
    QString generateSuggestedMountPoint();

    /**
     * @brief Update the mount point suggestions
     */
    void updateMountPointSuggestions();

    /**
     * @brief Show validation errors to the user
     * @param errors List of error messages
     */
    void showValidationErrors(const QStringList &errors);

    // Core components
    RemoteNFSShare m_remoteShare;
    MountManager *m_mountManager;

    // UI components
    QTabWidget *m_tabWidget;
    
    // Basic tab components
    QWidget *m_basicTab;
    QLabel *m_shareInfoLabel;
    QLineEdit *m_mountPointEdit;
    QPushButton *m_browseMountPointButton;
    QLabel *m_mountPointValidationLabel;
    QButtonGroup *m_mountTypeGroup;
    QRadioButton *m_temporaryMountRadio;
    QRadioButton *m_persistentMountRadio;
    QComboBox *m_nfsVersionCombo;
    QCheckBox *m_readOnlyCheckBox;

    // Advanced tab components
    QWidget *m_advancedTab;
    QSpinBox *m_timeoutSpinBox;
    QSpinBox *m_retryCountSpinBox;
    QCheckBox *m_softMountCheckBox;
    QCheckBox *m_backgroundMountCheckBox;
    QSpinBox *m_rsizeSpinBox;
    QSpinBox *m_wsizeSpinBox;
    QLineEdit *m_securityFlavorEdit;
    QTextEdit *m_customOptionsEdit;

    // Dialog buttons
    QPushButton *m_mountButton;
    QPushButton *m_cancelButton;
    QPushButton *m_testConnectionButton;
    QPushButton *m_resetDefaultsButton;

    // Status and progress
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;

    // State tracking
    bool m_isValidConfiguration;
    bool m_isMounting;
    QStringList m_validationErrors;
};

} // namespace NFSShareManager