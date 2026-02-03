#!/usr/bin/env python3
import sys

# Read the file
with open('src/kademlia/kademlia/Search.cpp', 'r') as f:
    lines = f.readlines()

# Find the destructor and replace it
new_lines = []
for i, line in enumerate(lines):
    if i >= 174 and i <= 183:  # Lines 175-183 (0-indexed 174-183)
        if i == 174:
            new_lines.append(line)  # Keep the destructor signature
        elif i == 183 and line.strip() == '}':
            # Add the new destructor implementation before the closing brace
            new_lines.append('\tm_destructing = true;\n')
            new_lines.append('\n')
            new_lines.append('\t// remember the closest node we found and tried to contact (if any) during this search\n')
            new_lines.append('\t// for statistical caluclations, but only if its a certain type\n')
            new_lines.append('\tswitch (m_type) {\n')
            new_lines.append('\t\tcase NODECOMPLETE:\n')
            new_lines.append('\t\tcase FILE:\n')
            new_lines.append('\t\tcase KEYWORD:\n')
            new_lines.append('\t\tcase NOTES:\n')
            new_lines.append('\t\tcase STOREFILE:\n')
            new_lines.append('\t\tcase STOREKEYWORD:\n')
            new_lines.append('\t\tcase STORENOTES:\n')
            new_lines.append('\t\tcase FINDSOURCE: // maybe also exclude\n')
            new_lines.append('\t\t\tif (m_closestDistantFound != 0) {\n')
            new_lines.append('\t\t\t\tCKademlia::StatsAddClosestDistance(m_closestDistantFound);\n')
            new_lines.append('\t\t\t}\n')
            new_lines.append('\t\t\tbreak;\n')
            new_lines.append('\t\tdefault: // NODE, NODESPECIAL, NODEFWCHECKUDP, FINDBUDDY\n')
            new_lines.append('\t\t\tbreak;\n')
            new_lines.append('\t}\n')
            new_lines.append('\n')
            new_lines.append('\tif (m_nodeSpecialSearchRequester != NULL) {\n')
            new_lines.append('\t\t// inform requester that our search failed\n')
            new_lines.append('\t\tm_nodeSpecialSearchRequester->KadSearchIPByNodeIDResult(KCSR_NOTFOUND, 0, 0);\n')
            new_lines.append('\t}\n')
            new_lines.append('\n')
            new_lines.append('\t// Check if a source search is currently being done.\n')
            new_lines.append('\tCPartFile* temp = theApp->downloadqueue->GetFileByKadFileSearchID(GetSearchID());\n')
            new_lines.append('\t// Reset the searchID if a source search is currently being done.\n')
            new_lines.append('\tif (temp) {\n')
            new_lines.append('\t\ttemp->SetKadFileSearchID(0);\n')
            new_lines.append('\t}\n')
            new_lines.append('\n')
            new_lines.append('\t// Decrease the use count for any contacts that are in our contact list.\n')
            new_lines.append('\tfor (ContactMap::iterator it = m_inUse.begin(); it != m_inUse.end(); ++it) {\n')
            new_lines.append('\t\tit->second->DecUse();\n')
            new_lines.append('\t}\n')
            new_lines.append('\n')
            new_lines.append('\t// Delete any temp contacts...\n')
            new_lines.append('\tfor (ContactList::const_iterator it = m_delete.begin(); it != m_delete.end(); ++it) {\n')
            new_lines.append('\t\tif (!(*it)->InUse()) {\n')
            new_lines.append('\t\t\tdelete *it;\n')
            new_lines.append('\t\t}\n')
            new_lines.append('\t}\n')
            new_lines.append('\n')
            new_lines.append('\t// Clear all contact maps and lists to prevent double deletion\n')
            new_lines.append('\tm_possible.clear();\n')
            new_lines.append('\tm_tried.clear();\n')
            new_lines.append('\tm_best.clear();\n')
            new_lines.append('\tm_inUse.clear();\n')
            new_lines.append('\tm_delete.clear();\n')
            new_lines.append('\tm_deleteSet.clear();\n')
            new_lines.append('\n')
            new_lines.append('\t// Free search terms data if allocated\n')
            new_lines.append('\tif (m_searchTermsData) {\n')
            new_lines.append('\t\tdelete[] m_searchTermsData;\n')
            new_lines.append('\t\tm_searchTermsData = NULL;\n')
            new_lines.append('\t}\n')
            new_lines.append(line)  # Add the closing brace
        # Skip all other lines in the old destructor
    else:
        new_lines.append(line)

# Write the file back
with open('src/kademlia/kademlia/Search.cpp', 'w') as f:
    f.writelines(new_lines)

print("Destructor updated successfully")
