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
#include "search/SearchLogging.h"		// Debug logging

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
	: m_searchTimer(this, 0 /* Timer-id doesn't matter. */ ),
	  m_64bitSearchPacket(false),
	  m_KadSearchFinished(true)
{
	// Retry logic now handled by controllers
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
	static uint32 nextID = 0;
	return ++nextID;
}


wxString CSearchList::RequestMoreResults(long searchID)
{
	// Check if we're connected to eD2k
	if (!theApp->IsConnectedED2K()) {
		return _("eD2k search can't be done if eD2k is not connected");
	}

	// Get the original search parameters
	CSearchParams params;
	if (!GetSearchParams(searchID, params)) {
		return _("No search parameters available for this search");
	}

	// Stop any current search to prevent race conditions
	StopSearch(true);

	// Use the SearchResultRouter to find the appropriate controller for this search ID
	search::SearchResultRouter& router = search::SearchResultRouter::Instance();
	search::SearchController* controller = router.GetController(static_cast<uint32_t>(searchID));
	
	if (!controller) {
		// No existing controller found, create a new one based on the search type
		std::unique_ptr<search::SearchController> controllerPtr;
		if (params.searchType == GlobalSearch) {
			controllerPtr = search::SearchControllerFactory::createController(search::ModernSearchType::GlobalSearch);
		} else {
			// For LocalSearch, use ED2K controller
			controllerPtr = search::SearchControllerFactory::createController(search::ModernSearchType::LocalSearch);
		}
		
		// Set the search ID for the controller to match the original search
		controllerPtr->setSearchId(searchID);
		
		// Get the raw pointer before transferring ownership
		controller = controllerPtr.get();
		
		// Register the controller with the router using the raw pointer
		router.RegisterController(static_cast<uint32_t>(searchID), controller);
		
		// Transfer ownership to some manager if needed, or let it be destroyed automatically
		// Note: In a real implementation, you would need to store the unique_ptr somewhere
		// For now, we'll just release it since the router likely stores a raw pointer
		(void)controllerPtr.release();
	}
	
	// Request more results
	controller->requestMoreResults();
	
	return wxEmptyString;
}


void CSearchList::LocalSearchEnd()
{
	if (m_searchPacket) {
		// This is a global search timer event, not a local search end
		wxCHECK_RET(m_searchPacket, wxT("Global search, but no packet"));

		// Ensure that every global search starts over.
		theApp->serverlist->RemoveObserver(&m_serverQueue);
		m_searchTimer.Start(750);
		return;
	}

	// Find all active local searches and complete them
	std::vector<long> localSearchesToComplete;
	
	{
		wxMutexLocker lock(m_searchMutex);
		for (const auto& search : m_activeSearches) {
			if (search.second == LocalSearch) {
				localSearchesToComplete.push_back(search.first);
			}
		}
	}

	// Complete each local search
	for (long searchId : localSearchesToComplete) {
		// Check if there are results for this search
		ResultMap::iterator it = m_results.find(searchId);
		bool hasResults = (it != m_results.end()) && !it->second.empty();
		
		OnSearchComplete(searchId, LocalSearch, hasResults);
	}
}


uint32 CSearchList::GetSearchProgress(long searchId) const
{
	wxMutexLocker lock(m_searchMutex);
	
	// Find the search type for this search ID
	auto searchIt = m_activeSearches.find(searchId);
	if (searchIt == m_activeSearches.end()) {
		// Invalid search ID
		return 0;
	}
	
	SearchType searchType = searchIt->second;
	
	if (searchType == KadSearch) {
		// We cannot measure the progress of Kad searches.
		// But we can tell when they are over.
		return m_KadSearchFinished ? 0xfffe : 0;
	}
	
	// For ED2K searches (Local/Global), check if the search is still active
	// If it's in the active searches map, it's in progress
	bool searchInProgress = (searchIt != m_activeSearches.end());
	
	if (!searchInProgress) {
		// No search, no progress ;)
		return 0;
	}

	switch (searchType) {
		case LocalSearch:
			return 0xffff;

		case GlobalSearch:
			// TODO: Implement actual global search progress tracking
			// For now, return a reasonable value
			return 0x8000;

		default:
			return 0;
	}
}

// Keep the old method for backward compatibility but make it return 0
// since we no longer have a single "current" search
uint32 CSearchList::GetSearchProgress() const
{
	// In the new architecture, there's no single current search
	// Return 0 to indicate no global progress
	return 0;
}


