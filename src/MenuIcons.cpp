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

#include "MenuIcons.h"
#include "CamuleArtProvider.h"

#include <wx/artprov.h>
#include <wx/bmpbndl.h>
#include <wx/settings.h>

wxMenuItem *AppendMenuIcon(wxMenu *menu, int id, const wxString &label, MenuIcon icon)
{
	const char *name = nullptr;
	switch (icon) {
	case MenuIcon::Pause:
		name = "pause_fill";
		break;
	case MenuIcon::Resume:
		name = "play_fill";
		break;
	case MenuIcon::Stop:
		name = "stop_fill";
		break;
	case MenuIcon::Cancel:
		name = "x_lg";
		break;
	case MenuIcon::Folder:
		name = "folder2_open";
		break;
	case MenuIcon::Info:
		name = "info_circle";
		break;
	case MenuIcon::Link:
		name = "link_45deg";
		break;
	case MenuIcon::Preview:
		name = "eye";
		break;
	case MenuIcon::Comments:
		name = "chat_left_text";
		break;
	case MenuIcon::ClearCompleted:
		name = "check2_all";
		break;
	case MenuIcon::Download:
		name = "download";
		break;
	case MenuIcon::Open:
		name = "file_earmark";
		break;
	}
	auto *item = new wxMenuItem(menu, id, label);
#ifdef __WXOSX_COCOA__
	// AppKit template images use black RGB with the artwork's alpha mask.
	const wxColour colour = *wxBLACK;
#else
	const wxColour colour = wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT);
#endif
	const wxBitmapBundle bitmap =
		CamuleArtProvider::GetMenuBitmapBundle(wxString("menu_") + name, wxSize(16, 16), colour);
	if (bitmap.IsOk()) {
		// Set the image before insertion; native ports own layout and disabled states.
		// Do not override GTK's menu-image preference or replace native checkmarks.
		item->SetBitmap(bitmap);
	}
	menu->Append(item);
#ifdef __WXOSX_COCOA__
	if (bitmap.IsOk()) {
		MarkMenuIconAsTemplate(*menu);
	}
#endif
	return item;
}
