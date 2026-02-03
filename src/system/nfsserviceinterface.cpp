#include "nfsserviceinterface.h"
#include "../core/remotenfsshare.h"
#include "../core/shareconfiguration.h"
#include <QProcess>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDebug>
#include <QHostAddress>

namespace NFSShareManager {

NFSServiceInterface::NFSServiceInterface(QObject *parent)
    : QObject(parent)
    , m_currentProcess(nullptr)
    , m_timeoutTimer(new QTimer(this))
    , m_defaultTimeout(10000)
    , m_toolsChecked(false)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &NFSServiceInterface::onCommandTimeout);
}

NFSServiceInterface::~NFSServiceInterface()
{
    cleanup();
}

bool NFSServiceInterface::isNFSToolsAvailable() const
{
    if (!m_toolsChecked) {
        // Check for required NFS tools
        const QStringList requiredTools = {
            "exportfs", "showmount", "rpcinfo", "mount", "umount"
        };
        
        for (const QString &tool : requiredTools) {
            m_toolAvailability[tool] = isCommandAvailable(tool);
        }
        m_toolsChecked = true;
    }
    
    // All tools must be available
    for (auto it = m_toolAvailability.constBegin(); it != m_toolAvailability.constEnd(); ++it) {
        if (!it.value()) {
            return false;
        }
    }
    return true;
}

QStringList NFSServiceInterface::getMissingTools() const
{
    QStringList missing;
    if (!m_toolsChecked) {
        isNFSToolsAvailable(); // This will populate the cache
    }
    
    for (auto it = m_toolAvailability.constBegin(); it != m_toolAvailability.constEnd(); ++it) {
        if (!it.value()) {
            missing << it.key();
        }
    }
    return missing;
}

NFSCommandResult NFSServiceInterface::exportDirectory(const QString &exportPath, const ShareConfiguration &config)
{
    if (!isCommandAvailable("exportfs")) {
        return NFSCommandResult(false, -1, "", "exportfs command not available", "exportfs");
    }
    
    if (exportPath.isEmpty()) {
        return NFSCommandResult(false, -1, "", "Export path cannot be empty", "exportfs");
    }
    
    // Generate the export line from configuration
    QString exportLine = config.toExportLine(exportPath);
    if (exportLine.isEmpty()) {
        return NFSCommandResult(false, -1, "", "Failed to generate export configuration", "exportfs");
    }
    
    // Parse the export line to extract options
    QStringList parts = exportLine.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        return NFSCommandResult(false, -1, "", "Invalid export configuration generated", "exportfs");
    }
    
    // Extract options from the client specification
    QString clientSpec = parts[1]; // First client specification
    int optionsStart = clientSpec.indexOf('(');
    QString options;
    
    if (optionsStart != -1) {
        int optionsEnd = clientSpec.lastIndexOf(')');
        if (optionsEnd > optionsStart) {
            options = clientSpec.mid(optionsStart + 1, optionsEnd - optionsStart - 1);
        }
    }
    
    // Use exportfs to add the export
    QStringList args;
    if (!options.isEmpty()) {
        args << "-o" << options;
    }
    args << exportPath;
    
    return executeCommand("exportfs", args);
}

NFSCommandResult NFSServiceInterface::unexportDirectory(const QString &exportPath)
{
    if (!isCommandAvailable("exportfs")) {
        return NFSCommandResult(false, -1, "", "exportfs command not available", "exportfs");
    }
    
    if (exportPath.isEmpty()) {
        return NFSCommandResult(false, -1, "", "Export path cannot be empty", "exportfs");
    }
    
    QStringList args;
    args << "-u" << exportPath;
    
    return executeCommand("exportfs", args);
}

NFSCommandResult NFSServiceInterface::getExportedDirectories()
{
    if (!isCommandAvailable("exportfs")) {
        return NFSCommandResult(false, -1, "", "exportfs command not available", "exportfs");
    }
    
    QStringList args;
    args << "-v"; // Verbose output
    
    return executeCommand("exportfs", args);
}

NFSCommandResult NFSServiceInterface::reloadExports()
{
    if (!isCommandAvailable("exportfs")) {
        return NFSCommandResult(false, -1, "", "exportfs command not available", "exportfs");
    }
    
    QStringList args;
    args << "-r"; // Re-export all directories
    
    return executeCommand("exportfs", args);
}

NFSCommandResult NFSServiceInterface::queryRemoteExports(const QHostAddress &hostAddress, int timeout)
{
    return queryRemoteExports(hostAddress.toString(), timeout);
}

NFSCommandResult NFSServiceInterface::queryRemoteExports(const QString &hostname, int timeout)
{
    if (!isCommandAvailable("showmount")) {
        return NFSCommandResult(false, -1, "", "showmount command not available", "showmount");
    }
    
    QStringList args;
    args << "-e" << hostname; // Show exports
    
    return executeCommand("showmount", args, timeout);
}

NFSCommandResult NFSServiceInterface::queryRPCServices(const QHostAddress &hostAddress, int timeout)
{
    return queryRPCServices(hostAddress.toString(), timeout);
}

