# Search Auto-Retry Integration Guide

## Overview

This document describes how to integrate the SearchAutoRetry class into the existing search system.

## Files Created

1. **src/search/SearchAutoRetry.h** - Header file for auto-retry manager
2. **src/search/SearchAutoRetry.cpp** - Implementation of auto-retry logic
3. **docs/SEARCH_AUTO_RETRY_DESIGN.md** - Complete design documentation

## Integration Steps

### Step 1: Update CMakeLists.txt

Add new source files to the build system:

```cmake
# In cmake/source-vars.cmake, add to CORE_SOURCES:
src/search/SearchAutoRetry.cpp
```

### Step 2: Integrate into SearchList

Add auto-retry manager to CSearchList:

**In SearchList.h:**
```cpp
#include "search/SearchAutoRetry.h"

class CSearchList {
private:
    // Auto-retry manager
    search::SearchAutoRetry m_autoRetry;

    // Track result counts per search
    std::map<long, int> m_resultCounts;

    // Track search completion
    void OnSearchComplete(long searchId, SearchType type, bool hasResults);

    // Retry callback
    void OnSearchRetry(long searchId, search::ModernSearchType type, int retryNum);
};
```

**In SearchList.cpp:**
```cpp
// In constructor, initialize auto-retry
CSearchList::CSearchList()
    : m_autoRetry()
{
    // Set up retry callback
    m_autoRetry.SetOnRetry(
        [this](long searchId, search::ModernSearchType type, int retryNum) {
            OnSearchRetry(searchId, type, retryNum);
        }
    );
}

// When search completes, check for retry
void CSearchList::OnSearchComplete(long searchId, SearchType type, bool hasResults)
{
    // Update result count
    m_resultCounts[searchId] = GetResultCount(searchId);

    // Check if we should retry
    if (!hasResults && m_autoRetry.ShouldRetry(searchId)) {
        // Increment retry count
        m_autoRetry.IncrementRetryCount(searchId);

        // Schedule retry
        m_autoRetry.StartRetry(searchId, 
            type == KadSearch ? search::ModernSearchType::KadSearch :
            type == LocalSearch ? search::ModernSearchType::LocalSearch :
            search::ModernSearchType::GlobalSearch);

        // Log retry
        AddDebugLogLineC(logSearch, 
            wxString::Format(_("Search %ld returned no results, scheduling retry (%d/%d)"),
                searchId, m_autoRetry.GetRetryCount(searchId), 
                m_autoRetry.GetMaxRetryCount()));

        return; // Don't mark as finished yet
    }

    // Mark search as finished
    if (type == KadSearch) {
        m_KadSearchFinished = true;
    } else {
        m_searchInProgress = false;
        Notify_SearchLocalEnd();
    }
}

// Handle retry callback
void CSearchList::OnSearchRetry(long searchId, search::ModernSearchType type, int retryNum)
{
    // Get original parameters
    CSearchParams params = GetSearchParams(searchId);
    if (params.searchString.IsEmpty()) {
        return;
    }

    // Convert to SearchType
    SearchType searchType;
    switch (type) {
        case search::ModernSearchType::KadSearch:
            searchType = KadSearch;
            break;
        case search::ModernSearchType::LocalSearch:
            searchType = LocalSearch;
            break;
        case search::ModernSearchType::GlobalSearch:
        default:
            searchType = GlobalSearch;
            break;
    }

    // Start new search with same parameters
    uint32 newSearchId = 0;
    wxString error = StartNewSearch(&newSearchId, searchType, params);

    if (!error.IsEmpty()) {
        AddDebugLogLineC(logSearch, 
            wxString::Format(_("Retry %d for search %ld failed: %s"),
                retryNum, searchId, error));
        return;
    }

    // Update search ID mapping
    m_resultCounts[newSearchId] = m_resultCounts[searchId];
    m_resultCounts.erase(searchId);

    // Log success
    AddDebugLogLineC(logSearch, 
        wxString::Format(_("Retry %d started for search %ld (new ID: %u)"),
                retryNum, searchId, newSearchId));
}
```

### Step 3: Integrate into SearchDlg

Add timeout handling for "More" button:

**In SearchDlg.h:**
```cpp
#include "search/SearchAutoRetry.h"

class CSearchDlg {
private:
    // Auto-retry manager
    search::SearchAutoRetry m_autoRetry;

    // Timer for checking timeouts
    wxTimer m_timeoutCheckTimer;

    // Handle timeout checks
    void OnTimeoutCheck(wxTimerEvent& event);

    // Handle "More" button timeout
    void HandleMoreButtonTimeout(long searchId);
};
```

