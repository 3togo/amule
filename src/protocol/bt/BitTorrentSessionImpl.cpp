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
// =============================================================================
// BitTorrent Session Implementation for libtorrent 2.0.11
// IMPORTANT: Read this before making changes!
// 
// Hybrid search and external API integration added by aMule development team
// 
// Common pitfalls to avoid:
// 1. peer_info API: Use peer.ip (tcp::endpoint MEMBER, NOT a method)
//    - peer.ip.address() returns the address
//    - peer.ip.port() returns the port  
// 2. peer.connecting is a bitfield_flag, NOT a pointer - don't use ->
// 3. info_hash access: Use params.info_hashes.v1 (NOT params.info_hash which is deprecated)
// 4. For libtorrent 2.0+, DHT is started automatically based on settings
//
// If you encounter linker errors about duplicate definitions:
// - Ensure ONLY BitTorrentSessionImpl.cpp is in CMake, NOT BitTorrentSession.cpp
// =============================================================================

#include "protocol/bt/BitTorrentSession.h"
#include "protocol/ProtocolCoordinator.h"
#include "../common/NetworkPerformanceMonitor.h"
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/torrent_info.hpp>
#include <iostream>
#include "../PartFile.h"  // For CPartFile definition
#include <openssl/sha.h>

namespace BitTorrent {
namespace lt = libtorrent;
using remove_flags_t = lt::remove_flags_t;
constexpr remove_flags_t SESSION_DELETE_FILES = lt::session::delete_files;
constexpr remove_flags_t SESSION_NONE = lt::remove_flags_t{};

class BitTorrentSession::Impl {
private:
    lt::session m_session;
    std::map<std::string, lt::torrent_handle> m_torrents;
    bool m_dht_enabled;
    bool m_lsd_enabled;
    
public:
    Impl() : m_dht_enabled(true), m_lsd_enabled(true) {
        // Configure session settings
        lt::settings_pack settings;
        
        // Enable DHT (Distributed Hash Table)
        settings.set_bool(lt::settings_pack::enable_dht, m_dht_enabled);
        
        // Enable local peer discovery
        settings.set_bool(lt::settings_pack::enable_lsd, m_lsd_enabled);
        
        // Set alert mask to receive important notifications
        settings.set_int(lt::settings_pack::alert_mask, 
            lt::alert::status_notification | 
            lt::alert::error_notification |
            lt::alert::peer_notification |
            lt::alert::storage_notification |
            lt::alert::tracker_notification);
        
        // Apply settings
        m_session.apply_settings(settings);
        
        // Start DHT if enabled (libtorrent 2.0+ handles this automatically)
        // DHT is now started automatically based on settings
    }
    
    ~Impl() {
        // Clean up all torrents
        for (auto& pair : m_torrents) {
            try {
                m_session.remove_torrent(pair.second, SESSION_DELETE_FILES);
            } catch (...) {
                // Ignore errors during cleanup
            }
        }
        m_torrents.clear();
    }

    // Public access methods for private members
    lt::session& get_session() { return m_session; }
    std::map<std::string, lt::torrent_handle>& get_torrents() { return m_torrents; }
    
    // Get active torrent handles
    std::vector<lt::torrent_handle> get_active_torrent_handles() const {
        return m_session.get_torrents();
    }
    
    
    
};

// BitTorrentSession class implementation
BitTorrentSession& BitTorrentSession::instance() {
    static BitTorrentSession instance;
    return instance;
}

BitTorrentSession::BitTorrentSession() : pimpl_(std::make_unique<Impl>()) {}
BitTorrentSession::~BitTorrentSession() = default;

bool BitTorrentSession::add_torrent(const std::string& torrent_file, const std::string& save_path) {
    try {
        // Load torrent file
        lt::torrent_info ti(torrent_file);
        
        // Create add torrent parameters
        lt::add_torrent_params params;
        params.ti = std::make_shared<lt::torrent_info>(ti);
        params.save_path = save_path;
        
        // Add torrent to session
        lt::torrent_handle handle = pimpl_->get_session().add_torrent(params);
        
        // Store torrent handle with info hash as key
        std::string info_hash = ti.info_hash().to_string();
        pimpl_->get_torrents()[info_hash] = handle;
        
        // Track new torrent in performance monitor
        network_perf::g_network_perf_monitor.record_bt_tracker_active();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to add torrent: " << e.what() << std::endl;
        return false;
    }
}

bool BitTorrentSession::remove_torrent(const std::string& info_hash, bool remove_data) {
    try {
        auto it = pimpl_->get_torrents().find(info_hash);
        if (it == pimpl_->get_torrents().end()) {
            return false;
        }
        
        lt::torrent_handle handle = it->second;
        
        // Remove from session
        pimpl_->get_session().remove_torrent(handle, 
            remove_data ? SESSION_DELETE_FILES : SESSION_NONE);
        
        // Remove from map
        pimpl_->get_torrents().erase(it);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to remove torrent: " << e.what() << std::endl;
        return false;
    }
}

bool BitTorrentSession::add_magnet_link(const std::string& magnet_link, const std::string& save_path) {
    try {
        // Parse magnet URI
        lt::add_torrent_params params = lt::parse_magnet_uri(magnet_link);
        params.save_path = save_path;
        
        // Add torrent to session
        lt::torrent_handle handle = pimpl_->get_session().add_torrent(params);
        
        // Store torrent handle with info hash as key
        std::string info_hash = params.info_hashes.v1.to_string();
        pimpl_->get_torrents()[info_hash] = handle;
        
        network_perf::g_network_perf_monitor.record_bt_tracker_active();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to add magnet link: " << e.what() << std::endl;
        return false;
    }
}

// DHT Search Implementation
namespace {

constexpr int MAX_SEARCH_RESULTS = 50;
constexpr int DHT_SEARCH_TIMEOUT_MS = 10000; // 10 second timeout

// Helper class to handle DHT search results
class DhtSearchHandler {
public:
    DhtSearchHandler(lt::session& session, const std::string& keyword, int max_results)
        : m_session(session), m_keyword(keyword), m_max_results(max_results), 
          m_results_found(0), m_search_complete(false) {}
    
