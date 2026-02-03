
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
#include "../amule.h"
#include "../Logger.h"
#include "../MemFile.h"
#include "../search/SearchLogging.h"
#include "../search/KadSearchPacketBuilder.h"
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/SearchManager.h"
#include "../kademlia/kademlia/Search.h"
#include <wx/string.h>

namespace search {

KadSearchController::KadSearchController()
{
}

KadSearchController::~KadSearchController()
{
    stopSearch();
}

void KadSearchController::startSearch(const SearchParams& params)
{
    if (!validateSearchParams(params)) {
        uint32_t searchId = m_model->getSearchId();
        handleSearchError(searchId, _("Invalid search parameters"));
        return;
    }
    
    // Store search parameters in model
    m_model->setSearchParams(params);
    
    // Generate unique search ID
    uint32_t searchId = m_model->getSearchId();
    updateSearchState(params, searchId, SearchState::Searching);
    
    // Create search packet data
    uint8_t* packetData = nullptr;
    uint32_t packetSize = 0;
    
    if (!KadSearchPacketBuilder::CreateSearchPacket(params, packetData, packetSize)) {
        handleSearchError(searchId, _("Failed to create Kad search packet"));
        return;
    }
    
    // Start Kad search
    bool success = false;
    if (Kademlia::CKademlia::IsRunning()) {
        // Use the correct CSearchManager API
        auto search = Kademlia::CSearchManager::PrepareFindKeywords(
            params.searchString, packetSize, packetData, searchId);
        
        if (search) {
            success = Kademlia::CSearchManager::StartSearch(search);
        }
        
        if (success) {
            AddLogLineN(wxString::Format(_("Kad search started with ID: %u"), searchId));
            notifySearchStarted(searchId);
        } else {
            handleSearchError(searchId, _("Failed to start Kad search"));
        }
    } else {
        handleSearchError(searchId, _("Kad network is not running"));
    }
    
    delete[] packetData;
}

void KadSearchController::stopSearch()
{
    if (Kademlia::CKademlia::IsRunning()) {
        // Stop search with current search ID
        uint32_t searchId = m_model->getSearchId();
        Kademlia::CSearchManager::StopSearch(searchId, true);
    }
    m_model->setSearchState(SearchState::Completed);
}

bool KadSearchController::validateSearchParams(const SearchParams& params)
{
    // Basic validation - ensure search string is not empty
    return !params.searchString.IsEmpty();
}

void KadSearchController::requestMoreResults()
{
    // Kad searches automatically continue until completion
    uint32_t searchId = m_model->getSearchId();
    notifyError(searchId, _("Kad searches do not support manual request for more results"));
}

} // namespace search
