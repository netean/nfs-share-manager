#include "generators.h"
#include <QDir>

namespace NFSShareManager {
namespace PropertyTesting {

Generators::Generators(QRandomGenerator &generator)
    : m_generator(generator)
{
}

QString Generators::generateString(int minLength, int maxLength)
{
    int length = randomInt(minLength, maxLength);
    QString charset = QStringLiteral("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-./");
    return generateRandomString(charset, length);
}

QString Generators::generateAlphanumericString(int minLength, int maxLength)
{
    int length = randomInt(minLength, maxLength);
    QString charset = QStringLiteral("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    return generateRandomString(charset, length);
}

QString Generators::generatePath(bool absolute)
{
    QStringList components;
    
    if (absolute) {
        components << QString(); // Empty string creates leading slash
    }
    
    int numComponents = randomInt(1, 5);
    for (int i = 0; i < numComponents; ++i) {
        components << generateAlphanumericString(3, 15);
    }
    
    return components.join(QStringLiteral("/"));
}

QString Generators::generateHostname()
{
    QStringList parts;
    int numParts = randomInt(1, 3);
    
    for (int i = 0; i < numParts; ++i) {
        parts << generateAlphanumericString(3, 10).toLower();
    }
    
    return parts.join(QStringLiteral("."));
}

QHostAddress Generators::generateIPAddress()
{
    quint32 addr = m_generator.generate();
    return QHostAddress(addr);
}

QDateTime Generators::generateDateTime()
{
    // Generate a datetime within the last year
    QDateTime now = QDateTime::currentDateTime();
    qint64 secondsInYear = 365 * 24 * 60 * 60;
    qint64 randomSeconds = randomInt(0, secondsInYear);
    return now.addSecs(-randomSeconds);
}

AccessMode Generators::generateAccessMode()
{
    return randomEnum(AccessMode::NoAccess, AccessMode::ReadWrite);
}

NFSVersion Generators::generateNFSVersion()
{
    return randomEnum(NFSVersion::Version3, NFSVersion::Version4_2);
}

MountStatus Generators::generateMountStatus()
{
    return randomEnum(MountStatus::NotMounted, MountStatus::Unmounting);
}

PermissionSet Generators::generatePermissionSet()
{
    PermissionSet permissions(generateAccessMode());
    
    permissions.setEnableRootSquash(randomBool());
    permissions.setAnonymousUser(generateValidUsername());
    
    // Add some host permissions
    int numHosts = randomInt(0, 3);
    for (int i = 0; i < numHosts; ++i) {
        permissions.setHostPermission(generateValidHostname(), generateAccessMode());
    }
    
    // Add some user permissions
    int numUsers = randomInt(0, 3);
    for (int i = 0; i < numUsers; ++i) {
        permissions.setUserPermission(generateValidUsername(), generateAccessMode());
    }
    
    return permissions;
}

ShareConfiguration Generators::generateShareConfiguration()
{
    ShareConfiguration config(generateAlphanumericString(5, 20), generateAccessMode());
    
    config.setAllowRootAccess(randomBool());
    config.setNfsVersion(generateNFSVersion());
    config.setAllowedHosts(generateHostList());
    config.setAllowedUsers(generateUserList());
    
    return config;
}

NFSShare Generators::generateNFSShare()
{
    QString path = generateValidPath();
    QString exportPath = path; // Usually the same
    ShareConfiguration config = generateShareConfiguration();
    
    NFSShare share(path, exportPath, config);
    share.setPermissions(generatePermissionSet());
    share.setActive(randomBool());
    share.setCreatedAt(generateDateTime());
    
    if (randomBool()) {
        share.setErrorMessage(generateString(10, 50));
    }
    
    return share;
}

RemoteNFSShare Generators::generateRemoteNFSShare()
{
    QString hostname = generateValidHostname();
    QHostAddress address = generateIPAddress();
    QString exportPath = generatePath(true);
    
    RemoteNFSShare share(hostname, address, exportPath);
    share.setDescription(generateString(10, 100));
    share.setAvailable(randomBool());
    share.setSupportedVersion(generateNFSVersion());
    share.setDiscoveredAt(generateDateTime());
    share.setServerInfo(generateString(5, 30));
    
    if (randomBool()) {
        share.setServerPort(randomInt(1024, 65535));
    }
    
    return share;
}

MountOptions Generators::generateMountOptions()
{
    MountOptions options;
    
    options.nfsVersion = generateNFSVersion();
    options.readOnly = randomBool();
    options.timeoutSeconds = randomInt(10, 300);
    options.retryCount = randomInt(0, 10);
    options.softMount = randomBool();
    options.backgroundMount = randomBool();
    
    if (randomBool()) {
        options.rsize = randomInt(1024, 65536);
    }
    if (randomBool()) {
        options.wsize = randomInt(1024, 65536);
    }
    if (randomBool()) {
        options.securityFlavor = generateAlphanumericString(3, 10);
    }
    
    return options;
}

NFSMount Generators::generateNFSMount()
{
    RemoteNFSShare remoteShare = generateRemoteNFSShare();
    QString mountPoint = generateValidPath();
    MountOptions options = generateMountOptions();
    bool persistent = randomBool();
    
    NFSMount mount(remoteShare, mountPoint, options, persistent);
    mount.setStatus(generateMountStatus());
    mount.setMountedAt(generateDateTime());
    
    if (randomBool()) {
        mount.setErrorMessage(generateString(10, 50));
    }
    
    return mount;
}

QStringList Generators::generateStringList(int minSize, int maxSize)
{
    QStringList list;
    int size = randomInt(minSize, maxSize);
    
    for (int i = 0; i < size; ++i) {
        list << generateString();
    }
    
    return list;
}

QStringList Generators::generateHostList(int minSize, int maxSize)
{
    QStringList list;
    int size = randomInt(minSize, maxSize);
    
    for (int i = 0; i < size; ++i) {
        list << generateValidHostname();
    }
    
    return list;
}

QStringList Generators::generateUserList(int minSize, int maxSize)
{
    QStringList list;
    int size = randomInt(minSize, maxSize);
    
    for (int i = 0; i < size; ++i) {
        list << generateValidUsername();
    }
    
    return list;
}

QString Generators::generateValidHostname()
{
    // Generate a valid hostname according to RFC standards
    QStringList parts;
    int numParts = randomInt(1, 3);
    
    for (int i = 0; i < numParts; ++i) {
        QString part;
        int length = randomInt(1, 10);
        
        // First character must be alphanumeric
        part += generateRandomString(QStringLiteral("abcdefghijklmnopqrstuvwxyz0123456789"), 1);
        
        // Remaining characters can include hyphens
        if (length > 1) {
            part += generateRandomString(QStringLiteral("abcdefghijklmnopqrstuvwxyz0123456789-"), length - 1);
        }
        
        parts << part;
    }
    
    return parts.join(QStringLiteral("."));
}

QString Generators::generateInvalidHostname()
{
    // Generate various types of invalid hostnames
    int type = randomInt(0, 3);
    
    switch (type) {
    case 0:
        return QString(); // Empty hostname
    case 1:
        return QStringLiteral("-invalid"); // Starts with hyphen
    case 2:
        return QStringLiteral("invalid-"); // Ends with hyphen
    case 3:
        return generateString(1, 10) + QStringLiteral(" ") + generateString(1, 10); // Contains space
    default:
        return QStringLiteral("_invalid"); // Starts with underscore
    }
}

QString Generators::generateValidPath()
{
    // Generate a valid absolute path
    return generatePath(true);
}

QString Generators::generateInvalidPath()
{
    // Generate various types of invalid paths
    int type = randomInt(0, 2);
    
    switch (type) {
    case 0:
        return QString(); // Empty path
    case 1:
        return generateAlphanumericString(5, 20); // Relative path (no leading slash)
    default:
        return generatePath(true) + QStringLiteral("\n"); // Contains newline
    }
}

QString Generators::generateValidUsername()
{
    // Generate a valid POSIX username
    QString username;
    
    // First character must be letter or underscore
    username += generateRandomString(QStringLiteral("abcdefghijklmnopqrstuvwxyz_"), 1);
    
    // Remaining characters can be alphanumeric, underscore, or hyphen
    int length = randomInt(0, 15); // Total length 1-16
    if (length > 0) {
        username += generateRandomString(QStringLiteral("abcdefghijklmnopqrstuvwxyz0123456789_-"), length);
    }
    
    return username;
}

QString Generators::generateInvalidUsername()
{
    // Generate various types of invalid usernames
    int type = randomInt(0, 3);
    
    switch (type) {
    case 0:
        return QString(); // Empty username
    case 1:
        return QStringLiteral("123invalid"); // Starts with number
    case 2:
        return generateValidUsername() + QStringLiteral("@"); // Contains invalid character
    default:
        return generateRandomString(QStringLiteral("abcdefghijklmnopqrstuvwxyz"), 40); // Too long
    }
}

QString Generators::generateRandomString(const QString &charset, int length)
{
    QString result;
    result.reserve(length);
    
    for (int i = 0; i < length; ++i) {
        int index = randomInt(0, charset.length() - 1);
        result.append(charset.at(index));
    }
    
    return result;
}

int Generators::randomInt(int min, int max)
{
    return m_generator.bounded(min, max + 1);
}

bool Generators::randomBool()
{
    return m_generator.bounded(2) == 1;
}

} // namespace PropertyTesting
} // namespace NFSShareManager