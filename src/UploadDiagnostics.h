// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#pragma once
#include "UploadBacklog.h"
#include <sstream>
#include <algorithm>
namespace BroadbandUpload
{
class DiagnosticGate
{
public:
	bool Due(uint64_t now)
	{
		if (!initialized || now < last || now - last >= 10000) {
			initialized = true;
			last = now;
			return true;
		}
		return false;
	}

private:
	bool initialized = false;
	uint64_t last = 0;
};
struct DiagnosticTotals
{
	uint64_t queued = 0, pending = 0, noWork = 0, network = 0, requests = 0, diskPeers = 0,
		 busySnapshots = 0;
	void Add(const UploadBacklog &b)
	{
		if (b.snapshotBusy) {
			++busySnapshots;
			return;
		}
		queued += std::min(b.payloadBytes, std::numeric_limits<uint64_t>::max() - queued);
		requests += b.requestedBlocks;
		if (b.pendingReads)
			++diskPeers;
		pending += b.pendingReads;
		if (!b.pendingReads && !b.requestedBlocks && !b.payloadBytes && !b.socketHasData)
			++noWork;
		if (b.socketHasData)
			++network;
	}
	std::string Format(
		uint64_t budget, uint64_t rate, uint64_t slots, uint64_t waiting, uint64_t age) const
	{
		std::ostringstream s;
		s << "Upload broadband: budget_Bps=" << budget << " rate_Bps=" << rate << " slots=" << slots
		  << " waiting=" << waiting << " underfill_ms=" << age << " queued_payload=" << queued
		  << " pending_reads=" << pending << " no_peer_work=" << noWork
		  << " queued_output=" << network << " requested_blocks=" << requests
		  << " disk_pending_peers=" << diskPeers << " busy_snapshots=" << busySnapshots;
		return s.str();
	}
};
} // namespace BroadbandUpload
