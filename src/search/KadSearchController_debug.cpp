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
#include "KadSearchPacketBuilder.h"
#include "SearchPackageValidator.h"
#include "SearchResultRouter.h"
#include "../amule.h"
#include "../SearchFile.h"
#include "../SearchList.h"
#include "../Logger.h"
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/SearchManager.h"
#include "../kademlia/kademlia/Search.h"

namespace search {

KadSearchController::KadSearchController()
    : SearchControllerBase()
    , m_maxNodesToQuery(DEFAULT_MAX_NODES)
    , m_nodesContacted(0)
{
    AddDebugLogLineC(logSearch, "KadSearchController: Constructor called");
}

KadSearchController::~KadSearchController()
{
    AddDebugLogLineC(logSearch, "KadSearchController: Destructor called");
}

void KadSearchController::startSearch(const SearchParams& params)
{
    AddDebugLogLineC(logSearch, "KadSearchController::startSearch called with keyword='%s'", params.strKeyword.c_str());

    // Step 1: Validate prerequisites
    if (!validatePrerequisites()) {
	AddDebugLogLineC(logSearch, "KadSearchController: Prerequisites validation failed");
	return;
    }

    // Step 2: Validate search parameters
    if (!validateSearchParams(params)) {
	AddDebugLogLineC(logSearch, "KadSearchController: Search parameters validation failed");
	return;
    }

    // Step 3: Prepare search
    initializeProgress();
    resetSearchState();

    // Build search packet using KadSearchPacketBuilder
    KadSearchPacketBuilder packetBuilder;
    wxString error;

    try {
	// Build Kad search packet
	uint8_t* packet = nullptr;
	uint32_t packetSize = 0;
	bool success = packetBuilder.CreateSearchPacket(params, packet, packetSize);

	if (!packet) {
	    error = "Failed to create Kad search packet";
	    AddDebugLogLineC(logSearch, "KadSearchController: " + error.ToStdString());
	    return handleSearchError(0, error);
	}
	
	AddDebugLogLineC(logSearch, "KadSearchController: Search packet created, size=%u", packetSize);
	
	// Check if Kad network is running
	if (theApp->kad && theApp->kad->IsRunning()) {
	    AddDebugLogLineC(logSearch, "KadSearchController: Kad network is running, starting search");
	    
	    // Start the Kad search
	    uint32_t searchId = theApp->kad->StartSearch(packet, packetSize, params.strKeyword);
	    AddDebugLogLineC(logSearch, "KadSearchController: Kad search manager returned search ID=%u", searchId);
	    
	    if (searchId != 0) {
		// Register this controller for the search ID
		SearchResultRouter::Instance().registerController(searchId, this);
		AddDebugLogLineC(logSearch, "KadSearchController: Registered controller for search ID=%u", searchId);
		
		// Set current search ID in SearchList
		if (theApp && theApp->searchlist) {
		    theApp->searchlist->SetCurrentID(searchId);
		    AddDebugLogLineC(logSearch, "KadSearchController: Set current search ID=%u in SearchList", searchId);
		}
		
		// Notify that search has started
		notifySearchStarted(searchId);
		AddDebugLogLineC(logSearch, "KadSearchController: Notified search started for ID=%u", searchId);
		
		return std::make_pair(searchId, wxString());
	    } else {
		error = "Kad search manager returned invalid search ID";
		AddDebugLogLineC(logSearch, "KadSearchController: Exception - " + error.ToStdString());
		return handleSearchError(0, error);
	    }
	} else {
	    error = "Kad network not available";
	    AddDebugLogLineC(logSearch, "KadSearchController: Exception - " + error.ToStdString());
	    return handleSearchError(0, error);
	}

    } catch (const std::exception& e) {
	error = wxString(e.what());
	AddDebugLogLineC(logSearch, "KadSearchController: Exception in startSearch - " + error.ToStdString());
	return handleSearchError(0, error);
    } catch (...) {
	AddDebugLogLineC(logSearch, "KadSearchController: Unknown exception in startSearch");
	error = "Unknown exception";
	return handleSearchError(0, error);
    }
}

void KadSearchController::stopSearch()
{
    AddDebugLogLineC(logSearch, "KadSearchController::stopSearch called");
    
    // Get the current search ID
    long searchId = getSearchId();
    if (searchId != -1) {
	// Unregister this controller
	SearchResultRouter::Instance().unregisterController(searchId);
	AddDebugLogLineC(logSearch, "KadSearchController: Unregistered controller for search ID=%ld", searchId);
    }

    // Clear results
    m_model->clearResults();

    // Use base class to handle common stop logic
    stopSearchBase();
}

void KadSearchController::requestMoreResults()
{
    AddDebugLogLineC(logSearch, "KadSearchController::requestMoreResults called");
    
    // Kad searches don't support "more results" in the traditional sense
    // as they are keyword-based and query the entire network
    uint32_t searchId = m_model->getSearchId();
    handleSearchError(searchId, _("Kad searches query the entire network and don't support requesting more results"));
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

    uint32_t searchId = m_model->getSearchId();
    notifyDetailedProgress(searchId, info);
    notifyProgress(searchId, info.percentage);
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
	uint32_t searchId = m_model->getSearchId();
	AddDebugLogLineC(logSearch, "KadSearchController: Kad network validation failed");
	handleSearchError(searchId, _("Kad network not available"));
	return false;
    }

    return true;
}

bool KadSearchController::isValidKadNetwork() const
{
    if (!isKadNetworkAvailable()) {
	AddDebugLogLineC(logSearch, "KadSearchController: Kad network validation failed");
	return false;
    }
    
    if (!theApp) {
	AddDebugLogLineC(logSearch, "KadSearchController: theApp is null");
	return false;
    }
    
    bool isRunning = theApp->kad && theApp->kad->IsRunning();
    AddDebugLogLineC(logSearch, "KadSearchController: Kad network is running: %s", isRunning ? "yes" : "no");
    return isRunning;
}

uint32_t KadSearchController::GenerateSearchId()
{
    // Generate a unique search ID for Kad
    // Kad uses a different ID space than ED2K
    static uint32_t s_nextKadSearchId = 0;
    s_nextKadSearchId = (s_nextKadSearchId + 1) % 0xFFFFFFFE;
    if (s_nextKadSearchId == 0) {
	s_nextKadSearchId = 1;
    }
    return s_nextKadSearchId;
}

} // namespace search
