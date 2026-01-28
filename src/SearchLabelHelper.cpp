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

#include "SearchLabelHelper.h"
#include "SearchListCtrl.h"
#include "SearchDlg.h"
#include "MuleNotebook.h"
#include <common/Format.h>
#include "amule.h"
#include "SearchList.h"

void UpdateSearchState(CSearchListCtrl* list, CSearchDlg* parentDlg, const wxString& state)
{
	if (!list || !parentDlg) {
		return;
	}

	// Get the notebook from parent dialog
	CMuleNotebook* notebook = parentDlg->GetNotebook();
	if (!notebook) {
		return;
	}

	for (uint32 i = 0; i < (uint32)notebook->GetPageCount(); ++i) {
		if (notebook->GetPage(i) == list) {
			// Get the current tab text
			wxString tabText = notebook->GetPageText(i);

			// Preserve the search type prefix [Local], [ED2K], or [Kad]
			wxString typePrefix;
			if (tabText.StartsWith(wxT("[Local] "))) {
				typePrefix = wxT("[Local] ");
				tabText = tabText.Mid(8); // Remove "[Local] "
			} else if (tabText.StartsWith(wxT("[ED2K] "))) {
				typePrefix = wxT("[ED2K] ");
				tabText = tabText.Mid(7); // Remove "[ED2K] "
			} else if (tabText.StartsWith(wxT("[Kad] "))) {
				typePrefix = wxT("[Kad] ");
				tabText = tabText.Mid(6); // Remove "[Kad] "
			}

			// Remove any existing state prefix (if different from type prefix)
			if (tabText.StartsWith(wxT("["))) {
				size_t stateEnd = tabText.Find(wxT("]"));
				if (stateEnd != wxString::npos) {
					tabText = tabText.Mid(stateEnd + 2); // Skip "] "
				}
			}

			// Remove any existing count suffix
			int parenPos = tabText.Find(wxT(" ("));
			if (parenPos != wxNOT_FOUND) {
				tabText = tabText.Left(parenPos);
			}

			// Get the result counts
			size_t shown = list->GetItemCount();
			size_t hidden = list->GetHiddenItemCount();

			// Build the new tab text with type prefix and state
			wxString newText = typePrefix;
			if (!state.IsEmpty()) {
				newText += wxT("[") + state + wxT("] ") + tabText;
			} else {
				newText += tabText;
			}

			// Add count information
			if (hidden) {
				newText += CFormat(wxT(" (%u/%u)")) % shown % (shown + hidden);
			} else {
				newText += CFormat(wxT(" (%u)")) % shown;
			}

			notebook->SetPageText(i, newText);
			break;
		}
	}
}

void UpdateHitCountWithState(CSearchListCtrl* page, CSearchDlg* parentDlg)
{
	if (!page || !parentDlg) {
		return;
	}

	// Determine the search state based on result count
	size_t shown = page->GetItemCount();
	size_t hidden = page->GetHiddenItemCount();

	wxString stateStr;
	if (shown == 0 && hidden == 0) {
		// No results yet - check if search is in progress
		long searchId = page->GetSearchId();
		if (searchId != 0) {
			// Search is active but no results yet
			stateStr = wxT("Searching");
		} else {
			// Search completed with no results
			stateStr = wxT("No Results");
		}
	} else {
		// Results are being populated or already populated
		// Reset retry counter when results are found
		page->ResetRetryCount();
		// Clear state when results are shown - this removes [Searching], [Retrying N], etc.
		// Even if the search is still active, we clear the state when results are present
		stateStr = wxEmptyString;
	}

	// Update the tab label with state information
	UpdateSearchState(page, parentDlg, stateStr);
}

