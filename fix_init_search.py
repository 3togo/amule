#!/usr/bin/env python3

# Read the file
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'r') as f:
    content = f.read()

# Define the old and new content
old_content = """	// Initialize search state tracking
	m_stateManager.InitializeSearch(searchId,
		searchType == search::ModernSearchType::KadSearch ? wxT("Kad") : wxT("ED2K"),
		searchString, {});"""

new_content = """	// Initialize search state tracking
	m_stateManager.InitializeSearch(searchId,
		searchType == search::ModernSearchType::KadSearch ? wxT("Kad") : wxT("ED2K"),
		searchString, CreateSearchParamsFromUI(searchString, searchType));"""

# Replace the content
if old_content in content:
    content = content.replace(old_content, new_content)
    print("Successfully replaced the InitializeSearch call")
else:
    print("Could not find the target content")

# Write the file back
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'w') as f:
    f.write(content)

# Verify the file
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'r') as f:
    lines = f.readlines()

print(f"Total lines in file: {len(lines)}")
print("File updated successfully")
