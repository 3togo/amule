// This file is part of the aMule Project.
// Copyright (c) 2026 aMule Team ( https://amule-org.github.io )
// Licensed under the GNU General Public License, version 2 or later.

#ifndef REMOTE_DOWNLOAD_EVENTS_H
#define REMOTE_DOWNLOAD_EVENTS_H

#include "Constants.h"

#include <cstdint>
#include <map>

// Only statuses actually received in this EC session establish a baseline. The first
// status for a file is a snapshot, even when the file is first seen during a later poll.
// Reset the history whenever a new EC session begins.
class CRemoteDownloadEvents
{
public:
	bool Observe(uint32_t id, uint8_t status, bool hasStatus = true)
	{
		if (!hasStatus) {
			return false;
		}
		const auto inserted = m_statuses.emplace(id, status);
		if (inserted.second) {
			return false;
		}
		uint8_t &previous = inserted.first->second;
		const bool completed = previous != PS_COMPLETE && status == PS_COMPLETE;
		previous = status;
		return completed;
	}

	void Forget(uint32_t id) { m_statuses.erase(id); }
	void Reset() { m_statuses.clear(); }

private:
	std::map<uint32_t, uint8_t> m_statuses;
};

#endif // REMOTE_DOWNLOAD_EVENTS_H
