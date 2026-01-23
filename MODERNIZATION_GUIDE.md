# aMule Modernization Guide

This document describes the modern C++20 features and patterns adopted in aMule.

## 🚀 C++20 Features Enabled

### Compiler Configuration
```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Coroutine support for compatible compilers
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fcoroutines)
    add_compile_definitions(HAS_COROUTINES)
endif()
```

## 📝 Modern Logging System

### New Header: `src/common/ModernLogging.h`
```cpp
namespace modern_log {
    #ifdef USE_CPP20
    void Log(std::string_view msg, 
             bool critical = false,
             std::source_location loc = std::source_location::current());
    #else
    void Log(const wxString& msg, bool critical = false);
    #endif
}
```

### Usage Examples

**Basic Usage:**
```cpp
// Traditional compatibility
modern_log::Log("Simple message");
modern_log::Log("Critical error", true);

// Modern C++20 (when available)
modern_log::Log(std::string_view("Efficient message"));
modern_log::Log("Message with auto location", false);
```

**Performance Critical:**
```cpp
// Avoids string copies
constexpr std::string_view perf_msg = "High performance";
modern_log::Log(perf_msg);
```

## 🌐 GeoIP Auto-Migration

### Automatic URL Updates
aMule now automatically detects and migrates obsolete GeoIP URLs:

```cpp
// Before: (obsolete)
http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/GeoIP.dat.gz

// After: (auto-migrated to)
https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb
```

### Configuration Persistence
URL migrations are automatically saved to `amule.conf` for permanent updates.

## 🔧 Best Practices

### 1. Prefer Modern Interfaces
```cpp
// Good: Modern C++20
modern_log::Log(std::string_view_data);

// Okay: Traditional compatibility  
modern_log::Log(wxString("Legacy code"));
```

### 2. Use Source Location (C++20)
```cpp
// Automatic file/line information
modern_log::Log("Debug message", false);
// Logs: filename.cpp(line): Debug message
```

### 3. Coroutine Readiness
Network I/O code is being prepared for C++20 coroutines:
- Async operations with `co_await`
- Non-blocking network calls
- Improved scalability

## 📊 Validation

### Testing
```bash
# Run all tests to verify compatibility
cd build && ctest --output-on-failure

# Build with modern features
make -j$(nproc) amule
```

### Compatibility Matrix
| Feature | C++20 Compilers | Legacy Compilers |
|---------|----------------|------------------|
| `std::string_view` | ✅ Full support | ⚠️ Fallback to `wxString` |
| `std::source_location` | ✅ Automatic | ❌ No location info |
| Coroutines | ✅ Enabled | ❌ Disabled |

## 🚀 Migration Path

### Immediate Actions
1. Use `modern_log::Log()` for new code
2. Test with both C++20 and legacy compilers
3. Verify GeoIP auto-migration works

### Future Enhancements
1. Gradual coroutine adoption in network layer
2. More `std::filesystem` integration
3. Enhanced compile-time checking

## 📝 License & Attribution
Modernization efforts maintain full compatibility with aMule's GPL v2 license.
GeoLite2 data provided via jsDelivr CDN for reliability.
```