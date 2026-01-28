
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

#include "KadSearchController.h"
#include "../SearchList.h"
#include "../amule.h"
#include <wx/utils.h>

namespace search {

KadSearchController::KadSearchController(CSearchList* searchList)
    : SearchControllerBase(searchList)
    , m_maxNodesToQuery(DEFAULT_MAX_NODES)
    , m_nodesContacted(0)
{
}

KadSearchController::~KadSearchController()
{
}

void KadSearchController::startSearch(const SearchParams& params)
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

    // Step 4: Convert to old parameter format
    CSearchList::CSearchParams oldParams;
    oldParams.searchString = params.searchString;
    oldParams.strKeyword = params.strKeyword;
    oldParams.typeText = params.typeText;
    oldParams.extension = params.extension;
    oldParams.minSize = params.minSize;
    oldParams.maxSize = params.maxSize;
    oldParams.availability = params.availability;

    // Step 5: Start Kad search
    // Use 0xffffffff (UINT32_MAX) to indicate a new Kad search
    uint32 searchId = 0xffffffff;
    wxString error = m_searchList->StartNewSearch(&searchId, KadSearch, oldParams);

    // Step 6: Handle result
    if (error.IsEmpty()) {
	updateSearchState(params, searchId, SearchState::Searching);
	notifySearchStarted();
    } else {
	handleSearchError(error);
    }
}

void KadSearchController::stopSearch()
{
    // Step 1: Validate prerequisites
    if (!m_searchList) {
	return;
    }

    // Step 2: Stop the search
    m_searchList->StopSearch(false); // Stop Kad search

    // Step 3: Update state
    m_model->setSearchState(SearchState::Idle);
    resetSearchState();

    // Step 4: Notify completion
    notifySearchCompleted();
}

void KadSearchController::requestMoreResults()
{
    // Kad searches don't support "more results" in the traditional sense
    // as they are keyword-based and query the entire network
    handleSearchError(_("Kad searches query the entire network and don't support requesting more results"));
}

void KadSearchController::setMaxNodesToQuery(int maxNodes)
{
    m_maxNodesToQuery = maxNodes;
}

int KadSearchController::getMaxNodesToQuery() const
{
    return m_maxNodesToQuery;
}

void KadSearchController::setRetryCount(int retryCount)
{
    m_retryCount = retryCount;
}

int KadSearchController::getRetryCount() const
{
    return m_retryCount;
}

bool KadSearchController::validateConfiguration() const
{
    if (!SearchControllerBase::validateConfiguration()) {
	return false;
    }

    if (m_maxNodesToQuery <= 0) {
	return false;
    }

    return true;
}

void KadSearchController::updateProgress()
{
    if (!m_searchList) {
	return;
    }

    ProgressInfo info;

    // Calculate percentage based on nodes contacted vs max
    if (m_maxNodesToQuery > 0) {
	info.percentage = (m_nodesContacted * 100) / m_maxNodesToQuery;
    }

    info.serversContacted = 0; // Not applicable for Kad
    info.resultsReceived = getResultCount();

    // Set status based on state
    switch (getState()) {
	case SearchState::Searching:
	    info.currentStatus = _("Searching Kad network...");
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

void KadSearchController::initializeProgress()
{
    m_nodesContacted = 0;
    updateProgress();
}

bool KadSearchController::validatePrerequisites()
{
    if (!SearchControllerBase::validatePrerequisites()) {
	return false;
    }

    if (!isValidKadNetwork()) {
	handleSearchError(_("Kad network not available"));
	return false;
    }

    return true;
}

bool KadSearchController::isValidKadNetwork() const
{
    if (!theApp) {
	return false;
    }

    // Check if Kad is running
    return theApp->IsKadRunning();
}

} // namespace search
