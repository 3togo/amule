# Search Architecture Implementation

## Overview

This document describes the new search architecture implemented in aMule, which provides a modular, extensible framework for handling searches across different networks (eD2k and Kad).

## Architecture Components

### 1. Core Interfaces

#### SearchController
The abstract base class that defines the interface for all search controllers.

**Key Features:**
- Abstract interface for search operations
- Callback-based notification system
- Detailed progress reporting support
- Thread-safe result access

**Main Methods:**
- `startSearch()` - Initiates a new search
- `stopSearch()` - Stops the current search
- `requestMoreResults()` - Requests additional results
- `getResults()` - Retrieves search results
- `getResultCount()` - Gets the number of results

**Callbacks:**
- `SearchStartedCallback` - Notifies when search starts
- `SearchCompletedCallback` - Notifies when search completes
- `ResultsReceivedCallback` - Notifies when new results arrive
- `ErrorCallback` - Reports errors
- `ProgressCallback` - Reports progress (0-100%)
- `DetailedProgressCallback` - Reports detailed progress information

#### SearchModel
Data model that manages search parameters, state, and results.

**Features:**
- Thread-safe parameter management
- Result caching mechanism
- State tracking
- Search ID management

**Key Methods:**
- Parameter management: `setSearchParams()`, `getSearchParams()`
- Result management: `addResult()`, `getResults()`, `getResultCount()`
- State management: `setSearchState()`, `getSearchState()`
- Caching: `cacheResults()`, `clearCachedResults()`

### 2. Network-Specific Controllers

#### ED2KSearchController
Specialized controller for eD2k network searches.

**Features:**
- Handles both local and global eD2k searches
- Configurable server query limits
- Automatic retry mechanism
- Detailed progress tracking
- Efficient result aggregation

**Configuration Options:**
- `setMaxServersToQuery()` - Maximum servers to contact
- `setRetryCount()` - Number of retry attempts

**Progress Information:**
- Percentage of servers contacted
- Number of servers contacted
- Results received count
- Current status message

#### KadSearchController
Specialized controller for Kademlia network searches.

**Features:**
- Keyword-based searches
- Configurable node query limits
- Retry mechanism for failed searches
- Progress tracking specific to Kad network

**Configuration Options:**
- `setMaxNodesToQuery()` - Maximum nodes to contact
- `setRetryCount()` - Number of retry attempts

**Note:** Kad searches query the entire network and don't support traditional "more results" requests.

#### LegacySearchController
Adapter that wraps the old search system for backward compatibility.

**Purpose:**
- Maintains compatibility with existing code
- Provides migration path to new architecture
- Uses existing CSearchList implementation

### 3. Factory Pattern

#### SearchControllerFactory
Factory for creating appropriate search controllers.

**Factory Methods:**
- `createController()` - Main factory method
- `createLegacyController()` - Creates legacy adapter
- `createED2KController()` - Creates eD2k controller
- `createKadController()` - Creates Kad controller
- `createModernController()` - Creates modern controllers

**Usage Example:**
```cpp
auto controller = SearchControllerFactory::createController(ModernSearchType::GlobalSearch);
controller->setOnProgress([](int progress) {
    // Handle progress updates
});
controller->startSearch(searchParams);
```

## Data Flow

### Search Initialization
1. User initiates search through UI
2. SearchDlg creates SearchParams
3. Factory creates appropriate controller
4. Controller initializes SearchModel
5. Search starts on selected network

### Result Processing
1. Network returns results
2. Controller processes results
3. Results cached in SearchModel
4. UI notified via callbacks
5. Results displayed to user

### Progress Updates
1. Controller tracks progress
2. ProgressInfo structure updated
3. Detailed progress callback triggered
4. UI updates progress indicators

## Thread Safety

All components are designed for thread-safe operation:
- SearchModel uses wxMutex for synchronization
- Result access is protected
- Callbacks are thread-safe
- State changes are atomic

## Migration Path

### Phase 1: Foundation (Complete)
- Basic controller interfaces
- Legacy adapter implementation
- Result management
- Progress reporting

### Phase 2: Network Controllers (Complete)
- ED2KSearchController
- KadSearchController
- Factory implementation
- Configuration options

### Phase 3: Enhanced Features (Future)
- Search queue management
- Result filtering and sorting
- Advanced search parameters
- Search history

### Phase 4: Optimization (Future)
- Efficient result storage
- Caching improvements
- Performance optimizations
- Memory management

## Configuration

### eD2k Search Settings
```cpp
auto controller = SearchControllerFactory::createED2KController();
controller->setMaxServersToQuery(100);
controller->setRetryCount(3);
```

### Kad Search Settings
```cpp
auto controller = SearchControllerFactory::createKadController();
controller->setMaxNodesToQuery(500);
controller->setRetryCount(3);
```

## Error Handling

All controllers implement comprehensive error handling:
- Invalid parameters detected early
- Network errors reported via callbacks
- State transitions validated
- Retry logic for transient failures

## Future Enhancements

1. **Search Queue**
   - Priority-based scheduling
   - Concurrent search limits
   - Search throttling

2. **Result Management**
   - Advanced filtering
   - Sorting options
   - Deduplication
   - Pagination

3. **Search Analytics**
   - Search history
   - Performance metrics
   - Usage statistics

4. **Modern Protocol Support**
   - Direct protocol communication
   - Optimized packet handling
   - Enhanced security

## Testing

The architecture supports comprehensive testing:
- Unit tests for each component
- Integration tests for search flow
- Mock implementations for testing
- Performance benchmarks

## Conclusion

The new search architecture provides a solid foundation for future enhancements while maintaining backward compatibility with existing code. The modular design allows for easy extension and optimization of search functionality across different networks.
