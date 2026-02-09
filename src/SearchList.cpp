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

#include "SearchList.h"		// Interface declarations.
#include "search/SearchAutoRetry.h"	// Auto-retry manager
#include "search/SearchPackageValidator.h"	// Package validator
#include "search/SearchPackageException.h"	// Package exception
#include "search/SearchResultHandler.h"	// Result handler interface
#include "search/SearchResultRouter.h"	// Result router
#include "search/PerSearchState.h"	// Per-search state management
#include "search/SearchIdGenerator.h"	// Search ID generation

#include "include/common/MacrosProgramSpecific.h"	// Needed for NOT_ON_REMOTEGUI

#include <protocol/Protocols.h>
#include <protocol/kad/Constants.h>
#include <tags/ClientTags.h>
#include <tags/FileTags.h>

#include "updownclient.h"	// Needed for CUpDownClient
#include "MemFile.h"		// Needed for CMemFile
#include "amule.h"			// Needed for theApp
#include "ServerConnect.h"	// Needed for theApp->serverconnect
#include "Server.h"			// Needed for CServer
#include "ServerList.h"		// Needed for theApp->serverlist
#include "Statistics.h"		// Needed for theStats
#include "ObservableQueue.h"// Needed for CQueueObserver
#include <common/Format.h>
#include "Logger.h"			// Needed for AddLogLineM/...
#include "Packet.h"			// Needed for CPacket
#include "GuiEvents.h"		// Needed for Notify_*


#ifndef AMULE_DAEMON
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "SearchDlg.h"		// Needed for CSearchDlg
#endif

#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Search.h"

#include "SearchExpr.h"

#include "Scanner.h"
void LexInit(const wxString& pszInput);
void LexFree();

#include "Parser.hpp"
int yyerror(wxString errstr);


static wxString s_strCurKadKeyword;

static CSearchExpr _SearchExpr;

wxArrayString _astrParserErrors;


// Helper function for lexer.
void ParsedSearchExpression(const CSearchExpr* pexpr)
{
	int iOpAnd = 0;
	int iOpOr = 0;
	int iOpNot = 0;

	for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
		const wxString& str = pexpr->m_aExpr[i];
		if (str == SEARCHOPTOK_AND) {
			iOpAnd++;
		} else if (str == SEARCHOPTOK_OR) {
			iOpOr++;
		} else if (str == SEARCHOPTOK_NOT) {
			iOpNot++;
		}
	}

	// this limit (+ the additional operators which will be added later) has to match the limit in 'CreateSearchExpressionTree'
	//	+1 Type (Audio, Video)
	//	+1 MinSize
	//	+1 MaxSize
	//	+1 Avail
	//	+1 Extension
	//	+1 Complete sources
	//	+1 Codec
	//	+1 Bitrate
	//	+1 Length
	//	+1 Title
	//	+1 Album
	//	+1 Artist
	// ---------------
	//  12
	if (iOpAnd + iOpOr + iOpNot > 10) {
		yyerror(wxT("Search expression is too complex"));
	}

	_SearchExpr.m_aExpr.Empty();

	// optimize search expression, if no OR nor NOT specified
	if (iOpAnd > 0 && iOpOr == 0 && iOpNot == 0) {
		// figure out if we can use a better keyword than the one the user selected
		// for example most user will search like this "The oxymoronaccelerator 2", which would ask the node which indexes "the"
		// This causes higher traffic for such nodes and makes them a viable target to attackers, while the kad result should be
		// the same or even better if we ask the node which indexes the rare keyword "oxymoronaccelerator", so we try to rearrange
		// keywords and generally assume that the longer keywords are rarer
		if (/*thePrefs::GetRearrangeKadSearchKeywords() &&*/ !s_strCurKadKeyword.IsEmpty()) {
			for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
				if (pexpr->m_aExpr[i] != SEARCHOPTOK_AND) {
					if (pexpr->m_aExpr[i] != s_strCurKadKeyword
						&& pexpr->m_aExpr[i].find_first_of(Kademlia::CSearchManager::GetInvalidKeywordChars()) == wxString::npos
						&& pexpr->m_aExpr[i].Find('"') != 0 // no quoted expressions as keyword
						&& pexpr->m_aExpr[i].length() >= 3
						&& s_strCurKadKeyword.length() < pexpr->m_aExpr[i].length())
					{
						s_strCurKadKeyword = pexpr->m_aExpr[i];
					}
				}
			}
		}
		wxString strAndTerms;
		for (unsigned int i = 0; i < pexpr->m_aExpr.GetCount(); i++) {
			if (pexpr->m_aExpr[i] != SEARCHOPTOK_AND) {
				// Minor optimization: Because we added the Kad keyword to the boolean search expression,
				// we remove it here (and only here) again because we know that the entire search expression
				// does only contain (implicit) ANDed strings.
				if (pexpr->m_aExpr[i] != s_strCurKadKeyword) {
					if (!strAndTerms.IsEmpty()) {
						strAndTerms += ' ';
					}
					strAndTerms += pexpr->m_aExpr[i];
				}
			}
		}
		wxASSERT( _SearchExpr.m_aExpr.GetCount() == 0);
		_SearchExpr.m_aExpr.Add(strAndTerms);
	} else {
		if (pexpr->m_aExpr.GetCount() != 1 || pexpr->m_aExpr[0] != s_strCurKadKeyword)
			_SearchExpr.Add(pexpr);
	}
}


//! Helper class for packet creation
class CSearchExprTarget
{
public:
	CSearchExprTarget(CMemFile* pData, EUtf8Str eStrEncode, bool supports64bit, bool& using64bit)
		: m_data(pData),
		  m_eStrEncode(eStrEncode),
		  m_supports64bit(supports64bit),
		  m_using64bit(using64bit)
	{
		m_using64bit = false;
	}

	void WriteBooleanAND()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x00);			// "AND"
	}

	void WriteBooleanOR()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x01);			// "OR"
	}

	void WriteBooleanNOT()
	{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x02);			// "NOT"
	}

	void WriteMetaDataSearchParam(const wxString& rstrValue)
	{
		m_data->WriteUInt8(1);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
	}

	void WriteMetaDataSearchParam(uint8 uMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParamASCII(uint8 uMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, utf8strNone); // string value
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParam(const wxString& pszMetaTagID, const wxString& rstrValue)
	{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteString(pszMetaTagID);	// meta tag ID
	}

	void WriteMetaDataSearchParam(uint8_t uMetaTagID, uint8_t uOperator, uint64_t value)
	{
		bool largeValue = value > wxULL(0xFFFFFFFF);
		if (largeValue && m_supports64bit) {
			m_using64bit = true;
			m_data->WriteUInt8(8);		// numeric parameter type (int64)
			m_data->WriteUInt64(value);	// numeric value
		} else {
			if (largeValue) {
				value = 0xFFFFFFFFu;
			}
			m_data->WriteUInt8(3);		// numeric parameter type (int32)
			m_data->WriteUInt32(value);	// numeric value
		}
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteUInt16(sizeof(uint8));	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);		// meta tag ID name
	}

	void WriteMetaDataSearchParam(const wxString& pszMetaTagID, uint8_t uOperator, uint64_t value)
	{
		bool largeValue = value > wxULL(0xFFFFFFFF);
		if (largeValue && m_supports64bit) {
			m_using64bit = true;
			m_data->WriteUInt8(8);		// numeric parameter type (int64)
			m_data->WriteUInt64(value);	// numeric value
		} else {
			if (largeValue) {
				value = 0xFFFFFFFFu;
			}
			m_data->WriteUInt8(3);		// numeric parameter type (int32)
			m_data->WriteUInt32(value);	// numeric value
		}
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteString(pszMetaTagID);	// meta tag ID
	}

