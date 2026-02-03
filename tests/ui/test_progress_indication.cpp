#include <QtTest/QtTest>
#include <QApplication>
#include <QTimer>
#include <QSignalSpy>
#include "../../src/ui/progressdialog.h"
#include "../../src/ui/operationmanager.h"

using namespace NFSShareManager;

class TestProgressIndication : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testProgressDialog();
    void testProgressDialogCancellation();
    void testOperationManager();
    void testOperationManagerCancellation();
    void testMultipleOperations();

private:
    QApplication *m_app;
};

void TestProgressIndication::initTestCase()
{
    // QApplication is needed for UI components
    if (!QApplication::instance()) {
        int argc = 0;
        char **argv = nullptr;
        m_app = new QApplication(argc, argv);
    } else {
        m_app = nullptr;
    }
}

void TestProgressIndication::cleanupTestCase()
{
    if (m_app) {
        delete m_app;
        m_app = nullptr;
    }
}

void TestProgressIndication::testProgressDialog()
{
    ProgressDialog dialog("Test Operation", "Testing progress dialog functionality");
    
    // Test initial state
    QVERIFY(!dialog.wasCancelled());
    
    // Test setting progress
    dialog.setProgress(50);
    dialog.setStatus("Processing...");
    dialog.setDetails("Processing item 5 of 10");
    
    // Test completion
    QSignalSpy completedSpy(&dialog, &ProgressDialog::completed);
    dialog.completeSuccess("Operation completed successfully");
    QCOMPARE(completedSpy.count(), 1);
}

void TestProgressIndication::testProgressDialogCancellation()
{
    ProgressDialog dialog("Cancellable Operation", "Testing cancellation");
    dialog.setCancellable(true);
    
    QSignalSpy cancelledSpy(&dialog, &ProgressDialog::cancelled);
    
    // Simulate user cancellation
    dialog.onCancelClicked();
    
    QVERIFY(dialog.wasCancelled());
    QCOMPARE(cancelledSpy.count(), 1);
}

void TestProgressIndication::testOperationManager()
{
    OperationManager manager;
    manager.setShowProgressDialogs(false); // Don't show dialogs during testing
    
    QSignalSpy startedSpy(&manager, &OperationManager::operationStarted);
    QSignalSpy progressSpy(&manager, &OperationManager::operationProgressUpdated);
    QSignalSpy completedSpy(&manager, &OperationManager::operationCompleted);
    
    // Start an operation
    QUuid operationId = manager.startOperation("Test Operation", "Testing operation manager");
    QVERIFY(!operationId.isNull());
    QCOMPARE(startedSpy.count(), 1);
    QVERIFY(manager.hasOperation(operationId));
    
    // Update progress
    manager.updateProgress(operationId, 50, "Half way done");
    QCOMPARE(progressSpy.count(), 1);
    
    // Complete operation
    manager.completeOperation(operationId, "Test completed");
    QCOMPARE(completedSpy.count(), 1);
}

void TestProgressIndication::testOperationManagerCancellation()
{
    OperationManager manager;
    manager.setShowProgressDialogs(false);
    
    bool cancelCallbackCalled = false;
    auto cancelCallback = [&cancelCallbackCalled]() {
        cancelCallbackCalled = true;
    };
    
    QSignalSpy cancelledSpy(&manager, &OperationManager::operationCancelled);
    
    // Start cancellable operation
    QUuid operationId = manager.startOperation("Cancellable Operation", 
                                              "Testing cancellation", 
                                              true, 
                                              cancelCallback);
    
    // Cancel the operation
    manager.cancelOperation(operationId);
    
    QVERIFY(cancelCallbackCalled);
    QCOMPARE(cancelledSpy.count(), 1);
}

void TestProgressIndication::testMultipleOperations()
{
    OperationManager manager;
    manager.setShowProgressDialogs(false);
    
    // Start multiple operations
    QUuid op1 = manager.startOperation("Operation 1", "First operation");
    QUuid op2 = manager.startOperation("Operation 2", "Second operation");
    QUuid op3 = manager.startOperation("Operation 3", "Third operation");
    
    QList<QUuid> activeOps = manager.getActiveOperations();
    QCOMPARE(activeOps.size(), 3);
    QVERIFY(activeOps.contains(op1));
    QVERIFY(activeOps.contains(op2));
    QVERIFY(activeOps.contains(op3));
    
    // Complete one operation
    manager.completeOperation(op2, "Second operation completed");
    activeOps = manager.getActiveOperations();
    QCOMPARE(activeOps.size(), 2);
    QVERIFY(!activeOps.contains(op2));
    
    // Cancel all remaining operations
    manager.cancelAllOperations();
    activeOps = manager.getActiveOperations();
    QCOMPARE(activeOps.size(), 0);
}

QTEST_MAIN(TestProgressIndication)
#include "test_progress_indication.moc"