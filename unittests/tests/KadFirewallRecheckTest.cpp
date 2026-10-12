// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include <kademlia/kademlia/FirewallRecheck.h>
#include <kademlia/kademlia/UDPVerificationExpiry.h>

using namespace muleunit;
using Kademlia::CFirewallRecheck;

DECLARE_SIMPLE(KadFirewallRecheck)

namespace
{
const time_t kStart = 1000000;

// An open node after a finished recheck: the state the hourly recheck starts from.
CFirewallRecheck OpenNode()
{
	CFirewallRecheck fw;
	for (uint32_t i = 0; i < KADEMLIAFIREWALLCHECKS; ++i) {
		fw.AddAnswer(kStart - 100);
	}
	fw.AddConfirmation();
	fw.AddConfirmation();
	return fw;
}

void AddAllAnswers(CFirewallRecheck &fw, time_t now)
{
	for (uint32_t i = 0; i < KADEMLIAFIREWALLCHECKS; ++i) {
		fw.AddAnswer(now);
	}
}
} // namespace

TEST(KadFirewallRecheck, StartsFirewalledUntilConfirmed)
{
	CFirewallRecheck fw;
	ASSERT_TRUE(fw.IsFirewalled(kStart));
	AddAllAnswers(fw, kStart);
	ASSERT_TRUE(fw.IsFirewalled(kStart + 1));
	fw.AddConfirmation();
	fw.AddConfirmation();
	ASSERT_FALSE(fw.IsFirewalled(kStart + 2));
}

// The reported race: every IP answer arrives before the TCP confirmations. The node must not
// show as firewalled in between.
TEST(KadFirewallRecheck, AnswersBeforeConfirmationsKeepOpenState)
{
	CFirewallRecheck fw = OpenNode();
	fw.Start(kStart);
	ASSERT_FALSE(fw.IsFirewalled(kStart));
	AddAllAnswers(fw, kStart + 1);
	ASSERT_FALSE(fw.AwaitingAnswers());
	ASSERT_FALSE(fw.IsFirewalled(kStart + 1));
	fw.AddConfirmation();
	ASSERT_FALSE(fw.IsFirewalled(kStart + 2));
	fw.AddConfirmation();
	ASSERT_FALSE(fw.IsFirewalled(kStart + 2));
}

TEST(KadFirewallRecheck, ConfirmationsBeforeAnswersKeepOpenState)
{
	CFirewallRecheck fw = OpenNode();
	fw.Start(kStart);
	fw.AddConfirmation();
	fw.AddConfirmation();
	ASSERT_FALSE(fw.IsFirewalled(kStart + 1));
	AddAllAnswers(fw, kStart + 2);
	ASSERT_FALSE(fw.IsFirewalled(kStart + 2));
}

// A node that really became unreachable still turns firewalled, once the grace period is over.
TEST(KadFirewallRecheck, NoConfirmationTurnsFirewalledAfterGrace)
{
	CFirewallRecheck fw = OpenNode();
	fw.Start(kStart);
	AddAllAnswers(fw, kStart + 1);
	ASSERT_FALSE(fw.IsFirewalled(kStart + CFirewallRecheck::kGraceSeconds));
	ASSERT_TRUE(fw.IsFirewalled(kStart + 1 + CFirewallRecheck::kGraceSeconds));
	// One confirmation is not enough.
	fw.AddConfirmation();
	ASSERT_TRUE(fw.IsFirewalled(kStart + 2 + CFirewallRecheck::kGraceSeconds));
}

// A recheck started while one is still running keeps the state that was showing.
TEST(KadFirewallRecheck, RestartDuringRecheckKeepsShownState)
{
	CFirewallRecheck fw = OpenNode();
	fw.Start(kStart);
	fw.Start(kStart + 5);
	ASSERT_FALSE(fw.IsFirewalled(kStart + 5));
}

// More answers than requested must not restart the grace period.
TEST(KadFirewallRecheck, LateAnswersDoNotExtendGrace)
{
	CFirewallRecheck fw = OpenNode();
	fw.Start(kStart);
	AddAllAnswers(fw, kStart);
	fw.AddAnswer(kStart + 50);
	ASSERT_TRUE(fw.IsFirewalled(kStart + CFirewallRecheck::kGraceSeconds));
}

DECLARE_SIMPLE(KadUDPVerificationExpiry)

namespace
{
using Kademlia::CUDPVerificationExpiry;
constexpr uint64_t kUDPStart = 1000000;
constexpr uint64_t kUDPNextRound = kUDPStart + CUDPVerificationExpiry::kRecheckIntervalMs;
constexpr uint64_t kUDPTimeout = CUDPVerificationExpiry::kRoundTimeoutMs + 1;
} // namespace

TEST(KadUDPVerificationExpiry, TwoFailedRoundsExpire)
{
	CUDPVerificationExpiry expiry;
	expiry.Start(kUDPStart);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + kUDPTimeout));
	expiry.Start(kUDPNextRound);
	ASSERT_TRUE(expiry.ShouldExpire(kUDPNextRound + kUDPTimeout));
}

TEST(KadUDPVerificationExpiry, PollingCannotCountOneRoundTwice)
{
	CUDPVerificationExpiry expiry;
	expiry.Start(kUDPStart);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + kUDPTimeout));
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound));
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound + kUDPTimeout));
}

