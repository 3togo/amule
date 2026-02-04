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
    SEARCH_DEBUG(
        CFormat(wxT("DEBUG: RouteResult called - SearchID=%u, FileName='%s'"))
            % searchId % result->GetFileName().GetPrintable());

    // First try to route by specific search ID
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

    // If no specific controller found, try routing by search type
    SEARCH_DEBUG(
        CFormat(wxT("No specific controller for SearchID=%u, trying fallback routing")) % searchId);

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
            // Route result to the type-based controller using the correct client search ID
            handler->handleResult(clientSearchId, result);
            SEARCH_DEBUG(
                CFormat(wxT("Routed result to controller for search type %d (clientSearchId=%u, serverSearchId=%u)"))
                % (int)searchType % clientSearchId % searchId);
            return true;
        }
    }

    // No controller found, add result to SearchList for display
    SEARCH_DEBUG(
        CFormat(wxT("No controller registered for search type %d, adding result to SearchList"))
        % (int)searchType);

    // Add result to SearchList for display
    if (theApp && theApp->searchlist) {
        // Use the client search ID instead of the server ID to ensure UI consistency
        result->SetSearchID(clientSearchId);
        theApp->searchlist->AddToList(result, false);
        return true;
    }

    // Clean up result since no one will handle it
    delete result;

    return false;
}


size_t SearchResultRouter::RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    size_t routedCount = 0;
    
    // First try to route by specific search ID
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        // Get the controller as SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route all results to controller's handler
            handler->handleResults(searchId, results);
            routedCount = results.size();
            
            SEARCH_DEBUG( 
                CFormat(wxT("Routed %u results for search ID %u")) % (unsigned int)routedCount % searchId);
            return routedCount;
        }
    }

    // If no specific controller found, try routing by search type
    SEARCH_DEBUG(
        CFormat(wxT("No specific controller for SearchID=%u, trying fallback routing for %u results")) 
        % searchId % (unsigned int)results.size());
        
    // Determine search type from active searches
    SearchType searchType = GlobalSearch;
    {
        wxMutexLocker lock(theApp->searchlist->GetSearchMutex());
        const std::map<long, SearchType>& activeSearches = theApp->searchlist->GetActiveSearches();
        auto typeIt = activeSearches.find(searchId);
        if (typeIt != activeSearches.end()) {
            searchType = typeIt->second;
        }
    }

    // Try to find a controller registered for this search type
    TypeControllerMap::iterator typeIt = m_typeControllers.find(searchType);
    if (typeIt != m_typeControllers.end() && typeIt->second) {
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(typeIt->second);
        if (handler) {
            // Route all results to the type-based controller
            handler->handleResults(searchId, results);
            routedCount = results.size();
            
            SEARCH_DEBUG(
                CFormat(wxT("Routed %u results by search type %d for search ID %u")) 
                % (unsigned int)routedCount % (int)searchType % searchId);
            return routedCount;
        }
    }

    // No controller found - add results directly to SearchList
    SEARCH_DEBUG(
        CFormat(wxT("No controller found for SearchID=%u, adding %u results directly to SearchList")) 
        % searchId % (unsigned int)results.size());
        
    for (CSearchFile* result : results) {
        if (theApp && theApp->searchlist) {
            CSearchFile* resultCopy = new CSearchFile(*result);
            resultCopy->SetSearchID(searchId);
            theApp->searchlist->AddToList(resultCopy, false);
            routedCount++;
        }
    }
    
    return routedCount;
}


} // namespace search
