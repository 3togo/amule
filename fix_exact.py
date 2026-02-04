#!/usr/bin/env python3

# Read the file
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'r') as f:
    content = f.read()

# Define the old and new content with exact tabs
old_content = "		// Add results to the list
		for (CSearchFile* result : results) {
			AddResult(result);
		}
		
		// Update hit count
		CSearchListCtrl* list = GetSearchList(searchId);
		if (list) {
			UpdateHitCount(list);
		}
	});"

new_content = "		// Get the list control for this search
		CSearchListCtrl* list = GetSearchList(searchId);
		if (!list) {
			return;
		}
		
		// Refresh the list to show new results
		// The results are already in SearchList, we just need to refresh the display
		list->ShowResults(searchId);
		
		// Update hit count
		UpdateHitCount(list);
	});"

# Replace the content
if old_content in content:
    content = content.replace(old_content, new_content)
    print("Successfully replaced the callback content")
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
