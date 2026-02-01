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

#include "SearchStateManager.h"
#include <common/Format.h>
#include <cstdint>

// Maximum number of retries
const int MAX_RETRIES = 3;

SearchStateManager::SearchStateManager()
{
}

SearchStateManager::~SearchStateManager()
{
	m_observers.clear();
	m_searches.clear();
}

void SearchStateManager::RegisterObserver(ISearchStateObserver* observer)
{
	if (observer) {
		m_observers.insert(observer);
	}
}

void SearchStateManager::UnregisterObserver(ISearchStateObserver* observer)
{
	if (observer) {
		m_observers.erase(observer);
	}
}

void SearchStateManager::InitializeSearch(uint32_t searchId, const wxString& searchType, const wxString& keyword)
{
	InitializeSearch(searchId, searchType, keyword, CSearchList::CSearchParams());
}

void SearchStateManager::InitializeSearch(uint32_t searchId, const wxString& searchType, const wxString& keyword, const CSearchList::CSearchParams& params)
{
	SearchData data;
	data.searchId = searchId;
	data.searchType = searchType;
	data.keyword = keyword;
	data.state = STATE_SEARCHING;
	data.retryCount = 0;
	data.shownCount = 0;
	data.hiddenCount = 0;
	
	// Store search parameters for retry
	data.searchString = params.searchString;
	data.strKeyword = params.strKeyword;
	data.typeText = params.typeText;
	data.extension = params.extension;
	data.minSize = params.minSize;
	data.maxSize = params.maxSize;
	data.availability = params.availability;
	
	printf("InitializeSearch: storing params for searchId=%u, searchString='%s'\n", searchId, (const char*)data.searchString.utf8_str());
	m_searches[searchId] = data;
	printf("InitializeSearch: m_searches size=%zu\n", m_searches.size());

	// Notify observers of the new search
	NotifyObservers(searchId, STATE_SEARCHING, 0);
}

void SearchStateManager::UpdateResultCount(uint32_t searchId, size_t shown, size_t hidden)
{
	SearchMap::iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return;
	}

	SearchData& data = it->second;
	data.shownCount = shown;
	data.hiddenCount = hidden;

	// Update state based on result count
	if (shown > 0 || hidden > 0) {
		// Results found - reset retry count and update state
		if (data.retryCount > 0) {
			data.retryCount = 0;
		}
		// Only update state if we were in a non-result state
		if (data.state == STATE_SEARCHING ||
			data.state == STATE_RETRYING ||
			data.state == STATE_NO_RESULTS) {
			UpdateState(searchId, STATE_HAS_RESULTS);
		}
	}
}

void SearchStateManager::EndSearch(uint32_t searchId)
{
	SearchMap::iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return;
	}

	SearchData& data = it->second;

	// Determine final state based on results
	if (data.shownCount > 0 || data.hiddenCount > 0) {
		UpdateState(searchId, STATE_HAS_RESULTS);
	} else {
		// No results - check if we should retry
		if (data.retryCount < MAX_RETRIES) {
			// Don't set to NO_RESULTS yet, let retry mechanism handle it
			// The retry will be initiated by the caller
			return;
		} else {
			// Max retries reached, set to NO_RESULTS
			UpdateState(searchId, STATE_NO_RESULTS);
		}
	}
}

bool SearchStateManager::RequestRetry(uint32_t searchId)
{
	SearchMap::iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return false;
	}

	SearchData& data = it->second;

	// Check if we've reached the retry limit
	if (data.retryCount >= MAX_RETRIES) {
		return false;
	}

	// Increment retry count
	data.retryCount++;

	// Update state to retrying
	UpdateState(searchId, STATE_RETRYING);

	// Notify observers
	NotifyObservers(searchId, STATE_RETRYING, data.retryCount);

	return true;
}

SearchState SearchStateManager::GetSearchState(uint32_t searchId) const
{
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return STATE_IDLE;
	}
	return it->second.state;
}

void SearchStateManager::StoreSearchParams(uint32_t searchId, const CSearchList::CSearchParams& params)
{
	SearchMap::iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return;
	}

	SearchData& data = it->second;
	// Store all search parameters for retry
	data.searchString = params.searchString;
	data.strKeyword = params.strKeyword;
	data.typeText = params.typeText;
	data.extension = params.extension;
	data.minSize = params.minSize;
	data.maxSize = params.maxSize;
	data.availability = params.availability;
}

bool SearchStateManager::GetSearchParams(uint32_t searchId, CSearchList::CSearchParams& params) const
{
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		printf("GetSearchParams: searchId=%u not found in m_searches (size=%zu)\n", searchId, m_searches.size());
		return false;
	}
	printf("GetSearchParams: searchId=%u found, searchString='%s'\n", searchId, (const char*)it->second.searchString.utf8_str());

	const SearchData& data = it->second;
	// Retrieve all stored search parameters
	params.searchString = data.searchString;
	params.strKeyword = data.strKeyword;
	params.typeText = data.typeText;
	params.extension = data.extension;
	params.minSize = data.minSize;
	params.maxSize = data.maxSize;
	params.availability = data.availability;
	return true;
}

int SearchStateManager::GetRetryCount(uint32_t searchId) const
{
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return 0;
	}
	return it->second.retryCount;
}

wxString SearchStateManager::GetSearchType(uint32_t searchId) const
{
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return wxEmptyString;
	}
	return it->second.searchType;
}

wxString SearchStateManager::GetKeyword(uint32_t searchId) const
{
	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return wxEmptyString;
	}
	return it->second.keyword;
}

void SearchStateManager::GetResultCount(uint32_t searchId, size_t& shown, size_t& hidden) const
{
	shown = 0;
	hidden = 0;

	SearchMap::const_iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return;
	}

	shown = it->second.shownCount;
	hidden = it->second.hiddenCount;
}

bool SearchStateManager::HasSearch(uint32_t searchId) const
{
	return m_searches.find(searchId) != m_searches.end();
}

void SearchStateManager::RemoveSearch(uint32_t searchId)
{
	m_searches.erase(searchId);
}

void SearchStateManager::UpdateState(uint32_t searchId, SearchState newState)
{
	SearchMap::iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return;
	}

	SearchData& data = it->second;

	// Only update if state is changing
	if (data.state != newState) {
		data.state = newState;
		NotifyObservers(searchId, newState, data.retryCount);
	}
}

void SearchStateManager::NotifyObservers(uint32_t searchId, SearchState state, int retryCount)
{
	for (ObserverSet::iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
		(*it)->OnSearchStateChanged(searchId, state, retryCount);
	}
}
