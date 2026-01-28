
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

#include "SearchControllerBase.h"
#include "../SearchList.h"
#include <memory>
#include <cstdint>
#include <utility>
#include <wx/string.h>

// Forward declarations
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
class ED2KSearchController : public SearchControllerBase {
public:
    explicit ED2KSearchController(CSearchList* searchList = nullptr);
    virtual ~ED2KSearchController();

    // Delete copy constructor and copy assignment operator
    ED2KSearchController(const ED2KSearchController&) = delete;
    ED2KSearchController& operator=(const ED2KSearchController&) = delete;

    // SearchController implementation
    void startSearch(const SearchParams& params) override;
    void stopSearch() override;
    void requestMoreResults() override;

    // ED2K-specific methods
    void setMaxServersToQuery(int maxServers);
    int getMaxServersToQuery() const;

    void setRetryCount(int retryCount);
    int getRetryCount() const;

    // Configuration validation
    bool validateConfiguration() const;

private:
    // ED2K-specific settings
    int m_maxServersToQuery;

    // Progress tracking
    int m_serversContacted;
    int m_resultsSinceLastUpdate;
    static constexpr int DEFAULT_MAX_SERVERS = 100;
    static constexpr int PROGRESS_UPDATE_INTERVAL = 5;

    // Helper methods
    void updateProgress();
    void initializeProgress();
    bool isValidServerList() const;

    // Validation methods
    bool validatePrerequisites();
    bool validateSearchStateForMoreResults(wxString& error) const;

    // Execution methods
    std::pair<uint32_t, wxString> executeSearch(const SearchParams& params);
    CSearchList::CSearchParams convertParams(const SearchParams& params) const;
};

} // namespace search

#endif // ED2KSEARCHCONTROLLER_H
