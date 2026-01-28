
# Search State Management Architecture

## Overview

This document describes the new architecture for managing search state in aMule. The architecture combines a centralized state manager with the observer pattern to ensure atomic state operations while maintaining good separation between GUI and business logic.

## Components

### 1. SearchStateManager Class

The `SearchStateManager` class is responsible for:
- Managing the state of all search tabs atomically
- Tracking search metadata (type, keyword, result counts, retry count)
- Implementing state transitions automatically based on events
- Notifying observers of state changes

#### Key Methods:
- `InitializeSearch()`: Initialize a new search
- `UpdateResultCount()`: Update the result count for a search
- `EndSearch()`: Mark a search as ended
- `StartRetry()`: Start a retry for a search
- `GetSearchState()`: Get the current state of a search
- `GetRetryCount()`: Get the retry count for a search
- `RegisterObserver()`: Register an observer for state changes
- `UnregisterObserver()`: Unregister an observer

### 2. ISearchStateObserver Interface

The `ISearchStateObserver` interface defines the contract for classes that want to be notified of search state changes.

#### Key Methods:
- `OnSearchStateChanged()`: Called when the search state changes
- `OnRetryRequested()`: Called when a retry is requested for a search

### 3. SearchState Enumeration

The `SearchState` enumeration defines the possible states of a search:
- `STATE_IDLE`: Search not started
- `STATE_SEARCHING`: Search in progress
- `STATE_POPULATING`: Results are being populated
- `STATE_RETRYING`: Search is being retried
- `STATE_NO_RESULTS`: Search completed with no results
- `STATE_HAS_RESULTS`: Search has results

## Integration with Existing Code

### CSearchDlg Changes:
- Implements `ISearchStateObserver` interface
- Registers as observer with `SearchStateManager` in constructor
- Updated `CreateNewTab` to initialize search in state manager
- Modified `AddResult` and `UpdateResult` to use state manager
- Simplified `LocalSearchEnd` and `KadSearchEnd` to delegate to state manager
- Implemented `OnSearchStateChanged` to handle state updates
- Implemented `OnRetryRequested` to handle retry requests

## Benefits

1. **Atomic Operations**: All state changes go through the `SearchStateManager`, ensuring consistency
2. **GUI/Operation Separation**: Observer pattern decouples state logic from UI updates
3. **Maintainability**: Single source of truth for search state makes debugging easier
4. **Extensibility**: Easy to add new observers or state types in the future
5. **Automatic Retry Logic**: Kad searches automatically retry when no results are found

## State Transitions

### Initial State Flow:
1. User starts a search
2. `CreateNewTab` is called
3. `SearchStateManager::InitializeSearch` is called
4. State is set to `STATE_SEARCHING`
5. Observer is notified to update tab label

### Result Arrival Flow:
1. Results arrive from network
2. `AddResult` or `UpdateResult` is called
3. `SearchStateManager::UpdateResultCount` is called
4. If results are found, state is set to `STATE_HAS_RESULTS` and retry count is reset
5. Observer is notified to update tab label

### Search End Flow:
1. Search completes (either normally or with timeout)
2. `LocalSearchEnd` or `KadSearchEnd` is called
3. `SearchStateManager::EndSearch` is called
4. If no results and retry limit not reached, retry is requested
5. If retry succeeds, state is set to `STATE_RETRYING`
6. If retry fails or max retries reached, state is set to `STATE_NO_RESULTS`
7. Observer is notified to update tab label

### Retry Flow:
1. `SearchStateManager::EndSearch` determines a retry is needed
2. `SearchStateManager` calls `OnRetryRequested` on observers
3. `CSearchDlg::OnRetryRequested` initiates the actual retry
4. If retry succeeds, `SearchStateManager::StartRetry` is called
5. State is set to `STATE_RETRYING`
6. Observer is notified to update tab label

## Future Improvements

1. Add support for retrying ED2K searches
2. Add configurable retry limits per search type
3. Add support for custom retry strategies
4. Add metrics collection for search performance
5. Add support for search state persistence across restarts
