//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net /
// http://www.emule-project.net )
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

#ifndef SEARCHMOREHELPER_H
#define SEARCHMOREHELPER_H

#include <cstdint>
#include <map>
#include <memory>
#include <wx/string.h>

// Forward declarations
class CSearchListCtrl;
class SearchStateManager;

// Include SearchList.h to access CSearchParams
#include "SearchList.h"

namespace search {

// Forward declarations inside search namespace
class SearchController;

/**
 * Helper class for handling "More" button functionality
 *
 * This class encapsulates the logic for requesting more search results,
 * separating it from the UI code and making it easier to maintain.
 */
class SearchMoreHelper {
public:
  /**
   * Request more results for a search
   *
   * @param searchId The search ID to request more results for
   * @param listCtrl The search list control to display results in
   * @param searchControllers Map of search controllers
   * @param stateManager The search state manager
   * @return true if the request was initiated successfully, false otherwise
   */
  static bool RequestMoreResults(
      uint32_t searchId, CSearchListCtrl *listCtrl,
      std::map<uint32_t, std::unique_ptr<SearchController>> &searchControllers,
      SearchStateManager &stateManager);

private:
  /**
   * Get search parameters from the state manager
   *
   * @param searchId The search ID
   * @param stateManager The search state manager
   * @param params Output parameter for search parameters
   * @return true if parameters were retrieved successfully
   */
  static bool GetSearchParams(uint32_t searchId,
                              SearchStateManager &stateManager,
                              CSearchList::CSearchParams &params);

  /**
   * Validate search parameters
   *
   * @param params The search parameters to validate
   * @return true if parameters are valid
   */
  static bool ValidateSearchParams(const CSearchList::CSearchParams &params);
};

} // namespace search

#endif // SEARCHMOREHELPER_H
