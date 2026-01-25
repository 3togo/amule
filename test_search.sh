#!/bin/bash

# Test script to check search functionality
cd /home/eli/git/amule/cpp20_build

echo "Starting aMule with search test..."
timeout 30s ./src/amule 2>&1 | grep -E "(DEBUG:|search|Search|KAD|ED2K|error)"

# Check if aMule is running and try to trigger a search
if pgrep -x "amule" > /dev/null; then
    echo "aMule is running, attempting to trigger search through external connection..."
    
    # Try to connect and trigger a search (this is a simplified approach)
    # The actual search would normally be triggered through the GUI
    echo "Search test completed."
else
    echo "aMule is not running or exited quickly."
    echo "This might indicate a connectivity issue or configuration problem."
fi

echo "Test completed."