#include "GlobalSearchController.h"
#include "../amule.h"
#include "../ServerConnect.h"
#include "../Server.h"
#include "../ServerList.h"
#include "../Logger.h"
#include "../SearchList.h"
#include "../Packet.h"
#include "../Statistics.h"
#include "../Tags/ClientTags.h"
#include "../Tags/FileTags.h"

namespace search {

GlobalSearchController::GlobalSearchController()
    : m_64bitSearchPacket(false)
{
}

GlobalSearchController::~GlobalSearchController()
{
    stopSearch();
}

void GlobalSearchController::startSearch(const SearchParams& params)
{
    // Create search data packet
    bool packetUsing64bit = false;
    CMemFilePtr data = theApp->searchlist->CreateSearchData(params, GlobalSearch, 
        theApp->serverconnect->GetCurrentServer() ? theApp->serverconnect->GetCurrentServer()->SupportsLargeFilesUDP() : false, 
        packetUsing64bit);
    
    if (!data) {
        handleSearchError(m_model->getSearchId(), _("Failed to create search data"));
        return;
    }

    // Create packet
    m_searchPacket.reset(new CPacket(*data.get(), OP_EDONKEYPROT, OP_GLOBSEARCHREQ));
    m_64bitSearchPacket = packetUsing64bit;
    m_queriedServers.clear();

    // Start timer
    m_searchTimer.Start(100);
}

void GlobalSearchController::stopSearch()
{
    m_searchTimer.Stop();
    m_searchPacket.reset();
    m_queriedServers.clear();
    m_model->setSearchState(SearchState::Idle);
}

void GlobalSearchController::onTimer()
{
    if (!m_searchPacket) {
        return;
    }

    // Get next server to query
    CServer* server = theApp->serverlist->GetNextServerForQuery();
    if (!server) {
        // No more servers
        stopSearch();
        return;
    }

    // Skip if already queried
    if (m_queriedServers.find(server->GetIP()) != m_queriedServers.end()) {
        return;
    }

    // Send search request
    if (server->SupportsLargeFilesUDP() && (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)) {
        CMemFile extData(50);
        uint32_t tagCount = 1;
        extData.WriteUInt32(tagCount);
        CTagVarInt flags(CT_SERVER_UDPSEARCH_FLAGS, SRVCAP_UDP_NEWTAGS_LARGEFILES);
        flags.WriteNewEd2kTag(&extData);
        CPacket *extSearchPacket = new CPacket(OP_GLOBSEARCHREQ3, 
            m_searchPacket->GetPacketSize() + extData.GetLength(), OP_EDONKEYPROT);
        extSearchPacket->CopyToDataBuffer(0, extData.GetRawBuffer(), extData.GetLength());
        extSearchPacket->CopyToDataBuffer(extData.GetLength(), 
            m_searchPacket->GetDataBuffer(), m_searchPacket->GetPacketSize());
        theStats::AddUpOverheadServer(extSearchPacket->GetPacketSize());
        theApp->serverconnect->SendUDPPacket(extSearchPacket, server, true);
    } else if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
        m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ2);
        theStats::AddUpOverheadServer(m_searchPacket->GetPacketSize());
        theApp->serverconnect->SendUDPPacket(m_searchPacket.get(), server, false);
    } else {
        m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ);
        theStats::AddUpOverheadServer(m_searchPacket->GetPacketSize());
        theApp->serverconnect->SendUDPPacket(m_searchPacket.get(), server, false);
    }

    // Mark server as queried
    m_queriedServers.insert(server->GetIP());
}

bool GlobalSearchController::isSearching() const
{
    return m_searchTimer.IsRunning();
}

} // namespace search