# NFS Share Manager Release Notes

## Version 1.1.0 - February 4, 2026

### 🚀 Major Features
- **Enhanced Network Discovery**: Complete overhaul of network scanning with configurable scan modes
- **Automatic Network Access**: Created shares are now automatically configured for network accessibility
- **Advanced Scan Modes**: Four distinct scan modes with customizable parameters

### 🔧 New Features
- **Complete Scan Mode**: Exhaustive network discovery including port scanning and extended subnet ranges
- **Configurable Scan Parameters**: Each scan mode now has configurable timeout, max hosts, and port scanning options
- **User-Configurable Discovery Timeout**: Discovery timeout now configurable from 30 seconds to 5 minutes (default 2 minutes)
- **Enhanced Share Creation**: Shares automatically include local network ranges for better discoverability
- **Improved Network Detection**: Better subnet scanning and automatic local network configuration

### 📊 Scan Mode Details
- **Quick Scan**: 50 hosts max, 3s timeout - Fast scan of common network addresses
- **Full Scan**: 500 hosts max, 5s timeout - Comprehensive local network scanning  
- **Complete Scan**: 1000 hosts max, 8s timeout + port scanning - Exhaustive discovery
- **Targeted Scan**: 100 hosts max, 5s timeout - Only user-specified hosts

### 🛠️ Improvements
- **Better Progress Reporting**: More accurate host count and scan progress information
- **Enhanced UI**: Added scan mode selection to preferences dialog
- **Persistent Configuration**: Scan mode preferences are saved and restored
- **Network Range Detection**: Automatic detection and inclusion of local network ranges
- **Performance Optimization**: Host limiting based on scan mode to prevent overwhelming networks

### 🐛 Bug Fixes
- Fixed hardcoded 40-second discovery timeout
- Fixed network discovery not reading user preferences
- Improved share persistence and loading
- Better error handling for network operations
- Fixed scan mode not being applied correctly

### 🔒 Security & Compatibility
- Maintained PolicyKit integration for secure operations
- Compatible with existing configuration files
- Backward compatible with v1.0.0 configurations
- Enhanced network security with proper host filtering

---

## Version 1.0.0 - February 3, 2026

## Release Information

- **Version**: 1.0.0
- **Release Date**: February 3, 2026
- **Platform**: Linux x86_64
- **Build**: Production release

## What's Included

### Binary Distribution (`nfs-share-manager-v1.0.0-linux-x86_64.tar.gz`)

- **nfs-share-manager**: Main application binary (18MB)
- **README.md**: Comprehensive documentation and installation guide
- **USAGE.md**: Quick start guide and basic operations
- **install.sh**: Automated dependency installation script
- **policy/**: PolicyKit policy files for secure privilege escalation

### Individual Files

- **nfs-share-manager**: Standalone executable binary
- **VERSION**: Version and build information
- **RELEASE_NOTES.md**: This file

## Installation Options

### Option 1: Quick Start (Recommended)
```bash
# Download and extract
tar -xzf nfs-share-manager-v1.0.0-linux-x86_64.tar.gz
cd nfs-share-manager-v1.0.0-linux-x86_64

# Install dependencies automatically
./install.sh

# Run the application
./nfs-share-manager
```

### Option 2: Manual Installation
```bash
# Extract files
tar -xzf nfs-share-manager-v1.0.0-linux-x86_64.tar.gz

# Install dependencies manually (see README.md)
# Make executable and run
chmod +x nfs-share-manager
./nfs-share-manager
```

### Option 3: System-wide Installation
```bash
# After extracting and installing dependencies:
sudo cp nfs-share-manager /usr/local/bin/
sudo cp policy/org.kde.nfs-share-manager.policy /usr/share/polkit-1/actions/
nfs-share-manager
```

## Key Features

✅ **Local NFS Share Management**: Create, edit, and remove NFS shares
✅ **Network Discovery**: Automatic discovery of remote NFS shares
✅ **Mount Management**: Mount and unmount remote shares with custom options
✅ **System Tray Integration**: Background operation with system tray icon
✅ **Configuration Persistence**: All settings and shares saved automatically
✅ **User-Configurable Discovery**: Adjustable scan intervals (30s - 1 hour)
✅ **Security Integration**: PolicyKit for secure privilege escalation
✅ **Desktop Notifications**: Real-time status updates
✅ **Progress Indication**: Visual feedback for long operations

## System Requirements

- **OS**: Linux (Ubuntu 20.04+, Fedora 35+, Arch Linux, or compatible)
- **Desktop**: KDE Plasma (recommended) or any Qt6-compatible environment
- **Memory**: 50MB RAM minimum
- **Storage**: 20MB disk space
- **Network**: For NFS operations and discovery

## Dependencies

- Qt6 (Core, Widgets, Network)
- KDE Frameworks 6 (KNotifications, KConfigCore)
- NFS utilities (`nfs-common` or `nfs-utils`)
- PolicyKit (`polkit`)

## Known Issues

- Test suite has some compilation issues (does not affect main application)
- Some advanced NFS options may require manual configuration
- Network discovery may be limited by firewall configurations

## Compatibility

- **Tested on**: Ubuntu 22.04, Fedora 39, Arch Linux
- **Qt Version**: 6.2+
- **KDE Frameworks**: 6.0+
- **Architecture**: x86_64 (64-bit Intel/AMD)

## Security Notes

- All privileged operations use PolicyKit for secure authentication
- NFS traffic is not encrypted by default - use on trusted networks
- Share permissions should be configured carefully
- Application follows KDE security guidelines

## Support

For issues, questions, or feature requests:
1. Check the README.md for troubleshooting
2. Review USAGE.md for operation guidance
3. Ensure all dependencies are properly installed
4. Run from terminal to see detailed error messages

## Future Releases

Planned features for future versions:
- Additional mount options and protocols
- Enhanced network discovery options
- Configuration import/export improvements
- Performance optimizations
- Additional desktop environment support

---

**Thank you for using NFS Share Manager!**

This release represents a fully functional NFS management solution with 
comprehensive features for both local share management and remote share 
discovery and mounting.