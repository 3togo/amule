
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

#ifndef KADSEARCHCONTROLLER_H
#define KADSEARCHCONTROLLER_H

#include "SearchController.h"
#include "SearchModel.h"
#include <memory>

// Forward declarations
class CSearchList;

namespace search {

/**
 * KadSearchController - Specialized controller for Kademlia network searches
 * 
 * This controller handles Kad searches with:
 * - Optimized keyword-based searches
 * - Efficient result aggregation from Kad nodes
 * - Detailed progress reporting
 * - Automatic retry logic for failed searches
 */
class KadSearchController : public SearchController {
public:
    KadSearchController();
    virtual ~KadSearchController();

    // SearchController implementation
    void startSearch(const SearchParams& params) override;
    void stopSearch() override;
    void requestMoreResults() override;

    SearchState getState() const override;
    SearchParams getSearchParams() const override;
    long getSearchId() const override;

    const std::vector<CSearchFile*>& getResults() const override;
    size_t getResultCount() const override;

    // Kad-specific methods
    void setMaxNodesToQuery(int maxNodes);
    int getMaxNodesToQuery() const;

    void setRetryCount(int retryCount);
    int getRetryCount() const;

private:
    std::unique_ptr<SearchModel> m_model;
    CSearchList* m_searchList;

    // Kad-specific settings
    int m_maxNodesToQuery;
    int m_retryCount;
    int m_currentRetry;

    // Progress tracking
    int m_nodesContacted;

    // Helper methods
    void connectToSearchSystem();
    void disconnectFromSearchSystem();
    void updateProgress();
    void handleSearchError(const wxString& error);
    void initializeProgress();
};

} // namespace search

#endif // KADSEARCHCONTROLLER_H