protected:
	CMemFile* m_data;
	EUtf8Str m_eStrEncode;
	bool m_supports64bit;
	bool& m_using64bit;
};




///////////////////////////////////////////////////////////
// CSearchList

BEGIN_EVENT_TABLE(CSearchList, wxEvtHandler)
	EVT_MULE_TIMER(wxID_ANY, CSearchList::OnGlobalSearchTimer)
END_EVENT_TABLE()


CSearchList::CSearchList()
{
	// Retry logic now handled by controllers
	// Per-search state initialized on demand
	// All legacy global state has been removed
}


CSearchList::~CSearchList()
{
	StopSearch();


	while (!m_results.empty()) {
		RemoveResults(m_results.begin()->first);
	}
}


void CSearchList::RemoveResults(long searchID)
{
	// A non-existent search id will just be ignored
	Kademlia::CSearchManager::StopSearch(searchID, true);

	ResultMap::iterator it = m_results.find(searchID);
	if ( it != m_results.end() ) {
		CSearchResultList& list = it->second;

		for (size_t i = 0; i < list.size(); ++i) {
			delete list.at(i);
		}

		m_results.erase( it );
	}
}


uint32 CSearchList::GetNextSearchID()
{
	// Use the new thread-safe search ID generator
	return search::SearchIdGenerator::Instance().generateId();
}

// Per-search state management methods implementation

::PerSearchState* CSearchList::getOrCreateSearchState(long searchId, SearchType searchType, const wxString& searchString)
{
	wxMutexLocker lock(m_searchMutex);
	
	auto it = m_searchStates.find(searchId);
	if (it != m_searchStates.end()) {
		// Search state already exists, return it
		return it->second.get();
	}
	
	// Create new search state
	auto state = std::make_unique<::PerSearchState>(searchId, static_cast<uint8_t>(searchType), searchString);
	auto* statePtr = state.get();

	// Set the owner reference for callbacks
	statePtr->setSearchList(this);

	m_searchStates[searchId] = std::move(state);

	return statePtr;
}

::PerSearchState* CSearchList::getSearchState(long searchId)
{
	wxMutexLocker lock(m_searchMutex);
	auto it = m_searchStates.find(searchId);
	return (it != m_searchStates.end()) ? it->second.get() : nullptr;
}

const ::PerSearchState* CSearchList::getSearchState(long searchId) const
{
	wxMutexLocker lock(m_searchMutex);
	auto it = m_searchStates.find(searchId);
	return (it != m_searchStates.end()) ? it->second.get() : nullptr;
}

void CSearchList::removeSearchState(long searchId)
{
	wxMutexLocker lock(m_searchMutex);

	// Get the search type to determine if we need to remove Kad ID mapping
	auto* searchState = getSearchState(searchId);
	if (searchState && searchState->getSearchType() == static_cast<uint8_t>(KadSearch)) {
		// Remove the Kad search ID mapping
		uint32_t kadSearchId = 0xffffff00 | (searchId & 0xff);
		m_kadSearchIdMap.erase(kadSearchId);
	}

	// Remove from search states map
	m_searchStates.erase(searchId);

	// Also remove from legacy active searches map for compatibility
	// Remove search parameters
	m_searchParams.erase(searchId);

	// Release the search ID for reuse
	search::SearchIdGenerator::Instance().releaseId(searchId);
}

bool CSearchList::hasSearchState(long searchId) const
{
	wxMutexLocker lock(m_searchMutex);
	return m_searchStates.find(searchId) != m_searchStates.end();
}

std::vector<long> CSearchList::getActiveSearchIds() const
{
	wxMutexLocker lock(m_searchMutex);
	std::vector<long> ids;
	ids.reserve(m_searchStates.size());

	for (const auto& pair : m_searchStates) {
		ids.push_back(pair.first);
	}

	return ids;
}

void CSearchList::mapKadSearchId(uint32_t kadSearchId, long originalSearchId)
{
	wxMutexLocker lock(m_searchMutex);
	m_kadSearchIdMap[kadSearchId] = originalSearchId;

	AddDebugLogLineC(logSearch, CFormat(wxT("Mapped Kad search ID %u to original search ID %ld"))
		% kadSearchId % originalSearchId);
}

long CSearchList::getOriginalSearchId(uint32_t kadSearchId) const
{
	wxMutexLocker lock(m_searchMutex);
	KadSearchIdMap::const_iterator it = m_kadSearchIdMap.find(kadSearchId);
	if (it != m_kadSearchIdMap.end()) {
		return it->second;
	}
	return 0;
}

void CSearchList::removeKadSearchIdMapping(uint32_t kadSearchId)
{
	wxMutexLocker lock(m_searchMutex);
	m_kadSearchIdMap.erase(kadSearchId);

	AddDebugLogLineC(logSearch, CFormat(wxT("Removed Kad search ID mapping for %u"))
		% kadSearchId);
}

