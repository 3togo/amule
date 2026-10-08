#include <muleunit/test.h>
#include "DownloadBufferPolicy.h"
#include "DownloadBandwidthThrottler.h"
#include "EMSocket.h"
void CEMSocket::WakeIfPaused() {}
using namespace muleunit;
DECLARE_SIMPLE(DownloadBufferPolicy)
TEST(DownloadBufferPolicy, AdaptiveBudget) {
    ASSERT_EQUALS(uint64_t(0), DownloadBufferPolicy::Budget(0, UINT64_MAX));
    ASSERT_EQUALS(uint64_t(1024*1024), DownloadBufferPolicy::Budget(64, 4*1024*1024));
    ASSERT_EQUALS(uint64_t(512*1024*1024), DownloadBufferPolicy::Budget(UINT32_MAX, UINT64_MAX));
    ASSERT_EQUALS(uint64_t(0), DownloadBufferPolicy::Headroom(10, 11));
}
TEST(DownloadBufferPolicy, DiskBackpressureAppliesWithUnlimitedBandwidth) {
    auto &b = CDownloadBandwidthThrottler::Get();
    b.RefillBudget(0, 100, 100);
    ASSERT_EQUALS(uint32(80), b.Reserve(80));
    ASSERT_EQUALS(uint32(20), b.Reserve(80));
    ASSERT_EQUALS(uint32(0), b.Reserve(1));
    b.Refund(10);
    ASSERT_EQUALS(uint32(10), b.Reserve(20));
    b.RefillBudget(0, 100, 0);
    ASSERT_EQUALS(uint32(0), b.Reserve(1));
    b.RefillBudget(0, 100, 100);
    ASSERT_EQUALS(uint32(100), b.Reserve(200));
    b.RefillBudget(0, 100);
    ASSERT_EQUALS(uint32(200), b.Reserve(200));
}
TEST(DownloadBufferPolicy, BandwidthAndMemoryIntersect) {
    auto &b = CDownloadBandwidthThrottler::Get();
    b.RefillBudget(1, 100, 50);
    ASSERT_EQUALS(uint32(50), b.Reserve(200));
    ASSERT_EQUALS(uint32(0), b.Reserve(1));
}
