# ED2K Search Debug and Fix Guide

## Problem Analysis

From the logs, we can see:
1. Kad searches work correctly with detailed logging
2. ED2K searches have NO logging from ED2KSearchController
3. ED2K search packets are created via CreateSearchData (old code)
4. Controllers are registered via SearchResultRouter

This indicates that ED2KSearchController::startSearch is NOT being called, or it's failing silently.

## Solution

### Step 1: Add Logging to ED2KSearchController

The file `ED2KSearchController_Logging.patch` contains all the logging statements you need to add.

**Manual Steps:**

Open `/home/eli/git/amule/src/search/ED2KSearchController.cpp` and add these lines:

#### In executeSearch method (around line 111):

After line 112:
```cpp
AddDebugLogLineN(logSearch, wxT("ED2KSearchController: executeSearch called"));
```

After line 119:
```cpp
AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: oldSearchType=%d"), (int)oldSearchType));
```

After line 131:
```cpp
AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: isLocalSearch=%d"), isLocalSearch));
```

After line 127 (add new lines):
```cpp
bool supports64bit = theApp->serverconnect->GetCurrentServer() ?
    theApp->serverconnect->GetCurrentServer()->SupportsLargeFilesTCP() : false;
AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: supports64bit=%d"), supports64bit));
```

After line 140:
```cpp
AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: CreateSearchPacket success=%d, packetSize=%u"), success, packetSize));
```

After line 142:
```cpp
AddDebugLogLineN(logSearch, wxT("ED2KSearchController: Failed to create ED2K search packet"));
```

After line 144:
```cpp
AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: searchId=%u"), searchId));
```

After line 152:
```cpp
AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: Sending packet to server, isLocalSearch=%d"), isLocalSearch));
```

After line 164:
```cpp
AddDebugLogLineN(logSearch, wxT("ED2KSearchController: Not connected to eD2k server"));
```

After line 167:
```cpp
AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: Exception: %s"), e.c_str()));
```

After line 176:
```cpp
AddDebugLogLineN(logSearch, wxString::Format(wxT("ED2KSearchController: Search completed, searchId=%u"), searchId));
```

### Step 2: Rebuild

```bash
cd /home/eli/git/amule/build
make -j$(nproc)
```

### Step 3: Run and Test

Run aMule and perform an ED2K search (Local or Global). Check the logs for:
- "ED2KSearchController: startSearch called"
- "ED2KSearchController: executeSearch called"
- "ED2KSearchController: Sending packet to server"

### Step 4: Analyze Results

#### If you see "startSearch called" but NOT "executeSearch called":
- Check validatePrerequisites() - it might be failing
- Check validateSearchParams() - it might be failing

#### If you see "executeSearch called" but NOT "Sending packet to server":
- Check CreateSearchPacket - it might be failing
- Check if server is connected

#### If you see "Sending packet to server":
- The packet should be sent to the server
- Check if the server is responding

### Step 5: Check Server Connection

From your logs:
```
Connected to eMule Sunrise with LowID
WARNING: You have received Low-ID!
```

LowID means you're behind a firewall/router. This can affect:
- Receiving search results from other clients
- But should NOT prevent sending search requests to the server

### Expected Log Output

After adding logging, you should see something like:
```
ED2KSearchController: startSearch called
ED2KSearchController:   searchString='super'
ED2KSearchController:   strKeyword='super'
ED2KSearchController:   typeText='Any'
ED2KSearchController:   extension=''
ED2KSearchController:   minSize=0
ED2KSearchController:   maxSize=0
ED2KSearchController:   availability=0
ED2KSearchController:   searchType=0
ED2KSearchController: executeSearch called
ED2KSearchController: oldSearchType=0
ED2KSearchController: isLocalSearch=1
ED2KSearchController: supports64bit=1
ED2KSearchController: CreateSearchPacket success=1, packetSize=19
ED2KSearchController: searchId=1
ED2KSearchController: Sending packet to server, isLocalSearch=1
ED2KSearchController: Search completed, searchId=1
```

### Next Steps

1. Add the logging statements
2. Rebuild and test
3. Share the new log output
4. We'll analyze the results to find the exact issue

Don't give up! We're getting closer to finding the problem.
