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

#ifndef LOG_TEXT_H
#define LOG_TEXT_H

#include <wx/string.h>

// EC retains its historical display byte for older GUIs. Text/JSON exports are
// human records, so strip exactly that byte from each wire line. Keep unmarked
// errors and already clean records intact, including messages containing markers.
inline wxString ECLogToText(const wxString &text)
{
	wxString result;
	size_t start = 0;
	while (start < text.length()) {
		const size_t end = text.find('\n', start);
		const size_t length = end == wxString::npos ? text.length() - start : end - start + 1;
		const wxUniChar marker = text[start];
		const size_t skip = marker == ' ' || marker == '!' || marker == '.' ? 1 : 0;
		result += text.Mid(start + skip, length - skip);
		start += length;
	}
	return result;
}

#endif