wxString CSearchList::StartNewSearch(uint32* searchID, SearchType type, CSearchParams& params)
{
	// Check that we can actually perform the specified desired search.
	if ((type == KadSearch) && !Kademlia::CKademlia::IsRunning()) {
		return _("Kad search can't be done if Kad is not running");
	} else if ((type == LocalSearch || type == GlobalSearch) && !theApp->IsConnectedED2K()) {
		return _("eD2k search can't be done if eD2k is not connected");
	}

	// Check for duplicate searches (same type and search string)
	// This prevents multiple searches with identical parameters
	wxMutexLocker lock(m_searchMutex);
	for (const auto& pair : m_searchParams) {
		const CSearchParams& existingParams = pair.second;
		::PerSearchState* state = getSearchState(pair.first);
		if (state && state->getSearchType() == static_cast<uint8_t>(type)) {
			// Check if search string matches
			if (existingParams.searchString == params.searchString) {
				// Found a duplicate search
				// Return the existing search ID
				*searchID = pair.first;
				
				AddDebugLogLineC(logSearch, CFormat(wxT("Duplicate search detected in SearchList: type=%d, string='%s', existing ID=%u"))
					% (int)type % params.searchString % *searchID);
				
				// Return empty string to indicate success (reusing existing search)
				return wxEmptyString;
			}
		}
	}

	if (type == KadSearch) {
		Kademlia::WordList words;
		Kademlia::CSearchManager::GetWords(params.searchString, &words);
		if (!words.empty()) {
			params.strKeyword = words.front();
		} else {
			return _("No keyword for Kad search - aborting");
		}
	}

	bool supports64bit = type == KadSearch ? true : theApp->serverconnect->GetCurrentServer() != NULL && (theApp->serverconnect->GetCurrentServer()->GetTCPFlags() & SRV_TCPFLG_LARGEFILES);
	bool packetUsing64bit;

	// This MemFile is automatically free'd
	CMemFilePtr data = CreateSearchData(params, type, supports64bit, packetUsing64bit);

	if (data.get() == NULL) {
		wxASSERT(_astrParserErrors.GetCount());
		wxString error;

		for (unsigned int i = 0; i < _astrParserErrors.GetCount(); ++i) {
			error += _astrParserErrors[i] + wxT("\n");
		}

		return error;
	}

	if (type == KadSearch) {
		try {
			// Generate search ID through SearchIdGenerator for consistency
			if (*searchID == 0) {
				*searchID = GetNextSearchID();
			} else {
				// If searchID was provided, reserve it in the generator to ensure uniqueness
				// First check if it's already active (e.g., from duplicate detection)
				if (search::SearchIdGenerator::Instance().isValidId(*searchID)) {
					// ID is already active - this happens for duplicate active searches
					// Don't try to reserve it (it's already reserved)
					AddDebugLogLineC(logSearch, CFormat(wxT("Reusing active search ID %u for Kad search"))
						% *searchID);
				} else if (!search::SearchIdGenerator::Instance().reserveId(*searchID)) {
					// Add debugging info
					AddDebugLogLineC(logSearch, CFormat(wxT("Failed to reserve search ID %u for Kad search: already in use or invalid"))
						% *searchID);
					return _("Search ID is already in use");
				}
			}

			// Convert to Kademlia's special ID format (0xffffff??)
			// This ensures Kademlia uses our ID instead of generating its own
			uint32_t kadSearchId = 0xffffff00 | (*searchID & 0xff);

			// Stop any existing search with this ID (for safety)
			Kademlia::CSearchManager::StopSearch(kadSearchId, false);

			// searchstring will get tokenized there
			// Pass our generated ID to Kademlia
			Kademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(params.strKeyword, data->GetLength(), data->GetRawBuffer(), kadSearchId);

			// Verify Kademlia used our ID
			if (search->GetSearchID() != kadSearchId) {
				AddDebugLogLineC(logSearch, CFormat(wxT("Kademlia changed search ID: expected %u, got %u"))
					% kadSearchId % search->GetSearchID());
				// Release our reserved ID
				search::SearchIdGenerator::Instance().releaseId(*searchID);
				delete search;
				return _("Kademlia search ID mismatch");
			}

			// Map Kad search ID to original search ID for result routing
			mapKadSearchId(kadSearchId, *searchID);

			// Create per-search state for Kad search
			auto* searchState = getOrCreateSearchState(*searchID, type, params.searchString);
			if (!searchState) {
				// Release the reserved ID on failure
				search::SearchIdGenerator::Instance().releaseId(*searchID);
				removeKadSearchIdMapping(kadSearchId);
				delete search;
				return _("Failed to create per-search state for Kad search");
			}

			// Initialize Kad search state
			searchState->setKadSearchFinished(false);
			searchState->setKadSearchRetryCount(0);

			// Store search parameters for this search ID
			m_searchParams[*searchID] = params;
		} catch (const wxString& what) {
			AddLogLineC(what);
			return _("Unexpected error while attempting Kad search: ") + what;
		}
	} else if (type == LocalSearch || type == GlobalSearch) {
		// This is an ed2k search, local or global
		// Always generate search ID through SearchIdGenerator for consistency
		if (*searchID == 0) {
			*searchID = GetNextSearchID();
		} else {
			// If searchID was provided, reserve it in the generator to ensure uniqueness
			// First check if it's already active (e.g., from duplicate detection)
			if (search::SearchIdGenerator::Instance().isValidId(*searchID)) {
				// ID is already active - this happens for duplicate active searches
				// Don't try to reserve it (it's already reserved)
				AddDebugLogLineC(logSearch, CFormat(wxT("Reusing active search ID %u for ED2K search"))
					% *searchID);
			} else if (!search::SearchIdGenerator::Instance().reserveId(*searchID)) {
				// Add debugging info
				AddDebugLogLineC(logSearch, CFormat(wxT("Failed to reserve search ID %u for ED2K search: already in use or invalid"))
					% *searchID);
				return _("Search ID is already in use");
			}
		}
		
		// Create per-search state for ED2K search
		auto* searchState = getOrCreateSearchState(*searchID, type, params.searchString);
		if (!searchState) {
			// Release the reserved ID on failure
			search::SearchIdGenerator::Instance().releaseId(*searchID);
			return _("Failed to create per-search state for ED2K search");
		}

		// Store search parameters for this search ID
		m_searchParams[*searchID] = params;

		CPacket* searchPacket = new CPacket(*data.get(), OP_EDONKEYPROT, OP_SEARCHREQUEST);

		NOT_ON_REMOTEGUI(
			theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
		)
		theApp->serverconnect->SendPacket(searchPacket, (type == LocalSearch));

		if (type == GlobalSearch) {
			// Store search packet in per-search state
			searchState->setSearchPacket(std::unique_ptr<CPacket>(searchPacket), packetUsing64bit);
		}
		// Note: For local searches, SendPacket with delpacket=true takes ownership of the packet
		// For global searches, delpacket=false so we retain ownership and store it in searchState
	}

	// Log search start
	AddDebugLogLineC(logSearch, CFormat(wxT("Search started: ID=%u, Type=%d, String='%s'"))
		% *searchID % (int)type % params.searchString);

	// Log Kad-specific info
	if (type == KadSearch) {
		AddDebugLogLineC(logSearch, CFormat(wxT("Kad search prepared: ID=%u, Keyword='%s'"))
			% *searchID % params.strKeyword);
	}

	return wxEmptyString;
}


CSearchList::CSearchParams CSearchList::GetSearchParams(long searchID)
{
	ParamMap::iterator it = m_searchParams.find(searchID);
	if (it != m_searchParams.end()) {
		return it->second;
	}
	return CSearchParams(); // Return empty params if not found
}


wxString CSearchList::RequestMoreResults(long searchID)
{
	// Check if we're connected to eD2k
	if (!theApp->IsConnectedED2K()) {
		return _("eD2k search can't be done if eD2k is not connected");
	}

	// Get the original search parameters
	CSearchParams params = GetSearchParams(searchID);
	if (params.searchString.IsEmpty()) {
		return _("No search parameters available for this search");
	}

	// Stop any current search to prevent race conditions
	StopSearch(true);

	// Use the original search ID to append results to the same search
	// Don't create a new search ID - we want to append results to the existing search
	uint32 originalSearchID = searchID;

	// Create a new global search with the same parameters using the original search ID
	return StartNewSearch(&originalSearchID, GlobalSearch, params);
}


