#!/bin/bash

# NFS Share Manager v1.1.0 Release Verification Script
# This script verifies the integrity and completeness of the release package

set -e

echo "=== NFS Share Manager v1.1.0 Release Verification ==="
echo

PACKAGE="nfs-share-manager-v1.1.0-linux-x86_64.tar.gz"
TEMP_DIR="/tmp/nfs-verify-$$"

if [ ! -f "$PACKAGE" ]; then
    echo "Error: Release package $PACKAGE not found"
    exit 1
fi

echo "Verifying package: $PACKAGE"
echo "Package size: $(du -h "$PACKAGE" | cut -f1)"
echo

# Extract to temporary directory
mkdir -p "$TEMP_DIR"
tar -xzf "$PACKAGE" -C "$TEMP_DIR"

EXTRACT_DIR="$TEMP_DIR/nfs-share-manager-v1.1.0-linux-x86_64"
if [ ! -d "$EXTRACT_DIR" ]; then
    # Try without version directory
    EXTRACT_DIR="$TEMP_DIR"
fi

cd "$EXTRACT_DIR"

echo "Checking required files..."
REQUIRED_FILES=(
    "nfs-share-manager"
    "README.md"
    "USAGE.md"
    "RELEASE_NOTES.md"
    "VERSION"
    "install.sh"
    "policy/org.kde.nfs-share-manager.policy"
)

MISSING_FILES=()
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ $file (MISSING)"
        MISSING_FILES+=("$file")
    fi
done

echo
echo "Checking binary..."
if [ -x "nfs-share-manager" ]; then
    echo "✓ Binary is executable"
    
    # Check binary size
    SIZE=$(stat -c%s "nfs-share-manager")
    if [ "$SIZE" -gt 1000000 ]; then  # > 1MB
        echo "✓ Binary size looks reasonable: $(du -h nfs-share-manager | cut -f1)"
    else
        echo "⚠ Binary size seems small: $(du -h nfs-share-manager | cut -f1)"
    fi
    
    # Check if it's a valid ELF binary
    if file nfs-share-manager | grep -q "ELF.*executable"; then
        echo "✓ Valid ELF executable"
    else
        echo "✗ Not a valid ELF executable"
    fi
    
    # Check dependencies
    echo "Checking dependencies..."
    if ldd nfs-share-manager | grep -q "libQt6"; then
        echo "✓ Qt6 libraries linked"
    else
        echo "⚠ Qt6 libraries not found (may need to be installed)"
    fi
    
else
    echo "✗ Binary is not executable"
fi

echo
echo "Checking install script..."
if [ -x "install.sh" ]; then
    echo "✓ Install script is executable"
    
    # Check for key distribution support
    if grep -q "ubuntu\|debian\|fedora\|arch" install.sh; then
        echo "✓ Multiple distributions supported"
    else
        echo "⚠ Limited distribution support"
    fi
else
    echo "✗ Install script is not executable"
fi

echo
echo "Checking PolicyKit policy..."
if [ -f "policy/org.kde.nfs-share-manager.policy" ]; then
    if grep -q "org.kde.nfs-share-manager" policy/org.kde.nfs-share-manager.policy; then
        echo "✓ PolicyKit policy looks valid"
    else
        echo "⚠ PolicyKit policy may be malformed"
    fi
else
    echo "✗ PolicyKit policy missing"
fi

echo
echo "Checking documentation..."
DOC_FILES=("README.md" "USAGE.md" "RELEASE_NOTES.md")
for doc in "${DOC_FILES[@]}"; do
    if [ -f "$doc" ]; then
        LINES=$(wc -l < "$doc")
        if [ "$LINES" -gt 10 ]; then
            echo "✓ $doc ($LINES lines)"
        else
            echo "⚠ $doc seems short ($LINES lines)"
        fi
    fi
done

# Check version information
echo
echo "Checking version information..."
if [ -f "VERSION" ]; then
    if grep -q "v1.1.0" VERSION; then
        echo "✓ Version file contains v1.1.0"
    else
        echo "⚠ Version file may not be updated"
    fi
fi

if grep -q "v1.1.0" README.md 2>/dev/null; then
    echo "✓ README mentions v1.1.0"
else
    echo "⚠ README may not be updated for v1.1.0"
fi

# Cleanup
cd /
rm -rf "$TEMP_DIR"

echo
echo "=== Verification Summary ==="
if [ ${#MISSING_FILES[@]} -eq 0 ]; then
    echo "✓ All required files present"
    echo "✓ Release package appears to be complete"
    echo
    echo "Package is ready for distribution!"
else
    echo "✗ Missing files: ${MISSING_FILES[*]}"
    echo "✗ Release package is incomplete"
    exit 1
fi