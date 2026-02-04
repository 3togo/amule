#!/usr/bin/env python3
# Read the file
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'r') as f:
    lines = f.readlines()

# Find the lines to replace (799-807)
new_lines = []
skip = False
for i, line in enumerate(lines):
    line_num = i + 1
    if 799 <= line_num <= 807:
        if line_num == 799:
            new_lines.append('		// Get the list control for this search
')
            new_lines.append('		CSearchListCtrl* list = GetSearchList(searchId);
')
            new_lines.append('		if (!list) {
')
            new_lines.append('			return;
')
            new_lines.append('		}
')
            new_lines.append('
')
            new_lines.append('		// Refresh the list to show new results
')
            new_lines.append('		// The results are already in SearchList, we just need to refresh the display
')
            new_lines.append('		list->ShowResults(searchId);
')
            new_lines.append('
')
            new_lines.append('		// Update hit count
')
            new_lines.append('		UpdateHitCount(list);
')
        skip = True
    if not skip:
        new_lines.append(line)
    else:
        if line_num == 807:
            skip = False

# Write the file back
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'w') as f:
    f.writelines(new_lines)

print("File updated successfully")
