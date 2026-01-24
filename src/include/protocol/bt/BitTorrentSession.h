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

#pragma once

#include "protocol/Protocols.h"
#include "protocol/ed2k/Constants.h"
#include "protocol/kad/Constants.h"
#include "../../../common/NetworkPerformanceMonitor.h"
#include "../../../MD4Hash.h"
#include <memory>
#include <vector>
#include <string>
#include <ctime>

// Forward declarations
class CPartFile;
class CKnownFile;
class CUpDownClient;

namespace BitTorrent {

// Search result structure for DHT searches
struct SearchResult {
    std::string name;
    std::string info_hash;
    uint64_t size;
    std::vector<std::string> trackers;
    std::vector<std::pair<std::string, uint16_t>> peers;
    time_t creation_date;
    std::string comment;
    std::string source;  // "dht", "tracker", "peer_exchange"
    double download_speed;
    double upload_speed;
    uint32_t seeders;
    uint32_t leechers;
    
    SearchResult() : size(0), creation_date(0), download_speed(0), upload_speed(0), seeders(0), leechers(0) {}
};

class BitTorrentSession {
public:
    // Singleton access
    static BitTorrentSession& instance();
    
    // Initialization and shutdown
    bool initialize();
    void shutdown();
    
    // File operations
    bool add_torrent(const std::string& torrent_file, const std::string& save_path);
    bool add_magnet_link(const std::string& magnet_link, const std::string& save_path);
    bool remove_torrent(const std::string& info_hash, bool remove_data = false);
    
    // Cross-protocol integration
    std::vector<std::shared_ptr<CUpDownClient>> find_cross_protocol_sources(
        const CPartFile* ed2k_file, const CKnownFile* known_file);
    
    bool import_ed2k_to_torrent(const CPartFile* ed2k_file, const std::string& torrent_path);
    bool export_torrent_to_ed2k(const std::string& torrent_file, CPartFile* ed2k_file);
    
    // Protocol coordination
    enum class ProtocolPriority {
        ED2K_FIRST,
        BT_FIRST, 
        HYBRID_AUTO,
        HYBRID_BANDWIDTH
    };
    
    void set_protocol_priority(ProtocolPriority priority);
    ProtocolPriority get_protocol_priority() const;
    
    // Statistics and monitoring
    struct SessionStats {
        uint64_t total_downloaded;
        uint64_t total_uploaded;
        uint32_t active_torrents;
        uint32_t connected_peers;
        uint32_t active_trackers;
        double download_rate;
        double upload_rate;
    };
    
    SessionStats get_session_stats() const;
    
    // DHT integration with KAD
    bool enable_dht_integration(bool enable);
    bool share_dht_routing_table();
    std::vector<std::string> get_shared_peers() const;
    
    // DHT Search functionality (Step 1: Basic DHT Search API)
    std::vector<SearchResult> dht_search(const std::string& keyword);
    std::vector<SearchResult> dht_search_by_infohash(const std::string& info_hash);
    void start_dht_announce(const std::string& info_hash);
    
private:
    BitTorrentSession();
    ~BitTorrentSession();
    
    // Internal implementation
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Disable copying
    BitTorrentSession(const BitTorrentSession&) = delete;
    BitTorrentSession& operator=(const BitTorrentSession&) = delete;
};

// Cross-protocol helper functions
std::string ed2k_hash_to_info_hash(const CMD4Hash& ed2k_hash);
CMD4Hash info_hash_to_ed2k_hash(const std::string& info_hash);

bool is_cross_protocol_compatible(const CPartFile* ed2k_file, const std::string& info_hash);
bool create_hybrid_download(const std::string& info_hash, CPartFile* ed2k_file);

} // namespace BitTorrent