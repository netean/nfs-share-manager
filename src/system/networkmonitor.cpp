#include "networkmonitor.h"
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QVariant>
#include <QDebug>

namespace NFSShareManager {

NetworkMonitor::NetworkMonitor(QObject *parent)
    : QObject(parent)
    , m_networkInfo(QNetworkInformation::instance())
    , m_changeTimer(new QTimer(this))
    , m_interfaceMonitorTimer(new QTimer(this))
    , m_monitoring(false)
    , m_lastReachability(QNetworkInformation::Reachability::Unknown)
    , m_interfaceMonitorInterval(5000) // 5 second interval for interface monitoring
{
    // Configure change timer to debounce rapid network changes
    m_changeTimer->setSingleShot(true);
    m_changeTimer->setInterval(2000); // 2 second debounce
    
    connect(m_changeTimer, &QTimer::timeout, this, &NetworkMonitor::onChangeTimer);
    
    // Configure interface monitoring timer
    m_interfaceMonitorTimer->setSingleShot(false);
    m_interfaceMonitorTimer->setInterval(m_interfaceMonitorInterval);
    
    connect(m_interfaceMonitorTimer, &QTimer::timeout, this, &NetworkMonitor::onInterfaceMonitorTimer);
    
    // Load network information backend if available
    if (!m_networkInfo) {
        QNetworkInformation::loadDefaultBackend();
        m_networkInfo = QNetworkInformation::instance();
    }
    
    if (m_networkInfo) {
        // Connect to network information signals
        connect(m_networkInfo, &QNetworkInformation::reachabilityChanged,
                this, &NetworkMonitor::onReachabilityChanged);
        
        // Initialize reachability state
        m_lastReachability = m_networkInfo->reachability();
    } else {
        qWarning() << "NetworkMonitor: Failed to initialize QNetworkInformation";
    }
    
    // Initialize interface list
    updateInterfaceList();
    
    // Start monitoring by default
    startMonitoring();
}

NetworkMonitor::~NetworkMonitor()
{
    stopMonitoring();
}

bool NetworkMonitor::isMonitoring() const
{
    return m_monitoring;
}

void NetworkMonitor::startMonitoring()
{
    if (!m_monitoring) {
        m_monitoring = true;
        
        // Start interface monitoring timer
        m_interfaceMonitorTimer->start();
        
        qDebug() << "NetworkMonitor: Started network monitoring with interface monitoring every" 
                 << m_interfaceMonitorInterval << "ms";
    }
}

void NetworkMonitor::stopMonitoring()
{
    if (m_monitoring) {
        m_monitoring = false;
        m_changeTimer->stop();
        m_interfaceMonitorTimer->stop();
        qDebug() << "NetworkMonitor: Stopped network monitoring";
    }
}

bool NetworkMonitor::isNetworkAvailable() const
{
    if (!m_networkInfo) {
        return true; // Assume available if we can't check
    }
    
    QNetworkInformation::Reachability reachability = m_networkInfo->reachability();
    return reachability == QNetworkInformation::Reachability::Online ||
           reachability == QNetworkInformation::Reachability::Site;
}

QStringList NetworkMonitor::getActiveInterfaces() const
{
    QStringList activeInterfaces;
    
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        if (interface.flags() & QNetworkInterface::IsUp &&
            interface.flags() & QNetworkInterface::IsRunning &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            activeInterfaces << interface.name();
        }
    }
    
    return activeInterfaces;
}

QHash<QString, QVariant> NetworkMonitor::getInterfaceInfo(const QString &interfaceName) const
{
    QNetworkInterface interface = QNetworkInterface::interfaceFromName(interfaceName);
    
    if (!interface.isValid()) {
        return QHash<QString, QVariant>();
    }
    
    return interfaceToHash(interface);
}

bool NetworkMonitor::isInterfaceActive(const QString &interfaceName) const
{
    QNetworkInterface interface = QNetworkInterface::interfaceFromName(interfaceName);
    
    if (!interface.isValid()) {
        return false;
    }
    
    return (interface.flags() & QNetworkInterface::IsUp) &&
           (interface.flags() & QNetworkInterface::IsRunning);
}

QNetworkInformation::Reachability NetworkMonitor::currentReachability() const
{
    if (m_networkInfo) {
        return m_networkInfo->reachability();
    }
    return QNetworkInformation::Reachability::Unknown;
}

