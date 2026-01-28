#!/bin/bash

# Script to normalize whitespace in SearchDlg.cpp
# Converts all indentation to tabs and removes trailing whitespace

FILE="/home/eli/git/amule/src/SearchDlg.cpp"

echo "Normalizing whitespace in $FILE..."

# Backup the original file
cp "$FILE" "$FILE.backup"

# Convert leading spaces to tabs (4 spaces = 1 tab)
sed -i 's/^        /	/g' "$FILE"
sed -i 's/^	        /		/g' "$FILE"
sed -i 's/^		        /			/g' "$FILE"
sed -i 's/^			        /				/g' "$FILE"

# Remove trailing whitespace
sed -i 's/[[:space:]]*$//' "$FILE"

# Ensure file ends with a newline
if [ -n "$(tail -c1 "$FILE")" ]; then
    echo >> "$FILE"
fi

echo "Done! Original file backed up to $FILE.backup"
echo "To compare: diff -u $FILE.backup $FILE"
