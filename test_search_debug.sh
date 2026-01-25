#!/bin/bash
# Test script to verify search debug logging

echo "=== Testing aMule Search Debug Logging ==="
echo "Current time: $(date)"
echo "Log file: ~/.aMule/logfile"
echo ""

# Check if aMule is running
if pgrep -x "amule" > /dev/null; then
    echo "✓ aMule is running"
    echo "✓ Debug logging enabled in config"
    echo "✓ Search-specific debug categories enabled"
    echo ""
    echo "To test search functionality:"
    echo "1. Perform a search in aMule GUI"
    echo "2. Check log file for debug messages:"
    echo "   tail -f ~/.aMule/logfile | grep -E \"(StartNewSearch|ProcessSearchAnswer|AddToList)\""
    echo ""
    echo "Expected debug messages:"
    echo "- StartNewSearch: with search parameters"
    echo "- ProcessSearchAnswer: with result count from servers"  
    echo "- AddToList: when results are added to display"
    echo ""
    echo "If no debug messages appear, try restarting aMule:"
    echo "pkill amule && sleep 2 && ./src/amule &"
else
    echo "✗ aMule is not running"
    echo "Start aMule with: ./src/amule"
fi