void NetworkMonitor::onReachabilityChanged()
{
    if (!m_monitoring || !m_networkInfo) {
        return;
    }
    
    QNetworkInformation::Reachability currentReachability = m_networkInfo->reachability();
    
    if (currentReachability != m_lastReachability) {
        qDebug() << "NetworkMonitor: Reachability changed from" << (int)m_lastReachability 
                 << "to" << (int)currentReachability;
        
        bool wasAvailable = (m_lastReachability == QNetworkInformation::Reachability::Online ||
                           m_lastReachability == QNetworkInformation::Reachability::Site);
        bool isAvailable = (currentReachability == QNetworkInformation::Reachability::Online ||
                          currentReachability == QNetworkInformation::Reachability::Site);
        
        m_lastReachability = currentReachability;
        
        if (isAvailable && !wasAvailable) {
            qDebug() << "NetworkMonitor: Network became available";
            emit networkAvailable();
        } else if (!isAvailable && wasAvailable) {
            qDebug() << "NetworkMonitor: Network became unavailable";
            emit networkUnavailable();
        }
        
        // Trigger general network change signal
        m_changeTimer->start();
    }
}

void NetworkMonitor::onChangeTimer()
{
    if (m_monitoring) {
        qDebug() << "NetworkMonitor: Emitting network changed signal";
        emit networkChanged();
    }
}

void NetworkMonitor::onInterfaceMonitorTimer()
{
    if (!m_monitoring) {
        return;
    }
    
    // Update interface list and check for changes
    updateInterfaceList();
}

void NetworkMonitor::updateInterfaceList()
{
    QHash<QString, QNetworkInterface> currentInterfaces;
    
    // Get current interface states
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        // Skip loopback interfaces for monitoring
        if (!(interface.flags() & QNetworkInterface::IsLoopBack)) {
            currentInterfaces[interface.name()] = interface;
        }
    }
    
    // Compare with previous state if we have one
    if (!m_lastInterfaces.isEmpty()) {
        compareInterfaceStates(m_lastInterfaces, currentInterfaces);
    }
    
    // Update stored interface state
    m_lastInterfaces = currentInterfaces;
}

void NetworkMonitor::compareInterfaceStates(const QHash<QString, QNetworkInterface> &previousInterfaces,
                                          const QHash<QString, QNetworkInterface> &currentInterfaces)
{
    bool hasTopologyChanged = false;
    
    // Check for removed interfaces
    for (auto it = previousInterfaces.begin(); it != previousInterfaces.end(); ++it) {
        const QString &interfaceName = it.key();
        if (!currentInterfaces.contains(interfaceName)) {
            qDebug() << "NetworkMonitor: Interface removed:" << interfaceName;
            emit interfaceChanged(interfaceName, InterfaceChangeType::InterfaceRemoved);
            hasTopologyChanged = true;
        }
    }
    
    // Check for new interfaces and changes to existing ones
    for (auto it = currentInterfaces.begin(); it != currentInterfaces.end(); ++it) {
        const QString &interfaceName = it.key();
        const QNetworkInterface &currentInterface = it.value();
        
        if (!previousInterfaces.contains(interfaceName)) {
            // New interface
            qDebug() << "NetworkMonitor: Interface added:" << interfaceName;
            emit interfaceChanged(interfaceName, InterfaceChangeType::InterfaceAdded);
            hasTopologyChanged = true;
        } else {
            // Check for changes to existing interface
            const QNetworkInterface &previousInterface = previousInterfaces[interfaceName];
            
            if (interfacesDiffer(previousInterface, currentInterface)) {
                // Determine the type of change
                bool wasUp = (previousInterface.flags() & QNetworkInterface::IsUp) &&
                           (previousInterface.flags() & QNetworkInterface::IsRunning);
                bool isUp = (currentInterface.flags() & QNetworkInterface::IsUp) &&
                          (currentInterface.flags() & QNetworkInterface::IsRunning);
                
                if (!wasUp && isUp) {
                    qDebug() << "NetworkMonitor: Interface came up:" << interfaceName;
                    emit interfaceChanged(interfaceName, InterfaceChangeType::InterfaceUp);
                    hasTopologyChanged = true;
                } else if (wasUp && !isUp) {
                    qDebug() << "NetworkMonitor: Interface went down:" << interfaceName;
                    emit interfaceChanged(interfaceName, InterfaceChangeType::InterfaceDown);
                    hasTopologyChanged = true;
                } else {
                    // Check for address changes
                    QStringList previousAddresses;
                    QStringList currentAddresses;
                    
                    for (const QNetworkAddressEntry &entry : previousInterface.addressEntries()) {
                        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                            previousAddresses << entry.ip().toString();
                        }
                    }
                    
                    for (const QNetworkAddressEntry &entry : currentInterface.addressEntries()) {
                        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                            currentAddresses << entry.ip().toString();
                        }
                    }
                    
                    previousAddresses.sort();
                    currentAddresses.sort();
                    
                    if (previousAddresses != currentAddresses) {
                        qDebug() << "NetworkMonitor: Interface address changed:" << interfaceName
                                 << "from" << previousAddresses << "to" << currentAddresses;
                        emit interfaceChanged(interfaceName, InterfaceChangeType::AddressChanged);
                        hasTopologyChanged = true;
                    } else {
                        qDebug() << "NetworkMonitor: Interface state changed:" << interfaceName;
                        emit interfaceChanged(interfaceName, InterfaceChangeType::StateChanged);
                    }
                }
            }
        }
    }
    
    // Emit topology changed signal if significant changes occurred
    if (hasTopologyChanged) {
        qDebug() << "NetworkMonitor: Network topology changed";
        emit topologyChanged();
        
        // Also trigger the general network change signal with debouncing
        m_changeTimer->start();
    }
}