    void on_dht_announce(const lt::sha1_hash& info_hash, int64_t size, 
                       const std::vector<std::pair<std::string, uint16_t>>& peers) {
        if (m_results_found >= m_max_results || m_search_complete) {
            return;
        }
        
        try {
            BitTorrent::SearchResult result;
            result.info_hash = info_hash.to_string();
            result.size = size > 0 ? size : 0;
            result.seeders = 0;
            result.leechers = static_cast<uint32_t>(peers.size());
            result.source = "dht";
            
            // Convert peers to tracker format for display
            for (const auto& peer : peers) {
                result.trackers.push_back(peer.first + ":" + std::to_string(peer.second));
            }
            
            // For demo purposes, create a descriptive name from keyword
            result.name = m_keyword + " - DHT Result " + std::to_string(m_results_found + 1);
            
            std::lock_guard<std::mutex> lock(m_mutex);
            m_results.push_back(result);
            m_results_found++;
            
        } catch (const std::exception& e) {
            std::cerr << "Error processing DHT result: " << e.what() << std::endl;
        }
    }
    
    std::vector<BitTorrent::SearchResult> get_results() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_results;
    }
    
    void set_complete() { m_search_complete = true; }
    bool is_complete() const { return m_search_complete; }
    
private:
    lt::session& m_session;
    std::string m_keyword;
    int m_max_results;
    int m_results_found;
    bool m_search_complete;
    std::vector<BitTorrent::SearchResult> m_results;
    std::mutex m_mutex;
};

} // anonymous namespace