NFSCommandResult NFSServiceInterface::queryRPCServices(const QString &hostname, int timeout)
{
    if (!isCommandAvailable("rpcinfo")) {
        return NFSCommandResult(false, -1, "", "rpcinfo command not available", "rpcinfo");
    }
    
    QStringList args;
    args << "-p" << hostname; // Show port mapper info
    
    return executeCommand("rpcinfo", args, timeout);
}

NFSCommandResult NFSServiceInterface::mountNFSShare(const QString &serverAddress, const QString &remotePath,
                                                    const QString &localMountPoint, const QStringList &options,
                                                    NFSVersion nfsVersion)
{
    if (!isCommandAvailable("mount")) {
        return NFSCommandResult(false, -1, "", "mount command not available", "mount");
    }
    
    // Validate mount point
    if (!validateMountPoint(localMountPoint)) {
        return NFSCommandResult(false, -1, "", "Invalid mount point: " + localMountPoint, "mount");
    }
    
    // Create mount point if it doesn't exist
    QDir dir;
    if (!dir.exists(localMountPoint)) {
        if (!dir.mkpath(localMountPoint)) {
            return NFSCommandResult(false, -1, "", "Failed to create mount point: " + localMountPoint, "mount");
        }
    }
    
    QStringList args;
    args << "-t" << "nfs";
    
    // Add mount options
    QString optionsStr = generateMountOptions(options, nfsVersion);
    if (!optionsStr.isEmpty()) {
        args << "-o" << optionsStr;
    }
    
    // Add source and destination
    args << QString("%1:%2").arg(serverAddress, remotePath) << localMountPoint;
    
    return executeCommand("mount", args);
}

NFSCommandResult NFSServiceInterface::unmountNFSShare(const QString &mountPoint, bool force)
{
    if (!isCommandAvailable("umount")) {
        return NFSCommandResult(false, -1, "", "umount command not available", "umount");
    }
    
    QStringList args;
    if (force) {
        args << "-f"; // Force unmount
    }
    args << mountPoint;
    
    return executeCommand("umount", args);
}

QList<MountInfo> NFSServiceInterface::getMountedNFSShares()
{
    QList<MountInfo> mounts;
    
    if (!isCommandAvailable("mount")) {
        return mounts;
    }
    
    NFSCommandResult result = executeCommand("mount", QStringList());
    if (result.success) {
        mounts = parseMountOutput(result.output);
    }
    
    return mounts;
}

bool NFSServiceInterface::isNFSMountPoint(const QString &path)
{
    QList<MountInfo> mounts = getMountedNFSShares();
    for (const MountInfo &mount : mounts) {
        if (mount.mountPoint == path && mount.isNFS) {
            return true;
        }
    }
    return false;
}

QStringList NFSServiceInterface::parseExportfsOutput(const QString &output)
{
    QStringList exports;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith('#')) {
            exports << trimmed;
        }
    }
    
    return exports;
}

QList<RemoteNFSShare> NFSServiceInterface::parseShowmountOutput(const QString &output, const QString &serverAddress)
{
    QList<RemoteNFSShare> shares;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    // Skip the header line if present
    bool skipFirst = false;
    if (!lines.isEmpty() && lines.first().contains("Export list")) {
        skipFirst = true;
    }
    
    for (int i = skipFirst ? 1 : 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) continue;
        
        // Parse export line: "/path/to/export client1,client2,..."
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (!parts.isEmpty()) {
            QString exportPath = parts.first();
            
            RemoteNFSShare share;
            share.setHostName(serverAddress);
            
            // Try to parse as IP address
            QHostAddress addr(serverAddress);
            if (!addr.isNull()) {
                share.setHostAddress(addr);
            }
            
            share.setExportPath(exportPath);
            share.setDiscoveredAt(QDateTime::currentDateTime());
            share.setAvailable(true);
            share.setSupportedVersion(NFSVersion::Version4); // Default assumption
            
            shares << share;
        }
    }
    
    return shares;
}

