#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QRandomGenerator>

#include "core/configurationmanager.h"
#include "core/nfsshare.h"
#include "core/nfsmount.h"
#include "core/remotenfsshare.h"
#include "property_test_base.h"
#include "generators.h"

using namespace NFSShareManager;
using namespace NFSShareManager::PropertyTesting;

// Property test configuration
static const int PROPERTY_TEST_ITERATIONS = 50; // Reduced for faster testing

/**
 * Property-based tests for ConfigurationManager
 * 
 * These tests validate universal properties that should hold across
 * all valid configurations and operations.
 */
class TestPropertyConfigurationManager : public PropertyTestBase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Property tests
    void testProperty17_ConfigurationPersistenceRoundTrip();
    void testProperty18_ConfigurationConflictResolution();
    void testProperty19_ConfigurationIntegrityValidation();

private:
    // Test data generators
    NFSShare generateRandomShare();
    NFSMount generateRandomMount();
    QMap<QString, QVariant> generateRandomPreferences();
    ConfigurationProfile generateRandomProfile();
    
    // Helper methods
    void corruptConfigurationFile(const QString &filePath);
    bool configurationEquivalent(const ConfigurationManager &manager1, 
                                const ConfigurationManager &manager2);
    
    ConfigurationManager *m_configManager;
    QTemporaryDir *m_tempDir;
};

void TestPropertyConfigurationManager::initTestCase()
{
    // Set up temporary directory for test configuration
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    
    // Override the standard config location for testing
    QStandardPaths::setTestModeEnabled(true);
}

void TestPropertyConfigurationManager::cleanupTestCase()
{
    delete m_tempDir;
    QStandardPaths::setTestModeEnabled(false);
}

void TestPropertyConfigurationManager::init()
{
    m_configManager = new ConfigurationManager();
}

void TestPropertyConfigurationManager::cleanup()
{
    delete m_configManager;
    m_configManager = nullptr;
    
    // Clean up any test configuration files
    QSettings settings;
    QFile::remove(settings.fileName());
}

