
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
#include "SearchController.h"
#include "../SearchList.h"
#include "../amule.h"
#include "../MemFile.h"
#include <wx/utils.h>

namespace search {

bool KadSearchPacketBuilder::CreateSearchPacket(const SearchParams& params,
						uint8_t*& packetData, uint32_t& packetSize)
{
    if (!theApp || !theApp->searchlist) {
	return false;
    }

    // Convert to old parameter format
    CSearchList::CSearchParams oldParams;
    oldParams.searchString = params.searchString;
    oldParams.strKeyword = params.strKeyword;
    oldParams.typeText = params.typeText;
    oldParams.extension = params.extension;
    oldParams.minSize = params.minSize;
    oldParams.maxSize = params.maxSize;
    oldParams.availability = params.availability;

    // Use SearchList's CreateSearchData method
    bool packetUsing64bit = false;
    CSearchList::CMemFilePtr data = theApp->searchlist->CreateSearchData(
	oldParams, ::KadSearch, true, packetUsing64bit);

    if (data.get() == NULL) {
	return false;
    }

    // Store packet data
    packetSize = data->GetLength();
    packetData = new uint8_t[packetSize];
    memcpy(packetData, data->GetRawBuffer(), packetSize);

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
    // For now, we use SearchList's CreateSearchData method
    // This is temporary during migration
    // We'll implement proper packet encoding in Phase 3
    return false;
}

} // namespace search