bool NetworkMonitor::interfacesDiffer(const QNetworkInterface &interface1, const QNetworkInterface &interface2) const
{
    // Compare interface flags
    if (interface1.flags() != interface2.flags()) {
        return true;
    }
    
    // Compare hardware address
    if (interface1.hardwareAddress() != interface2.hardwareAddress()) {
        return true;
    }
    
    // Compare address entries
    QList<QNetworkAddressEntry> addresses1 = interface1.addressEntries();
    QList<QNetworkAddressEntry> addresses2 = interface2.addressEntries();
    
    if (addresses1.size() != addresses2.size()) {
        return true;
    }
    
    // Sort addresses for comparison
    std::sort(addresses1.begin(), addresses1.end(), 
              [](const QNetworkAddressEntry &a, const QNetworkAddressEntry &b) {
                  return a.ip().toString() < b.ip().toString();
              });
    std::sort(addresses2.begin(), addresses2.end(), 
              [](const QNetworkAddressEntry &a, const QNetworkAddressEntry &b) {
                  return a.ip().toString() < b.ip().toString();
              });
    
    for (int i = 0; i < addresses1.size(); ++i) {
        if (addresses1[i].ip() != addresses2[i].ip() ||
            addresses1[i].netmask() != addresses2[i].netmask()) {
            return true;
        }
    }
    
    return false;
}

QHash<QString, QVariant> NetworkMonitor::interfaceToHash(const QNetworkInterface &interface) const
{
    QHash<QString, QVariant> info;
    
    info["name"] = interface.name();
    info["humanReadableName"] = interface.humanReadableName();
    info["hardwareAddress"] = interface.hardwareAddress();
    info["isValid"] = interface.isValid();
    info["isUp"] = bool(interface.flags() & QNetworkInterface::IsUp);
    info["isRunning"] = bool(interface.flags() & QNetworkInterface::IsRunning);
    info["isLoopBack"] = bool(interface.flags() & QNetworkInterface::IsLoopBack);
    info["isPointToPoint"] = bool(interface.flags() & QNetworkInterface::IsPointToPoint);
    info["canBroadcast"] = bool(interface.flags() & QNetworkInterface::CanBroadcast);
    info["canMulticast"] = bool(interface.flags() & QNetworkInterface::CanMulticast);
    info["maximumTransmissionUnit"] = interface.maximumTransmissionUnit();
    info["type"] = static_cast<int>(interface.type());
    
    // Add address information
    QVariantList addresses;
    for (const QNetworkAddressEntry &entry : interface.addressEntries()) {
        QVariantHash addressInfo;
        addressInfo["ip"] = entry.ip().toString();
        addressInfo["netmask"] = entry.netmask().toString();
        addressInfo["broadcast"] = entry.broadcast().toString();
        addressInfo["prefixLength"] = entry.prefixLength();
        addressInfo["protocol"] = (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) ? "IPv4" : "IPv6";
        addresses.append(addressInfo);
    }
    info["addresses"] = addresses;
    
    return info;
}

} // namespace NFSShareManager