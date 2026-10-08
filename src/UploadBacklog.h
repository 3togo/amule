// This file is part of the aMule Project.
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef UPLOADBACKLOG_H
#define UPLOADBACKLOG_H
#include <cstdint>
#include <limits>
struct UploadBacklog
{
	uint64_t delivered = 0;
	uint64_t payloadBytes = 0;
	uint32_t requestedBlocks = 0;
	uint32_t pendingReads = 0;
	uint32_t rate = 0;
	bool socketHasData = false;
	bool snapshotBusy = false;
};
inline UploadBacklog BuildUploadBacklog(int64_t prepared, int64_t sent, uint64_t recent)
{
	UploadBacklog result;
	const uint64_t completed = sent > 0 ? static_cast<uint64_t>(sent) : 0;
	result.delivered = recent > std::numeric_limits<uint64_t>::max() - completed
				   ? std::numeric_limits<uint64_t>::max()
				   : completed + recent;
	const uint64_t ready = prepared > 0 ? static_cast<uint64_t>(prepared) : 0;
	result.payloadBytes = ready > result.delivered ? ready - result.delivered : 0;
	return result;
}
#endif
