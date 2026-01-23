#pragma once

#include <cstddef>
#include <cstdint>

// BitTorrent Protocol Constants
namespace BitTorrent {

// Protocol identifier
constexpr const char* PROTOCOL_IDENTIFIER = "BitTorrent protocol";

// Message types
enum MessageType : uint8_t {
    MSG_CHOKE = 0,
    MSG_UNCHOKE = 1,
    MSG_INTERESTED = 2,
    MSG_NOT_INTERESTED = 3,
    MSG_HAVE = 4,
    MSG_BITFIELD = 5,
    MSG_REQUEST = 6,
    MSG_PIECE = 7,
    MSG_CANCEL = 8,
    MSG_PORT = 9,          // DHT port
    MSG_EXTENDED = 20      // Extension protocol
};

// Extension protocol messages
enum ExtendedMessage : uint8_t {
    EXT_HANDSHAKE = 0,
    EXT_UT_PEX = 1,        // Peer Exchange
    EXT_UT_METADATA = 2,   // Metadata exchange
    EXT_UT_DHT = 3         // DHT support
};

// Tracker event types
enum TrackerEvent : uint8_t {
    TRACKER_EVENT_NONE = 0,
    TRACKER_EVENT_COMPLETED = 1,
    TRACKER_EVENT_STARTED = 2,
    TRACKER_EVENT_STOPPED = 3
};

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