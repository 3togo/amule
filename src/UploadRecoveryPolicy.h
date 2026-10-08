// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#pragma once
#include "UploadBacklog.h"
namespace BroadbandUpload
{
class StallProgress
{
public:
	bool Recycle(uint64_t now, const UploadBacklog &b, bool eligible)
	{
		if (!initialized || now < last || now - last > 5000 || b.delivered != delivered ||
			b.pendingReads || b.snapshotBusy || b.rate || !eligible) {
			since = now;
			initialized = true;
		}
		last = now;
		delivered = b.delivered;
		const bool noWork = !b.requestedBlocks && !b.payloadBytes && !b.socketHasData;
		return eligible && !b.pendingReads && !b.snapshotBusy && now >= since &&
		       now - since >= (noWork ? 30000u : 60000u);
	}

private:
	bool initialized = false;
	uint64_t since = 0, last = 0, delivered = 0;
};
} // namespace BroadbandUpload
