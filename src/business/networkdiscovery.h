#pragma once

#include <QObject>
#include <QTimer>
#include <QList>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QStringList>
#include <QHash>
#include <QDateTime>
#include "../core/remotenfsshare.h"
#include "../system/nfsserviceinterface.h"

namespace NFSShareManager {

class NetworkMonitor;

/**
 * @brief Network discovery class for automatic NFS share detection
 * 
 * This class implements network scanning using showmount and rpcinfo to
 * automatically discover available NFS shares on the local network.
 * It supports configurable scan intervals and integrates with Avahi/Zeroconf
 * service discovery when available.
 */
class NetworkDiscovery : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Discovery scan modes
     */
    enum class ScanMode {
        Quick,      ///< Quick scan of common hosts and subnets
        Full,       ///< Full scan of all network interfaces
        Complete,   ///< Complete scan including port scanning and extended discovery
        Targeted    ///< Scan specific hosts/ranges only
    };

    /**
     * @brief Discovery status enumeration
     */
    enum class DiscoveryStatus {
        Idle,       ///< Not currently scanning
        Scanning,   ///< Active scan in progress
        Completed,  ///< Last scan completed successfully
        Error       ///< Last scan encountered errors
    };

    explicit NetworkDiscovery(QObject *parent = nullptr);
    ~NetworkDiscovery();

    /**
     * @brief Start automatic network discovery
     * @param scanInterval Interval between scans in milliseconds (default: 30000)
     */
    void startDiscovery(int scanInterval = 30000);

    /**
     * @brief Stop automatic network discovery
     */
    void stopDiscovery();

    /**
     * @brief Check if discovery is currently active
     * @return True if discovery is running
     */
    bool isDiscoveryActive() const;

    /**
     * @brief Get current discovery status
     * @return Current status of the discovery process
     */
    DiscoveryStatus discoveryStatus() const;

    /**
     * @brief Get list of discovered NFS shares
     * @return List of all discovered shares
     */
    QList<RemoteNFSShare> getDiscoveredShares() const;

    /**
     * @brief Get list of recently discovered shares
     * @param maxAge Maximum age in seconds (default: 300)
     * @return List of shares discovered within the specified time
     */
    QList<RemoteNFSShare> getRecentShares(int maxAge = 300) const;

    /**
     * @brief Manually refresh discovery (perform immediate scan)
     * @param mode Scan mode to use for this refresh
     */
    void refreshDiscovery(ScanMode mode = ScanMode::Quick);

    /**
     * @brief Set the scan interval for automatic discovery
     * @param intervalMs Interval in milliseconds
     */
    void setScanInterval(int intervalMs);

    /**
     * @brief Get the current scan interval
     * @return Scan interval in milliseconds
     */
    int scanInterval() const;

    /**
     * @brief Set the scan mode for automatic discovery
     * @param mode The scan mode to use
     */
    void setScanMode(ScanMode mode);

    /**
     * @brief Get the current scan mode
     * @return Current scan mode
     */
    ScanMode scanMode() const;

    /**
     * @brief Add a specific host to scan
     * @param hostAddress Host IP address or hostname
     */
    void addTargetHost(const QString &hostAddress);

    /**
     * @brief Remove a host from the scan list
     * @param hostAddress Host IP address or hostname
     */
    void removeTargetHost(const QString &hostAddress);

    /**
     * @brief Get list of target hosts for scanning
     * @return List of target host addresses
     */
    QStringList getTargetHosts() const;

    /**
     * @brief Clear all target hosts
     */
    void clearTargetHosts();

    /**
     * @brief Enable or disable Avahi/Zeroconf integration
     * @param enabled True to enable Avahi integration
     */
    void setAvahiEnabled(bool enabled);

    /**
     * @brief Check if Avahi/Zeroconf integration is enabled
     * @return True if Avahi is enabled
     */
    bool isAvahiEnabled() const;

    /**
     * @brief Check if Avahi/Zeroconf is available on the system
     * @return True if Avahi tools are available
     */
    bool isAvahiAvailable() const;

