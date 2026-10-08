// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#include <muleunit/test.h>
#include "UploadDiagnostics.h"
using namespace muleunit;
DECLARE_SIMPLE(UploadDiagnostics)
TEST(UploadDiagnostics, ThrottlingAndRegression)
{
	BroadbandUpload::DiagnosticGate gate;
	ASSERT_TRUE(gate.Due(0));
	ASSERT_FALSE(gate.Due(9999));
	ASSERT_TRUE(gate.Due(10000));
	ASSERT_TRUE(gate.Due(1));
}
TEST(UploadDiagnostics, AggregateHasNoPeerIdentifiers)
{
	BroadbandUpload::DiagnosticTotals d;
	UploadBacklog b;
	d.Add(b);
	b.pendingReads = 1;
	b.payloadBytes = 100;
	b.socketHasData = true;
	d.Add(b);
	ASSERT_EQUALS(uint64_t(1), d.noWork);
	ASSERT_EQUALS(uint64_t(1), d.pending);
	ASSERT_EQUALS(uint64_t(100), d.queued);
	ASSERT_TRUE(d.Format(0, 10, 2, 3, 0).find("budget_Bps=0") != std::string::npos);
}

TEST(UploadDiagnostics, BusySnapshotsAndPayloadOverflowAreExplicit)
{
	BroadbandUpload::DiagnosticTotals d;
	UploadBacklog b;
	b.snapshotBusy = true;
	d.Add(b);
	ASSERT_EQUALS(uint64_t(1), d.busySnapshots);
	ASSERT_EQUALS(uint64_t(0), d.noWork);
	b.snapshotBusy = false;
	b.payloadBytes = UINT64_MAX;
	d.Add(b);
	b.payloadBytes = 1;
	d.Add(b);
	ASSERT_EQUALS(UINT64_MAX, d.queued);
}
