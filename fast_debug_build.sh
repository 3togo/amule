#!/bin/bash

# Fast Debug Build Script for aMule
# Optimized for rapid development and debugging

set -e

BUILD_DIR="${1:-fast_build}"
CMAKE_OPTIONS=(
    -DCMAKE_BUILD_TYPE=Debug
    -DENABLE_IP2COUNTRY=OFF       # Disable GeoIP for faster compile
    -DBUILD_TESTING=OFF           # Disable tests during development
    -DBUILD_WEBSERVER=OFF         # Disable webserver if not needed
    -DBUILD_CAS=OFF               # Disable CAS statistics
    -DBUILD_WXCAS=OFF             # Disable wxCas statistics
    -DBUILD_ALC=OFF               # Disable aLinkCreator GUI
    -DBUILD_ALCC=OFF              # Disable aLinkCreator console
    -DBUILD_XAS=OFF               # Disable XChat plugin
    -DENABLE_UPNP=OFF             # Disable UPNP if not needed
    -DCMAKE_CXX_FLAGS_DEBUG="-O0 -g -fno-omit-frame-pointer"
    -DCMAKE_C_FLAGS_DEBUG="-O0 -g -fno-omit-frame-pointer"
)

echo "========================================"
echo "Setting up Fast Debug Build"
echo "========================================"
echo "Build directory: $BUILD_DIR"
echo ""

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    echo "Created build directory: $BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with optimized debug settings
echo "Configuring CMake with debug optimizations..."
cmake "${CMAKE_OPTIONS[@]}" .. 

# Get number of CPU cores for parallel build
CORES=$(nproc)
if [ "$CORES" -gt 8 ]; then
    # For systems with many cores, use slightly fewer to avoid memory issues
    BUILD_CORES=$((CORES - 2))
else
    BUILD_CORES=$CORES
fi

echo ""
echo "========================================"
echo "Building with $BUILD_CORES parallel jobs"
echo "========================================"

# Build only the main target first for fastest iteration
time make -j$BUILD_CORES amule

echo ""
echo "========================================"
echo "Fast Debug Build Complete!"
echo "========================================"
echo ""
echo "Optional: Build other components as needed:"
echo "  make -j$BUILD_CORES all       # Build everything"
echo "  make -j$BUILD_CORES muleapp   # Build just the main app"
echo ""
echo "Debug tips:"
echo "  export ASAN_OPTIONS=detect_leaks=1  # Enable address sanitizer"
echo "  gdb ./src/amule                   # Debug with gdb"
echo ""