# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-02-03

### Added
- Initial release of NFS Share Manager
- Local NFS share management (create, edit, remove)
- Remote NFS share discovery with configurable timeouts
- Mount and unmount operations for remote shares
- System tray integration with proper quit behavior
- Configuration persistence for shares and settings
- User-configurable discovery intervals (30 seconds to 1 hour)
- PolicyKit integration for secure privilege escalation
- Desktop notifications for all operations
- Progress indication for long-running operations
- Comprehensive test suite (unit, integration, end-to-end)
- Professional documentation and installation guides
- Pre-built binary releases for Linux x86_64

### Features
- **Local Share Management**: Create NFS shares from local directories with customizable permissions
- **Network Discovery**: Automatic discovery of NFS shares on the network with three scan modes (Quick, Full, Targeted)
- **Mount Management**: Mount remote shares with custom options and automatic unmounting
- **System Integration**: System tray icon, desktop notifications, PolicyKit security
- **Configuration**: Persistent storage of all shares and settings using Qt Settings
- **User Experience**: Intuitive tabbed interface, progress dialogs, error handling

### Technical Details
- Built with Qt6 and KDE Frameworks 6
- CMake build system with comprehensive testing
- Modular architecture (core, business, system, UI layers)
- Cross-platform Linux support (Ubuntu, Fedora, Arch)
- Apache License 2.0 for maximum compatibility

### Security
- PolicyKit integration for privilege escalation
- Input validation and sanitization
- Secure default configurations
- Minimal privilege principle

[1.0.0]: https://github.com/yourusername/nfs-share-manager/releases/tag/v1.0.0