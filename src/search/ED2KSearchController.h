
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

#ifndef ED2KSEARCHCONTROLLER_H
#define ED2KSEARCHCONTROLLER_H

#include "SearchController.h"
#include "SearchModel.h"
#include <memory>

// Forward declarations
class CSearchList;
class CServer;

namespace search {

/**
 * ED2KSearchController - Specialized controller for eD2k network searches
 * 
 * This controller handles both local and global eD2k searches with:
 * - Optimized server communication
 * - Efficient result aggregation
 * - Detailed progress reporting
 * - Automatic retry logic
 */
class ED2KSearchController : public SearchController {
public:
    ED2KSearchController();
    virtual ~ED2KSearchController();

    // SearchController implementation
    void startSearch(const SearchParams& params) override;
    void stopSearch() override;
    void requestMoreResults() override;

    SearchState getState() const override;
    SearchParams getSearchParams() const override;
    long getSearchId() const override;

    const std::vector<CSearchFile*>& getResults() const override;
    size_t getResultCount() const override;

    // ED2K-specific methods
    void setMaxServersToQuery(int maxServers);
    int getMaxServersToQuery() const;

    void setRetryCount(int retryCount);
    int getRetryCount() const;

private:
    std::unique_ptr<SearchModel> m_model;
    CSearchList* m_searchList;

    // ED2K-specific settings
    int m_maxServersToQuery;
    int m_retryCount;
    int m_currentRetry;

    // Progress tracking
    int m_serversContacted;
    int m_resultsSinceLastUpdate;

    // Helper methods
    void connectToSearchSystem();
    void disconnectFromSearchSystem();
    void updateProgress();
    void handleSearchError(const wxString& error);
    void initializeProgress();
};

} // namespace search

#endif // ED2KSEARCHCONTROLLER_H