void CSearchList::OnGlobalSearchTimer(CTimerEvent& evt)
{
	// UDP requests must not be sent to this server.
	const CServer* localServer = theApp->serverconnect->GetCurrentServer();
	if (localServer) {
		uint32 localIP = localServer->GetIP();
		uint16 localPort = localServer->GetPort();

		// Find the most recent active GlobalSearch
		long globalSearchId = FindMostRecentActiveSearch(GlobalSearch);
		if (globalSearchId == -1) {
			// No active global search, clean up and return
			m_searchPacket.reset();
			m_serverQueue.Reset();
			return;
		}

		// Get the search parameters for this search
		CSearchParams params;
		if (!GetSearchParams(globalSearchId, params)) {
			m_searchPacket.reset();
			m_serverQueue.Reset();
			return;
		}

		// Process servers for global search
		while (m_serverQueue.GetRemaining() > 0) {
			const CServer* serverConst = m_serverQueue.GetNext();
			// Need to cast away const for SendUDPPacket compatibility
			CServer* server = const_cast<CServer*>(serverConst);
			if (!server || server->GetIP() == localIP || server->GetPort() == localPort) {
				continue;
			}

			if (server->GetUDPFlags() & SRV_UDPFLG_NEWTAGS) {
				if (!m_64bitSearchPacket || server->SupportsLargeFilesUDP()) {
					m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ3);
					AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ3 to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
					theStats::AddUpOverheadServer(m_searchPacket->GetPacketSize());
					theApp->serverconnect->SendUDPPacket(m_searchPacket.get(), server, false);
				} else {
					AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
				}
			} else if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES) {
				if (!m_64bitSearchPacket || server->SupportsLargeFilesUDP()) {
					m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ2);
					AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ2 to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
					theStats::AddUpOverheadServer(m_searchPacket->GetPacketSize());
					theApp->serverconnect->SendUDPPacket(m_searchPacket.get(), server, false);
				} else {
					AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
				}
			} else {
				if (!m_64bitSearchPacket || server->SupportsLargeFilesUDP()) {
					m_searchPacket->SetOpCode(OP_GLOBSEARCHREQ);
					AddDebugLogLineN(logServerUDP, wxT("Sending OP_GLOBSEARCHREQ to server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()));
					theStats::AddUpOverheadServer(m_searchPacket->GetPacketSize());
					theApp->serverconnect->SendUDPPacket(m_searchPacket.get(), server, false);
				} else {
					AddDebugLogLineN(logServerUDP, wxT("Skipped UDP search on server ") + Uint32_16toStringIP_Port(server->GetIP(), server->GetPort()) + wxT(": No large file support"));
				}
			}
			CoreNotify_Search_Update_Progress(GetSearchProgress(globalSearchId));
			return;
		}
	}
	// No more servers left to ask.

	// Don't trigger retry here - let the UI (SearchDlg/SearchStateManager) handle it
	// The retry mechanism is now managed by SearchStateManager to ensure proper state transitions
	long globalSearchId = FindMostRecentActiveSearch(GlobalSearch);
	if (globalSearchId != -1) {
		// Get search parameters for this search ID
		CSearchParams params;
		if (GetSearchParams(globalSearchId, params)) {
			// Complete the search by unregistering it
			UnregisterActiveSearch(globalSearchId);
			// The UI will detect that the search is no longer active and handle completion
		}
	} else {
		// Clean up search packet if no active search
		m_searchPacket.reset();
	}
	
	m_serverQueue.Reset();
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


void CSearchList::ProcessSearchAnswer(const uint8_t* packet, uint32_t size, bool optUTF8, uint32_t serverIP, uint16_t serverPort)
{
	// Create CMemFile from raw packet data
	CMemFile memFile(packet, size);
	
	// Read number of results first
	uint32 numResults = memFile.ReadUInt32();
	
	// Process all results
	std::vector<CSearchFile*> resultVector;
	resultVector.reserve(numResults);
	
	for (uint32 i = 0; i < numResults; ++i) {
		if (memFile.GetPosition() >= memFile.GetLength()) {
			break;
		}
		
		// For TCP local search results, use type-based routing
		// Search ID is set to -1 to indicate type-based routing
		CSearchFile* result = new CSearchFile(memFile, optUTF8, -1, serverIP, serverPort);
		resultVector.push_back(result);
	}
	
	// Route all results through type-based routing for LocalSearch
	if (!resultVector.empty()) {
		search::SearchResultRouter::Instance().RouteResultsByType(LocalSearch, resultVector);
	}
}


