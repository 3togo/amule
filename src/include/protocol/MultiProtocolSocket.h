#pragma once

#include "protocol/Protocols.h"
#include "protocol/ed2k/Constants.h"
#include "protocol/kad/Constants.h"
#include "protocol/bt/Constants.h"
#include "../LibSocket.h"
#include "../Packet.h"
#include "../MD4Hash.h"
#include <memory>
#include <vector>
class CUpDownClient;
class CServer;

namespace MultiProtocol {

enum class SocketProtocol {
    ED2K_TCP,
    ED2K_UDP, 
    KAD_UDP,
    BT_TCP,
    BT_UDP,
    HYBRID_AUTO
};

class MultiProtocolSocket : public CLibSocket {
public:
    MultiProtocolSocket(SocketProtocol protocol = SocketProtocol::HYBRID_AUTO);
    virtual ~MultiProtocolSocket();
    
    // Protocol negotiation and handshake
    bool protocol_handshake();
    bool supports_protocol(SocketProtocol protocol) const;
    SocketProtocol get_negotiated_protocol() const;
    
    // Packet processing
    bool process_packet(CPacket* packet);
    bool send_packet(CPacket* packet, bool delpacket = true);
    
    // Cross-protocol packet conversion
    static std::unique_ptr<CPacket> convert_ed2k_to_bt(const CPacket* ed2k_packet);
    static std::unique_ptr<CPacket> convert_bt_to_ed2k(const CPacket* bt_packet);
    
    // Protocol-specific operations
    bool setup_bt_connection(const std::string& info_hash, uint16_t port = 6881);
    bool setup_ed2k_connection(const CMD4Hash& file_hash, uint16_t port = 4662);
    bool setup_kad_connection(uint16_t port = 4672);
    
    // Bandwidth management
    void set_protocol_bandwidth(SocketProtocol protocol, uint32_t max_kbps);
    uint32_t get_protocol_bandwidth(SocketProtocol protocol) const;
    
    // Statistics
    struct SocketStats {
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint32_t packets_sent;
        uint32_t packets_received;
        double current_throughput_kbps;
        SocketProtocol active_protocol;
    };
    
    SocketStats get_socket_stats() const;
    
    // Error handling and recovery
    bool can_fallback_protocol() const;
    bool switch_protocol(SocketProtocol new_protocol);
    
protected:
    virtual void OnConnect(int nErrorCode) override;
    virtual void OnReceive(int nErrorCode) override;
    virtual void OnSend(int nErrorCode) override;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
    
    // Disable copying
    MultiProtocolSocket(const MultiProtocolSocket&) = delete;
    MultiProtocolSocket& operator=(const MultiProtocolSocket&) = delete;
};

// Factory functions
std::unique_ptr<MultiProtocolSocket> create_protocol_socket(
    SocketProtocol protocol,
    const std::string& remote_address,
    uint16_t remote_port);

std::unique_ptr<MultiProtocolSocket> create_hybrid_socket(
    const std::vector<SocketProtocol>& supported_protocols,
    const std::string& remote_address,
    uint16_t remote_port);

// Protocol detection
SocketProtocol detect_protocol_from_packet(const CPacket* packet);
SocketProtocol negotiate_best_protocol(
    const std::vector<SocketProtocol>& client_protocols,
    const std::vector<SocketProtocol>& server_protocols);

} // namespace MultiProtocol