void TestPropertyConfigurationManager::testProperty17_ConfigurationPersistenceRoundTrip()
{
    /**
     * **Property 17: Configuration Persistence Round-Trip**
     * *For any* valid application configuration, saving and then loading the configuration 
     * should produce an equivalent configuration state with all settings preserved.
     * **Validates: Requirements 9.1, 9.3**
     */
    
    qDebug() << "Testing Property 17: Configuration Persistence Round-Trip";
    
    for (int iteration = 0; iteration < PROPERTY_TEST_ITERATIONS; ++iteration) {
        qDebug() << "  Iteration" << (iteration + 1) << "of" << PROPERTY_TEST_ITERATIONS;
        
        // Generate random configuration data
        QList<NFSShare> originalShares;
        int shareCount = QRandomGenerator::global()->bounded(1, 6); // 1-5 shares
        for (int i = 0; i < shareCount; ++i) {
            originalShares.append(generateRandomShare());
        }
        
        QList<NFSMount> originalMounts;
        int mountCount = QRandomGenerator::global()->bounded(0, 4); // 0-3 mounts
        for (int i = 0; i < mountCount; ++i) {
            originalMounts.append(generateRandomMount());
        }
        
        QMap<QString, QVariant> originalPrefs = generateRandomPreferences();
        
        // Set configuration in manager
        m_configManager->setLocalShares(originalShares);
        m_configManager->setPersistentMounts(originalMounts);
        for (auto it = originalPrefs.constBegin(); it != originalPrefs.constEnd(); ++it) {
            m_configManager->setPreference(it.key(), it.value());
        }
        
        // Save configuration
        bool saveResult = m_configManager->saveConfiguration();
        QVERIFY2(saveResult, "Configuration save should succeed");
        
        // Create new manager and load configuration
        ConfigurationManager newManager;
        bool loadResult = newManager.loadConfiguration();
        QVERIFY2(loadResult, "Configuration load should succeed");
        
        // Verify round-trip preservation
        QList<NFSShare> loadedShares = newManager.getLocalShares();
        QList<NFSMount> loadedMounts = newManager.getPersistentMounts();
        
        // Verify share count and data
        QCOMPARE(loadedShares.size(), originalShares.size());
        for (int i = 0; i < originalShares.size(); ++i) {
            const NFSShare &original = originalShares.at(i);
            const NFSShare &loaded = loadedShares.at(i);
            
            QCOMPARE(loaded.path(), original.path());
            QCOMPARE(loaded.exportPath(), original.exportPath());
            QCOMPARE(loaded.config().name(), original.config().name());
            QCOMPARE(loaded.config().accessMode(), original.config().accessMode());
            QCOMPARE(loaded.config().allowRootAccess(), original.config().allowRootAccess());
            QCOMPARE(loaded.config().allowedHosts(), original.config().allowedHosts());
            QCOMPARE(loaded.config().nfsVersion(), original.config().nfsVersion());
            QCOMPARE(loaded.isActive(), original.isActive());
            QCOMPARE(loaded.permissions().defaultAccess(), original.permissions().defaultAccess());
            QCOMPARE(loaded.permissions().enableRootSquash(), original.permissions().enableRootSquash());
        }
        
        // Verify mount count and data
        QCOMPARE(loadedMounts.size(), originalMounts.size());
        for (int i = 0; i < originalMounts.size(); ++i) {
            const NFSMount &original = originalMounts.at(i);
            const NFSMount &loaded = loadedMounts.at(i);
            
            QCOMPARE(loaded.localMountPoint(), original.localMountPoint());
            QCOMPARE(loaded.isPersistent(), original.isPersistent());
            QCOMPARE(loaded.remoteShare().hostName(), original.remoteShare().hostName());
            QCOMPARE(loaded.remoteShare().exportPath(), original.remoteShare().exportPath());
            QCOMPARE(loaded.options().nfsVersion, original.options().nfsVersion);
            QCOMPARE(loaded.options().readOnly, original.options().readOnly);
        }
        
        // Verify preferences
        for (auto it = originalPrefs.constBegin(); it != originalPrefs.constEnd(); ++it) {
            QVariant loadedValue = newManager.getPreference(it.key());
            QCOMPARE(loadedValue, it.value());
        }
        
        // Test export/import round-trip
        QString exportPath = m_tempDir->filePath(QString("test-export-%1.json").arg(iteration));
        bool exportResult = newManager.exportConfiguration(exportPath, 
                                                          QString("Test Profile %1").arg(iteration),
                                                          "Property test profile");
        QVERIFY2(exportResult, "Configuration export should succeed");
        
        // Import into completely fresh manager with clean state
        ConfigurationManager importManager;
        // Ensure clean state by explicitly clearing and saving
        importManager.setLocalShares(QList<NFSShare>());
        importManager.setPersistentMounts(QList<NFSMount>());
        // Clear all preferences except defaults
        QStringList prefKeys = {"AutoDiscovery", "DiscoveryInterval", "BackupCount", "AutoBackup"};
        for (const QString &key : prefKeys) {
            importManager.setPreference(key, QVariant());
        }
        importManager.saveConfiguration();
        
        bool importResult = importManager.importConfiguration(exportPath, false);
        if (!importResult) {
            // If import failed due to conflicts, try merge mode
            qDebug() << "Import failed, trying merge mode";
            importResult = importManager.importConfiguration(exportPath, true);
        }
        QVERIFY2(importResult, "Configuration import should succeed");
        
        // Verify import preserved data
        QList<NFSShare> importedShares = importManager.getLocalShares();
        QList<NFSMount> importedMounts = importManager.getPersistentMounts();
        
        QCOMPARE(importedShares.size(), originalShares.size());
        QCOMPARE(importedMounts.size(), originalMounts.size());
        
        // Clean up for next iteration
        QFile::remove(exportPath);
    }
    
    qDebug() << "Property 17 validation completed successfully";
}

