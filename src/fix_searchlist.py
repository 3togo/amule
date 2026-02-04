#!/usr/bin/env python3
with open("SearchList.cpp", "r") as f:
    lines = f.readlines()

# Update line 791 (index 790)
lines[790] = "	// If file is too large for network, drop it\n"

# Update line 801 (index 800) - remove the extra "n" character
lines[800] = "	// Note: Kad search results may have filesize 0\n"
lines[801] = "	// This is acceptable and will be handled by the UI\n"

with open("SearchList.cpp", "w") as f:
    f.writelines(lines)

print("File updated successfully!")
