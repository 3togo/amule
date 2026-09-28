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

#include <muleunit/test.h>
#include "SearchSourceCount.h"
#include "SearchSourceFormat.h"
#include <algorithm>
#include <array>

using namespace muleunit;
DECLARE_SIMPLE(SearchSourceCount)

TEST(SearchSourceCount, MixedReportsAreIndependentOfArrivalOrder)
{
	const std::array<CSearchSourceCount, 4> reports{ CSearchSourceCount(10, false),
		CSearchSourceCount(15, false),
		CSearchSourceCount(20, true),
		CSearchSourceCount(20, true) };
	std::array<int, 4> order{ 0, 1, 2, 3 };
	do {
		CSearchSourceCount total;
		for (int index : order) {
			total.Merge(reports[index]);
		}
		ASSERT_EQUALS(uint32_t(25), total.Total());
		ASSERT_EQUALS(uint32_t(25), total.Ed2k());
		ASSERT_EQUALS(uint32_t(20), total.Kad());
	} while (std::next_permutation(order.begin(), order.end()));
}

TEST(SearchSourceCount, FilenameGroupsRetainBothNetworkContributions)
{
	CSearchSourceCount firstName(10, false);
	firstName.Merge(CSearchSourceCount(20, true));
	CSearchSourceCount secondName(15, false);
	secondName.Merge(CSearchSourceCount(20, true));
	// Combining only the displayed child counts would incorrectly give 40.
	auto parent = firstName;
	parent.Merge(secondName);
	ASSERT_EQUALS(uint32_t(25), parent.Total());
	parent.Merge(CSearchSourceCount(30, true));
	ASSERT_EQUALS(uint32_t(30), parent.Total());
}

TEST(SearchSourceCount, SingleNetworkCountsKeepTheirExistingSemantics)
{
	CSearchSourceCount servers(10, false);
	servers.Merge(CSearchSourceCount(15, false));
	ASSERT_EQUALS(uint32_t(25), servers.Total());
	CSearchSourceCount kad(20, true);
	kad.Merge(CSearchSourceCount(20, true));
	kad.Merge(CSearchSourceCount(5, true));
	ASSERT_EQUALS(uint32_t(20), kad.Total());
	ASSERT_EQUALS(uint32_t(0), CSearchSourceCount().Total());
}

TEST(SearchSourceCount, AllDisplayPreservesCompleteAndClientCounts)
{
	const auto counts = CSearchSourceCount::FromNetworks(10, 50);
	ASSERT_EQUALS(
		wxString::FromUTF8("50 (3) · E:10 K:50"), FormatSearchSources(counts.Total(), 3, 0, counts));
	ASSERT_EQUALS(wxString::FromUTF8("50 (3) [2] · E:10 K:50"),
		FormatSearchSources(counts.Total(), 3, 2, counts));
}

TEST(SearchSourceCount, UnknownSplitAndSingleNetworkFallbackAreDistinct)
{
	ASSERT_EQUALS(wxString("50 (3) [2]"), FormatSearchSources(50, 3, 2));
	ASSERT_EQUALS(wxString::FromUTF8("10 · E:10 K:0"),
		FormatSearchSources(10, 0, 0, CSearchSourceCount::FromNetworks(10, 0)));
	ASSERT_EQUALS(wxString::FromUTF8("50 · E:0 K:50"),
		FormatSearchSources(50, 0, 0, CSearchSourceCount::FromNetworks(0, 50)));
}
