#include "protocol/bt/BitTorrentSession.h"
#include "protocol/ProtocolCoordinator.h"
#include "../../common/NetworkPerformanceMonitor.h"
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/magnet_uri.hpp>

namespace BitTorrent {

class BitTorrentSession::Impl {
private:
    lt::session m_session;
    std::vector<lt::torrent_handle> m_torrents;
    
public:
    Impl() {
        lt::settings_pack settings;
        settings.set_int(lt::settings_pack::alert_mask, 
            lt::alert::status_notification | 
            lt::alert::error_notification);
        
        m_session.apply_settings(settings);
    }
    
    bool add_torrent(const std::string& torrent_file, const std::string& save_path) {
        try {
            lt::add_torrent_params params;
            params.ti = std::make_shared<lt::torrent_info>(torrent_file);
            params.save_path = save_path;
            
            auto handle = m_session.add_torrent(params);
            m_torrents.push_back(handle);
            
            // Track new torrent in performance monitor
            network_perf::g_network_perf_monitor.record_bt_tracker_active();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool add_magnet_link(const std::string& magnet_link, const std::string& save_path) {
        try {
            lt::add_torrent_params params = lt::parse_magnet_uri(magnet_link);
            params.save_path = save_path;
            
            auto handle = m_session.add_torrent(params);
            m_torrents.push_back(handle);
            
            network_perf::g_network_perf_monitor.record_bt_tracker_active();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // ...其他实现方法...
};

// Singleton instance
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