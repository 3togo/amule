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
#include "SearchControllerBase.h"
#include "SearchController.h"
#include "SearchResultHandler.h"
#include "SearchLogging.h"
#include "../Logger.h"
#include <common/Format.h>
#include "../amule.h"
#include "../OtherFunctions.h" // For CFormat
#include "../SearchFile.h"
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

void SearchResultRouter::RegisterController(uint32_t searchId, SearchControllerBase* controller)
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
        CFormat(wxT("Attempting to route result for SearchID=%u, File='%s'"))
            % searchId % result->GetFileName().GetPrintable());

    // Try to route by specific search ID only - NO FALLBACK LOGIC
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        // Get the controller as SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route result to controller's handler using original search ID
            handler->handleResult(searchId, result);

            SEARCH_DEBUG( 
                CFormat(wxT("Successfully routed result for search ID %u")) % searchId);
            return true;
        }
    }

    // No controller found - log detailed information about the failure
    {
        wxString activeControllersInfo = wxT("Active controllers: ");
        for (const auto& controllerPair : m_controllers) {
            activeControllersInfo += CFormat(wxT("[ID=%u] ")) % controllerPair.first;
        }
        
        SEARCH_CRITICAL(
            CFormat(wxT("UNROUTEABLE RESULT - SearchID=%u, File='%s', Reason='No controller registered'. %s"))
            % searchId % result->GetFileName().GetPrintable() % activeControllersInfo);
    }

    // Track dropped results for diagnostics
    static std::atomic<uint64_t> s_droppedSingleResults{0};
    s_droppedSingleResults.fetch_add(1);
    
    // According to specification: "宁可丢弃无法明确路由的结果，也不进行猜测性处理"
    // So we drop the result instead of trying to add it to SearchList
    SEARCH_CRITICAL(
        CFormat(wxT("DROPPED RESULT - SearchID=%u, File='%s', No controller available"))
        % searchId % result->GetFileName().GetPrintable());
    
    return false;
}


size_t SearchResultRouter::RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    if (results.empty()) {
        SEARCH_DEBUG(wxT("RouteResults called with empty results vector"));
        return 0;
    }

    SEARCH_DEBUG(
        CFormat(wxT("Attempting to route %u results for SearchID=%u"))
            % (unsigned int)results.size() % searchId);

    size_t routedCount = 0;

    // Try to route by specific search ID only - NO FALLBACK LOGIC
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Route all results to the controller using original search ID
            handler->handleResults(searchId, results);
            routedCount = results.size();
            
            SEARCH_DEBUG(
                CFormat(wxT("Successfully routed %u results for search ID %u")) 
                % (unsigned int)routedCount % searchId);
            return routedCount;
        }
    }

    // No controller found - log detailed information about the failure
    {
        wxString activeControllersInfo = wxT("Active controllers: ");
        for (const auto& controllerPair : m_controllers) {
            activeControllersInfo += CFormat(wxT("[ID=%u] ")) % controllerPair.first;
        }
        
        SEARCH_CRITICAL(
            CFormat(wxT("UNROUTEABLE RESULTS - SearchID=%u, Count=%u, Reason='No controller registered'. %s"))
            % searchId % (unsigned int)results.size() % activeControllersInfo);
    }

    // Track dropped results for diagnostics
    static std::atomic<uint64_t> s_droppedBatchResults{0};
    s_droppedBatchResults.fetch_add(results.size());
    
    // According to specification: "宁可丢弃无法明确路由的结果，也不进行猜测性处理"
    // So we drop all results instead of trying to add them to SearchList
    SEARCH_CRITICAL(
        CFormat(wxT("DROPPED RESULTS - SearchID=%u, Count=%u, No controller available"))
        % searchId % (unsigned int)results.size());
    
    return 0;
}


} // namespace search