#include "protocol/ProtocolCoordinator.h"
#include "protocol/bt/BitTorrentSession.h"
#include "../amule.h"
#include "../DownloadQueue.h"
#include "../ClientList.h"
#include "../PartFile.h"
#include "../updownclient.h"
#include "../common/NetworkPerformanceMonitor.h"

namespace ProtocolIntegration {

class ProtocolCoordinator::Impl {
private:
    bool m_hybrid_mode;
    uint32_t m_max_cross_sources;
    CoordinationStats m_stats;
    
public:
    Impl() : 
        m_hybrid_mode(true),
        m_max_cross_sources(50) {}
        
    std::vector<SourceEndpoint> discover_sources(
        const CPartFile* file,
        ProtocolType preferred,
        uint32_t max_sources) {
        
        std::vector<SourceEndpoint> sources;
        
        // 1. Get ED2K sources
        if (preferred == ProtocolType::ED2K || preferred == ProtocolType::HYBRID_AUTO) {
            const auto& source_list = file->GetSourceList();
            for (const auto& client_ref : source_list) {
                CUpDownClient* client = client_ref.GetClient();
                if (client && client->GetIP() != 0) {
                    SourceEndpoint endpoint;
                    endpoint.protocol = ProtocolType::ED2K;
                    endpoint.address = Uint32toStringIP(client->GetIP());
                    endpoint.port = client->GetUserPort();
                    endpoint.ed2k_hash = file->GetFileHash();
                    sources.push_back(endpoint);
                }
            }
            m_stats.total_sources_discovered += source_list.size();
        }
        
        // 2. Get BT sources if hybrid enabled
        if ((preferred == ProtocolType::BITTORRENT || preferred == ProtocolType::HYBRID_AUTO) && 
            m_hybrid_mode) {
            
            auto& bt_session = BitTorrent::BitTorrentSession::instance();
            std::string bt_hash = BitTorrent::ed2k_hash_to_info_hash(file->GetFileHash());
            
            // TODO: Implement proper peer discovery for BitTorrent
            // For now, use placeholder empty list
            std::vector<std::pair<std::string, uint16_t>> bt_peers;
            
            for (const auto& peer : bt_peers) {
                SourceEndpoint endpoint;
                endpoint.protocol = ProtocolType::BITTORRENT;
                endpoint.address = peer.first;
                endpoint.port = peer.second;
                endpoint.info_hash = bt_hash;
                sources.push_back(endpoint);
            }
            m_stats.total_sources_discovered += bt_peers.size();
            m_stats.cross_protocol_sources += bt_peers.size();
        }
        
        // 3. Remove duplicates
        remove_duplicates(sources);
        
        // 4. Limit results
        if (sources.size() > max_sources) {
            sources.resize(max_sources);
        }
        
        return sources;
    }
    
    void remove_duplicates(std::vector<SourceEndpoint>& sources) {
        // Implementation of duplicate removal logic
        size_t removed_count = 0;
        // ... duplicate removal implementation
        m_stats.duplicate_sources_removed += removed_count;
    }
    
    // ...其他实现方法...
};

ProtocolCoordinator::ProtocolCoordinator() : 
    pimpl_(std::make_unique<Impl>()) {}
    
ProtocolCoordinator::~ProtocolCoordinator() = default;

std::vector<SourceEndpoint> ProtocolCoordinator::discover_sources(
    const CPartFile* file,
    ProtocolType preferred,
    uint32_t max_sources) {
    
    return pimpl_->discover_sources(file, preferred, max_sources);
}

} // namespace ProtocolIntegration