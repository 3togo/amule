# ED2K Search Fix - Complete Guide

## Problem
ED2K searches were failing because new and old code paths were being mixed, causing conflicts and immediate search termination.

## Solution
Replace the `RetrySearchWithState` function in SearchLabelHelper.cpp with the new controller-based version.

## Files Created

1. **SearchLabelHelper_RetrySearchWithState_new.cpp** - New version of the function
2. **SearchDlg_StoreController.patch** - StoreController method for SearchDlg
3. **SEARCH_FIX_README.md** - Detailed explanation of the fix

## Step-by-Step Instructions

### Step 1: Replace RetrySearchWithState in SearchLabelHelper.cpp

1. Open `/home/eli/git/amule/src/SearchLabelHelper.cpp`
2. Find the `RetrySearchWithState` function (starts at line 162)
3. Replace the entire function with the content from `SearchLabelHelper_RetrySearchWithState_new.cpp`
4. Save the file

### Step 2: Add StoreController to SearchDlg

#### SearchDlg.h
Add this public method declaration (after `StartNewSearch()`):
```cpp
// Method to store search controller (needed for retry functionality)
void StoreController(uint32_t searchId, std::unique_ptr<search::SearchController> controller);
```

#### SearchDlg.cpp
Add this method implementation (anywhere in the file):
```cpp
void CSearchDlg::StoreController(uint32_t searchId, std::unique_ptr<search::SearchController> controller)
{
	m_searchControllers[searchId] = std::move(controller);
}
```

### Step 3: Rebuild

```bash
cd /home/eli/git/amule/build
make -j$(nproc)
```

### Step 4: Test

Run aMule and test ED2K searches (Local and Global). They should now work correctly.

## Key Changes

The main change is in `RetrySearchWithState`:

**Old code:**
```cpp
wxString error = theApp->searchlist->StartNewSearch(&newSearchId, searchType, params);
```

**New code:**
```cpp
// Create SearchController using factory
auto controller = search::SearchControllerFactory::createController(modernSearchType);
// Set up callbacks
parentDlg->SetupControllerCallbacks(controller.get());
// Start search
controller->startSearch(modernParams);
// Store controller
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
