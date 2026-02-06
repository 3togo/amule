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
#include "../OtherFunctions.h" // For CFormat
#include "../SearchFile.h"
#include "../SearchList.h"
#include <wx/string.h>
#include <wx/utils.h>

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
    if (!controller) {
        SEARCH_CRITICAL(
            CFormat(wxT("Attempted to register null controller for SearchID=%u")) % searchId);
        return;
    }

    // Check if controller already registered for this ID
    if (m_controllers.find(searchId) != m_controllers.end()) {
        SEARCH_CRITICAL(
            CFormat(wxT("Controller already registered for SearchID=%u, overwriting")) % searchId);
    }

    m_controllers[searchId] = controller;
    
    SEARCH_DEBUG(
        CFormat(wxT("Registered controller for SearchID=%u")) % searchId);
}

void SearchResultRouter::UnregisterController(uint32_t searchId)
{
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        SEARCH_DEBUG(
            CFormat(wxT("Unregistered controller for SearchID=%u")) % searchId);
        m_controllers.erase(it);
    } else {
        SEARCH_CRITICAL(
            CFormat(wxT("Attempted to unregister non-existent controller for SearchID=%u")) % searchId);
    }
}

bool SearchResultRouter::RouteResult(uint32_t searchId, CSearchFile* result)
{
    if (!result) {
        SEARCH_CRITICAL(wxT("RouteResult called with null result"));
        return false;
    }

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

    // No controller found - add result to SearchList for display (fallback behavior)
    SEARCH_DEBUG(
        CFormat(wxT("No controller registered for search ID %u, adding result to SearchList"))
        % searchId);

    // Add result to SearchList for display
    if (theApp && theApp->searchlist) {
        theApp->searchlist->AddToList(result, false);
        return true;
    }

    // Clean up result since no one will handle it
    delete result;

    return false;
}


size_t SearchResultRouter::RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    if (results.empty()) {
        SEARCH_DEBUG(wxT("RouteResults called with empty results vector"));
        return 0;
    }

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

    // No controller found - add results directly to SearchList (fallback behavior)
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