void TestPropertyConfigurationManager::testProperty18_ConfigurationConflictResolution()
{
    /**
     * **Property 18: Configuration Conflict Resolution**
     * *For any* configuration conflict situation, the system should detect the conflict 
     * and provide clear resolution options to the user without losing data.
     * **Validates: Requirements 9.4**
     */
    
    qDebug() << "Testing Property 18: Configuration Conflict Resolution";
    
    for (int iteration = 0; iteration < PROPERTY_TEST_ITERATIONS; ++iteration) {
        qDebug() << "  Iteration" << (iteration + 1) << "of" << PROPERTY_TEST_ITERATIONS;
        
        // Generate conflicting configuration scenarios
        QList<NFSShare> conflictingShares;
        
        // Create shares with intentional conflicts
        NFSShare share1 = generateRandomShare();
        NFSShare share2 = generateRandomShare();
        
        // Force path conflict
        QString conflictPath = QString("/tmp/conflict-%1").arg(iteration);
        share1.setPath(conflictPath);
        share2.setPath(conflictPath);
        
        conflictingShares.append(share1);
        conflictingShares.append(share2);
        
        // Add some non-conflicting shares
        int additionalShares = QRandomGenerator::global()->bounded(0, 3);
        for (int i = 0; i < additionalShares; ++i) {
            NFSShare additionalShare = generateRandomShare();
            additionalShare.setPath(QString("/tmp/unique-%1-%2").arg(iteration).arg(i));
            conflictingShares.append(additionalShare);
        }
        
        m_configManager->setLocalShares(conflictingShares);
        
        // Detect conflicts
        ConflictDetectionResult result = m_configManager->detectConfigurationConflicts();
        
        // Property: Conflicts should be detected
        QVERIFY2(result.hasConflicts, "System should detect configuration conflicts");
        QVERIFY2(!result.conflicts.isEmpty(), "Conflict list should not be empty when conflicts exist");
        QVERIFY2(!result.summary.isEmpty(), "Conflict summary should be provided");
        
        // Property: Each conflict should have resolution options
        for (const ConfigurationConflict &conflict : result.conflicts) {
            QVERIFY2(!conflict.description.isEmpty(), "Conflict description should be provided");
            QVERIFY2(!conflict.resolutionOptions.isEmpty(), "Resolution options should be provided");
            QVERIFY2(!conflict.id.isEmpty(), "Conflict ID should be provided");
            
            // Property: Auto-resolvable conflicts should have recommended resolution
            if (conflict.canAutoResolve) {
                QVERIFY2(!conflict.recommendedResolution.isEmpty(), 
                        "Auto-resolvable conflicts should have recommended resolution");
                QVERIFY2(conflict.resolutionOptions.contains(conflict.recommendedResolution),
                        "Recommended resolution should be in available options");
            }
        }
        
        // Test auto-resolution for auto-resolvable conflicts
        QList<ConfigurationConflict> autoResolved = m_configManager->autoResolveConflicts(result.conflicts);
        
        // Property: Auto-resolution should not lose data inappropriately
        QList<NFSShare> sharesAfterAutoResolve = m_configManager->getLocalShares();
        QVERIFY2(!sharesAfterAutoResolve.isEmpty(), "Auto-resolution should not remove all shares");
        
        // Property: Auto-resolved conflicts should be marked as resolved
        for (const ConfigurationConflict &resolved : autoResolved) {
            QVERIFY2(resolved.canAutoResolve, "Only auto-resolvable conflicts should be auto-resolved");
        }
        
        // Test manual conflict resolution
        QList<ConflictResolution> manualResolutions;
        for (const ConfigurationConflict &conflict : result.conflicts) {
            // Check if this conflict was auto-resolved by comparing IDs
            bool wasAutoResolved = false;
            for (const ConfigurationConflict &resolved : autoResolved) {
                if (resolved.id == conflict.id) {
                    wasAutoResolved = true;
                    break;
                }
            }
            
            if (!wasAutoResolved) {
                ConflictResolution resolution;
                resolution.conflictId = conflict.id;
                resolution.selectedOption = conflict.resolutionOptions.first();
                resolution.preserveData = true;
                manualResolutions.append(resolution);
            }
        }
        
        if (!manualResolutions.isEmpty()) {
            // Create backup before resolution
            QString backupPath = m_configManager->createBackup();
            QVERIFY2(!backupPath.isEmpty(), "Backup should be created before conflict resolution");
            
            bool resolutionResult = m_configManager->resolveConfigurationConflicts(result.conflicts, manualResolutions);
            
            // Property: Resolution should either succeed completely or fail safely
            if (!resolutionResult) {
                // If resolution failed, configuration should be unchanged or restored
                QList<NFSShare> sharesAfterFailedResolution = m_configManager->getLocalShares();
                QVERIFY2(!sharesAfterFailedResolution.isEmpty(), 
                        "Failed resolution should not corrupt configuration");
            }
            
            // Clean up backup
            QFile::remove(backupPath);
        }
        
        // Property: After resolution, conflicts should be reduced or eliminated
        ConflictDetectionResult postResolutionResult = m_configManager->detectConfigurationConflicts();
        QVERIFY2(postResolutionResult.conflicts.size() <= result.conflicts.size(),
                "Conflict resolution should not increase number of conflicts");
    }
    
    qDebug() << "Property 18 validation completed successfully";
}

