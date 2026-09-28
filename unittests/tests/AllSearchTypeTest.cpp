//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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

// AllSearch (issue #1542) adds a fourth search type that simultaneously queries eD2k
// (local + global) and Kad. Its enum value, its EC protocol code, and its interaction
// with CSearchRequest's reuse logic are all pinned here, because:
//
//   - The numeric value is wire format (EC_TAG_SEARCH_LIFECYCLE_KIND) and on-disk format
//     (StoredSearches.met), so a silent renumber would corrupt both.
//   - The EC mapping in ExternalConn.cpp is a ternary that falls through to LocalSearch
//     for unknown codes; a missing AllSearch case would quietly start a local-only search
//     when a remote client asked for "all".
//   - CSearchRequest::CanReuse treats type as a distinguishing filter, so AllSearch must
//     not be reused as (or by) any of the other three types.

#include <muleunit/test.h>

#include "SearchList.h"
#include "SearchRequest.h"

#include <ECCodes.h>

using namespace muleunit;

DECLARE_SIMPLE(AllSearchType)

// The enum value is pinned to 5: it shares the numeric space with EC_SEARCH_TYPE
// (cast straight to uint8 for EC_TAG_SEARCH_LIFECYCLE_KIND) and with StoredSearches.met
// (written as a raw byte). Renumbering silently reinterprets both.
TEST(AllSearchType, EnumValueIsPinned)
{
	ASSERT_EQUALS(5, static_cast<int>(AllSearch));
}

// Every SearchType member must carry its EC_SEARCH_TYPE number. The static_asserts in
// ExternalConn.cpp check this at build time, but a test makes the contract visible and
// catches a drift introduced by a header that ExternalConn.cpp does not include.
TEST(AllSearchType, AlignsWithECSearchType)
{
	ASSERT_EQUALS(static_cast<int>(LocalSearch), static_cast<int>(EC_SEARCH_LOCAL));
	ASSERT_EQUALS(static_cast<int>(GlobalSearch), static_cast<int>(EC_SEARCH_GLOBAL));
	ASSERT_EQUALS(static_cast<int>(KadSearch), static_cast<int>(EC_SEARCH_KAD));
	ASSERT_EQUALS(static_cast<int>(BrowseSearch), static_cast<int>(EC_SEARCH_BROWSE));
	ASSERT_EQUALS(static_cast<int>(AllSearch), static_cast<int>(EC_SEARCH_ALL));
}

// The gap at 3 (EC_SEARCH_WEB) is deliberate and documented in SearchList.h. AllSearch
// must not fill it: doing so would make every "all" search report itself as a web search
// on the wire.
TEST(AllSearchType, DoesNotCollideWithWebSearchValue)
{
	ASSERT_TRUE(static_cast<int>(AllSearch) != 3);
	ASSERT_EQUALS(3, static_cast<int>(EC_SEARCH_WEB));
}

// CSearchRequest uses the type as a distinguishing filter: an AllSearch tab must not be
// reused for a Local/Global/Kad-only request, and vice versa. This is the gate that
// prevents the dialog from silently switching a user's "All" tab to a single-network one.
TEST(AllSearchType, SearchRequestDistinguishesAllFromOthers)
{
	CSearchList::CSearchParams params;
	params.searchString = "ubuntu";
	const CSearchRequest allRequest(AllSearch, params);

	ASSERT_TRUE(allRequest.CanReuse(CSearchRequest(AllSearch, params), 0));
	ASSERT_TRUE(!allRequest.CanReuse(CSearchRequest(LocalSearch, params), 0));
	ASSERT_TRUE(!allRequest.CanReuse(CSearchRequest(GlobalSearch, params), 0));
	ASSERT_TRUE(!allRequest.CanReuse(CSearchRequest(KadSearch, params), 0));
}

// Symmetric: a request for a single-network search must not grab an AllSearch tab.
TEST(AllSearchType, SingleNetworkRequestDoesNotReuseAllTab)
{
	CSearchList::CSearchParams params;
	params.searchString = "ubuntu";
	const CSearchRequest localRequest(LocalSearch, params);
	const CSearchRequest allRequest(AllSearch, params);

	ASSERT_TRUE(!localRequest.CanReuse(allRequest, 0));
	ASSERT_TRUE(!allRequest.CanReuse(localRequest, 0));
}

// FindReusableSearch must pick the AllSearch tab for an AllSearch request, not fall
// through to a same-query Local/Global/Kad tab that happens to be open.
TEST(AllSearchType, FindReusableSearchPicksCorrectType)
{
	CSearchList::CSearchParams params;
	params.searchString = "ubuntu";
	const CSearchRequest allRequest(AllSearch, params);
	const CSearchRequest globalRequest(GlobalSearch, params);

	const std::vector<CSearchReuseCandidate> pages{
		{ &globalRequest, 25 },
		{ &allRequest, 25 },
	};
	ASSERT_EQUALS(size_t(1), FindReusableSearch(pages, allRequest));
	ASSERT_EQUALS(size_t(0), FindReusableSearch(pages, globalRequest));
}

// AllSearch is a distinct type for reuse even before the first progress arrives
// (the pending-submission case).
TEST(AllSearchType, PendingAllSearchIsReusableBeforeProgress)
{
	CSearchList::CSearchParams params;
	params.searchString = "ubuntu";
	const CSearchRequest allRequest(AllSearch, params);
	std::vector<CSearchReuseCandidate> pages{ { &allRequest, std::nullopt } };
	ASSERT_EQUALS(size_t(0), FindReusableSearch(pages, allRequest));

	// A terminal status still blocks reuse, same as every other type.
	pages[0].progress = 0xffffu;
	ASSERT_EQUALS(pages.size(), FindReusableSearch(pages, allRequest));
	pages[0].progress = 0xfffeu;
	ASSERT_EQUALS(pages.size(), FindReusableSearch(pages, allRequest));
}

// Exercise the actual dropdown conversion before matching a submitted request:
// using the raw index 3 previously made the same All query start another tab.
TEST(AllSearchType, AllChoiceReusesSubmittedAllSearch)
{
	CSearchList::CSearchParams params;
	params.searchString = "ubuntu";
	const CSearchRequest submitted(AllSearch, params);
	const CSearchRequest selected(SearchTypeFromChoice(3, true), params);
	const std::vector<CSearchReuseCandidate> pages{ { &submitted, 25 } };
	ASSERT_EQUALS(size_t(0), FindReusableSearch(pages, selected));
	ASSERT_EQUALS(static_cast<int>(EC_SEARCH_ALL), SearchTypeFromChoice(3, true));
}

TEST(AllSearchType, ChoiceMappingPreservesSingleNetworkSearches)
{
	ASSERT_EQUALS(static_cast<int>(LocalSearch), SearchTypeFromChoice(0, true));
	ASSERT_EQUALS(static_cast<int>(GlobalSearch), SearchTypeFromChoice(1, true));
	ASSERT_EQUALS(static_cast<int>(KadSearch), SearchTypeFromChoice(2, true));
	ASSERT_EQUALS(static_cast<int>(KadSearch), SearchTypeFromChoice(0, false));
	ASSERT_EQUALS(-1, SearchTypeFromChoice(-1, true));
	ASSERT_EQUALS(-1, SearchTypeFromChoice(-1, false));
	ASSERT_EQUALS(-1, SearchTypeFromChoice(1, false));
	ASSERT_EQUALS(-1, SearchTypeFromChoice(4, true));
}
