
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
	SearchData data;
	data.searchId = searchId;
	data.searchType = searchType;
	data.keyword = keyword;
	data.state = STATE_SEARCHING;
	data.retryCount = 0;
	data.shownCount = 0;
	data.hiddenCount = 0;

	m_searches[searchId] = data;

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

	// Update state based on result count
	if (data.shownCount == 0 && data.hiddenCount == 0) {
		// No results - check if we should retry
		if (data.searchType == wxT("Kad") && data.retryCount < MAX_RETRIES) {
			// Automatically retry Kad searches
			// Notify observers to initiate the retry
			for (ObserverSet::iterator it = m_observers.begin(); it != m_observers.end(); ++it) {
				if ((*it)->OnRetryRequested(searchId)) {
					// Retry initiated - update state
					StartRetry(searchId);
					return;
				}
			}
		}
		// No retry or retry failed
		UpdateState(searchId, STATE_NO_RESULTS);
	} else {
		// Results found
		UpdateState(searchId, STATE_HAS_RESULTS);
	}
}

bool SearchStateManager::StartRetry(uint32_t searchId)
{
	SearchMap::iterator it = m_searches.find(searchId);
	if (it == m_searches.end()) {
		return false;
	}

	SearchData& data = it->second;

	// Check retry limit
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
