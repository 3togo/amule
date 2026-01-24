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
#include "protocol/ProtocolConversion.h"
#include "protocol/ed2k/Constants.h"
#include "protocol/ed2k/Client2Client/TCP.h"
#include "protocol/bt/Constants.h"
#include "../MD4Hash.h"
#include <openssl/sha.h>

namespace BitTorrent {

std::string ed2k_hash_to_info_hash(const CMD4Hash& ed2k_hash) {
    // Convert ED2K MD4 hash to BitTorrent SHA-1 info hash
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1(ed2k_hash.GetHash(), 16, sha1_hash); // MD4 hash is always 16 bytes
    
    return std::string(reinterpret_cast<char*>(sha1_hash), SHA_DIGEST_LENGTH);
}

} // namespace BitTorrent

namespace ProtocolIntegration {

CMD4Hash info_hash_to_ed2k_hash(const std::string& info_hash) {
    // Convert BitTorrent SHA-1 info hash to ED2K MD4 hash
    CMD4Hash ed2k_hash;
    // Note: This is a simplified conversion - MD4 hash of SHA1 hash
    // In practice, you might need a different approach for cross-protocol hash mapping
    SHA1(reinterpret_cast<const unsigned char*>(info_hash.data()), 
        info_hash.size(), 
        ed2k_hash.GetHash());
    
    return ed2k_hash;
}

std::unique_ptr<CPacket> convert_ed2k_to_bt(const CPacket* ed2k_packet) {
    // Convert ED2K packet to BitTorrent protocol
    auto bt_packet = std::make_unique<CPacket>(OP_BITTORRENTHEADER);
    
    switch(ed2k_packet->GetOpCode()) {
        case OP_REQUESTPARTS:
            // Convert ED2K part request to BT request
            bt_packet->SetOpCode(BitTorrent::MSG_REQUEST);
            // ... conversion logic ...
            break;
            
        case OP_SENDINGPART:
            // Convert ED2K part data to BT piece
            bt_packet->SetOpCode(BitTorrent::MSG_PIECE);
            // ... conversion logic ...
            break;
            
        default:
            return nullptr;
    }
    
    return bt_packet;
}

std::unique_ptr<CPacket> convert_bt_to_ed2k(const CPacket* bt_packet) {
    // Convert BitTorrent packet to ED2K protocol
    auto ed2k_packet = std::make_unique<CPacket>(OP_EDONKEYPROT);
    
    switch(bt_packet->GetOpCode()) {
        case BitTorrent::MSG_REQUEST:
            // Convert BT request to ED2K part request
            ed2k_packet->SetOpCode(OP_REQUESTPARTS);
            // ... conversion logic ...
            break;
            
        case BitTorrent::MSG_PIECE:
            // Convert BT piece to ED2K part data
            ed2k_packet->SetOpCode(OP_SENDINGPART);
            // ... conversion logic ...
            break;
            
        default:
            return nullptr;
    }
    
    return ed2k_packet;
}

} // namespace ProtocolIntegration