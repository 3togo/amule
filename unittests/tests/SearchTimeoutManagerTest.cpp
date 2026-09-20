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
// it under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
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

#include "SearchTimeoutManager.h"

#include <wx/app.h>
#include <wx/thread.h>

using namespace muleunit;

// The SearchTimeoutManager singleton starts a wxTimer in its constructor and
// stops it in the destructor. Without a running wx event loop the timer is
// never registered with the scheduler, so the destructor's Stop() raises a
// benign wx assert ("removing inexistent timer?"). That assert is a harness
// artifact, not a logic failure. muleunit's own ASSERT_* macros do not route
// through wxAssert, so swallowing wx asserts here keeps real test failures
// reporting correctly while letting the singleton tear down cleanly.
static void NoopAssertHandler(const wxString&, int, const wxString&, const wxString&, const wxString&) {}

DECLARE_SIMPLE(SearchTimeoutManager)

TEST(SearchTimeoutManager, RegisterUnregisterAndType)
{
	// Swallow the benign wx timer-destructor assert (see NoopAssertHandler
	// above). Must be set here, not at static-init, because UnitTestApp::
	// OnInit installs MuleUnitAssertHandler afterwards and would otherwise
	// overwrite it. muleunit's own ASSERT_* macros are unaffected.
	wxSetAssertHandler(NoopAssertHandler);

	auto& mgr = SearchTimeoutManager::Instance();
	mgr.unregisterSearch(42); // start clean

	ASSERT_TRUE(mgr.registerSearch(42, SearchTimeoutManager::LocalSearch));
	ASSERT_TRUE(mgr.isSearchRegistered(42));
	ASSERT_EQUALS((int)SearchTimeoutManager::LocalSearch, (int)mgr.getSearchType(42));
	ASSERT_EQUALS(size_t(1), mgr.getRegisteredSearchCount());

	mgr.unregisterSearch(42);
	ASSERT_TRUE(!mgr.isSearchRegistered(42));
	ASSERT_EQUALS(size_t(0), mgr.getRegisteredSearchCount());

	// Unknown search reports type -1.
	ASSERT_EQUALS((int)-1, (int)mgr.getSearchType(999));
}

TEST(SearchTimeoutManager, ConfigGettersSetters)
{
	auto& mgr = SearchTimeoutManager::Instance();

	mgr.setLocalSearchTimeout(123);
	ASSERT_EQUALS(123, mgr.getLocalSearchTimeout());
	mgr.setGlobalSearchTimeout(456);
	ASSERT_EQUALS(456, mgr.getGlobalSearchTimeout());
	mgr.setKadSearchTimeout(789);
	ASSERT_EQUALS(789, mgr.getKadSearchTimeout());

	// Restore defaults so other tests are not skewed.
	mgr.setLocalSearchTimeout(30000);
	mgr.setGlobalSearchTimeout(120000);
	mgr.setKadSearchTimeout(180000);
}

TEST(SearchTimeoutManager, ElapsedAndRemainingShrink)
{
	auto& mgr = SearchTimeoutManager::Instance();
	mgr.unregisterSearch(7);
	mgr.registerSearch(7, SearchTimeoutManager::GlobalSearch);

	int64_t remaining0 = mgr.getRemainingTime(7);
	ASSERT_TRUE(remaining0 > 0); // just registered, near full timeout left

	wxThread::Sleep(15);

	ASSERT_TRUE(mgr.getElapsedTime(7) >= 0);
	int64_t remaining1 = mgr.getRemainingTime(7);
	ASSERT_TRUE(remaining1 <= remaining0); // remaining time only shrinks

	mgr.unregisterSearch(7);
}

TEST(SearchTimeoutManager, TimeoutFiresCallbackAndCounts)
{
	auto& mgr = SearchTimeoutManager::Instance();
	mgr.unregisterSearch(11);
	mgr.resetStatistics();

	int fired = 0;
	mgr.setTimeoutCallback(
		[&fired](uint32_t, SearchTimeoutManager::SearchType, const wxString&) {
			fired++;
		});

	// A 1ms local timeout. Sleep well over a second so the wall-clock
	// elapsed time deterministically exceeds the timeout regardless of the
	// clock granularity wxDateTime::Now() offers in this build.
	mgr.setLocalSearchTimeout(1);
	mgr.registerSearch(11, SearchTimeoutManager::LocalSearch);

	wxThread::Sleep(1200);
	mgr.checkTimeouts();

	ASSERT_EQUALS(1, fired);
	ASSERT_EQUALS(size_t(1), mgr.getTotalTimeouts());

	mgr.unregisterSearch(11);
	mgr.setTimeoutCallback(nullptr);   // drop the capturing lambda
	mgr.setLocalSearchTimeout(30000);  // restore default
}
