// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include <KadCallbackPolicy.h>

using namespace muleunit;

DECLARE_SIMPLE(KadCallbackPolicy)

TEST(KadCallbackPolicy, ReachabilityTruthTable)
{
	for (bool tcpFirewalled : { false, true }) {
		for (bool udpFirewalled : { false, true }) {
			for (bool udpVerified : { false, true }) {
				const bool direct = Kademlia::DirectCallbackAvailable(
					tcpFirewalled, udpFirewalled, udpVerified);
				const bool buddy =
					Kademlia::NeedsBuddy(tcpFirewalled, udpFirewalled, udpVerified);
				if (!tcpFirewalled) {
					ASSERT_FALSE(direct);
					ASSERT_FALSE(buddy);
				} else if (!udpFirewalled && udpVerified) {
					ASSERT_TRUE(direct);
					ASSERT_FALSE(buddy);
				} else {
					ASSERT_FALSE(direct);
					ASSERT_TRUE(buddy);
				}
			}
		}
	}
}
