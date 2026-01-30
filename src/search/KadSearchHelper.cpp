
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

#include "KadSearchHelper.h"
#include "SearchController.h"
#include "../amule.h"
#include "../SearchList.h"
#include "../MemFile.h"
#include <wx/utils.h>

// Kad headers
#include "../kademlia/kademlia/Kademlia.h"
#include "../kademlia/kademlia/SearchManager.h"
#include "../kademlia/kademlia/Search.h"

namespace search {

bool KadSearchHelper::CanPerformSearch()
{
	return Kademlia::CKademlia::IsRunning();
}

bool KadSearchHelper::ExtractKeyword(const wxString& searchString, wxString& keyword)
{
	if (!theApp || !theApp->searchlist) {
		return false;
	}

	// Use SearchList's word extraction (temporary)
	Kademlia::WordList words;
	Kademlia::CSearchManager::GetWords(searchString, &words);

	if (words.empty()) {
		return false;
	}

	keyword = words.front();
	return true;
}

bool KadSearchHelper::CreateSearchPacket(const SearchParams& params,
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

	// For now, we use SearchList's StartNewSearch method
	// This is temporary during migration
	return false;
}

bool KadSearchHelper::StartSearch(const uint8_t* packetData, uint32_t packetSize,
				  const wxString& keyword, uint32_t& searchId)
{
	if (!Kademlia::CKademlia::IsRunning() || !packetData) {
		return false;
	}

	// For now, we use SearchList's StartNewSearch method
	// This is temporary during migration
	return false;
}

bool KadSearchHelper::StopSearch(uint32_t searchId)
{
	if (!Kademlia::CKademlia::IsRunning()) {
		return false;
	}

	// For now, we use SearchList's StopSearch method
	// This is temporary during migration
	return false;
}

void KadSearchHelper::FreeSearchPacket(uint8_t* packetData)
{
	if (packetData) {
		delete[] packetData;
	}
}

} // namespace search
