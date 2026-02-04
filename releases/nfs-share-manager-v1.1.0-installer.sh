#!/bin/bash

# NFS Share Manager v1.1.0 - One-Click Installer
# This script downloads and installs NFS Share Manager with all dependencies

set -e

RELEASE_URL="https://github.com/netean/nfs-share-manager/releases/download/v1.1.0/nfs-share-manager-v1.1.0-linux-x86_64.tar.gz"
TEMP_DIR="/tmp/nfs-share-manager-install"
INSTALL_DIR="$HOME/.local/share/nfs-share-manager"

echo "=== NFS Share Manager v1.1.0 One-Click Installer ==="
echo

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    echo "Warning: Running as root. Consider running as a regular user."
    INSTALL_DIR="/opt/nfs-share-manager"
fi

# Create temporary directory
echo "Creating temporary directory..."
mkdir -p "$TEMP_DIR"
cd "$TEMP_DIR"

# Download release package
echo "Downloading NFS Share Manager v1.1.0..."
if command -v wget >/dev/null 2>&1; then
    wget -O nfs-share-manager-v1.1.0.tar.gz "$RELEASE_URL"
elif command -v curl >/dev/null 2>&1; then
    curl -L -o nfs-share-manager-v1.1.0.tar.gz "$RELEASE_URL"
else
    echo "Error: Neither wget nor curl found. Please install one of them."
    exit 1
fi

# Extract package
echo "Extracting package..."
tar -xzf nfs-share-manager-v1.1.0.tar.gz

# Create installation directory
echo "Creating installation directory: $INSTALL_DIR"
mkdir -p "$INSTALL_DIR"

# Copy files
echo "Installing files..."
cp -r nfs-share-manager-v1.1.0-linux-x86_64/* "$INSTALL_DIR/"

# Make binary executable
chmod +x "$INSTALL_DIR/nfs-share-manager"

# Run dependency installation
echo "Installing dependencies..."
cd "$INSTALL_DIR"
./install.sh

# Create desktop entry
DESKTOP_FILE="$HOME/.local/share/applications/nfs-share-manager.desktop"
if [ "$EUID" -eq 0 ]; then
    DESKTOP_FILE="/usr/share/applications/nfs-share-manager.desktop"
fi

echo "Creating desktop entry..."
mkdir -p "$(dirname "$DESKTOP_FILE")"
cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Name=NFS Share Manager
Comment=Manage NFS shares and network discovery
Exec=$INSTALL_DIR/nfs-share-manager
Icon=folder-network
Terminal=false
Type=Application
Categories=System;Network;
Keywords=NFS;Network;Share;Mount;
StartupNotify=true
EOF

# Create symlink for command line access
if [ "$EUID" -eq 0 ]; then
    ln -sf "$INSTALL_DIR/nfs-share-manager" /usr/local/bin/nfs-share-manager
else
    mkdir -p "$HOME/.local/bin"
    ln -sf "$INSTALL_DIR/nfs-share-manager" "$HOME/.local/bin/nfs-share-manager"
    
    # Add to PATH if not already there
    if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
        echo "Added $HOME/.local/bin to PATH in .bashrc"
    fi
fi

# Cleanup
echo "Cleaning up..."
rm -rf "$TEMP_DIR"

echo
echo "=== Installation Complete ==="
echo
echo "NFS Share Manager v1.1.0 has been installed to: $INSTALL_DIR"
echo
echo "To run the application:"
echo "  • From desktop: Look for 'NFS Share Manager' in your applications menu"
echo "  • From terminal: nfs-share-manager"
echo "  • Direct path: $INSTALL_DIR/nfs-share-manager"
echo
echo "New features in v1.1.0:"
echo "  • Enhanced network discovery with configurable scan modes"
echo "  • Complete scan mode for exhaustive network discovery"
echo "  • Configurable discovery timeout (30s - 5 minutes)"
echo "  • Automatic local network access for created shares"
echo "  • Improved performance and progress reporting"
echo
echo "For usage instructions, see: $INSTALL_DIR/USAGE.md"
echo "For troubleshooting, see: $INSTALL_DIR/README.md"
echo
echo "Enjoy using NFS Share Manager v1.1.0!"