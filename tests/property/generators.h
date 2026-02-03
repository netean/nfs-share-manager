#pragma once

#include "core/types.h"
#include "core/permissionset.h"
#include "core/shareconfiguration.h"
#include "core/nfsshare.h"
#include "core/remotenfsshare.h"
#include "core/nfsmount.h"

#include <QRandomGenerator>
#include <QString>
#include <QStringList>
#include <QHostAddress>
#include <QDateTime>

namespace NFSShareManager {
namespace PropertyTesting {

/**
 * @brief Generators for property-based testing
 * 
 * This class provides methods to generate random instances of various
 * data types for use in property-based tests.
 */
class Generators
{
public:
    explicit Generators(QRandomGenerator &generator);

    // Basic type generators
    QString generateString(int minLength = 1, int maxLength = 50);
    QString generateAlphanumericString(int minLength = 1, int maxLength = 20);
    QString generatePath(bool absolute = true);
    QString generateHostname();
    QHostAddress generateIPAddress();
    QDateTime generateDateTime();
    
    // Random primitive generators (public for test use)
    int randomInt(int min, int max);
    bool randomBool();
    
    // Enum generators
    AccessMode generateAccessMode();
    NFSVersion generateNFSVersion();
    MountStatus generateMountStatus();
    
    // Complex type generators
    PermissionSet generatePermissionSet();
    ShareConfiguration generateShareConfiguration();
    NFSShare generateNFSShare();
    RemoteNFSShare generateRemoteNFSShare();
    MountOptions generateMountOptions();
    NFSMount generateNFSMount();
    
    // List generators
    QStringList generateStringList(int minSize = 0, int maxSize = 5);
    QStringList generateHostList(int minSize = 0, int maxSize = 3);
    QStringList generateUserList(int minSize = 0, int maxSize = 3);
    
    // Validation-aware generators (generate both valid and invalid data)
    QString generateValidHostname();
    QString generateInvalidHostname();
    QString generateValidPath();
    QString generateInvalidPath();
    QString generateValidUsername();
    QString generateInvalidUsername();

private:
    QRandomGenerator &m_generator;
    
    // Helper methods
    QString generateRandomString(const QString &charset, int length);
    template<typename T>
    T randomEnum(T minValue, T maxValue);
};

/**
 * @brief Template function to generate random enum values
 */
template<typename T>
T Generators::randomEnum(T minValue, T maxValue)
{
    int min = static_cast<int>(minValue);
    int max = static_cast<int>(maxValue);
    int value = randomInt(min, max);
    return static_cast<T>(value);
}

} // namespace PropertyTesting
} // namespace NFSShareManager