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

#ifndef SEARCHSOURCEFORMAT_H
#define SEARCHSOURCEFORMAT_H

#include "SearchSourceCount.h"
#include <common/Format.h>
#include <optional>

// Shared by native and remote result models. Unknown network splits keep the
// legacy display; zero is meaningful when a known ALL search used one network.
inline wxString FormatSearchSources(uint32_t total,
	uint32_t complete,
	size_t clients,
	const std::optional<CSearchSourceCount> &networks = std::nullopt)
{
	wxString text = CFormat("%u") % total;
	if (complete) {
		text += CFormat(" (%u)") % complete;
	}
	if (clients) {
		text += CFormat(" [%u]") % clients;
	}
	if (networks) {
		text += CFormat(wxString::FromUTF8(" · E:%u K:%u")) % networks->Ed2k() % networks->Kad();
	}
	return text;
}

#endif
