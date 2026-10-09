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

#ifndef USEREVENTCOMMAND_H
#define USEREVENTCOMMAND_H

#include <wx/cmdline.h>

#include <utility>
#include <vector>

// Parse only the user's template. Event values must never become command syntax,
// and placeholder-looking text inside a value must remain literal.
inline wxArrayString BuildUserEventCommand(
	const wxString &command, const std::vector<std::pair<wxString, wxString>> &values)
{
#ifdef __WINDOWS__
	wxArrayString args = wxCmdLineParser::ConvertStringToArgs(command, wxCMD_LINE_SPLIT_DOS);
#else
	wxArrayString args = wxCmdLineParser::ConvertStringToArgs(command, wxCMD_LINE_SPLIT_UNIX);
#endif
	for (wxString &arg : args) {
		wxString expanded;
		for (size_t i = 0; i < arg.length();) {
			bool replaced = false;
			if (arg[i] == '%') {
				for (const auto &value : values) {
					if (arg.Mid(i, value.first.length()) == value.first) {
						expanded += value.second;
						i += value.first.length();
						replaced = true;
						break;
					}
				}
			}
			if (!replaced) {
				expanded += arg[i++];
			}
		}
		arg = expanded;
	}
	return args;
}

// Windows CreateProcess accepts a string, and wxWidgets 3.2's argv overload
// does not double backslashes before embedded quotes or the closing quote.
// Quote each argument using the Windows C runtime rules instead.
inline wxString BuildWindowsUserEventCommandLine(const wxArrayString &args)
{
	wxString command;
	for (const wxString &arg : args) {
		if (!command.empty()) {
			command += ' ';
		}
		command += '"';
		size_t backslashes = 0;
		for (const wxUniChar ch : arg) {
			if (ch == '\\') {
				++backslashes;
				continue;
			}
			const size_t count = ch == '"' ? backslashes * 2 + 1 : backslashes;
			command += wxString('\\', count);
			command += ch;
			backslashes = 0;
		}
		command += wxString('\\', backslashes * 2);
		command += '"';
	}
	return command;
}

#endif // USEREVENTCOMMAND_H