wxString CSearchList::RequestMoreResultsFromServer(const CServer* server, long searchId)
{
	// Check if we're connected to eD2k
	if (!theApp->IsConnectedED2K()) {
		return _("eD2k search can't be done if eD2k is not connected");
	}

	// Check if server is valid
	if (!server) {
		return _("Invalid server");
	}

	// Get the original search parameters
	CSearchParams params = GetSearchParams(searchId);
	if (params.searchString.IsEmpty()) {
		return _("No search parameters available for this search");
	}

	// Create search data packet
	bool packetUsing64bit = false;
	CMemFilePtr data = CreateSearchData(params, GlobalSearch, server->SupportsLargeFilesUDP(), packetUsing64bit);
	if (!data) {
		return _("Failed to create search data");
	}

	// Determine which search request type to use based on server capabilities
	CPacket* searchPacket = NULL;
	if (server->SupportsLargeFilesUDP() && (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)) {
		// Use OP_GLOBSEARCHREQ3 for servers that support large files and extended getfiles
		CMemFile extData(50);
		uint32_t tagCount = 1;
		extData.WriteUInt32(tagCount);
		CTagVarInt flags(CT_SERVER_UDPSEARCH_FLAGS, SRVCAP_UDP_NEWTAGS_LARGEFILES);
		flags.WriteNewEd2kTag(&extData);
		searchPacket = new CPacket(OP_GLOBSEARCHREQ3, data->GetLength() + (uint32_t)extData.GetLength(), OP_EDONKEYPROT);
		searchPacket->CopyToDataBuffer(0, extData.GetRawBuffer(), extData.GetLength());
		searchPacket->CopyToDataBuffer(extData.GetLength(), data->GetRawBuffer(), data->GetLength());
		AddDebugLogLineN(logServerUDP, wxT("Requesting more results from server using OP_GLOBSEARCHREQ3: ") +
			Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
	} else if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
		// Use OP_GLOBSEARCHREQ2 for servers that support extended getfiles
		searchPacket = new CPacket(*data.get(), OP_EDONKEYPROT, OP_GLOBSEARCHREQ2);
		AddDebugLogLineN(logServerUDP, wxT("Requesting more results from server using OP_GLOBSEARCHREQ2: ") +
			Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
	} else {
		// Use OP_GLOBSEARCHREQ for basic servers
		searchPacket = new CPacket(*data.get(), OP_EDONKEYPROT, OP_GLOBSEARCHREQ);
		AddDebugLogLineN(logServerUDP, wxT("Requesting more results from server using OP_GLOBSEARCHREQ: ") +
			Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
	}

	// Send the search request to the server
	NOT_ON_REMOTEGUI(
		theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
	)
	// Cast away const because SendUDPPacket doesn't take const pointer
	theApp->serverconnect->SendUDPPacket(searchPacket, const_cast<CServer*>(server), true);

	return wxEmptyString;
}


void CSearchList::LocalSearchEnd()
{
	if (m_currentSearch == -1) {
		// No active search
		return;
	}
	
	// Get the per-search state
	::PerSearchState* state = getSearchState(m_currentSearch);
	if (!state) {
		// No state found for this search ID
		return;
	}
	
	// Get search type from per-search state
	uint8_t searchType = state->getSearchType();
	
	if (searchType == GlobalSearch) {
		CPacket* searchPacket = state->getSearchPacket();
		wxCHECK_RET(searchPacket, wxT("Global search, but no packet"));

		// Ensure that every global search starts over.
		theApp->serverlist->RemoveObserver(&m_serverQueue);
		m_searchTimer.Start(750);
	} else {
		// Don't trigger retry here - let the UI (SearchDlg/SearchStateManager) handle it
		// The retry mechanism is now managed by SearchStateManager to ensure proper state transitions
		ResultMap::iterator it = m_results.find(m_currentSearch);
		bool hasResults = (it != m_results.end()) && !it->second.empty();
		
		// Only mark the search as finished if we have results
		if (hasResults) {
			OnSearchComplete(m_currentSearch, static_cast<SearchType>(searchType), hasResults);
		} else {
			// No results - let the UI handle retry through SearchStateManager
			// Just mark the search as finished internally
			// Release the search ID since search is complete (no results)
			if (search::SearchIdGenerator::Instance().releaseId(m_currentSearch)) {
				AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u (no results)"))
					% m_currentSearch);
			} else {
				AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release search ID %u (no results) - already released?"))
					% m_currentSearch);
			}
			// Remove from active searches map
			m_activeSearches.erase(m_currentSearch);
			m_searchInProgress = false;
		}
	}
}


uint32 CSearchList::GetSearchProgress() const
{
	// Call the new version with current search ID
	return GetSearchProgress(m_currentSearch);
}

uint32 CSearchList::GetSearchProgress(long searchId) const
{
	if (searchId == -1) {
		// No active search
		return 0;
	}
	
	// Get the per-search state
	const ::PerSearchState* state = getSearchState(searchId);
	if (!state) {
		// No state found for this search ID
		return 0;
	}
	
	// Get search type from per-search state
	uint8_t searchType = state->getSearchType();
	
	if (searchType == KadSearch) {
		// We cannot measure the progress of Kad searches.
		// But we can tell when they are over.
		return state->isKadSearchFinished() ? 0xfffe : 0;
	}
	
	// Check if search is in progress
	// Note: m_searchInProgress is a legacy global flag
	// In the new architecture, we should track per-search progress
	// For now, we check if this is the current search
	if (searchId != m_currentSearch || !m_searchInProgress) {
		// Not the current search or no search in progress
		return 0;
	}

	switch (searchType) {
		case LocalSearch:
			return 0xffff;

		case GlobalSearch:
			// TODO: Global search progress should be calculated per-search
			// For now, use the legacy server queue
			return 100 - (m_serverQueue.GetRemaining() * 100)
					/ theApp->serverlist->GetServerCount();

		default:
			wxFAIL;
	}
	return 0;
}


