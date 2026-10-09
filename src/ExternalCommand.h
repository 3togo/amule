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

#ifndef EXTERNALCOMMAND_H
#define EXTERNALCOMMAND_H

#include <wx/arrstr.h>
#include <utility>
#include <vector>

namespace ExternalCommand
{
// Parse the template once, then expand each placeholder once inside its argument.
// Empty results also signal rejected Windows values containing a double quote.
wxArrayString Build(const wxString &command,
	const std::vector<std::pair<wxString, wxString>> &values,
	bool *substituted = nullptr);

// Serialize arguments with Windows C runtime quoting, including trailing backslashes.
wxString BuildWindowsCommandLine(const wxArrayString &args);

// Spawn asynchronously using the AppImage-safe environment. False means no child
// was spawned; successful spawning does not establish successful child execution.
bool RunDetached(const wxString &description, const wxArrayString &args);
} // namespace ExternalCommand

#endif // EXTERNALCOMMAND_H
