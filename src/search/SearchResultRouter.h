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

#ifndef SEARCHRESULTROUTER_H
#define SEARCHRESULTROUTER_H

#include <map>
#include <memory>
#include <vector>
#include <wx/string.h>
#include "../SearchFile.h"

// Forward declarations
namespace search {
    class SearchController;
    class SearchModel;
    class SearchResultHandler;
}

#include "../SearchList.h"

namespace search {

/**
 * SearchResultRouter manages the routing of search results to appropriate controllers.
 * 
 * This class implements strict protocol-based routing with complete isolation between
 * different search types (LocalSearch, GlobalSearch, KadSearch).
 * 
 * Key features:
 * - Each search ID is routed only to its registered controller
 * - No fallback logic that uses "recent activity" or global state
 * - Invalid search IDs result in discarded results (no memory leaks)
 * - Thread-safe operation with proper mutex protection
 */
class SearchResultRouter
{
public:
    /**
     * Get the singleton instance of SearchResultRouter.
     */
    static SearchResultRouter& Instance();

    /**
     * Register a controller for a specific search ID.
     * 
     * @param searchId The unique search ID
     * @param controller The controller to register (takes ownership)
     */
    void RegisterController(uint32_t searchId, SearchController* controller);

    /**
     * Unregister a controller for a specific search ID.
     * 
     * @param searchId The search ID to unregister
     */
    void UnregisterController(uint32_t searchId);

    /**
     * Register a controller for a specific search type (fallback for unregistered search IDs).
     * 
     * @param searchType The search type (LocalSearch, GlobalSearch, KadSearch)
     * @param controller The controller to register (takes ownership)
     */
    void RegisterTypeController(SearchType searchType, SearchController* controller);

    /**
     * Unregister a controller for a specific search type.
     * 
     * @param searchType The search type to unregister
     */
    void UnregisterTypeController(SearchType searchType);

    /**
     * Route a single search result to the appropriate handler.
     * 
     * @param searchId The search ID associated with the result
     * @param result The search result to route (ownership transferred)
     * @return true if result was handled, false if it was deleted
     */
    bool RouteResult(uint32_t searchId, CSearchFile* result);

    /**
     * Route multiple search results to the appropriate handler.
     * 
     * @param searchId The search ID associated with the results
     * @param results The search results to route (ownership transferred)
     * @return number of results successfully handled
     */
    size_t RouteResults(uint32_t searchId, const std::vector<CSearchFile*>& results);

private:
    SearchResultRouter() = default;
    ~SearchResultRouter() = default;

    // Disable copy and assignment
    SearchResultRouter(const SearchResultRouter&) = delete;
    SearchResultRouter& operator=(const SearchResultRouter&) = delete;

    typedef std::map<uint32_t, SearchController*> ControllerMap;
    typedef std::map<SearchType, SearchController*> TypeControllerMap;

    ControllerMap m_controllers;
    TypeControllerMap m_typeControllers;
};

} // namespace search

#endif // SEARCHRESULTROUTER_H