void CSearchList::OnGlobalSearchTimer(CTimerEvent& ev)
{
	if (m_currentSearch == -1) {
		// No active search
		return;
	}
	
	// Get the per-search state for the current search
	::PerSearchState* state = getSearchState(m_currentSearch);
	if (!state) {
		// No state found for this search ID
		return;
	}
	
	// Get search packet from per-search state
	CPacket* searchPacket = state->getSearchPacket();
	if (!searchPacket) {
		// This was a pending event, handled after 'Stop' was pressed.
		return;
	}
	
	if (!m_serverQueue.IsActive()) {
		theApp->serverlist->AddObserver(&m_serverQueue);
	}

	// UDP requests must not be sent to this server.
	const CServer* localServer = theApp->serverconnect->GetCurrentServer();
	if (localServer) {
		uint32 localIP = localServer->GetIP();
		uint16 localPort = localServer->GetPort();
		while (m_serverQueue.GetRemaining()) {
			CServer* server = m_serverQueue.GetNext();

			// Compare against the currently connected server.
			if ((server->GetPort() == localPort) && (server->GetIP() == localIP)) {
				// We've already requested from the local server.
				continue;
			} else {
				if (server->SupportsLargeFilesUDP() && (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)) {
					CMemFile data(50);
					uint32_t tagCount = 1;
					data.WriteUInt32(tagCount);
					CTagVarInt flags(CT_SERVER_UDPSEARCH_FLAGS, SRVCAP_UDP_NEWTAGS_LARGEFILES);
					flags.WriteNewEd2kTag(&data);
					CPacket *extSearchPacket = new CPacket(OP_GLOBSEARCHREQ3, searchPacket->GetPacketSize() + (uint32_t)data.GetLength(), OP_EDONKEYPROT);
					extSearchPacket->CopyToDataBuffer(0, data.GetRawBuffer(), data.GetLength());
					extSearchPacket->CopyToDataBuffer(data.GetLength(), searchPacket->GetDataBuffer(), searchPacket->GetPacketSize());
					NOT_ON_REMOTEGUI(
											theStats::AddUpOverheadServer(extSearchPacket->GetPacketSize());
					)
					theApp->serverconnect->SendUDPPacket(extSearchPacket, server, true);
					AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ3 to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
				} else if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
					if (!state->is64bitPacket() || server->SupportsLargeFilesUDP()) {
						searchPacket->SetOpCode(OP_GLOBSEARCHREQ2);
						AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ2 to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
						NOT_ON_REMOTEGUI(
													theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
						)
						theApp->serverconnect->SendUDPPacket(searchPacket, server, false);
					} else {
						AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
					}
				} else {
					if (!state->is64bitPacket() || server->SupportsLargeFilesUDP()) {
						searchPacket->SetOpCode(OP_GLOBSEARCHREQ);
						AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
						NOT_ON_REMOTEGUI(
													theStats::AddUpOverheadServer(searchPacket->GetPacketSize());
						)
						theApp->serverconnect->SendUDPPacket(searchPacket, server, false);
					} else {
						AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
					}
				}
				CoreNotify_Search_Update_Progress(GetSearchProgress(m_currentSearch));
				return;
			}
		}
	}
	// No more servers left to ask.

	// Don't trigger retry here - let the UI (SearchDlg/SearchStateManager) handle it
	// The retry mechanism is now managed by SearchStateManager to ensure proper state transitions
	ResultMap::iterator it = m_results.find(m_currentSearch);
	bool hasResults = (it != m_results.end()) && !it->second.empty();

	// Only mark the search as finished if we have results
	if (hasResults) {
		OnSearchComplete(m_currentSearch, m_searchType, hasResults);
		// Only stop if not retrying
		if (m_searchInProgress) {
			StopSearch(true);
		}
	} else {
		// No results - let the UI handle retry through SearchStateManager
		// Notify the UI that global search has ended
		// Release the search ID since search is complete (no results)
		if (search::SearchIdGenerator::Instance().releaseId(m_currentSearch)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u (global search, no results)"))
				% m_currentSearch);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release search ID %u (global search, no results) - already released?"))
				% m_currentSearch);
		}
		
		// Remove from active searches map
		m_activeSearches.erase(m_currentSearch);
		Notify_GlobalSearchEnd();
		// Just mark the search as finished internally
		m_searchInProgress = false;
	}
}


void CSearchList::ProcessSharedFileList(const uint8_t* in_packet, uint32 size,
	CUpDownClient* sender, bool *moreResultsAvailable, const wxString& directory)
{
	wxCHECK_RET(sender, wxT("No sender in search-results from client."));

	long searchID = reinterpret_cast<wxUIntPtr>(sender);

#ifndef AMULE_DAEMON
	if (!theApp->amuledlg->m_searchwnd->CheckTabNameExists(LocalSearch, sender->GetUserName())) {
		theApp->amuledlg->m_searchwnd->CreateNewTab(sender->GetUserName() + wxT(" (0)"), searchID);
	}
#endif

	const CMemFile packet(in_packet, size);
	uint32 results = packet.ReadUInt32();
	bool unicoded = (sender->GetUnicodeSupport() != utf8strNone);
	for (unsigned int i = 0; i != results; ++i){
		CSearchFile* toadd = new CSearchFile(packet, unicoded, searchID, 0, 0, directory);
		toadd->SetClientID(sender->GetUserIDHybrid());
		toadd->SetClientPort(sender->GetUserPort());
		AddToList(toadd, true);
	}

	if (moreResultsAvailable)
		*moreResultsAvailable = false;

	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1) {
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (moreResultsAvailable) {
				*moreResultsAvailable = (ucMore == 1);
			}
		}
	}
}


void CSearchList::ProcessSearchAnswer(const uint8_t* in_packet, uint32_t size, bool optUTF8, uint32_t serverIP, uint16_t serverPort)
{
	CMemFile packet(in_packet, size);

	uint32_t results = packet.ReadUInt32();

	// Get the search ID from the active searches map in a thread-safe manner
	// This ensures results are associated with the correct search
	long searchId = -1;
	SearchType searchType = LocalSearch; // Default to local search

	{
		wxMutexLocker lock(m_searchMutex);
		if (!m_activeSearches.empty()) {
			// Check if this is from the local server (TCP) or a remote server (UDP)
			// TCP responses are for local searches, UDP responses are for global searches
			bool isFromLocalServer = false;
			if (theApp && theApp->serverconnect) {
				const CServer* currentServer = theApp->serverconnect->GetCurrentServer();
				if (currentServer && currentServer->GetIP() == serverIP && currentServer->GetPort() == serverPort) {
					isFromLocalServer = true;
				}
			}

			// Find the most recent search matching the response type
			for (auto it = m_activeSearches.rbegin(); it != m_activeSearches.rend(); ++it) {
				if (isFromLocalServer && it->second == LocalSearch) {
					searchId = it->first;
					searchType = LocalSearch;
					break;
				} else if (!isFromLocalServer && it->second == GlobalSearch) {
					searchId = it->first;
					searchType = GlobalSearch;
					break;
				}
			}
		}
	}

	// If no valid search ID found, drop the results
	// We should NOT use m_currentSearch as a fallback because it can cause
	// results from different searches to be mixed together
	if (searchId == -1) {
		AddDebugLogLineN(logSearch, wxString::Format(wxT("Received search results from %s:%u but no matching active search found, dropping results"),
			(uint32_t)serverIP, serverPort));
		return;
	}

	// Collect all results first
	std::vector<CSearchFile*> resultVector;
	for (; results > 0; --results) {
		resultVector.push_back(new CSearchFile(packet, optUTF8, searchId, serverIP, serverPort));
	}

	// Process results through validator (this adds them to SearchList)
	NOT_ON_REMOTEGUI(
		if (!resultVector.empty()) {
			search::SearchResultRouter::Instance().RouteResults(searchId, resultVector);
		}
	)
}


void CSearchList::ProcessUDPSearchAnswer(const CMemFile& packet, bool optUTF8, uint32_t serverIP, uint16_t serverPort)
{
	// Get the search ID from the active searches map in a thread-safe manner
	// This ensures results are associated with the correct search
	long searchId = -1;
	{
		wxMutexLocker lock(m_searchMutex);
		if (!m_activeSearches.empty()) {
			// Find the most recent global search (UDP is only used for global searches)
			// We need to ensure we don't accidentally route to a local search
			for (auto it = m_activeSearches.rbegin(); it != m_activeSearches.rend(); ++it) {
				if (it->second == GlobalSearch) {
					searchId = it->first;
					break;
				}
			}
		}
	}

	// If no valid search ID found, drop the result
	// UDP results should only go to global searches, not local searches
	if (searchId == -1) {
		AddDebugLogLineN(logSearch, wxString::Format(wxT("Received UDP search result from %s:%u but no active global search found, dropping result"),
			(uint32_t)serverIP, serverPort));
		return;
	}

	// Create result
	CSearchFile* result = new CSearchFile(packet, optUTF8, searchId, serverIP, serverPort);

	// Process result through validator (this adds it to SearchList)
	NOT_ON_REMOTEGUI(
		search::SearchResultRouter::Instance().RouteResult(searchId, result);
	)
}


