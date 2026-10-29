//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef OFFERFILESPOLICY_H
#define OFFERFILESPOLICY_H

#include "MD4Hash.h"
#include <algorithm>
#include <set>
#include <array>
#include <cstdint>
class CTag;

// Draft v1 advertisement, validated as one packet; never stored in server.met.
class COfferFilesAdvertisement
{
public:
	static uint32_t LegacyPacketLimit(uint32_t soft, uint32_t hard);
	void AddTag(const CTag &tag);
	bool IsValid() const;
	uint32_t AdvertisedBatchLimit() const { return m_values[1]; }
	uint32_t AdvertisedIntervalMs() const { return m_values[2]; }
	uint32_t SoftLimit() const { return m_values[3]; }
	uint32_t HardLimit() const { return m_values[4]; }
	// Total records, including refreshes of previously offered hashes.
	uint32_t PacketLimit(uint32_t liveHard = 0) const;
	// Maximum new distinct candidates within that packet.
	uint32_t BatchLimit(uint32_t alreadyOffered, uint32_t liveSoft = 0, uint32_t liveHard = 0) const;
	uint32_t IntervalMs() const;
	uint32_t PublicationIntervalMs(bool accelerated, uint32_t legacyIntervalMs) const;

private:
	std::array<uint32_t, 5> m_values{};
	uint8_t m_seen = 0;
	bool m_invalid = false;
};

// Counts distinct candidates, including legacy offers before negotiation. Socket-owned.
class COfferFilesPublication
{
public:
	void Record(const CMD4Hash &hash) { m_offered.insert(hash); }
	bool Contains(const CMD4Hash &hash) const { return m_offered.count(hash) != 0; }
	uint32_t Count() const { return static_cast<uint32_t>(m_offered.size()); }
	bool Due(uint64_t now, uint32_t interval) const { return !m_sent || now - m_lastSent >= interval; }
	void Sent(uint64_t now)
	{
		m_sent = true;
		m_lastSent = now;
	}
	// A late advertisement binds the connection from login, so the server skipped
	// any earlier batch larger than its advertised limit.
	void SentBeforeAdvertisement(uint32_t records)
	{
		m_largestEarlyBatch = std::max(m_largestEarlyBatch, records);
	}
	bool EarlyBatchExceeds(uint32_t batchLimit) const { return m_largestEarlyBatch > batchLimit; }
	// Live UDP metadata may change while publication is idle. Recheck work once
	// when limits change, without rescanning the library on every core tick.
	bool UpdateLimits(uint32_t soft, uint32_t hard, uint32_t listedSoft, uint32_t listedHard)
	{
		const std::array<uint32_t, 4> limits{ { soft, hard, listedSoft, listedHard } };
		if (m_haveLimits && m_limits == limits) {
			return false;
		}
		m_limits = limits;
		m_haveLimits = true;
		return true;
	}
	void Reset() { *this = COfferFilesPublication(); }

private:
	std::array<uint32_t, 4> m_limits{};
	bool m_haveLimits = false;
	std::set<CMD4Hash> m_offered;
	uint64_t m_lastSent = 0;
	bool m_sent = false;
	uint32_t m_largestEarlyBatch = 0;
};

// Socket-owned snapshot. Begin before decoding so a malformed first packet
// consumes negotiation. Later SERVERIDENT packets cannot revise the snapshot.
// A valid snapshot remains binding until disconnect, even if the experimental
// preference is switched off after the login requested v1.
class COfferFilesConnectionPolicy
{
public:
	// A v1 server sends SERVERIDENT in the same login batch as the ID. Lugdunum
	// sends none at login, so wait only briefly for it.
	static constexpr uint64_t kIdentWaitMs = 5000;

	void BeginLogin(bool requestV1)
	{
		Reset();
		m_requested = requestV1;
	}
	void IdAssigned(uint64_t now)
	{
		if (!m_idAssigned) {
			m_idAssigned = true;
			m_idAt = now;
		}
	}
	// Wait for the requested post-login snapshot before sizing the first packet.
	bool CanPublish(uint64_t now) const
	{
		return !m_requested || (m_seen && !m_pending) ||
		       (!m_seen && m_idAssigned && now >= m_idAt && now - m_idAt >= kIdentWaitMs);
	}
	bool Requested() const { return m_requested; }
	bool AwaitingAdvertisement() const { return m_requested && !m_seen; }
	bool BeginAdvertisement();
	void RejectAdvertisement();
	void Commit(const COfferFilesAdvertisement &advertisement);
	void Reset() { *this = COfferFilesConnectionPolicy(); }
	const COfferFilesAdvertisement *Get() const { return m_valid ? &m_snapshot : nullptr; }

private:
	COfferFilesAdvertisement m_snapshot;
	bool m_requested = false;
	bool m_idAssigned = false;
	uint64_t m_idAt = 0;
	bool m_seen = false;
	bool m_pending = false;
	bool m_valid = false;
};
#endif
