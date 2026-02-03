# NFS Share Manager v1.0.0

A comprehensive KDE Plasma application for managing NFS shares and mounts with an intuitive graphical interface.

## Features

- **Local NFS Share Management**: Create, edit, and remove NFS shares from local directories
- **Remote Share Discovery**: Automatically discover NFS shares on your network
- **Mount Management**: Mount and unmount remote NFS shares with custom options
- **System Tray Integration**: Minimize to system tray for background operation
- **Configuration Persistence**: All shares and settings are saved and restored on restart
- **User-Configurable Discovery**: Set custom timeouts for network discovery (30s - 1 hour)
- **PolicyKit Integration**: Secure privilege escalation for system operations
- **Comprehensive Notifications**: Desktop notifications for all operations
- **Progress Indication**: Real-time progress for long-running operations

## System Requirements

- **Operating System**: Linux (tested on Ubuntu/Debian-based systems)
- **Desktop Environment**: KDE Plasma (recommended) or any Qt6-compatible environment
- **Dependencies**:
  - Qt6 (Core, Widgets, Network)
  - KDE Frameworks 6 (KNotifications, KConfigCore)
  - NFS utilities (`nfs-utils` or `nfs-common`)
  - PolicyKit (`polkit`)

## Installation

### Quick Start (Pre-compiled Binary)

1. **Install Dependencies**:
   ```bash
   # Ubuntu/Debian:
   sudo apt update
   sudo apt install qt6-base-dev libkf6notifications-dev libkf6configcore-dev nfs-common polkit-kde-agent-1
   
   # Fedora/RHEL:
   sudo dnf install qt6-qtbase-devel kf6-knotifications-devel kf6-kconfigcore-devel nfs-utils polkit-kde
   
   # Arch Linux:
   sudo pacman -S qt6-base kf6-knotifications kf6-kconfigcore nfs-utils polkit-kde-agent
   ```

2. **Make Binary Executable**:
   ```bash
   chmod +x nfs-share-manager
   ```

3. **Run the Application**:
   ```bash
   ./nfs-share-manager
   ```

### System Installation (Optional)

To install system-wide:

```bash
# Copy binary to system location
sudo cp nfs-share-manager /usr/local/bin/

# Copy PolicyKit policy
sudo cp policy/org.kde.nfs-share-manager.policy /usr/share/polkit-1/actions/

# Make executable
sudo chmod +x /usr/local/bin/nfs-share-manager

# Run from anywhere
nfs-share-manager
```

## Usage Guide

### Getting Started

1. **Launch the Application**: Run `./nfs-share-manager` or click the application icon
2. **System Tray**: The application will appear in your system tray when minimized
3. **Main Interface**: Three tabs provide access to different functionality:
   - **Local Shares**: Manage NFS shares from your local directories
   - **Remote Shares**: Discover and mount NFS shares from other servers
   - **Mounted Shares**: View and manage currently mounted NFS shares

### Creating Local NFS Shares

1. Go to the **Local Shares** tab
2. Click **Create Share**
3. Select a local directory to share
4. Configure share options:
   - **Export Path**: How the share appears to clients
   - **Permissions**: Read-only or read-write access
   - **Client Access**: Specify which clients can access the share
   - **Options**: Advanced NFS export options
5. Click **OK** to create the share

**Note**: Creating shares requires administrator privileges and will prompt for authentication.

### Discovering and Mounting Remote Shares

1. Go to the **Remote Shares** tab
2. Click **Refresh** to discover NFS shares on your network
3. **Auto Discovery**: Toggle automatic discovery to scan periodically
4. **Discovery Mode**: Choose between Quick, Full, or Targeted scanning
5. Select a discovered share and click **Mount Share**
6. Choose a local mount point and configure mount options
7. Click **OK** to mount the share

### Managing Mounted Shares

1. Go to the **Mounted Shares** tab
2. View all currently mounted NFS shares
3. Select a share and click **Unmount Share** to disconnect it
4. Mounted shares show connection status and mount details

### Configuration and Preferences

Access **Settings > Preferences** to configure:

- **Auto Discovery**: Enable/disable automatic network scanning
- **Discovery Interval**: Set how often to scan (30 seconds to 1 hour)
- **Notifications**: Configure desktop notification preferences
- **System Integration**: System tray and startup options

### Menu Options

- **File Menu**:
  - **Export Configuration**: Save current settings to a file
  - **Import Configuration**: Load settings from a file
  - **Quit**: Exit the application completely

- **View Menu**:
  - **Refresh All**: Update all share lists and status

- **Settings Menu**:
  - **Preferences**: Open configuration dialog

## Troubleshooting

### Common Issues

1. **"Permission Denied" when creating shares**:
   - Ensure PolicyKit is installed and running
   - Check that your user has sudo privileges
   - Verify the PolicyKit policy file is installed

2. **No shares discovered during network scan**:
   - Ensure NFS servers are running on target machines
   - Check firewall settings (NFS uses ports 111, 2049, and others)
   - Try "Targeted Scan" with specific IP addresses

3. **Mount operations fail**:
   - Verify the remote NFS server is accessible
   - Check that the export exists and is accessible to your IP
   - Ensure local mount point directory exists and is empty

4. **Application won't start**:
   - Check that all Qt6 and KDE dependencies are installed
   - Run from terminal to see error messages
   - Verify system has sufficient permissions

### Log Files

Application logs are written to:
- **System logs**: `/var/log/syslog` or `journalctl -u nfs-share-manager`
- **User logs**: `~/.local/share/nfs-share-manager/logs/`

### Getting Help

1. **Check System Requirements**: Ensure all dependencies are installed
2. **Review Error Messages**: Run from terminal to see detailed error output
3. **Check Network Connectivity**: Verify NFS servers are reachable
4. **Verify Permissions**: Ensure proper user privileges for NFS operations

## Security Considerations

- **PolicyKit Integration**: All privileged operations use PolicyKit for secure authentication
- **Network Security**: NFS traffic is not encrypted by default - use secure networks
- **Access Control**: Configure share permissions carefully to prevent unauthorized access
- **Firewall**: Ensure appropriate firewall rules for NFS traffic

## Technical Details

- **Built with**: Qt6, KDE Frameworks 6, CMake
- **Architecture**: Modular design with separate business logic, UI, and system layers
- **Configuration**: Uses Qt Settings for persistent configuration storage
- **Testing**: Comprehensive test suite with unit, integration, and property-based tests

## Version History

### v1.0.0 (Current)
- Initial release with full NFS share management functionality
- Local share creation, editing, and removal
- Network discovery with configurable timeouts
- Remote share mounting and unmounting
- System tray integration
- Configuration persistence
- PolicyKit security integration
- Comprehensive notification system

## License

This software is distributed under the terms of the GNU General Public License v3.0.
See the LICENSE file for full license text.

## Support

For issues, feature requests, or contributions, please refer to the project repository
or contact the development team.

---

**NFS Share Manager** - Making NFS management simple and secure.