#include "LocalSearchController.h"
#include "../amule.h"
#include "../ServerConnect.h"
#include "../Server.h"
#include "../Logger.h"
#include "../MemFile.h"
#include "../Packet.h"
#include "../Statistics.h"
#include "../include/tags/ClientTags.h"
#include "../include/tags/FileTags.h"
#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Server/TCP.h>
#include "../search/SearchLogging.h"
#include "../search/SearchTypeConverter.h"
#include <wx/utils.h>

namespace search {

LocalSearchController::LocalSearchController()
    : m_64bitSearchPacket(false)
{
}

LocalSearchController::~LocalSearchController()
{
    stopSearch();
}

void LocalSearchController::startSearch(const SearchParams& params)
{
    // Validate parameters
    if (params.searchString.IsEmpty()) {
        handleSearchError(-1, _("Search string is empty"));
        return;
    }

    // Generate search ID
    uint32_t searchId = m_model->getSearchId() == -1 ? GenerateSearchId() : m_model->getSearchId();
    SEARCH_DEBUG_CONTROLLER(
        wxString::Format(wxT("LocalSearchController: Generated search ID %u for local search: %s"), 
            searchId, params.searchString.c_str()));

    // Store search ID in model
    m_model->setSearchId(searchId);
    m_model->setSearchState(SearchState::Searching);

    // Register with SearchResultRouter for result routing
    SearchResultRouter::Instance().RegisterController(searchId, this);
    SearchResultRouter::Instance().RegisterTypeController(::LocalSearch, this);

    // Build and send search packet directly
    uint8_t* packetData = nullptr;
    uint32_t packetSize = 0;

    if (!createSearchPacket(params, searchId, packetData, packetSize)) {
        handleSearchError(m_model->getSearchId(), _("Failed to create local search packet"));
        return;
    }

    // Send packet to server
    if (theApp && theApp->serverconnect) {
        theStats::AddUpOverheadServer(packetSize);
        CMemFile dataFile(packetData, packetSize);
        m_searchPacket.reset(new CPacket(dataFile, OP_EDONKEYPROT, OP_SEARCHREQUEST));

        SEARCH_DEBUG_CONTROLLER(
            wxString::Format(wxT("LocalSearchController: Sending local search packet to server, searchId=%u, packetSize=%u"),
                searchId, packetSize));

        theApp->serverconnect->SendPacket(m_searchPacket.get(), true); // true for local search
        
        delete[] packetData;
    } else {
        delete[] packetData;
        handleSearchError(m_model->getSearchId(), _("No server connection available"));
    }
}

uint32_t LocalSearchController::GenerateSearchId()
{
    // Local search IDs: 0x80000001 to 0xBFFFFFFF (2,147,483,649 to 3,221,225,471)
    // This provides ~1 billion unique IDs for local searches
    uint32_t baseId = 0x80000001;  // 2,147,483,649
    uint32_t rangeSize = 0x40000000; // 1,073,741,824 IDs available
    return baseId + (s_searchIdCounter++ % rangeSize);
}

void LocalSearchController::stopSearch()
{
    m_searchPacket.reset();
    m_model->setSearchState(SearchState::Idle);
}

bool LocalSearchController::isSearching() const
{
    return m_searchPacket != nullptr;
}

void LocalSearchController::requestMoreResults()
{
    // For local search, "more results" means sending the same search request again
    // This is typically used when the user wants to refresh results or get more from the same server
    if (!isSearching() || m_model->getSearchId() == -1) {
        return;
    }

    // Re-send the same search packet
    if (theApp && theApp->serverconnect && m_searchPacket) {
        theApp->serverconnect->SendPacket(m_searchPacket.get(), true);
        SEARCH_DEBUG_CONTROLLER(
            wxString::Format(wxT("LocalSearchController: Requested more results for search ID %u"), 
                m_model->getSearchId()));
    }
}

bool LocalSearchController::createSearchPacket(const SearchParams& params, uint32_t searchId, uint8_t*& packetData, uint32_t& packetSize)
{
    CMemFile packet;
    
    // OP_SEARCHREQUEST (0x13)
    packet.WriteUInt8(0x13);
    
    // Search ID (4 bytes)
    packet.WriteUInt32(searchId);
    
    // Search string with length prefix
    wxString searchString = params.searchString;
    if (searchString.IsEmpty()) {
        searchString = params.strKeyword;
    }
    
    // Write string length (16-bit)
    uint16_t strLen = (uint16_t)std::min(searchString.length(), (size_t)65535);
    packet.WriteUInt16(strLen);
    
    // Write string data (UTF-8)
    wxCharBuffer utf8Buffer = searchString.utf8_str();
    packet.Write(utf8Buffer.data(), strLen);
    
    // Optional parameters
    if (!params.typeText.IsEmpty()) {
        // Type parameter
        packet.WriteUInt8(0x01); // TYPE
        packet.WriteUInt8(params.typeText[0]); // First char of type
    }
    
    if (params.minSize > 0) {
        // Min size parameter
        packet.WriteUInt8(0x02); // MIN_SIZE
        packet.WriteUInt32((uint32_t)params.minSize);
    }
    
    if (params.maxSize > 0) {
        // Max size parameter  
        packet.WriteUInt8(0x03); // MAX_SIZE
        packet.WriteUInt32((uint32_t)params.maxSize);
    }
    
    if (!params.extension.IsEmpty()) {
        // Extension parameter
        packet.WriteUInt8(0x04); // EXTENSION
        wxString ext = params.extension;
        uint16_t extLen = (uint16_t)std::min(ext.length(), (size_t)65535);
        packet.WriteUInt16(extLen);
        wxCharBuffer extUtf8 = ext.utf8_str();
        packet.Write(extUtf8.data(), extLen);
    }
    
    // End of parameters marker
    packet.WriteUInt8(0x00);
    
    packetSize = packet.GetLength();
    packetData = new uint8_t[packetSize];
    memcpy(packetData, packet.GetBuffer(), packetSize);
    
    return true;
}

} // namespace search