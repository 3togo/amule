# Search Auto-Retry Implementation Design

## Requirements

1. **Auto-retry on zero results**: If Kad/ED2K/Local search returns 0 hits, automatically retry with delays
2. **More button failure handling**: If "More" button cannot complete (stuck in "updating" state), provide recovery mechanism

## Current State Analysis

### Existing Kad Retry Implementation
- Location: `SearchList.cpp` lines 1164-1201
- Current behavior:
  - Only retries once if no results
  - No delay between retries
  - No configurable retry limit
  - No user notification of retry attempts

### Current ED2K/Local Search Behavior
- No auto-retry mechanism
- Manual "More" button required
- No failure recovery

### Current "More" Button Issues
- Can get stuck in "updating" state
- No timeout mechanism
- No recovery if search fails
- No user feedback on failure

## Proposed Solution

### 1. Auto-Retry for Zero Results

#### Configuration
Add new preferences:
```cpp
// Preferences.h
int GetSearchAutoRetryCount() const;      // Default: 3
int GetSearchRetryDelay() const;          // Default: 5000ms (5 seconds)
bool GetSearchAutoRetryEnabled() const;    // Default: true
```

#### Implementation

**For Kad Searches:**
- Enhance existing retry logic (lines 1164-1201)
- Add delay between retries
- Make retry count configurable
- Add user notification
- Track retry attempts per search

**For ED2K/Local Searches:**
- Add similar retry mechanism
- Check result count after search completes
- Retry with same parameters
- Delay between attempts
- Stop after max retries

**Retry Logic:**
```cpp
void CSearchList::OnSearchComplete(long searchId, SearchType type)
{
    // Check result count
    ResultMap::iterator it = m_results.find(searchId);
    bool hasResults = (it != m_results.end()) && !it->second.empty();

    // Get retry count for this search
    int retryCount = GetSearchRetryCount(searchId);
    int maxRetries = thePrefs::GetSearchAutoRetryCount();

    if (!hasResults && retryCount < maxRetries && 
        thePrefs::GetSearchAutoRetryEnabled()) {

        // Increment retry count
        SetSearchRetryCount(searchId, retryCount + 1);

        // Show notification
        NotifySearchRetry(searchId, retryCount + 1, maxRetries);

        // Schedule retry with delay
        ScheduleSearchRetry(searchId, type, 
            thePrefs::GetSearchRetryDelay());

        return; // Don't mark as finished yet
    }

    // Mark search as finished
    MarkSearchFinished(searchId);
}
```

### 2. More Button Failure Recovery

#### Timeout Mechanism
- Add timeout for "More" button searches
- Default timeout: 30 seconds
- Configurable via preferences

#### State Tracking
```cpp
// Track "More" button state
struct MoreSearchState {
    long searchId;
    wxDateTime startTime;
    bool isActive;
    int timeoutSeconds;
};

std::map<long, MoreSearchState> m_moreSearchStates;
```

#### Recovery Logic
```cpp
void CSearchDlg::CheckMoreSearchTimeouts()
{
    wxDateTime now = wxDateTime::Now();

    for (auto& pair : m_moreSearchStates) {
        MoreSearchState& state = pair.second;

        if (state.isActive) {
            wxTimeSpan elapsed = now - state.startTime;

            if (elapsed.GetSeconds().ToLong() >= state.timeoutSeconds) {
                // Timeout occurred
                HandleMoreSearchTimeout(state.searchId);
            }
        }
    }
}

void CSearchDlg::HandleMoreSearchTimeout(long searchId)
{
    // Remove "updating" from tab text
    UpdateTabText(searchId, false);

    // Show error to user
    wxMessageBox(
        _("Search request timed out. The search may have completed "
          "but the status was not updated. Please check the results."),
        _("Search Timeout"),
        wxOK | wxICON_WARNING);

    // Clean up state
    m_moreSearchStates.erase(searchId);

    // Re-enable buttons
    FindWindow(IDC_STARTS)->Enable();
    FindWindow(IDC_SDOWNLOAD)->Enable();
    FindWindow(IDC_CANCELS)->Disable();
}
```

#### User Recovery Options
When timeout occurs, offer options:
1. **Check Results**: Results may have arrived
2. **Retry**: Try "More" button again
3. **Cancel**: Give up on this search

## Implementation Plan

### Phase 1: Configuration (Priority: High)
1. Add preference settings
2. Update Preferences UI
3. Add configuration validation

### Phase 2: Kad Retry Enhancement (Priority: High)
1. Add delay mechanism
2. Make retry count configurable
3. Add user notifications
4. Track retry attempts per search

### Phase 3: ED2K/Local Retry (Priority: Medium)
1. Implement result checking
2. Add retry logic
3. Integrate with existing search flow

### Phase 4: More Button Recovery (Priority: High)
1. Add timeout mechanism
2. Implement state tracking
3. Add recovery handler
4. Update UI feedback

### Phase 5: Testing (Priority: High)
1. Unit tests for retry logic
2. Integration tests
3. User acceptance testing
4. Performance testing

## Configuration Values

### Recommended Defaults
- **Auto-retry enabled**: Yes
- **Max retry attempts**: 3
- **Retry delay**: 5 seconds
- **More button timeout**: 30 seconds

### Rationale
- 3 retries balance between persistence and user patience
- 5 second delay allows network to stabilize
- 30 second timeout prevents indefinite waiting

## User Experience

### Normal Flow
1. User starts search
2. Search completes with 0 results
3. System automatically retries (up to 3 times)
4. User sees notification: "Retrying search (1/3)..."
5. If still no results after retries, search marked as complete

### More Button Flow
1. User clicks "More"
2. Tab shows "updating..."
3. If timeout occurs:
   - User sees error dialog
   - Options to check results, retry, or cancel
   - Tab text updated to remove "updating"
4. User can take appropriate action

## Error Handling

### Retry Failures
- Log retry attempts
- Track failure reasons
- Limit retry attempts
- Notify user of final failure

### Network Issues
- Detect connection problems
- Adjust retry delay dynamically
- Skip retries if network unavailable
- Inform user of network status

## Performance Considerations

### Resource Management
- Limit concurrent retries
- Clean up completed retry states
- Avoid retry storms
- Monitor system load

### Network Impact
- Respect rate limits
- Stagger retry attempts
- Minimize unnecessary traffic
- Adapt to network conditions

## Future Enhancements

1. **Smart Retry**: Analyze failure reasons and adjust strategy
2. **Progressive Delay**: Increase delay between retries
3. **Result Caching**: Cache results during retries
4. **User Preferences**: Allow per-search-type retry settings
5. **Analytics**: Track retry success rates
