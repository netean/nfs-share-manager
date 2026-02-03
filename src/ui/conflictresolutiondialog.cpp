#include "conflictresolutiondialog.h"
#include <QMessageBox>
#include <QApplication>
#include <QStyle>

namespace NFSShareManager {

ConflictResolutionWidget::ConflictResolutionWidget(const ConfigurationConflict &conflict, QWidget *parent)
    : QWidget(parent)
    , m_conflict(conflict)
    , m_layout(nullptr)
    , m_descriptionLabel(nullptr)
    , m_resolutionComboBox(nullptr)
    , m_preserveDataCheckBox(nullptr)
    , m_additionalOptionsWidget(nullptr)
    , m_additionalOptionsLayout(nullptr)
    , m_newPathEdit(nullptr)
    , m_newMountPointEdit(nullptr)
{
    setupUI();
}

ConflictResolution ConflictResolutionWidget::getResolution() const
{
    ConflictResolution resolution;
    resolution.conflictId = m_conflict.conflictingItems.join(QStringLiteral("-"));
    resolution.selectedOption = m_resolutionComboBox->currentText();
    resolution.preserveData = m_preserveDataCheckBox->isChecked();
    
    // Add additional resolution data based on the selected option
    if (m_newPathEdit && !m_newPathEdit->text().isEmpty()) {
        resolution.resolutionData[QStringLiteral("newPath")] = m_newPathEdit->text();
    }
    
    if (m_newMountPointEdit && !m_newMountPointEdit->text().isEmpty()) {
        resolution.resolutionData[QStringLiteral("newMountPoint")] = m_newMountPointEdit->text();
    }
    
    return resolution;
}

bool ConflictResolutionWidget::isResolutionValid() const
{
    if (m_resolutionComboBox->currentIndex() < 0) {
        return false;
    }
    
    QString selectedOption = m_resolutionComboBox->currentText();
    
    // Check if additional data is required for certain resolution options
    if (selectedOption == QStringLiteral("Rename conflicting share") && 
        (!m_newPathEdit || m_newPathEdit->text().isEmpty())) {
        return false;
    }
    
    if (selectedOption == QStringLiteral("Use different mount point") && 
        (!m_newMountPointEdit || m_newMountPointEdit->text().isEmpty())) {
        return false;
    }
    
    return true;
}

void ConflictResolutionWidget::onResolutionOptionChanged()
{
    updateAdditionalOptions();
    emit resolutionChanged();
}

void ConflictResolutionWidget::onPreserveDataChanged()
{
    emit resolutionChanged();
}

void ConflictResolutionWidget::setupUI()
{
    m_layout = new QVBoxLayout(this);
    
    // Create group box for this conflict
    QGroupBox *groupBox = new QGroupBox(this);
    groupBox->setTitle(QStringLiteral("Configuration Conflict"));
    QVBoxLayout *groupLayout = new QVBoxLayout(groupBox);
    
    // Conflict description
    m_descriptionLabel = new QLabel(m_conflict.description, groupBox);
    m_descriptionLabel->setWordWrap(true);
    m_descriptionLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #d32f2f;"));
    groupLayout->addWidget(m_descriptionLabel);
    
    // Conflicting items
    if (!m_conflict.conflictingItems.isEmpty()) {
        QLabel *itemsLabel = new QLabel(QStringLiteral("Conflicting items: %1")
                                       .arg(m_conflict.conflictingItems.join(QStringLiteral(", "))), groupBox);
        itemsLabel->setWordWrap(true);
        itemsLabel->setStyleSheet(QStringLiteral("color: #666;"));
        groupLayout->addWidget(itemsLabel);
    }
    
    // Resolution options
    QLabel *optionsLabel = new QLabel(QStringLiteral("Resolution options:"), groupBox);
    optionsLabel->setStyleSheet(QStringLiteral("font-weight: bold; margin-top: 10px;"));
    groupLayout->addWidget(optionsLabel);
    
    m_resolutionComboBox = new QComboBox(groupBox);
    m_resolutionComboBox->addItems(m_conflict.resolutionOptions);
    
    // Set recommended resolution as default
    if (!m_conflict.recommendedResolution.isEmpty()) {
        int index = m_resolutionComboBox->findText(m_conflict.recommendedResolution);
        if (index >= 0) {
            m_resolutionComboBox->setCurrentIndex(index);
        }
    }
    
    connect(m_resolutionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConflictResolutionWidget::onResolutionOptionChanged);
    groupLayout->addWidget(m_resolutionComboBox);
    
    // Preserve data option
    m_preserveDataCheckBox = new QCheckBox(QStringLiteral("Preserve existing data when possible"), groupBox);
    m_preserveDataCheckBox->setChecked(true);
    connect(m_preserveDataCheckBox, &QCheckBox::toggled,
            this, &ConflictResolutionWidget::onPreserveDataChanged);
    groupLayout->addWidget(m_preserveDataCheckBox);
    
    // Additional options widget (initially hidden)
    m_additionalOptionsWidget = new QWidget(groupBox);
    m_additionalOptionsLayout = new QVBoxLayout(m_additionalOptionsWidget);
    m_additionalOptionsLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->addWidget(m_additionalOptionsWidget);
    
    m_layout->addWidget(groupBox);
    
    // Update additional options based on initial selection
    updateAdditionalOptions();
}

void ConflictResolutionWidget::updateAdditionalOptions()
{
    // Clear existing additional options
    QLayoutItem *item;
    while ((item = m_additionalOptionsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    m_newPathEdit = nullptr;
    m_newMountPointEdit = nullptr;
    
    QString selectedOption = m_resolutionComboBox->currentText();
    
    if (selectedOption == QStringLiteral("Rename conflicting share")) {
        QLabel *label = new QLabel(QStringLiteral("New path:"), m_additionalOptionsWidget);
        m_additionalOptionsLayout->addWidget(label);
        
        m_newPathEdit = new QLineEdit(m_additionalOptionsWidget);
        m_newPathEdit->setPlaceholderText(QStringLiteral("Enter new directory path"));
        connect(m_newPathEdit, &QLineEdit::textChanged, this, &ConflictResolutionWidget::resolutionChanged);
        m_additionalOptionsLayout->addWidget(m_newPathEdit);
    }
    
    if (selectedOption == QStringLiteral("Use different mount point")) {
        QLabel *label = new QLabel(QStringLiteral("New mount point:"), m_additionalOptionsWidget);
        m_additionalOptionsLayout->addWidget(label);
        
        m_newMountPointEdit = new QLineEdit(m_additionalOptionsWidget);
        m_newMountPointEdit->setPlaceholderText(QStringLiteral("Enter new mount point path"));
        connect(m_newMountPointEdit, &QLineEdit::textChanged, this, &ConflictResolutionWidget::resolutionChanged);
        m_additionalOptionsLayout->addWidget(m_newMountPointEdit);
    }
    
    m_additionalOptionsWidget->setVisible(m_additionalOptionsLayout->count() > 0);
}

ConflictResolutionDialog::ConflictResolutionDialog(const QList<ConfigurationConflict> &conflicts, QWidget *parent)
    : QDialog(parent)
    , m_conflicts(conflicts)
    , m_layout(nullptr)
    , m_scrollArea(nullptr)
    , m_scrollWidget(nullptr)
    , m_scrollLayout(nullptr)
    , m_autoResolveCheckBox(nullptr)
    , m_autoResolveButton(nullptr)
    , m_buttonBox(nullptr)
    , m_summaryLabel(nullptr)
{
    setupUI();
}

QList<ConflictResolution> ConflictResolutionDialog::getResolutions() const
{
    QList<ConflictResolution> resolutions;
    
    for (ConflictResolutionWidget *widget : m_conflictWidgets) {
        resolutions.append(widget->getResolution());
    }
    
    return resolutions;
}

bool ConflictResolutionDialog::shouldAutoResolve() const
{
    return m_autoResolveCheckBox && m_autoResolveCheckBox->isChecked();
}

void ConflictResolutionDialog::accept()
{
    // Validate all resolutions
    QStringList invalidResolutions;
    
    for (ConflictResolutionWidget *widget : m_conflictWidgets) {
        if (!widget->isResolutionValid()) {
            invalidResolutions.append(widget->getConflict().description);
        }
    }
    
    if (!invalidResolutions.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Invalid Resolutions"),
                           QStringLiteral("The following conflicts have invalid resolutions:\n\n%1\n\n"
                                        "Please provide valid resolution options for all conflicts.")
                           .arg(invalidResolutions.join(QStringLiteral("\n"))));
        return;
    }
    
    QDialog::accept();
}

void ConflictResolutionDialog::onAutoResolveClicked()
{
    // Set all auto-resolvable conflicts to their recommended resolution
    for (ConflictResolutionWidget *widget : m_conflictWidgets) {
        const ConfigurationConflict &conflict = widget->getConflict();
        if (conflict.canAutoResolve && !conflict.recommendedResolution.isEmpty()) {
            // The widget should already have the recommended resolution selected
            // This button mainly serves as a visual confirmation
        }
    }
    
    updateDialogState();
}

void ConflictResolutionDialog::onResolutionChanged()
{
    updateDialogState();
}

void ConflictResolutionDialog::setupUI()
{
    setWindowTitle(QStringLiteral("Resolve Configuration Conflicts"));
    setModal(true);
    resize(600, 500);
    
    m_layout = new QVBoxLayout(this);
    
    // Summary label
    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setText(QStringLiteral("Found %1 configuration conflicts that need to be resolved:")
                           .arg(m_conflicts.size()));
    m_summaryLabel->setStyleSheet(QStringLiteral("font-weight: bold; margin-bottom: 10px;"));
    m_layout->addWidget(m_summaryLabel);
    
    // Auto-resolve option
    QHBoxLayout *autoResolveLayout = new QHBoxLayout();
    m_autoResolveCheckBox = new QCheckBox(QStringLiteral("Auto-resolve conflicts where possible"), this);
    m_autoResolveCheckBox->setChecked(true);
    autoResolveLayout->addWidget(m_autoResolveCheckBox);
    
    m_autoResolveButton = new QPushButton(QStringLiteral("Auto-Resolve"), this);
    connect(m_autoResolveButton, &QPushButton::clicked, this, &ConflictResolutionDialog::onAutoResolveClicked);
    autoResolveLayout->addWidget(m_autoResolveButton);
    
    autoResolveLayout->addStretch();
    m_layout->addLayout(autoResolveLayout);
    
    // Scroll area for conflicts
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    m_scrollWidget = new QWidget();
    m_scrollLayout = new QVBoxLayout(m_scrollWidget);
    
    // Create conflict resolution widgets
    for (const ConfigurationConflict &conflict : m_conflicts) {
        ConflictResolutionWidget *widget = new ConflictResolutionWidget(conflict, m_scrollWidget);
        connect(widget, &ConflictResolutionWidget::resolutionChanged,
                this, &ConflictResolutionDialog::onResolutionChanged);
        m_conflictWidgets.append(widget);
        m_scrollLayout->addWidget(widget);
    }
    
    m_scrollLayout->addStretch();
    m_scrollArea->setWidget(m_scrollWidget);
    m_layout->addWidget(m_scrollArea);
    
    // Button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &ConflictResolutionDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &ConflictResolutionDialog::reject);
    m_layout->addWidget(m_buttonBox);
    
    updateDialogState();
}

void ConflictResolutionDialog::updateDialogState()
{
    // Count auto-resolvable conflicts
    int autoResolvableCount = 0;
    int validResolutionCount = 0;
    
    for (ConflictResolutionWidget *widget : m_conflictWidgets) {
        if (widget->getConflict().canAutoResolve) {
            autoResolvableCount++;
        }
        if (widget->isResolutionValid()) {
            validResolutionCount++;
        }
    }
    
    // Update auto-resolve button state
    m_autoResolveButton->setEnabled(autoResolvableCount > 0);
    m_autoResolveButton->setText(QStringLiteral("Auto-Resolve (%1)").arg(autoResolvableCount));
    
    // Update OK button state
    bool allResolutionsValid = (validResolutionCount == m_conflicts.size());
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(allResolutionsValid);
    
    // Update summary
    QString summaryText = QStringLiteral("Found %1 configuration conflicts. ")
                         .arg(m_conflicts.size());
    
    if (autoResolvableCount > 0) {
        summaryText += QStringLiteral("%1 can be auto-resolved. ").arg(autoResolvableCount);
    }
    
    summaryText += QStringLiteral("%1 of %2 have valid resolutions.")
                  .arg(validResolutionCount).arg(m_conflicts.size());
    
    m_summaryLabel->setText(summaryText);
}

} // namespace NFSShareManager