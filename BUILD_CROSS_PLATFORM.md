# aMule Cross-Platform Build Guide

## Supported Platforms
- ✅ Linux (GCC/Clang)
- ✅ Windows (MSVC/MinGW) 
- ✅ macOS (Clang)

## Platform-Specific Configuration

### Linux
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Windows
```bash
# MSVC
cmake -G "Visual Studio 17 2022" -A x64 ..
# or MinGW
cmake -G "MinGW Makefiles" ..
```

### macOS  
```bash
cmake -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 ..
make
```

## Dependency Management
All platform dependencies are automatically managed by CMake, supporting:
- vcpkg (Windows)
- Homebrew (macOS) 
- System package managers (Linux)

