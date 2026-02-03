#pragma once

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QElapsedTimer>
#include <QMovie>

namespace NFSShareManager {

/**
 * @brief Progress dialog for long-running operations
 * 
 * This dialog provides comprehensive progress indication for operations like
 * share creation, mounting, network discovery, etc. It includes:
 * - Progress bar (determinate or indeterminate)
 * - Status messages and detailed information
 * - Operation cancellation support
 * - Estimated time remaining
 * - Animated progress indicator
 */
class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Operation types for different progress styles
     */
    enum class OperationType {
        ShareCreation,      ///< Creating NFS shares
        ShareRemoval,       ///< Removing NFS shares
        ShareConfiguration, ///< Configuring share settings
        MountOperation,     ///< Mounting remote shares
        UnmountOperation,   ///< Unmounting shares
        NetworkDiscovery,   ///< Network scanning
        FileSystemOperation,///< File system operations
        SystemConfiguration ///< System configuration changes
    };

    explicit ProgressDialog(const QString &title, 
                           const QString &description,
                           OperationType type = OperationType::FileSystemOperation,
                           QWidget *parent = nullptr);
    ~ProgressDialog();

    /**
     * @brief Set the operation title
     * @param title Main operation title
     */
    void setTitle(const QString &title);

    /**
     * @brief Set the operation description
     * @param description Detailed description of what's happening
     */
    void setDescription(const QString &description);

    /**
     * @brief Set current status message
     * @param status Current operation status
     */
    void setStatus(const QString &status);

    /**
     * @brief Set detailed information
     * @param details Additional details about the operation
     */
    void setDetails(const QString &details);

    /**
     * @brief Set progress value (for determinate progress)
     * @param value Current progress value
     * @param maximum Maximum progress value (default: 100)
     */
    void setProgress(int value, int maximum = 100);

    /**
     * @brief Set indeterminate progress mode
     * @param indeterminate True for indeterminate progress
     */
    void setIndeterminate(bool indeterminate = true);

    /**
     * @brief Enable or disable cancellation
     * @param cancellable True to allow cancellation
     */
    void setCancellable(bool cancellable);

    /**
     * @brief Check if operation was cancelled
     * @return True if user cancelled the operation
     */
    bool wasCancelled() const;

    /**
     * @brief Set estimated time remaining
     * @param seconds Estimated seconds remaining (-1 for unknown)
     */
    void setEstimatedTimeRemaining(int seconds);

    /**
     * @brief Show the dialog and start progress indication
     */
    void startProgress();

    /**
     * @brief Complete the operation successfully
     * @param message Success message
     */
    void completeSuccess(const QString &message = QString());

    /**
     * @brief Complete the operation with error
     * @param errorMessage Error message
     */
    void completeError(const QString &errorMessage);

    /**
     * @brief Update progress with status message
     * @param value Progress value
     * @param status Status message
     * @param details Optional detailed information
     */
    void updateProgress(int value, const QString &status, const QString &details = QString());

signals:
    /**
     * @brief Emitted when user requests cancellation
     */
    void cancelled();

    /**
     * @brief Emitted when operation completes successfully
     */
    void completed();

    /**
     * @brief Emitted when operation fails
     * @param errorMessage Error message
     */
    void failed(const QString &errorMessage);

protected:
    /**
     * @brief Handle close event (treat as cancellation if operation is running)
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief Handle key press events (ESC for cancel)
     */
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    /**
     * @brief Handle cancel button click
     */
    void onCancelClicked();

    /**
     * @brief Update elapsed time display
     */
    void onUpdateTimer();

    /**
     * @brief Handle animation frame updates
     */
    void onAnimationUpdate();

private:
    /**
     * @brief Setup the dialog UI
     */
    void setupUI();

    /**
     * @brief Update the time display
     */
    void updateTimeDisplay();

    /**
     * @brief Format time duration
     * @param seconds Duration in seconds
     * @return Formatted time string
     */
    QString formatDuration(int seconds) const;

    /**
     * @brief Get operation icon based on type
     * @return Icon for the operation type
     */
    QIcon getOperationIcon() const;

    /**
     * @brief Update dialog state based on current status
     */
    void updateDialogState();

    // UI components
    QLabel *m_titleLabel;           ///< Operation title
    QLabel *m_descriptionLabel;     ///< Operation description
    QLabel *m_statusLabel;          ///< Current status
    QLabel *m_detailsLabel;         ///< Detailed information
    QProgressBar *m_progressBar;    ///< Progress bar
    QPushButton *m_cancelButton;    ///< Cancel button
    QPushButton *m_closeButton;     ///< Close button (shown after completion)
    QLabel *m_timeLabel;            ///< Elapsed/remaining time
    QLabel *m_iconLabel;            ///< Operation icon
    QLabel *m_animationLabel;       ///< Animated progress indicator

    // Animation
    QMovie *m_progressAnimation;    ///< Progress animation
    QTimer *m_updateTimer;          ///< UI update timer

    // State
    OperationType m_operationType;  ///< Type of operation
    bool m_cancellable;             ///< Whether operation can be cancelled
    bool m_cancelled;               ///< Whether operation was cancelled
    bool m_completed;               ///< Whether operation completed
    bool m_hasError;                ///< Whether operation failed
    QString m_errorMessage;         ///< Error message if failed

    // Timing
    QElapsedTimer m_elapsedTimer;   ///< Elapsed time tracker
    int m_estimatedTimeRemaining;   ///< Estimated time remaining in seconds
    qint64 m_startTime;             ///< Operation start time
};

} // namespace NFSShareManager