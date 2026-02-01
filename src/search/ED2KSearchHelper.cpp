
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

#include "ED2KSearchHelper.h"
#include "SearchController.h"
#include "SearchTypeConverter.h"
#include "../amule.h"
#include "../ServerConnect.h"
#include "../Server.h"
#include "../SearchList.h"
#include "../Packet.h"
#include "../MemFile.h"
#include "../OtherFunctions.h"
#include <wx/utils.h>

namespace search {

bool ED2KSearchHelper::CanPerformSearch()
{
	return theApp && theApp->IsConnectedED2K();
}

bool ED2KSearchHelper::SupportsLargeFiles()
{
	if (!theApp || !theApp->serverconnect) {
		return false;
	}

	CServer* server = theApp->serverconnect->GetCurrentServer();
	return server != NULL && (server->GetTCPFlags() & SRV_TCPFLG_LARGEFILES);
}

bool ED2KSearchHelper::CreateSearchPacket(const SearchParams& params, bool isLocalSearch,
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

	// Determine search type
	::SearchType type = SearchTypeConverter::toLegacy(searchType);

	// Check if server supports 64-bit
	bool supports64bit = SupportsLargeFiles();
	bool packetUsing64bit = false;

	// For now, we use SearchList's StartNewSearch method
	// This is temporary during migration
	// We'll implement proper packet creation in Phase 3
	return false;
}

bool ED2KSearchHelper::SendSearchPacket(const uint8_t* packetData, uint32_t packetSize,
					 bool isLocalSearch)
{
	// For now, we use SearchList's StartNewSearch method
	// This is temporary during migration
	return false;
}

void ED2KSearchHelper::FreeSearchPacket(uint8_t* packetData)
{
	if (packetData) {
		delete[] packetData;
	}
}

} // namespace search
