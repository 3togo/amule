#!/usr/bin/env python3
# Read the file
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'r') as f:
    lines = f.readlines()

# Process the file
output = []
skip_lines = False
for i, line in enumerate(lines):
    line_num = i + 1

    # Check if we're in the target section (lines 799-808)
    if 799 <= line_num <= 808:
        if line_num == 799:
            # Add the new code
            output.append('		// Get the list control for this search\n')
            output.append('		CSearchListCtrl* list = GetSearchList(searchId);\n')
            output.append('		if (!list) {\n')
            output.append('			return;\n')
            output.append('		}\n')
            output.append('\n')
            output.append('		// Refresh the list to show new results\n')
            output.append('		// The results are already in SearchList, we just need to refresh the display\n')
            output.append('		list->ShowResults(searchId);\n')
            output.append('\n')
            output.append('		// Update hit count\n')
            output.append('		UpdateHitCount(list);\n')
        skip_lines = True
    elif not skip_lines:
        output.append(line)
    else:
        if line_num == 808:
            skip_lines = False

# Write the file back
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'w') as f:
    f.writelines(output)

# Verify the file
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'r') as f:
    new_lines = f.readlines()

print(f"Original lines: {len(lines)}")
print(f"New lines: {len(new_lines)}")
print("File updated successfully")