bool CSearchList::AddToList(CSearchFile* toadd, bool clientResponse)
{
	const uint64 fileSize = toadd->GetFileSize();
	// If filesize is 0, or file is too large for the network, drop it
	if ((fileSize == 0) || (fileSize > MAX_FILE_SIZE)) {
		AddDebugLogLineN(logSearch,
				CFormat(wxT("Dropped result with filesize %u: %s"))
					% fileSize
					% toadd->GetFileName().GetPrintable());

		delete toadd;
		return false;
	}

	// Get the result type for this specific search (thread-safe)
	wxString resultTypeForSearch;
	{
		wxMutexLocker lock(m_searchMutex);
		ParamMap::iterator it = m_searchParams.find(toadd->GetSearchID());
		if (it != m_searchParams.end()) {
			resultTypeForSearch = it->second.typeText;
		}
	}

	// If the result was not the type the user wanted, drop it.
	if ((clientResponse == false) && !resultTypeForSearch.IsEmpty() && resultTypeForSearch != ED2KFTSTR_PROGRAM) {
		if (resultTypeForSearch.CmpNoCase(wxT("Any")) != 0) {
			if (GetFileTypeByName(toadd->GetFileName()) != resultTypeForSearch) {
				AddDebugLogLineN(logSearch,
					CFormat( wxT("Dropped result type %s != %s, file %s") )
						% GetFileTypeByName(toadd->GetFileName())
						% resultTypeForSearch
						% toadd->GetFileName().GetPrintable());

				delete toadd;
				return false;
			}
		}
	}

	// Get, or implicitly create, the map of results for this search
	CSearchResultList& results = m_results[toadd->GetSearchID()];

	for (size_t i = 0; i < results.size(); ++i) {
		CSearchFile* item = results.at(i);

		if ((toadd->GetFileHash() == item->GetFileHash()) && (toadd->GetFileSize() == item->GetFileSize())) {
			AddDebugLogLineN(logSearch, CFormat(wxT("Received duplicate results for '%s' : %s")) % item->GetFileName().GetPrintable() % item->GetFileHash().Encode());
			// Add the child, possibly updating the parents filename.
			item->AddChild(toadd);
			Notify_Search_Update_Sources(item);
			return true;
		}
	}

	AddDebugLogLineN(logSearch,
		CFormat(wxT("Added new result '%s' : %s"))
			% toadd->GetFileName().GetPrintable() % toadd->GetFileHash().Encode());

	// New unique result, simply add and display.
	results.push_back(toadd);
	Notify_Search_Add_Result(toadd);

	return true;
}


const CSearchResultList& CSearchList::GetSearchResults(long searchID) const
{
	ResultMap::const_iterator it = m_results.find(searchID);
	if (it != m_results.end()) {
		return it->second;
	}

	// TODO: Should we assert in this case?
	static CSearchResultList list;
	return list;
}


void CSearchList::AddFileToDownloadByHash(const CMD4Hash& hash, uint8 cat)
{
	ResultMap::iterator it = m_results.begin();
	for ( ; it != m_results.end(); ++it ) {
		CSearchResultList& list = it->second;

		for ( unsigned int i = 0; i < list.size(); ++i ) {
			if ( list[i]->GetFileHash() == hash ) {
				CoreNotify_Search_Add_Download( list[i], cat );

				return;
			}
		}
	}
}


void CSearchList::StopSearch(bool globalOnly)
{
	// This legacy function stops all searches
	// For backward compatibility, we stop all active searches
	auto activeIds = getActiveSearchIds();
	for (long searchId : activeIds) {
		StopSearch(searchId, globalOnly);
	}
}

void CSearchList::StopSearch(long searchID, bool globalOnly)
{
	// Get the search state for this ID
	auto* searchState = getSearchState(searchID);
	if (!searchState) {
		// Search not found, nothing to stop
		return;
	}

	// Get search type from state
	uint8_t searchType = searchState->getSearchType();

	if (searchType == GlobalSearch) {
		// Clear search packet for this search
		searchState->clearSearchPacket();

		// Stop the timer if this was the last global search
		bool hasOtherGlobalSearches = false;
		auto allIds = getActiveSearchIds();
		for (long id : allIds) {
			if (id != searchID) {
				auto* otherState = getSearchState(id);
				if (otherState && otherState->getSearchType() == GlobalSearch) {
					hasOtherGlobalSearches = true;
					break;
				}
			}
		}

		if (!hasOtherGlobalSearches) {
			m_searchTimer.Stop();
		}

		CoreNotify_Search_Update_Progress(0xffff);
	} else if (searchType == KadSearch && !globalOnly) {
		// Convert original search ID to Kad search ID format
		uint32_t kadSearchId = 0xffffff00 | (searchID & 0xff);

		// Remove the Kad search ID mapping
		removeKadSearchIdMapping(kadSearchId);

		// Stop Kad search using the Kad search ID
		Kademlia::CSearchManager::StopSearch(kadSearchId, false);
		searchState->setKadSearchFinished(true);
	}

	// Remove the search state
	removeSearchState(searchID);
}


