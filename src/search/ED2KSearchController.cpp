
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
#include "../ServerList.h"
#include "../amule.h"
#include <wx/utils.h>

namespace search {

ED2KSearchController::ED2KSearchController(CSearchList* searchList)
    : SearchControllerBase(searchList)
    , m_maxServersToQuery(DEFAULT_MAX_SERVERS)
    , m_serversContacted(0)
    , m_resultsSinceLastUpdate(0)
{
}

ED2KSearchController::~ED2KSearchController()
{
}

void ED2KSearchController::startSearch(const SearchParams& params)
{
    // Step 1: Validate prerequisites
    if (!validatePrerequisites()) {
	return;
    }

    // Step 2: Validate search parameters
    if (!validateSearchParams(params)) {
	return;
    }

    // Step 3: Prepare search
    initializeProgress();
    resetSearchState();

    // Step 4: Convert parameters and execute search
    auto [searchId, error] = executeSearch(params);

    // Step 5: Handle result
    if (error.IsEmpty()) {
	updateSearchState(params, searchId, SearchState::Searching);
	notifySearchStarted();
    } else {
	handleSearchError(error);
    }
}

bool ED2KSearchController::validatePrerequisites()
{
    if (!SearchControllerBase::validatePrerequisites()) {
	return false;
    }

    if (!isValidServerList()) {
	handleSearchError(_("No servers available for search"));
	return false;
    }

    return true;
}

std::pair<uint32_t, wxString> ED2KSearchController::executeSearch(const SearchParams& params)
{
    // Convert to old parameter format using base class method
    CSearchList::CSearchParams oldParams = convertParams(params);

    // Determine search type
    SearchType oldSearchType = (params.searchType == ModernSearchType::LocalSearch)
			      ? LocalSearch
			      : GlobalSearch;

    // Execute search
    uint32_t searchId = 0;
    wxString error = m_searchList->StartNewSearch(&searchId, oldSearchType, oldParams);

    return {searchId, error};
}

void ED2KSearchController::stopSearch()
{
    // Step 1: Validate prerequisites
    if (!m_searchList) {
	return;
    }

    // Step 2: Stop the search
    m_searchList->StopSearch(true);

    // Step 3: Use base class to handle common stop logic
    stopSearchBase();
}

void ED2KSearchController::requestMoreResults()
{
    // Step 1: Validate prerequisites
    if (!validatePrerequisites()) {
	return;
    }

    // Step 2: Validate search state
    wxString error;
    if (!validateSearchStateForMoreResults(error)) {
	handleSearchError(error);
	return;
    }

    // Step 3: Check retry limit
    if (!validateRetryLimit(error)) {
	handleSearchError(error);
	return;
    }

    // Step 4: Prepare for retry
    initializeProgress();
    m_currentRetry++;

    // Step 5: Execute search with same parameters
    auto [newSearchId, execError] = executeSearch(m_model->getSearchParams());

    // Step 6: Handle result
    if (execError.IsEmpty()) {
	m_model->setSearchId(newSearchId);
	m_model->setSearchState(SearchState::Retrying);
	notifySearchStarted();
    } else {
	handleSearchError(execError);
    }
}

bool ED2KSearchController::validateSearchStateForMoreResults(wxString& error) const
{
    SearchParams params = m_model->getSearchParams();

    if (params.searchType != ModernSearchType::GlobalSearch) {
	error = _("More results are only available for global eD2k searches");
	return false;
    }

    SearchState currentState = m_model->getSearchState();
    if (currentState == SearchState::Searching) {
	error = _("Cannot request more results while search is in progress");
	return false;
    }

    return true;
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

void ED2KSearchController::initializeProgress()
{
    m_serversContacted = 0;
    m_resultsSinceLastUpdate = 0;
    updateProgress();
}

bool ED2KSearchController::validateConfiguration() const
{
    if (!SearchControllerBase::validateConfiguration()) {
	return false;
    }

    if (m_maxServersToQuery <= 0) {
	return false;
    }

    return true;
}

bool ED2KSearchController::isValidServerList() const
{
    if (!theApp) {
	return false;
    }

    // Check if there are servers available
    CServerList* serverList = theApp->serverlist;
    if (!serverList) {
	return false;
    }

    return serverList->GetServerCount() > 0;
}

} // namespace search
