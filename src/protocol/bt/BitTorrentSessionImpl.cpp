// =============================================================================
// BitTorrent Session Implementation for libtorrent 2.0.11
// IMPORTANT: Read this before making changes!
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
constexpr int DHT_SEARCH_TIMEOUT_MS = 5000;

} // anonymous namespace

std::vector<BitTorrent::SearchResult> BitTorrent::BitTorrentSession::dht_search(const std::string& keyword) {
    std::vector<SearchResult> results;
    
    try {
        // libtorrent 2.x DHT search approach
        // We need to use a different method since direct DHT search API changed
        
        // For now, use a placeholder that searches through existing torrents
        // In a real implementation, you'd use external torrent search APIs
        // or implement a proper DHT crawler
        
        std::cerr << "DHT keyword search placeholder - would use external API in production" << std::endl;
        
        // Placeholder: Return any torrents that contain the keyword in their name
        for (const auto& pair : pimpl_->get_torrents()) {
            try {
                lt::torrent_handle handle = pair.second;
                lt::torrent_status status = handle.status();
                auto ti = handle.torrent_file();
                
                if (ti && ti->name().find(keyword) != std::string::npos) {
                    SearchResult result;
                    result.name = ti->name();
                    result.info_hash = pair.first;
                    result.size = ti->total_size();
                    result.source = "local";
                    
                    results.push_back(result);
                    
                    if (results.size() >= MAX_SEARCH_RESULTS) {
                        break;
                    }
                }
            } catch (...) {
                // Skip invalid torrents
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "DHT search error: " << e.what() << std::endl;
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

} // namespace BitTorrent