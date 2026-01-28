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

#ifndef LEGACYSEARCHCONTROLLER_H
#define LEGACYSEARCHCONTROLLER_H

#include "SearchController.h"
#include <memory>

// Forward declarations
class CSearchList;
class SearchModel;

class LegacySearchController : public SearchController {
public:
    LegacySearchController();
    virtual ~LegacySearchController();
    
    // SearchController implementation
    void startSearch(const SearchParams& params) override;
    void stopSearch() override;
    void requestMoreResults() override;
    
    SearchState getState() const override;
    SearchParams getSearchParams() const override;
    long getSearchId() const override;
    
    const std::vector<CSearchFile*>& getResults() const override;
    size_t getResultCount() const override;
    
private:
    std::unique_ptr<SearchModel> m_model;
    CSearchList* m_searchList; // Reference to existing search list
    
    // Convert between old and new parameter types
    SearchParams convertToSearchParams(const wxString& searchString, ModernSearchType type) const;
    
    // Helper methods
    void connectToExistingSystem();
    void disconnectFromExistingSystem();
};

#endif // LEGACYSEARCHCONTROLLER_H