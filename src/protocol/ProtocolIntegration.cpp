#include "protocol/ProtocolCoordinator.h"
#include "protocol/bt/BitTorrentSession.h"
#include "ClientList.h"
#include "ServerList.h"
#include "DownloadQueue.h"
#include "SharedFiles.h"
#include "common/NetworkPerformanceMonitor.h"

using namespace ProtocolIntegration;

ProtocolCoordinator& ProtocolCoordinator::instance() {
    static ProtocolCoordinator instance;
    return instance;
}

std::vector<SourceEndpoint> ProtocolCoordinator::discover_sources(
    const CPartFile* file, 
    ProtocolType preferred,
    uint32_t max_sources) {
    
    std::vector<SourceEndpoint> sources;
    
    // Get ED2K sources from existing infrastructure
    if (preferred == ProtocolType::ED2K || preferred == ProtocolType::HYBRID_AUTO) {
        auto ed2k_sources = theApp->downloadqueue->GetSourcesForFile(file);
        for (const auto& client : ed2k_sources) {
            SourceEndpoint endpoint;
            endpoint.protocol = ProtocolType::ED2K;
            endpoint.address = Uint32toStringIP(client->GetIP());
            endpoint.port = client->GetUserPort();
            endpoint.ed2k_hash = file->GetFileHash();
            endpoint.reliability_score = calculate_client_reliability(client);
            sources.push_back(endpoint);
        }
    }
    
    // Get BitTorrent sources if hybrid mode enabled
    if ((preferred == ProtocolType::BITTORRENT || preferred == ProtocolType::HYBRID_AUTO) &&
        is_hybrid_mode_enabled()) {
        
        auto& bt_session = BitTorrent::BitTorrentSession::instance();
        auto bt_sources = bt_session.find_cross_protocol_sources(file, nullptr);
        
        for (const auto& bt_client : bt_sources) {
            SourceEndpoint endpoint;
            endpoint.protocol = ProtocolType::BITTORRENT;
            // Convert BT client to endpoint (implementation depends on BT session)
            sources.push_back(endpoint);
        }
    }
    
    // Remove duplicates and limit results
    remove_duplicate_sources(sources);
    if (sources.size() > max_sources) {
        sources.resize(max_sources);
    }
    
    return sources;
}

bool ProtocolCoordinator::add_source(const SourceEndpoint& source, CPartFile* file) {
    switch (source.protocol) {
        case ProtocolType::ED2K:
            return add_ed2k_source(source, file);
        case ProtocolType::BITTORRENT:
            return add_bittorrent_source(source, file);
        case ProtocolType::KADEMLIA:
            return add_kad_source(source, file);
        default:
            return false;
    }
}

bool ProtocolCoordinator::add_ed2k_source(const SourceEndpoint& source, CPartFile* file) {
    try {
        // Create ED2K client from endpoint
        auto client = std::make_shared<CUpDownClient>(
            nullptr, // socket will be created later
            source.address,
            source.port,
            source.ed2k_hash
        );
        
        // Add to download queue
        return theApp->downloadqueue->AddSource(client.get(), file);
    } catch (...) {
        return false;
    }
}

bool ProtocolCoordinator::add_bittorrent_source(const SourceEndpoint& source, CPartFile* file) {
    auto& bt_session = BitTorrent::BitTorrentSession::instance();
    
    // Convert ED2K file to BT metadata if needed
    std::string bt_metadata;
    if (!convert_metadata_ed2k_to_bt(file, bt_metadata)) {
        return false;
    }
    
    // Add peer to BT session
    return bt_session.add_peer(source.address, source.port, bt_metadata);
}

ProtocolType ProtocolCoordinator::select_optimal_protocol(
    const CPartFile* file,
    const NetworkConditions& conditions) const {
    
    // Simple heuristic based on file properties and network conditions
    double ed2k_score = calculate_ed2k_score(file, conditions);
    double bt_score = calculate_bt_score(file, conditions);
    
    if (ed2k_score > bt_score * 1.2) {
        return ProtocolType::ED2K;
    } else if (bt_score > ed2k_score * 1.2) {
        return ProtocolType::BITTORRENT;
    } else {
        return ProtocolType::HYBRID_AUTO;
    }
}

double ProtocolCoordinator::calculate_ed2k_score(
    const CPartFile* file,
    const NetworkConditions& conditions) const {
    
    // ED2K works better for:
    // - Older files with many sources
    // - Stable, low-latency connections
    // - Files with good ED2K availability
    
    double score = 0.0;
    
    // Factor in file age and source count
    if (file->GetSourceCount() > 10) {
        score += 30.0;
    }
    
    // Network conditions
    if (conditions.latency_ms < 100) {
        score += 20.0;
    }
    
    return score;
}

double ProtocolCoordinator::calculate_bt_score(
    const CPartFile* file,
    const NetworkConditions& conditions) const {
    
    // BitTorrent works better for:
    // - Newer, popular files
    // - High-bandwidth connections
    // - Files with good BT availability
    
    double score = 0.0;
    
    // Factor in potential BT availability (estimation)
    if (file->GetFileSize() > 100 * 1024 * 1024) { // Large files
        score += 25.0;
    }
    
    // Network conditions
    if (conditions.bandwidth_kbps > 1000) {
        score += 30.0;
    }
    
    return score;
}

bool ProtocolCoordinator::convert_metadata_ed2k_to_bt(
    const CPartFile* ed2k_file,
    std::string& bt_metadata) {
    
    // Convert ED2K file metadata to BT info dictionary
    try {
        // Basic metadata conversion
        bt_metadata = "d4:info"; // Start of BT info dictionary
        
        // Add file name
        std::string name = ed2k_file->GetFileName().ToUTF8();
        bt_metadata += "4:name" + std::to_string(name.length()) + ":" + name;
        
        // Add file length
        uint64_t length = ed2k_file->GetFileSize();
        bt_metadata += "6:lengthi" + std::to_string(length) + "e";
        
        // Add piece length (standard 256KB for BT)
        bt_metadata += "10:piece lengthi262144e";
        
        // Convert ED2K hash to BT info hash
        std::string info_hash = BitTorrent::ed2k_hash_to_info_hash(ed2k_file->GetFileHash());
        bt_metadata += "7:pieces20:" + info_hash;
        
        bt_metadata += "e"; // End of dictionary
        
        return true;
    } catch (...) {
        return false;
    }
}

// Network performance monitoring integration
void monitor_cross_protocol_traffic(const SourceEndpoint& source, uint64_t bytes) {
    auto& monitor = network_perf::g_network_perf_monitor;
    
    switch (source.protocol) {
        case ProtocolType::ED2K:
            monitor.record_tcp_received(bytes);
            break;
        case ProtocolType::BITTORRENT:
            monitor.record_bt_received(bytes);
            break;
        case ProtocolType::KADEMLIA:
            monitor.record_udp_received(bytes);
            break;
        default:
            monitor.record_received(bytes);
    }
}