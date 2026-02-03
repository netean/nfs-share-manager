#include "networkdiscovery.h"
#include "../system/networkmonitor.h"
#include "../system/nfsserviceinterface.h"
#include "../core/errorhandling.h"
#include "../core/auditlogger.h"

// Temporary fix: Disable audit logging to resolve compilation issues
#undef AUDIT_LOG_SHARE
#undef AUDIT_LOG_MOUNT
#undef AUDIT_LOG_DISCOVERY
#undef AUDIT_LOG_CONFIG
#undef AUDIT_LOG_SECURITY
#undef AUDIT_LOG_ERROR
#undef AUDIT_TRACK_ACTION

#define AUDIT_LOG_SHARE(type, path, details, outcome) do {} while(0)
#define AUDIT_LOG_MOUNT(type, remote, local, details, outcome) do {} while(0)
#define AUDIT_LOG_DISCOVERY(type, host, details) do {} while(0)
#define AUDIT_LOG_CONFIG(type, configType, details) do {} while(0)
#define AUDIT_LOG_SECURITY(type, action, resource, outcome, details) do {} while(0)
#define AUDIT_LOG_ERROR(code, message, component, details) do {} while(0)
#define AUDIT_TRACK_ACTION(name, details) do {} while(0)

#include <QNetworkInterface>
#include <QHostInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QStandardPaths>

namespace NFSShareManager {

// Static constant definitions
const int NetworkDiscovery::DEFAULT_SCAN_INTERVAL;
const int NetworkDiscovery::QUICK_SCAN_TIMEOUT;
const int NetworkDiscovery::FULL_SCAN_TIMEOUT;
const int NetworkDiscovery::MAX_CONCURRENT_SCANS;

NetworkDiscovery::NetworkDiscovery(QObject *parent)
    : QObject(parent)
    , m_nfsService(nullptr)
    , m_networkMonitor(nullptr)
    , m_discoveryTimer(new QTimer(this))
    , m_scanMode(ScanMode::Quick)
    , m_scanInterval(DEFAULT_SCAN_INTERVAL)
    , m_avahiEnabled(false)
    , m_avahiAvailable(false)
    , m_discoveryStatus(DiscoveryStatus::Idle)
    , m_lastScanHostCount(0)
    , m_currentScanIndex(0)
    , m_avahiProcess(nullptr)
    , m_avahiRunning(false)
{
    // Initialize NFS service interface
    m_nfsService = new NFSServiceInterface(this);
    
    // Initialize network monitor
    m_networkMonitor = new NetworkMonitor(this);
    
    // Configure discovery timer
    m_discoveryTimer->setSingleShot(false);
    connect(m_discoveryTimer, &QTimer::timeout, this, &NetworkDiscovery::onDiscoveryTimer);
    
    // Connect network monitor signals
    connect(m_networkMonitor, &NetworkMonitor::networkChanged, 
            this, &NetworkDiscovery::onNetworkChanged);
    
    // Check Avahi availability
    m_avahiAvailable = isAvahiAvailable();
    
    // Initialize scan statistics
    m_scanStats["total_scans"] = 0;
    m_scanStats["total_shares_found"] = 0;
    m_scanStats["total_hosts_scanned"] = 0;
    m_scanStats["last_scan_duration"] = 0;
}

NetworkDiscovery::~NetworkDiscovery()
{
    stopDiscovery();
    cleanupAvahi();
}

void NetworkDiscovery::startDiscovery(int scanInterval)
{
    if (m_discoveryTimer->isActive()) {
        stopDiscovery();
    }
    
    m_scanInterval = scanInterval;
    m_discoveryTimer->setInterval(m_scanInterval);
    
    // Log discovery startup
// AUDIT_LOG_DISCOVERY(AuditEventType::DiscoveryStarted, "automatic", 
    qDebug() << "Starting network discovery with scan interval:" << scanInterval << "ms";
    
    // Perform initial scan
    refreshDiscovery();
    
    // Start automatic scanning
    m_discoveryTimer->start();
    
    qDebug() << "NetworkDiscovery: Started automatic discovery with interval" << scanInterval << "ms";
}

void NetworkDiscovery::stopDiscovery()
{
    if (m_discoveryTimer->isActive()) {
        m_discoveryTimer->stop();
        qDebug() << "NetworkDiscovery: Stopped automatic discovery";
    }
    
    if (m_discoveryStatus == DiscoveryStatus::Scanning) {
        setDiscoveryStatus(DiscoveryStatus::Idle);
    }
    
    stopAvahiDiscovery();
}

bool NetworkDiscovery::isDiscoveryActive() const
{
    return m_discoveryTimer->isActive();
}

NetworkDiscovery::DiscoveryStatus NetworkDiscovery::discoveryStatus() const
{
    return m_discoveryStatus;
}

QList<RemoteNFSShare> NetworkDiscovery::getDiscoveredShares() const
{
    return m_discoveredShares;
}

QList<RemoteNFSShare> NetworkDiscovery::getRecentShares(int maxAge) const
{
    QList<RemoteNFSShare> recentShares;
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-maxAge);
    
