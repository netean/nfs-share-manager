#include "progressdialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QIcon>
#include <QStyle>
#include <QPixmap>
#include <QDebug>

namespace NFSShareManager {

ProgressDialog::ProgressDialog(const QString &title, 
                              const QString &description,
                              OperationType type,
                              QWidget *parent)
    : QDialog(parent)
    , m_titleLabel(nullptr)
    , m_descriptionLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_detailsLabel(nullptr)
    , m_progressBar(nullptr)
    , m_cancelButton(nullptr)
    , m_closeButton(nullptr)
    , m_timeLabel(nullptr)
    , m_iconLabel(nullptr)
    , m_animationLabel(nullptr)
    , m_progressAnimation(nullptr)
    , m_updateTimer(new QTimer(this))
    , m_operationType(type)
    , m_cancellable(true)
    , m_cancelled(false)
    , m_completed(false)
    , m_hasError(false)
    , m_estimatedTimeRemaining(-1)
    , m_startTime(0)
{
    setupUI();
    setTitle(title);
    setDescription(description);
    
    // Setup update timer
    connect(m_updateTimer, &QTimer::timeout, this, &ProgressDialog::onUpdateTimer);
    m_updateTimer->setInterval(1000); // Update every second
    
    // Setup progress animation
    m_progressAnimation = new QMovie(":/animations/progress.gif", QByteArray(), this);
    if (m_progressAnimation->isValid()) {
        m_animationLabel->setMovie(m_progressAnimation);
        connect(m_progressAnimation, &QMovie::frameChanged, this, &ProgressDialog::onAnimationUpdate);
    } else {
        // Fallback to static icon if animation not available
        m_animationLabel->setPixmap(getOperationIcon().pixmap(32, 32));
    }
}

ProgressDialog::~ProgressDialog()
{
    if (m_progressAnimation) {
        m_progressAnimation->stop();
    }
    m_updateTimer->stop();
}

void ProgressDialog::setTitle(const QString &title)
{
    setWindowTitle(title);
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

void ProgressDialog::setDescription(const QString &description)
{
    if (m_descriptionLabel) {
        m_descriptionLabel->setText(description);
        m_descriptionLabel->setVisible(!description.isEmpty());
    }
}

void ProgressDialog::setStatus(const QString &status)
{
    if (m_statusLabel) {
        m_statusLabel->setText(status);
        m_statusLabel->setVisible(!status.isEmpty());
    }
}

void ProgressDialog::setDetails(const QString &details)
{
    if (m_detailsLabel) {
        m_detailsLabel->setText(details);
        m_detailsLabel->setVisible(!details.isEmpty());
    }
}

void ProgressDialog::setProgress(int value, int maximum)
{
    if (m_progressBar) {
        m_progressBar->setRange(0, maximum);
        m_progressBar->setValue(value);
        
        // Calculate estimated time remaining if we have progress
        if (maximum > 0 && value > 0 && m_elapsedTimer.isValid()) {
            qint64 elapsed = m_elapsedTimer.elapsed();
            if (elapsed > 1000) { // Only calculate after 1 second
                double progressRatio = static_cast<double>(value) / maximum;
                if (progressRatio > 0.01) { // Only if we have meaningful progress
                    qint64 estimatedTotal = elapsed / progressRatio;
                    m_estimatedTimeRemaining = (estimatedTotal - elapsed) / 1000;
                }
            }
        }
    }
}

void ProgressDialog::setIndeterminate(bool indeterminate)
{
    if (m_progressBar) {
        if (indeterminate) {
            m_progressBar->setRange(0, 0);
        } else {
            m_progressBar->setRange(0, 100);
            m_progressBar->setValue(0);
        }
    }
    
    m_estimatedTimeRemaining = -1; // Unknown for indeterminate progress
}

void ProgressDialog::setCancellable(bool cancellable)
{
    m_cancellable = cancellable;
    if (m_cancelButton) {
        m_cancelButton->setVisible(cancellable && !m_completed);
    }
}

bool ProgressDialog::wasCancelled() const
{
    return m_cancelled;
}

void ProgressDialog::setEstimatedTimeRemaining(int seconds)
{
    m_estimatedTimeRemaining = seconds;
}

void ProgressDialog::startProgress()
{
    m_elapsedTimer.start();
    m_startTime = QDateTime::currentMSecsSinceEpoch();
    m_updateTimer->start();
    
    if (m_progressAnimation && m_progressAnimation->isValid()) {
        m_progressAnimation->start();
    }
    
    updateDialogState();
    show();
    raise();
    activateWindow();
}

void ProgressDialog::completeSuccess(const QString &message)
{
    m_completed = true;
    m_hasError = false;
    
    if (m_progressAnimation) {
        m_progressAnimation->stop();
    }
    m_updateTimer->stop();
    
    if (m_progressBar) {
        m_progressBar->setValue(m_progressBar->maximum());
    }
    
    QString statusText = message.isEmpty() ? tr("Operation completed successfully") : message;
    setStatus(statusText);
    
    // Change icon to success
    if (m_iconLabel) {
        QIcon successIcon = QIcon::fromTheme("dialog-ok-apply", style()->standardIcon(QStyle::SP_DialogApplyButton));
        m_iconLabel->setPixmap(successIcon.pixmap(32, 32));
    }
    
    updateDialogState();
    emit completed();
    
    // Auto-close after 2 seconds for successful operations
    QTimer::singleShot(2000, this, [this]() {
        if (!m_hasError) {
            accept();
        }
    });
}

void ProgressDialog::completeError(const QString &errorMessage)
{
    m_completed = true;
    m_hasError = true;
    m_errorMessage = errorMessage;
    
    if (m_progressAnimation) {
        m_progressAnimation->stop();
    }
    m_updateTimer->stop();
    
    setStatus(tr("Operation failed: %1").arg(errorMessage));
    
    // Change icon to error
    if (m_iconLabel) {
        QIcon errorIcon = QIcon::fromTheme("dialog-error", style()->standardIcon(QStyle::SP_MessageBoxCritical));
        m_iconLabel->setPixmap(errorIcon.pixmap(32, 32));
    }
    
    updateDialogState();
    emit failed(errorMessage);
}

void ProgressDialog::updateProgress(int value, const QString &status, const QString &details)
{
    setProgress(value);
    setStatus(status);
    if (!details.isEmpty()) {
        setDetails(details);
    }
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
    if (!m_completed && m_cancellable) {
        // Treat close as cancellation
        onCancelClicked();
        event->ignore();
    } else {
        event->accept();
    }
}

void ProgressDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (!m_completed && m_cancellable) {
            onCancelClicked();
        } else if (m_completed) {
            accept();
        }
        return;
    }
    