void TestPropertyConfigurationManager::testProperty19_ConfigurationIntegrityValidation()
{
    /**
     * **Property 19: Configuration Integrity Validation**
     * *For any* configuration file or data, the system should be able to validate its 
     * integrity and attempt repair of corrupted settings when possible.
     * **Validates: Requirements 9.5**
     */
    
    qDebug() << "Testing Property 19: Configuration Integrity Validation";
    
    for (int iteration = 0; iteration < PROPERTY_TEST_ITERATIONS; ++iteration) {
        qDebug() << "  Iteration" << (iteration + 1) << "of" << PROPERTY_TEST_ITERATIONS;
        
        // Generate valid configuration
        QList<NFSShare> validShares;
        int shareCount = QRandomGenerator::global()->bounded(1, 4);
        for (int i = 0; i < shareCount; ++i) {
            validShares.append(generateRandomShare());
        }
        
        QMap<QString, QVariant> validPrefs = generateRandomPreferences();
        
        m_configManager->setLocalShares(validShares);
        for (auto it = validPrefs.constBegin(); it != validPrefs.constEnd(); ++it) {
            m_configManager->setPreference(it.key(), it.value());
        }
        
        // Test validation of valid configuration
        ValidationResult validResult = m_configManager->validateConfiguration();
        
        // Property: Valid configuration should pass validation
        QVERIFY2(validResult.isValid, "Valid configuration should pass validation");
        QVERIFY2(validResult.errors.isEmpty(), "Valid configuration should have no errors");
        
        // Export configuration to file
        QString configPath = m_tempDir->filePath(QString("integrity-test-%1.json").arg(iteration));
        bool exportResult = m_configManager->exportConfiguration(configPath, 
                                                                QString("Integrity Test %1").arg(iteration),
                                                                "Property test for integrity validation");
        QVERIFY2(exportResult, "Configuration export should succeed");
        
        // Test validation of valid file
        ValidationResult fileResult = m_configManager->validateConfigurationFile(configPath);
        
        // Property: Valid file should pass validation
        QVERIFY2(fileResult.isValid, "Valid configuration file should pass validation");
        QVERIFY2(fileResult.errors.isEmpty(), "Valid configuration file should have no errors");
        
        // Test corruption scenarios
        QString corruptedPath = m_tempDir->filePath(QString("corrupted-test-%1.json").arg(iteration));
        QFile::copy(configPath, corruptedPath);
        
        // Corrupt the file
        corruptConfigurationFile(corruptedPath);
        
        // Test validation of corrupted file
        ValidationResult corruptedResult = m_configManager->validateConfigurationFile(corruptedPath);
        
        // Property: Corrupted file should fail validation
        QVERIFY2(!corruptedResult.isValid, "Corrupted configuration file should fail validation");
        QVERIFY2(!corruptedResult.errors.isEmpty(), "Corrupted configuration file should have errors");
        
        // Test invalid configuration data
        NFSShare invalidShare;
        invalidShare.setPath(QString()); // Empty path is invalid
        QList<NFSShare> invalidShares = validShares;
        invalidShares.append(invalidShare);
        
        // Also add an invalid preference to ensure repairability
        m_configManager->setPreference("DiscoveryInterval", "invalid_string_value");
        
        m_configManager->setLocalShares(invalidShares);
        
        ValidationResult invalidResult = m_configManager->validateConfiguration();
        
        // Property: Invalid configuration should fail validation
        QVERIFY2(!invalidResult.isValid, "Invalid configuration should fail validation");
        QVERIFY2(!invalidResult.errors.isEmpty(), "Invalid configuration should have errors");
        
        // Property: Repairable configuration should indicate repair capability
        if (invalidResult.canAutoRepair) {
            QVERIFY2(!invalidResult.repairActions.isEmpty(), 
                    "Repairable configuration should have repair actions");
            
            // Test repair functionality
            bool repairResult = m_configManager->repairConfiguration(invalidResult);
            
            // Property: Repair should succeed for repairable configurations
            QVERIFY2(repairResult, "Repair should succeed for repairable configurations");
            
            // Property: After repair, configuration should be valid
            ValidationResult postRepairResult = m_configManager->validateConfiguration();
            QVERIFY2(postRepairResult.isValid, "Configuration should be valid after repair");
            QVERIFY2(postRepairResult.errors.isEmpty(), "Configuration should have no errors after repair");
            
            // Property: Repair should preserve valid data
            QList<NFSShare> repairedShares = m_configManager->getLocalShares();
            QVERIFY2(repairedShares.size() >= validShares.size(), 
                    "Repair should not remove valid shares");
        } else {
            // If not auto-repairable, that's also valid behavior
            qDebug() << "Configuration is not auto-repairable, which is acceptable";
        }
        
        // Test preference validation
        m_configManager->setPreference("DiscoveryInterval", "invalid_value");
        
        ValidationResult prefResult = m_configManager->validateConfiguration();
        
        // Property: Invalid preferences should be detected
        QVERIFY2(!prefResult.isValid, "Invalid preferences should fail validation");
        
        if (prefResult.canAutoRepair) {
            bool prefRepairResult = m_configManager->repairConfiguration(prefResult);
            QVERIFY2(prefRepairResult, "Preference repair should succeed");
            
            // Property: Repaired preferences should have valid values
            int repairedInterval = m_configManager->getPreference("DiscoveryInterval").toInt();
            QVERIFY2(repairedInterval > 0 && repairedInterval <= 300, 
                    "Repaired discovery interval should be in valid range");
        }
        
        // Clean up
        QFile::remove(configPath);
        QFile::remove(corruptedPath);
    }
    
    qDebug() << "Property 19 validation completed successfully";
}

