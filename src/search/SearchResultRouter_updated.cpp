size_t SearchResultRouter::RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    SEARCH_DEBUG_ROUTING(
        CFormat(wxT("RouteResults: searchId=%u, results=%zu, controllers=%zu"))
        % searchId % results.size() % m_controllers.size());

    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        // Get the controller as SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route all results to controller's handler
            handler->handleResults(searchId, results);

            SEARCH_DEBUG(
                CFormat(wxT("Routing %zu results for search ID %u")) % results.size() % searchId);

            return results.size();
        }
    }

    // No controller registered for this search ID, try fallback routing by search type
    SEARCH_DEBUG(
        CFormat(wxT("No controller registered for search ID %u, trying fallback routing")) % searchId);

    // Determine search type from SearchList based on active searches
    ::SearchType searchType = ::GlobalSearch; // Default to global
    if (theApp && theApp->searchlist) {
        wxMutexLocker lock(theApp->searchlist->m_searchMutex);
        // Find the most recent active ED2K search to determine type
        for (auto it = theApp->searchlist->m_activeSearches.rbegin(); it != theApp->searchlist->m_activeSearches.rend(); ++it) {
            if (it->first >= 0x80000001) {
                searchType = it->second;
                break;
            }
        }
    }

    // Try to find a controller registered for this search type
    TypeControllerMap::iterator typeIt = m_typeControllers.find(searchType);
    if (typeIt != m_typeControllers.end() && typeIt->second) {
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(typeIt->second);
        if (handler) {
            // Route results to the type-based controller
            handler->handleResults(searchId, results);
            SEARCH_DEBUG(
                CFormat(wxT("Routed %zu results to controller for search type %d"))
                % results.size() % (int)searchType);
            return results.size();
        }
    }

    // No controller found, add results to SearchList for display
    SEARCH_DEBUG(
        CFormat(wxT("No controller registered for search type %d, adding %zu results to SearchList"))
        % (int)searchType % results.size());

    // Add results to SearchList for display
    if (theApp && theApp->searchlist) {
        for (CSearchFile* result : results) {
            result->SetSearchID(searchId);
            theApp->searchlist->AddToList(result, false);
        }
        return results.size();
    }

    // Clean up all results since no one will handle them
    for (CSearchFile* result : results) {
        delete result;
    }

    return 0;
}
