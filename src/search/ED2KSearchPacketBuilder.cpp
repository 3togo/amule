//
// This file is part of the aMule Project.
//
// Copyright (c) 2026 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "ED2KSearchPacketBuilder.h"
#include "../MemFile.h"
#include "../include/protocol/ed2k/Client2Server/TCP.h"

namespace search {

bool ED2KSearchPacketBuilder::CreateSearchPacket(const SearchParams& params,
                                                uint8_t*& packetData, uint32_t& packetSize)
{
    if (params.strKeyword.IsEmpty()) {
        return false;
    }
    
    return EncodeSearchParams(params, packetData, packetSize);
}

void ED2KSearchPacketBuilder::FreeSearchPacket(uint8_t* packetData)
{
    delete[] packetData;
}

bool ED2KSearchPacketBuilder::EncodeSearchParams(const SearchParams& params,
                                                uint8_t*& packetData, uint32_t& packetSize)
{
    CMemFile packet;
    
    // Write packet opcode
    packet.WriteUInt8(OP_SEARCH_USER);
    
    // Write search string
    packet.WriteString(params.strKeyword);
    
    // Get packet data
    packetSize = packet.GetLength();
    packetData = new uint8_t[packetSize];
    memcpy(packetData, packet.GetRawBuffer(), packetSize);
    
    return true;
}

} // namespace search