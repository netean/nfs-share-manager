#include "notificationpreferencesdialog.h"

#include <QApplication>
#include <QPushButton>
#include <QMessageBox>
#include <QDebug>

namespace NFSShareManager {

NotificationPreferencesDialog::NotificationPreferencesDialog(const NotificationPreferences &preferences, QWidget *parent)
    : QDialog(parent)
    , m_preferences(preferences)
    , m_enableNotificationsCheck(nullptr)
    , m_urgencyLevelCombo(nullptr)
    , m_enableKNotificationsCheck(nullptr)
    , m_enableSystemTrayCheck(nullptr)
    , m_enableSoundCheck(nullptr)
    , m_enablePersistentCheck(nullptr)
    , m_enableSharesCheck(nullptr)
    , m_enableMountsCheck(nullptr)
    , m_enableDiscoveryCheck(nullptr)
    , m_enableSystemCheck(nullptr)
    , m_enableOperationsCheck(nullptr)
    , m_enableSecurityCheck(nullptr)
    , m_defaultTimeoutSpin(nullptr)
    , m_errorTimeoutSpin(nullptr)
    , m_successTimeoutSpin(nullptr)
    , m_groupSimilarCheck(nullptr)
    , m_groupingWindowSpin(nullptr)
    , m_buttonBox(nullptr)
    , m_resetButton(nullptr)
    , m_mainLayout(nullptr)
{
    setupUI();
    loadPreferences(preferences);
    updateUIState();
}

NotificationPreferencesDialog::~NotificationPreferencesDialog()
{
}

void NotificationPreferencesDialog::setupUI()
{
    setWindowTitle(tr("Notification Preferences"));
    setModal(true);
    resize(500, 600);

    m_mainLayout = new QVBoxLayout(this);

    // Add preference groups
    m_mainLayout->addWidget(createGeneralGroup());
    m_mainLayout->addWidget(createDeliveryGroup());
    m_mainLayout->addWidget(createCategoryGroup());
    m_mainLayout->addWidget(createTimingGroup());
    m_mainLayout->addWidget(createAdvancedGroup());

    // Add stretch to push buttons to bottom
    m_mainLayout->addStretch();

    // Create button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_resetButton = new QPushButton(tr("Reset to Defaults"), this);
    m_buttonBox->addButton(m_resetButton, QDialogButtonBox::ResetRole);

    m_mainLayout->addWidget(m_buttonBox);

    // Connect signals
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &NotificationPreferencesDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_resetButton, &QPushButton::clicked, this, &NotificationPreferencesDialog::resetToDefaults);

    connect(m_enableNotificationsCheck, &QCheckBox::toggled, this, &NotificationPreferencesDialog::onEnableNotificationsChanged);
    connect(m_enableKNotificationsCheck, &QCheckBox::toggled, this, &NotificationPreferencesDialog::onKNotificationsChanged);
    connect(m_enableSystemTrayCheck, &QCheckBox::toggled, this, &NotificationPreferencesDialog::onSystemTrayChanged);
    connect(m_urgencyLevelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NotificationPreferencesDialog::onUrgencyLevelChanged);
    connect(m_groupSimilarCheck, &QCheckBox::toggled, this, &NotificationPreferencesDialog::onGroupingChanged);

    connect(m_defaultTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NotificationPreferencesDialog::onTimeoutChanged);
    connect(m_errorTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NotificationPreferencesDialog::onTimeoutChanged);
    connect(m_successTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &NotificationPreferencesDialog::onTimeoutChanged);
}

QGroupBox* NotificationPreferencesDialog::createGeneralGroup()
{
    QGroupBox *group = new QGroupBox(tr("General Settings"), this);
    QFormLayout *layout = new QFormLayout(group);

    // Enable notifications
    m_enableNotificationsCheck = new QCheckBox(tr("Enable notifications"), this);
    m_enableNotificationsCheck->setToolTip(tr("Enable or disable all notifications"));
    layout->addRow(m_enableNotificationsCheck);

    // Minimum urgency level
    m_urgencyLevelCombo = new QComboBox(this);
    m_urgencyLevelCombo->addItem(tr("Low"), static_cast<int>(NotificationUrgency::Low));
    m_urgencyLevelCombo->addItem(tr("Normal"), static_cast<int>(NotificationUrgency::Normal));
    m_urgencyLevelCombo->addItem(tr("High"), static_cast<int>(NotificationUrgency::High));
    m_urgencyLevelCombo->addItem(tr("Critical"), static_cast<int>(NotificationUrgency::Critical));
    m_urgencyLevelCombo->setToolTip(tr("Minimum urgency level for notifications to be shown"));
    layout->addRow(tr("Minimum urgency level:"), m_urgencyLevelCombo);

    return group;
}