CSearchList::CMemFilePtr CSearchList::CreateSearchData(CSearchParams& params, SearchType type, bool supports64bit, bool& packetUsing64bit)
{
	// Count the number of used parameters
	unsigned int parametercount = 0;
	if ( !params.typeText.IsEmpty() )	++parametercount;
	if ( params.minSize > 0 )			++parametercount;
	if ( params.maxSize > 0 )			++parametercount;
	if ( params.availability > 0 )		++parametercount;
	if ( !params.extension.IsEmpty() )	++parametercount;

	wxString typeText = params.typeText;
	if (typeText == ED2KFTSTR_ARCHIVE){
		// eDonkeyHybrid 0.48 uses type "Pro" for archives files
		// www.filedonkey.com uses type "Pro" for archives files
		typeText = ED2KFTSTR_PROGRAM;
	} else if (typeText == ED2KFTSTR_CDIMAGE){
		// eDonkeyHybrid 0.48 uses *no* type for iso/nrg/cue/img files
		// www.filedonkey.com uses type "Pro" for CD-image files
		typeText = ED2KFTSTR_PROGRAM;
	}

	// Must write parametercount - 1 parameter headers
	CMemFilePtr data(new CMemFile(100));

	_astrParserErrors.Empty();
	_SearchExpr.m_aExpr.Empty();

	s_strCurKadKeyword.Clear();
	if (type == KadSearch) {
		wxASSERT( !params.strKeyword.IsEmpty() );
		s_strCurKadKeyword = params.strKeyword;
	}

	LexInit(params.searchString);
	int iParseResult = yyparse();
	LexFree();

	if (_astrParserErrors.GetCount() > 0) {
		for (unsigned int i=0; i < _astrParserErrors.GetCount(); ++i) {
			AddLogLineNS(CFormat(wxT("Error %u: %s\n")) % i % _astrParserErrors[i]);
		}

		return CMemFilePtr(nullptr);
	}

	if (iParseResult != 0) {
		_astrParserErrors.Add(CFormat(wxT("Undefined error %i on search expression")) % iParseResult);

		return CMemFilePtr(nullptr);
	}

	if (type == KadSearch && s_strCurKadKeyword != params.strKeyword) {
		AddDebugLogLineN(logSearch, CFormat(wxT("Keyword was rearranged, using '%s' instead of '%s'")) % s_strCurKadKeyword % params.strKeyword);
		params.strKeyword = s_strCurKadKeyword;
	}

	parametercount += _SearchExpr.m_aExpr.GetCount();

	/* Leave the unicode comment there, please... */
	CSearchExprTarget target(data.get(), true /*I assume everyone is unicoded */ ? utf8strRaw : utf8strNone, supports64bit, packetUsing64bit);

	unsigned int iParameterCount = 0;
	if (_SearchExpr.m_aExpr.GetCount() <= 1) {
		// lugdunummaster requested that searches without OR or NOT operators,
		// and hence with no more expressions than the string itself, be sent
		// using a series of ANDed terms, intersecting the ANDs on the terms
		// (but prepending them) instead of putting the boolean tree at the start
		// like other searches. This type of search is supposed to take less load
		// on servers. Go figure.
		//
		// input:      "a" AND min=1 AND max=2
		// instead of: AND AND "a" min=1 max=2
		// we use:     AND "a" AND min=1 max=2

		if (_SearchExpr.m_aExpr.GetCount() > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(_SearchExpr.m_aExpr[0]);
		}

		if (!typeText.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			// Type is always ascii string
			target.WriteMetaDataSearchParamASCII(FT_FILETYPE, typeText);
		}

		if (params.minSize > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_GREATER, params.minSize);
		}

		if (params.maxSize > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_LESS, params.maxSize);
		}

		if (params.availability > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_SOURCES, ED2K_SEARCH_OP_GREATER, params.availability);
		}

		if (!params.extension.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, params.extension);
		}

		//#warning TODO - I keep this here, ready if we ever allow such searches...
		#if 0
		if (complete > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(FT_COMPLETE_SOURCES, ED2K_SEARCH_OP_GREATER, complete);
		}

		if (minBitrate > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_BITRATE : FT_ED2K_MEDIA_BITRATE, ED2K_SEARCH_OP_GREATER, minBitrate);
		}

		if (minLength > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_LENGTH : FT_ED2K_MEDIA_LENGTH, ED2K_SEARCH_OP_GREATER, minLength);
		}

		if (!codec.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_CODEC : FT_ED2K_MEDIA_CODEC, codec);
		}

		if (!title.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_TITLE : FT_ED2K_MEDIA_TITLE, title);
		}

		if (!album.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ALBUM : FT_ED2K_MEDIA_ALBUM, album);
		}

		if (!artist.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ARTIST : FT_ED2K_MEDIA_ARTIST, artist);
		}
		#endif // 0

		// If this assert fails... we're seriously fucked up

		wxASSERT( iParameterCount == parametercount );

	} else {
		if (!params.extension.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.availability > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.maxSize > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (params.minSize > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!typeText.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		//#warning TODO - same as above...
		#if 0
		if (complete > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (minBitrate > 0){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (minLength > 0) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!codec.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!title.IsEmpty()){
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!album.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}

		if (!artist.IsEmpty()) {
			if (++iParameterCount < parametercount) {
				target.WriteBooleanAND();
			}
		}
		#endif // 0

		// As above, if this fails, we're seriously fucked up.
		wxASSERT( iParameterCount + _SearchExpr.m_aExpr.GetCount() == parametercount );

		for (unsigned int j = 0; j < _SearchExpr.m_aExpr.GetCount(); ++j) {
			if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_AND) {
				target.WriteBooleanAND();
			} else if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_OR) {
				target.WriteBooleanOR();
			} else if (_SearchExpr.m_aExpr[j] == SEARCHOPTOK_NOT) {
				target.WriteBooleanNOT();
			} else {
				target.WriteMetaDataSearchParam(_SearchExpr.m_aExpr[j]);
			}
		}

		if (!params.typeText.IsEmpty()) {
			// Type is always ASCII string
			target.WriteMetaDataSearchParamASCII(FT_FILETYPE, params.typeText);
		}

		if (params.minSize > 0) {
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_GREATER, params.minSize);
		}

		if (params.maxSize > 0) {
			target.WriteMetaDataSearchParam(FT_FILESIZE, ED2K_SEARCH_OP_LESS, params.maxSize);
		}

		if (params.availability > 0) {
			target.WriteMetaDataSearchParam(FT_SOURCES, ED2K_SEARCH_OP_GREATER, params.availability);
		}

		if (!params.extension.IsEmpty()) {
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, params.extension);
		}

		//#warning TODO - third and last warning of the same series.
		#if 0
		if (complete > 0) {
			target.WriteMetaDataSearchParam(FT_COMPLETE_SOURCES, ED2K_SEARCH_OP_GREATER, pParams->uComplete);
		}

		if (minBitrate > 0) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_BITRATE : FT_ED2K_MEDIA_BITRATE, ED2K_SEARCH_OP_GREATER, minBitrate);
		}

		if (minLength > 0) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_LENGTH : FT_ED2K_MEDIA_LENGTH, ED2K_SEARCH_OP_GREATER, minLength);
		}

		if (!codec.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_CODEC : FT_ED2K_MEDIA_CODEC, codec);
		}

		if (!title.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_TITLE : FT_ED2K_MEDIA_TITLE, title);
		}

		if (!album.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ALBUM : FT_ED2K_MEDIA_ALBUM, album);
		}

		if (!artist.IsEmpty()) {
			target.WriteMetaDataSearchParam(type == KadSearch ? TAG_MEDIA_ARTIST : FT_ED2K_MEDIA_ARTIST, artist);
		}

		#endif // 0
	}

	// Packet ready to go.
	return data;
}


