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

#ifndef KADEMLIA_UDPVERIFICATIONEXPIRY_H
#define KADEMLIA_UDPVERIFICATIONEXPIRY_H

#include <cstdint>

namespace Kademlia
{

// Tracks inconclusive UDP rechecks, independently of the last firewall result.
// Times are monotonic milliseconds, supplied by the caller.
class CUDPVerificationExpiry
{
public:
	static constexpr uint64_t kRoundTimeoutMs = 6 * 60 * 1000;
	static constexpr unsigned kFailedRoundsToExpire = 2;
	static constexpr uint64_t kRecheckIntervalMs = 60 * 60 * 1000;
	static constexpr uint64_t kMaxVerificationAgeMs =
		kFailedRoundsToExpire * kRecheckIntervalMs + kRoundTimeoutMs;

	void Start(uint64_t now)
	{
		m_startedAt = now;
		m_running = true;
		m_counted = false;
	}

	// A running round can count only once. Its deadline is independent of the age of
	// the last completed result, so frequent recheck restarts cannot retain stale evidence.
	bool ShouldExpire(uint64_t now)
	{
		if (m_running && !m_counted && now >= m_startedAt && now - m_startedAt > kRoundTimeoutMs) {
			m_counted = true;
			if (m_failedRounds < kFailedRoundsToExpire) {
				++m_failedRounds;
			}
		}
		return m_failedRounds >= kFailedRoundsToExpire ||
		       (m_hasResult && now >= m_verifiedAt && now - m_verifiedAt >= kMaxVerificationAgeMs);
	}

	// Either an open or a firewalled result renews verification; cancellations do not.
	void RecordResult(uint64_t now)
	{
		m_running = false;
		m_failedRounds = 0;
		m_verifiedAt = now;
		m_hasResult = true;
	}

	void Reset() { *this = CUDPVerificationExpiry(); }

private:
	uint64_t m_startedAt = 0;
	uint64_t m_verifiedAt = 0;
	bool m_hasResult = false;
	unsigned m_failedRounds = 0;
	bool m_running = false;
	bool m_counted = false;
};

} // namespace Kademlia

#endif // KADEMLIA_UDPVERIFICATIONEXPIRY_H
