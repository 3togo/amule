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
    
    bool add_torrent(const std::string& torrent_file, const std::string& save_path) {
        try {
            // Load torrent file
            lt::torrent_info ti(torrent_file);
            
            // Create add torrent parameters
            lt::add_torrent_params params;
            params.ti = std::make_shared<lt::torrent_info>(ti);
            params.save_path = save_path;
            
            // Add torrent to session
            lt::torrent_handle handle = m_session.add_torrent(params);
            
            // Store torrent handle with info hash as key
            std::string info_hash = ti.info_hash().to_string();
            m_torrents[info_hash] = handle;
            
            // Track new torrent in performance monitor
            network_perf::g_network_perf_monitor.record_bt_tracker_active();
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to add torrent: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool add_magnet_link(const std::string& magnet_link, const std::string& save_path) {
        try {
            // Parse magnet URI
            lt::add_torrent_params params = lt::parse_magnet_uri(magnet_link);
            params.save_path = save_path;
            
            // Add torrent to session
            lt::torrent_handle handle = m_session.add_torrent(params);
            
            // Store torrent handle with info hash as key
            std::string info_hash = params.info_hashes.v1.to_string();
            m_torrents[info_hash] = handle;
            
            network_perf::g_network_perf_monitor.record_bt_tracker_active();
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to add magnet link: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool remove_torrent(const std::string& info_hash, bool remove_data) {
        try {
            auto it = m_torrents.find(info_hash);
            if (it == m_torrents.end()) {
                return false;
            }
            
            lt::torrent_handle handle = it->second;
            
            // Remove from session
            m_session.remove_torrent(handle, 
                remove_data ? SESSION_DELETE_FILES : SESSION_NONE);
            
            // Remove from map
            m_torrents.erase(it);
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to remove torrent: " << e.what() << std::endl;
            return false;
        }
    }
    
    std::vector<std::pair<std::string, uint16_t>> get_peers_for_info_hash(
        const std::string& info_hash) {
        
        std::vector<std::pair<std::string, uint16_t>> peers;
        
        auto it = m_torrents.find(info_hash);
        if (it == m_torrents.end()) {
            return peers;
        }
        
        try {
            lt::torrent_handle handle = it->second;
            
            // Get peer list
            std::vector<lt::peer_info> peer_list;
            handle.get_peer_info(peer_list);
            
            for (const auto& peer : peer_list) {
                // libtorrent 2.0.11 peer_info API usage:
                // - peer.ip is a tcp::endpoint MEMBER (not a method)
                // - peer.ip.address() returns the address
                // - peer.ip.port() returns the port
                // Note: peer.connecting is a bitfield_flag, NOT a pointer!
                try {
                    // Access peer.ip as a member, not a method
                    const auto& ip = peer.ip;  // tcp::endpoint
                    if (ip.address().is_v4()) {
                        auto addr = ip.address().to_v4();
                        peers.push_back(std::make_pair(
                            addr.to_string(),
                            static_cast<uint16_t>(ip.port())
                        ));
                    }
                } catch (const std::exception& e) {
                    // Skip incompatible peers
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to get peers: " << e.what() << std::endl;
        }
        
        return peers;
    }
    
    SessionStats get_session_stats() const {
        SessionStats stats{};
        
        stats.total_downloaded = 0;
        stats.total_uploaded = 0;
        stats.active_torrents = m_torrents.size();
        stats.connected_peers = 0;
        stats.active_trackers = 0;
        stats.download_rate = 0.0;
        stats.upload_rate = 0.0;
        
        for (const auto& pair : m_torrents) {
            try {
                lt::torrent_handle handle = pair.second;
                lt::torrent_status status = handle.status();
                
                stats.total_downloaded += status.total_download;
                stats.total_uploaded += status.total_upload;
                stats.download_rate += status.download_rate;
                stats.upload_rate += status.upload_rate;
                
                // Count connected peers
                if (status.state == lt::torrent_status::downloading ||
                    status.state == lt::torrent_status::seeding) {
                    stats.connected_peers += status.num_peers;
                }
                
                // Check if tracker is active (libtorrent 2.0+)
                auto trackers = handle.trackers();
                if (!trackers.empty()) {
                    stats.active_trackers++;
                }
            } catch (...) {
                // Skip invalid torrents
            }
        }
        
        return stats;
    }
    
    // Cross-protocol integration
    std::vector<std::shared_ptr<CUpDownClient>> find_cross_protocol_sources(
        const CPartFile* ed2k_file, 
        const CKnownFile* known_file) {
        
        std::vector<std::shared_ptr<CUpDownClient>> sources;
        
        // If we have a matching torrent, get its peers
        if (ed2k_file) {
            std::string bt_hash = BitTorrent::ed2k_hash_to_info_hash(ed2k_file->GetFileHash());
            auto peers = get_peers_for_info_hash(bt_hash);
            
            // Convert peers to CUpDownClient objects
            for (const auto& peer : peers) {
                // TODO: Implement proper conversion
                // This is a placeholder for the actual implementation
            }
        }
        
        return sources;
    }
    
    bool enable_dht_integration(bool enable) {
        m_dht_enabled = enable;
        
        lt::settings_pack settings;
        settings.set_bool(lt::settings_pack::enable_dht, enable);
        m_session.apply_settings(settings);
        
        if (enable) {
            // DHT is now managed automatically by libtorrent
            // based on the enable_dht setting
        }
        
        return true;
    }
    
    bool share_dht_routing_table() {
        // TODO: Implement DHT routing table sharing with Kademlia
        return false;
    }
    
    std::vector<std::string> get_shared_peers() const {
        std::vector<std::string> peers;
        
        for (const auto& pair : m_torrents) {
            try {
                lt::torrent_handle handle = pair.second;
                lt::torrent_status status = handle.status();
                
                // Get trackers (libtorrent 2.0+)
                auto trackers = handle.trackers();
                for (const auto& tracker : trackers) {
                    if (!tracker.url.empty()) {
                        peers.push_back(tracker.url);
                    }
                }
            } catch (...) {
                // Skip invalid torrents
            }
        }
        
        return peers;
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
    return pimpl_->add_torrent(torrent_file, save_path);
}

bool BitTorrentSession::add_magnet_link(const std::string& magnet_link, const std::string& save_path) {
    return pimpl_->add_magnet_link(magnet_link, save_path);
}

} // namespace BitTorrent