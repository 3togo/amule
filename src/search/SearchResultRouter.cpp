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
#include <algorithm>
#include <memory>
#include <wx/app.h>
#include <wx/debug.h>
#include <wx/thread.h>

#include "SearchController.h"
#include "SearchResultHandler.h"
#include "../SearchList.h"
#include "../SearchFile.h"
#include "../amule.h"
#include "../Logger.h"

// Define a simple function pointer approach instead of complex classes to avoid linking issues
namespace search {

// Static instance for singleton
SearchResultRouter SearchResultRouter::m_instance;

SearchResultRouter& SearchResultRouter::Instance()
{
	return m_instance;
}

void SearchResultRouter::RegisterController(uint32_t searchId, SearchController* controller)
{
	wxMutexLocker lock(m_controllersMutex);
	m_controllers[searchId] = controller;
}

void SearchResultRouter::UnregisterController(uint32_t searchId)
{
	wxMutexLocker lock(m_controllersMutex);
	m_controllers.erase(searchId);
}

bool SearchResultRouter::RouteResult(uint32_t searchId, CSearchFile* result)
{
    wxMutexLocker lock(m_controllersMutex);
    ControllerMap::iterator it = m_controllers.find(searchId);
    if (it != m_controllers.end() && it->second) {
        // We have a controller registered for this search ID
        // Check if the controller implements SearchResultHandler
        SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(it->second);
        if (handler) {
            // Pass the result to the controller's handler
            std::vector<CSearchFile*> singleResult;
            singleResult.push_back(result);
            handler->handleResults(searchId, singleResult);
            return true;
        }
        // If not a SearchResultHandler, fall through to default handling
    }

    // No controller registered for this search or controller doesn't implement handler
    AddDebugLogLineN(logSearch, 
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

size_t SearchResultRouter::RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results) {
    size_t shown = 0;

    for (auto* result : results) {
        // Use the single result routing logic which properly handles controller lookup
        if (RouteResult(searchId, result)) {
            shown++; 
        }
        // RouteResult handles the deletion if needed
    }

    return shown;
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

SearchController* SearchResultRouter::GetActiveController(ModernSearchType type) {
    wxMutexLocker lock(m_controllersMutex);
    
    // Look for a controller registered for this type
    auto it = m_typeControllers.find(static_cast<::SearchType>(type));
    if (it != m_typeControllers.end()) {
        return it->second;
    }
    
    return nullptr;
}

} // namespace search