bool NFSServiceInterface::parseRPCInfoOutput(const QString &output)
{
    // Look for NFS-related services in rpcinfo output
    // We need to be more specific - portmapper/rpcbind alone doesn't mean NFS is available
    QStringList nfsServices = {"nfs", "mountd"};
    
    for (const QString &service : nfsServices) {
        if (output.contains(service, Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}

QList<MountInfo> NFSServiceInterface::parseMountOutput(const QString &output)
{
    QList<MountInfo> mounts;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        
        // Parse mount line: "device on mountpoint type filesystem (options)"
        QRegularExpression re(R"(^(.+?)\s+on\s+(.+?)\s+type\s+(\S+)\s*(?:\((.+?)\))?$)");
        QRegularExpressionMatch match = re.match(trimmed);
        
        if (match.hasMatch()) {
            MountInfo info;
            info.device = match.captured(1);
            info.mountPoint = match.captured(2);
            info.fileSystem = match.captured(3);
            info.options = match.captured(4);
            info.isNFS = (info.fileSystem.startsWith("nfs", Qt::CaseInsensitive) ||
                         info.device.contains(':'));
            
            mounts << info;
        }
    }
    
    return mounts;
}

void NFSServiceInterface::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    
    if (m_currentProcess) {
        m_timeoutTimer->stop();
        
        QString output = QString::fromUtf8(m_currentProcess->readAllStandardOutput());
        QString error = QString::fromUtf8(m_currentProcess->readAllStandardError());
        
        NFSCommandResult result(exitCode == 0, exitCode, output, error, m_currentCommand);
        emit commandFinished(result);
        
        m_currentProcess->deleteLater();
        m_currentProcess = nullptr;
        m_currentCommand.clear();
    }
}

void NFSServiceInterface::onProcessError(QProcess::ProcessError error)
{
    if (m_currentProcess) {
        m_timeoutTimer->stop();
        
        QString errorString;
        switch (error) {
        case QProcess::FailedToStart:
            errorString = "Failed to start command";
            break;
        case QProcess::Crashed:
            errorString = "Command crashed";
            break;
        case QProcess::Timedout:
            errorString = "Command timed out";
            break;
        case QProcess::ReadError:
            errorString = "Read error";
            break;
        case QProcess::WriteError:
            errorString = "Write error";
            break;
        default:
            errorString = "Unknown error";
            break;
        }
        
        NFSCommandResult result(false, -1, "", errorString, m_currentCommand);
        emit commandFinished(result);
        
        m_currentProcess->deleteLater();
        m_currentProcess = nullptr;
        m_currentCommand.clear();
    }
}

void NFSServiceInterface::onCommandTimeout()
{
    if (m_currentProcess) {
        m_currentProcess->kill();
        emit commandTimeout(m_currentCommand);
        
        NFSCommandResult result(false, -1, "", "Command timed out", m_currentCommand);
        emit commandFinished(result);
        
        m_currentProcess->deleteLater();
        m_currentProcess = nullptr;
        m_currentCommand.clear();
    }
}

NFSCommandResult NFSServiceInterface::executeCommand(const QString &program, const QStringList &arguments, int timeout)
{
    // Check if the program exists first
    if (!isCommandAvailable(program)) {
        return NFSCommandResult(false, -1, "", QString("Command not found: %1").arg(program), program + " " + arguments.join(" "));
    }
    
    QString command = program + " " + arguments.join(" ");
    
    // Create a temporary process for this command
    QProcess process;
    process.start(program, arguments);
    
    emit commandStarted(command);
    
    // Wait for the process to finish with timeout
    if (!process.waitForFinished(timeout)) {
        process.kill();
        process.waitForFinished(1000); // Give it a second to die
        
        NFSCommandResult result(false, -1, "", "Command execution timed out", command);
        emit commandFinished(result);
        return result;
    }
    
    // Get the results
    int exitCode = process.exitCode();
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QString error = QString::fromUtf8(process.readAllStandardError());
    
    NFSCommandResult result(exitCode == 0, exitCode, output, error, command);
    emit commandFinished(result);
    
    return result;
}

bool NFSServiceInterface::isCommandAvailable(const QString &command) const
{
    // Check if command exists in PATH
    QString path = QStandardPaths::findExecutable(command);
    return !path.isEmpty();
}

QString NFSServiceInterface::generateMountOptions(const QStringList &options, NFSVersion nfsVersion) const
{
    QStringList allOptions = options;
    
    // Add NFS version option
    switch (nfsVersion) {
    case NFSVersion::Version3:
        allOptions << "nfsvers=3";
        break;
    case NFSVersion::Version4:
        allOptions << "nfsvers=4";
        break;
    case NFSVersion::Version4_1:
        allOptions << "nfsvers=4.1";
        break;
    case NFSVersion::Version4_2:
        allOptions << "nfsvers=4.2";
        break;
    }
    
    // Add some sensible defaults if not specified
    if (!allOptions.contains(QRegularExpression("^rsize="))) {
        allOptions << "rsize=8192";
    }
    if (!allOptions.contains(QRegularExpression("^wsize="))) {
        allOptions << "wsize=8192";
    }
    if (!allOptions.contains(QRegularExpression("^timeo="))) {
        allOptions << "timeo=14";
    }
    if (!allOptions.contains(QRegularExpression("^retrans="))) {
        allOptions << "retrans=3";
    }
    
    return allOptions.join(",");
}

bool NFSServiceInterface::validateMountPoint(const QString &path) const
{
    if (path.isEmpty()) {
        return false;
    }
    
    // Check if path is absolute
    if (!QDir::isAbsolutePath(path)) {
        return false;
    }
    
    // Check if parent directory exists
    QFileInfo info(path);
    QDir parentDir = info.dir();
    if (!parentDir.exists()) {
        return false;
    }
    
    return true;
}

void NFSServiceInterface::cleanup()
{
    if (m_currentProcess) {
        m_timeoutTimer->stop();
        m_currentProcess->kill();
        m_currentProcess->waitForFinished(1000);
        m_currentProcess->deleteLater();
        m_currentProcess = nullptr;
        m_currentCommand.clear();
    }
}

} // namespace NFSShareManager