#pragma once

#include "protocol/Protocols.h"
#include "protocol/ed2k/Constants.h"
#include "protocol/kad/Constants.h"
#include "protocol/bt/Constants.h"
#include "../../MD4Hash.h"
#include "../../common/NetworkPerformanceMonitor.h"
#include <vector>
#include <string>
#include <memory>

// Forward declarations
class CPartFile;
class CKnownFile;
class CUpDownClient;
class CServer;

namespace ProtocolIntegration {

// Network condition metrics for protocol selection
struct NetworkConditions {
    double bandwidth_kbps;          // Available bandwidth in kbps
    uint32_t latency_ms;            // Network latency in milliseconds
    double packet_loss_rate;        // Packet loss rate (0.0 to 1.0)
    uint32_t connection_stability;  // Connection stability score (0-100)
    bool supports_nat_traversal;    // Supports NAT traversal techniques
    bool high_bandwidth_mode;       // High bandwidth mode available
};

enum class ProtocolType {
    ED2K,
    KADEMLIA,
    BITTORRENT,
    HYBRID_AUTO
};

struct SourceEndpoint {
    ProtocolType protocol;
    std::string address;
    uint16_t port;
    double reliability_score;
    double bandwidth_estimate;
    uint32_t latency_ms;
    
    // Cross-protocol metadata
    std::string info_hash;      // For BitTorrent
    CMD4Hash ed2k_hash;         // For ED2K
    bool supports_hybrid;       // Supports cross-protocol transfers
    
    bool operator==(const SourceEndpoint& other) const;
    bool is_duplicate(const SourceEndpoint& other) const;
};

class ProtocolCoordinator {
public:
    static ProtocolCoordinator& instance();
    
    // Source discovery and management
    std::vector<SourceEndpoint> discover_sources(
        const CPartFile* file, 
        ProtocolType preferred = ProtocolType::HYBRID_AUTO,
        uint32_t max_sources = 50);
    
    std::vector<SourceEndpoint> find_cross_protocol_sources(
        const CPartFile* ed2k_file, 
        const std::string& bt_info_hash);
    
    bool add_source(const SourceEndpoint& source, CPartFile* file);
    bool remove_duplicate_sources(CPartFile* file);
    
    // Protocol selection and optimization
    ProtocolType select_optimal_protocol(
        const CPartFile* file,
        const NetworkConditions& conditions) const;
    
    bool should_switch_protocol(
        const CPartFile* file,
        ProtocolType current,
        ProtocolType proposed) const;
    
    // Bandwidth management
    struct BandwidthAllocation {
        uint32_t ed2k_download_kbps;
        uint32_t ed2k_upload_kbps;
        uint32_t bt_download_kbps;
        uint32_t bt_upload_kbps;
        uint32_t kad_download_kbps;
        uint32_t kad_upload_kbps;
    };
    
    BandwidthAllocation calculate_bandwidth_allocation() const;
    void apply_bandwidth_allocation(const BandwidthAllocation& allocation);
    
    // Cross-protocol metadata conversion
    bool convert_metadata_ed2k_to_bt(const CPartFile* ed2k_file, std::string& bt_metadata);
    bool convert_metadata_bt_to_ed2k(const std::string& bt_metadata, CPartFile* ed2k_file);
    
    // Statistics and monitoring
    struct CoordinationStats {
        uint32_t total_sources_discovered;
        uint32_t cross_protocol_sources;
        uint32_t protocol_switches;
        uint32_t duplicate_sources_removed;
        double avg_discovery_time_ms;
        double cross_protocol_success_rate;
    };
    
    CoordinationStats get_stats() const;
    void reset_stats();
    
    // Configuration
    void set_hybrid_mode_enabled(bool enabled);
    bool is_hybrid_mode_enabled() const;
    
    void set_max_cross_protocol_sources(uint32_t max);
    uint32_t get_max_cross_protocol_sources() const;
    
private:
    ProtocolCoordinator();
    ~ProtocolCoordinator();
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Disable copying
    ProtocolCoordinator(const ProtocolCoordinator&) = delete;
    ProtocolCoordinator& operator=(const ProtocolCoordinator&) = delete;
};

// Helper functions
bool is_hybrid_transfer_supported(const SourceEndpoint& source);
double calculate_protocol_efficiency(ProtocolType protocol, const NetworkConditions& conditions);
SourceEndpoint create_hybrid_endpoint(const SourceEndpoint& ed2k_source, const SourceEndpoint& bt_source);

} // namespace ProtocolIntegration