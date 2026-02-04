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
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/Search.h"
#include "../kademlia/net/KademliaUDPListener.h"
#include "../amule.h"
#include "../SearchList.h"
#include "../SearchFile.h"
#include "../Statistics.h"
#include <protocol/Protocols.h>
#include "../Logger.h"
#include "SearchTypeConverter.h"
#include "../search/SearchLogging.h"  // Include for search logging macros
#include "SearchIdGenerator.h"  // Include for shared search ID generation

namespace search {

KadSearchController::KadSearchController()
    : SearchControllerBase()
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

    // Build search packet using KadSearchPacketBuilder
    KadSearchPacketBuilder packetBuilder;
    wxString error;

    try {
	// Build Kad search packet
	uint8_t* packetData = nullptr;
	uint32_t packetSize = 0;
	bool success = packetBuilder.CreateSearchPacket(params, packetData, packetSize);

	if (!success || !packetData) {
	    error = "Failed to create Kad search packet";
	    return handleSearchError(0, error);
	}

	// Send packet to Kad network
	if (theApp && Kademlia::CKademlia::IsRunning()) {
	    // Use legacy Kad search implementation
	    try {
		Kademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(
		    params.strKeyword,
		    packetSize,
		    packetData,
		    0  // Let Kad search manager generate the search ID
		);

		// Get the actual search ID from Kad search manager
		uint32_t searchId = search->GetSearchID();

		// Store search ID and state
		m_model->setSearchParams(params);
		m_model->setSearchId(searchId);
		m_model->setSearchState(SearchState::Searching);

		// Register with SearchResultRouter for result routing
		SearchResultRouter::Instance().RegisterController(searchId, this);

		// Set the current search ID in SearchList after registration
		if (theApp->searchlist) {
		    theApp->searchlist->SetCurrentSearch(searchId);
		}

		notifySearchStarted(searchId);
	    } catch (const wxString& what) {
		error = wxString::Format(_("Failed to start Kad search: %s"), what.c_str());
		handleSearchError(0, error);
	    }

	    // Clean up packet data
	    delete[] packetData;
	} else {
	    delete[] packetData;
	    error = _("Kad network not available");
	    handleSearchError(0, error);
	}
    } catch (const wxString& e) {
	error = wxString::Format(_("Failed to execute Kad search: %s"), e.c_str());
	handleSearchError(0, error);
    }
}

void KadSearchController::stopSearch()
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

void KadSearchController::requestMoreResults()
{
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
    info.percentage = (m_nodesContacted * 100) / m_maxNodesToQuery;
    info.serversContacted = m_nodesContacted;
    info.resultsReceived = getResultCount();
    info.currentStatus = wxString::Format(_("Contacted %d of %d nodes"), m_nodesContacted, m_maxNodesToQuery);

    notifyDetailedProgress(m_model->getSearchId(), info);
}

void KadSearchController::initializeProgress()
{
    m_nodesContacted = 0;
    ProgressInfo info;
    info.percentage = 0;
    info.serversContacted = 0;
    info.resultsReceived = 0;
    info.currentStatus = _("Initializing Kad search");

    notifyDetailedProgress(m_model->getSearchId(), info);
}

bool KadSearchController::isValidKadNetwork() const
{
    return theApp && Kademlia::CKademlia::IsRunning();
}

uint32_t KadSearchController::GenerateSearchId()
{
    // Use SearchList's search ID generation for consistency
    if (theApp && theApp->searchlist) {
	return theApp->searchlist->GetNextSearchID();
    }
    return 0;
}

bool KadSearchController::validatePrerequisites()
{
    if (!isValidKadNetwork()) {
	wxString error = _("Kad network is not available");
	handleSearchError(0, error);
	return false;
    }
    return true;
}

} // namespace search
