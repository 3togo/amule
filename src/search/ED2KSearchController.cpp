
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

#include "ED2KSearchController.h"
#include "../SearchList.h"
#include "../amule.h"
#include <wx/utils.h>

namespace search {

ED2KSearchController::ED2KSearchController()
    : m_model(std::make_unique<SearchModel>())
    , m_searchList(nullptr)
    , m_maxServersToQuery(100)
    , m_retryCount(3)
    , m_currentRetry(0)
    , m_serversContacted(0)
    , m_resultsSinceLastUpdate(0)
{
    connectToSearchSystem();
}

ED2KSearchController::~ED2KSearchController()
{
    disconnectFromSearchSystem();
}

void ED2KSearchController::connectToSearchSystem()
{
    if (theApp) {
        m_searchList = theApp->searchlist;
    }
}

void ED2KSearchController::disconnectFromSearchSystem()
{
    m_searchList = nullptr;
}

void ED2KSearchController::startSearch(const SearchParams& params)
{
    if (!m_searchList) {
        handleSearchError(_("ED2K search system not available"));
        return;
    }

    // Validate parameters
    if (!params.isValid()) {
        handleSearchError(_("Invalid search parameters"));
        return;
    }

    // Initialize progress tracking
    initializeProgress();

    // Convert to old parameter format
    CSearchList::CSearchParams oldParams;
    oldParams.searchString = params.searchString;
    oldParams.strKeyword = params.strKeyword;
    oldParams.typeText = params.typeText;
    oldParams.extension = params.extension;
    oldParams.minSize = params.minSize;
    oldParams.maxSize = params.maxSize;
    oldParams.availability = params.availability;

    // Determine search type
    SearchType oldSearchType = (params.searchType == ModernSearchType::LocalSearch) 
                              ? LocalSearch 
                              : GlobalSearch;

    // Start the search
    uint32 searchId = 0;
    wxString error = m_searchList->StartNewSearch(&searchId, oldSearchType, oldParams);

    if (!error.IsEmpty()) {
        handleSearchError(error);
        return;
    }

    // Update model
    m_model->setSearchParams(params);
    m_model->setSearchId(searchId);
    m_model->setSearchState(SearchState::Searching);

    notifySearchStarted();
}

void ED2KSearchController::stopSearch()
{
    if (m_searchList) {
        m_searchList->StopSearch(true); // Stop global search
        m_model->setSearchState(SearchState::Idle);
        notifySearchCompleted();
    }
}

void ED2KSearchController::requestMoreResults()
{
    if (!m_searchList) {
        handleSearchError(_("ED2K search system not available"));
        return;
    }

    // Get current search parameters
    SearchParams params = m_model->getSearchParams();

    // Only allow more results for global searches
    if (params.searchType != ModernSearchType::GlobalSearch) {
        handleSearchError(_("More results are only available for global eD2k searches"));
        return;
    }

    // Check retry limit
    if (m_currentRetry >= m_retryCount) {
        handleSearchError(_("Maximum retry limit reached"));
        return;
    }

    // Initialize progress tracking
    initializeProgress();
    m_currentRetry++;

    // Convert to old parameter format
    CSearchList::CSearchParams oldParams;
    oldParams.searchString = params.searchString;
    oldParams.strKeyword = params.strKeyword;
    oldParams.typeText = params.typeText;
    oldParams.extension = params.extension;
    oldParams.minSize = params.minSize;
    oldParams.maxSize = params.maxSize;
    oldParams.availability = params.availability;

    // Start new search with same parameters
    uint32 newSearchId = 0;
    wxString error = m_searchList->StartNewSearch(&newSearchId, GlobalSearch, oldParams);

    if (!error.IsEmpty()) {
        handleSearchError(error);
        return;
    }

    // Update model
    m_model->setSearchId(newSearchId);
    m_model->setSearchState(SearchState::Retrying);

    notifySearchStarted();
}

SearchState ED2KSearchController::getState() const
{
    return m_model->getSearchState();
}

SearchParams ED2KSearchController::getSearchParams() const
{
    return m_model->getSearchParams();
}

long ED2KSearchController::getSearchId() const
{
    return m_model->getSearchId();
}

const std::vector<CSearchFile*>& ED2KSearchController::getResults() const
{
    static std::vector<CSearchFile*> results;
    results.clear();

    if (m_searchList) {
        long searchId = m_model->getSearchId();
        if (searchId != -1) {
            const CSearchResultList& searchResults = m_searchList->GetSearchResults(searchId);
            results = searchResults;
            m_model->cacheResults(results);
        }
    }

    return results;
}

size_t ED2KSearchController::getResultCount() const
{
    if (m_searchList) {
        long searchId = m_model->getSearchId();
        if (searchId != -1) {
            const CSearchResultList& searchResults = m_searchList->GetSearchResults(searchId);
            return searchResults.size();
        }
    }

    return 0;
}

void ED2KSearchController::setMaxServersToQuery(int maxServers)
{
    m_maxServersToQuery = maxServers;
}

int ED2KSearchController::getMaxServersToQuery() const
{
    return m_maxServersToQuery;
}

void ED2KSearchController::setRetryCount(int retryCount)
{
    m_retryCount = retryCount;
}

int ED2KSearchController::getRetryCount() const
{
    return m_retryCount;
}

void ED2KSearchController::updateProgress()
{
    if (!m_searchList) {
        return;
    }

    ProgressInfo info;

    // Calculate percentage based on servers contacted vs max
    if (m_maxServersToQuery > 0) {
        info.percentage = (m_serversContacted * 100) / m_maxServersToQuery;
    }

    info.serversContacted = m_serversContacted;
    info.resultsReceived = getResultCount();

    // Set status based on state
    switch (getState()) {
        case SearchState::Searching:
            info.currentStatus = _("Searching eD2k network...");
            break;
        case SearchState::Retrying:
            info.currentStatus = wxString::Format(_("Retrying search (%d/%d)..."), 
                                                m_currentRetry, m_retryCount);
            break;
        case SearchState::Completed:
            info.currentStatus = _("Search completed");
            break;
        default:
            info.currentStatus = _("Idle");
            break;
    }

    notifyDetailedProgress(info);
    notifyProgress(info.percentage);
}

void ED2KSearchController::handleSearchError(const wxString& error)
{
    m_model->setSearchState(SearchState::Error);
    notifyError(error);
}

void ED2KSearchController::initializeProgress()
{
    m_serversContacted = 0;
    m_resultsSinceLastUpdate = 0;
    updateProgress();
}

} // namespace search
