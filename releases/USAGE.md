# NFS Share Manager - Quick Usage Guide

## Getting Started

1. **Run the application**: `./nfs-share-manager`
2. **System tray**: Click the network folder icon in your system tray
3. **Main window**: Three tabs for different functions

## Basic Operations

### Creating a Local Share

1. **Local Shares** tab → **Create Share**
2. Browse and select a folder to share
3. Set export path (e.g., `/shared/documents`)
4. Choose permissions (read-only or read-write)
5. Click **OK** (requires admin password)

### Finding Remote Shares

1. **Remote Shares** tab → **Refresh**
2. Wait for network scan to complete
3. Found shares appear in the list
4. Enable **Auto Discovery** for automatic scanning

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

## Configuration

### Auto Discovery Settings

1. **Settings** → **Preferences**
2. **Network Discovery** section:
   - Enable/disable automatic discovery
   - Set discovery interval (30 seconds to 1 hour)
3. Click **OK** to save

### Notification Settings

1. **Settings** → **Preferences**
2. **Notifications** section
3. **Configure Notifications** button
4. Customize notification preferences

## Tips

- **System Tray**: Close window to minimize to tray, use **File → Quit** to exit completely
- **Persistence**: All shares and settings are automatically saved
- **Network Issues**: Try "Targeted Scan" with specific IP addresses if auto-discovery fails
- **Permissions**: Share creation requires administrator privileges via PolicyKit

## Keyboard Shortcuts

- **F5**: Refresh all data
- **Ctrl+Q**: Quit application
- **Ctrl+,**: Open preferences (on some systems)

## Troubleshooting

- **No shares found**: Check network connectivity and firewall settings
- **Mount fails**: Verify remote server is accessible and export exists
- **Permission denied**: Ensure PolicyKit is installed and user has sudo access
- **App won't start**: Check that Qt6 and KDE dependencies are installed

For detailed troubleshooting, see the main README.md file.