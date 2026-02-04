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

#include "SearchMoreHelper.h"
#include "SearchList.h"
#include "SearchListCtrl.h"
#include "SearchStateManager.h"
#include "amule.h"
#include "search/SearchController.h"
#include <wx/msgdlg.h>

namespace search {

bool SearchMoreHelper::RequestMoreResults(
    uint32_t searchId, CSearchListCtrl *listCtrl,
    std::map<uint32_t, std::unique_ptr<SearchController>> &searchControllers,
    SearchStateManager &stateManager) {

  if (!listCtrl) {
    return false;
  }

  // Get search type from state manager
  wxString searchType = stateManager.GetSearchType(searchId);

  // Kad searches don't support "More" button
  if (searchType == wxT("Kad")) {
    wxMessageBox(_("The 'More' button does not work for Kad searches."),
                 _("Search Information"), wxOK | wxICON_INFORMATION);
    return false;
  }

  // Only Local and Global searches are supported
  if (searchType != wxT("Local") && searchType != wxT("Global")) {
    wxMessageBox(
        _("Unknown search type. The 'More' button only works for Local "
          "and Global searches."),
        _("Search Error"), wxOK | wxICON_ERROR);
    return false;
  }

  // Get search parameters
  CSearchList::CSearchParams params;
  if (!GetSearchParams(searchId, stateManager, params)) {
    wxMessageBox(_("No search parameters available for this search."),
                 _("Search Error"), wxOK | wxICON_ERROR);
    return false;
  }

  // Validate parameters
  if (!ValidateSearchParams(params)) {
    wxMessageBox(_("Invalid search parameters."), _("Search Error"),
                 wxOK | wxICON_ERROR);
    return false;
  }

  // Get the search controller for this search
  auto it = searchControllers.find(searchId);
  if (it == searchControllers.end() || !it->second) {
    wxMessageBox(_("Search controller not found for this search."),
                 _("Search Error"), wxOK | wxICON_ERROR);
    return false;
  }

  // Request more results through the controller
  it->second->requestMoreResults();

  return true;
}

bool SearchMoreHelper::GetSearchParams(uint32_t searchId,
                                       SearchStateManager &stateManager,
                                       CSearchList::CSearchParams &params) {

  if (!stateManager.HasSearch(searchId)) {
    return false;
  }

  return stateManager.GetSearchParams(searchId, params);
}

bool SearchMoreHelper::ValidateSearchParams(const CSearchList::CSearchParams &params) {
  if (params.searchString.IsEmpty()) {
    return false;
  }

  // Additional validation can be added here as needed
  // For example, checking min/max size, availability, etc.

  return true;
}

} // namespace search
