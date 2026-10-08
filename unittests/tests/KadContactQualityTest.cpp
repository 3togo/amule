// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "kademlia/routing/ContactQuality.h"
using namespace muleunit;
using namespace Kademlia;
DECLARE_SIMPLE(KadContactQuality)
TEST(KadContactQuality, ObservedResponsesAndVerification)
{
	const time_t now = 100000;
	ContactQuality unknown = { false, false, false, 3, 0 };
	ASSERT_EQUALS(20u, LocalContactQuality(unknown, now));
	ContactQuality live = { true, true, true, 0, now };
	ASSERT_EQUALS(1010u, LocalContactQuality(live, now));
	live.type = 4;
	ASSERT_EQUALS(0u, LocalContactQuality(live, now));
	live.type = 0;
	live.lastResponse = now + 1;
	ASSERT_EQUALS(920u, LocalContactQuality(live, now));
}
TEST(KadContactQuality, WeakContactsFirstWithoutStarvingHealthyContacts)
{
	const time_t now = 100000;
	ASSERT_TRUE(BetterContactProbe(20, now - 1, 1010, now - 2, now));
	ASSERT_TRUE(BetterContactProbe(1010, now - 1800, 20, now - 1, now));
	ASSERT_FALSE(BetterContactProbe(20, now - 1, 1010, now - 1800, now));
	ASSERT_FALSE(BetterContactProbe(20, now - 1, 20, now - 1, now));
}
TEST(KadContactQuality, DueAndDeadContactBoundaries)
{
	ASSERT_FALSE(ContactProbeDue(4, 0, 100));
	ASSERT_FALSE(ContactProbeDue(3, 100, 100));
	ASSERT_TRUE(ContactProbeDue(3, 99, 100));
	ASSERT_FALSE(ContactProbeDue(3, 101, 100));
	ContactQuality live = { true, false, false, 2, 1000 };
	ASSERT_EQUALS(550u, LocalContactQuality(live, 1900));
	ASSERT_EQUALS(530u, LocalContactQuality(live, 1901));
}
