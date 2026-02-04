# ED2K Search Fix - New/Old Code Mixing Issue

## Problem
ED2K searches were failing because new and old code paths were being mixed:
- Search packets were created using old code (SearchList::CreateSearchData)
- Controllers were created using new code (ED2KSearchController)
- This caused conflicts and searches to stop immediately

## Root Cause
In SearchLabelHelper.cpp, the RetrySearchWithState function was calling:
```cpp
wxString error = theApp->searchlist->StartNewSearch(&newSearchId, searchType, params);
```
This uses the old code path instead of the new controller-based approach.

## Solution
Replace the old code in RetrySearchWithState with new controller-based code.

## Files to Modify

### 1. SearchDlg.h
Add this public method declaration (after StartNewSearch()):
```cpp
// Method to store search controller (needed for retry functionality)
void StoreController(uint32_t searchId, std::unique_ptr<search::SearchController> controller);
```

### 2. SearchDlg.cpp
Add this method implementation:
```cpp
void CSearchDlg::StoreController(uint32_t searchId, std::unique_ptr<search::SearchController> controller)
{
	m_searchControllers[searchId] = std::move(controller);
}
```

### 3. SearchLabelHelper.cpp
In RetrySearchWithState function (around line 233), replace:
```cpp
// Start a new ED2K search with the same parameters
// Use the same search ID to keep results in the same tab
uint32 newSearchId = searchId;
// Determine if this is a Local or Global search
SearchType searchType = LocalSearch;
if (tabText.StartsWith(wxT("[Global] "))) {
	searchType = GlobalSearch;
}

wxString error = theApp->searchlist->StartNewSearch(&newSearchId, searchType, params);

if (!error.IsEmpty()) {
	// Retry failed - update state to show error
	UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
	return false;
}
```

With:
```cpp
// Start a new ED2K search with the same parameters
// Use the same search ID to keep results in the same tab
// Determine if this is a Local or Global search
search::ModernSearchType modernSearchType = search::ModernSearchType::LocalSearch;
if (tabText.StartsWith(wxT("[Global] "))) {
	modernSearchType = search::ModernSearchType::GlobalSearch;
}

// Convert legacy params to modern params
search::SearchParams modernParams;
modernParams.searchString = params.searchString;
modernParams.strKeyword = params.strKeyword;
modernParams.typeText = params.typeText;
modernParams.extension = params.extension;
modernParams.minSize = params.minSize;
modernParams.maxSize = params.maxSize;
modernParams.availability = params.availability;
modernParams.searchType = modernSearchType;

// Create SearchController using factory
auto controller = search::SearchControllerFactory::createController(modernSearchType);
if (!controller) {
	// Retry failed - update state to show error
	UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
	return false;
}

// Set up callbacks for the controller
parentDlg->SetupControllerCallbacks(controller.get());

// Start the search
controller->startSearch(modernParams);

// Get the search ID after starting
uint32_t newSearchId = controller->getSearchId();
if (newSearchId == 0) {
	// Retry failed - update state to show error
	UpdateSearchState(page, parentDlg, wxT("Retry Failed"));
	return false;
}

// Store the controller
parentDlg->m_searchControllers[newSearchId] = std::move(controller);
```

## Expected Result
After applying these changes:
1. ED2K searches will use the new controller-based code path
2. Search packets will be correctly created and sent to servers
3. Search results should be received properly
4. No more conflicts between old and new code paths

## Note
Kad searches may still have limited results due to firewall issues, but this is a network configuration problem, not a code bug.