    /**
     * @brief Get the last scan completion time
     * @return Timestamp of the last completed scan
     */
    QDateTime lastScanTime() const;

    /**
     * @brief Get the number of hosts scanned in the last scan
     * @return Number of hosts that were scanned
     */
    int lastScanHostCount() const;

    /**
     * @brief Get scan statistics
     * @return Map of scan statistics (hosts_scanned, shares_found, etc.)
     */
    QHash<QString, QVariant> getScanStatistics() const;

    /**
     * @brief Configure scan mode parameters
     * @param mode The scan mode to configure
     * @param maxHosts Maximum hosts to scan for this mode
     * @param timeout Timeout per host in milliseconds
     * @param enablePortScan Enable port scanning for this mode
     */
    void configureScanMode(ScanMode mode, int maxHosts, int timeout, bool enablePortScan = false);

    /**
     * @brief Get scan mode configuration
     * @param mode The scan mode to query
     * @return Configuration parameters for the scan mode
     */
    QHash<QString, QVariant> getScanModeConfig(ScanMode mode) const;

signals:
    /**
     * @brief Emitted when a new NFS share is discovered
     * @param share The newly discovered share
     */
    void shareDiscovered(const RemoteNFSShare &share);

    /**
     * @brief Emitted when a previously discovered share becomes unavailable
     * @param hostAddress The host address that became unavailable
     * @param exportPath The export path that became unavailable
     */
    void shareUnavailable(const QString &hostAddress, const QString &exportPath);

    /**
     * @brief Emitted when a discovery scan completes
     * @param sharesFound Number of shares found in this scan
     * @param hostsScanned Number of hosts scanned
     */
    void discoveryCompleted(int sharesFound, int hostsScanned);

    /**
     * @brief Emitted when discovery scan starts
     * @param mode The scan mode being used
     */
    void discoveryStarted(ScanMode mode);

    /**
     * @brief Emitted when discovery status changes
     * @param status The new discovery status
     */
    void discoveryStatusChanged(DiscoveryStatus status);

    /**
     * @brief Emitted when a discovery error occurs
     * @param error Error message describing the issue
     */
    void discoveryError(const QString &error);

    /**
     * @brief Emitted when scan progress updates
     * @param current Current host being scanned
     * @param total Total hosts to scan
     * @param hostAddress Current host address
     */
    void scanProgress(int current, int total, const QString &hostAddress);

private slots:
    /**
     * @brief Handle automatic discovery timer timeout
     */
    void onDiscoveryTimer();

    /**
     * @brief Handle network interface changes
     */
    void onNetworkChanged();

    /**
     * @brief Handle showmount command completion
     * @param hostAddress The host that was queried
     * @param result The command result
     */
    void onShowmountCompleted(const QString &hostAddress, const NFSCommandResult &result);

    /**
     * @brief Handle rpcinfo command completion
     * @param hostAddress The host that was queried
     * @param result The command result
     */
    void onRPCInfoCompleted(const QString &hostAddress, const NFSCommandResult &result);

    /**
     * @brief Handle Avahi service discovery results
     * @param services List of discovered services
     */
    void onAvahiServicesDiscovered(const QStringList &services);

private:
    /**
     * @brief Perform network scan based on current mode
     */
    void performNetworkScan();

    /**
     * @brief Scan specific host for NFS shares
     * @param hostAddress Host to scan
     */
    void scanHost(const QString &hostAddress);

    /**
     * @brief Get list of hosts to scan based on current mode
     * @return List of host addresses to scan
     */
    QStringList getHostsToScan() const;

    /**
     * @brief Get network addresses from all interfaces
     * @return List of network addresses to scan
     */
    QStringList getNetworkAddresses() const;

    /**
     * @brief Generate IP addresses for a subnet
     * @param networkAddress Network address (e.g., "192.168.1.0/24")
     * @return List of IP addresses in the subnet
     */
    QStringList generateSubnetAddresses(const QString &networkAddress) const;

