#!/bin/bash

# CCache Setup Script for aMule
# Dramatically speeds up rebuilds by caching compilation results

set -e

echo "========================================"
echo "Setting up CCache for Faster Builds"
echo "========================================"

# Check if ccache is installed
if ! command -v ccache &> /dev/null; then
    echo "Installing ccache..."
    sudo apt update && sudo apt install -y ccache
fi

# Configure ccache
CCACHE_DIR="${CCACHE_DIR:-$HOME/.ccache}"
mkdir -p "$CCACHE_DIR"

# Set ccache size to 10GB
ccache -M 10G

echo ""
echo "CCache Configuration:"
echo "  Cache directory: $CCACHE_DIR"
echo "  Cache size: 10GB"
ccache -s

echo ""
echo "To use CCache with your builds:"
echo "1. Add these flags to your CMake command:"
echo "   -DCMAKE_C_COMPILER_LAUNCHER=ccache"
echo "   -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
echo ""
echo "2. Or export these environment variables:"
echo "   export CC='ccache gcc'"
echo "   export CXX='ccache g++'"
echo ""
echo "Example fast build with CCache:"
echo "  mkdir build_ccache && cd build_ccache"
echo "  cmake .. -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
echo "  make -j\$(nproc)"
echo ""
