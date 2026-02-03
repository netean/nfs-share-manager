#include <QtTest/QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>

#include "../../src/ui/permissionmanagerdialog.h"
#include "../../src/core/permissionset.h"
#include "../../src/core/types.h"

using namespace NFSShareManager;

class TestPermissionManagerDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic functionality tests
    void testDialogCreation();
    void testSetAndGetPermissions();
    void testDefaultValues();
    void testValidation();

    // Host permission tests
    void testAddHostPermission();
    void testRemoveHostPermission();
    void testHostValidation();
    void testHostAutoCompletion();

    // User permission tests
    void testAddUserPermission();
    void testRemoveUserPermission();
    void testUserValidation();
    void testUserAutoCompletion();

    // Advanced features tests
    void testPresets();
    void testImportExport();
    void testRealTimeValidation();
    void testExportPreview();

    // Integration tests
    void testComplexConfiguration();
    void testErrorHandling();

private:
    QApplication *m_app;
    PermissionManagerDialog *m_dialog;
};

void TestPermissionManagerDialog::initTestCase()
{
    // QApplication is needed for GUI tests
    if (!QApplication::instance()) {
        int argc = 0;
        char **argv = nullptr;
        m_app = new QApplication(argc, argv);
    } else {
        m_app = nullptr;
    }
}

void TestPermissionManagerDialog::cleanupTestCase()
{
    delete m_app;
}

void TestPermissionManagerDialog::init()
{
    m_dialog = new PermissionManagerDialog();
    // Don't show the dialog during tests
    m_dialog->hide();
}

void TestPermissionManagerDialog::cleanup()
{
    delete m_dialog;
    m_dialog = nullptr;
}

void TestPermissionManagerDialog::testDialogCreation()
{
    QVERIFY(m_dialog != nullptr);
    QVERIFY(m_dialog->isModal());
    QCOMPARE(m_dialog->windowTitle(), QString("Permission Manager"));
    
    // Check that all tabs are present
    QTabWidget *tabWidget = m_dialog->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
    QVERIFY(tabWidget->count() >= 4); // Basic, Host, User, Advanced tabs
    
    // Check tab names
    QCOMPARE(tabWidget->tabText(0), QString("Basic Permissions"));
    QCOMPARE(tabWidget->tabText(1), QString("Host Permissions"));
    QCOMPARE(tabWidget->tabText(2), QString("User Permissions"));
    QCOMPARE(tabWidget->tabText(3), QString("Advanced Options"));
}

void TestPermissionManagerDialog::testSetAndGetPermissions()
{
    // Create a test permission set
    PermissionSet testPermissions(AccessMode::ReadWrite);
    testPermissions.setEnableRootSquash(false);
    testPermissions.setAnonymousUser("testuser");
    testPermissions.setHostPermission("192.168.1.100", AccessMode::ReadOnly);
    testPermissions.setUserPermission("alice", AccessMode::ReadWrite);
    
    // Set permissions in dialog
    m_dialog->setPermissions(testPermissions);
    
    // Get permissions back
    PermissionSet retrievedPermissions = m_dialog->getPermissions();
    
    // Verify they match
    QCOMPARE(retrievedPermissions.defaultAccess(), AccessMode::ReadWrite);
    QCOMPARE(retrievedPermissions.enableRootSquash(), false);
    QCOMPARE(retrievedPermissions.anonymousUser(), QString("testuser"));
    QCOMPARE(retrievedPermissions.getEffectiveAccessForHost("192.168.1.100"), AccessMode::ReadOnly);
    QCOMPARE(retrievedPermissions.getEffectiveAccessForUser("alice"), AccessMode::ReadWrite);
}

void TestPermissionManagerDialog::testDefaultValues()
{
    PermissionSet defaultPermissions = m_dialog->getPermissions();
    
    QCOMPARE(defaultPermissions.defaultAccess(), AccessMode::ReadOnly);
    QCOMPARE(defaultPermissions.enableRootSquash(), true);
    QCOMPARE(defaultPermissions.anonymousUser(), QString("nobody"));
    QVERIFY(defaultPermissions.hostPermissions().isEmpty());
    QVERIFY(defaultPermissions.userPermissions().isEmpty());
}