TEST(KadUDPVerificationExpiry, OneFailedHourlyRecheckAllowsNextRoundToFinish)
{
	CUDPVerificationExpiry expiry;
	const uint64_t interval = CUDPVerificationExpiry::kRecheckIntervalMs;
	const uint64_t initialResult = kUDPStart + 60 * 1000;
	const uint64_t secondRecheck = kUDPStart + 2 * interval;
	expiry.Start(kUDPStart);
	expiry.RecordResult(initialResult);
	expiry.Start(kUDPStart + interval);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + interval + kUDPTimeout));
	ASSERT_FALSE(expiry.ShouldExpire(secondRecheck));
	expiry.Start(secondRecheck);
	// The old two-hour age limit expired here, while this round could still answer.
	ASSERT_FALSE(expiry.ShouldExpire(secondRecheck + 90 * 1000));
	const uint64_t result = secondRecheck + CUDPVerificationExpiry::kRoundTimeoutMs;
	ASSERT_FALSE(expiry.ShouldExpire(result));
	expiry.RecordResult(result);
	ASSERT_FALSE(expiry.ShouldExpire(result + kUDPTimeout));
}

TEST(KadUDPVerificationExpiry, TimeoutBoundary)
{
	CUDPVerificationExpiry expiry;
	expiry.Start(kUDPStart);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + kUDPTimeout));
	expiry.Start(kUDPNextRound);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound + CUDPVerificationExpiry::kRoundTimeoutMs));
	ASSERT_TRUE(expiry.ShouldExpire(kUDPNextRound + kUDPTimeout));
}

TEST(KadUDPVerificationExpiry, SuccessBreaksFailureSequence)
{
	CUDPVerificationExpiry expiry;
	expiry.Start(kUDPStart);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + kUDPTimeout));
	expiry.Start(kUDPNextRound);
	expiry.RecordResult(kUDPNextRound); // verified open
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound + kUDPTimeout));
	expiry.Start(kUDPNextRound + kUDPTimeout);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound + 2 * kUDPTimeout));
}

TEST(KadUDPVerificationExpiry, FirewalledResultBreaksFailureSequence)
{
	CUDPVerificationExpiry expiry;
	expiry.Start(kUDPStart);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + kUDPTimeout));
	expiry.Start(kUDPNextRound);
	expiry.RecordResult(kUDPNextRound); // completed firewalled result is evidence too
	expiry.Start(kUDPNextRound + kUDPTimeout);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound + 2 * kUDPTimeout));
}

TEST(KadUDPVerificationExpiry, LateSuccessRestoresFreshSequence)
{
	CUDPVerificationExpiry expiry;
	expiry.Start(kUDPStart);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + kUDPTimeout));
	expiry.Start(kUDPNextRound);
	ASSERT_TRUE(expiry.ShouldExpire(kUDPNextRound + kUDPTimeout));
	expiry.RecordResult(kUDPNextRound + kUDPTimeout);
	expiry.Start(kUDPNextRound + kUDPTimeout);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound + 2 * kUDPTimeout));
}

TEST(KadUDPVerificationExpiry, ResetDropsPreviousSession)
{
	CUDPVerificationExpiry expiry;
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart));
	expiry.Start(kUDPStart);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + kUDPTimeout));
	expiry.Reset();
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound));
	expiry.Start(kUDPNextRound);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPNextRound + kUDPTimeout));
}

TEST(KadUDPVerificationExpiry, RecheckRestartsCannotExtendMaximumAge)
{
	CUDPVerificationExpiry expiry;
	expiry.RecordResult(kUDPStart);
	expiry.Start(kUDPStart);
	const uint64_t interval = 5 * 60 * 1000;
	for (uint64_t elapsed = interval; elapsed < CUDPVerificationExpiry::kMaxVerificationAgeMs;
		elapsed += interval) {
		const uint64_t now = kUDPStart + elapsed;
		// Match ReCheckFirewallUDP: account for the old round, then start a new one.
		ASSERT_FALSE(expiry.ShouldExpire(now));
		expiry.Start(now);
	}
	const uint64_t deadline = kUDPStart + CUDPVerificationExpiry::kMaxVerificationAgeMs;
	ASSERT_FALSE(expiry.ShouldExpire(deadline - 1));
	ASSERT_TRUE(expiry.ShouldExpire(deadline));
	expiry.Start(deadline);
	ASSERT_TRUE(expiry.ShouldExpire(deadline + 1));
}

TEST(KadUDPVerificationExpiry, MaximumAgeAppliesWithNoRunningRound)
{
	CUDPVerificationExpiry expiry;
	expiry.RecordResult(kUDPStart);
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + CUDPVerificationExpiry::kMaxVerificationAgeMs - 1));
	ASSERT_TRUE(expiry.ShouldExpire(kUDPStart + CUDPVerificationExpiry::kMaxVerificationAgeMs));
}

TEST(KadUDPVerificationExpiry, CompletedResultRenewsMaximumAge)
{
	CUDPVerificationExpiry expiry;
	expiry.RecordResult(kUDPStart);
	expiry.Start(kUDPNextRound);
	expiry.RecordResult(kUDPNextRound);
	const uint64_t deadline = kUDPNextRound + CUDPVerificationExpiry::kMaxVerificationAgeMs;
	ASSERT_FALSE(expiry.ShouldExpire(kUDPStart + CUDPVerificationExpiry::kMaxVerificationAgeMs));
	ASSERT_FALSE(expiry.ShouldExpire(deadline - 1));
	ASSERT_TRUE(expiry.ShouldExpire(deadline));
}

TEST(KadUDPVerificationExpiry, ResetClearsAgeAndZeroIsAValidResultTime)
{
	CUDPVerificationExpiry expiry;
	expiry.RecordResult(0);
	ASSERT_FALSE(expiry.ShouldExpire(CUDPVerificationExpiry::kMaxVerificationAgeMs - 1));
	ASSERT_TRUE(expiry.ShouldExpire(CUDPVerificationExpiry::kMaxVerificationAgeMs));
	expiry.Reset();
	ASSERT_FALSE(expiry.ShouldExpire(CUDPVerificationExpiry::kMaxVerificationAgeMs));
}