    QDialog::keyPressEvent(event);
}

void ProgressDialog::onCancelClicked()
{
    if (m_completed) {
        reject();
        return;
    }
    
    if (!m_cancellable) {
        return;
    }
    
    m_cancelled = true;
    setStatus(tr("Cancelling operation..."));
    
    if (m_cancelButton) {
        m_cancelButton->setEnabled(false);
        m_cancelButton->setText(tr("Cancelling..."));
    }
    
    emit cancelled();
}

void ProgressDialog::onUpdateTimer()
{
    updateTimeDisplay();
}

void ProgressDialog::onAnimationUpdate()
{
    // Animation frame updated - could add custom logic here if needed
}

void ProgressDialog::setupUI()
{
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(400, 200);
    setMinimumSize(350, 150);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    
    // Header with icon and title
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(32, 32);
    m_iconLabel->setScaledContents(true);
    m_iconLabel->setPixmap(getOperationIcon().pixmap(32, 32));
    headerLayout->addWidget(m_iconLabel);
    
    QVBoxLayout *titleLayout = new QVBoxLayout();
    
    m_titleLabel = new QLabel();
    m_titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 12pt; }");
    titleLayout->addWidget(m_titleLabel);
    
    m_descriptionLabel = new QLabel();
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setStyleSheet("QLabel { color: gray; }");
    titleLayout->addWidget(m_descriptionLabel);
    
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    
    // Animation label (for indeterminate progress)
    m_animationLabel = new QLabel();
    m_animationLabel->setFixedSize(24, 24);
    m_animationLabel->setScaledContents(true);
    headerLayout->addWidget(m_animationLabel);
    
    mainLayout->addLayout(headerLayout);
    
    // Progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setTextVisible(true);
    mainLayout->addWidget(m_progressBar);
    
    // Status and details
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("QLabel { font-weight: bold; }");
    mainLayout->addWidget(m_statusLabel);
    
    m_detailsLabel = new QLabel();
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    mainLayout->addWidget(m_detailsLabel);
    
    // Time display
    m_timeLabel = new QLabel();
    m_timeLabel->setAlignment(Qt::AlignRight);
    m_timeLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    mainLayout->addWidget(m_timeLabel);
    
    mainLayout->addStretch();
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton(tr("Cancel"));
    connect(m_cancelButton, &QPushButton::clicked, this, &ProgressDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);
    
    m_closeButton = new QPushButton(tr("Close"));
    m_closeButton->setVisible(false);
    connect(m_closeButton, &QPushButton::clicked, this, &ProgressDialog::accept);
    buttonLayout->addWidget(m_closeButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Initially hide optional elements
    m_descriptionLabel->setVisible(false);
    m_statusLabel->setVisible(false);
    m_detailsLabel->setVisible(false);
}