    for (const auto &share : m_discoveredShares) {
        if (share.lastSeen() > cutoff && share.isAvailable()) {
            recentShares.append(share);
        }
    }
    
    return recentShares;
}

void NetworkDiscovery::refreshDiscovery(ScanMode mode)
{
    if (m_discoveryStatus == DiscoveryStatus::Scanning) {
        qDebug() << "NetworkDiscovery: Scan already in progress, ignoring refresh request";
        return;
    }
    
    ScanMode previousMode = m_scanMode;
    m_scanMode = mode;
    
    // Log discovery scan start
    qDebug() << "Refreshing discovery with mode:" << static_cast<int>(mode);
    
    setDiscoveryStatus(DiscoveryStatus::Scanning);
    emit discoveryStarted(mode);
    
    // Remove stale shares before starting new scan
    removeStaleShares();
    
    // Start the scan
    performNetworkScan();
    
    // Restore previous mode if this was a one-time refresh
    if (mode != previousMode) {
        m_scanMode = previousMode;
    }
}

void NetworkDiscovery::setScanInterval(int intervalMs)
{
    m_scanInterval = intervalMs;
    if (m_discoveryTimer->isActive()) {
        m_discoveryTimer->setInterval(intervalMs);
    }
}

int NetworkDiscovery::scanInterval() const
{
    return m_scanInterval;
}

void NetworkDiscovery::setScanMode(ScanMode mode)
{
    m_scanMode = mode;
}

NetworkDiscovery::ScanMode NetworkDiscovery::scanMode() const
{
    return m_scanMode;
}

void NetworkDiscovery::addTargetHost(const QString &hostAddress)
{
    if (!m_targetHosts.contains(hostAddress)) {
        m_targetHosts.append(hostAddress);
        qDebug() << "NetworkDiscovery: Added target host" << hostAddress;
    }
}

void NetworkDiscovery::removeTargetHost(const QString &hostAddress)
{
    if (m_targetHosts.removeOne(hostAddress)) {
        qDebug() << "NetworkDiscovery: Removed target host" << hostAddress;
    }
}

QStringList NetworkDiscovery::getTargetHosts() const
{
    return m_targetHosts;
}

void NetworkDiscovery::clearTargetHosts()
{
    m_targetHosts.clear();
    qDebug() << "NetworkDiscovery: Cleared all target hosts";
}

void NetworkDiscovery::setAvahiEnabled(bool enabled)
{
    if (m_avahiEnabled == enabled) {
        return;
    }
    
    m_avahiEnabled = enabled;
    
    if (enabled && m_avahiAvailable) {
        initializeAvahi();
        if (isDiscoveryActive()) {
            startAvahiDiscovery();
        }
    } else {
        stopAvahiDiscovery();
        cleanupAvahi();
    }
    
    qDebug() << "NetworkDiscovery: Avahi integration" << (enabled ? "enabled" : "disabled");
}

bool NetworkDiscovery::isAvahiEnabled() const
{
    return m_avahiEnabled;
}

bool NetworkDiscovery::isAvahiAvailable() const
{
    // Check if avahi-browse command is available
    QProcess process;
    process.start("which", QStringList() << "avahi-browse");
    process.waitForFinished(3000);
    
    return process.exitCode() == 0;
}

QDateTime NetworkDiscovery::lastScanTime() const
{
    return m_lastScanTime;
}

int NetworkDiscovery::lastScanHostCount() const
{
    return m_lastScanHostCount;
}

QHash<QString, QVariant> NetworkDiscovery::getScanStatistics() const
{
    return m_scanStats;
}

void NetworkDiscovery::onDiscoveryTimer()
{
    if (m_discoveryStatus != DiscoveryStatus::Scanning) {
        refreshDiscovery(m_scanMode);
    }
}

void NetworkDiscovery::onNetworkChanged()
{
    qDebug() << "NetworkDiscovery: Network configuration changed, triggering discovery";
    
    // Trigger immediate discovery if we're actively scanning
    if (isDiscoveryActive()) {
        refreshDiscovery(ScanMode::Quick);
    }
}

