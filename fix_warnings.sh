#!/bin/bash
# Script to fix common wxWidgets deprecation warnings

echo "Fixing wxWidgets deprecation warnings..."

# Fix deprecated brush and pen functions
# Replace: FindOrCreateBrush(colour, style) -> new wxBrush(colour, style)
find src/ -name "*.cpp" -exec sed -i 's/FindOrCreateBrush(\([^,]*\), \([0-9]*\))/new wxBrush(\1, \2)/g' {} +

# Replace: FindOrCreatePen(colour, width, style) -> new wxPen(colour, width, style)  
find src/ -name "*.cpp" -exec sed -i 's/FindOrCreatePen(\([^,]*\), \([^,]*\), \([0-9]*\))/new wxPen(\1, \2, \3)/g' {} +

# Fix deprecated wxPATH_NORM_ALL
find src/ -name "*.cpp" -exec sed -i 's/wxPATH_NORM_ALL/wxPATH_NORM_CASE|wxPATH_NORM_SLASHES|wxPATH_NORM_DOTS/g' {} +

# Fix deprecated font constants
find src/ -name "*.cpp" -exec sed -i 's/wxROMAN/wxFONTFAMILY_ROMAN/g' {} +
find src/ -name "*.cpp" -exec sed -i 's/wxNORMAL/wxFONTSTYLE_NORMAL/g' {} +
find src/ -name "*.cpp" -exec sed -i 's/wxBOLD/wxFONTWEIGHT_BOLD/g' {} +
find src/ -name "*.cpp" -exec sed -i 's/wxLIGHT/wxFONTWEIGHT_LIGHT/g' {} +

echo "Done!"
