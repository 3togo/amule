// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "UploadRetentionPolicy.h"
using namespace muleunit;
using namespace BroadbandUpload;
DECLARE_SIMPLE(UploadRetention)
TEST(UploadRetention, ProductiveSessionHasBoundedExtension)
{
	SessionRetention s;
	ASSERT_TRUE(s.Retain(0, true, true, 10000, 3000));
	ASSERT_TRUE(s.Retain(119999, true, true, 10000, 3000));
	ASSERT_FALSE(s.Retain(120000, true, true, 10000, 3000));
	ASSERT_FALSE(s.Retain(120001, true, true, 10000, 3000));
}
TEST(UploadRetention, CapacityAndProductivityRemainRequired)
{
	SessionRetention s;
	ASSERT_FALSE(s.Retain(0, false, true, 10000, 3000));
	ASSERT_FALSE(s.Retain(0, true, false, 10000, 3000));
	ASSERT_FALSE(s.Retain(0, true, true, 0, 0));
	ASSERT_FALSE(s.Retain(0, true, true, 2000, 3000));
	ASSERT_TRUE(s.Retain(100, true, true, 3000, 3000));
	ASSERT_FALSE(s.Retain(101, true, false, 10000, 3000));
	ASSERT_FALSE(s.Retain(99, true, true, 10000, 3000));
}
