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

#include "LegacySearchController.h"
#include "SearchModel.h"
#include "../SearchList.h" // For CSearchList
#include "../amule.h"      // For theApp
#include "../libs/common/Format.h"
#include <wx/utils.h>

namespace search {

LegacySearchController::LegacySearchController()
    : m_model(std::make_unique<SearchModel>())
    , m_searchList(nullptr)
{
    connectToExistingSystem();
}

LegacySearchController::~LegacySearchController()
{
    disconnectFromExistingSystem();
}

void LegacySearchController::connectToExistingSystem()
{
    // Connect to the existing search system
    if (theApp) {
        m_searchList = theApp->searchlist;
    }
}

void LegacySearchController::disconnectFromExistingSystem()
{
    m_searchList = nullptr;
}

void LegacySearchController::startSearch(const SearchParams& params)
{
    if (!m_searchList) {
        notifyError(_("Search system not available"));
        return;
    }
    
    // Convert new params to old format
    CSearchList::CSearchParams oldParams;
    oldParams.searchString = params.searchString;
    oldParams.strKeyword = params.strKeyword;
    oldParams.typeText = params.typeText;
    oldParams.extension = params.extension;
    oldParams.minSize = params.minSize;
    oldParams.maxSize = params.maxSize;
    oldParams.availability = params.availability;
    
    // Determine search type
    ModernSearchType searchType = params.searchType;
    SearchType oldSearchType;
    
    switch (searchType) {
        case ModernSearchType::LocalSearch:
            oldSearchType = LocalSearch;
            break;
        case ModernSearchType::GlobalSearch:
            oldSearchType = GlobalSearch;
            break;
        case ModernSearchType::KadSearch:
            oldSearchType = KadSearch;
            break;
        default:
            oldSearchType = GlobalSearch;
            break;
    }
    
    // Start the search using existing system
    uint32 searchId = 0;
    wxString error = m_searchList->StartNewSearch(&searchId, oldSearchType, oldParams);
    
    if (!error.IsEmpty()) {
        notifyError(error);
        return;
    }
    
    // Update model
    m_model->setSearchParams(params);
    m_model->setSearchId(searchId);
    m_model->setSearchState(SearchState::Searching);
    
    notifySearchStarted();
}

void LegacySearchController::stopSearch()
{
    if (m_searchList) {
        m_searchList->StopSearch();
        m_model->setSearchState(SearchState::Idle);
        notifySearchCompleted();
    }
}

void LegacySearchController::requestMoreResults()
{
    if (!m_searchList) {
        notifyError(_("Search system not available"));
        return;
    }
    
    // Get current search parameters
    SearchParams params = m_model->getSearchParams();
    
    // Only allow more results for eD2k searches (GlobalSearch)
    if (params.searchType != ModernSearchType::GlobalSearch) {
        notifyError(_("More results are only available for eD2k network searches"));
        return;
    }
    
    // Convert new params to old format
    CSearchList::CSearchParams oldParams;
    oldParams.searchString = params.searchString;
    oldParams.strKeyword = params.strKeyword;
    oldParams.typeText = params.typeText;
    oldParams.extension = params.extension;
    oldParams.minSize = params.minSize;
    oldParams.maxSize = params.maxSize;
    oldParams.availability = params.availability;
    
    // Generate a new search ID instead of reusing the old one
    uint32 newSearchId = 0;
    wxString error = m_searchList->StartNewSearch(&newSearchId, GlobalSearch, oldParams);
    
    if (!error.IsEmpty()) {
        notifyError(error);
        return;
    }
    
    // Update model with new search ID and state
    m_model->setSearchId(newSearchId);
    m_model->setSearchState(SearchState::Searching);
    m_model->setSearchParams(params);
    
    notifySearchStarted();
}

SearchState LegacySearchController::getState() const
{
    return m_model->getSearchState();
}

SearchParams LegacySearchController::getSearchParams() const
{
    return m_model->getSearchParams();
}

long LegacySearchController::getSearchId() const
{
    return m_model->getSearchId();
}

const std::vector<CSearchFile*>& LegacySearchController::getResults() const
{
    // TODO: Implement result retrieval from existing system
    static std::vector<CSearchFile*> emptyResults;
    return emptyResults;
}

size_t LegacySearchController::getResultCount() const
{
    // TODO: Implement result count retrieval from existing system
    return 0;
}

} // namespace search
