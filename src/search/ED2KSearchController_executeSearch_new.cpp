std::pair<uint32_t, wxString> ED2KSearchController::executeSearch(const SearchParams& params)
{
    AddDebugLogLineN(logSearch, "ED2KSearchController: executeSearch called");
    
    // Determine search type
    ::SearchType oldSearchType = SearchTypeConverter::toLegacy(params.searchType);
    AddDebugLogLineN(logSearch, "ED2KSearchController: oldSearchType=%d", (int)oldSearchType);
    
    // Convert to old parameter format
    uint32_t searchId = 0;
    
    // Build search packet using ED2KSearchPacketBuilder
    ED2KSearchPacketBuilder packetBuilder;
    wxString error;
    
    try {
        // Determine search type
        bool isLocalSearch = SearchTypeConverter::isLocalSearch(params.searchType);
        AddDebugLogLineN(logSearch, "ED2KSearchController: isLocalSearch=%d", isLocalSearch);
        
        bool supports64bit = thePrefs->GetED2KSupports64Bit();
        AddDebugLogLineN(logSearch, "ED2KSearchController: supports64bit=%d", supports64bit);
        
        // Get search ID from model or generate new one
        // IMPORTANT: When requesting more results, we MUST use the existing search ID
        // to ensure results are routed to the correct search tab
        if (m_model->getSearchId() == -1) {
            searchId = GenerateSearchId();
            SEARCH_DEBUG_CONTROLLER(
                CFormat("ED2KSearchController: Generated new search ID %u for %s search")
                % searchId % (isLocalSearch ? "local" : "global"));
        } else {
            // Use the existing search ID from the model
            // This is critical for "more results" functionality
            searchId = m_model->getSearchId();
            SEARCH_DEBUG_CONTROLLER(
                CFormat("ED2KSearchController: Using existing search ID %u for %s search (more results)")
                % searchId % (isLocalSearch ? "local" : "global"));
        }
        
        auto [packet, packetSize] = packetBuilder.buildSearchPacket(params, searchId);
        bool success = (packet != nullptr);
        AddDebugLogLineN(logSearch, "ED2KSearchController: CreateSearchPacket success=%d, packetSize=%u", success, packetSize);
        
        if (!success) {
            AddDebugLogLineN(logSearch, "ED2KSearchController: Failed to create ED2K search packet");
            error = "Failed to create ED2K search packet";
            return std::make_pair(0, error);
        }
        
        AddDebugLogLineN(logSearch, "ED2KSearchController: searchId=%u", searchId);
        
        // Set up early registration in SearchList to ensure proper result routing
        // --- EARLY REGISTRATION ---
        if (theApp && theApp->searchlist) {
            // Store search parameters in SearchList for later use
            CSearchList::CSearchParams oldParams;
            oldParams.searchString = params.searchString;
            oldParams.strKeyword = params.strKeyword;
            oldParams.typeText = params.typeText;
            oldParams.extension = params.extension;
            oldParams.minSize = params.minSize;
            oldParams.maxSize = params.maxSize;
            oldParams.availability = params.availability;
            oldParams.searchType = isLocalSearch ? LocalSearch : GlobalSearch;

            // Set the current search ID in SearchList
            // This is used by ProcessSearchAnswer to route results
            theApp->searchlist->SetCurrentID(searchId);
            
            // Send packet to server
            if (theApp->serverconnect->IsConnected()) {
                AddDebugLogLineN(logSearch, "ED2KSearchController: Sending packet to server, isLocalSearch=%d", isLocalSearch);
                
                // Send the search packet
                theApp->serverconnect->SendUDPPacket(packet, packetSize, 
                    isLocalSearch ? OP_GLOBSEARCHREQ : OP_GLOBSEARCHREQ);
            } else {
                AddDebugLogLineN(logSearch, "ED2KSearchController: Not connected to eD2k server");
                error = "Not connected to eD2k server";
                return std::make_pair(0, error);
            }
        } else {
            error = "Application not initialized properly";
            return std::make_pair(0, error);
        }
        
        // Return the search ID and empty error string on success
        return std::make_pair(searchId, wxString());
        
    } catch (const std::exception& e) {
        AddDebugLogLineN(logSearch, "ED2KSearchController: Exception: %s", e.what());
        error = wxString(e.what());
        return std::make_pair(0, error);
    }
    
    // Notify search completion
    notifySearchCompleted(searchId);
    AddDebugLogLineN(logSearch, "ED2KSearchController: Search completed, searchId=%u", searchId));

}
