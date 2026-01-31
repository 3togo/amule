
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
#include "ED2KSearchPacketBuilder.h"
#include "SearchPackageValidator.h"
#include "SearchResultRouter.h"
#include "../ServerList.h"
#include "../Server.h"
#include "../ServerConnect.h"
#include "../amule.h"
#include "../SearchList.h"
#include "../SearchFile.h"
#include "../Packet.h"
#include "../Statistics.h"
#include "../MemFile.h"
#include <protocol/Protocols.h>
#include <wx/utils.h>

namespace search {

ED2KSearchController::ED2KSearchController()
    : SearchControllerBase()
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
	notifySearchStarted(searchId);
    } else {
	handleSearchError(searchId, error);
    }
}

bool ED2KSearchController::validatePrerequisites()
{
    if (!SearchControllerBase::validatePrerequisites()) {
	return false;
    }

    if (!isValidServerList()) {
	uint32_t searchId = m_model->getSearchId();
	handleSearchError(searchId, _("No servers available for search"));
	return false;
    }

    return true;
}

std::pair<uint32_t, wxString> ED2KSearchController::executeSearch(const SearchParams& params)
{
    // Store search parameters
    m_model->setSearchParams(params);

    // Determine search type
    ::SearchType oldSearchType = (params.searchType == ModernSearchType::LocalSearch)
			      ? ::LocalSearch
			      : ::GlobalSearch;

    // Convert to old parameter format
    // Generate new search ID
    uint32_t searchId = 0;
    
    // Build search packet using ED2KSearchPacketBuilder
    ED2KSearchPacketBuilder packetBuilder;
    wxString error;
    
    try {
	// Determine search type
	bool isLocalSearch = (params.searchType == ModernSearchType::LocalSearch);
	
	// Build search packet
	uint8_t* packetData = nullptr;
	uint32_t packetSize = 0;
	bool supports64bit = theApp->serverconnect->GetCurrentServer() ?
		theApp->serverconnect->GetCurrentServer()->SupportsLargeFilesTCP() : false;
	bool success = packetBuilder.CreateSearchPacket(params, supports64bit, packetData, packetSize);
	
	if (!success || !packetData) {
	    error = wxT("Failed to create ED2K search packet");
	    return {0, error};
	}
	
	// Get search ID from model or generate new one
	if (m_model->getSearchId() == -1) {
	    searchId = GenerateSearchId();
	} else {
	    searchId = m_model->getSearchId();
	}
	
	// Send packet to server
	if (theApp && theApp->serverconnect) {
	    // Set the current search ID in SearchList before sending
	    if (theApp->searchlist) {
		theApp->searchlist->SetCurrentSearch(searchId);
	    }

	    theStats::AddUpOverheadServer(packetSize);
	    // Create a CMemFile from the raw data
	    CMemFile dataFile(packetData, packetSize);
	    CPacket* packet = new CPacket(dataFile, OP_EDONKEYPROT, OP_SEARCHREQUEST);
	    theApp->serverconnect->SendPacket(packet, isLocalSearch);
	    
	    // For global search, store packet for querying more servers
	    if (!isLocalSearch) {
		// Store packet for later use in querying more servers
		// TODO: Implement global search server queue
	    }
	    
	    // Clean up the packet data
	    delete[] packetData;
	} else {
	    delete[] packetData;
	    error = _("Not connected to eD2k server");
	    return {0, error};
	}
    } catch (const wxString& e) {
	error = wxString::Format(_("Failed to execute search: %s"), e.c_str());
	return {0, error};
    }

    // Store search ID and state
    m_model->setSearchId(searchId);
    m_model->setSearchState(SearchState::Searching);

    // Register with SearchResultRouter for result routing
    SearchResultRouter::Instance().RegisterController(searchId, this);

    // Initialize progress tracking
    initializeProgress();

    return {searchId, error};
}

void ED2KSearchController::handleSearchError(uint32_t searchId, const wxString& error)
{
    SearchControllerBase::handleSearchError(searchId, error);
}

void ED2KSearchController::stopSearch()
{
    // Unregister from SearchResultRouter
    long searchId = m_model->getSearchId();
    if (searchId != -1) {
	SearchResultRouter::Instance().UnregisterController(searchId);
    }
    
    // Clear results
    m_model->clearResults();
    
    // Use base class to handle common stop logic
    stopSearchBase();
}

void ED2KSearchController::requestMoreResults()
{
    // Check if another request is already in progress
    if (m_moreResultsInProgress) {
	uint32_t searchId = m_model->getSearchId();
	notifyMoreResults(searchId, false, _("Another 'More' request is already in progress"));
	return;
    }

    // Step 1: Validate search state
    wxString error;
    if (!validateSearchStateForMoreResults(error)) {
	uint32_t searchId = m_model->getSearchId();
	handleSearchError(searchId, error);
	notifyMoreResults(searchId, false, error);
	return;
    }

    // Step 2: Check retry limit
    if (!validateRetryLimit(error)) {
	uint32_t searchId = m_model->getSearchId();
	handleSearchError(searchId, error);
	notifyMoreResults(searchId, false, error);
	return;
    }

    // Mark as in progress
    m_moreResultsInProgress = true;
    m_moreResultsSearchId = m_model->getSearchId();

    // Step 3: Prepare for retry
    initializeProgress();
    m_currentRetry++;

    // Step 4: Execute search with same parameters
    auto [newSearchId, execError] = executeSearch(m_model->getSearchParams());

    // Step 5: Handle result
    if (execError.IsEmpty()) {
	m_model->setSearchId(newSearchId);
	m_model->setSearchState(SearchState::Retrying);
	notifySearchStarted(newSearchId);

	// Start timeout timer for async completion
	// The actual results will come back through the result handler
	// and we'll notify completion when either:
	// 1. Results are received
	// 2. Timeout occurs
    } else {
	uint32_t searchId = m_model->getSearchId();
	handleSearchError(searchId, execError);
	m_moreResultsInProgress = false;
	notifyMoreResults(searchId, false, execError);
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

    uint32_t searchId = m_model->getSearchId();
    notifyDetailedProgress(searchId, info);
    notifyProgress(searchId, info.percentage);
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

void ED2KSearchController::handleResults(uint32_t searchId, const std::vector<CSearchFile*>& results)
{
    // Call base implementation first
    SearchControllerBase::handleResults(searchId, results);

    // Check if we're in "more results" mode
    if (m_moreResultsInProgress && searchId == static_cast<uint32_t>(m_model->getSearchId())) {
	// Don't mark as complete yet - we want to continue receiving results
	// Just notify that we received some results
	if (!results.empty()) {
	    wxString message = wxString::Format(_("Received %zu additional result(s)"), results.size());
	    notifyMoreResults(searchId, true, message);
	}
    }
}

uint32_t ED2KSearchController::GenerateSearchId()
{
    // Generate a unique search ID
    // This is a simple implementation - could be improved with a global counter
    static uint32_t s_nextSearchId = 0;
    s_nextSearchId = (s_nextSearchId + 1) % 0xFFFFFFFE;
    if (s_nextSearchId == 0) {
	s_nextSearchId = 1;
    }
    return s_nextSearchId;
}

} // namespace search