void ProgressDialog::updateTimeDisplay()
{
    if (!m_elapsedTimer.isValid()) {
        return;
    }
    
    qint64 elapsed = m_elapsedTimer.elapsed() / 1000; // Convert to seconds
    QString timeText = tr("Elapsed: %1").arg(formatDuration(elapsed));
    
    if (m_estimatedTimeRemaining >= 0) {
        timeText += tr(" | Remaining: %1").arg(formatDuration(m_estimatedTimeRemaining));
    }
    
    if (m_timeLabel) {
        m_timeLabel->setText(timeText);
    }
}

QString ProgressDialog::formatDuration(int seconds) const
{
    if (seconds < 60) {
        return tr("%1s").arg(seconds);
    } else if (seconds < 3600) {
        int minutes = seconds / 60;
        int remainingSeconds = seconds % 60;
        return tr("%1m %2s").arg(minutes).arg(remainingSeconds);
    } else {
        int hours = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        return tr("%1h %2m").arg(hours).arg(minutes);
    }
}

QIcon ProgressDialog::getOperationIcon() const
{
    switch (m_operationType) {
    case OperationType::ShareCreation:
        return QIcon::fromTheme("folder-new", style()->standardIcon(QStyle::SP_DirIcon));
    case OperationType::ShareRemoval:
        return QIcon::fromTheme("edit-delete", style()->standardIcon(QStyle::SP_TrashIcon));
    case OperationType::ShareConfiguration:
        return QIcon::fromTheme("configure", style()->standardIcon(QStyle::SP_ComputerIcon));
    case OperationType::MountOperation:
        return QIcon::fromTheme("drive-harddisk", style()->standardIcon(QStyle::SP_DriveHDIcon));
    case OperationType::UnmountOperation:
        return QIcon::fromTheme("drive-harddisk-unmounted", style()->standardIcon(QStyle::SP_DriveHDIcon));
    case OperationType::NetworkDiscovery:
        return QIcon::fromTheme("network-server", style()->standardIcon(QStyle::SP_ComputerIcon));
    case OperationType::FileSystemOperation:
        return QIcon::fromTheme("folder", style()->standardIcon(QStyle::SP_DirIcon));
    case OperationType::SystemConfiguration:
        return QIcon::fromTheme("preferences-system", style()->standardIcon(QStyle::SP_ComputerIcon));
    default:
        return QIcon::fromTheme("system-run", style()->standardIcon(QStyle::SP_ComputerIcon));
    }
}

void ProgressDialog::updateDialogState()
{
    if (m_completed) {
        // Operation completed
        if (m_cancelButton) {
            m_cancelButton->setVisible(false);
        }
        if (m_closeButton) {
            m_closeButton->setVisible(true);
            m_closeButton->setDefault(true);
        }
        
        // Hide animation
        if (m_animationLabel) {
            m_animationLabel->setVisible(false);
        }
    } else {
        // Operation in progress
        if (m_cancelButton) {
            m_cancelButton->setVisible(m_cancellable);
            m_cancelButton->setEnabled(!m_cancelled);
        }
        if (m_closeButton) {
            m_closeButton->setVisible(false);
        }
        
        // Show animation for indeterminate progress
        if (m_animationLabel && m_progressBar) {
            bool indeterminate = (m_progressBar->minimum() == 0 && m_progressBar->maximum() == 0);
            m_animationLabel->setVisible(indeterminate);
        }
    }
}

} // namespace NFSShareManager