QGroupBox* NotificationPreferencesDialog::createDeliveryGroup()
{
    QGroupBox *group = new QGroupBox(tr("Delivery Methods"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    // KNotifications
    m_enableKNotificationsCheck = new QCheckBox(tr("Use KDE notifications (KNotifications)"), this);
    m_enableKNotificationsCheck->setToolTip(tr("Use KDE's native notification system"));
    layout->addWidget(m_enableKNotificationsCheck);

    // System tray notifications
    m_enableSystemTrayCheck = new QCheckBox(tr("Show system tray notifications"), this);
    m_enableSystemTrayCheck->setToolTip(tr("Show notifications in the system tray"));
    layout->addWidget(m_enableSystemTrayCheck);

    // Sound notifications
    m_enableSoundCheck = new QCheckBox(tr("Play notification sounds"), this);
    m_enableSoundCheck->setToolTip(tr("Play sounds for notifications"));
    layout->addWidget(m_enableSoundCheck);

    // Persistent notifications
    m_enablePersistentCheck = new QCheckBox(tr("Keep notifications until dismissed"), this);
    m_enablePersistentCheck->setToolTip(tr("Notifications will remain visible until manually dismissed"));
    layout->addWidget(m_enablePersistentCheck);

    return group;
}

QGroupBox* NotificationPreferencesDialog::createCategoryGroup()
{
    QGroupBox *group = new QGroupBox(tr("Notification Categories"), this);
    QVBoxLayout *layout = new QVBoxLayout(group);

    // Share notifications
    m_enableSharesCheck = new QCheckBox(tr("Share management notifications"), this);
    m_enableSharesCheck->setToolTip(tr("Notifications for share creation, removal, and updates"));
    layout->addWidget(m_enableSharesCheck);

    // Mount notifications
    m_enableMountsCheck = new QCheckBox(tr("Mount/unmount notifications"), this);
    m_enableMountsCheck->setToolTip(tr("Notifications for mount and unmount operations"));
    layout->addWidget(m_enableMountsCheck);

    // Discovery notifications
    m_enableDiscoveryCheck = new QCheckBox(tr("Network discovery notifications"), this);
    m_enableDiscoveryCheck->setToolTip(tr("Notifications for network share discovery"));
    layout->addWidget(m_enableDiscoveryCheck);

    // System notifications
    m_enableSystemCheck = new QCheckBox(tr("System notifications"), this);
    m_enableSystemCheck->setToolTip(tr("Notifications for system events and configuration changes"));
    layout->addWidget(m_enableSystemCheck);

    // Operation notifications
    m_enableOperationsCheck = new QCheckBox(tr("Operation notifications"), this);
    m_enableOperationsCheck->setToolTip(tr("Notifications for long-running operations"));
    layout->addWidget(m_enableOperationsCheck);

    // Security notifications
    m_enableSecurityCheck = new QCheckBox(tr("Security notifications"), this);
    m_enableSecurityCheck->setToolTip(tr("Notifications for authentication and permission events"));
    layout->addWidget(m_enableSecurityCheck);

    return group;
}

QGroupBox* NotificationPreferencesDialog::createTimingGroup()
{
    QGroupBox *group = new QGroupBox(tr("Timing Settings"), this);
    QFormLayout *layout = new QFormLayout(group);

    // Default timeout
    m_defaultTimeoutSpin = new QSpinBox(this);
    m_defaultTimeoutSpin->setRange(1000, 30000);
    m_defaultTimeoutSpin->setSuffix(tr(" ms"));
    m_defaultTimeoutSpin->setSingleStep(1000);
    m_defaultTimeoutSpin->setToolTip(tr("Default timeout for normal notifications"));
    layout->addRow(tr("Default timeout:"), m_defaultTimeoutSpin);

    // Error timeout
    m_errorTimeoutSpin = new QSpinBox(this);
    m_errorTimeoutSpin->setRange(1000, 60000);
    m_errorTimeoutSpin->setSuffix(tr(" ms"));
    m_errorTimeoutSpin->setSingleStep(1000);
    m_errorTimeoutSpin->setToolTip(tr("Timeout for error notifications"));
    layout->addRow(tr("Error timeout:"), m_errorTimeoutSpin);

    // Success timeout
    m_successTimeoutSpin = new QSpinBox(this);
    m_successTimeoutSpin->setRange(1000, 15000);
    m_successTimeoutSpin->setSuffix(tr(" ms"));
    m_successTimeoutSpin->setSingleStep(500);
    m_successTimeoutSpin->setToolTip(tr("Timeout for success notifications"));
    layout->addRow(tr("Success timeout:"), m_successTimeoutSpin);

    return group;
}

QGroupBox* NotificationPreferencesDialog::createAdvancedGroup()
{
    QGroupBox *group = new QGroupBox(tr("Advanced Settings"), this);
    QFormLayout *layout = new QFormLayout(group);

    // Group similar notifications
    m_groupSimilarCheck = new QCheckBox(tr("Group similar notifications"), this);
    m_groupSimilarCheck->setToolTip(tr("Combine similar notifications to reduce clutter"));
    layout->addRow(m_groupSimilarCheck);

    // Grouping time window
    m_groupingWindowSpin = new QSpinBox(this);
    m_groupingWindowSpin->setRange(500, 10000);
    m_groupingWindowSpin->setSuffix(tr(" ms"));
    m_groupingWindowSpin->setSingleStep(500);
    m_groupingWindowSpin->setToolTip(tr("Time window for grouping similar notifications"));
    layout->addRow(tr("Grouping window:"), m_groupingWindowSpin);

    return group;
}

void NotificationPreferencesDialog::loadPreferences(const NotificationPreferences &preferences)
{
    m_preferences = preferences;

    // General settings
    m_enableNotificationsCheck->setChecked(preferences.enableNotifications);
    
    int urgencyIndex = m_urgencyLevelCombo->findData(static_cast<int>(preferences.minimumUrgency));
    if (urgencyIndex >= 0) {
        m_urgencyLevelCombo->setCurrentIndex(urgencyIndex);
    }

    // Delivery methods
    m_enableKNotificationsCheck->setChecked(preferences.enableKNotifications);
    m_enableSystemTrayCheck->setChecked(preferences.enableSystemTrayNotifications);
    m_enableSoundCheck->setChecked(preferences.enableSoundNotifications);
    m_enablePersistentCheck->setChecked(preferences.enablePersistentNotifications);

    // Categories
    m_enableSharesCheck->setChecked(preferences.enableShareNotifications);
    m_enableMountsCheck->setChecked(preferences.enableMountNotifications);
    m_enableDiscoveryCheck->setChecked(preferences.enableDiscoveryNotifications);
    m_enableSystemCheck->setChecked(preferences.enableSystemNotifications);
    m_enableOperationsCheck->setChecked(preferences.enableOperationNotifications);
    m_enableSecurityCheck->setChecked(preferences.enableSecurityNotifications);

    // Timing
    m_defaultTimeoutSpin->setValue(preferences.defaultTimeout);
    m_errorTimeoutSpin->setValue(preferences.errorTimeout);
    m_successTimeoutSpin->setValue(preferences.successTimeout);

    // Advanced
    m_groupSimilarCheck->setChecked(preferences.groupSimilarNotifications);
    m_groupingWindowSpin->setValue(preferences.groupingTimeWindow);
}

NotificationPreferences NotificationPreferencesDialog::getPreferences() const
{
    NotificationPreferences preferences;

    // General settings
    preferences.enableNotifications = m_enableNotificationsCheck->isChecked();
    preferences.minimumUrgency = static_cast<NotificationUrgency>(
        m_urgencyLevelCombo->currentData().toInt());

    // Delivery methods
    preferences.enableKNotifications = m_enableKNotificationsCheck->isChecked();
    preferences.enableSystemTrayNotifications = m_enableSystemTrayCheck->isChecked();
    preferences.enableSoundNotifications = m_enableSoundCheck->isChecked();
    preferences.enablePersistentNotifications = m_enablePersistentCheck->isChecked();

    // Categories
    preferences.enableShareNotifications = m_enableSharesCheck->isChecked();
    preferences.enableMountNotifications = m_enableMountsCheck->isChecked();
    preferences.enableDiscoveryNotifications = m_enableDiscoveryCheck->isChecked();
    preferences.enableSystemNotifications = m_enableSystemCheck->isChecked();
    preferences.enableOperationNotifications = m_enableOperationsCheck->isChecked();
    preferences.enableSecurityNotifications = m_enableSecurityCheck->isChecked();

    // Timing
    preferences.defaultTimeout = m_defaultTimeoutSpin->value();
    preferences.errorTimeout = m_errorTimeoutSpin->value();
    preferences.successTimeout = m_successTimeoutSpin->value();

    // Advanced
    preferences.groupSimilarNotifications = m_groupSimilarCheck->isChecked();
    preferences.groupingTimeWindow = m_groupingWindowSpin->value();

    return preferences;
}

void NotificationPreferencesDialog::accept()
{
    if (!validatePreferences()) {
        return;
    }

    m_preferences = getPreferences();
    QDialog::accept();
}

void NotificationPreferencesDialog::resetToDefaults()
{
    int result = QMessageBox::question(this, tr("Reset to Defaults"),
                                      tr("Are you sure you want to reset all notification preferences to their default values?"),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);

    if (result == QMessageBox::Yes) {
        NotificationPreferences defaults;
        loadPreferences(defaults);
        updateUIState();
    }
}

void NotificationPreferencesDialog::onEnableNotificationsChanged(bool enabled)
{
    updateUIState();
}

void NotificationPreferencesDialog::onKNotificationsChanged(bool enabled)
{
    updateUIState();
}

void NotificationPreferencesDialog::onSystemTrayChanged(bool enabled)
{
    updateUIState();
}

void NotificationPreferencesDialog::onUrgencyLevelChanged(int index)
{
    Q_UNUSED(index)
    // Could add logic here to warn about filtering out important notifications
}

void NotificationPreferencesDialog::onTimeoutChanged()
{
    // Could add validation logic here
}

void NotificationPreferencesDialog::onGroupingChanged(bool enabled)
{
    m_groupingWindowSpin->setEnabled(enabled);
}

void NotificationPreferencesDialog::updateUIState()
{
    bool notificationsEnabled = m_enableNotificationsCheck->isChecked();

    // Enable/disable all notification-related controls
    m_urgencyLevelCombo->setEnabled(notificationsEnabled);
    m_enableKNotificationsCheck->setEnabled(notificationsEnabled);
    m_enableSystemTrayCheck->setEnabled(notificationsEnabled);
    m_enableSoundCheck->setEnabled(notificationsEnabled);
    m_enablePersistentCheck->setEnabled(notificationsEnabled);

    m_enableSharesCheck->setEnabled(notificationsEnabled);
    m_enableMountsCheck->setEnabled(notificationsEnabled);
    m_enableDiscoveryCheck->setEnabled(notificationsEnabled);
    m_enableSystemCheck->setEnabled(notificationsEnabled);
    m_enableOperationsCheck->setEnabled(notificationsEnabled);
    m_enableSecurityCheck->setEnabled(notificationsEnabled);

    m_defaultTimeoutSpin->setEnabled(notificationsEnabled);
    m_errorTimeoutSpin->setEnabled(notificationsEnabled);
    m_successTimeoutSpin->setEnabled(notificationsEnabled);

    m_groupSimilarCheck->setEnabled(notificationsEnabled);
    m_groupingWindowSpin->setEnabled(notificationsEnabled && m_groupSimilarCheck->isChecked());

    // Show warning if no delivery methods are enabled
    if (notificationsEnabled) {
        bool hasDeliveryMethod = m_enableKNotificationsCheck->isChecked() || 
                                m_enableSystemTrayCheck->isChecked();
        
        if (!hasDeliveryMethod) {
            // Could show a warning label here
        }
    }
}

bool NotificationPreferencesDialog::validatePreferences() const
{
    // Check if at least one delivery method is enabled when notifications are enabled
    if (m_enableNotificationsCheck->isChecked()) {
        bool hasDeliveryMethod = m_enableKNotificationsCheck->isChecked() || 
                                m_enableSystemTrayCheck->isChecked();
        
        if (!hasDeliveryMethod) {
            QMessageBox::warning(const_cast<NotificationPreferencesDialog*>(this), 
                                tr("Invalid Configuration"),
                                tr("At least one delivery method must be enabled when notifications are enabled."));
            return false;
        }
    }

    // Validate timeout values
    if (m_defaultTimeoutSpin->value() <= 0 || 
        m_errorTimeoutSpin->value() <= 0 || 
        m_successTimeoutSpin->value() <= 0) {
        QMessageBox::warning(const_cast<NotificationPreferencesDialog*>(this), 
                            tr("Invalid Configuration"),
                            tr("All timeout values must be greater than zero."));
        return false;
    }

    return true;
}

} // namespace NFSShareManager