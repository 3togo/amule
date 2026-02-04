# Search Window Debug Logging

## Overview

The search window debug logging system provides a centralized way to control debug logging for all search-related components.

## Quick Start

### Disabling All Search Debug Logging

To disable all search window debug logging, you have three options:

#### Option 1: CMake Configuration (Recommended)

Use the CMake option when configuring your build:

```bash
# Disable search window debug logging
cmake -DENABLE_SEARCH_WINDOW_DEBUG=OFF ..

# Enable search window debug logging (default)
cmake -DENABLE_SEARCH_WINDOW_DEBUG=ON ..
```

Or use `ccmake` or `cmake-gui` to toggle the option interactively.

#### Option 2: Compile-time Control (Manual override)

Edit `SearchLogging.h` and change the following line:

```cpp
#define ENABLE_SEARCH_WINDOW_DEBUG false
```

Then recompile the project.

#### Option 3: Runtime Control

Use the existing debug category system in aMule:
- Disable the "Searching" debug category in the Preferences/Debug settings
- Or programmatically: `theLogger.SetEnabled(logSearch, false);`

## Usage in Code

### Basic Debug Logging

```cpp
#include "SearchLogging.h"

// This will only log if ENABLE_SEARCH_WINDOW_DEBUG is true
SEARCH_DEBUG(wxT("Search started with parameters: ") + params);
```

### Critical Messages

```cpp
// Critical messages are always logged, regardless of ENABLE_SEARCH_WINDOW_DEBUG
SEARCH_CRITICAL(wxT("Search failed: ") + error);
```

## How It Works

1. **Compile-time Control**: The `ENABLE_SEARCH_WINDOW_DEBUG` macro acts as a master switch
   - When `false`: All SEARCH_DEBUG calls are completely compiled out (zero overhead)
   - When `true`: Debug messages are logged only if the logSearch category is enabled

2. **Runtime Control**: The existing `logSearch` debug category provides runtime control
   - Can be toggled through Preferences/Debug settings
   - Can be controlled programmatically via `theLogger.SetEnabled(logSearch, bool)`

## Benefits

- **Zero Overhead When Disabled**: When `ENABLE_SEARCH_WINDOW_DEBUG` is false, all debug code is compiled out
- **Easy to Re-enable**: Simply change the macro definition and recompile
- **Backward Compatible**: Works with the existing logging infrastructure
- **Centralized Control**: Single point of control for all search window debug logging

## Example Scenarios

### Scenario 1: Temporarily Disable All Search Debug Logging

1. Edit `SearchLogging.h`
2. Change `#define ENABLE_SEARCH_WINDOW_DEBUG true` to `false`
3. Recompile
4. Focus on other aspects of development
5. When needed, change back to `true` and recompile

### Scenario 2: Enable Search Debug Logging for Testing

1. Ensure `ENABLE_SEARCH_WINDOW_DEBUG` is `true` in `SearchLogging.h`
2. Enable "Searching" debug category in Preferences/Debug settings
3. Run the application and perform search operations
4. Review debug logs in the log window or file
5. Disable "Searching" debug category when done

## Migration Guide

To migrate existing search window debug logging:

**Old way:**
```cpp
theLogger.AddLogLine(wxT(__FILE__), __LINE__, false, logSearch, wxT("Search started"));
```

**New way:**
```cpp
SEARCH_DEBUG(wxT("Search started"));
```

For critical messages that should always be logged:
```cpp
SEARCH_CRITICAL(wxT("Search failed: ") + error);
```
