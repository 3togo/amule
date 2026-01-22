#!/bin/bash

# aMule Build Requirements Script
# This script installs all necessary dependencies for building aMule

set -e  # Exit on any error

echo "========================================"
echo "Installing aMule Build Dependencies"
echo "========================================"
echo ""

# Check if running as root or with sudo
if [ "$EUID" -ne 0 ]; then 
    echo "This script requires root privileges."
    echo "Please run with: sudo $0"
    exit 1
fi

echo "Updating package lists..."
apt-get update -y

echo ""
echo "========================================"
echo "Installing Core Build Tools"
echo "========================================"
apt-get install -y \
    build-essential \
    cmake \
    pkg-config

echo ""
echo "========================================"
echo "Installing wxWidgets (GUI Framework)"
echo "========================================"
apt-get install -y \
    libwxgtk3.2-dev \
    wx3.2-headers

echo ""
echo "========================================"
echo "Installing Cryptography Libraries"
echo "========================================"
apt-get install -y \
    libcrypto++-dev \
    libboost-dev

echo ""
echo "========================================"
echo "Installing GeoIP/IP2Country Libraries"
echo "========================================"
apt-get install -y \
    libmaxminddb-dev \
    libgeoip-dev

echo ""
echo "========================================"
echo "Installing Image Processing Libraries"
echo "========================================"
apt-get install -y \
    libgd-dev \
    libjpeg-dev \
    libpng-dev

echo ""
echo "========================================"
echo "Installing Network Libraries"
echo "========================================"
apt-get install -y \
    libcurl4-openssl-dev \
    libasio-dev

echo ""
echo "========================================"
echo "Installing Other Dependencies"
echo "========================================"
apt-get install -y \
    zlib1g-dev \
    libtinyxml-dev \
    libupnp-dev

echo ""
echo "========================================"
echo "Installing Optional Documentation Tools"
echo "========================================"
apt-get install -y \
    gettext \
    intltool

echo ""
echo "========================================"
echo "Installation Complete!"
echo "========================================"
echo ""
echo "Optional: For UPnP support, also install:"
echo "  sudo apt install libupnp-dev"
echo ""
echo "To build aMule:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j$(nproc)"
echo ""
echo "Note: libmaxminddb is REQUIRED for country flag display."
echo "If flags are not showing, run:"
echo "  ldd ./src/amule | grep maxmind"
echo ""