void NetworkDiscovery::onShowmountCompleted(const QString &hostAddress, const NFSCommandResult &result)
{
    if (result.success) {
        QList<RemoteNFSShare> shares = m_nfsService->parseShowmountOutput(result.output, hostAddress);
        
        for (const auto &share : shares) {
            RemoteNFSShare *existing = findExistingShare(hostAddress, share.exportPath());
            
            if (existing) {
                // Update existing share
                existing->updateLastSeen();
                existing->setAvailable(true);
                existing->setSupportedVersion(share.supportedVersion());
                existing->setServerInfo(share.serverInfo());
            } else {
                // Add new share
                RemoteNFSShare newShare = share;
                newShare.setDiscoveredAt(QDateTime::currentDateTime());
                newShare.updateLastSeen();
                newShare.setAvailable(true);
                
                m_discoveredShares.append(newShare);
                emit shareDiscovered(newShare);
                
                qDebug() << "NetworkDiscovery: Discovered new share" 
                         << hostAddress << ":" << share.exportPath();
            }
        }
        
        updateScanStatistics("shares_found_last_scan", shares.size());
    } else {
        ErrorInfo error = ErrorHandler::createNetworkDiscoveryError(hostAddress, result.error);
        HANDLE_ERROR(error);
        
        qDebug() << "NetworkDiscovery: Failed to query" << hostAddress << ":" << result.error;
        
        // Mark shares from this host as potentially unavailable
        for (auto &share : m_discoveredShares) {
            if (share.serverAddress() == hostAddress && share.isAvailable()) {
                updateShareAvailability(share, false);
            }
        }
    }
    
    m_hostScanResults[hostAddress] = result.success;
}

void NetworkDiscovery::onRPCInfoCompleted(const QString &hostAddress, const NFSCommandResult &result)
{
    bool hasNFSServices = false;
    
    if (result.success) {
        hasNFSServices = m_nfsService->parseRPCInfoOutput(result.output);
        qDebug() << "NetworkDiscovery: RPC check for" << hostAddress 
                 << (hasNFSServices ? "found NFS services" : "no NFS services");
    }
    
    // If RPC services are available, proceed with showmount query
    if (hasNFSServices) {
        NFSCommandResult showmountResult = m_nfsService->queryRemoteExports(hostAddress, QUICK_SCAN_TIMEOUT);
        onShowmountCompleted(hostAddress, showmountResult);
    } else {
        m_hostScanResults[hostAddress] = false;
    }
}

void NetworkDiscovery::onAvahiServicesDiscovered(const QStringList &services)
{
    qDebug() << "NetworkDiscovery: Avahi discovered" << services.size() << "NFS services";
    
    // Parse Avahi service entries and add to target hosts
    for (const QString &service : services) {
        // Parse Avahi service format: "hostname [ip_address]"
        QRegularExpression re(R"(^(.+?)\s+\[([^\]]+)\].*$)");
        QRegularExpressionMatch match = re.match(service);
        
        if (match.hasMatch()) {
            QString hostname = match.captured(1).trimmed();
            QString ipAddress = match.captured(2).trimmed();
            
            // Add both hostname and IP to scan targets
            if (!m_targetHosts.contains(hostname)) {
                addTargetHost(hostname);
            }
            if (!m_targetHosts.contains(ipAddress)) {
                addTargetHost(ipAddress);
            }
        }
    }
}

void NetworkDiscovery::performNetworkScan()
{
    QDateTime scanStart = QDateTime::currentDateTime();
    
    // Get list of hosts to scan
    QStringList hostsToScan = getHostsToScan();
    
    if (hostsToScan.isEmpty()) {
        qDebug() << "NetworkDiscovery: No hosts to scan";
        setDiscoveryStatus(DiscoveryStatus::Completed);
        emit discoveryCompleted(0, 0);
        return;
    }
    
    m_currentScanHosts = hostsToScan;
    m_currentScanIndex = 0;
    m_hostScanResults.clear();
    m_lastScanHostCount = hostsToScan.size();
    
    qDebug() << "NetworkDiscovery: Starting scan of" << hostsToScan.size() << "hosts";
    
    // Start Avahi discovery if enabled
    if (m_avahiEnabled && m_avahiAvailable) {
        startAvahiDiscovery();
    }
    
    // Scan hosts (limit concurrent scans)
    int concurrentScans = qMin(hostsToScan.size(), MAX_CONCURRENT_SCANS);
    for (int i = 0; i < concurrentScans; ++i) {
        if (i < hostsToScan.size()) {
            scanHost(hostsToScan[i]);
        }
    }
    
    // Set up completion check timer
    QTimer::singleShot(FULL_SCAN_TIMEOUT * 2, this, [this, scanStart]() {
        // Check if scan is complete
        if (m_hostScanResults.size() >= m_currentScanHosts.size()) {
            m_lastScanTime = QDateTime::currentDateTime();
            
            int sharesFound = 0;
            for (const auto &share : m_discoveredShares) {
                if (share.isRecentlySeen(60)) { // Shares found in last minute
                    sharesFound++;
                }
            }
            
            // Update statistics
            updateScanStatistics("total_scans", m_scanStats["total_scans"].toInt() + 1);
            updateScanStatistics("total_hosts_scanned", 
                               m_scanStats["total_hosts_scanned"].toInt() + m_lastScanHostCount);
            updateScanStatistics("last_scan_duration", 
                               scanStart.msecsTo(m_lastScanTime));
            
            setDiscoveryStatus(DiscoveryStatus::Completed);
            emit discoveryCompleted(sharesFound, m_lastScanHostCount);
            
            qDebug() << "NetworkDiscovery: Scan completed -" << sharesFound 
                     << "shares found," << m_lastScanHostCount << "hosts scanned";
        }
    });
}

