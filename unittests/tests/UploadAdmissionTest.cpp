// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "UploadAdmissionPolicy.h"
#include "UploadUtilization.h"
#include <limits>
using namespace muleunit;
using namespace BroadbandUpload;
DECLARE_SIMPLE(UploadAdmission)
TEST(UploadAdmission, BoundedGrowthAfterSustainedUnderfill)
{
	ASSERT_EQUALS(20u, AdmissionLimit(20, 250, false));
	ASSERT_EQUALS(25u, AdmissionLimit(20, 250, true));
	ASSERT_EQUALS(250u, AdmissionLimit(240, 250, true));
	ASSERT_EQUALS(250u, AdmissionLimit(300, 250, true));
	ASSERT_EQUALS(0u, AdmissionLimit(0, 0, true));
	ASSERT_EQUALS(std::numeric_limits<uint32_t>::max(),
		AdmissionLimit(std::numeric_limits<uint32_t>::max() - 1,
			std::numeric_limits<uint32_t>::max(),
			true));
}
TEST(UploadAdmission, HysteresisCapacityChangeAndUnlimited)
{
	Utilization u;
	for (uint64_t now = 0; now <= 10000; now += 1000)
		u.Update(now, 1024 * 1024, 0);
	ASSERT_TRUE(u.Sustained(10000));
	u.Update(11000, 1024 * 1024, 1024 * 1024);
	ASSERT_FALSE(u.Sustained(11000));
	u.Update(12000, 0, 0);
	ASSERT_FALSE(u.Underfilled());
	u.Update(13000, 1000, 0);
	ASSERT_FALSE(u.Underfilled());
}
TEST(UploadAdmission, PausesAndClockRegressionResetWarmup)
{
	Utilization u;
	u.Update(100, kMinimumBudget, 0);
	u.Update(20000, kMinimumBudget, 0);
	ASSERT_FALSE(u.Sustained(20000));
	u.Update(50, kMinimumBudget, 0);
	ASSERT_FALSE(u.Sustained(50));
	ASSERT_FALSE(u.Sustained(20000));
}