void TestPermissionManagerDialog::testValidation()
{
    // Test with valid configuration
    QVERIFY(m_dialog->isValid());
    QVERIFY(m_dialog->getValidationErrors().isEmpty());
    
    // Test with invalid anonymous user
    QLineEdit *anonUserEdit = m_dialog->findChild<QLineEdit*>("m_anonymousUserEdit");
    if (anonUserEdit) {
        anonUserEdit->setText("invalid-user-name-with-invalid-chars!");
        
        // Trigger validation
        QTest::keyClick(anonUserEdit, Qt::Key_Tab);
        QTest::qWait(100);
        
        QVERIFY(!m_dialog->isValid());
        QVERIFY(!m_dialog->getValidationErrors().isEmpty());
    }
}

void TestPermissionManagerDialog::testAddHostPermission()
{
    // Find the host input controls
    QLineEdit *hostEdit = m_dialog->findChild<QLineEdit*>();
    QComboBox *accessCombo = m_dialog->findChild<QComboBox*>();
    QPushButton *addButton = m_dialog->findChild<QPushButton*>();
    QTableWidget *hostTable = m_dialog->findChild<QTableWidget*>();
    
    if (hostEdit && accessCombo && addButton && hostTable) {
        // Set up to add a host permission
        hostEdit->setText("192.168.1.100");
        accessCombo->setCurrentIndex(2); // ReadWrite
        
        int initialRowCount = hostTable->rowCount();
        
        // Click add button
        QTest::mouseClick(addButton, Qt::LeftButton);
        
        // Verify host was added
        QCOMPARE(hostTable->rowCount(), initialRowCount + 1);
        
        // Verify the permission was set correctly
        PermissionSet permissions = m_dialog->getPermissions();
        QCOMPARE(permissions.getEffectiveAccessForHost("192.168.1.100"), AccessMode::ReadWrite);
    }
}

void TestPermissionManagerDialog::testRemoveHostPermission()
{
    // First add a host permission
    testAddHostPermission();
    
    QTableWidget *hostTable = m_dialog->findChild<QTableWidget*>();
    QPushButton *removeButton = nullptr;
    
    // Find remove button (look for button with "Remove" in text)
    QList<QPushButton*> buttons = m_dialog->findChildren<QPushButton*>();
    for (QPushButton *button : buttons) {
        if (button->text().contains("Remove")) {
            removeButton = button;
            break;
        }
    }
    
    if (hostTable && removeButton) {
        // Select first row
        hostTable->selectRow(0);
        
        int initialRowCount = hostTable->rowCount();
        
        // Click remove button
        QTest::mouseClick(removeButton, Qt::LeftButton);
        
        // Verify host was removed
        QCOMPARE(hostTable->rowCount(), initialRowCount - 1);
    }
}

void TestPermissionManagerDialog::testHostValidation()
{
    QLineEdit *hostEdit = m_dialog->findChild<QLineEdit*>();
    
    if (hostEdit) {
        // Test valid IP address
        hostEdit->setText("192.168.1.100");
        QTest::keyClick(hostEdit, Qt::Key_Tab);
        QTest::qWait(100);
        
        // Should be valid
        QVERIFY(m_dialog->isValid());
        
        // Test invalid IP address
        hostEdit->setText("999.999.999.999");
        QTest::keyClick(hostEdit, Qt::Key_Tab);
        QTest::qWait(100);
        
        // Should be invalid
        QVERIFY(!m_dialog->isValid());
        
        // Test valid hostname
        hostEdit->setText("example.com");
        QTest::keyClick(hostEdit, Qt::Key_Tab);
        QTest::qWait(100);
        
        // Should be valid
        QVERIFY(m_dialog->isValid());
        
        // Test valid network range
        hostEdit->setText("192.168.1.0/24");
        QTest::keyClick(hostEdit, Qt::Key_Tab);
        QTest::qWait(100);
        
        // Should be valid
        QVERIFY(m_dialog->isValid());
    }
}

void TestPermissionManagerDialog::testHostAutoCompletion()
{
    QLineEdit *hostEdit = m_dialog->findChild<QLineEdit*>();
    
    if (hostEdit && hostEdit->completer()) {
        QCompleter *completer = hostEdit->completer();
        QVERIFY(completer != nullptr);
        
        // Test that completer has suggestions
        QAbstractItemModel *model = completer->model();
        QVERIFY(model != nullptr);
        QVERIFY(model->rowCount() > 0);
        
        // Test that common suggestions are present
        QStringList suggestions;
        for (int i = 0; i < model->rowCount(); ++i) {
            suggestions.append(model->data(model->index(i, 0)).toString());
        }
        
        QVERIFY(suggestions.contains("localhost"));
        QVERIFY(suggestions.contains("192.168.1.0/24"));
    }
}

