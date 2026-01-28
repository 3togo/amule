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

#include "SearchModel.h"
#include "../SearchFile.h" // For CSearchFile

namespace search {

SearchModel::SearchModel()
    : m_state(SearchState::Idle)
    , m_searchId(-1)
{
}

SearchModel::~SearchModel()
{
    clearResults();
}

void SearchModel::setSearchParams(const SearchParams& params)
{
    wxMutexLocker lock(m_mutex);
    m_params = params;
}

SearchParams SearchModel::getSearchParams() const
{
    wxMutexLocker lock(m_mutex);
    return m_params;
}

SearchParams SearchModel::getSearchParamsThreadSafe() const
{
    wxMutexLocker lock(m_mutex);
    return m_params;
}

void SearchModel::addResult(CSearchFile* result)
{
    wxMutexLocker lock(m_mutex);
    m_results.push_back(result);
}

void SearchModel::clearResults()
{
    wxMutexLocker lock(m_mutex);
    for (auto result : m_results) {
        delete result;
    }
    m_results.clear();
}

size_t SearchModel::getResultCount() const
{
    wxMutexLocker lock(m_mutex);
    // Return cached results count if available, otherwise return internal count
    if (m_hasCachedResults) {
        return m_cachedResults.size();
    }
    return m_results.size();
}

std::vector<CSearchFile*> SearchModel::getResults() const
{
    wxMutexLocker lock(m_mutex);
    // Return cached results if available, otherwise return internal results
    if (m_hasCachedResults) {
        return m_cachedResults;
    }
    return m_results;
}

bool SearchModel::hasResults() const
{
    wxMutexLocker lock(m_mutex);
    if (m_hasCachedResults) {
        return !m_cachedResults.empty();
    }
    return !m_results.empty();
}

void SearchModel::cacheResults(const std::vector<CSearchFile*>& results)
{
    wxMutexLocker lock(m_mutex);
    m_cachedResults = results;
    m_hasCachedResults = true;
}

void SearchModel::clearCachedResults()
{
    wxMutexLocker lock(m_mutex);
    m_cachedResults.clear();
    m_hasCachedResults = false;
}

void SearchModel::setSearchState(SearchState state)
{
    wxMutexLocker lock(m_mutex);
    m_state = state;
}

SearchState SearchModel::getSearchState() const
{
    wxMutexLocker lock(m_mutex);
    return m_state;
}

SearchState SearchModel::getSearchStateThreadSafe() const
{
    wxMutexLocker lock(m_mutex);
    return m_state;
}

void SearchModel::setSearchId(long searchId)
{
    wxMutexLocker lock(m_mutex);
    m_searchId = searchId;
}

long SearchModel::getSearchId() const
{
    wxMutexLocker lock(m_mutex);
    return m_searchId;
}

long SearchModel::getSearchIdThreadSafe() const
{
    wxMutexLocker lock(m_mutex);
    return m_searchId;
}

} // namespace search