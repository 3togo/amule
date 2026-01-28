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

#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include <vector>
#include <memory>
#include <wx/string.h>
#include <wx/thread.h>

// Forward declarations
class CSearchFile;

namespace search {

enum class ModernSearchType {
    LocalSearch = 0,
    GlobalSearch,
    KadSearch
};

enum class SearchState {
    Idle,
    Searching,
    Completed,
    Error,
    Retrying
};

struct SearchParams {
    wxString searchString;
    wxString strKeyword;
    wxString typeText;
    wxString extension;
    uint64_t minSize = 0;
    uint64_t maxSize = 0;
    uint32_t availability = 0;
    ModernSearchType searchType = ModernSearchType::GlobalSearch;
    
    SearchParams() = default;
    
    bool isValid() const {
        return !searchString.IsEmpty();
    }
    
    bool operator==(const SearchParams& other) const {
        return searchString == other.searchString &&
               strKeyword == other.strKeyword &&
               typeText == other.typeText &&
               extension == other.extension &&
               minSize == other.minSize &&
               maxSize == other.maxSize &&
               availability == other.availability &&
               searchType == other.searchType;
    }
    
    bool operator!=(const SearchParams& other) const {
        return !(*this == other);
    }
};

class SearchModel {
public:
    SearchModel();
    ~SearchModel();
    
    // Parameter management
    void setSearchParams(const SearchParams& params);
    SearchParams getSearchParams() const;
    
    // Thread-safe parameter access
    SearchParams getSearchParamsThreadSafe() const;
    
    // Result management
    void addResult(CSearchFile* result);
    void clearResults();
    size_t getResultCount() const;
    
    // State management
    void setSearchState(SearchState state);
    SearchState getSearchState() const;
    
    // Search ID management
    void setSearchId(long searchId);
    long getSearchId() const;
    
    // Thread-safe methods
    SearchState getSearchStateThreadSafe() const;
    long getSearchIdThreadSafe() const;
    
private:
    mutable wxMutex m_mutex;
    
    SearchParams m_params;
    SearchState m_state = SearchState::Idle;
    long m_searchId = -1;
    
    // Results storage - TODO: Replace with more efficient structure
    std::vector<CSearchFile*> m_results;
};

} // namespace search

#endif // SEARCHMODEL_H