// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "UploadRecoveryPolicy.h"
#include "UploadSnapshotLock.h"
using namespace muleunit;
DECLARE_SIMPLE(UploadRecoveryPolicy)
TEST(UploadRecoveryPolicy, GraceProgressAndDiskPending)
{
	BroadbandUpload::StallProgress p;
	UploadBacklog b;
	for (uint64_t t = 0; t < 30000; t += 1000)
		ASSERT_FALSE(p.Recycle(t, b, true));
	ASSERT_TRUE(p.Recycle(30000, b, true));
	b.pendingReads = 1;
	ASSERT_FALSE(p.Recycle(31000, b, true));
	b.pendingReads = 0;
	b.delivered = 1;
	ASSERT_FALSE(p.Recycle(32000, b, true));
	ASSERT_FALSE(p.Recycle(100, b, true));
}
TEST(UploadRecoveryPolicy, QueuedNetworkNeedsLongerGrace)
{
	BroadbandUpload::StallProgress p;
	UploadBacklog b;
	b.payloadBytes = 100;
	for (uint64_t t = 0; t < 60000; t += 1000)
		ASSERT_FALSE(p.Recycle(t, b, true));
	ASSERT_TRUE(p.Recycle(60000, b, true));
	ASSERT_FALSE(p.Recycle(61000, b, false));
}

TEST(UploadRecoveryPolicy, PayloadAccountingIncludesUnconsumedSocketCounter)
{
	auto b = BuildUploadBacklog(1000, 200, 300);
	ASSERT_EQUALS(uint64_t(500), b.delivered);
	ASSERT_EQUALS(uint64_t(500), b.payloadBytes);
	b = BuildUploadBacklog(-1, -1, UINT64_MAX);
	ASSERT_EQUALS(UINT64_MAX, b.delivered);
	ASSERT_EQUALS(uint64_t(0), b.payloadBytes);
	b = BuildUploadBacklog(100, 100, UINT64_MAX);
	ASSERT_EQUALS(UINT64_MAX, b.delivered);
	ASSERT_EQUALS(uint64_t(0), b.payloadBytes);
}

TEST(UploadRecoveryPolicy, SlowProgressAndTimerPausesAreNotStalls)
{
	BroadbandUpload::StallProgress p;
	UploadBacklog b;
	b.payloadBytes = 100;
	b.rate = 1;
	for (uint64_t t = 0; t <= 120000; t += 1000)
		ASSERT_FALSE(p.Recycle(t, b, true));
	b.rate = 0;
	ASSERT_FALSE(p.Recycle(200000, b, true));
	ASSERT_FALSE(p.Recycle(199000, b, true));
}

TEST(UploadRecoveryPolicy, BusySnapshotIsNotPeerInactivity)
{
	BroadbandUpload::StallProgress p;
	UploadBacklog b;
	b.snapshotBusy = true;
	for (uint64_t t = 0; t <= 120000; t += 1000)
		ASSERT_FALSE(p.Recycle(t, b, true));
}
TEST(UploadRecoveryPolicy, SnapshotTryLockDefersAndReleases)
{
	wxMutex mutex;
	ASSERT_EQUALS(wxMUTEX_NO_ERROR, mutex.Lock());
	{
		UploadSnapshotLock busy(mutex);
		ASSERT_FALSE(static_cast<bool>(busy));
	}
	mutex.Unlock();
	{
		UploadSnapshotLock acquired(mutex);
		ASSERT_TRUE(static_cast<bool>(acquired));
	}
	ASSERT_EQUALS(wxMUTEX_NO_ERROR, mutex.TryLock());
	mutex.Unlock();
}
