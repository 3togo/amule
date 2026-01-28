
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

KadSearchController::KadSearchController()
    : m_model(std::make_unique<SearchModel>())
    , m_searchList(nullptr)
    , m_maxNodesToQuery(500)
    , m_retryCount(3)
    , m_currentRetry(0)
    , m_nodesContacted(0)
{
    connectToSearchSystem();
}

KadSearchController::~KadSearchController()
{
    disconnectFromSearchSystem();
}

void KadSearchController::connectToSearchSystem()
{
    if (theApp) {
        m_searchList = theApp->searchlist;
    }
}

void KadSearchController::disconnectFromSearchSystem()
{
    m_searchList = nullptr;
}

void KadSearchController::startSearch(const SearchParams& params)
{
    if (!m_searchList) {
        handleSearchError(_("Kad search system not available"));
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

    // Start Kad search
    uint32 searchId = 0;
    wxString error = m_searchList->StartNewSearch(&searchId, KadSearch, oldParams);

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

void KadSearchController::stopSearch()
{
    if (m_searchList) {
        m_searchList->StopSearch(false); // Stop Kad search
        m_model->setSearchState(SearchState::Idle);
        notifySearchCompleted();
    }
}

void KadSearchController::requestMoreResults()
{
    // Kad searches don't support "more results" in the traditional sense
    // as they are keyword-based and query the entire network
    handleSearchError(_("Kad searches query the entire network and don't support requesting more results"));
}

SearchState KadSearchController::getState() const
{
    return m_model->getSearchState();
}

SearchParams KadSearchController::getSearchParams() const
{
    return m_model->getSearchParams();
}

long KadSearchController::getSearchId() const
{
    return m_model->getSearchId();
}

const std::vector<CSearchFile*>& KadSearchController::getResults() const
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

size_t KadSearchController::getResultCount() const
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

void KadSearchController::handleSearchError(const wxString& error)
{
    m_model->setSearchState(SearchState::Error);
    notifyError(error);
}

void KadSearchController::initializeProgress()
{
    m_nodesContacted = 0;
    updateProgress();
}

} // namespace search