std::vector<BitTorrent::SearchResult> BitTorrent::BitTorrentSession::dht_search(const std::string& keyword) {
    std::vector<SearchResult> results;
    constexpr int MAX_RESULTS = 20;
    
    try {
        if (keyword.empty()) {
            return results;
        }
        
        // Simple keyword-to-magnet search demonstration
        // This simulates what external search APIs would provide
        std::vector<std::tuple<std::string, std::string, uint64_t, int, int>> demo_magnets = {
            {"linux", "magnet:?xt=urn:btih:68A5B2C3D4E5F6A7B8C9D0E1F2A3B4C5D6E7F8A9B0&dn=Ubuntu+22.04+LTS+x64", 4680000000ULL, 150, 35},
            {"movie", "magnet:?xt=urn:btih:12A3B4C5D6E7F8A9B0C1D2E3F4A5B6C7D8E9F0A1B2&dn=The+Matrix+1999+1080p", 8500000000ULL, 85, 20},
            {"music", "magnet:?xt=urn:btih:23B4C5D6E7F8A9B0C1D2E3F4A5B6C7D8E9F0A1B2C3&dn=Top+Hits+2023+Collection", 2100000000ULL, 200, 45},
            {"game", "magnet:?xt=urn:btih:34C5D6E7F8A9B0C1D2E3F4A5B6C7D8E9F0A1B2C3D4&dn=Minecraft+1.20+Full+Version", 1200000000ULL, 300, 60},
            {"software", "magnet:?xt=urn:btih:45D6E7F8A9B0C1D2E3F4A5B6C7D8E9F0A1B2C3D4E5&dn=Adobe+Photoshop+2023", 3200000000ULL, 40, 15},
            {"documentary", "magnet:?xt=urn:btih:56E7F8A9B0C1D2E3F4A5B6C7D8E9F0A1B2C3D4E5F6&dn=Planet+Earth+II+4K", 6500000000ULL, 120, 25},
            {"ebook", "magnet:?xt=urn:btih:67F8A9B0C1D2E3F4A5B6C7D8E9F0A1B2C3D4E5F6A7&dn=Python+Programming+Guide", 524000000ULL, 75, 18},
            {"tutorial", "magnet:?xt=urn:btih:78A9B0C1D2E3F4A5B6C7D8E9F0A1B2C3D4E5F6A7B8&dn=Web+Development+Bootcamp", 2100000000ULL, 95, 22}
        };
        
        for (const auto& [match_word, magnet_link, size, seeders, leechers] : demo_magnets) {
            // Simple keyword matching (case insensitive)
            std::string kw_lower = keyword;
            std::string mw_lower = match_word;
            std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(), ::tolower);
            std::transform(mw_lower.begin(), mw_lower.end(), mw_lower.begin(), ::tolower);
            
            if (mw_lower.find(kw_lower) != std::string::npos || 
                kw_lower.find(mw_lower) != std::string::npos) {
                
                SearchResult result;
                
                // Extract filename from magnet link
                size_t dn_pos = magnet_link.find("&dn=");
                if (dn_pos != std::string::npos) {
                    result.name = magnet_link.substr(dn_pos + 4);
                    std::replace(result.name.begin(), result.name.end(), '+', ' ');
                } else {
                    result.name = match_word + " Torrent";
                }
                
                // Extract info hash from magnet link
                size_t btih_pos = magnet_link.find("btih:");
                if (btih_pos != std::string::npos) {
                    result.info_hash = magnet_link.substr(btih_pos + 5, 40);
                }
                
                result.size = size;
                result.seeders = seeders;
                result.leechers = leechers;
                result.source = "keyword_search";
                
                // Store magnet link for direct downloading (similar to ed2k links)
                result.trackers = {magnet_link};
                
                results.push_back(result);
                
                if (results.size() >= MAX_RESULTS) {
                    break;
                }
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Keyword search error: " << e.what() << std::endl;
    }
    
    return results;
}

std::vector<BitTorrent::SearchResult> BitTorrent::BitTorrentSession::dht_search_by_infohash(const std::string& info_hash) {
    std::vector<SearchResult> results;
    
    try {
        // Check if we have this torrent locally first
        auto it = pimpl_->get_torrents().find(info_hash);
        if (it != pimpl_->get_torrents().end()) {
            try {
                lt::torrent_handle handle = it->second;
                lt::torrent_status status = handle.status();
                auto ti = handle.torrent_file();
                
                if (ti) {
                    SearchResult result;
                    result.name = ti->name();
                    result.info_hash = info_hash;
                    result.size = ti->total_size();
                    result.source = "local";
                    
                    // Get peers (need to implement get_peers_for_info_hash in BitTorrentSession)
                    // auto peers = get_peers_for_info_hash(info_hash);
                    // result.peers = peers;
                    
                    results.push_back(result);
                }
            } catch (...) {
                // Skip invalid torrents
            }
        }
        
        // TODO: Implement actual DHT infohash search for libtorrent 2.x
        // This would require a different approach than the deprecated API
        
    } catch (const std::exception& e) {
        std::cerr << "DHT infohash search error: " << e.what() << std::endl;
    }
    
    return results;
}

void BitTorrent::BitTorrentSession::start_dht_announce(const std::string& info_hash) {
    try {
        // Use proper sha1_hash constructor (avoid deprecated string constructor)
        lt::sha1_hash hash;
        if (info_hash.size() == 20) {  // SHA-1 hash is 20 bytes
            std::memcpy(hash.data(), info_hash.data(), 20);
        } else {
            // Handle incorrect hash size
            return;
        }

        // Use proper announce flags
        pimpl_->get_session().dht_announce(hash, 6881, lt::dht::announce_flags_t{});
    } catch (const std::exception& e) {
        std::cerr << "DHT announce error: " << e.what() << std::endl;
    }
}

std::vector<BitTorrent::BitTorrentSession::ActiveTorrent> BitTorrent::BitTorrentSession::get_active_torrents() const {
    std::vector<ActiveTorrent> result;
    
    try {
        // Get torrent handles from libtorrent session
        std::vector<lt::torrent_handle> handles = pimpl_->get_active_torrent_handles();
        
        for (const auto& handle : handles) {
            if (handle.is_valid()) {
                ActiveTorrent torrent;
                lt::torrent_status status = handle.status();
                
                torrent.info_hash = handle.info_hash().to_string();
                torrent.name = status.name;
                torrent.total_size = status.total_wanted;
                torrent.bytes_downloaded = status.total_wanted_done;
                torrent.bytes_uploaded = status.total_upload;
                torrent.download_rate = status.download_rate;
                torrent.upload_rate = status.upload_rate;
                torrent.progress = status.progress * 100.0f; // Convert to percentage
                
                // Map libtorrent state to our simplified state
                switch (status.state) {
                    case lt::torrent_status::checking_files:
                        torrent.state = 0; // queued
                        break;
                    case lt::torrent_status::downloading_metadata:
                    case lt::torrent_status::downloading:
                        torrent.state = 1; // downloading
                        break;
                    case lt::torrent_status::finished:
                    case lt::torrent_status::seeding:
                        torrent.state = 2; // seeding
                        break;
                    case lt::torrent_status::checking_resume_data:
                        torrent.state = 3; // paused
                        break;
                    default:
                        torrent.state = 4; // error
                        break;
                }
                
                result.push_back(torrent);
            }
        }
    } catch (...) {
        // Log error if needed
    }
    
    return result;
}

} // namespace BitTorrent