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

#ifndef SEARCHCONTROLLERBASE_H
#define SEARCHCONTROLLERBASE_H

#include "SearchController.h"
#include "SearchModel.h"
#include "../SearchList.h"
#include <memory>
#include <cstdint>
#include <utility>
#include <wx/string.h>

namespace search {

/**
 * SearchControllerBase - Base implementation class for search controllers
 *
 * This class provides common functionality shared between different
 * search controller implementations to eliminate code duplication.
 */
class SearchControllerBase : public SearchController {
public:
    explicit SearchControllerBase(CSearchList* searchList = nullptr);
    virtual ~SearchControllerBase();

    // Delete copy constructor and copy assignment operator
    SearchControllerBase(const SearchControllerBase&) = delete;
    SearchControllerBase& operator=(const SearchControllerBase&) = delete;

    // Common state accessors
    SearchState getState() const override;
    SearchParams getSearchParams() const override;
    long getSearchId() const override;

    // Result access
    const std::vector<CSearchFile*>& getResults() const override;
    size_t getResultCount() const override;

    // Configuration validation
    bool validateConfiguration() const;

protected:
    // Common data members
    std::unique_ptr<SearchModel> m_model;
    CSearchList* m_searchList;

    // Retry settings
    int m_retryCount;
    int m_currentRetry;
    static constexpr int DEFAULT_RETRY_COUNT = 3;

    // Helper methods
    void connectToSearchSystem();
    void disconnectFromSearchSystem();
    void handleSearchError(const wxString& error);
    virtual void resetSearchState();
    void stopSearchBase();

    // Validation methods
    bool validatePrerequisites();
    bool validateSearchParams(const SearchParams& params);
    bool validateRetryLimit(wxString& error) const;

    // State update methods
    void updateSearchState(const SearchParams& params, uint32_t searchId, SearchState state);

    // Helper methods
    const std::vector<CSearchFile*>* getSearchResults() const;
    CSearchList::CSearchParams convertParams(const SearchParams& params) const;
};

} // namespace search

#endif // SEARCHCONTROLLERBASE_H