void TestPermissionManagerDialog::testAddUserPermission()
{
    // Similar to testAddHostPermission but for users
    // This would test the user permission functionality
    
    // Find user-specific controls
    QList<QLineEdit*> lineEdits = m_dialog->findChildren<QLineEdit*>();
    QLineEdit *userEdit = nullptr;
    
    // Look for user input field (might need better identification)
    for (QLineEdit *edit : lineEdits) {
        if (edit->placeholderText().contains("username")) {
            userEdit = edit;
            break;
        }
    }
    
    if (userEdit) {
        userEdit->setText("alice");
        
        // Find and click add user button
        QList<QPushButton*> buttons = m_dialog->findChildren<QPushButton*>();
        for (QPushButton *button : buttons) {
            if (button->text() == "Add" && button->isVisible()) {
                QTest::mouseClick(button, Qt::LeftButton);
                break;
            }
        }
        
        // Verify user was added
        PermissionSet permissions = m_dialog->getPermissions();
        QVERIFY(permissions.userPermissions().contains("alice"));
    }
}

void TestPermissionManagerDialog::testRemoveUserPermission()
{
    // First add a user permission
    testAddUserPermission();
    
    // Then test removal (similar to host removal)
    QList<QTableWidget*> tables = m_dialog->findChildren<QTableWidget*>();
    QTableWidget *userTable = nullptr;
    
    // Find user table (second table widget)
    if (tables.size() >= 2) {
        userTable = tables[1];
    }
    
    if (userTable && userTable->rowCount() > 0) {
        userTable->selectRow(0);
        
        // Find and click remove user button
        QList<QPushButton*> buttons = m_dialog->findChildren<QPushButton*>();
        for (QPushButton *button : buttons) {
            if (button->text().contains("Remove") && button->isEnabled()) {
                QTest::mouseClick(button, Qt::LeftButton);
                break;
            }
        }
        
        // Verify user was removed
        PermissionSet permissions = m_dialog->getPermissions();
        QVERIFY(!permissions.userPermissions().contains("alice"));
    }
}

void TestPermissionManagerDialog::testUserValidation()
{
    QList<QLineEdit*> lineEdits = m_dialog->findChildren<QLineEdit*>();
    QLineEdit *userEdit = nullptr;
    
    // Find user input field
    for (QLineEdit *edit : lineEdits) {
        if (edit->placeholderText().contains("username")) {
            userEdit = edit;
            break;
        }
    }
    
    if (userEdit) {
        // Test valid username
        userEdit->setText("alice");
        QTest::keyClick(userEdit, Qt::Key_Tab);
        QTest::qWait(100);
        
        // Test invalid username (starts with number)
        userEdit->setText("123invalid");
        QTest::keyClick(userEdit, Qt::Key_Tab);
        QTest::qWait(100);
        
        // Test invalid username (contains invalid characters)
        userEdit->setText("user@invalid");
        QTest::keyClick(userEdit, Qt::Key_Tab);
        QTest::qWait(100);
    }
}

void TestPermissionManagerDialog::testUserAutoCompletion()
{
    QList<QLineEdit*> lineEdits = m_dialog->findChildren<QLineEdit*>();
    QLineEdit *userEdit = nullptr;
    
    // Find user input field
    for (QLineEdit *edit : lineEdits) {
        if (edit->placeholderText().contains("username")) {
            userEdit = edit;
            break;
        }
    }
    
    if (userEdit && userEdit->completer()) {
        QCompleter *completer = userEdit->completer();
        QVERIFY(completer != nullptr);
        
        // Test that completer has suggestions
        QAbstractItemModel *model = completer->model();
        QVERIFY(model != nullptr);
        QVERIFY(model->rowCount() > 0);
        
        // Test that common suggestions are present
        QStringList suggestions;
        for (int i = 0; i < model->rowCount(); ++i) {
            suggestions.append(model->data(model->index(i, 0)).toString());
        }
        
        QVERIFY(suggestions.contains("nobody"));
        QVERIFY(suggestions.contains("www-data"));
    }
}

void TestPermissionManagerDialog::testPresets()
{
    QComboBox *presetsCombo = m_dialog->findChild<QComboBox*>();
    
    if (presetsCombo) {
        // Test that presets are available
        QVERIFY(presetsCombo->count() > 1); // At least "Custom" + some presets
        
        // Test applying a preset
        int readOnlyIndex = presetsCombo->findText("Read Only (Secure)");
        if (readOnlyIndex >= 0) {
            presetsCombo->setCurrentIndex(readOnlyIndex);
            
            // Verify preset was applied
            PermissionSet permissions = m_dialog->getPermissions();
            QCOMPARE(permissions.defaultAccess(), AccessMode::ReadOnly);
            QCOMPARE(permissions.enableRootSquash(), true);
        }
    }
}

