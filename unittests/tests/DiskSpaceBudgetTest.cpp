#include <muleunit/test.h>
#include "DiskSpaceBudget.h"
using namespace muleunit;
DECLARE_SIMPLE(DiskSpaceBudget)
TEST(DiskSpaceBudget, CumulativeAndSameVolume) {
    CDiskSpaceBudget b; b.Set("disk", 1000, 100);
    ASSERT_TRUE(b.Reserve("disk", 600, "disk", 900));
    ASSERT_FALSE(b.Reserve("disk", 400, "disk", 900));
    ASSERT_TRUE(b.Reserve("disk", 300, "disk", 900));
}
TEST(DiskSpaceBudget, CrossVolumeIsAtomic) {
    CDiskSpaceBudget b; b.Set("temp", 1000, 100); b.Set("incoming", 500, 100);
    ASSERT_FALSE(b.Reserve("temp", 800, "incoming", 401));
    ASSERT_TRUE(b.Reserve("temp", 800, "incoming", 400));
    ASSERT_FALSE(b.Reserve("temp", 1, "unknown", 1));
}
TEST(DiskSpaceBudget, ActiveDemandAndUnderflow) {
    CDiskSpaceBudget b; b.Set("disk", 100, 200);
    ASSERT_FALSE(b.Reserve("disk", 1, "disk", 0));
    b.Set("other", 1000, 100); b.AccountActive("other", UINT64_MAX);
    ASSERT_FALSE(b.Reserve("other", 1, "other", 0));
}
