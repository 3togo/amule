#!/bin/bash

# Magnet Conversion Progress Implementation Test Script
echo "=================================================="
echo "Testing Magnet Conversion Progress Implementation"
echo "=================================================="

# Check if amule executable exists
AMULE_PATH="../cpp20_build/src/amule"
if [ -f "$AMULE_PATH" ]; then
    echo "✓ aMule executable found: $AMULE_PATH"
    echo "  Size: $(du -h "$AMULE_PATH" | cut -f1)"
    echo "  Build timestamp: $(stat -c %y "$AMULE_PATH")"
else
    echo "✗ aMule executable not found at $AMULE_PATH!"
    echo "Checking alternative locations..."
    
    # Check other possible build directories
    ALTERNATIVE_PATHS=(
        "../super_fast_build/src/amule"
        "../build/src/amule" 
        "../build/debug/src/amule"
    )
    
    FOUND=false
    for path in "${ALTERNATIVE_PATHS[@]}"; do
        if [ -f "$path" ]; then
            AMULE_PATH="$path"
            echo "✓ Found alternative: $AMULE_PATH"
            FOUND=true
            break
        fi
    done
    
    if [ "$FOUND" = false ]; then
        echo "✗ No aMule executable found in any build directory!"
        exit 1
    fi
fi

# Check for compilation artifacts
echo ""
echo "Checking compilation artifacts:"
echo "-------------------------------"

# Check if object files were created (try multiple build directories)
BUILD_DIRS=("../cpp20_build" "../super_fast_build" "../build" "../build/debug")
FOUND_DOWNLOADQUEUE=false
FOUND_MAGNETTRACKER=false

for build_dir in "${BUILD_DIRS[@]}"; do
    if [ -f "$build_dir/src/CMakeFiles/amule.dir/DownloadQueue.cpp.o" ]; then
        echo "✓ DownloadQueue.cpp compiled successfully in $build_dir"
        FOUND_DOWNLOADQUEUE=true
    fi
    if [ -f "$build_dir/src/CMakeFiles/amule.dir/MagnetProgressTracker.cpp.o" ]; then
        echo "✓ MagnetProgressTracker.cpp compiled successfully in $build_dir"
        FOUND_MAGNETTRACKER=true
    fi
done

if [ "$FOUND_DOWNLOADQUEUE" = false ]; then
    echo "✗ DownloadQueue.cpp object file not found in any build directory"
fi
if [ "$FOUND_MAGNETTRACKER" = false ]; then
    echo "✗ MagnetProgressTracker.cpp object file not found in any build directory"
fi

# Verify new header files exist
echo ""
echo "Checking header files:"
echo "----------------------"

if [ -f "../src/MagnetProgressTracker.h" ]; then
    echo "✓ MagnetProgressTracker.h exists"
    echo "  Lines: $(wc -l < ../src/MagnetProgressTracker.h)"
else
    echo "✗ MagnetProgressTracker.h not found"
fi

if [ -f "../src/MagnetProgressTracker.cpp" ]; then
    echo "✓ MagnetProgressTracker.cpp exists"
    echo "  Lines: $(wc -l < ../src/MagnetProgressTracker.cpp)"
else
    echo "✗ MagnetProgressTracker.cpp not found"
fi

# Check for symbol existence in executable
echo ""
echo "Checking for implemented symbols:"
echo "---------------------------------"

# Use nm to check for symbols (simplified check)
if nm "../cpp20_build/src/amule" | grep -q "UpdateMagnetConversionProgress"; then
    echo "✓ UpdateMagnetConversionProgress symbol found in executable"
else
    echo "✗ UpdateMagnetConversionProgress symbol not found"
fi

if nm "../cpp20_build/src/amule" | grep -q "PS_CONVERTING_MAGNET"; then
    echo "✓ PS_CONVERTING_MAGNET symbol found in executable"
else
    echo "✗ PS_CONVERTING_MAGNET symbol not found"
fi

# Verify Constants.h was modified
echo ""
echo "Checking Constants.h modification:"
echo "----------------------------------"

if grep -q "PS_CONVERTING_MAGNET" "../src/Constants.h"; then
    echo "✓ PS_CONVERTING_MAGNET constant found in Constants.h"
    PS_LINE=$(grep -n "PS_CONVERTING_MAGNET" "../src/Constants.h")
    echo "  Line: $PS_LINE"
else
    echo "✗ PS_CONVERTING_MAGNET constant not found in Constants.h"
fi

# Verify PartFile.h modifications
echo ""
echo "Checking PartFile.h modifications:"
echo "----------------------------------"

if grep -q "m_magnetConversionProgress" "../src/PartFile.h"; then
    echo "✓ m_magnetConversionProgress member found in PartFile.h"
else
    echo "✗ m_magnetConversionProgress member not found"
fi

if grep -q "SetMagnetConversionProgress" "../src/PartFile.h"; then
    echo "✓ SetMagnetConversionProgress method found in PartFile.h"
else
    echo "✗ SetMagnetConversionProgress method not found"
fi

# Verify DownloadListCtrl.cpp modifications
echo ""
echo "Checking DownloadListCtrl.cpp modifications:"
echo "--------------------------------------------"

if grep -q "PS_CONVERTING_MAGNET" "../src/DownloadListCtrl.cpp"; then
    echo "✓ PS_CONVERTING_MAGNET handling found in DownloadListCtrl.cpp"
    DL_LINES=$(grep -n "PS_CONVERTING_MAGNET" "../src/DownloadListCtrl.cpp")
    echo "  Lines found:"
    echo "$DL_LINES"
else
    echo "✗ PS_CONVERTING_MAGNET handling not found in DownloadListCtrl.cpp"
fi

# Summary
echo ""
echo "=================================================="
echo "Test Summary:"
echo "=================================================="

# Count successes and failures
SUCCESS_COUNT=$(grep -c "✓" "$0")
FAILURE_COUNT=$(grep -c "✗" "$0")

echo "Successful checks: $SUCCESS_COUNT"
echo "Failed checks: $FAILURE_COUNT"

if [ $FAILURE_COUNT -eq 0 ]; then
    echo "🎉 All tests passed! Magnet conversion progress implementation appears complete."
    echo ""
    echo "Next steps:"
    echo "1. Run aMule to test visual progress bars"
    echo "2. Add magnet links to verify real-time updates"
    echo "3. Test error handling with invalid magnet links"
else
    echo "⚠️  Some tests failed. Please check the implementation."
    exit 1
fi

echo "=================================================="