#include "LocalSearchController.h"
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

// Static member definition
uint32_t LocalSearchController::s_searchIdCounter = 0;

LocalSearchController::LocalSearchController()
{
}

LocalSearchController::~LocalSearchController()
{
    stopSearch();
}

void LocalSearchController::startSearch(const SearchParams& params)
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
    
    // Create search packet
    uint8_t* packetData = nullptr;
    uint32_t packetSize = 0;
    
    if (!ED2KSearchPacketBuilder::CreateSearchPacket(params, packetData, packetSize)) {
        handleSearchError(searchId, _("Failed to create local search packet"));
        return;
    }
    
    // Send packet to server
    bool success = false;
    if (theApp && theApp->serverconnect) {
        theStats::AddUpOverheadServer(packetSize);
        CMemFile dataFile(packetData, packetSize);
        CPacket packet(dataFile, OP_EDONKEYPROT, OP_SEARCHREQUEST);

        SEARCH_DEBUG_CONTROLLER(
            wxString::Format(wxT("LocalSearchController: Sending local search packet to server, searchId=%u, packetSize=%u"),
                searchId, packetSize));

        success = theApp->serverconnect->SendPacket(&packet, true); // true for local search
        
        if (success) {
            AddLogLineN(wxString::Format(_("Local search started with ID: %u"), searchId));
            notifySearchStarted(searchId);
        } else {
            handleSearchError(searchId, _("Failed to send local search packet"));
        }
    } else {
        handleSearchError(searchId, _("No server connection available"));
    }
    
    delete[] packetData;
}

void LocalSearchController::stopSearch()
{
    // Local searches are handled by the server, so we just mark as completed
    m_model->setSearchState(SearchState::Completed);
}

void LocalSearchController::requestMoreResults()
{
    // Local searches don't support requesting more results explicitly
    uint32_t searchId = m_model->getSearchId();
    notifyError(searchId, _("Local searches do not support requesting more results"));
}

bool LocalSearchController::validateSearchParams(const SearchParams& params)
{
    // Basic validation - ensure search string is not empty
    return !params.searchString.IsEmpty();
}

} // namespace search