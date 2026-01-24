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
// Hybrid search manager for combining eD2k and BitTorrent search results
//

#pragma once

#include "../../SearchList.h"
#include "protocol/bt/BitTorrentSession.h"
#include <vector>
#include <memory>

class HybridSearchManager {
public:
    static HybridSearchManager& instance();
    
    // Hybrid search that combines eD2k and BitTorrent results
    std::vector<CSearchFile*> hybrid_search(const CSearchList::CSearchParams& params);
    
    // Convert BitTorrent search results to CSearchFile format
    CSearchFile* convert_to_ed2k_searchfile(const BitTorrent::SearchResult& bt_result, wxUIntPtr search_id);
    
    // Merge duplicate results from different protocols
    void merge_duplicate_results(std::vector<CSearchFile*>& results);
    
    // Protocol preference settings
    enum class SearchPriority {
        ED2K_FIRST,
        BT_FIRST,
        COMBINED_RANKED,
        BANDWIDTH_OPTIMIZED
    };
    
    void set_search_priority(SearchPriority priority);
    SearchPriority get_search_priority() const;
    
private:
    HybridSearchManager();
    ~HybridSearchManager();
    
    SearchPriority m_priority;
    wxUIntPtr m_next_search_id;
    
    // Disable copying
    HybridSearchManager(const HybridSearchManager&) = delete;
    HybridSearchManager& operator=(const HybridSearchManager&) = delete;
};