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
#include "UnifiedSearchManager.h"
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
	wxMutexLocker lock(m_controllersMutex);
	m_controllers[searchId] = controller;
	SEARCH_DEBUG( 
		CFormat(wxT("Registered controller for search ID %u")) % searchId);
}

void SearchResultRouter::UnregisterController(uint32_t searchId)
{
	wxMutexLocker lock(m_controllersMutex);
	ControllerMap::iterator it = m_controllers.find(searchId);
	if (it != m_controllers.end()) {
		m_controllers.erase(it);
		SEARCH_DEBUG( 
			CFormat(wxT("Unregistered controller for search ID %u")) % searchId);
	}
}

bool SearchResultRouter::RouteResult(uint32_t searchId, CSearchFile* result)
{
	SearchController* controller = nullptr;
	{
		wxMutexLocker lock(m_controllersMutex);
		ControllerMap::iterator it = m_controllers.find(searchId);
		if (it != m_controllers.end()) {
			controller = it->second;
		}
	}

	if (controller) {
		SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(controller);
		if (handler) {
			handler->handleResult(searchId, result);

			SEARCH_DEBUG( 
				CFormat(wxT("Routed result for search ID %u")) % searchId);
			return true;
		}
	}

	SEARCH_DEBUG( 
		CFormat(wxT("No controller registered for search ID %u, adding to SearchList")) % searchId);

	if (theApp && theApp->searchlist) {
		result->SetSearchID(searchId);
		UnifiedSearchManager::Instance().addToList(result, false);
		return true;
	}
	delete result;
	return false;
}

size_t SearchResultRouter::RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
	SearchController* controller = nullptr;
	{
		wxMutexLocker lock(m_controllersMutex);
		ControllerMap::iterator it = m_controllers.find(searchId);
		if (it != m_controllers.end()) {
			controller = it->second;
		}
	}

	if (controller) {
		SearchResultHandler* handler = dynamic_cast<SearchResultHandler*>(controller);
		if (handler) {
			handler->handleResults(searchId, results);

			SEARCH_DEBUG( 
				CFormat(wxT("Routing %zu results for search ID %u")) % results.size() % searchId);

			return results.size();
		}
	}

	SEARCH_DEBUG( 
		CFormat(wxT("No controller registered for search ID %u, adding %zu results to SearchList")) 
		% searchId % results.size());

	if (theApp && theApp->searchlist) {
		for (CSearchFile* result : results) {
			result->SetSearchID(searchId);
			UnifiedSearchManager::Instance().addToList(result, false);
		}
		return results.size();
	}

	for (CSearchFile* result : results) {
		delete result;
	}

	return 0;
}

} // namespace search
