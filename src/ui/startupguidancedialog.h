#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QTimer>
#include <QScrollArea>

#include "../core/startupmanager.h"

namespace NFSShareManager {

/**
 * @brief Startup guidance dialog for handling startup issues
 * 
 * This dialog provides comprehensive guidance to users when startup
 * validation detects missing dependencies, unavailable services, or
 * configuration issues. It offers automated solutions where possible
 * and clear instructions for manual resolution.
 */
class StartupGuidanceDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Dialog result options
     */
    enum class Result {
        ContinueAnyway,     ///< Continue despite issues
        TryAgain,           ///< Retry startup validation
        InstallDependencies, ///< Attempt to install dependencies
        StartServices,      ///< Attempt to start services
        Exit                ///< Exit the application
    };

    explicit StartupGuidanceDialog(const StartupValidationResult &result, 
                                  StartupManager *startupManager,
                                  QWidget *parent = nullptr);
    ~StartupGuidanceDialog();

    /**
     * @brief Get the user's chosen result
     * @return Selected result option
     */
    Result getResult() const { return m_result; }

    /**
     * @brief Check if user wants to continue despite issues
     * @return True if user chose to continue
     */
    bool shouldContinue() const { return m_result == Result::ContinueAnyway; }

public slots:
    /**
     * @brief Update the validation result and refresh display
     * @param result New validation result
     */
    void updateValidationResult(const StartupValidationResult &result);

private slots:
    /**
     * @brief Handle continue anyway button click
     */
    void onContinueAnywayClicked();

    /**
     * @brief Handle try again button click
     */
    void onTryAgainClicked();

    /**
     * @brief Handle install dependencies button click
     */
    void onInstallDependenciesClicked();

    /**
     * @brief Handle start services button click
     */
    void onStartServicesClicked();

    /**
     * @brief Handle exit button click
     */
    void onExitClicked();

    /**
     * @brief Handle show details toggle
     */
    void onShowDetailsToggled(bool show);

    /**
     * @brief Handle copy system info button click
     */
    void onCopySystemInfoClicked();

    /**
     * @brief Handle dependency installation progress
     */
    void onDependencyInstallationStarted();
    void onDependencyInstallationFinished(bool success);

    /**
     * @brief Handle service startup progress
     */
    void onServiceStartupStarted();
    void onServiceStartupFinished(bool success);

    /**
     * @brief Update progress display
     */
    void updateProgress();

private:
    /**
     * @brief Setup the dialog UI
     */
    void setupUI();

    /**
     * @brief Setup the overview tab
     */
    void setupOverviewTab();

    /**
     * @brief Setup the dependencies tab
     */
    void setupDependenciesTab();

    /**
     * @brief Setup the services tab
     */
    void setupServicesTab();

    /**
     * @brief Setup the system info tab
     */
    void setupSystemInfoTab();

    /**
     * @brief Update the overview display
     */
    void updateOverview();

    /**
     * @brief Update the dependencies display
     */
    void updateDependencies();

    /**
     * @brief Update the services display
     */
    void updateServices();

    /**
     * @brief Update the system info display
     */
    void updateSystemInfo();

    /**
     * @brief Update button states based on current situation
     */
    void updateButtons();

    /**
     * @brief Create dependency item widget
     * @param dependency Dependency information
     * @return Widget for the dependency
     */
    QWidget* createDependencyWidget(const StartupDependency &dependency);

    /**
     * @brief Create service item widget
     * @param service Service information
     * @return Widget for the service
     */
    QWidget* createServiceWidget(const ServiceInfo &service);

    /**
     * @brief Get status icon for dependency
     * @param dependency Dependency to get icon for
     * @return Status icon
     */
    QIcon getDependencyIcon(const StartupDependency &dependency) const;

    /**
     * @brief Get status icon for service
     * @param service Service to get icon for
     * @return Status icon
     */
    QIcon getServiceIcon(const ServiceInfo &service) const;

    /**
     * @brief Format dependency status text
     * @param dependency Dependency to format
     * @return Formatted status text
     */
    QString formatDependencyStatus(const StartupDependency &dependency) const;

    /**
     * @brief Format service status text
     * @param service Service to format
     * @return Formatted status text
     */
    QString formatServiceStatus(const ServiceInfo &service) const;

    StartupValidationResult m_validationResult;
    StartupManager *m_startupManager;
    Result m_result;

    // UI components
    QTabWidget *m_tabWidget;
    
    // Overview tab
    QWidget *m_overviewTab;
    QLabel *m_statusIcon;
    QLabel *m_statusText;
    QLabel *m_summaryText;
    QListWidget *m_suggestionsList;
    
    // Dependencies tab
    QWidget *m_dependenciesTab;
    QVBoxLayout *m_dependenciesLayout;
    QScrollArea *m_dependenciesScrollArea;
    QWidget *m_dependenciesContent;
    
    // Services tab
    QWidget *m_servicesTab;
    QVBoxLayout *m_servicesLayout;
    QScrollArea *m_servicesScrollArea;
    QWidget *m_servicesContent;
    
    // System info tab
    QWidget *m_systemInfoTab;
    QTextEdit *m_systemInfoText;
    QPushButton *m_copySystemInfoButton;
    
    // Progress and buttons
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;
    QPushButton *m_continueButton;
    QPushButton *m_tryAgainButton;
    QPushButton *m_installDepsButton;
    QPushButton *m_startServicesButton;
    QPushButton *m_exitButton;
    QPushButton *m_detailsButton;
    
    // State tracking
    bool m_showingDetails;
    bool m_operationInProgress;
    QTimer *m_progressTimer;
};

} // namespace NFSShareManager