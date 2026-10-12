//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

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
