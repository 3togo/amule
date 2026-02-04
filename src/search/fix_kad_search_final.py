# Fix KadSearchController.cpp
with open("KadSearchController.cpp", "r") as f:
    lines = f.readlines()

# Find the start and end of the problematic section
start_idx = None
end_idx = None

for i, line in enumerate(lines):
    if "// Generate search ID" in line and start_idx is None:
        start_idx = i
    if "handleSearchError(searchId, error);" in line and start_idx is not None and end_idx is None:
        if i > start_idx + 20:
            end_idx = i + 1
            break

if start_idx is not None and end_idx is not None:
    replacement = [
        "\t// Send packet to Kad network\n",
        "\tif (theApp && Kademlia::CKademlia::IsRunning()) {\n",
        "\t    // Use legacy Kad search implementation\n",
        "\t    try {\n",
        "\t\t// Let Kad search manager generate the search ID\n",
        "\t\tKademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(\n",
        "\t\t    params.strKeyword,\n",
        "\t\t    packetSize,\n",
        "\t\t    packetData,\n",
        "\t\t    0  // Let Kad search manager generate the search ID\n",
        "\t\t);\n",
        "\n",
        "\t\t// Get the actual search ID from Kad search manager\n",
        "\t\tuint32_t searchId = search->GetSearchID();\n",
        "\n",
        "\t\t// Store search ID and state\n",
        "\t\tm_model->setSearchParams(params);\n",
        "\t\tm_model->setSearchId(searchId);\n",
        "\t\tm_model->setSearchState(SearchState::Searching);\n",
        "\n",
        "\t\t// Register with SearchResultRouter for result routing\n",
        "\t\tSearchResultRouter::Instance().RegisterController(searchId, this);\n",
        "\n",
        "\t\t// Set the current search ID in SearchList after registration\n",
        "\t\tif (theApp->searchlist) {\n",
        "\t\t    theApp->searchlist->SetCurrentSearch(searchId);\n",
        "\t\t}\n",
        "\n",
        "\t\tnotifySearchStarted(searchId);\n",
        "\t    } catch (const wxString& what) {\n",
        "\t\terror = wxString::Format(_("Failed to start Kad search: %s"), what.c_str());\n",
        "\t\thandleSearchError(0, error);\n",
        "\t    }\n",
        "\n",
        "\t    // Clean up packet data\n",
        "\t    delete[] packetData;\n",
        "\t} else {\n",
        "\t    delete[] packetData;\n",
        "\t    error = _("Kad network not available");\n",
        "\t    handleSearchError(0, error);\n",
        "\t}\n"
    ]

    new_lines = lines[:start_idx] + replacement + lines[end_idx:]

    with open("KadSearchController.cpp", "w") as f:
        f.writelines(new_lines)

    print("File fixed successfully!")
else:
    print("Could not find the target code section!")
