#!/bin/bash

# NFS Share Manager v1.1.0 Installation Script
# This script helps install dependencies and set up the application

set -e

echo "=== NFS Share Manager v1.1.0 Installation ==="
echo

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo "Warning: Running as root. Consider running as a regular user."
    echo
fi

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    VERSION_ID=${VERSION_ID:-"unknown"}
else
    echo "Cannot detect Linux distribution. Please install dependencies manually."
    exit 1
fi

echo "Detected distribution: $DISTRO $VERSION_ID"
echo

# Check for required tools
echo "Checking system requirements..."
MISSING_TOOLS=()

# Check for basic tools
for tool in exportfs showmount rpcinfo mount umount; do
    if ! command -v $tool >/dev/null 2>&1; then
        MISSING_TOOLS+=($tool)
    fi
done

if [ ${#MISSING_TOOLS[@]} -gt 0 ]; then
    echo "Missing NFS tools: ${MISSING_TOOLS[*]}"
    echo "These will be installed with NFS utilities package."
fi

# Install dependencies based on distribution
case $DISTRO in
    ubuntu|debian)
        echo "Installing dependencies for Ubuntu/Debian..."
        sudo apt update
        
        # Core Qt6 and KDE packages
        sudo apt install -y \
            qt6-base-dev \
            libkf6notifications-dev \
            libkf6configcore-dev \
            nfs-common \
            polkit-kde-agent-1 \
            avahi-utils \
            net-tools
        
        # Optional but recommended
        sudo apt install -y \
            nfs-kernel-server \
            rpcbind \
            || echo "Note: Some optional packages may not be available"
        ;;
    fedora|rhel|centos)
        echo "Installing dependencies for Fedora/RHEL..."
        sudo dnf install -y \
            qt6-qtbase-devel \
            kf6-knotifications-devel \
            kf6-kconfigcore-devel \
            nfs-utils \
            polkit-kde \
            avahi-tools \
            net-tools
        
        # Optional but recommended
        sudo dnf install -y \
            nfs-server \
            rpcbind \
            || echo "Note: Some optional packages may not be available"
        ;;
    arch|manjaro)
        echo "Installing dependencies for Arch Linux..."
        sudo pacman -S --needed \
            qt6-base \
            kf6-knotifications \
            kf6-kconfigcore \
            nfs-utils \
            polkit-kde-agent \
            avahi \
            net-tools
        ;;
    opensuse*|sles)
        echo "Installing dependencies for openSUSE/SLES..."
        sudo zypper install -y \
            qt6-base-devel \
            kf6-knotifications-devel \
            kf6-kconfigcore-devel \
            nfs-client \
            polkit-kde-agent-6 \
            avahi-utils \
            net-tools-deprecated
        ;;
    *)
        echo "Unsupported distribution: $DISTRO"
        echo "Please install the following packages manually:"
        echo "- Qt6 base libraries"
        echo "- KDE Frameworks 6 (KNotifications, KConfigCore)"
        echo "- NFS utilities (nfs-utils, nfs-common)"
        echo "- PolicyKit"
        echo "- Avahi utilities (optional, for enhanced discovery)"
        echo "- Network tools (net-tools)"
        exit 1
        ;;
esac

echo
echo "Configuring services..."

# Enable and start NFS-related services
if systemctl list-unit-files | grep -q rpcbind; then
    echo "Enabling rpcbind service..."
    sudo systemctl enable rpcbind
    sudo systemctl start rpcbind || echo "Note: rpcbind may already be running"
fi

if systemctl list-unit-files | grep -q nfs-server; then
    echo "NFS server available. You can enable it with:"
    echo "  sudo systemctl enable nfs-server"
    echo "  sudo systemctl start nfs-server"
fi

echo
echo "Making binary executable..."
chmod +x nfs-share-manager

# Install PolicyKit policy
if [ -f "policy/org.kde.nfs-share-manager.policy" ]; then
    echo "Installing PolicyKit policy..."
    sudo cp policy/org.kde.nfs-share-manager.policy /usr/share/polkit-1/actions/
    echo "PolicyKit policy installed."
fi

echo
echo "Verifying installation..."
VERIFICATION_FAILED=false

# Check NFS tools again
for tool in exportfs showmount rpcinfo; do
    if ! command -v $tool >/dev/null 2>&1; then
        echo "ERROR: $tool not found after installation"
        VERIFICATION_FAILED=true
    fi
done

# Check if binary can find Qt libraries
if ! ldd nfs-share-manager | grep -q libQt6; then
    echo "WARNING: Qt6 libraries may not be properly linked"
    echo "You may need to install additional Qt6 runtime packages"
fi

if [ "$VERIFICATION_FAILED" = true ]; then
    echo
    echo "Installation completed with warnings. Some features may not work properly."
else
    echo "Installation verification successful!"
fi

echo
echo "=== Installation Complete ==="
echo
echo "To run the application:"
echo "  ./nfs-share-manager"
echo
echo "For system-wide installation (optional):"
echo "  sudo cp nfs-share-manager /usr/local/bin/"
echo "  # PolicyKit policy already installed"
echo
echo "New in v1.1.0:"
echo "  • Enhanced network discovery with configurable scan modes"
echo "  • Complete scan mode for exhaustive network discovery"
echo "  • Configurable discovery timeout (30s - 5 minutes)"
echo "  • Automatic local network access for created shares"
echo "  • Improved performance and progress reporting"
echo
echo "Enjoy using NFS Share Manager v1.1.0!"