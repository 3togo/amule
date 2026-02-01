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

#ifndef SEARCHTYPECONVERTER_H
#define SEARCHTYPECONVERTER_H

#include "SearchModel.h"
#include "../SearchList.h"

namespace search {

/**
 * Utility class for converting between legacy and modern search types
 * 
 * This class provides static methods to convert between the old ::SearchType
 * enum and the new ModernSearchType enum class. This helps during the migration
 * phase by providing a clean interface for type conversions.
 * 
 * TODO: Once all code has been migrated to use ModernSearchType, this class
 * can be removed and the legacy ::SearchType enum can be deprecated.
 */
class SearchTypeConverter {
public:
    /**
     * Convert from legacy SearchType to ModernSearchType
     * 
     * @param legacyType The legacy SearchType value
     * @return The corresponding ModernSearchType value
     */
    static ModernSearchType fromLegacy(::SearchType legacyType) {
        switch (legacyType) {
            case ::LocalSearch:
                return ModernSearchType::LocalSearch;
            case ::GlobalSearch:
                return ModernSearchType::GlobalSearch;
            case ::KadSearch:
                return ModernSearchType::KadSearch;
            default:
                // Default to GlobalSearch for unknown types
                return ModernSearchType::GlobalSearch;
        }
    }

    /**
     * Convert from ModernSearchType to legacy SearchType
     * 
     * @param modernType The ModernSearchType value
     * @return The corresponding legacy SearchType value
     */
    static ::SearchType toLegacy(ModernSearchType modernType) {
        switch (modernType) {
            case ModernSearchType::LocalSearch:
                return ::LocalSearch;
            case ModernSearchType::GlobalSearch:
                return ::GlobalSearch;
            case ModernSearchType::KadSearch:
                return ::KadSearch;
            default:
                // Default to GlobalSearch for unknown types
                return ::GlobalSearch;
        }
    }

    /**
     * Check if a search type is local search
     * 
     * @param type The search type to check
     * @return true if it's a local search, false otherwise
     */
    static bool isLocalSearch(ModernSearchType type) {
        return type == ModernSearchType::LocalSearch;
    }

    /**
     * Check if a search type is global search
     * 
     * @param type The search type to check
     * @return true if it's a global search, false otherwise
     */
    static bool isGlobalSearch(ModernSearchType type) {
        return type == ModernSearchType::GlobalSearch;
    }

    /**
     * Check if a search type is Kad search
     * 
     * @param type The search type to check
     * @return true if it's a Kad search, false otherwise
     */
    static bool isKadSearch(ModernSearchType type) {
        return type == ModernSearchType::KadSearch;
    }
};

} // namespace search

#endif // SEARCHTYPECONVERTER_H
