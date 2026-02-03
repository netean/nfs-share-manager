#!/bin/bash

# NFS Share Manager Installation Script
# This script helps install dependencies and set up the application

set -e

echo "=== NFS Share Manager Installation ==="
echo

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
else
    echo "Cannot detect Linux distribution. Please install dependencies manually."
    exit 1
fi

echo "Detected distribution: $DISTRO"
echo

# Install dependencies based on distribution
case $DISTRO in
    ubuntu|debian)
        echo "Installing dependencies for Ubuntu/Debian..."
        sudo apt update
        sudo apt install -y qt6-base-dev libkf6notifications-dev libkf6configcore-dev nfs-common polkit-kde-agent-1
        ;;
    fedora|rhel|centos)
        echo "Installing dependencies for Fedora/RHEL..."
        sudo dnf install -y qt6-qtbase-devel kf6-knotifications-devel kf6-kconfigcore-devel nfs-utils polkit-kde
        ;;
    arch|manjaro)
        echo "Installing dependencies for Arch Linux..."
        sudo pacman -S --needed qt6-base kf6-knotifications kf6-kconfigcore nfs-utils polkit-kde-agent
        ;;
    *)
        echo "Unsupported distribution: $DISTRO"
        echo "Please install the following packages manually:"
        echo "- Qt6 base libraries"
        echo "- KDE Frameworks 6 (KNotifications, KConfigCore)"
        echo "- NFS utilities"
        echo "- PolicyKit"
        exit 1
        ;;
esac

echo
echo "Making binary executable..."
chmod +x nfs-share-manager

echo
echo "Installation completed successfully!"
echo
echo "To run the application:"
echo "  ./nfs-share-manager"
echo
echo "To install system-wide (optional):"
echo "  sudo cp nfs-share-manager /usr/local/bin/"
echo "  sudo cp policy/org.kde.nfs-share-manager.policy /usr/share/polkit-1/actions/"
echo
echo "Enjoy using NFS Share Manager!"