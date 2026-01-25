#!/bin/bash

# Ultra-Fast Incremental Build Script
# For rapid development when modifying single files

set -e

BUILD_DIR="${1:-fast_build}"
FILE_TO_BUILD="$2"

if [ -z "$FILE_TO_BUILD" ]; then
    echo "Usage: $0 [build_dir] <source_file>"
    echo "Example: $0 fast_build src/OtherFunctions.cpp"
    exit 1
fi

# Convert source file to object file path
OBJECT_FILE="${FILE_TO_BUILD%.cpp}.o"
OBJECT_FILE="${OBJECT_FILE%.c}.o"
OBJECT_FILE="src/CMakeFiles/mulecommon.dir/${OBJECT_FILE#src/}"

echo "========================================"
echo "Ultra-Fast Incremental Build"
echo "========================================"
echo "Building: $FILE_TO_BUILD"
echo "Target: $OBJECT_FILE"
echo ""

cd "$BUILD_DIR"

# Build only the specific object file
echo "Compiling single file..."
time make -j1 "$OBJECT_FILE"

# Then link if it's part of the main executable
if [[ "$OBJECT_FILE" == *"mulecommon"* ]]; then
    echo ""
    echo "Linking main executable..."
    time make -j$(nproc) amule
fi

echo ""
echo "========================================"
echo "Incremental Build Complete!"
echo "========================================"
echo "File: $FILE_TO_BUILD"
echo "Built: $OBJECT_FILE"
echo ""