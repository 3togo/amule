//								-*- C++ -*-
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

#ifndef AMULE_LOOKUPTRACE_H
#define AMULE_LOOKUPTRACE_H

#include <array>
#include <cstdint>
#include <deque>
#include <map>
#include <utility>

namespace Kademlia
{
// Main-thread, transient diagnostics. Values own their data and never retain a
// routing contact. Bounds apply even when a remote node floods a lookup.
class CLookupTrace
{
public:
	using Address = std::pair<uint32_t, uint16_t>; // Kad (host order) IP and UDP port
	using ID = std::array<uint8_t, 16>;
	static constexpr size_t MaxPeers = 128;
	static constexpr size_t MaxEvents = 256;
	enum class Kind
	{
		Query,
		Reply,
		Referral,
		ItemQuery,
		ItemReply,
		Result
	};
	struct Peer
	{
		ID id{};
		uint64_t sent = 0;
		uint32_t requests = 0;
		uint32_t replies = 0;
		uint32_t results = 0;
		uint32_t itemRequests = 0;
		uint32_t itemReplies = 0;
		uint64_t roundTrip = 0;
		bool pending = false;
	};
	struct Event
	{
		Kind kind;
		Address peer;
		Address source;
		uint64_t tick;
		bool closer;
	};
	void Query(Address peer, const ID &id, uint64_t tick)
	{
		auto it = m_peers.find(peer);
		if (it == m_peers.end() && m_peers.size() == MaxPeers) {
			++m_omitted;
			return;
		}
		auto &row = m_peers[peer];
		row.id = id;
		row.sent = tick;
		++row.requests;
		row.pending = true;
		Record({ Kind::Query, peer, {}, tick, false });
	}
	bool Reply(Address peer, uint64_t tick)
	{
		auto it = m_peers.find(peer);
		if (it == m_peers.end() || !it->second.pending) {
			return false;
		}
		++it->second.replies;
		it->second.roundTrip = tick >= it->second.sent ? tick - it->second.sent : 0;
		it->second.pending = false;
		Record({ Kind::Reply, peer, {}, tick, false });
		return true;
	}
	void Referral(Address peer, Address source, uint64_t tick, bool closer)
	{
		if (m_peers.count(source)) {
			Record({ Kind::Referral, peer, source, tick, closer });
		}
	}
	void ItemRequest(Address peer, uint64_t tick)
	{
		auto it = m_peers.find(peer);
		if (it == m_peers.end()) {
			return;
		}
		++it->second.itemRequests;
		Record({ Kind::ItemQuery, peer, {}, tick, false });
	}
	void ItemReply(Address peer, uint64_t tick)
	{
		auto it = m_peers.find(peer);
		if (it == m_peers.end() || it->second.itemRequests == 0) {
			return;
		}
		++it->second.itemReplies;
		Record({ Kind::ItemReply, peer, {}, tick, false });
	}
	void Result(Address peer, uint64_t tick)
	{
		auto it = m_peers.find(peer);
		if (it == m_peers.end() || it->second.itemRequests == 0) {
			return;
		}
		++it->second.results;
		Record({ Kind::Result, peer, {}, tick, false });
	}
	const std::map<Address, Peer> &Peers() const { return m_peers; }
	const std::deque<Event> &Events() const { return m_events; }
	uint32_t Omitted() const { return m_omitted; }
	// This is an observation at the supplied ceiling, not a change to scheduling.
	size_t Overdue(uint64_t now, uint64_t ceiling) const
	{
		size_t count = 0;
		for (const auto &item : m_peers) {
			const auto &row = item.second;
			if (row.pending && now >= row.sent && now - row.sent >= ceiling) {
				++count;
			}
		}
		return count;
	}

private:
	void Record(const Event &event)
	{
		if (m_events.size() == MaxEvents) {
			m_events.pop_front();
			++m_omitted;
		}
		m_events.push_back(event);
	}
	std::map<Address, Peer> m_peers;
	std::deque<Event> m_events;
	uint32_t m_omitted = 0;
};
} // namespace Kademlia
#endif
