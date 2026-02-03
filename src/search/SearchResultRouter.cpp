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

SearchResultRouter::SearchResultRouter()
{
}

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

void SearchResultRouter::RegisterControllerByType(::SearchType type, SearchController* controller)
{
        wxMutexLocker lock(m_controllersMutex);
        m_typeControllers[type] = controller;
}

void SearchResultRouter::UnregisterControllerByType(::SearchType type, SearchController* controller)
{
        wxMutexLocker lock(m_controllersMutex);
        auto it = m_typeControllers.find(type);
        if (it != m_typeControllers.end() && it->second == controller) {
                m_typeControllers.erase(it);
        }
}

bool SearchResultRouter::RouteResult(uint32_t searchId, CSearchFile* result)
{
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        // Get the controller as SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route result to controller's handler
            handler->handleResult(searchId, result);

            SEARCH_DEBUG( 
                CFormat(wxT("Routed result for search ID %u")) % searchId);
            return true;
        }
    }

    // No controller registered for this search
    SEARCH_DEBUG( 
        CFormat(wxT("No controller registered for search ID %u, adding to SearchList")) % searchId);

    // Add result to SearchList for display
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

    // No controller registered for this search ID, trying fallback routing by search type
    SEARCH_DEBUG( 
        CFormat(wxT("No controller registered for search ID %u, trying fallback routing")) % searchId);

    // Determine search type from SearchList based on active searches
    // This is the fallback when no specific controller is registered
    ::SearchType searchType = ::GlobalSearch; // Default to global
    uint32_t clientSearchId = searchId; // Use the provided searchId by default
    
    if (theApp && theApp->searchlist) {
        wxMutexLocker lock(theApp->searchlist->GetSearchMutex());
        
        // First, try to find an exact match for the search ID
        const auto& activeSearches = theApp->searchlist->GetActiveSearches();
        auto exactIt = activeSearches.find(searchId);
        if (exactIt != activeSearches.end()) {
            // Found exact match, use that search type and ID
            searchType = exactIt->second;
            clientSearchId = searchId;
        } else {
            // No exact match, check for global searches first (TCP results can go to global searches)
            for (auto it = activeSearches.rbegin(); it != activeSearches.rend(); ++it) {
                if (it->first >= 0x80000001) { // Only consider ED2K searches
                    if (it->second == ::GlobalSearch) {
                        searchType = ::GlobalSearch;
                        clientSearchId = static_cast<uint32_t>(it->first);
                        break;
                    }
                }
            }
            
            // If no global search found, fall back to local searches
            if (searchType != ::GlobalSearch) {
                for (auto it = activeSearches.rbegin(); it != activeSearches.rend(); ++it) {
                    if (it->first >= 0x80000001) { // Only consider ED2K searches
                        if (it->second == ::LocalSearch) {
                            searchType = ::LocalSearch;
                            clientSearchId = static_cast<uint32_t>(it->first);
                            break;
                        }
                    }
                }
            }
        }
    }

    // Try to find a controller registered for this search type
    TypeControllerMap::iterator typeIt = m_typeControllers.find(searchType);
    if (typeIt != m_typeControllers.end() && typeIt->second) {
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(typeIt->second);
        if (handler) {
            // Route results to the type-based controller using the correct client search ID
            handler->handleResults(clientSearchId, results);
            SEARCH_DEBUG(
                CFormat(wxT("Routed %zu results to controller for search type %d (clientSearchId=%u, serverSearchId=%u)"))
                % results.size() % (int)searchType % clientSearchId % searchId);
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
            // Use the client search ID instead of the server ID to ensure UI consistency
            result->SetSearchID(clientSearchId);
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

size_t SearchResultRouter::RouteResultsByType(SearchType searchType, const std::vector<CSearchFile*>& results)
{
    if (results.empty()) {
        return 0;
    }

    SEARCH_DEBUG(
        CFormat(wxT("RouteResultsByType: searchType=%d, results=%zu")) 
        % (int)searchType % results.size());

    // Find type controller
    TypeControllerMap::iterator it = m_typeControllers.find(searchType);
    if (it != m_typeControllers.end() && it->second) {
        // Get the controller as SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route all results to controller's handler
            // Use a dummy search ID of 0 since type-based routing doesn't have a specific ID
            handler->handleResults(0, results);

            SEARCH_DEBUG(
                CFormat(wxT("RouteResultsByType: routed %zu results to type controller")) 
                % results.size());

            return results.size();
        }
    }

    // No type controller found - add results directly to SearchList
    SEARCH_DEBUG(
        CFormat(wxT("RouteResultsByType: no type controller for searchType=%d, adding to SearchList")) 
        % (int)searchType);

    if (theApp && theApp->searchlist) {
        for (CSearchFile* result : results) {
            if (result) {
                result->SetSearchID(0); // Set dummy ID
                theApp->searchlist->AddToList(result, false);
            }
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