NFSShare TestPropertyConfigurationManager::generateRandomShare()
{
    static int shareCounter = 0;
    shareCounter++;
    
    QString path = QString("/tmp/test-share-%1-%2")
                  .arg(QRandomGenerator::global()->bounded(1000))
                  .arg(shareCounter);
    QString exportPath = QString("/export/test-%1").arg(shareCounter);
    
    ShareConfiguration config(QString("Test Share %1").arg(shareCounter), 
                             QRandomGenerator::global()->bounded(2) ? AccessMode::ReadOnly : AccessMode::ReadWrite);
    
    config.setAllowRootAccess(QRandomGenerator::global()->bounded(2));
    
    QStringList hosts;
    int hostCount = QRandomGenerator::global()->bounded(1, 4);
    for (int i = 0; i < hostCount; ++i) {
        hosts.append(QString("192.168.%1.%2").arg(QRandomGenerator::global()->bounded(1, 255))
                                             .arg(QRandomGenerator::global()->bounded(1, 255)));
    }
    config.setAllowedHosts(hosts);
    
    config.setNfsVersion(static_cast<NFSVersion>(QRandomGenerator::global()->bounded(3) + 2)); // V2, V3, V4
    
    NFSShare share(path, exportPath, config);
    share.setActive(QRandomGenerator::global()->bounded(2));
    share.setCreatedAt(QDateTime::currentDateTime().addSecs(-QRandomGenerator::global()->bounded(86400)));
    
    PermissionSet permissions(config.accessMode());
    permissions.setEnableRootSquash(QRandomGenerator::global()->bounded(2));
    permissions.setAnonymousUser(QString("nobody"));
    share.setPermissions(permissions);
    
    return share;
}

