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

#ifndef AMULE_MENUICONS_H
#define AMULE_MENUICONS_H

#include <wx/menu.h>

// Semantic names keep a command's artwork consistent across context menus.
enum class MenuIcon
{
	Pause,
	Resume,
	Stop,
	Cancel,
	Folder,
	Info,
	Link,
	Preview,
	Comments,
	ClearCompleted,
	Download,
	Open
};

wxMenuItem *AppendMenuIcon(wxMenu *menu, int id, const wxString &label, MenuIcon icon);

#ifdef __WXOSX_COCOA__
void MarkMenuIconAsTemplate(wxMenu &menu);
bool LastMenuIconIsTemplate(wxMenu &menu);
#endif

#endif