bool RetrySearchWithState(CSearchListCtrl* page, CSearchDlg* parentDlg)
{
	if (!page || !parentDlg) {
		return false;
	}

	// Get the notebook from parent dialog
	CMuleNotebook* notebook = parentDlg->GetNotebook();
	if (!notebook) {
		return false;
	}

	// Find the page index
	int pageIndex = -1;
	for (uint32 i = 0; i < (uint32)notebook->GetPageCount(); ++i) {
		if (notebook->GetPage(i) == page) {
			pageIndex = i;
			break;
		}
	}

	if (pageIndex == -1) {
		return false;
	}

	// Get the search ID and tab text
	long searchId = page->GetSearchId();
	wxString tabText = notebook->GetPageText(pageIndex);

	// Determine search type from tab text
	bool isKadSearch = tabText.StartsWith(wxT("[Kad]")) || tabText.StartsWith(wxT("!"));
	bool isEd2kSearch = tabText.StartsWith(wxT("[Local] ")) || tabText.StartsWith(wxT("[ED2K] "));

	// Only retry ED2K searches (Local/Global), not Kad searches
	if (!isEd2kSearch) {
		return false;
	}

	// Check retry limit (max 3 retries)
	const int MAX_RETRIES = 3;
	if (page->GetRetryCount() >= MAX_RETRIES) {
		// Maximum retries reached - show final state
		UpdateSearchState(page, parentDlg, wxT("No Results"));
		return false;
	}

	// Increment retry counter
	page->IncrementRetryCount();

	// Update state to show retry is in progress with count
	wxString retryState = CFormat(wxT("Retrying %d")) % page->GetRetryCount();
	UpdateSearchState(page, parentDlg, retryState);

	// Request more results for this search
	// Note: RequestMoreResults creates a new search with a new ID
	// We need to update the page's search ID to match the new search
	wxString error = theApp->searchlist->RequestMoreResults(searchId);

	if (!error.IsEmpty()) {
		// Retry failed - update state to show error
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Get the new search ID from the search list
	// The new search should be the most recent one with the same parameters
	CSearchList::CSearchParams params = theApp->searchlist->GetSearchParams(searchId);
	if (!params.searchString.IsEmpty()) {
		// Find the new search ID by getting the current search ID
		// RequestMoreResults creates a new search and updates the current search ID
		long newSearchId = theApp->searchlist->GetCurrentSearchID();
		
		if (newSearchId != searchId && newSearchId != 0) {
			// Update the page's search ID to match the new search
			page->ShowResults(newSearchId);
		}
	}

	// Retry initiated successfully - state will be updated to "Searching"
	// when the search starts
	return true;
}

bool RetryKadSearchWithState(CSearchListCtrl* page, CSearchDlg* parentDlg)
{
	if (!page || !parentDlg) {
		return false;
	}

	// Get the notebook from parent dialog
	CMuleNotebook* notebook = parentDlg->GetNotebook();
	if (!notebook) {
		return false;
	}

	// Find the page index
	int pageIndex = -1;
	for (uint32 i = 0; i < (uint32)notebook->GetPageCount(); ++i) {
		if (notebook->GetPage(i) == page) {
		pageIndex = i;
		break;
		}
	}

	if (pageIndex == -1) {
		return false;
	}

	// Get the search ID and tab text
	long searchId = page->GetSearchId();
	wxString tabText = notebook->GetPageText(pageIndex);

	// Determine search type from tab text
	// Check for [Kad] prefix or ! prefix (which indicates Kad search in progress)
	// Also check if the tab text contains [Kad] anywhere (it might be after state info)
	bool isKadSearch = tabText.StartsWith(wxT("[Kad]")) || 
					   tabText.StartsWith(wxT("!")) ||
					   tabText.Contains(wxT("[Kad]"));

	// Only retry Kad searches
	if (!isKadSearch) {
		return false;
	}

	// Check retry limit (max 3 retries)
	const int MAX_RETRIES = 3;
	if (page->GetRetryCount() >= MAX_RETRIES) {
		// Maximum retries reached - show final state
		UpdateSearchState(page, parentDlg, wxT("No Results"));
		return false;
	}

	// Increment retry counter
	page->IncrementRetryCount();

	// Update state to show retry is in progress with count
	wxString retryState = CFormat(wxT("Retrying %d")) % page->GetRetryCount();
	UpdateSearchState(page, parentDlg, retryState);

	// Get the search parameters for this search
	CSearchList::CSearchParams params = theApp->searchlist->GetSearchParams(searchId);
	if (params.searchString.IsEmpty()) {
		// No search parameters available - cannot retry
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Start a new Kad search with the same parameters
	// Use the same search ID to keep results in the same tab
	uint32 newSearchId = searchId;
	wxString error = theApp->searchlist->StartNewSearch(&newSearchId, KadSearch, params);

	if (!error.IsEmpty()) {
		// Retry failed - update state to show error
		UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
		return false;
	}

	// Update the page to show results from the new search
	page->ShowResults(newSearchId);

	// Retry initiated successfully - state will be updated to "Searching"
	// when the search starts
	return true;
}