void CSearchList::KademliaSearchKeyword(uint32_t searchID, const Kademlia::CUInt128 *fileID,
	const wxString& name, uint64_t size, const wxString& type, uint32_t kadPublishInfo, const TagPtrList& taglist)
{
	// Convert Kad search ID to original search ID for routing
	long originalSearchId = getOriginalSearchId(searchID);
	if (originalSearchId == 0) {
		AddDebugLogLineC(logSearch, CFormat(wxT("KademliaSearchKeyword: No mapping found for Kad search ID %u, result will be lost"))
			% searchID);
		return;
	}

	AddDebugLogLineC(logSearch, CFormat(wxT("KademliaSearchKeyword: Routing result from Kad ID %u to original ID %ld"))
		% searchID % originalSearchId);

	EUtf8Str eStrEncode = utf8strRaw;

	CMemFile temp(250);
	uint8_t fileid[16];
	fileID->ToByteArray(fileid);
	temp.WriteHash(CMD4Hash(fileid));

	temp.WriteUInt32(0);	// client IP
	temp.WriteUInt16(0);	// client port

	// write tag list
	unsigned int uFilePosTagCount = temp.GetPosition();
	uint32 tagcount = 0;
	temp.WriteUInt32(tagcount); // dummy tag count, will be filled later

	// standard tags
	CTagString tagName(FT_FILENAME, name);
	tagName.WriteTagToFile(&temp, eStrEncode);
	tagcount++;

	CTagInt64 tagSize(FT_FILESIZE, size);
	tagSize.WriteTagToFile(&temp, eStrEncode);
	tagcount++;

	if (!type.IsEmpty()) {
		CTagString tagType(FT_FILETYPE, type);
		tagType.WriteTagToFile(&temp, eStrEncode);
		tagcount++;
	}

	// Misc tags (bitrate, etc)
	for (TagPtrList::const_iterator it = taglist.begin(); it != taglist.end(); ++it) {
		(*it)->WriteTagToFile(&temp,eStrEncode);
		tagcount++;
	}

	temp.Seek(uFilePosTagCount, wxFromStart);
	temp.WriteUInt32(tagcount);

	temp.Seek(0, wxFromStart);

	CSearchFile *tempFile = new CSearchFile(temp, (eStrEncode == utf8strRaw), originalSearchId, 0, 0, wxEmptyString, true);
	tempFile->SetKadPublishInfo(kadPublishInfo);


	// Process result through validator (this adds it to SearchList)
	NOT_ON_REMOTEGUI(
		search::SearchResultRouter::Instance().RouteResult(originalSearchId, tempFile);
	)
}

void CSearchList::UpdateSearchFileByHash(const CMD4Hash& hash)
{
	for (ResultMap::iterator it = m_results.begin(); it != m_results.end(); ++it) {
		CSearchResultList& results = it->second;
		for (size_t i = 0; i < results.size(); ++i) {
			CSearchFile* item = results.at(i);

			if (hash == item->GetFileHash()) {
				// This covers only parent items,
				// child items have to be updated separately.
				Notify_Search_Update_Sources(item);
			}
		}
	}
}


void CSearchList::SetKadSearchFinished()
{
	if (m_currentSearch == -1) {
		// No active search
		return;
	}
	
	// Get the per-search state
	::PerSearchState* state = getSearchState(m_currentSearch);
	if (!state) {
		// No state found for this search ID
		return;
	}
	
	// Check if we have any results for the current search
	ResultMap::iterator it = m_results.find(m_currentSearch);
	bool hasResults = (it != m_results.end()) && !it->second.empty();

	// Don't trigger retry here - let the UI (SearchDlg/SearchStateManager) handle it
	// The retry mechanism is now managed by SearchStateManager to ensure proper state transitions
	// Only mark the search as finished if we have results
	if (hasResults) {
		OnSearchComplete(m_currentSearch, KadSearch, hasResults);
	} else {
		// No results - let the UI handle retry through SearchStateManager
		// Just mark the Kad search as finished internally in per-search state
		state->setKadSearchFinished(true);
		// Release the search ID since search is complete (no results)
		if (search::SearchIdGenerator::Instance().releaseId(m_currentSearch)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Released Kad search ID %u (no results)"))
				% m_currentSearch);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release Kad search ID %u (no results) - already released?"))
				% m_currentSearch);
		}
		// Remove from active searches map
		m_activeSearches.erase(m_currentSearch);
	}
}


void CSearchList::OnSearchComplete(long searchId, SearchType type, bool hasResults)
{
	// Update result count
	ResultMap::iterator it = m_results.find(searchId);
	int resultCount = (it != m_results.end()) ? it->second.size() : 0;

	// Log marking search as finished
	AddDebugLogLineC(logSearch, CFormat(wxT("Marking search finished: ID=%ld, Type=%d"))
		% searchId % (int)type);

	// Remove this search from the active searches map
	m_activeSearches.erase(searchId);

	// Mark search as finished
	if (type == KadSearch) {
		m_KadSearchFinished = true;
		// Release Kad search ID
		if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Released Kad search ID %u (search completed with results)"))
				% searchId);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release Kad search ID %u (search completed) - already released?"))
				% searchId);
		}
	} else {
		m_searchInProgress = false;
		Notify_SearchLocalEnd();
		
		// Release the search ID for non-Kad searches
		if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
			AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u (search completed with results)"))
				% searchId);
		} else {
			AddDebugLogLineC(logSearch, CFormat(wxT("Failed to release search ID %u (search completed) - already released?"))
				% searchId);
		}
	}
}


void CSearchList::OnSearchRetry(long searchId, SearchType type, int retryNum)
{
	// Log retry attempt
	AddDebugLogLineC(logSearch, CFormat(wxT("OnSearchRetry: SearchID=%ld, Type=%d, RetryNum=%d"))
		% searchId % (int)type % retryNum);

	// Get original parameters
	CSearchParams params = GetSearchParams(searchId);
	if (params.searchString.IsEmpty()) {
		AddDebugLogLineC(logSearch,
			CFormat(wxT("Retry %d for search %ld failed: no parameters"))
				% retryNum % searchId);
		return;
	}

	// Clean up old search state before retrying
	// Remove from active searches
	m_activeSearches.erase(searchId);
	// Remove search parameters (they will be recreated with new ID)
	m_searchParams.erase(searchId);
	// Remove per-search state
	removeSearchState(searchId);
	
	// Release the old search ID before retrying
	if (search::SearchIdGenerator::Instance().releaseId(searchId)) {
		AddDebugLogLineC(logSearch, CFormat(wxT("Released search ID %u before retry"))
			% searchId);
	}
	// Note: if release fails, the ID might already be released (e.g., search completed)

	// Start new search with same parameters
	uint32 newSearchId = 0;
	wxString error = StartNewSearch(&newSearchId, type, params);

	if (!error.IsEmpty()) {
		AddDebugLogLineC(logSearch,
			wxString::Format(wxT("Retry %d for search %ld failed: %s"),
				retryNum, searchId, error.c_str()));
		return;
	}


	// Move results from old search ID to new search ID
	ResultMap::iterator resultsIt = m_results.find(searchId);
	if (resultsIt != m_results.end()) {
		// Update the search ID for all results
		CSearchResultList& results = resultsIt->second;
		for (size_t i = 0; i < results.size(); ++i) {
			results[i]->SetSearchID(newSearchId);
		}
		// Move the results to the new search ID
		m_results[newSearchId] = results;
		m_results.erase(searchId);
		AddDebugLogLineC(logSearch, wxString::Format(wxT("Moved %zu results from search %ld to %ld"), results.size(), searchId, newSearchId));
	}


	// Log success
	AddDebugLogLineC(logSearch,
		wxString::Format(wxT("Retry %d started for search %ld (new ID: %u)"),
			retryNum, searchId, newSearchId));
}


// File_checked_for_headers

wxString CSearchList::RequestMoreResultsForSearch(long searchId)
{
	// Request more results for the given search
	// This is a wrapper around RequestMoreResults that handles the search ID
	return RequestMoreResults(searchId);
}
