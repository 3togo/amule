//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "SearchResultRouter.h"
#include "SearchController.h"
#include "SearchResultHandler.h"
#include "SearchLogging.h"
#include "../Logger.h"
#include <common/Format.h>
#include "../amule.h"
#include "../SearchList.h"

namespace search {


SearchResultRouter& SearchResultRouter::Instance()
{
    static SearchResultRouter instance;
    return instance;
}

void SearchResultRouter::RegisterController(uint32_t searchId, SearchController* controller)
{
    m_controllers[searchId] = controller;
    SEARCH_DEBUG( 
        CFormat(wxT("Registered controller for search ID %u")) % searchId);
}

void SearchResultRouter::UnregisterController(uint32_t searchId)
{
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        m_controllers.erase(it);
        SEARCH_DEBUG( 
            CFormat(wxT("Unregistered controller for search ID %u")) % searchId);
    }
}

SearchController* SearchResultRouter::GetController(uint32_t searchId) const
{
    ControllerMap::const_iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        return it->second;
    }
    return nullptr;
}

void SearchResultRouter::RegisterTypeController(::SearchType searchType, SearchController* controller)
{
    m_typeControllers[searchType] = controller;
    SEARCH_DEBUG(
        CFormat(wxT("Registered type controller for search type %d")) % (int)searchType);
}

bool SearchResultRouter::RouteResult(uint32_t searchId, CSearchFile* result)
{
    if (!result) {
        return false;
    }

    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        // Get the controller as SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route single result to controller's handler using batch method
            std::vector<CSearchFile*> results;
            results.push_back(result);
            handler->handleResults(searchId, results);
            
            SEARCH_DEBUG( 
                CFormat(wxT("Routing single result for search ID %u")) % searchId);

            return true;
        }
    }

    // No controller registered for this search ID, add to SearchList for display
    SEARCH_DEBUG( 
        CFormat(wxT("No controller registered for search ID %u, adding result to SearchList")) % searchId);

    if (theApp && theApp->searchlist) {
        result->SetSearchID(searchId);
        theApp->searchlist->AddToList(result, false);
        return true;
    }
    // Clean up the result since no one will handle it
    delete result;
    return false;
}

size_t SearchResultRouter::RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    if (results.empty()) {
        return 0;
    }

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

    // No controller registered for this specific search ID
    // Check if this is a valid active search before falling back to SearchList
    bool isValidSearch = false;
    ::SearchType searchType = ::GlobalSearch;
    
    if (theApp && theApp->searchlist) {
        wxMutexLocker lock(theApp->searchlist->GetSearchMutex());
        const auto& activeSearches = theApp->searchlist->GetActiveSearches();
        auto searchIt = activeSearches.find(static_cast<long>(searchId));
        if (searchIt != activeSearches.end()) {
            isValidSearch = true;
            searchType = searchIt->second;
        }
    }

    if (!isValidSearch) {
        // Invalid search ID - no active search exists for this ID
        // This can happen with stale results or race conditions
        SEARCH_DEBUG(
            CFormat(wxT("Invalid search ID %u - no active search, discarding %zu results"))
            % searchId % results.size());
        
        // Clean up all results since no one will handle them
        for (CSearchFile* result : results) {
            delete result;
        }
        return 0;
    }

    // Valid search but no controller - add results to SearchList for display
    SEARCH_DEBUG(
        CFormat(wxT("No controller for valid search ID %u (type %d), adding %zu results to SearchList"))
        % searchId % (int)searchType % results.size());

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

} // namespace search
