#include "GlobalSearchController.h"
#include "../amule.h"
#include "../ServerConnect.h"
#include "../Server.h"
#include "../ServerList.h"
#include "../Logger.h"
#include "../SearchList.h"
#include "../MemFile.h"  // Include for CMemFile
#include "../Packet.h"
#include "../Statistics.h"
#include "../include/tags/ClientTags.h"
#include "../include/tags/FileTags.h"
#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Server/UDP.h>
#include "../search/SearchLogging.h"  // Include for search logging macros

namespace search {

GlobalSearchController::GlobalSearchController()
    : m_64bitSearchPacket(false)
{
}

GlobalSearchController::~GlobalSearchController()
{
    stopSearch();
}

void GlobalSearchController::startSearch(const SearchParams& params)
{
	// Validate parameters
	if (params.searchString.IsEmpty()) {
		handleSearchError(-1, _("Search string is empty"));
		return;
	}

	// Generate search ID
	uint32_t searchId = m_model->getSearchId() == -1 ? GenerateSearchId() : m_model->getSearchId();
	SEARCH_DEBUG_CONTROLLER(
		CFormat(wxT("GlobalSearchController: Generated search ID %u for global search: %s"))
		% searchId % params.searchString);

	// Set the current search ID in SearchList for result routing
	if (theApp && theApp->searchlist) {
		// Store search parameters in SearchList for later use
		CSearchList::CSearchParams oldParams;
		oldParams.searchString = params.searchString;
		oldParams.strKeyword = params.strKeyword;
		oldParams.typeText = params.typeText;
		oldParams.extension = params.extension;
		oldParams.minSize = params.minSize;
		oldParams.maxSize = params.maxSize;
		oldParams.availability = params.availability;
		oldParams.searchType = GlobalSearch;

		// Set the current search ID in SearchList
		// This is used by ProcessSearchAnswer to route results
		theApp->searchlist->SetCurrentSearch(searchId);

		// Register this search in active searches map
		// This is critical for ProcessSearchAnswer to find the search
		theApp->searchlist->RegisterActiveSearch(searchId, GlobalSearch);

		// Store search parameters in SearchStateManager
		theApp->searchlist->StoreSearchParams(searchId, oldParams);

		SEARCH_DEBUG_CONTROLLER(
			CFormat(wxT("GlobalSearchController: Set current search ID %u in SearchList"))
			% searchId);
	}

	// Store search ID in model
	m_model->setSearchId(searchId);
	m_model->setSearchState(SearchState::Searching);

	// Register with SearchResultRouter for result routing
	SearchResultRouter::Instance().RegisterController(searchId, this);
	SearchResultRouter::Instance().RegisterControllerByType(::GlobalSearch, this);

	// Convert modern search params to legacy format
	CSearchList::CSearchParams legacyParams;
	legacyParams.searchString = params.searchString;
	legacyParams.strKeyword = params.strKeyword;
	legacyParams.typeText = params.typeText;
	legacyParams.extension = params.extension;
	legacyParams.minSize = params.minSize;
	legacyParams.maxSize = params.maxSize;
	legacyParams.availability = params.availability;
	legacyParams.searchType = GlobalSearch;

	// Create search data packet
	bool packetUsing64bit = false;
	CSearchList::CMemFilePtr data = theApp->searchlist->CreateSearchData(legacyParams, GlobalSearch, 
		theApp->serverconnect->GetCurrentServer() ? theApp->serverconnect->GetCurrentServer()->SupportsLargeFilesUDP() : false, 
		packetUsing64bit);
	
	if (!data) {
		handleSearchError(m_model->getSearchId(), _("Failed to create search data"));
		return;
	}

	// Create packet
	m_searchPacket.reset(new CPacket(*data.get(), OP_EDONKEYPROT, OP_GLOBSEARCHREQ));
	m_64bitSearchPacket = packetUsing64bit;
	m_queriedServers.clear();

	// Start timer
	m_searchTimer.Start(100);
}


uint32_t GlobalSearchController::GenerateSearchId()
{
    // Generate a unique search ID for ED2K searches
    // Use a different ID range than Kad to avoid conflicts
    // Kad uses IDs in range [1, 0x7FFFFFFF]
    // ED2K uses IDs in range [0x80000001, 0xFFFFFFFE]
    // Note: The server will assign its own search ID, so this is just for tracking
    static uint32_t s_nextSearchId = 0x80000000;
    s_nextSearchId = (s_nextSearchId + 1) % 0xFFFFFFFE;
    if (s_nextSearchId < 0x80000001) {
        s_nextSearchId = 0x80000001;
    }
    return s_nextSearchId;
}

void GlobalSearchController::stopSearch()
{
    m_searchTimer.Stop();
    m_searchPacket.reset();
    m_queriedServers.clear();
    m_model->setSearchState(SearchState::Idle);
}

bool GlobalSearchController::isSearching() const
{
    return m_searchTimer.IsRunning();
}

} // namespace search