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
    // Reset retry counter - derived classes can override to reset additional state
    m_currentRetry = 0;
}

void SearchControllerBase::stopSearchBase()
{
    // Validate prerequisites
    if (!m_searchList) {
	return;
    }

    // Update state
    m_model->setSearchState(SearchState::Idle);
    resetSearchState();

    // Notify completion
    notifySearchCompleted();
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
    // Combine validation checks for efficiency
    if (!params.isValid() || params.searchString.IsEmpty()) {
	handleSearchError(params.searchString.IsEmpty() 
	    ? _("Search string cannot be empty")
	    : _("Invalid search parameters"));
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

const std::vector<CSearchFile*>* SearchControllerBase::getSearchResults() const
{
    if (!m_searchList) {
	return nullptr;
    }

    long searchId = m_model->getSearchId();
    if (searchId == -1) {
	return nullptr;
    }

    return &m_searchList->GetSearchResults(searchId);
}

CSearchList::CSearchParams SearchControllerBase::convertParams(const SearchParams& params) const
{
    CSearchList::CSearchParams oldParams;
    oldParams.searchString = params.searchString;
    oldParams.strKeyword = params.strKeyword;
    oldParams.typeText = params.typeText;
    oldParams.extension = params.extension;
    oldParams.minSize = params.minSize;
    oldParams.maxSize = params.maxSize;
    oldParams.availability = params.availability;
    return oldParams;
}

const std::vector<CSearchFile*>& SearchControllerBase::getResults() const
{
    // Use thread_local instead of static to avoid thread safety issues
    static thread_local std::vector<CSearchFile*> results;
    results.clear();

    const CSearchResultList* searchResults = getSearchResults();
    if (searchResults) {
	results = *searchResults;
	m_model->cacheResults(results);
    }

    return results;
}

size_t SearchControllerBase::getResultCount() const
{
    const CSearchResultList* searchResults = getSearchResults();
    return searchResults ? searchResults->size() : 0;
}

bool SearchControllerBase::validateConfiguration() const
{
    if (m_retryCount < 0) {
	return false;
    }
    return true;
}

} // namespace search
