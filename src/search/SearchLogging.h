//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SEARCH_LOGGING_H
#define SEARCH_LOGGING_H

#include "../Logger.h"

// Central switch for search window debug logging
// Controlled by CMake option ENABLE_SEARCH_WINDOW_DEBUG
// Default to enabled if not defined by CMake
#ifndef ENABLE_SEARCH_WINDOW_DEBUG
#define ENABLE_SEARCH_WINDOW_DEBUG 0
#endif

namespace search {
    // Convenience macro for search window logging
    #define SEARCH_DEBUG(msg) \
        do { \
            if (ENABLE_SEARCH_WINDOW_DEBUG && theLogger.IsEnabled(logSearch)) { \
                theLogger.AddLogLine(wxT(__FILE__), __LINE__, false, logSearch, msg); \
            } \
        } while(0)

    // Convenience macro for critical search window messages
    #define SEARCH_CRITICAL(msg) \
        do { \
            theLogger.AddLogLine(wxT(__FILE__), __LINE__, true, logSearch, msg); \
        } while(0)
}

#endif // SEARCH_LOGGING_H
