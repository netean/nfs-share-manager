#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QScrollArea>
#include <QDialogButtonBox>

#include "core/configurationmanager.h"

namespace NFSShareManager {

/**
 * @brief Widget for resolving a single configuration conflict
 * 
 * This widget displays information about a specific conflict and provides
 * UI controls for the user to select a resolution option.
 */
class ConflictResolutionWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param conflict The conflict to display and resolve
     * @param parent Parent widget
     */
    explicit ConflictResolutionWidget(const ConfigurationConflict &conflict, QWidget *parent = nullptr);

    /**
     * @brief Get the user's resolution choice
     * @return The selected conflict resolution
     */
    ConflictResolution getResolution() const;

    /**
     * @brief Check if the resolution is valid
     * @return True if the user has made a valid resolution choice
     */
    bool isResolutionValid() const;

    /**
     * @brief Get the conflict being resolved
     * @return The configuration conflict
     */
    const ConfigurationConflict& getConflict() const { return m_conflict; }

signals:
    /**
     * @brief Emitted when the resolution changes
     */
    void resolutionChanged();

private slots:
    /**
     * @brief Handle resolution option selection change
     */
    void onResolutionOptionChanged();

    /**
     * @brief Handle preserve data checkbox change
     */
    void onPreserveDataChanged();

private:
    /**
     * @brief Set up the UI layout
     */
    void setupUI();

    /**
     * @brief Update the additional options based on selected resolution
     */
    void updateAdditionalOptions();

    ConfigurationConflict m_conflict;
    QVBoxLayout *m_layout;
    QLabel *m_descriptionLabel;
    QComboBox *m_resolutionComboBox;
    QCheckBox *m_preserveDataCheckBox;
    QWidget *m_additionalOptionsWidget;
    QVBoxLayout *m_additionalOptionsLayout;
    QLineEdit *m_newPathEdit;
    QLineEdit *m_newMountPointEdit;
};

/**
 * @brief Dialog for resolving configuration conflicts
 * 
 * This dialog presents all detected configuration conflicts to the user
 * and allows them to choose resolution options for each conflict.
 */
class ConflictResolutionDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param conflicts List of conflicts to resolve
     * @param parent Parent widget
     */
    explicit ConflictResolutionDialog(const QList<ConfigurationConflict> &conflicts, QWidget *parent = nullptr);

    /**
     * @brief Get all user resolution choices
     * @return List of conflict resolutions
     */
    QList<ConflictResolution> getResolutions() const;

    /**
     * @brief Check if auto-resolution should be attempted first
     * @return True if auto-resolution is enabled
     */
    bool shouldAutoResolve() const;

public slots:
    /**
     * @brief Accept the dialog and validate all resolutions
     */
    void accept() override;

private slots:
    /**
     * @brief Handle auto-resolve button click
     */
    void onAutoResolveClicked();

    /**
     * @brief Handle resolution change in any conflict widget
     */
    void onResolutionChanged();

private:
    /**
     * @brief Set up the UI layout
     */
    void setupUI();

    /**
     * @brief Update the dialog state based on current resolutions
     */
    void updateDialogState();

    QList<ConfigurationConflict> m_conflicts;
    QVBoxLayout *m_layout;
    QScrollArea *m_scrollArea;
    QWidget *m_scrollWidget;
    QVBoxLayout *m_scrollLayout;
    QList<ConflictResolutionWidget*> m_conflictWidgets;
    QCheckBox *m_autoResolveCheckBox;
    QPushButton *m_autoResolveButton;
    QDialogButtonBox *m_buttonBox;
    QLabel *m_summaryLabel;
};

} // namespace NFSShareManager