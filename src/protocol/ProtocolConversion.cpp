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
#include "../MD4Hash.h"
#include <openssl/sha.h>

// Removed BitTorrent namespace and related conversions

namespace ProtocolIntegration {

// Only keeping ED2K-related conversions
std::unique_ptr<CPacket> convert_ed2k_to_local_format(const CPacket* ed2k_packet) {
    // Return the packet as-is for local use
    return std::make_unique<CPacket>(*ed2k_packet);
}

std::unique_ptr<CPacket> convert_for_local_processing(const CPacket* packet) {
    // Generic conversion for local processing
    return std::make_unique<CPacket>(*packet);
}

} // namespace ProtocolIntegration