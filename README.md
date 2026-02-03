# NFS Share Manager

[![License: Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Platform](https://img.shields.io/badge/Platform-Linux-green.svg)](https://www.linux.org/)
[![Qt](https://img.shields.io/badge/Qt-6.x-brightgreen.svg)](https://www.qt.io/)
[![KDE](https://img.shields.io/badge/KDE-Frameworks%206-blue.svg)](https://kde.org/)

A comprehensive KDE Plasma application for managing NFS shares and mounts with an intuitive graphical interface.

![NFS Share Manager Screenshot](docs/screenshot.png)

## Features

- 🗂️ **Local NFS Share Management**: Create, edit, and remove NFS shares from local directories
- 🔍 **Remote Share Discovery**: Automatically discover NFS shares on your network
- 💾 **Mount Management**: Mount and unmount remote NFS shares with custom options
- 🔔 **System Tray Integration**: Minimize to system tray for background operation
- 💾 **Configuration Persistence**: All shares and settings are saved and restored on restart
- ⏱️ **User-Configurable Discovery**: Set custom timeouts for network discovery (30s - 1 hour)
- 🔒 **PolicyKit Integration**: Secure privilege escalation for system operations
- 📢 **Desktop Notifications**: Real-time notifications for all operations
- 📊 **Progress Indication**: Visual feedback for long-running operations

## Quick Start

### Download Pre-built Binary

1. **Download the latest release**:
   ```bash
   wget https://github.com/yourusername/nfs-share-manager/releases/download/v1.0.0/nfs-share-manager-v1.0.0-linux-x86_64.tar.gz
   ```

2. **Extract and install**:
   ```bash
   tar -xzf nfs-share-manager-v1.0.0-linux-x86_64.tar.gz
   cd nfs-share-manager-v1.0.0-linux-x86_64
   ./install.sh
   ```

3. **Run the application**:
   ```bash
   ./nfs-share-manager
   ```

### Build from Source

#### Prerequisites

- **Qt6** (6.2 or later)
- **KDE Frameworks 6**
- **CMake** (3.16 or later)
- **C++17** compatible compiler
- **NFS utilities** (`nfs-common` or `nfs-utils`)
- **PolicyKit** development files

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev libkf6notifications-dev \
                 libkf6configcore-dev nfs-common polkit-kde-agent-1
```

#### Fedora/RHEL
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel kf6-knotifications-devel \
                 kf6-kconfigcore-devel nfs-utils polkit-kde
```

#### Build Steps
```bash
git clone https://github.com/yourusername/nfs-share-manager.git
cd nfs-share-manager
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### Run
```bash
./src/nfs-share-manager
```

## Usage

### Creating Local NFS Shares

1. Open the **Local Shares** tab
2. Click **Create Share**
3. Select a directory to share
4. Configure export options and permissions
5. Click **OK** (requires admin authentication)

### Discovering Remote Shares

1. Go to **Remote Shares** tab
2. Click **Refresh** to scan the network
3. Enable **Auto Discovery** for periodic scanning
4. Configure discovery interval in **Settings → Preferences**

### Mounting Remote Shares

1. Select a discovered share
2. Click **Mount Share**
3. Choose local mount point
4. Configure mount options
5. Click **OK**

## Configuration

All configuration is stored using Qt Settings and persists between sessions:

- **Shares**: Local share configurations
- **Discovery Settings**: Auto-discovery preferences and intervals
- **Mount Options**: Default mount parameters
- **UI Preferences**: Window state and notification settings

## Architecture

The application follows a modular architecture:

```
src/
├── core/           # Core data structures and configuration
├── business/       # Business logic (ShareManager, MountManager, etc.)
├── system/         # System integration (PolicyKit, NFS commands)
└── ui/             # User interface components
```

## Security

- **PolicyKit Integration**: All privileged operations use PolicyKit for secure authentication
- **Privilege Separation**: Minimal privilege escalation only when necessary
- **Input Validation**: All user inputs are validated and sanitized
- **Secure Defaults**: Conservative default settings for shares and mounts

## Testing

The project includes comprehensive tests:

```bash
# Run all tests
cd build
make test

# Run specific test categories
./tests/core/test_configurationmanager
./tests/business/test_sharemanager
./tests/integration/test_end_to_end_simple
```

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes and add tests
4. Ensure all tests pass: `make test`
5. Commit your changes: `git commit -m 'Add amazing feature'`
6. Push to the branch: `git push origin feature/amazing-feature`
7. Open a Pull Request

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

This means you can:
- ✅ Use the software for any purpose
- ✅ Distribute the software
- ✅ Modify the software
- ✅ Distribute modified versions
- ✅ Use the software privately
- ✅ Use the software commercially
- ✅ Place warranty on the software (if you want)

The only requirements are:
- 📄 Include the original copyright notice
- 📄 Include the Apache License 2.0
- 📄 State significant changes made to the software

## Acknowledgments

- Built with [Qt6](https://www.qt.io/) and [KDE Frameworks 6](https://kde.org/)
- Uses [CMake](https://cmake.org/) for build system
- Inspired by the need for better NFS management tools on Linux
- Thanks to the KDE community for excellent frameworks and guidelines

## Support

- 📖 **Documentation**: Check the [docs/](docs/) directory
- 🐛 **Bug Reports**: [GitHub Issues](https://github.com/yourusername/nfs-share-manager/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/yourusername/nfs-share-manager/discussions)
- 📧 **Contact**: [your.email@example.com](mailto:your.email@example.com)

## Roadmap

- [ ] Support for additional NFS versions (NFSv2, NFSv4.1+)
- [ ] Integration with other network filesystems (CIFS/SMB)
- [ ] Enhanced security options and encryption
- [ ] Mobile/remote management interface
- [ ] Automated backup and sync features

---

**NFS Share Manager** - Making NFS management simple and secure on Linux.