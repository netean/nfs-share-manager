#pragma once

#include <QObject>
#include <QNetworkInformation>
#include <QNetworkInterface>
#include <QTimer>
#include <QList>
#include <QHash>

namespace NFSShareManager {

/**
 * @brief Network monitor for detecting interface changes
 * 
 * This class monitors network interface changes and emits signals
 * when the network configuration changes, allowing other components
 * to react to network topology changes. It provides detailed
 * monitoring of interface state changes, IP address changes, and
 * network connectivity events.
 */
class NetworkMonitor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Network interface change types
     */
    enum class InterfaceChangeType {
        InterfaceAdded,     ///< New network interface added
        InterfaceRemoved,   ///< Network interface removed
        InterfaceUp,        ///< Interface became active
        InterfaceDown,      ///< Interface became inactive
        AddressChanged,     ///< IP address changed on interface
        StateChanged        ///< General interface state changed
    };

    explicit NetworkMonitor(QObject *parent = nullptr);
    ~NetworkMonitor();

    /**
     * @brief Check if network monitoring is active
     * @return True if monitoring is active
     */
    bool isMonitoring() const;

    /**
     * @brief Start network monitoring
     */
    void startMonitoring();

    /**
     * @brief Stop network monitoring
     */
    void stopMonitoring();

    /**
     * @brief Check if network is currently available
     * @return True if network is available
     */
    bool isNetworkAvailable() const;

    /**
     * @brief Get list of currently active network interfaces
     * @return List of active interface names
     */
    QStringList getActiveInterfaces() const;

    /**
     * @brief Get detailed information about a specific interface
     * @param interfaceName Name of the interface
     * @return Interface information or empty hash if not found
     */
    QHash<QString, QVariant> getInterfaceInfo(const QString &interfaceName) const;

    /**
     * @brief Check if a specific interface is currently active
     * @param interfaceName Name of the interface to check
     * @return True if the interface is up and running
     */
    bool isInterfaceActive(const QString &interfaceName) const;

    /**
     * @brief Get the current network reachability status
     * @return Current reachability status
     */
    QNetworkInformation::Reachability currentReachability() const;

signals:
    /**
     * @brief Emitted when network configuration changes
     */
    void networkChanged();

    /**
     * @brief Emitted when network becomes available
     */
    void networkAvailable();

    /**
     * @brief Emitted when network becomes unavailable
     */
    void networkUnavailable();

    /**
     * @brief Emitted when a specific interface change is detected
     * @param interfaceName Name of the interface that changed
     * @param changeType Type of change that occurred
     */
    void interfaceChanged(const QString &interfaceName, InterfaceChangeType changeType);

    /**
     * @brief Emitted when network topology changes significantly
     * This signal is emitted when multiple interfaces change or
     * when the overall network configuration changes substantially
     */
    void topologyChanged();

private slots:
    /**
     * @brief Handle reachability changes
     */
    void onReachabilityChanged();

    /**
     * @brief Handle change timer timeout
     */
    void onChangeTimer();

    /**
     * @brief Handle interface monitoring timer timeout
     */
    void onInterfaceMonitorTimer();

private:
    /**
     * @brief Update the list of network interfaces
     */
    void updateInterfaceList();

    /**
     * @brief Compare interface states and emit appropriate signals
     * @param previousInterfaces Previous interface state
     * @param currentInterfaces Current interface state
     */
    void compareInterfaceStates(const QHash<QString, QNetworkInterface> &previousInterfaces,
                               const QHash<QString, QNetworkInterface> &currentInterfaces);

    /**
     * @brief Check if two interfaces have significant differences
     * @param interface1 First interface to compare
     * @param interface2 Second interface to compare
     * @return True if interfaces have significant differences
     */
    bool interfacesDiffer(const QNetworkInterface &interface1, const QNetworkInterface &interface2) const;

    /**
     * @brief Get interface information as a hash
     * @param interface The network interface
     * @return Hash containing interface information
     */
    QHash<QString, QVariant> interfaceToHash(const QNetworkInterface &interface) const;

    QNetworkInformation *m_networkInfo;             ///< Network information instance
    QTimer *m_changeTimer;                          ///< Timer to debounce rapid changes
    QTimer *m_interfaceMonitorTimer;                ///< Timer for interface monitoring
    bool m_monitoring;                              ///< Monitoring state
    QNetworkInformation::Reachability m_lastReachability; ///< Last known reachability state
    QHash<QString, QNetworkInterface> m_lastInterfaces;   ///< Last known interface states
    int m_interfaceMonitorInterval;                 ///< Interface monitoring interval in ms
};

} // namespace NFSShareManager