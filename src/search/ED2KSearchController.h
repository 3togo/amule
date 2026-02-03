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

#ifndef __ED2K_SEARCH_CONTROLLER_H__
#define __ED2K_SEARCH_CONTROLLER_H__

#include "SearchControllerBase.h"
#include <memory>

// Forward declarations
class CPacket;

namespace search {

/**
 * Controller for managing ED2K network searches
 */
class ED2KSearchController : public SearchControllerBase
{
public:
    /**
     * Constructor
     */
    ED2KSearchController();
    
    /**
     * Destructor
     */
    virtual ~ED2KSearchController();
    
    // Delete copy constructor and copy assignment operator
    ED2KSearchController(const ED2KSearchController&) = delete;
    ED2KSearchController& operator=(const ED2KSearchController&) = delete;

    // Implement SearchController interface
    virtual void startSearch(const SearchParams& params) override;
    virtual void stopSearch() override;
    virtual void requestMoreResults() override;
    virtual SearchState getState() const override { return isSearching() ? SearchState::Searching : SearchState::Idle; }

protected:
    // Check if searching is in progress
    bool isSearching() const;

private:
    bool m_isActive;                     ///< Whether search is currently active
    bool validateSearchParams(const SearchParams& params);  ///< Validate search parameters before starting search
};


} // namespace search

#endif // __ED2K_SEARCH_CONTROLLER_H__
