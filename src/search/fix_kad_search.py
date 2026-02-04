
import re

# Read the file
with open('KadSearchController.cpp', 'r') as f:
    lines = f.readlines()

# Find and replace the problematic section
new_lines = []
skip_until = -1
in_replacement = False

for i, line in enumerate(lines):
    if skip_until > i:
        continue

    # Check if we're at the start of the problematic code
    if '// Generate search ID' in line and i < skip_until + 1:
        # Skip lines until we find the PrepareFindKeywords call
        in_replacement = True
        continue

    if in_replacement:
        # Check if we've reached the PrepareFindKeywords call
        if 'Kademlia::CSearchManager::PrepareFindKeywords' in line:
            # Add the replacement code
            new_lines.append('	// Send packet to Kad network
')
            new_lines.append('	if (theApp && Kademlia::CKademlia::IsRunning()) {
')
            new_lines.append('	    // Use legacy Kad search implementation
')
            new_lines.append('	    try {
')
            new_lines.append('		// Let Kad search manager generate the search ID
')
            new_lines.append(line)
            # Skip the next few lines until we find the searchId declaration
            skip_until = i + 6
            in_replacement = False
            continue
        else:
            continue

    # Check if we need to modify the searchId parameter
    if i == skip_until and 'searchId' in line and 'packetData' in lines[i-1]:
        # Replace searchId with 0
        new_lines.append(line.replace('searchId', '0  // Let Kad search manager generate the search ID'))
        skip_until = -1
        continue

    new_lines.append(line)

# Write the file back
with open('KadSearchController.cpp', 'w') as f:
    f.writelines(new_lines)

print("File fixed successfully!")
