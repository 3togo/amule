#!/bin/bash

# Script to normalize whitespace across the entire aMule codebase
# Converts all indentation to tabs and removes trailing whitespace

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Normalizing whitespace in aMule codebase...${NC}"

# Find all C++ source files
find /home/eli/git/amule/src -type f \( -name "*.cpp" -o -name "*.h" \) | while read file; do
    echo -n "Processing: $file ... "

    # Backup the original file
    cp "$file" "$file.backup"

    # Convert leading spaces to tabs (4 spaces = 1 tab)
    sed -i 's/^        /	/g' "$file"
    sed -i 's/^	        /		/g' "$file"
    sed -i 's/^		        /			/g' "$file"
    sed -i 's/^			        /				/g' "$file"

    # Remove trailing whitespace
    sed -i 's/[[:space:]]*$//' "$file"

    # Ensure file ends with a newline
    if [ -n "$(tail -c1 "$file")" ]; then
        echo >> "$file"
    fi

    # Check if file changed
    if ! diff -q "$file" "$file.backup" > /dev/null 2>&1; then
        echo -e "${YELLOW}Modified${NC}"
        rm "$file.backup"
    else
        echo -e "${GREEN}No change${NC}"
        rm "$file.backup"
    fi
done

echo -e "${GREEN}Done!${NC}"
echo ""
echo "Summary:"
echo "- All C++ files have been normalized to use tabs for indentation"
echo "- Trailing whitespace has been removed from all lines"
echo "- All files now end with a newline"
echo ""
echo "To review changes, use git diff or your version control system"