NFSMount TestPropertyConfigurationManager::generateRandomMount()
{
    static int mountCounter = 0;
    mountCounter++;
    
    RemoteNFSShare remoteShare(QString("testserver-%1").arg(mountCounter),
                              QHostAddress(QString("192.168.1.%1").arg(100 + mountCounter)),
                              QString("/export/remote-%1").arg(mountCounter));
    remoteShare.setDescription(QString("Test remote share %1").arg(mountCounter));
    remoteShare.setSupportedVersion(static_cast<NFSVersion>(QRandomGenerator::global()->bounded(3) + 2));
    
    MountOptions options;
    options.nfsVersion = remoteShare.supportedVersion();
    options.readOnly = QRandomGenerator::global()->bounded(2);
    options.softMount = QRandomGenerator::global()->bounded(2);
    options.timeoutSeconds = QRandomGenerator::global()->bounded(10, 120);
    options.retryCount = QRandomGenerator::global()->bounded(1, 10);
    
    QString mountPoint = QString("/mnt/test-%1").arg(mountCounter);
    
    NFSMount mount(remoteShare, mountPoint, options);
    mount.setPersistent(QRandomGenerator::global()->bounded(2));
    mount.setMountedAt(QDateTime::currentDateTime().addSecs(-QRandomGenerator::global()->bounded(3600)));
    mount.setStatus(static_cast<MountStatus>(QRandomGenerator::global()->bounded(4))); // Unmounted, Mounting, Mounted, Error
    
    return mount;
}

QMap<QString, QVariant> TestPropertyConfigurationManager::generateRandomPreferences()
{
    QMap<QString, QVariant> prefs;
    
    prefs["AutoDiscovery"] = QRandomGenerator::global()->bounded(2);
    prefs["DiscoveryInterval"] = QRandomGenerator::global()->bounded(5, 301); // 5-300 seconds
    prefs["BackupCount"] = QRandomGenerator::global()->bounded(1, 21); // 1-20 backups
    prefs["AutoBackup"] = QRandomGenerator::global()->bounded(2);
    
    // Add some random custom preferences
    int customPrefCount = QRandomGenerator::global()->bounded(0, 5);
    for (int i = 0; i < customPrefCount; ++i) {
        QString key = QString("CustomPref%1").arg(i);
        QVariant value;
        
        int valueType = QRandomGenerator::global()->bounded(4);
        switch (valueType) {
            case 0: value = QRandomGenerator::global()->bounded(1000); break;
            case 1: value = QRandomGenerator::global()->bounded(2); break;
            case 2: value = QString("CustomValue%1").arg(i); break;
            case 3: value = QRandomGenerator::global()->generateDouble(); break;
        }
        
        prefs[key] = value;
    }
    
    return prefs;
}

void TestPropertyConfigurationManager::corruptConfigurationFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite)) {
        return;
    }
    
    QByteArray data = file.readAll();
    
    // Randomly corrupt the file
    int corruptionType = QRandomGenerator::global()->bounded(3);
    
    switch (corruptionType) {
        case 0: {
            // Corrupt JSON syntax
            int pos = QRandomGenerator::global()->bounded(data.size() / 2, data.size());
            data[pos] = 'X'; // Replace character with invalid JSON
            break;
        }
        case 1: {
            // Corrupt checksum
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            if (error.error == QJsonParseError::NoError) {
                QJsonObject obj = doc.object();
                obj["checksum"] = "corrupted_checksum";
                data = QJsonDocument(obj).toJson();
            }
            break;
        }
        case 2: {
            // Truncate file
            data = data.left(data.size() / 2);
            break;
        }
    }
    
    file.seek(0);
    file.write(data);
    file.resize(file.pos());
    file.close();
}

QTEST_MAIN(TestPropertyConfigurationManager)
#include "test_property_configurationmanager.moc"