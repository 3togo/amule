#pragma once

#include <cstddef>
#include <cstdint>

// Essential constants that may still be referenced by the codebase
namespace BitTorrent {

// Protocol constants
constexpr uint16_t DEFAULT_PORT = 6881;
constexpr uint16_t PORT_RANGE_START = 6881;
constexpr uint16_t PORT_RANGE_END = 6889;
constexpr size_t MAX_PIECE_SIZE = 16 * 1024 * 1024; // 16MB
constexpr size_t DEFAULT_BLOCK_SIZE = 16 * 1024;    // 16KB
constexpr uint32_t PROTOCOL_TIMEOUT_MS = 30000;     // 30 seconds

// DHT constants
constexpr uint16_t DHT_DEFAULT_PORT = 6881;
constexpr size_t DHT_BUCKET_SIZE = 8;
constexpr uint32_t DHT_REFRESH_INTERVAL = 900000;   // 15 minutes

// Peer wire protocol flags
enum PeerFlags {
    PEER_DHT_SUPPORT = 0x01,
    PEER_FASTEXT_SUPPORT = 0x04,
    PEER_EXTENDED_SUPPORT = 0x10,
    PEER_UTP_SUPPORT = 0x100
};

} // namespace BitTorrent