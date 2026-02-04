# NFS Share Manager v1.1.0 - Quick Usage Guide

## Getting Started

1. **Run the application**: `./nfs-share-manager`
2. **System tray**: Click the network folder icon in your system tray
3. **Main window**: Three tabs for different functions

## Basic Operations

### Creating a Local Share (Enhanced in v1.1.0)

1. **Local Shares** tab → **Create Share**
2. Browse and select a folder to share
3. Set share name and access mode (read-only or read-write)
4. **Permissions tab**: Configure network access
   - **Add Local Networks**: Automatically adds your local network ranges
   - **Allow All (*)**: Allow access from any host (less secure)
   - **Manual entry**: Add specific IP addresses or network ranges
5. Click **OK** (requires admin password)

**New**: Shares are now automatically configured for network accessibility!

### Network Discovery (Major Enhancement in v1.1.0)

#### Automatic Discovery
1. **Remote Shares** tab → **Auto Discovery** toggle
2. Configure discovery settings in **Preferences**
3. Shares appear automatically as they're found

#### Manual Discovery with Scan Modes
1. **Remote Shares** tab → **Discovery Mode** button
2. Choose scan mode:
   - **Quick Scan**: Fast scan of common network addresses (50 hosts, 3s timeout)
   - **Full Scan**: Comprehensive local network scanning (500 hosts, 5s timeout)
   - **Complete Scan**: Exhaustive discovery with port scanning (1000 hosts, 8s timeout)
   - **Targeted Scan**: Only scan specific hosts you specify
3. Click **Refresh** to start discovery

### Mounting a Remote Share

1. Select a share from **Remote Shares** list
2. Click **Mount Share**
3. Choose local mount point (e.g., `/mnt/remote-share`)
4. Configure mount options if needed
5. Click **OK**

### Unmounting Shares

1. **Mounted Shares** tab
2. Select mounted share
3. Click **Unmount Share**

## Configuration (Enhanced in v1.1.0)

### Discovery Settings

1. **Settings** → **Preferences**
2. **Network Discovery** section:
   - **Enable automatic discovery**: Toggle auto-discovery on/off
   - **Discovery interval**: Set scan frequency (30 seconds to 1 hour)
   - **Discovery timeout**: Maximum time to wait for scans (30s - 5 minutes)
   - **Default scan mode**: Choose default mode for automatic discovery
3. Click **OK** to save

### Advanced Scan Mode Configuration

Each scan mode has different characteristics:

- **Quick Scan**: Best for daily use, scans common network addresses quickly
- **Full Scan**: Comprehensive scanning of your local networks
- **Complete Scan**: Most thorough, includes port scanning and extended ranges
- **Targeted Scan**: Only scans hosts you specifically add

### Share Creation with Network Access

When creating shares, use the **Permissions** tab to:
- **Add Local Networks**: Automatically detects and adds your network ranges
- **Manual Configuration**: Add specific IP addresses (e.g., `192.168.1.100`)
- **Network Ranges**: Add subnet ranges (e.g., `192.168.1.0/24`)
- **Wildcard Access**: Use `*` for unrestricted access (not recommended)

### Notification Settings

1. **Settings** → **Preferences**
2. **Notifications** section
3. **Configure Notifications** button
4. Customize notification preferences

## Tips

- **System Tray**: Close window to minimize to tray, use **File → Quit** to exit completely
- **Persistence**: All shares and settings are automatically saved
- **Network Discovery**: Start with Quick Scan, use Complete Scan for thorough discovery
- **Share Access**: Created shares are now automatically accessible from your local network
- **Performance**: Complete Scan may take longer but finds more shares
- **Targeted Scanning**: Use when you know specific server addresses

## Keyboard Shortcuts

- **F5**: Refresh all data
- **Ctrl+Q**: Quit application
- **Ctrl+,**: Open preferences (on some systems)

## New Features in v1.1.0

### Enhanced Network Discovery
- **Four scan modes** with different thoroughness levels
- **Configurable timeouts** for discovery operations
- **Better progress reporting** with accurate host counts
- **Automatic network range detection**

### Improved Share Creation
- **Automatic local network configuration** for new shares
- **Network accessibility by default** for created shares
- **Enhanced permission management** with network range support
- **Better validation and error handling**

### Performance Improvements
- **Host limiting** to prevent network overload
- **Optimized scanning algorithms**
- **Better resource management**
- **Improved error handling**

## Troubleshooting

### Discovery Issues
- **No shares found**: Try different scan modes (Full or Complete)
- **Slow discovery**: Reduce discovery timeout or use Quick scan mode
- **Network timeouts**: Check firewall settings and network connectivity

### Share Creation Issues
- **Shares not accessible**: Verify network ranges in Permissions tab
- **Permission denied**: Ensure PolicyKit is installed and user has sudo access
- **NFS server not running**: Start NFS server service (`sudo systemctl start nfs-server`)

### General Issues
- **App won't start**: Check that Qt6 and KDE dependencies are installed
- **Mount fails**: Verify remote server is accessible and export exists
- **Performance issues**: Use Quick scan mode for better performance

For detailed troubleshooting, see the main README.md file.