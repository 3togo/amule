// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "UdpQueuePolicy.h"
#include <limits>
using namespace muleunit;
using namespace UdpQueuePolicy;
DECLARE_SIMPLE(UdpQueuePolicy)
TEST(UdpQueuePolicy, PacketAndByteLimitsAreIndependent)
{
	ASSERT_TRUE(CanAccept(0, 0, kClientBytes - kPacketOverhead, kClientPackets, kClientBytes));
	ASSERT_FALSE(CanAccept(kClientPackets, 0, 0, kClientPackets, kClientBytes));
	ASSERT_FALSE(CanAccept(1, kClientBytes, 1, kClientPackets, kClientBytes));
	ASSERT_FALSE(CanAccept(0, 0, kServerBytes, kServerPackets, kServerBytes));
	ASSERT_TRUE(CanAccept(kServerPackets - 1, 0, 1, kServerPackets, kServerBytes));
}
TEST(UdpQueuePolicy, OversizedAndOverflowInputsRejected)
{
	const size_t maximum = std::numeric_limits<size_t>::max();
	ASSERT_FALSE(CanAccept(0, 0, maximum, kClientPackets, kClientBytes));
	ASSERT_FALSE(CanAccept(0, maximum, 1, kClientPackets, kClientBytes));
	ASSERT_FALSE(CanAccept(0, kClientBytes - 31, 0, kClientPackets, kClientBytes));
	ASSERT_FALSE(CanAccept(0, 0, 0, 0, kClientBytes));
}
TEST(UdpQueuePolicy, ExpirationAtBoundaryAndClockRegression)
{
	ASSERT_FALSE(Expired(100, 109, 10));
	ASSERT_TRUE(Expired(100, 110, 10));
	ASSERT_FALSE(Expired(100, 99, 10));
}