void TestPermissionManagerDialog::testImportExport()
{
    // Test export functionality
    PermissionSet testPermissions(AccessMode::ReadWrite);
    testPermissions.setEnableRootSquash(false);
    m_dialog->setPermissions(testPermissions);
    
    // Export should generate a valid options string
    QString exportString = m_dialog->getPermissions().toExportOptions();
    QVERIFY(!exportString.isEmpty());
    QVERIFY(exportString.contains("rw"));
    QVERIFY(exportString.contains("no_root_squash"));
    
    // Test import functionality (would need to simulate user input)
    // This is more complex to test in unit tests due to dialog interactions
}

void TestPermissionManagerDialog::testRealTimeValidation()
{
    // Test that validation updates in real-time
    QLineEdit *anonUserEdit = m_dialog->findChild<QLineEdit*>();
    
    if (anonUserEdit) {
        // Start with valid state
        QVERIFY(m_dialog->isValid());
        
        // Enter invalid data
        anonUserEdit->setText("invalid@user");
        
        // Trigger validation timer (simulate real-time validation)
        QTest::qWait(600); // Wait for validation timer
        
        // Should now be invalid
        QVERIFY(!m_dialog->isValid());
        
        // Fix the data
        anonUserEdit->setText("validuser");
        QTest::qWait(600);
        
        // Should be valid again
        QVERIFY(m_dialog->isValid());
    }
}

void TestPermissionManagerDialog::testExportPreview()
{
    QTextEdit *previewEdit = m_dialog->findChild<QTextEdit*>();
    
    if (previewEdit) {
        // Set some permissions
        PermissionSet testPermissions(AccessMode::ReadWrite);
        testPermissions.setEnableRootSquash(true);
        m_dialog->setPermissions(testPermissions);
        
        // Check that preview is updated
        QString previewText = previewEdit->toPlainText();
        QVERIFY(!previewText.isEmpty());
        QVERIFY(previewText.contains("rw"));
        QVERIFY(previewText.contains("root_squash"));
    }
}

void TestPermissionManagerDialog::testComplexConfiguration()
{
    // Test a complex configuration with multiple hosts and users
    PermissionSet complexPermissions(AccessMode::ReadOnly);
    complexPermissions.setHostPermission("192.168.1.0/24", AccessMode::ReadWrite);
    complexPermissions.setHostPermission("10.0.0.100", AccessMode::ReadOnly);
    complexPermissions.setUserPermission("admin", AccessMode::ReadWrite);
    complexPermissions.setUserPermission("guest", AccessMode::ReadOnly);
    complexPermissions.setEnableRootSquash(true);
    complexPermissions.setAnonymousUser("nfsnobody");
    
    m_dialog->setPermissions(complexPermissions);
    
    // Verify all settings are preserved
    PermissionSet retrieved = m_dialog->getPermissions();
    QCOMPARE(retrieved.defaultAccess(), AccessMode::ReadOnly);
    QCOMPARE(retrieved.getEffectiveAccessForHost("192.168.1.100"), AccessMode::ReadWrite); // In range
    QCOMPARE(retrieved.getEffectiveAccessForHost("10.0.0.100"), AccessMode::ReadOnly);
    QCOMPARE(retrieved.getEffectiveAccessForUser("admin"), AccessMode::ReadWrite);
    QCOMPARE(retrieved.getEffectiveAccessForUser("guest"), AccessMode::ReadOnly);
    QCOMPARE(retrieved.enableRootSquash(), true);
    QCOMPARE(retrieved.anonymousUser(), QString("nfsnobody"));
}

void TestPermissionManagerDialog::testErrorHandling()
{
    // Test various error conditions
    
    // Test with empty anonymous user
    QLineEdit *anonUserEdit = m_dialog->findChild<QLineEdit*>();
    if (anonUserEdit) {
        anonUserEdit->setText("");
        QTest::qWait(600);
        
        // Should be invalid
        QVERIFY(!m_dialog->isValid());
        QVERIFY(!m_dialog->getValidationErrors().isEmpty());
    }
    
    // Test dialog acceptance with invalid configuration
    // The dialog should not accept invalid configurations
    if (!m_dialog->isValid()) {
        // Try to accept the dialog - it should remain open
        m_dialog->accept();
        QVERIFY(m_dialog->isVisible()); // Dialog should still be visible
    }
}

QTEST_MAIN(TestPermissionManagerDialog)
#include "test_permissionmanagerdialog.moc"