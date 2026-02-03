#pragma once

#include <QObject>
#include <QHash>
#include <QTimer>
#include <QUuid>
#include <QMutex>
#include <QThread>
#include <QFuture>
#include <QFutureWatcher>
#include <QDateTime>
#include <functional>

namespace NFSShareManager {

class ProgressDialog;

/**
 * @brief Manages long-running operations with progress indication and cancellation
 * 
 * This class provides a centralized way to manage long-running operations
 * in the NFS Share Manager application. It handles:
 * - Progress tracking and UI updates
 * - Operation cancellation
 * - Background operation execution
 * - Operation queuing and coordination
 */
class OperationManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Operation status enumeration
     */
    enum class OperationStatus {
        Queued,     ///< Operation is queued for execution
        Running,    ///< Operation is currently running
        Completed,  ///< Operation completed successfully
        Failed,     ///< Operation failed with error
        Cancelled   ///< Operation was cancelled by user
    };

    /**
     * @brief Operation information structure
     */
    struct OperationInfo {
        QUuid id;                           ///< Unique operation ID
        QString title;                      ///< Operation title
        QString description;                ///< Operation description
        OperationStatus status;             ///< Current status
        int progress;                       ///< Progress value (0-100)
        QString statusMessage;              ///< Current status message
        QString errorMessage;               ///< Error message if failed
        bool cancellable;                   ///< Whether operation can be cancelled
        QDateTime startTime;                ///< Operation start time
        QDateTime endTime;                  ///< Operation end time
        ProgressDialog *progressDialog;     ///< Associated progress dialog
        std::function<void()> cancelCallback; ///< Cancellation callback
    };

    explicit OperationManager(QWidget *parentWidget = nullptr, QObject *parent = nullptr);
    ~OperationManager();

    /**
     * @brief Start a new operation with progress dialog
     * @param title Operation title
     * @param description Operation description
     * @param cancellable Whether the operation can be cancelled
     * @param cancelCallback Callback function for cancellation
     * @return Unique operation ID
     */
    QUuid startOperation(const QString &title, 
                        const QString &description,
                        bool cancellable = true,
                        std::function<void()> cancelCallback = nullptr);

    /**
     * @brief Update operation progress
     * @param operationId Operation ID
     * @param progress Progress value (0-100)
     * @param statusMessage Current status message
     * @param details Optional detailed information
     */
    void updateProgress(const QUuid &operationId, 
                       int progress, 
                       const QString &statusMessage,
                       const QString &details = QString());

    /**
     * @brief Set operation status message
     * @param operationId Operation ID
     * @param statusMessage Status message
     */
    void setStatus(const QUuid &operationId, const QString &statusMessage);

    /**
     * @brief Set operation details
     * @param operationId Operation ID
     * @param details Detailed information
     */
    void setDetails(const QUuid &operationId, const QString &details);

    /**
     * @brief Complete operation successfully
     * @param operationId Operation ID
     * @param message Success message
     */
    void completeOperation(const QUuid &operationId, const QString &message = QString());

    /**
     * @brief Fail operation with error
     * @param operationId Operation ID
     * @param errorMessage Error message
     */
    void failOperation(const QUuid &operationId, const QString &errorMessage);

    /**
     * @brief Cancel operation
     * @param operationId Operation ID
     */
    void cancelOperation(const QUuid &operationId);

    /**
     * @brief Check if operation exists
     * @param operationId Operation ID
     * @return True if operation exists
     */
    bool hasOperation(const QUuid &operationId) const;

    /**
     * @brief Get operation information
     * @param operationId Operation ID
     * @return Operation information
     */
    OperationInfo getOperationInfo(const QUuid &operationId) const;

    /**
     * @brief Get list of active operations
     * @return List of active operation IDs
     */
    QList<QUuid> getActiveOperations() const;

    /**
     * @brief Cancel all active operations
     */
    void cancelAllOperations();

    /**
     * @brief Set whether to show progress dialogs
     * @param show True to show progress dialogs
     */
    void setShowProgressDialogs(bool show);

    /**
     * @brief Check if progress dialogs are shown
     * @return True if progress dialogs are shown
     */
    bool showProgressDialogs() const;

signals:
    /**
     * @brief Emitted when an operation starts
     * @param operationId Operation ID
     * @param title Operation title
     */
    void operationStarted(const QUuid &operationId, const QString &title);

    /**
     * @brief Emitted when operation progress updates
     * @param operationId Operation ID
     * @param progress Progress value
     * @param statusMessage Status message
     */
    void operationProgressUpdated(const QUuid &operationId, int progress, const QString &statusMessage);

    /**
     * @brief Emitted when an operation completes successfully
     * @param operationId Operation ID
     * @param message Success message
     */
    void operationCompleted(const QUuid &operationId, const QString &message);

    /**
     * @brief Emitted when an operation fails
     * @param operationId Operation ID
     * @param errorMessage Error message
     */
    void operationFailed(const QUuid &operationId, const QString &errorMessage);

    /**
     * @brief Emitted when an operation is cancelled
     * @param operationId Operation ID
     */
    void operationCancelled(const QUuid &operationId);

private slots:
    /**
     * @brief Handle progress dialog cancellation
     */
    void onProgressDialogCancelled();

    /**
     * @brief Handle progress dialog completion
     */
    void onProgressDialogCompleted();

    /**
     * @brief Handle progress dialog failure
     * @param errorMessage Error message
     */
    void onProgressDialogFailed(const QString &errorMessage);

    /**
     * @brief Clean up completed operations
     */
    void onCleanupTimer();

private:
    /**
     * @brief Create progress dialog for operation
     * @param info Operation information
     * @return Created progress dialog
     */
    ProgressDialog* createProgressDialog(const OperationInfo &info);

    /**
     * @brief Remove operation from tracking
     * @param operationId Operation ID
     */
    void removeOperation(const QUuid &operationId);

    /**
     * @brief Find operation by progress dialog
     * @param dialog Progress dialog
     * @return Operation ID or null UUID if not found
     */
    QUuid findOperationByDialog(ProgressDialog *dialog) const;

    QWidget *m_parentWidget;                        ///< Parent widget for dialogs
    QHash<QUuid, OperationInfo> m_operations;       ///< Active operations
    QTimer *m_cleanupTimer;                         ///< Timer for cleaning up completed operations
    bool m_showProgressDialogs;                     ///< Whether to show progress dialogs
    mutable QMutex m_operationsMutex;               ///< Mutex for thread-safe access
};

} // namespace NFSShareManager