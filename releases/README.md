# NFS Share Manager v1.1.0

A comprehensive NFS (Network File System) share management application for Linux with KDE Plasma integration.

## What's New in v1.1.0

### 🚀 Enhanced Network Discovery
- **Four configurable scan modes**: Quick, Full, Complete, and Targeted scanning
- **Configurable discovery timeout**: 30 seconds to 5 minutes (default 2 minutes)
- **Complete Scan mode**: Exhaustive discovery with port scanning and extended subnet ranges
- **Better progress reporting**: Accurate host counts and scan statistics

### 🔧 Improved Share Creation
- **Automatic network accessibility**: Created shares are automatically configured for local network access
- **Enhanced permissions management**: Easy network range configuration with "Add Local Networks" button
- **Better validation**: Improved error handling and user guidance

### ⚡ Performance Enhancements
- **Host limiting**: Prevents network overload by limiting hosts per scan mode
- **Optimized scanning**: Better algorithms for network discovery
- **Resource management**: Improved memory and CPU usage

## Quick Start

### 1. Installation
```bash
# Extract the package
tar -xzf nfs-share-manager-v1.1.0-linux-x86_64.tar.gz
cd nfs-share-manager-v1.1.0-linux-x86_64

# Install dependencies automatically
./install.sh

# Run the application
./nfs-share-manager
```

### 2. Create Your First Share
1. Open the application
2. Go to **Local Shares** tab
3. Click **Create Share**
4. Select a folder to share
5. In **Permissions** tab, click **Add Local Networks**
6. Click **OK** (enter admin password when prompted)

### 3. Discover Network Shares
1. Go to **Remote Shares** tab
2. Click **Discovery Mode** and select **Full Scan**
3. Click **Refresh** to start discovery
4. Found shares will appear in the list

## Features

### Core Functionality
✅ **Local NFS Share Management**: Create, edit, and remove NFS shares with network accessibility  
✅ **Advanced Network Discovery**: Four scan modes with configurable parameters  
✅ **Remote Share Mounting**: Mount and unmount remote shares with custom options  
✅ **System Tray Integration**: Background operation with system tray icon  
✅ **Configuration Persistence**: All settings and shares saved automatically  
✅ **Security Integration**: PolicyKit for secure privilege escalation  

### Network Discovery Modes
- **Quick Scan**: 50 hosts max, 3s timeout - Fast scan of common addresses
- **Full Scan**: 500 hosts max, 5s timeout - Comprehensive local network scanning
- **Complete Scan**: 1000 hosts max, 8s timeout + port scanning - Exhaustive discovery
- **Targeted Scan**: 100 hosts max, 5s timeout - Only user-specified hosts

### User Interface
✅ **Desktop Notifications**: Real-time status updates  
✅ **Progress Indication**: Visual feedback for long operations  
✅ **Configurable Discovery**: Adjustable timeouts and scan modes  
✅ **Intuitive Design**: Clean, KDE-style interface  

## System Requirements

### Minimum Requirements
- **OS**: Linux (Ubuntu 20.04+, Fedora 35+, Arch Linux, or compatible)
- **Desktop**: KDE Plasma (recommended) or any Qt6-compatible environment
- **Memory**: 50MB RAM minimum
- **Storage**: 25MB disk space
- **Network**: For NFS operations and discovery

### Dependencies
- **Qt6**: Core, Widgets, Network libraries
- **KDE Frameworks 6**: KNotifications, KConfigCore
- **NFS utilities**: `nfs-common` (Ubuntu/Debian) or `nfs-utils` (Fedora/Arch)
- **PolicyKit**: For secure privilege escalation
- **Avahi** (optional): Enhanced service discovery
- **Network tools**: For network interface detection

## Installation Options

### Option 1: Automated Installation (Recommended)
```bash
./install.sh
```
This script automatically detects your distribution and installs all required dependencies.

### Option 2: Manual Installation
Install dependencies for your distribution:

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install qt6-base-dev libkf6notifications-dev libkf6configcore-dev nfs-common polkit-kde-agent-1 avahi-utils
```

**Fedora/RHEL:**
```bash
sudo dnf install qt6-qtbase-devel kf6-knotifications-devel kf6-kconfigcore-devel nfs-utils polkit-kde avahi-tools
```

**Arch Linux:**
```bash
sudo pacman -S qt6-base kf6-knotifications kf6-kconfigcore nfs-utils polkit-kde-agent avahi
```

### Option 3: System-wide Installation
```bash
# After installing dependencies:
sudo cp nfs-share-manager /usr/local/bin/
sudo cp policy/org.kde.nfs-share-manager.policy /usr/share/polkit-1/actions/
```

## Configuration

### Network Discovery Settings
Access via **Settings** → **Preferences** → **Network Discovery**:
- **Auto Discovery**: Enable/disable automatic scanning
- **Discovery Interval**: How often to scan (30s - 1 hour)
- **Discovery Timeout**: Maximum scan time (30s - 5 minutes)
- **Default Scan Mode**: Choose default mode for auto-discovery

### Share Creation Best Practices
1. **Use "Add Local Networks"**: Automatically configures access for your network
2. **Avoid wildcard (*)**: Only use for trusted networks
3. **Test accessibility**: Use discovery to verify your shares are visible
4. **Check firewall**: Ensure NFS ports (2049, 111) are open

## Troubleshooting

### Common Issues

**Application won't start:**
- Install Qt6 and KDE Frameworks 6 dependencies
- Check that all required libraries are available

**No shares discovered:**
- Try different scan modes (Full or Complete)
- Check network connectivity and firewall settings
- Verify NFS services are running on target machines

**Share creation fails:**
- Ensure NFS server is installed and running
- Check PolicyKit is properly configured
- Verify user has sudo privileges

**Shares not accessible from network:**
- Use "Add Local Networks" in share permissions
- Check NFS server configuration
- Verify firewall allows NFS traffic

### Advanced Troubleshooting

**Enable NFS server (if creating shares):**
```bash
sudo systemctl enable nfs-server
sudo systemctl start nfs-server
sudo systemctl enable rpcbind
sudo systemctl start rpcbind
```

**Check NFS tools:**
```bash
exportfs -v          # List current exports
showmount -e localhost  # Test local NFS server
rpcinfo -p           # Check RPC services
```

**Debug network discovery:**
Run the application from terminal to see detailed discovery logs.

## Security Notes

- All privileged operations use PolicyKit for secure authentication
- NFS traffic is not encrypted by default - use on trusted networks only
- Share permissions should be configured carefully
- Application follows KDE security guidelines
- Created shares include network access controls

## Support

For issues, questions, or feature requests:
1. Check this README and USAGE.md for guidance
2. Ensure all dependencies are properly installed
3. Run from terminal to see detailed error messages
4. Check system logs for NFS-related issues

## License

This software is released under the Apache License 2.0. See the LICENSE file for details.

## Changelog

### v1.1.0 (February 4, 2026)
- Enhanced network discovery with four configurable scan modes
- Automatic network accessibility for created shares
- Configurable discovery timeout (30s - 5 minutes)
- Complete Scan mode with port scanning and extended ranges
- Better progress reporting and performance optimization
- Improved share creation with network range detection
- Enhanced UI with scan mode selection in preferences

### v1.0.0 (February 3, 2026)
- Initial release with core NFS management functionality
- Local share creation and management
- Remote share discovery and mounting
- System tray integration and configuration persistence

---

**Thank you for using NFS Share Manager v1.1.0!**

This release significantly enhances network discovery capabilities and makes share creation more user-friendly with automatic network accessibility configuration.