#!/bin/bash

# Script to format SearchDlg.cpp using clang-format

FILE="/home/eli/git/amule/src/SearchDlg.cpp"

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    echo "clang-format not found. Installing..."
    sudo apt-get update && sudo apt-get install -y clang-format
fi

# Create a .clang-format configuration file
cat > /home/eli/git/amule/.clang-format << 'EOF'
BasedOnStyle: Google
IndentWidth: 4
TabWidth: 4
UseTab: Always
ColumnLimit: 120
PointerAlignment: Left
EOF

# Format the file
echo "Formatting $FILE..."
clang-format -i "$FILE"

echo "Done! File has been formatted."
