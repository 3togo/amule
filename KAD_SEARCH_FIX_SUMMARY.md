# Kad Search Fix Summary

## Problem
Clicking the start button in the search window was not returning any hits for Kad searches. The logs showed:
- Controller registered for search ID 1
- Immediately unregistered
- New controller registered for search ID 3
- Search showed "Searching" state but with 0 results

## Root Cause
The `KadSearchController::startSearch()` method had a search ID management issue:
1. The controller generated its own search ID and registered with SearchResultRouter
2. Then called `Kademlia::CSearchManager::PrepareFindKeywords()` which returned a different search ID
3. The controller re-registered with the new ID, but there was a timing/synchronization issue
4. Results were being sent with the Kad search manager's ID
5. But the controller registration state was inconsistent
6. SearchResultRouter couldn't reliably route results to the correct controller

## Solution
Modified `KadSearchController::startSearch()` to:
1. Remove premature search ID generation and registration
2. Let Kad search manager generate the search ID by passing 0 to `PrepareFindKeywords()`
3. Get the actual search ID from the Kad search object
4. Only then register with SearchResultRouter using the correct ID
5. Set the current search ID in SearchList after proper registration

Also fixed `isValidKadNetwork()` to properly check if Kad is running instead of always returning true.

## Key Changes in KadSearchController.cpp

### Before:
```cpp
// Generate search ID
searchId = GenerateSearchId();

// Store search ID and state
m_model->setSearchParams(params);
m_model->setSearchId(searchId);
m_model->setSearchState(SearchState::Searching);

// Register with SearchResultRouter for result routing
SearchResultRouter::Instance().RegisterController(searchId, this);

// Send packet to Kad network
if (theApp && Kademlia::CKademlia::IsRunning()) {
    // Set the current search ID in SearchList before sending
    if (theApp->searchlist) {
        theApp->searchlist->SetCurrentSearch(searchId);
    }

    // Use legacy Kad search implementation
    try {
        Kademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(
            params.strKeyword,
            packetSize,
            packetData,
            searchId
        );

        // Update search ID from Kad search manager
        uint32_t oldSearchId = searchId;
        searchId = search->GetSearchID();
        m_model->setSearchId(searchId);

        // Re-register with new search ID
        SearchResultRouter::Instance().UnregisterController(oldSearchId);
        SearchResultRouter::Instance().RegisterController(searchId, this);

        notifySearchStarted(searchId);
    } catch (const wxString& what) {
        error = wxString::Format(_("Failed to start Kad search: %s"), what.c_str());
        handleSearchError(searchId, error);
    }
```

### After:
```cpp
// Send packet to Kad network
if (theApp && Kademlia::CKademlia::IsRunning()) {
    // Use legacy Kad search implementation
    try {
        // Let Kad search manager generate the search ID
        Kademlia::CSearch* search = Kademlia::CSearchManager::PrepareFindKeywords(
            params.strKeyword,
            packetSize,
            packetData,
            0  // Let Kad search manager generate the search ID
        );

        // Get the actual search ID from Kad search manager
        uint32_t searchId = search->GetSearchID();

        // Store search ID and state
        m_model->setSearchParams(params);
        m_model->setSearchId(searchId);
        m_model->setSearchState(SearchState::Searching);

        // Register with SearchResultRouter for result routing
        SearchResultRouter::Instance().RegisterController(searchId, this);

        // Set the current search ID in SearchList after registration
        if (theApp->searchlist) {
            theApp->searchlist->SetCurrentSearch(searchId);
        }

        notifySearchStarted(searchId);
    } catch (const wxString& what) {
        error = wxString::Format(_("Failed to start Kad search: %s"), what.c_str());
        handleSearchError(0, error);
    }
```

## Benefits
1. **Proper Search ID Synchronization**: The controller is now registered with the exact same search ID that Kad uses
2. **Reliable Result Routing**: Results are properly routed through SearchResultRouter to the correct controller
3. **Correct UI Updates**: The UI receives and displays search results correctly
4. **Thread Safety**: Each search operation is more atomic and self-contained
5. **Phased Out Legacy System**: Reduced dependency on global variables and legacy search ID management

## Testing
After rebuilding the project, Kad searches should:
1. Properly register with SearchResultRouter using the correct search ID
2. Receive and display search results correctly
3. Update the UI with the correct hit counts
4. Show proper search state in the tab labels

## Build Instructions
```bash
cd /home/eli/git/amule
make
```

The fix has been applied to `/home/eli/git/amule/src/search/KadSearchController.cpp`.
