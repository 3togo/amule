# Magnet Progress Tracking - Developer Guide

## Architecture Overview

The magnet conversion progress tracking system consists of:

### Core Components
- `CMagnetProgressTracker` - Main thread class managing conversion process
- `CDownloadQueue::UpdateMagnetConversionProgress()` - GUI update interface
- `PS_CONVERTING_MAGNET` status - New part file status for conversions

### Class Hierarchy
```
wxThread
  └── CMagnetProgressTracker (implements Entry())
      
wxEvtHandler
  └── CMagnetProgressTracker (for GUI updates)
```

## Key Classes

### CMagnetProgressTracker
**Purpose**: Manages magnet-to-ed2k conversion with progress tracking

**Key Methods:**
- `StartConversion()` - Begins conversion process
- `UpdateProgress()` - Updates progress and handles stage transitions  
- `GetEstimatedTimeRemaining()` - Calculates ETA
- `GetConversionRate()` - Returns progress per second

**Member Variables:**
- `m_currentProgress` - Current progress (0.0-1.0)
- `m_currentStage` - Active conversion stage
- `m_startTime` - Conversion start timestamp
- `m_lastProgress` - Previous progress for rate calculation

## Conversion Stages

### Stage Flow
```cpp
enum ConversionStage {
    STAGE_PARSING,       // 0 - Parse magnet URI
    STAGE_TRACKER_QUERY, // 1 - Query torrent trackers  
    STAGE_DHT_LOOKUP,    // 2 - DHT network lookup
    STAGE_METADATA_FETCH,// 3 - Fetch file metadata
    STAGE_CONVERSION,    // 4 - Convert to ed2k format
    STAGE_COMPLETE,      // 5 - Conversion successful
    STAGE_ERROR          // 6 - Conversion failed
};
```

### Progress Mapping
- Parsing: 0.0 → 0.1 (10%)
- Tracker Query: 0.1 → 0.3 (30%)  
- DHT Lookup: 0.3 → 0.5 (50%)
- Metadata Fetch: 0.5 → 0.7 (70%)
- Conversion: 0.7 → 0.9 (90%)
- Complete: 1.0 (100%)

## Integration Points

### GUI Integration
**DownloadListCtrl.cpp**:
```cpp
// Visual progress rendering
if (file->GetStatus() == PS_CONVERTING_MAGNET) {
    // Blue progress bar for magnet conversions
    uint64 progressPos = file->GetMagnetConversionProgress() * file->GetFileSize();
    s_ChunkBar.FillRange(0, progressPos, crMagnet);
}
```

### Download Queue Integration
**DownloadQueue.cpp**:
```cpp
// Magnet link detection and status setting
if (link->IsFromMagnet()) {
    file->SetStatus(PS_CONVERTING_MAGNET);
    file->SetMagnetConversionProgress(0.0f);
}

// Progress update method  
void CDownloadQueue::UpdateMagnetConversionProgress(const CMD4Hash& fileHash, float progress);
```

## Thread Safety

### Synchronization
- `wxCriticalSection` protects all shared state
- GUI updates are thread-safe through queue mechanism
- Progress updates use atomic float operations

### Resource Management
- Automatic thread cleanup in destructor
- Proper exception handling in Entry() method
- Graceful abort support via `m_abort` flag

## Performance Considerations

### Memory Usage
- Minimal memory footprint (~1KB per conversion)
- No large buffers or complex data structures
- Stateless stage processing

### CPU Usage
- Lightweight progress calculations
- Infrequent GUI updates (0.5-2 second intervals)
- Background thread with low priority

## Extension Points

### Adding New Stages
1. Add to `ConversionStage` enum
2. Implement stage logic in Entry()
3. Update progress mapping
4. Add stage timing tracking

### Custom Progress Algorithms
Override `UpdateProgress()` for:
- Non-linear progress mapping
- Custom ETA calculations
- Alternative rate limiting

### Enhanced Logging
Extend `LogConversionStatistics()` for:
- Custom log formats
- Additional performance metrics
- External monitoring integration

## Testing

### Unit Tests
```cpp
// Test progress tracking
TEST(MagnetProgress, BasicProgress) {
    CMagnetProgressTracker tracker(...);
    tracker.UpdateProgress(0.5f, STAGE_DHT_LOOKUP);
    ASSERT_EQ(tracker.GetProgress(), 0.5f);
}

// Test time estimation  
TEST(MagnetProgress, TimeEstimation) {
    // Simulate 50% progress in 10 seconds
    // Expected ETA: 10 seconds remaining
}
```

### Integration Tests
- Magnet link parsing validation
- GUI progress bar rendering
- Error handling scenarios
- Concurrent conversion testing

## Debugging

### Logging
- Enable `logGeneral` for basic operations
- Use `logDebug` for detailed stage tracking
- Check `~/.amule_magnet_stats.log` for performance data

### Common Issues
- Thread synchronization problems
- Progress not updating (GUI issue)
- Conversion getting stuck (network issue)
- Memory leaks (resource cleanup)

## Best Practices

### Code Style
- Use consistent naming: `m_` prefix for members
- Document all public methods with Doxygen
- Follow aMule's error handling patterns

### Performance
- Minimize critical section duration
- Batch GUI updates when possible
- Use efficient data structures

### Maintenance
- Keep conversion stages well-documented
- Maintain backward compatibility
- Update tests when modifying logic

---
*This implementation provides a robust foundation for magnet conversion progress tracking with extensibility for future enhancements.*