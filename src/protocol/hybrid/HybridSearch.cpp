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
// Hybrid search implementation combining eD2k and BitTorrent protocols
//

#include "protocol/hybrid/HybridSearch.h"
#include "SearchFile.h"
#include "amule.h"
#include <algorithm>
#include <thread>
#include <future>

HybridSearchManager& HybridSearchManager::instance() {
    static HybridSearchManager instance;
    return instance;
}

HybridSearchManager::HybridSearchManager() 
    : m_priority(SearchPriority::COMBINED_RANKED), m_next_search_id(1) {
}

HybridSearchManager::~HybridSearchManager() {
}

std::vector<CSearchFile*> HybridSearchManager::hybrid_search(const CSearchList::CSearchParams& params) {
    std::vector<CSearchFile*> all_results;
    
    // Run eD2k and BitTorrent searches in parallel
    auto ed2k_future = std::async(std::launch::async, [&]() {
        return theApp->searchlist->GetSearchResults(m_next_search_id);
    });
    
    auto bt_future = std::async(std::launch::async, [&]() {
        auto bt_results = BitTorrent::BitTorrentSession::instance().dht_search(params.searchString.ToUTF8().data());
        std::vector<CSearchFile*> converted_results;
        
        for (const auto& bt_result : bt_results) {
            if (auto search_file = convert_to_ed2k_searchfile(bt_result, m_next_search_id)) {
                converted_results.push_back(search_file);
            }
        }
        return converted_results;
    });
    
    // Wait for both searches to complete
    auto ed2k_results = ed2k_future.get();
    auto bt_results = bt_future.get();
    
    // Combine results based on priority
    switch (m_priority) {
        case SearchPriority::ED2K_FIRST:
            all_results = ed2k_results;
            all_results.insert(all_results.end(), bt_results.begin(), bt_results.end());
            break;
            
        case SearchPriority::BT_FIRST:
            all_results = bt_results;
            all_results.insert(all_results.end(), ed2k_results.begin(), ed2k_results.end());
            break;
            
        case SearchPriority::COMBINED_RANKED:
        case SearchPriority::BANDWIDTH_OPTIMIZED:
            all_results = ed2k_results;
            all_results.insert(all_results.end(), bt_results.begin(), bt_results.end());
            
            // Sort by relevance (size matching, source count, etc.)
            std::sort(all_results.begin(), all_results.end(), 
                [](CSearchFile* a, CSearchFile* b) {
                    // Simple ranking: more sources = better
                    return a->GetSourceCount() > b->GetSourceCount();
                });
            break;
    }
    
    // Merge duplicates across protocols
    merge_duplicate_results(all_results);
    
    m_next_search_id++;
    return all_results;
}

CSearchFile* HybridSearchManager::convert_to_ed2k_searchfile(const BitTorrent::SearchResult& bt_result, wxUIntPtr search_id) {
    try {
        // Create a minimal CMemFile with the required data
        CMemFile data;
        
        // Write basic file info
        data.WriteString(wxString(bt_result.name.c_str(), wxConvUTF8));
        data.WriteUInt64(bt_result.size);
        
        // Write source info (approximate BitTorrent peers as sources)
        data.WriteUInt32(bt_result.seeders + bt_result.leechers); // Total sources
        data.WriteUInt32(bt_result.seeders); // Complete sources
        
        // Create search file
        auto search_file = new CSearchFile(
            data,
            true, // UTF8
            search_id,
            0, // server IP
            0, // server port
            wxEmptyString, // directory
            false // not from Kad
        );
        
        // Set additional BitTorrent-specific info
        search_file->SetExtraInfo(wxString::Format(
            "BT:%s|Seeders:%d|Leechers:%d|Speed:%.1f/%.1f",
            bt_result.source.c_str(),
            bt_result.seeders,
            bt_result.leechers,
            bt_result.download_speed,
            bt_result.upload_speed
        ));
        
        return search_file;
        
    } catch (...) {
        return nullptr;
    }
}

void HybridSearchManager::merge_duplicate_results(std::vector<CSearchFile*>& results) {
    // Simple duplicate detection by name and size
    std::vector<CSearchFile*> unique_results;
    std::map<std::pair<wxString, uint64_t>, CSearchFile*> seen;
    
    for (auto* result : results) {
        auto key = std::make_pair(result->GetFileName().GetPrintable(), result->GetFileSize());
        
        if (auto it = seen.find(key); it != seen.end()) {
            // Merge sources from duplicate
            it->second->MergeSources(*result);
            delete result;
        } else {
            seen[key] = result;
            unique_results.push_back(result);
        }
    }
    
    results = std::move(unique_results);
}

void HybridSearchManager::set_search_priority(SearchPriority priority) {
    m_priority = priority;
}

HybridSearchManager::SearchPriority HybridSearchManager::get_search_priority() const {
    return m_priority;
}