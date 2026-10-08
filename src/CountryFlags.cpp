//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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

// Country-code resolution stays in the core; this cache is shared by both GUIs.
#include "CountryFlags.h"
#include "icons/icon_data.h"

#include <wx/artprov.h>
#include <cmath>

CCountryFlags::CCountryFlags()
{
	int count = 0;
	const auto *entries = amule_get_all_icons(&count);
	for (int i = 0; i < count; ++i) {
		const wxString name = wxString::FromUTF8(entries[i].name);
		if (name.StartsWith("flag_")) {
			m_codes.insert(name.Mid(5));
		}
	}
}

wxBitmap CCountryFlags::GetFlag(const wxString &code, const wxSize &logicalSize, double contentScale)
{
	if (logicalSize.x <= 0 || logicalSize.y <= 0 || !std::isfinite(contentScale) || contentScale <= 0) {
		return wxNullBitmap;
	}
	// Normalize missing codes once through a fixed set, without rescanning the
	// embedded table or retaining arbitrary unknown strings from the network.
	const wxString key = m_codes.count(code) ? code : wxString("unknown");
	auto it = m_flags.find(key);
	if (it == m_flags.end()) {
		FlagArtwork artwork;
		artwork.bundle =
			wxArtProvider::GetBitmapBundle("amule:flag_" + key, wxART_OTHER, wxSize(16, 12));
		it = m_flags.emplace(key, artwork).first;
	}
	auto &artwork = it->second;
	if (!artwork.bundle.IsOk()) {
		return wxNullBitmap;
	}
	const auto sizeKey = std::make_tuple(logicalSize.x, logicalSize.y, contentScale);
	const auto cached = artwork.bitmaps.find(sizeKey);
	if (cached != artwork.bitmaps.end()) {
		return cached->second;
	}
	const wxSize pixels(wxRound(logicalSize.x * contentScale), wxRound(logicalSize.y * contentScale));
	wxBitmap bitmap = artwork.bundle.GetBitmap(pixels);
	if (bitmap.IsOk()) {
		// SetScaleFactor may copy pixel data. Do it only for a new scale, then
		// share the completed bitmap on subsequent list-cell draws.
		bitmap.SetScaleFactor(contentScale);
		if (artwork.bitmaps.size() >= 8) {
			artwork.bitmaps.erase(artwork.bitmaps.begin());
		}
		artwork.bitmaps.emplace(sizeKey, bitmap);
	}
	return bitmap;
}
