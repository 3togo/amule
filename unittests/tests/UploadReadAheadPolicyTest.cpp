// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "UploadReadAheadPolicy.h"
using namespace muleunit;
DECLARE_SIMPLE(UploadReadAheadPolicy)
TEST(UploadReadAheadPolicy, FloorsCapsAndFairShare)
{
	using namespace BroadbandUpload;
	ASSERT_EQUALS(uint64_t(100), ReadAheadTarget(100000, false, 100, 1));
	ASSERT_EQUALS(uint64_t(1000), ReadAheadTarget(1, true, 100, 1));
	ASSERT_EQUALS(uint64_t(3200), ReadAheadTarget(UINT64_MAX, true, 100, 1));
	ASSERT_TRUE(ReadAheadTarget(UINT64_MAX, true, 184320, 250) <= kReadAheadBudget / 250);
}
TEST(UploadReadAheadPolicy, SharedBudgetNeverWraps)
{
	using namespace BroadbandUpload;
	ASSERT_TRUE(CanReadAhead(kReadAheadBudget - 100, 100));
	ASSERT_FALSE(CanReadAhead(kReadAheadBudget - 100, 101));
	ASSERT_FALSE(CanReadAhead(UINT64_MAX, 1));
	ASSERT_FALSE(CanReadAhead(0, UINT64_MAX));
}
