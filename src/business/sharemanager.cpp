#include "sharemanager.h"
#include "../core/nfsshare.h"
#include "../core/shareconfiguration.h"
#include "../core/permissionset.h"
#include <QDebug>

namespace NFSShareManager {

ShareManager::ShareManager(QObject *parent)
    : QObject(parent)
    , m_policyKitHelper(nullptr)
    , m_nfsService(nullptr)
    , m_fileWatcher(nullptr)
    , m_refreshTimer(nullptr)
    , m_initialized(false)
{
    qDebug() << "ShareManager initialized";
    // Initialize the shares list
    m_activeShares.clear();
}

ShareManager::~ShareManager()
{
}

bool ShareManager::createShare(const QString &path, const ShareConfiguration &config)
{
    qDebug() << "ShareManager::createShare - creating share for path:" << path;
    
    // Check if path is already shared
    if (isShared(path)) {
        qDebug() << "Path is already shared:" << path;
        return false;
    }
    
    // Validate the path
    if (!validateSharePath(path)) {
        qDebug() << "Invalid share path:" << path;
        return false;
    }
    
    // Create the NFSShare object
    NFSShare newShare(path, path, config);
    newShare.setCreatedAt(QDateTime::currentDateTime());
    newShare.setActive(true);
    
    // Add to our active shares list
    m_activeShares.append(newShare);
    
    qDebug() << "Share created successfully:" << path << "Total shares:" << m_activeShares.size();
    
    // Emit signal that share was created
    emit shareCreated(newShare);
    
    // Save to configuration for persistence
    emit sharesPersistenceRequested();
    
    return true;
}

bool ShareManager::removeShare(const QString &path)
{
    qDebug() << "ShareManager::removeShare - removing share for path:" << path;
    
    // Find and remove the share
    for (int i = 0; i < m_activeShares.size(); ++i) {
        if (m_activeShares[i].path() == path) {
            NFSShare removedShare = m_activeShares[i];
            m_activeShares.removeAt(i);
            
            qDebug() << "Share removed successfully:" << path << "Remaining shares:" << m_activeShares.size();
            
            // Emit signal that share was removed
            emit shareRemoved(path);
            
            // Save to configuration for persistence
            emit sharesPersistenceRequested();
            
            return true;
        }
    }
    
    qDebug() << "Share not found for removal:" << path;
    return false;
}

QList<NFSShare> ShareManager::getActiveShares() const
{
    qDebug() << "ShareManager::getActiveShares - returning" << m_activeShares.size() << "shares";
    return m_activeShares;
}

bool ShareManager::addExistingShare(const NFSShare &share)
{
    qDebug() << "ShareManager::addExistingShare - adding existing share:" << share.path();
    
    // Check if share already exists
    if (isShared(share.path())) {
        qDebug() << "Share already exists:" << share.path();
        return false;
    }
    
    // Add the share to our list
    m_activeShares.append(share);
    
    qDebug() << "Existing share added successfully:" << share.path() << "Total shares:" << m_activeShares.size();
    
    return true;
}

bool ShareManager::updateSharePermissions(const QString &path, const PermissionSet &permissions)
{
    qDebug() << "ShareManager::updateSharePermissions - updating permissions for path:" << path;
    
    // Find and update the share permissions
    for (int i = 0; i < m_activeShares.size(); ++i) {
        if (m_activeShares[i].path() == path) {
            m_activeShares[i].setPermissions(permissions);
            
            qDebug() << "Share permissions updated successfully:" << path;
            
            // Emit signal that share was updated
            emit shareUpdated(m_activeShares[i]);
            
            return true;
        }
    }
    
    qDebug() << "Share not found for permission update:" << path;
    return false;
}

bool ShareManager::updateShareConfiguration(const QString &path, const ShareConfiguration &config)
{
    qDebug() << "ShareManager::updateShareConfiguration - updating share for path:" << path;
    
    // Find and update the share
    for (int i = 0; i < m_activeShares.size(); ++i) {
        if (m_activeShares[i].path() == path) {
            m_activeShares[i].setConfig(config);
            
            qDebug() << "Share configuration updated successfully:" << path;
            
            // Emit signal that share was updated
            emit shareUpdated(m_activeShares[i]);
            
            return true;
        }
    }
    
    qDebug() << "Share not found for update:" << path;
    return false;
}

NFSShare ShareManager::getShare(const QString &path) const
{
    qDebug() << "ShareManager::getShare - looking for share at path:" << path;
    
    // Find the share by path
    for (const NFSShare &share : m_activeShares) {
        if (share.path() == path) {
            qDebug() << "Share found:" << path;
            return share;
        }
    }
    
    qDebug() << "Share not found:" << path;
    return NFSShare(); // Return empty share if not found
}

bool ShareManager::isShared(const QString &path) const
{
    // Check if the path is already in our active shares
    for (const NFSShare &share : m_activeShares) {
        if (share.path() == path) {
            return true;
        }
    }
    return false;
}

void ShareManager::refreshShares()
{
    qDebug() << "ShareManager::refreshShares - refreshing share list";
    
    // In a real implementation, this would reload shares from the system
    // For now, just emit the signal to update the UI
    emit sharesRefreshed();
}

bool ShareManager::validateSharePath(const QString &path) const
{
    Q_UNUSED(path)
    return true;
}

QStringList ShareManager::getPathValidationErrors(const QString &path) const
{
    Q_UNUSED(path)
    return QStringList();
}

QString ShareManager::generateExportsFileContent() const
{
    return QString();
}

bool ShareManager::isNFSServerRunning() const
{
    return true;
}

bool ShareManager::startNFSServer()
{
    qDebug() << "ShareManager::startNFSServer - stub implementation";
    return true;
}

bool ShareManager::restartNFSServer()
{
    qDebug() << "ShareManager::restartNFSServer - stub implementation";
    return true;
}

void ShareManager::onPolicyKitActionCompleted(PolicyKitHelper::Action action, bool success, const QString &errorMessage)
{
    Q_UNUSED(action)
    Q_UNUSED(success)
    Q_UNUSED(errorMessage)
}

void ShareManager::onNFSCommandFinished(const NFSCommandResult &result)
{
    Q_UNUSED(result)
}

void ShareManager::onFileSystemChanged(const QString &path)
{
    Q_UNUSED(path)
}

void ShareManager::onRefreshTimer()
{
}

void ShareManager::initialize()
{
    m_initialized = true;
}

void ShareManager::loadExistingShares()
{
}

bool ShareManager::validateShareConfiguration(const ShareConfiguration &config) const
{
    Q_UNUSED(config)
    return true;
}

bool ShareManager::updateExportsFile()
{
    return true;
}

bool ShareManager::backupExportsFile()
{
    return true;
}

QList<NFSShare>::iterator ShareManager::findShare(const QString &path)
{
    Q_UNUSED(path)
    return m_activeShares.end();
}

QList<NFSShare>::const_iterator ShareManager::findShare(const QString &path) const
{
    Q_UNUSED(path)
    return m_activeShares.end();
}

QString ShareManager::normalizePath(const QString &path) const
{
    return path;
}

bool ShareManager::checkDirectoryPermissions(const QString &path) const
{
    Q_UNUSED(path)
    return true;
}

QString ShareManager::generateExportPath(const QString &basePath) const
{
    return basePath;
}

} // namespace NFSShareManager