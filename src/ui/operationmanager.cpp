#include "operationmanager.h"
#include "progressdialog.h"

#include <QApplication>
#include <QMutexLocker>
#include <QDebug>

namespace NFSShareManager {

OperationManager::OperationManager(QWidget *parentWidget, QObject *parent)
    : QObject(parent)
    , m_parentWidget(parentWidget)
    , m_cleanupTimer(new QTimer(this))
    , m_showProgressDialogs(true)
{
    // Setup cleanup timer to remove completed operations
    connect(m_cleanupTimer, &QTimer::timeout, this, &OperationManager::onCleanupTimer);
    m_cleanupTimer->setInterval(30000); // Clean up every 30 seconds
    m_cleanupTimer->start();
}

OperationManager::~OperationManager()
{
    cancelAllOperations();
}

QUuid OperationManager::startOperation(const QString &title, 
                                      const QString &description,
                                      bool cancellable,
                                      std::function<void()> cancelCallback)
{
    QMutexLocker locker(&m_operationsMutex);
    
    QUuid operationId = QUuid::createUuid();
    
    OperationInfo info;
    info.id = operationId;
    info.title = title;
    info.description = description;
    info.status = OperationStatus::Running;
    info.progress = 0;
    info.cancellable = cancellable;
    info.startTime = QDateTime::currentDateTime();
    info.progressDialog = nullptr;
    info.cancelCallback = cancelCallback;
    
    // Create progress dialog if enabled
    if (m_showProgressDialogs) {
        info.progressDialog = createProgressDialog(info);
    }
    
    m_operations[operationId] = info;
    
    qDebug() << "Started operation:" << operationId << title;
    emit operationStarted(operationId, title);
    
    return operationId;
}

void OperationManager::updateProgress(const QUuid &operationId, 
                                     int progress, 
                                     const QString &statusMessage,
                                     const QString &details)
{
    QMutexLocker locker(&m_operationsMutex);
    
    if (!m_operations.contains(operationId)) {
        qWarning() << "Operation not found:" << operationId;
        return;
    }
    
    OperationInfo &info = m_operations[operationId];
    info.progress = qBound(0, progress, 100);
    info.statusMessage = statusMessage;
    
    if (info.progressDialog) {
        info.progressDialog->updateProgress(progress, statusMessage, details);
    }
    
    emit operationProgressUpdated(operationId, progress, statusMessage);
}

void OperationManager::setStatus(const QUuid &operationId, const QString &statusMessage)
{
    QMutexLocker locker(&m_operationsMutex);
    
    if (!m_operations.contains(operationId)) {
        qWarning() << "Operation not found:" << operationId;
        return;
    }
    
    OperationInfo &info = m_operations[operationId];
    info.statusMessage = statusMessage;
    
    if (info.progressDialog) {
        info.progressDialog->setStatus(statusMessage);
    }
}

void OperationManager::setDetails(const QUuid &operationId, const QString &details)
{
    QMutexLocker locker(&m_operationsMutex);
    
    if (!m_operations.contains(operationId)) {
        qWarning() << "Operation not found:" << operationId;
        return;
    }
    
    OperationInfo &info = m_operations[operationId];
    
    if (info.progressDialog) {
        info.progressDialog->setDetails(details);
    }
}

void OperationManager::completeOperation(const QUuid &operationId, const QString &message)
{
    QMutexLocker locker(&m_operationsMutex);
    
    if (!m_operations.contains(operationId)) {
        qWarning() << "Operation not found:" << operationId;
        return;
    }
    
    OperationInfo &info = m_operations[operationId];
    info.status = OperationStatus::Completed;
    info.endTime = QDateTime::currentDateTime();
    info.progress = 100;
    
    if (info.progressDialog) {
        info.progressDialog->completeSuccess(message);
    }
    
    qDebug() << "Completed operation:" << operationId << message;
    emit operationCompleted(operationId, message);
}

void OperationManager::failOperation(const QUuid &operationId, const QString &errorMessage)
{
    QMutexLocker locker(&m_operationsMutex);
    
    if (!m_operations.contains(operationId)) {
        qWarning() << "Operation not found:" << operationId;
        return;
    }
    
    OperationInfo &info = m_operations[operationId];
    info.status = OperationStatus::Failed;
    info.endTime = QDateTime::currentDateTime();
    info.errorMessage = errorMessage;
    
    if (info.progressDialog) {
        info.progressDialog->completeError(errorMessage);
    }
    
    qWarning() << "Failed operation:" << operationId << errorMessage;
    emit operationFailed(operationId, errorMessage);
}

void OperationManager::cancelOperation(const QUuid &operationId)
{
    QMutexLocker locker(&m_operationsMutex);
    
    if (!m_operations.contains(operationId)) {
        qWarning() << "Operation not found:" << operationId;
        return;
    }
    
    OperationInfo &info = m_operations[operationId];
    
    if (info.status != OperationStatus::Running) {
        qDebug() << "Operation not running, cannot cancel:" << operationId;
        return;
    }
    
    info.status = OperationStatus::Cancelled;
    info.endTime = QDateTime::currentDateTime();
    
    // Call cancellation callback if provided
    if (info.cancelCallback) {
        locker.unlock(); // Unlock before calling callback to avoid deadlock
        info.cancelCallback();
        locker.relock();
    }
    
    if (info.progressDialog) {
        info.progressDialog->completeError(tr("Operation was cancelled"));
    }
    
    qDebug() << "Cancelled operation:" << operationId;
    emit operationCancelled(operationId);
}

bool OperationManager::hasOperation(const QUuid &operationId) const
{
    QMutexLocker locker(&m_operationsMutex);
    return m_operations.contains(operationId);
}

OperationManager::OperationInfo OperationManager::getOperationInfo(const QUuid &operationId) const
{
    QMutexLocker locker(&m_operationsMutex);
    return m_operations.value(operationId);
}

QList<QUuid> OperationManager::getActiveOperations() const
{
    QMutexLocker locker(&m_operationsMutex);
    
    QList<QUuid> activeOps;
    for (auto it = m_operations.begin(); it != m_operations.end(); ++it) {
        if (it.value().status == OperationStatus::Running || 
            it.value().status == OperationStatus::Queued) {
            activeOps.append(it.key());
        }
    }
    
    return activeOps;
}

void OperationManager::cancelAllOperations()
{
    QList<QUuid> activeOps = getActiveOperations();
    for (const QUuid &operationId : activeOps) {
        cancelOperation(operationId);
    }
}

void OperationManager::setShowProgressDialogs(bool show)
{
    m_showProgressDialogs = show;
}

bool OperationManager::showProgressDialogs() const
{
    return m_showProgressDialogs;
}

void OperationManager::onProgressDialogCancelled()
{
    ProgressDialog *dialog = qobject_cast<ProgressDialog*>(sender());
    if (!dialog) {
        return;
    }
    
    QUuid operationId = findOperationByDialog(dialog);
    if (!operationId.isNull()) {
        cancelOperation(operationId);
    }
}

void OperationManager::onProgressDialogCompleted()
{
    ProgressDialog *dialog = qobject_cast<ProgressDialog*>(sender());
    if (!dialog) {
        return;
    }
    
    // Dialog completed - operation should already be marked as completed
    // Just clean up the dialog reference
    QUuid operationId = findOperationByDialog(dialog);
    if (!operationId.isNull()) {
        QMutexLocker locker(&m_operationsMutex);
        if (m_operations.contains(operationId)) {
            m_operations[operationId].progressDialog = nullptr;
        }
    }
}

void OperationManager::onProgressDialogFailed(const QString &errorMessage)
{
    ProgressDialog *dialog = qobject_cast<ProgressDialog*>(sender());
    if (!dialog) {
        return;
    }
    
    QUuid operationId = findOperationByDialog(dialog);
    if (!operationId.isNull()) {
        failOperation(operationId, errorMessage);
    }
}

void OperationManager::onCleanupTimer()
{
    QMutexLocker locker(&m_operationsMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-300); // 5 minutes ago
    
    auto it = m_operations.begin();
    while (it != m_operations.end()) {
        const OperationInfo &info = it.value();
        
        // Remove completed operations older than 5 minutes
        if ((info.status == OperationStatus::Completed || 
             info.status == OperationStatus::Failed || 
             info.status == OperationStatus::Cancelled) &&
            info.endTime.isValid() && info.endTime < cutoff) {
            
            qDebug() << "Cleaning up old operation:" << it.key();
            
            // Clean up progress dialog if it still exists
            if (info.progressDialog) {
                info.progressDialog->deleteLater();
            }
            
            it = m_operations.erase(it);
        } else {
            ++it;
        }
    }
}

ProgressDialog* OperationManager::createProgressDialog(const OperationInfo &info)
{
    ProgressDialog::OperationType dialogType = ProgressDialog::OperationType::FileSystemOperation;
    
    // Determine dialog type based on operation title/description
    QString title = info.title.toLower();
    if (title.contains("shar") && title.contains("creat")) {
        dialogType = ProgressDialog::OperationType::ShareCreation;
    } else if (title.contains("shar") && (title.contains("remov") || title.contains("delet"))) {
        dialogType = ProgressDialog::OperationType::ShareRemoval;
    } else if (title.contains("shar") && title.contains("config")) {
        dialogType = ProgressDialog::OperationType::ShareConfiguration;
    } else if (title.contains("mount") && !title.contains("unmount")) {
        dialogType = ProgressDialog::OperationType::MountOperation;
    } else if (title.contains("unmount")) {
        dialogType = ProgressDialog::OperationType::UnmountOperation;
    } else if (title.contains("discover") || title.contains("scan")) {
        dialogType = ProgressDialog::OperationType::NetworkDiscovery;
    } else if (title.contains("config") || title.contains("setting")) {
        dialogType = ProgressDialog::OperationType::SystemConfiguration;
    }
    
    ProgressDialog *dialog = new ProgressDialog(info.title, info.description, dialogType, m_parentWidget);
    dialog->setCancellable(info.cancellable);
    
    // Connect dialog signals
    connect(dialog, &ProgressDialog::cancelled, this, &OperationManager::onProgressDialogCancelled);
    connect(dialog, &ProgressDialog::completed, this, &OperationManager::onProgressDialogCompleted);
    connect(dialog, &ProgressDialog::failed, this, &OperationManager::onProgressDialogFailed);
    
    // Start the dialog
    dialog->startProgress();
    
    return dialog;
}

void OperationManager::removeOperation(const QUuid &operationId)
{
    QMutexLocker locker(&m_operationsMutex);
    
    if (m_operations.contains(operationId)) {
        OperationInfo info = m_operations.take(operationId);
        if (info.progressDialog) {
            info.progressDialog->deleteLater();
        }
    }
}

QUuid OperationManager::findOperationByDialog(ProgressDialog *dialog) const
{
    QMutexLocker locker(&m_operationsMutex);
    
    for (auto it = m_operations.begin(); it != m_operations.end(); ++it) {
        if (it.value().progressDialog == dialog) {
            return it.key();
        }
    }
    
    return QUuid();
}

} // namespace NFSShareManager