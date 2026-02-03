# PolicyKit Helper Documentation

## Overview

The `PolicyKitHelper` class provides secure privilege escalation for NFS Share Manager operations that require administrative privileges. It integrates with the system's PolicyKit daemon to authenticate users before performing privileged operations.

## Features

- **Secure Authentication**: Uses PolicyKit for proper privilege escalation
- **Comprehensive Actions**: Supports all NFS-related administrative operations
- **Error Handling**: Provides detailed error messages and user feedback
- **Backup Safety**: Automatically creates backups before modifying system files
- **Audit Logging**: Logs all operations for security auditing

## Supported Actions

The helper supports the following privileged actions:

### Share Management
- `CreateShare`: Create new NFS shares by modifying `/etc/exports`
- `ModifyShare`: Modify existing NFS share configurations
- `RemoveShare`: Remove NFS shares from system configuration

### Mount Operations
- `MountRemoteShare`: Mount remote NFS shares to local directories
- `UnmountShare`: Unmount NFS shares from the local filesystem
- `ModifyFstab`: Add persistent mount entries to `/etc/fstab`

### System Operations
- `RestartNFSService`: Restart the NFS server service
- `ModifySystemFiles`: General system file modifications with backup

## Usage Example

```cpp
#include "system/policykithelper.h"

// Create helper instance
PolicyKitHelper *helper = new PolicyKitHelper(this);

// Check if user is authorized for an action
PolicyKitHelper::AuthResult result = helper->checkAuthorization(
    PolicyKitHelper::Action::CreateShare);

if (result == PolicyKitHelper::AuthResult::Authorized) {
    // User is authorized, proceed with action
    QVariantMap parameters;
    parameters["shareEntry"] = "/home/shared *(rw,sync,no_subtree_check)";
    
    bool success = helper->executePrivilegedAction(
        PolicyKitHelper::Action::CreateShare, parameters);
    
    if (success) {
        qDebug() << "Share created successfully";
    }
}

// Connect to signals for async feedback
connect(helper, &PolicyKitHelper::actionCompleted,
        this, [](PolicyKitHelper::Action action, bool success, const QString &error) {
    if (success) {
        qDebug() << "Action completed successfully";
    } else {
        qWarning() << "Action failed:" << error;
    }
});
```

## Policy Configuration

The helper uses PolicyKit policy files located in `/usr/share/polkit-1/actions/`. The policy file `org.kde.nfs-share-manager.policy` defines the following actions:

- `org.kde.nfs-share-manager.create-share`
- `org.kde.nfs-share-manager.remove-share`
- `org.kde.nfs-share-manager.modify-share`
- `org.kde.nfs-share-manager.mount-share`
- `org.kde.nfs-share-manager.unmount-share`
- `org.kde.nfs-share-manager.modify-fstab`
- `org.kde.nfs-share-manager.restart-service`
- `org.kde.nfs-share-manager.modify-system-files`

All actions require administrative authentication (`auth_admin_keep`).

## Security Features

### Input Validation
All parameters are validated before execution to prevent injection attacks and ensure data integrity.

### Backup Creation
System files are automatically backed up before modification with timestamps:
- Format: `filename.backup.YYYYMMDD_HHMMSS`
- Location: Same directory as original file

### Audit Logging
All operations are logged using Qt's logging framework with the `nfs.policykit` category.

### Error Handling
Comprehensive error handling with user-friendly messages:
- Network connectivity issues
- Permission denied scenarios
- Missing system dependencies
- Invalid parameters

## Installation

1. Install the PolicyKit policy file:
   ```bash
   sudo cp policy/org.kde.nfs-share-manager.policy /usr/share/polkit-1/actions/
   ```

2. Ensure PolicyKit daemon is running:
   ```bash
   systemctl status polkit
   ```

3. Verify policy installation:
   ```bash
   pkaction --action-id org.kde.nfs-share-manager.create-share --verbose
   ```

## Testing

The helper includes comprehensive unit tests in `tests/system/test_policykithelper.cpp`:

```bash
# Run PolicyKit helper tests
cd build
./tests/system/test_policykithelper

# Run all tests including PolicyKit
ctest --verbose
```

## Troubleshooting

### PolicyKit Not Available
If PolicyKit is not available, the helper will gracefully degrade and report errors appropriately.

### Permission Denied
Ensure the user is in the appropriate administrative group (sudo, admin, or wheel).

### D-Bus Connection Issues
Verify that the system D-Bus is running and accessible:
```bash
systemctl status dbus
```

### Policy File Issues
Check that the policy file is properly installed and has correct permissions:
```bash
ls -la /usr/share/polkit-1/actions/org.kde.nfs-share-manager.policy
```

## Implementation Notes

- The current implementation uses a simplified authorization check based on user group membership
- In a production environment, you would implement the full PolicyKit D-Bus API
- The helper is designed to be thread-safe and can be used from multiple contexts
- All operations are performed synchronously to ensure proper error handling