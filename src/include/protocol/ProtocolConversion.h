#pragma once

#include "protocol/Protocols.h"
#include "../../MD4Hash.h"
#include "../../Packet.h"
#include <string>
#include <memory>

namespace ProtocolIntegration {

// Hash conversion functions
std::string ed2k_hash_to_info_hash(const CMD4Hash& ed2k_hash);
CMD4Hash info_hash_to_ed2k_hash(const std::string& info_hash);

// Packet conversion functions
std::unique_ptr<CPacket> convert_ed2k_to_bt(const CPacket* ed2k_packet);
std::unique_ptr<CPacket> convert_bt_to_ed2k(const CPacket* bt_packet);

} // namespace ProtocolIntegration