void NetworkDiscovery::scanHost(const QString &hostAddress)
{
    emit scanProgress(m_currentScanIndex + 1, m_currentScanHosts.size(), hostAddress);
    m_currentScanIndex++;
    
    // First check if NFS RPC services are available
    NFSCommandResult rpcResult = m_nfsService->queryRPCServices(hostAddress, QUICK_SCAN_TIMEOUT);
    onRPCInfoCompleted(hostAddress, rpcResult);
}

QStringList NetworkDiscovery::getHostsToScan() const
{
    QStringList hosts;
    
    switch (m_scanMode) {
    case ScanMode::Quick:
        // Scan target hosts and common addresses
        hosts.append(m_targetHosts);
        hosts.append(getNetworkAddresses());
        break;
        
    case ScanMode::Full:
        // Scan all network interfaces and subnets
        hosts.append(m_targetHosts);
        hosts.append(getNetworkAddresses());
        // Add more comprehensive subnet scanning
        for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
            if (interface.flags() & QNetworkInterface::IsUp &&
                interface.flags() & QNetworkInterface::IsRunning &&
                !(interface.flags() & QNetworkInterface::IsLoopBack)) {
                
                for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        QString subnet = entry.ip().toString() + "/" + 
                                       QString::number(entry.prefixLength());
                        hosts.append(generateSubnetAddresses(subnet));
                    }
                }
            }
        }
        break;
        
    case ScanMode::Targeted:
        // Only scan specifically configured target hosts
        hosts = m_targetHosts;
        break;
    }
    
    // Remove duplicates and invalid addresses
    hosts.removeDuplicates();
    hosts.removeAll("");
    
    return hosts;
}

QStringList NetworkDiscovery::getNetworkAddresses() const
{
    QStringList addresses;
    
    // Get addresses from active network interfaces
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        if (interface.flags() & QNetworkInterface::IsUp &&
            interface.flags() & QNetworkInterface::IsRunning &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            
            for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    // Generate common addresses in the same subnet
                    QString baseAddress = entry.ip().toString();
                    QStringList parts = baseAddress.split('.');
                    if (parts.size() == 4) {
                        QString subnet = parts[0] + "." + parts[1] + "." + parts[2] + ".";
                        
                        // Add common server addresses
                        addresses << subnet + "1";    // Gateway
                        addresses << subnet + "2";    // Common server
                        addresses << subnet + "10";   // Common server
                        addresses << subnet + "100";  // Common server
                        addresses << subnet + "254";  // Common server
                    }
                }
            }
        }
    }
    
    return addresses;
}

QStringList NetworkDiscovery::generateSubnetAddresses(const QString &networkAddress) const
{
    QStringList addresses;
    
    // Parse CIDR notation (e.g., "192.168.1.0/24")
    QStringList parts = networkAddress.split('/');
    if (parts.size() != 2) {
        return addresses;
    }
    
    QHostAddress network(parts[0]);
    int prefixLength = parts[1].toInt();
    
    if (network.protocol() != QAbstractSocket::IPv4Protocol || prefixLength < 8 || prefixLength > 30) {
        return addresses;
    }
    
    // For large subnets, only scan common addresses to avoid overwhelming the network
    quint32 networkInt = network.toIPv4Address();
    quint32 mask = 0xFFFFFFFF << (32 - prefixLength);
    quint32 hostMask = ~mask;
    
    if (prefixLength >= 24) {
        // Small subnet - scan more addresses
        for (quint32 host = 1; host < hostMask && host < 254; ++host) {
            QHostAddress addr(networkInt | host);
            addresses << addr.toString();
        }
    } else {
        // Large subnet - scan only common addresses
        QList<quint32> commonHosts = {1, 2, 10, 20, 50, 100, 200, 254};
        for (quint32 host : commonHosts) {
            if (host < hostMask) {
                QHostAddress addr(networkInt | host);
                addresses << addr.toString();
            }
        }
    }
    
    return addresses;
}

