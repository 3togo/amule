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

#include "LookupDiagnostics.h"
#include "../../NetworkFunctions.h"
#include <common/Format.h>
#include <wx/intl.h>
#include <algorithm>
using namespace Kademlia;
namespace
{
wxString DescribeLookup(const LookupSnapshot &snapshot, bool active, uint64_t now)
{
	const auto &trace = snapshot.trace;
	wxString text = CFormat(_("Lookup %s (type %u, %s): %u peers, %u overdue routing requests, %u "
				  "omitted records\n")) %
			snapshot.target % snapshot.type % (active ? _("active") : _("finished")) %
			static_cast<unsigned>(trace.Peers().size()) %
			static_cast<unsigned>(trace.Overdue(now, 3000)) % trace.Omitted();
	for (const auto &item : trace.Peers()) {
		const auto &row = item.second;
		wxString id;
		for (auto byte : row.id) {
			id += wxString::Format("%02x", byte);
		}
		text += CFormat(_("  %s [%s]: %u routing requests, %u replies, last RTT %u ms, %u item "
				  "requests, %u result packets, %u result records\n")) %
			KadIPPortToString(item.first.first, item.first.second) % id % row.requests %
			row.replies % static_cast<unsigned>(row.roundTrip) % row.itemRequests %
			row.itemReplies % row.results;
	}
	for (const auto &event : trace.Events()) {
		if (event.kind == Kademlia::CLookupTrace::Kind::Referral) {
			text += CFormat(_("  +%u ms: %s referred %s (%s)\n")) %
				static_cast<unsigned>(event.tick - snapshot.started) %
				KadIPPortToString(event.source.first, event.source.second) %
				KadIPPortToString(event.peer.first, event.peer.second) %
				(event.closer ? _("closer to target") : _("no closer to target"));
		}
	}
	return text + "\n";
}
} // namespace
wxString Kademlia::FormatLookupDiagnostics(
	const std::vector<LookupSnapshot> &active, const std::deque<LookupSnapshot> &recent, uint64_t now)
{

	// EC strings have a 16-bit byte length. 12000 Unicode characters fit
	// even as four-byte UTF-8, including the explanatory suffix.
	constexpr size_t maxCharacters = 12000;
	wxString text = _("Overdue means a routing request unanswered for at least 3 seconds; late replies "
			  "may still arrive. Result records are received records, not unique files. Only "
			  "bounded recent history is retained.\n\n");
	size_t count = 0;
	for (const auto &snapshot : active) {
		if (count++ == 16) {
			text += _("Additional active lookups omitted.\n");
			break;
		}
		text += DescribeLookup(snapshot, true, now)
				.Left(maxCharacters - std::min(maxCharacters, text.length()));
		if (text.length() >= maxCharacters) {
			break;
		}
	}
	for (auto it = recent.rbegin(); it != recent.rend(); ++it) {
		text += DescribeLookup(*it, false, it->finished)
				.Left(maxCharacters - std::min(maxCharacters, text.length()));
		if (text.length() >= maxCharacters) {
			break;
		}
	}
	if (active.empty() && recent.empty()) {
		text += _("No Kad lookup history available.");
	}
	if (text.length() >= maxCharacters) {
		text += _("\nDiagnostic output truncated.\n");
	}
	return text;
}
