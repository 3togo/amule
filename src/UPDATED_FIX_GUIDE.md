# ED2K Search Fix - Updated Guide

## Problem
ED2K searches were failing because new and old code paths were being mixed, causing conflicts and immediate search termination.

## Compilation Error Fix
The first attempt failed because `SetupControllerCallbacks` and `m_searchControllers` are private members.
We need to make them public.

## Files Created

1. **SearchLabelHelper_RetrySearchWithState_new.cpp** - New version of RetrySearchWithState function
2. **SearchDlg_PrivateToPublic.patch** - Instructions to move private members to public
3. **UPDATED_FIX_GUIDE.md** - This file

## Step-by-Step Instructions

### Step 1: Modify SearchDlg.h

Open `/home/eli/git/amule/src/SearchDlg.h` and make these changes:

#### 1.1. Move these methods from private to public section

Find the private section (around line 196):
```cpp
private:
	// Helper methods for search controller
	void		SetupControllerCallbacks(search::SearchController* controller);
	search::SearchParams	CreateSearchParamsFromUI(const wxString& searchString, search::ModernSearchType searchType);
```

Move these two methods to the public section (before line 196):
```cpp
public:
	// Helper methods for search controller (needed by SearchLabelHelper)
	void		SetupControllerCallbacks(search::SearchController* controller);
	search::SearchParams	CreateSearchParamsFromUI(const wxString& searchString, search::ModernSearchType searchType);
```

#### 1.2. Add StoreController method to public section

Add this to the public section (after StartNewSearch()):
```cpp
	// Method to store search controller (needed for retry functionality)
	void		StoreController(uint32_t searchId, std::unique_ptr<search::SearchController> controller);
```

#### 1.3. Move m_searchControllers to public section

Find this in the private section (around line 251):
```cpp
	// Map of search ID to SearchController for active searches
	std::map<uint32, std::unique_ptr<search::SearchController>> m_searchControllers;
```

Move it to the public section.

### Step 2: Add StoreController implementation to SearchDlg.cpp

Add this method anywhere in SearchDlg.cpp:
```cpp
void CSearchDlg::StoreController(uint32_t searchId, std::unique_ptr<search::SearchController> controller)
{
	m_searchControllers[searchId] = std::move(controller);
}
```

### Step 3: Replace RetrySearchWithState in SearchLabelHelper.cpp

1. Open `/home/eli/git/amule/src/SearchLabelHelper.cpp`
2. Find `RetrySearchWithState` function (starts at line 162)
3. Replace entire function with content from `SearchLabelHelper_RetrySearchWithState_new.cpp`
4. Save file

### Step 4: Rebuild

```bash
cd /home/eli/git/amule/build
make -j$(nproc)
```

### Step 5: Test

Run aMule and test ED2K searches (Local and Global). They should now work correctly.

## Summary of Changes

1. **SearchDlg.h**:
   - Moved `SetupControllerCallbacks` from private to public
   - Moved `CreateSearchParamsFromUI` from private to public
   - Moved `m_searchControllers` from private to public
   - Added `StoreController` method declaration

2. **SearchDlg.cpp**:
   - Added `StoreController` method implementation

3. **SearchLabelHelper.cpp**:
   - Replaced `RetrySearchWithState` to use new controller-based approach

## Expected Result

After applying these changes:
1. ED2K searches will use the new controller-based code path
2. Search packets will be correctly created and sent to servers
3. Search results should be received properly
4. No more conflicts between old and new code paths
5. No more compilation errors

## Note

Kad searches may still have limited results due to firewall issues, but this is a network configuration problem, not a code bug.
