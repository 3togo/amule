#include <muleunit/test.h>
#include "DiskSpaceBudget.h"
using namespace muleunit;
DECLARE_SIMPLE(DiskSpaceBudget)
TEST(DiskSpaceBudget, CumulativeAndSameVolume)
{
	CDiskSpaceBudget b;
	b.Set("disk", 1000, 100);
	ASSERT_TRUE(b.Reserve("disk", 600, "disk", 900));
	ASSERT_FALSE(b.Reserve("disk", 400, "disk", 900));
	ASSERT_TRUE(b.Reserve("disk", 300, "disk", 900));
}
TEST(DiskSpaceBudget, CrossVolumeIsAtomic)
{
	CDiskSpaceBudget b;
	b.Set("temp", 1000, 100);
	b.Set("incoming", 500, 100);
	ASSERT_FALSE(b.Reserve("temp", 800, "incoming", 401));
	ASSERT_TRUE(b.Reserve("temp", 800, "incoming", 400));
	ASSERT_FALSE(b.Reserve("temp", 1, "unknown", 1));
}
TEST(DiskSpaceBudget, ActiveDemandAndUnderflow)
{
	CDiskSpaceBudget b;
	b.Set("disk", 100, 200);
	ASSERT_FALSE(b.Reserve("disk", 1, "disk", 0));
	b.Set("other", 1000, 100);
	b.AccountActive("other", UINT64_MAX);
	ASSERT_FALSE(b.Reserve("other", 1, "other", 0));
}

TEST(DiskSpaceBudget, SnapshotsDoNotResetReservations)
{
	CDiskSpaceBudget b;
	b.Set("volume-guid", 1000, 100);
	ASSERT_TRUE(b.Reserve("volume-guid", 700, "volume-guid", 0));
	b.Set("volume-guid", 1000, 100);
	ASSERT_FALSE(b.Reserve("volume-guid", 201, "volume-guid", 0));
	ASSERT_TRUE(b.Reserve("volume-guid", 200, "volume-guid", 0));
}
TEST(DiskSpaceBudget, CompletionCopySharesIncomingHeadroom)
{
	CDiskSpaceBudget b;
	b.Set("temp", 2000, 100);
	b.Set("incoming", 1000, 100);
	b.AccountActive("incoming", 700);
	ASSERT_FALSE(b.Reserve("temp", 100, "incoming", 201));
	ASSERT_TRUE(b.Reserve("temp", 100, "incoming", 200));
}