void CSearchList::ProcessUDPSearchAnswer(const CMemFile& packet, bool optUTF8, uint32_t serverIP, uint16_t serverPort)
{
	// Create result from the packet
	CSearchFile* result = new CSearchFile(packet, optUTF8, -1, serverIP, serverPort);
	
	// For UDP global search results, use type-based routing to ensure they only go to GlobalSearch
	// This prevents mixing with LocalSearch results even if search IDs are reused
	std::vector<CSearchFile*> results;
	results.push_back(result);
	
	search::SearchResultRouter::Instance().RouteResultsByType(GlobalSearch, results);
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
		std::map<long, CSearchParams>::iterator it = m_searchParams.find(toadd->GetSearchID());
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
	if (globalOnly) {
		// Stop all active global searches
		std::vector<long> globalSearchesToStop;
		
		{
			wxMutexLocker lock(m_searchMutex);
			for (const auto& search : m_activeSearches) {
				if (search.second == GlobalSearch) {
					globalSearchesToStop.push_back(search.first);
				}
			}
		}

		// Stop each global search
		for (long searchId : globalSearchesToStop) {
			m_activeSearches.erase(searchId);
			// Note: For global searches, we just remove from active list,
			// the actual network operation is stopped by clearing m_searchPacket
		}
		
		// Reset search packet (unique_ptr handles deletion automatically)
		m_searchPacket.reset();
		
		m_searchTimer.Stop();
		CoreNotify_Search_Update_Progress(0xffff);
	} else {
		// Stop all active Kad searches
		std::vector<long> kadSearchesToStop;
		
		{
			wxMutexLocker lock(m_searchMutex);
			for (const auto& search : m_activeSearches) {
				if (search.second == KadSearch) {
					kadSearchesToStop.push_back(search.first);
				}
			}
		}

		// Stop each KAD search
		for (long searchId : kadSearchesToStop) {
			m_activeSearches.erase(searchId);
			Kademlia::CSearchManager::StopSearch(searchId, false);
		}
		
		m_KadSearchFinished = true;
	}
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

	CSearchFile *tempFile = new CSearchFile(temp, (eStrEncode == utf8strRaw), searchID, 0, 0, wxEmptyString, true);
	tempFile->SetKadPublishInfo(kadPublishInfo);


	// Process result through validator (this adds it to SearchList)
	search::SearchResultRouter::Instance().RouteResult(searchID, tempFile);
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
	// End all active KAD searches
	std::vector<long> kadSearchesToComplete;
	
	{
		wxMutexLocker lock(m_searchMutex);
		for (const auto& search : m_activeSearches) {
			if (search.second == KadSearch) {
				kadSearchesToComplete.push_back(search.first);
			}
		}
	}

	// Complete each KAD search
	for (long searchId : kadSearchesToComplete) {
		// Check if there are results for this search
		ResultMap::iterator it = m_results.find(searchId);
		bool hasResults = (it != m_results.end()) && !it->second.empty();
		
		OnSearchComplete(searchId, KadSearch, hasResults);
	}
	
	m_KadSearchFinished = true;
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
	} else {
		// For ED2K searches, check if there are any other active ED2K searches
		bool hasOtherEd2kSearches = false;
		for (const auto& activeSearch : m_activeSearches) {
			if (activeSearch.second == LocalSearch || activeSearch.second == GlobalSearch) {
				hasOtherEd2kSearches = true;
				break;
			}
		}
		
		// Only notify end if this was the last ED2K search
		if (!hasOtherEd2kSearches) {
			Notify_SearchLocalEnd();
		}
		// Note: We don't use m_searchInProgress anymore in the new architecture
	}
}

// File_checked_for_headers

wxString CSearchList::RequestMoreResultsForSearch(long searchId)
{
	// Request more results for the given search
	// This is a wrapper around RequestMoreResults that handles the search ID
	return RequestMoreResults(searchId);
}

void CSearchList::StopAllSearches()
{
	// Stop all active searches
	wxMutexLocker lock(m_searchMutex);
	for (const auto& activeSearch : m_activeSearches) {
		StopSearch(activeSearch.first);
	}
}

long CSearchList::FindMostRecentActiveSearch(SearchType type) const
{
	wxMutexLocker lock(m_searchMutex);
	
	// Since we don't track creation order in the map, we'll return the first match
	// In a real implementation, you might want to track creation timestamps
	for (const auto& activeSearch : m_activeSearches) {
		if (activeSearch.second == type) {
			return activeSearch.first;
		}
	}
	
	return -1; // No active search of the specified type
}
