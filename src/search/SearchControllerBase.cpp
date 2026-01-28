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

#include "SearchControllerBase.h"
#include "../SearchList.h"
#include "../amule.h"

namespace search {

SearchControllerBase::SearchControllerBase(CSearchList* searchList)
    : m_model(std::make_unique<SearchModel>())
    , m_searchList(searchList)
    , m_retryCount(DEFAULT_RETRY_COUNT)
    , m_currentRetry(0)
{
    connectToSearchSystem();
}

SearchControllerBase::~SearchControllerBase()
{
    disconnectFromSearchSystem();
}

void SearchControllerBase::connectToSearchSystem()
{
    if (theApp) {
	m_searchList = theApp->searchlist;
    }
}

void SearchControllerBase::disconnectFromSearchSystem()
{
    m_searchList = nullptr;
}

void SearchControllerBase::handleSearchError(const wxString& error)
{
    m_model->setSearchState(SearchState::Error);
    notifyError(error);
}

void SearchControllerBase::resetSearchState()
{
    m_currentRetry = 0;
}

bool SearchControllerBase::validatePrerequisites()
{
    if (!m_searchList) {
	handleSearchError(_("Search system not available"));
	return false;
    }
    return true;
}

bool SearchControllerBase::validateSearchParams(const SearchParams& params)
{
    if (!params.isValid()) {
	handleSearchError(_("Invalid search parameters"));
	return false;
    }

    if (params.searchString.IsEmpty()) {
	handleSearchError(_("Search string cannot be empty"));
	return false;
    }

    return true;
}

bool SearchControllerBase::validateRetryLimit(wxString& error) const
{
    if (m_currentRetry >= m_retryCount) {
	error = _("Maximum retry limit reached");
	return false;
    }
    return true;
}

void SearchControllerBase::updateSearchState(const SearchParams& params, uint32_t searchId, SearchState state)
{
    m_model->setSearchParams(params);
    m_model->setSearchId(searchId);
    m_model->setSearchState(state);
}

SearchState SearchControllerBase::getState() const
{
    return m_model->getSearchState();
}

SearchParams SearchControllerBase::getSearchParams() const
{
    return m_model->getSearchParams();
}

long SearchControllerBase::getSearchId() const
{
    return m_model->getSearchId();
}

const std::vector<CSearchFile*>& SearchControllerBase::getResults() const
{
    static std::vector<CSearchFile*> results;
    results.clear();

    if (m_searchList) {
	long searchId = m_model->getSearchId();
	if (searchId != -1) {
	    const CSearchResultList& searchResults = m_searchList->GetSearchResults(searchId);
	    results = searchResults;
	    m_model->cacheResults(results);
	}
    }

    return results;
}

size_t SearchControllerBase::getResultCount() const
{
    if (m_searchList) {
	long searchId = m_model->getSearchId();
	if (searchId != -1) {
	    const CSearchResultList& searchResults = m_searchList->GetSearchResults(searchId);
	    return searchResults.size();
	}
    }

    return 0;
}

bool SearchControllerBase::validateConfiguration() const
{
    if (m_retryCount < 0) {
	return false;
    }
    return true;
}

} // namespace search
