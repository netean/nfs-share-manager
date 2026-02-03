#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

#include "notificationmanager.h"

namespace NFSShareManager {

/**
 * @brief Dialog for configuring notification preferences
 * 
 * This dialog provides a comprehensive interface for users to configure
 * all aspects of the notification system, including categories, urgency
 * levels, timeouts, and delivery methods.
 */
class NotificationPreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NotificationPreferencesDialog(const NotificationPreferences &preferences, QWidget *parent = nullptr);
    ~NotificationPreferencesDialog();

    /**
     * @brief Get the configured notification preferences
     * @return Updated notification preferences
     */
    NotificationPreferences getPreferences() const;

public slots:
    /**
     * @brief Accept the dialog and apply preferences
     */
    void accept() override;

    /**
     * @brief Reset preferences to defaults
     */
    void resetToDefaults();

    /**
     * @brief Load preferences from configuration
     */
    void loadPreferences(const NotificationPreferences &preferences);

private slots:
    /**
     * @brief Handle enable notifications checkbox change
     * @param enabled Whether notifications are enabled
     */
    void onEnableNotificationsChanged(bool enabled);

    /**
     * @brief Handle KNotifications checkbox change
     * @param enabled Whether KNotifications are enabled
     */
    void onKNotificationsChanged(bool enabled);

    /**
     * @brief Handle system tray notifications checkbox change
     * @param enabled Whether system tray notifications are enabled
     */
    void onSystemTrayChanged(bool enabled);

    /**
     * @brief Handle urgency level change
     * @param index New urgency level index
     */
    void onUrgencyLevelChanged(int index);

    /**
     * @brief Handle timeout value changes
     */
    void onTimeoutChanged();

    /**
     * @brief Handle grouping preferences change
     * @param enabled Whether notification grouping is enabled
     */
    void onGroupingChanged(bool enabled);

private:
    /**
     * @brief Set up the dialog UI
     */
    void setupUI();

    /**
     * @brief Create general preferences group
     * @return General preferences group box
     */
    QGroupBox* createGeneralGroup();

    /**
     * @brief Create delivery methods group
     * @return Delivery methods group box
     */
    QGroupBox* createDeliveryGroup();

    /**
     * @brief Create category preferences group
     * @return Category preferences group box
     */
    QGroupBox* createCategoryGroup();

    /**
     * @brief Create timing preferences group
     * @return Timing preferences group box
     */
    QGroupBox* createTimingGroup();

    /**
     * @brief Create advanced preferences group
     * @return Advanced preferences group box
     */
    QGroupBox* createAdvancedGroup();

    /**
     * @brief Update UI state based on current preferences
     */
    void updateUIState();

    /**
     * @brief Validate current preferences
     * @return True if preferences are valid
     */
    bool validatePreferences() const;

    // Current preferences
    NotificationPreferences m_preferences;

    // General preferences
    QCheckBox *m_enableNotificationsCheck;
    QComboBox *m_urgencyLevelCombo;

    // Delivery methods
    QCheckBox *m_enableKNotificationsCheck;
    QCheckBox *m_enableSystemTrayCheck;
    QCheckBox *m_enableSoundCheck;
    QCheckBox *m_enablePersistentCheck;

    // Category preferences
    QCheckBox *m_enableSharesCheck;
    QCheckBox *m_enableMountsCheck;
    QCheckBox *m_enableDiscoveryCheck;
    QCheckBox *m_enableSystemCheck;
    QCheckBox *m_enableOperationsCheck;
    QCheckBox *m_enableSecurityCheck;

    // Timing preferences
    QSpinBox *m_defaultTimeoutSpin;
    QSpinBox *m_errorTimeoutSpin;
    QSpinBox *m_successTimeoutSpin;

    // Advanced preferences
    QCheckBox *m_groupSimilarCheck;
    QSpinBox *m_groupingWindowSpin;

    // Dialog buttons
    QDialogButtonBox *m_buttonBox;
    QPushButton *m_resetButton;

    // Layout
    QVBoxLayout *m_mainLayout;
};

} // namespace NFSShareManager