**In SearchDlg.cpp:**
```cpp
// In constructor
CSearchDlg::CSearchDlg(wxWindow* pParent)
    : m_autoRetry()
    , m_timeoutCheckTimer(this)
{
    // Set up timeout check timer (check every 5 seconds)
    m_timeoutCheckTimer.Start(5000);

    // Connect event
    Bind(wxEVT_TIMER, &CSearchDlg::OnTimeoutCheck, this, m_timeoutCheckTimer.GetId());
}

// When "More" button is clicked
void CSearchDlg::OnBnClickedMore(wxCommandEvent& WXUNUSED(event))
{
    // ... existing code ...

    // Start timeout tracking
    long searchId = list->GetSearchId();
    m_autoRetry.StartMoreButtonTimeout(searchId);
}

// Check for timeouts
void CSearchDlg::OnTimeoutCheck(wxTimerEvent& event)
{
    if (m_notebook->GetPageCount() > 0) {
        CSearchListCtrl* list = static_cast<CSearchListCtrl*>(
            m_notebook->GetPage(m_notebook->GetSelection()));

        long searchId = list->GetSearchId();

        if (m_autoRetry.IsMoreButtonTimedOut(searchId)) {
            HandleMoreButtonTimeout(searchId);
        }
    }
}

// Handle timeout
void CSearchDlg::HandleMoreButtonTimeout(long searchId)
{
    // Stop tracking
    m_autoRetry.StopMoreButtonTimeout(searchId);

    // Update tab text
    wxString tabText = m_notebook->GetPageText(m_notebook->GetSelection());
    if (tabText.Contains(wxT("(updating...)"))) {
        tabText.Replace(wxT("(updating...)"), wxT(""));
        m_notebook->SetPageText(m_notebook->GetSelection(), tabText);
    }

    // Show message to user
    wxMessageBox(
        _("The 'More' button request timed out. The search may have "
          "completed but the status was not updated.

"
          "Please check the results in the current tab. If no new "
          "results appear, you can try clicking 'More' again."),
        _("Search Timeout"),
        wxOK | wxICON_WARNING);

    // Re-enable buttons
    FindWindow(IDC_STARTS)->Enable();
    FindWindow(IDC_SDOWNLOAD)->Enable();
    FindWindow(IDC_CANCELS)->Disable();
}
```

## Configuration

### Default Values
- **Max retry count**: 3
- **Retry delay**: 5 seconds (5000ms)
- **More button timeout**: 30 seconds

### Customizing Values

```cpp
// In application initialization
theApp->searchlist->GetAutoRetry()->SetMaxRetryCount(5);
theApp->searchlist->GetAutoRetry()->SetRetryDelay(10000); // 10 seconds
theApp->searchlist->GetAutoRetry()->SetMoreButtonTimeout(60); // 60 seconds
```

## Testing

### Test Case 1: Zero Results Auto-Retry
1. Start a search with unlikely keywords
2. Wait for search to complete with 0 results
3. Verify automatic retry occurs
4. Check retry count increments
5. Verify max retry limit is respected

### Test Case 2: More Button Timeout
1. Start a search
2. Click "More" button
3. Simulate timeout (wait 30+ seconds)
4. Verify timeout message appears
5. Check tab text is updated
6. Verify buttons are re-enabled

### Test Case 3: Successful Retry
1. Start a search
2. Wait for 0 results
3. Let retry occur
4. Verify new search starts
5. Check results are combined

## Monitoring

### Logging
All retry attempts are logged:
```
[Search] Search 123 returned no results, scheduling retry (1/3)
[Search] Retry 1 started for search 123 (new ID: 124)
[Search] More button search timed out for search ID: 123
```

### Debug Information
Enable debug logging for detailed information:
```cpp
AddDebugLogLineC(logSearch, wxT("Search retry details: ..."));
```

## Performance Considerations

### Resource Management
- Auto-retry manager cleans up completed searches
- Timer-based approach is efficient
- No polling overhead

### Network Impact
- Configurable delays prevent retry storms
- Respects network conditions
- Limits concurrent retries

## Future Enhancements

1. **Progressive Delays**: Increase delay between retries
2. **Smart Retry**: Analyze failure reasons
3. **User Preferences**: Per-search-type settings
4. **Statistics**: Track retry success rates
5. **UI Feedback**: Show retry progress to user
