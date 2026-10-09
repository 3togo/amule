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

#ifdef __WXOSX_COCOA__
#import <AppKit/AppKit.h>

namespace
{
NSImage *LastMenuImage(wxMenu &menu)
{
	NSMenu *native = menu.GetHMenu();
	const NSInteger count = [native numberOfItems];
	return count ? [[native itemAtIndex:count - 1] image] : nil;
}
}

void MarkMenuIconAsTemplate(wxMenu &menu)
{
	// Called immediately after Append(), while this image is the last item.
	// Cocoa supplies appearance, highlighted-row and disabled-state colours
	// from the image's alpha mask instead of retaining our RGB pixels.
	[LastMenuImage(menu) setTemplate:YES];
}

bool LastMenuIconIsTemplate(wxMenu &menu)
{
	return [LastMenuImage(menu) isTemplate];
}
#endif
