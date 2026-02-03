
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
#include "../amule.h"
#include "../ServerConnect.h"
#include "../Logger.h"
#include "../MemFile.h"
#include "../Packet.h"
#include "../Statistics.h"
#include "../search/SearchLogging.h"
#include "../search/ED2KSearchPacketBuilder.h"
#include "../include/protocol/Protocols.h"
#include "../include/protocol/ed2k/Client2Server/TCP.h"
#include <wx/string.h>

namespace search {

ED2KSearchController::ED2KSearchController()
{
}

ED2KSearchController::~ED2KSearchController()
{
    stopSearch();
}

void ED2KSearchController::startSearch(const SearchParams& params)
{
    if (!validateSearchParams(params)) {
        uint32_t searchId = m_model->getSearchId();
        handleSearchError(searchId, _("Invalid search parameters"));
        return;
    }
    
    // Store search parameters in model
    m_model->setSearchParams(params);
    
    // Generate unique search ID
    uint32_t searchId = m_model->getSearchId(); // Use model's search ID instead of GenerateSearchId
    updateSearchState(params, searchId, SearchState::Searching);
    
    // Create search packet
    uint8_t* packetData = nullptr;
    uint32_t packetSize = 0;
    
    if (!ED2KSearchPacketBuilder::CreateSearchPacket(params, packetData, packetSize)) {
        handleSearchError(searchId, _("Failed to create ED2K search packet"));
        return;
    }
    
    // Send packet via UDP
    bool success = false;
    if (theApp && theApp->serverconnect) {
        theStats::AddUpOverheadServer(packetSize);
        CMemFile dataFile(packetData, packetSize);
        CPacket packet(dataFile, OP_EDONKEYPROT, OP_SEARCHREQUEST);

        SEARCH_DEBUG_CONTROLLER(
            wxString::Format(wxT("ED2KSearchController: Sending ED2K search packet to server, searchId=%u, packetSize=%u"),
                searchId, packetSize));

        CServer* currentServer = theApp->serverconnect->GetCurrentServer();
        if (currentServer) {
            success = theApp->serverconnect->SendUDPPacket(&packet, currentServer, true, false, 0);
        } else {
            success = theApp->serverconnect->SendPacket(&packet, false); // false for global search
        }
        
        if (success) {
            AddLogLineN(wxString::Format(_("ED2K search started with ID: %u"), searchId));
            notifySearchStarted(searchId);
        } else {
            handleSearchError(searchId, _("Failed to send ED2K search packet"));
        }
    } else {
        handleSearchError(searchId, _("No server connection available"));
    }
    
    // Clean up allocated packet data
    delete[] packetData;
}

void ED2KSearchController::stopSearch()
{
    // ED2K searches are handled by the server, so we just mark as completed
    m_model->setSearchState(SearchState::Completed);
}

void ED2KSearchController::requestMoreResults()
{
    // ED2K searches don't support requesting more results
    // They are fire-and-forget searches
    uint32_t searchId = m_model->getSearchId();
    notifyError(searchId, _("ED2K searches do not support requesting more results"));
}

bool ED2KSearchController::isSearching() const
{
    return m_model->getSearchState() == SearchState::Searching;
}

bool ED2KSearchController::validateSearchParams(const SearchParams& params)
{
    // Basic validation - ensure search string is not empty
    return !params.searchString.IsEmpty();
}

} // namespace search