    /**
     * @brief Update share availability status
     * @param share The share to update
     * @param available New availability status
     */
    void updateShareAvailability(RemoteNFSShare &share, bool available);

    /**
     * @brief Remove stale shares that haven't been seen recently
     * @param maxAge Maximum age in seconds
     */
    void removeStaleShares(int maxAge = 600);

    /**
     * @brief Check if a share already exists in the discovered list
     * @param share The share to check
     * @return True if the share already exists
     */
    bool shareExists(const RemoteNFSShare &share) const;

    /**
     * @brief Find existing share by host and path
     * @param hostAddress Host address
     * @param exportPath Export path
     * @return Pointer to existing share or nullptr if not found
     */
    RemoteNFSShare *findExistingShare(const QString &hostAddress, const QString &exportPath);

    /**
     * @brief Find existing share by host and path (const version)
     * @param hostAddress Host address
     * @param exportPath Export path
     * @return Pointer to existing share or nullptr if not found
     */
    const RemoteNFSShare *findExistingShare(const QString &hostAddress, const QString &exportPath) const;

    /**
     * @brief Initialize Avahi/Zeroconf integration
     */
    void initializeAvahi();

    /**
     * @brief Cleanup Avahi resources
     */
    void cleanupAvahi();

    /**
     * @brief Start Avahi service discovery
     */
    void startAvahiDiscovery();

    /**
     * @brief Stop Avahi service discovery
     */
    void stopAvahiDiscovery();

    /**
     * @brief Set discovery status and emit signal if changed
     * @param status New discovery status
     */
    void setDiscoveryStatus(DiscoveryStatus status);

    /**
     * @brief Update scan statistics
     * @param key Statistics key
     * @param value Statistics value
     */
    void updateScanStatistics(const QString &key, const QVariant &value);

    /**
     * @brief Initialize default scan mode configurations
     */
    void initializeDefaultScanModeConfigs();

    // Core components
    NFSServiceInterface *m_nfsService;     ///< NFS service interface
    NetworkMonitor *m_networkMonitor;      ///< Network change monitor
    QTimer *m_discoveryTimer;              ///< Automatic discovery timer

    // Discovery configuration
    ScanMode m_scanMode;                   ///< Current scan mode
    int m_scanInterval;                    ///< Scan interval in milliseconds
    QStringList m_targetHosts;             ///< Specific hosts to scan
    bool m_avahiEnabled;                   ///< Avahi integration enabled
    bool m_avahiAvailable;                 ///< Avahi tools available

    // Discovery state
    DiscoveryStatus m_discoveryStatus;     ///< Current discovery status
    QList<RemoteNFSShare> m_discoveredShares; ///< List of discovered shares
    QDateTime m_lastScanTime;              ///< Last scan completion time
    int m_lastScanHostCount;               ///< Hosts scanned in last scan
    QHash<QString, QVariant> m_scanStats;  ///< Scan statistics

    // Active scan state
    QStringList m_currentScanHosts;        ///< Hosts being scanned
    int m_currentScanIndex;                ///< Current scan progress
    QHash<QString, bool> m_hostScanResults; ///< Results of host scans

    // Avahi integration (if available)
    QProcess *m_avahiProcess;              ///< Avahi discovery process
    bool m_avahiRunning;                   ///< Avahi discovery active

    // Constants
    static const int DEFAULT_SCAN_INTERVAL = 30000;  ///< Default scan interval (30s)
    static const int QUICK_SCAN_TIMEOUT = 3000;      ///< Quick scan timeout (3s)
    static const int FULL_SCAN_TIMEOUT = 5000;       ///< Full scan timeout (5s)
    static const int COMPLETE_SCAN_TIMEOUT = 8000;   ///< Complete scan timeout (8s)
    static const int MAX_CONCURRENT_SCANS = 10;      ///< Max concurrent host scans
    
    // Scan mode configuration storage
    QHash<ScanMode, QHash<QString, QVariant>> m_scanModeConfigs;
};

} // namespace NFSShareManager