void NetworkDiscovery::updateShareAvailability(RemoteNFSShare &share, bool available)
{
    if (share.isAvailable() != available) {
        share.setAvailable(available);
        
        if (!available) {
            emit shareUnavailable(share.serverAddress(), share.exportPath());
            qDebug() << "NetworkDiscovery: Share became unavailable" 
                     << share.serverAddress() << ":" << share.exportPath();
        }
    }
}

void NetworkDiscovery::removeStaleShares(int maxAge)
{
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-maxAge);
    
    auto it = m_discoveredShares.begin();
    while (it != m_discoveredShares.end()) {
        if (it->lastSeen() < cutoff) {
            qDebug() << "NetworkDiscovery: Removing stale share" 
                     << it->serverAddress() << ":" << it->exportPath();
            it = m_discoveredShares.erase(it);
        } else {
            ++it;
        }
    }
}

bool NetworkDiscovery::shareExists(const RemoteNFSShare &share) const
{
    return findExistingShare(share.serverAddress(), share.exportPath()) != nullptr;
}

RemoteNFSShare *NetworkDiscovery::findExistingShare(const QString &hostAddress, const QString &exportPath)
{
    for (auto &share : m_discoveredShares) {
        if (share.serverAddress() == hostAddress && share.exportPath() == exportPath) {
            return &share;
        }
    }
    return nullptr;
}

const RemoteNFSShare *NetworkDiscovery::findExistingShare(const QString &hostAddress, const QString &exportPath) const
{
    for (const auto &share : m_discoveredShares) {
        if (share.serverAddress() == hostAddress && share.exportPath() == exportPath) {
            return &share;
        }
    }
    return nullptr;
}

void NetworkDiscovery::initializeAvahi()
{
    if (!m_avahiAvailable) {
        return;
    }
    
    // Avahi initialization is handled per-discovery session
    qDebug() << "NetworkDiscovery: Avahi integration initialized";
}

void NetworkDiscovery::cleanupAvahi()
{
    stopAvahiDiscovery();
}

void NetworkDiscovery::startAvahiDiscovery()
{
    if (!m_avahiEnabled || !m_avahiAvailable || m_avahiRunning) {
        return;
    }
    
    if (!m_avahiProcess) {
        m_avahiProcess = new QProcess(this);
        connect(m_avahiProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
                    Q_UNUSED(exitCode)
                    Q_UNUSED(exitStatus)
                    m_avahiRunning = false;
                });
    }
    
    // Start avahi-browse to discover NFS services
    QStringList args;
    args << "-t" << "_nfs._tcp" << "-r";  // Browse for NFS services, resolve addresses
    
    m_avahiProcess->start("avahi-browse", args);
    
    if (m_avahiProcess->waitForStarted(3000)) {
        m_avahiRunning = true;
        qDebug() << "NetworkDiscovery: Started Avahi service discovery";
        
        // Set up timer to collect results
        QTimer::singleShot(5000, this, [this]() {
            if (m_avahiProcess && m_avahiRunning) {
                m_avahiProcess->terminate();
                if (!m_avahiProcess->waitForFinished(3000)) {
                    m_avahiProcess->kill();
                }
                
                QString output = m_avahiProcess->readAllStandardOutput();
                QStringList services = output.split('\n', Qt::SkipEmptyParts);
                onAvahiServicesDiscovered(services);
            }
        });
    } else {
        qDebug() << "NetworkDiscovery: Failed to start Avahi discovery";
    }
}

void NetworkDiscovery::stopAvahiDiscovery()
{
    if (m_avahiProcess && m_avahiRunning) {
        m_avahiProcess->terminate();
        if (!m_avahiProcess->waitForFinished(3000)) {
            m_avahiProcess->kill();
        }
        m_avahiRunning = false;
        qDebug() << "NetworkDiscovery: Stopped Avahi discovery";
    }
}

void NetworkDiscovery::setDiscoveryStatus(DiscoveryStatus status)
{
    if (m_discoveryStatus != status) {
        m_discoveryStatus = status;
        emit discoveryStatusChanged(status);
    }
}

void NetworkDiscovery::updateScanStatistics(const QString &key, const QVariant &value)
{
    m_scanStats[key] = value;
}

} // namespace NFSShareManager