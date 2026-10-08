// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "SourceSearchPolicy.h"
using namespace muleunit;
using namespace SourceSearchPolicy;
DECLARE_SIMPLE(SourceSearchPolicy)
TEST(SourceSearchPolicy, ScarcityWithinBudget)
{
	Candidate scarce = { 0, 50, 1000, 0 };
	Candidate abundant = { 2, 2, 1000, 2 };
	ASSERT_TRUE(Better(scarce, abundant, 2000));
	ASSERT_FALSE(Better(abundant, scarce, 2000));
	Candidate fewer = { 0, 10, 1000, 0 };
	ASSERT_TRUE(Better(fewer, scarce, 2000));
}
TEST(SourceSearchPolicy, OverdueFileCannotStarve)
{
	Candidate waiting = { 100, 100, 1000, 0 };
	Candidate recent = { 0, 0, 2000, 2 };
	ASSERT_TRUE(Better(waiting, recent, 1000 + kMaximumWait));
	ASSERT_FALSE(Better(recent, waiting, 1000 + kMaximumWait));
}
TEST(SourceSearchPolicy, StableTiesAndClockBoundary)
{
	Candidate a = { 1, 2, 5000, 1 };
	ASSERT_FALSE(Better(a, a, 4000));
	Candidate priority = a;
	priority.priority = 2;
	ASSERT_TRUE(Better(priority, a, 4000));
	Candidate initial = { 0, 0, 0, 1 };
	ASSERT_TRUE(Better(initial, a, kMaximumWait + 6000));
}
