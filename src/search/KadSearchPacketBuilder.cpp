
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

#include "KadSearchPacketBuilder.h"
#include "../MemFile.h"
#include "../protocol/kad2/Client2Client/UDP.h"
#include <wx/string.h>

namespace search {

bool KadSearchPacketBuilder::CreateSearchPacket(const SearchParams& params,
						uint8_t*& packetData, uint32_t& packetSize)
{
    // Validate input parameters
    if (params.strKeyword.IsEmpty()) {
        return false;
    }

    // Create packet using MemFile
    CMemFile packet;
    
    // Add operation code for Kademlia2 search
    packet.WriteUInt8(KADEMLIA2_SEARCH_KEY_REQ);
    
    // Add search keyword
    packet.WriteString(params.strKeyword);
    
    // Add additional search parameters if needed
    // For now, keep it simple with just the keyword
    
    // Get the final packet data
    packetSize = packet.GetLength();
    packetData = new uint8_t[packetSize];
    memcpy(packetData, packet.GetBuffer(), packetSize);
    
    return true;
}

void KadSearchPacketBuilder::FreeSearchPacket(uint8_t* packetData)
{
    if (packetData) {
	delete[] packetData;
    }
}

bool KadSearchPacketBuilder::EncodeSearchParams(const SearchParams& params,
					       uint8_t*& packetData, uint32_t& packetSize)
{
    // Now implemented in CreateSearchPacket
    // This method can be used for future extensions
    return CreateSearchPacket(params, packetData, packetSize);
}

} // namespace search
