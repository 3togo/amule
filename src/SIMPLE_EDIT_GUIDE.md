# 简单手动编辑指南 - 添加ED2K搜索日志

## 文件位置
`/home/eli/git/amule/src/search/ED2KSearchController.cpp`

## 编辑步骤

### 1. 在executeSearch方法开头（第100行后）添加：
```cpp
    AddDebugLogLineN(logSearch, wxT("ED2KSearchController: executeSearch called"));
```

### 2. 在oldSearchType赋值后（第110行后）添加：
```cpp
    AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: oldSearchType=%d"), (int)oldSearchType));
```

### 3. 在isLocalSearch赋值后（第123行后）添加：
```cpp
	AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: isLocalSearch=%d"), isLocalSearch));
```

### 4. 在CreateSearchPacket调用后（第130行后）添加：
```cpp
	AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: CreateSearchPacket success=%d, packetSize=%u"), success, packetSize));
```

### 5. 在if (!success || !packetData)块内（第132行）添加：
```cpp
	    AddDebugLogLineN(logSearch, wxT("ED2KSearchController: Failed to create ED2K search packet"));
```

### 6. 在searchId赋值后（第150行后）添加：
```cpp
	AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: searchId=%u"), searchId));
```

### 7. 在SendPacket之前（第153行后）添加：
```cpp
	    AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: Sending packet to server, isLocalSearch=%d"), isLocalSearch));
```

### 8. 在else块内（第174行）添加：
```cpp
	    AddDebugLogLineN(logSearch, wxT("ED2KSearchController: Not connected to eD2k server"));
```

### 9. 在catch块内（第179行）添加：
```cpp
	AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: Exception: %s"), e.c_str()));
```

### 10. 在return之前（第193行）添加：
```cpp
    AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: Search completed, searchId=%u"), searchId));
```

## 编译和测试

```bash
cd /home/eli/git/amule/build
make -j$(nproc)
```

运行aMule，执行ED2K搜索，查看日志。

## 预期日志输出

```
ED2KSearchController: executeSearch called
ED2KSearchController: oldSearchType=0
ED2KSearchController: isLocalSearch=1
ED2KSearchController: CreateSearchPacket success=1, packetSize=19
ED2KSearchController: searchId=1
ED2KSearchController: Sending packet to server, isLocalSearch=1
ED2KSearchController: Search completed, searchId=1
```

## 故障排查

- ✅ 看到日志 = 方法被调用
- ❌ 没看到日志 = 方法未被调用，问题在别处
