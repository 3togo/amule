
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
#include "../kademlia/utils/KadUDPKey.h"
#include "../include/protocol/kad2/Client2Client/UDP.h"
#include "../MemFile.h"
#include "SearchModel.h"
#include <wx/string.h>

namespace search {

bool KadSearchPacketBuilder::CreateSearchPacket(const SearchParams& params,
                                               uint8_t*& packetData, uint32_t& packetSize)
{
    // For Kad searches, we don't actually create a packet here
    // The Kad network handles search requests through its own API
    // This method is kept for consistency with the interface
    
    // Return empty packet - Kad controller will handle the actual search
    packetData = nullptr;
    packetSize = 0;
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
