#include "GlobalSearchController.h"
#include "ED2KSearchPacketBuilder.h"
#include "SearchTypeConverter.h"
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
#include <protocol/ed2k/Client2Server/UDP.h>
#include <tags/ClientTags.h>
#include <wx/utils.h>
#include "../Logger.h"
#include "SearchLogging.h"
#include "SearchIdGenerator.h"

namespace search {

GlobalSearchController::GlobalSearchController()
{
}

GlobalSearchController::~GlobalSearchController()
{
    stopSearch();
}

uint32_t GlobalSearchController::GenerateSearchId()
{
    return SearchIdGenerator::GenerateEd2kSearchId();
}

void GlobalSearchController::startSearch(const SearchParams& params)
{
    // Validate prerequisites
    if (!validatePrerequisites()) {
        handleSearchError(m_model->getSearchId(), _("Failed to validate prerequisites for global search"));
        return;
    }
    
    // Validate search parameters
    wxString validationError;
    if (!validateSearchParams(params)) {
        handleSearchError(m_model->getSearchId(), _("Invalid search parameters"));
        return;
    }
    
    uint32_t searchId = 0;
    
    // Get search ID from model or generate new one
    if (m_model->getSearchId() == -1) {
        searchId = GenerateSearchId();
        SEARCH_DEBUG_CONTROLLER(
            CFormat(wxT("GlobalSearchController: Generated new search ID %u for global search"))
            % searchId);
    } else {
        // Use the existing search ID from the model
        searchId = m_model->getSearchId();
        SEARCH_DEBUG_CONTROLLER(
            CFormat(wxT("GlobalSearchController: Using existing search ID %u for global search (more results)"))
            % searchId);
    }
    
    // Set up early registration in SearchList to ensure proper result routing
    if (theApp && theApp->searchlist) {
        // Store search parameters in SearchList for later use
        CSearchList::CSearchParams oldParams;
        oldParams.searchString = params.searchString;
        oldParams.strKeyword = params.strKeyword;
        oldParams.typeText = params.typeText;
        oldParams.extension = params.extension;
        oldParams.minSize = params.minSize;
        oldParams.maxSize = params.maxSize;
        oldParams.availability = params.availability;
        oldParams.searchType = GlobalSearch;

        // Set the current search ID in SearchList
        theApp->searchlist->SetCurrentSearch(searchId);

        // Register this search in active searches map
        theApp->searchlist->RegisterActiveSearch(searchId, GlobalSearch);

        // Store search parameters in SearchStateManager
        theApp->searchlist->StoreSearchParams(searchId, oldParams);

        SEARCH_DEBUG_CONTROLLER(
            CFormat(wxT("GlobalSearchController: Set current search ID %u in SearchList for global search"))
            % searchId);
    }

    // Build search packet using ED2KSearchPacketBuilder
    ED2KSearchPacketBuilder packetBuilder;
    uint8_t* packetData = nullptr;
    uint32_t packetSize = 0;
    bool supports64bit = theApp->serverconnect->GetCurrentServer() ?
        theApp->serverconnect->GetCurrentServer()->SupportsLargeFilesTCP() : false;
    bool success = packetBuilder.CreateSearchPacket(params, supports64bit, packetData, packetSize);
    
    if (!success || !packetData) {
        wxString errorMessage = wxT("Failed to create global search packet");
        handleSearchError(searchId, errorMessage);
        return;
    }
    
    // Update search state with the new search ID
    updateSearchState(params, searchId, SearchState::Searching);
    
    // Send packet to servers via UDP for global search
    if (theApp && theApp->serverconnect) {
        theStats::AddUpOverheadServer(packetSize);
        
        // Create a CMemFile from the raw data
        CMemFile dataFile(packetData, packetSize);
        
        // For global search, we need to send UDP packets to multiple servers
        auto serverSnapshot = theApp->serverlist->CopySnapshot();
        bool packetSent = false;
        
        for (const CServer* server : serverSnapshot) {
            if (!server) continue;
            
            // Check if server supports UDP search
            if (!(server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)) {
                SEARCH_DEBUG_CONTROLLER(
                    CFormat(wxT("GlobalSearchController: Skipping server %s:%u - does not support UDP search"))
                    % server->GetAddress() % server->GetPort());
                continue; // Skip servers that don't support UDP search
            }
            
            // Determine which search request type to use based on server capabilities
            CPacket* packet = nullptr;
            uint8_t opcode = OP_GLOBSEARCHREQ;
            
            if (server->SupportsLargeFilesUDP() && (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)) {
                // Use OP_GLOBSEARCHREQ3 for servers that support large files and extended getfiles
                CMemFile extData(50);
                uint32_t tagCount = 1;
                extData.WriteUInt32(tagCount);
                CTagVarInt flags(CT_SERVER_UDPSEARCH_FLAGS, SRVCAP_UDP_NEWTAGS_LARGEFILES);
                flags.WriteNewEd2kTag(&extData);
                
                packet = new CPacket(OP_GLOBSEARCHREQ3, dataFile.GetLength() + (uint32_t)extData.GetLength(), OP_EDONKEYPROT);
                packet->CopyToDataBuffer(0, extData.GetRawBuffer(), extData.GetLength());
                packet->CopyToDataBuffer(extData.GetLength(), dataFile.GetRawBuffer(), dataFile.GetLength());
                opcode = OP_GLOBSEARCHREQ3;
            } else if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
                // Use OP_GLOBSEARCHREQ2 for servers that support extended getfiles
                packet = new CPacket(dataFile, OP_EDONKEYPROT, OP_GLOBSEARCHREQ2);
                opcode = OP_GLOBSEARCHREQ2;
            } else {
                // Use OP_GLOBSEARCHREQ for basic servers (shouldn't happen due to above check)
                packet = new CPacket(dataFile, OP_EDONKEYPROT, OP_GLOBSEARCHREQ);
                opcode = OP_GLOBSEARCHREQ;
            }
            
            SEARCH_DEBUG_CONTROLLER(
                CFormat(wxT("GlobalSearchController: Sending global search packet to server %s:%u, searchId=%u, packetSize=%u, opcode=0x%x"))
                % server->GetAddress() % server->GetPort() % searchId % packetSize % opcode);

            // Send UDP packet to server
            theApp->serverconnect->SendUDPPacket(packet, const_cast<CServer*>(server), true, false, 0);
            packetSent = true;
        }
        
        if (!packetSent) {
            wxString errorMessage = wxT("No servers available for global search");
            handleSearchError(searchId, errorMessage);
        }
    }
    
    // Clean up packet data
    delete[] packetData;
}

void GlobalSearchController::stopSearch()
{
    // Nothing special to do for stopping global search
    // The UDP packets have already been sent
    m_model->setSearchState(SearchState::Idle);
    AddDebugLogLineN(logSearch, "GlobalSearchController: Stopped global search");
}

void GlobalSearchController::requestMoreResults()
{
    // Global search doesn't support "more results" in the same way as local search
    // This is handled by the SearchList when results are received
    SEARCH_DEBUG_CONTROLLER("GlobalSearchController: requestMoreResults called (not implemented)");
}

} // namespace search