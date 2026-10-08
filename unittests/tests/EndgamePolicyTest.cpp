#include <muleunit/test.h>
#include "EndgamePolicy.h"
using namespace muleunit;
DECLARE_SIMPLE(EndgamePolicy)
TEST(EndgamePolicy, ThresholdsAndLargeFiles)
{
	ASSERT_FALSE(EndgamePolicy::AtLeast(0, 0, 900));
	ASSERT_TRUE(EndgamePolicy::AtLeast(UINT64_MAX - 1, UINT64_MAX, 999));
	ASSERT_FALSE(EndgamePolicy::IsEndgame(500, 1000, 0));
	ASSERT_TRUE(EndgamePolicy::IsEndgame(700, 1000, 10));
	ASSERT_FALSE(EndgamePolicy::IsEndgame(699, 1000, 10));
}
TEST(EndgamePolicy, ReservationsAndTinyTails)
{
	ASSERT_EQUALS(uint64_t(16384), EndgamePolicy::ReservationBytes(100, 184320));
	ASSERT_EQUALS(uint64_t(10000), EndgamePolicy::ClampEnd(0, 10000, 8192));
	ASSERT_EQUALS(uint64_t(8191), EndgamePolicy::ClampEnd(0, 8192 + 3072 - 1, 8192));
	ASSERT_EQUALS(uint64_t(8192 + 3072 - 2), EndgamePolicy::ClampEnd(0, 8192 + 3072 - 2, 8192));
	ASSERT_EQUALS(uint64_t(16383), EndgamePolicy::ClampEnd(0, 184319, 16384));
}
TEST(EndgamePolicy, NeverCancelUsefulPayloadAndCooldown)
{
	ASSERT_FALSE(EndgamePolicy::MaySteal(70000, 1, true, 10, 100));
	ASSERT_FALSE(EndgamePolicy::MaySteal(50000, 1, false, 10, 100));
	ASSERT_TRUE(EndgamePolicy::MaySteal(70000, 1, false, 10, 100));
	ASSERT_FALSE(EndgamePolicy::MaySteal(70000, 1, false, 30, 100));
}
