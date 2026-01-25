#!/bin/bash

echo "=== Quick Verification of Magnet Implementation ==="
echo "Current directory: $(pwd)"
echo ""

# Check if main executable exists
if [ -f "/home/eli/git/amule/cpp20_build/src/amule" ]; then
    echo "✓ aMule executable found"
    echo "  Size: $(du -h /home/eli/git/amule/cpp20_build/src/amule | cut -f1)"
else
    echo "✗ aMule executable not found"
    exit 1
fi

# Check key files were created
echo ""
echo "Checking key implementation files:"

FILES=(
    "/home/eli/git/amule/src/MagnetProgressTracker.h"
    "/home/eli/git/amule/src/MagnetProgressTracker.cpp"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ $file exists ($(wc -l < "$file") lines)"
    else
        echo "✗ $file missing"
    fi
done

# Check critical modifications
echo ""
echo "Checking critical modifications:"

# Check Constants.h
if grep -q "PS_CONVERTING_MAGNET" /home/eli/git/amule/src/Constants.h; then
    echo "✓ PS_CONVERTING_MAGNET defined in Constants.h"
else
    echo "✗ PS_CONVERTING_MAGNET not found in Constants.h"
fi

# Check PartFile.h
if grep -q "m_magnetConversionProgress" /home/eli/git/amule/src/PartFile.h; then
    echo "✓ Magnet progress tracking implemented in PartFile.h"
else
    echo "✗ Magnet progress tracking missing from PartFile.h"
fi

# Check DownloadListCtrl.cpp
if grep -q "PS_CONVERTING_MAGNET" /home/eli/git/amule/src/DownloadListCtrl.cpp; then
    echo "✓ Visual progress handling in DownloadListCtrl.cpp"
else
    echo "✗ Visual progress handling missing from DownloadListCtrl.cpp"
fi

echo ""
echo "=== Verification Complete ==="