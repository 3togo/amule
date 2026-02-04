# Kad Search Debug Instructions

## Problem
Clicking the start button in the search window is not returning any hits for Kad searches.

## Debug Steps

### 1. Replace KadSearchController.cpp with Debug Version

```bash
cd /home/eli/git/amule/src/search
cp KadSearchController_debug.cpp KadSearchController.cpp
```

### 2. Rebuild aMule

```bash
cd /home/eli/git/amule
make clean
make
```

### 3. Run aMule with Debug Logging Enabled

Make sure debug logging is enabled in your aMule configuration. You can enable it by:
- Setting the debug log level to include "Search" category
- Or using command line option: `amule --debug`

### 4. Perform a Kad Search

1. Start aMule
2. Connect to Kad network
3. Enter a search term (e.g., "test")
4. Click the "Start" button
5. Watch the debug logs

## What to Look For in Debug Logs

### Expected Flow

1. **KadSearchController Initialization**
   - `KadSearchController: Constructor called`
   - `KadSearchController::startSearch called with keyword='...'`

2. **Prerequisites Validation**
   - `KadSearchController: Kad network is running: yes`
   - If you see "Kad network is running: no", Kad is not connected

3. **Search Packet Creation**
   - `KadSearchController: Search packet created, size=XXX`
   - Size should be > 0

4. **Search ID Management**
   - `KadSearchController: Kad search manager returned search ID=XXX`
   - `KadSearchController: Registered controller for search ID=XXX`
   - `KadSearchController: Set current search ID=XXX in SearchList`
   - `KadSearchController: Notified search started for ID=XXX`

5. **Kad Search Manager Logs**
   - `Keyword for search: ...` (from SearchManager.cpp)
   - This shows the keyword being searched for

6. **Search Results Processing** (if results are found)
   - `Processing search response from ...` (from Search.cpp)
   - `Routed result for search ID XXX` (from SearchResultRouter.cpp)
   - If you see "No controller registered for search ID XXX", there's a registration issue

### Common Issues and Solutions

#### Issue 1: Kad Network Not Connected
**Symptoms:**
- `KadSearchController: Kad network is running: no`
- `KadSearchController: Kad network not available`

**Solution:**
- Wait for Kad to connect
- Check Kad status in aMule UI
- Ensure Kad is enabled in preferences

#### Issue 2: Search ID Mismatch
**Symptoms:**
- `No controller registered for search ID XXX, dropping result`
- Search ID in controller doesn't match search ID in results

**Solution:**
- This should be fixed by the current implementation
- If it still occurs, the search ID management needs further investigation

#### Issue 3: No Results Found
**Symptoms:**
- All logs show search started correctly
- No "Processing search response" messages
- No "Routed result" messages

**Possible Causes:**
1. Search term doesn't match any files on Kad network
   - Try a more common term like "test", "music", "video"
2. Kad network has no nodes or is not well-connected
   - Check Kad connection status
   - Wait for Kad to bootstrap properly
3. Search packet is malformed
   - Check `Search packet created, size=XXX` log
   - Size should be reasonable (not 0, not too large)

## Additional Debugging

If the issue persists, check:

1. **Kad Network Status**
   - Are you connected to Kad?
   - How many Kad contacts do you have?
   - Is your Kad ID valid?

2. **Search Term**
   - Try different search terms
   - Try simple, common words
   - Avoid special characters

3. **Network Traffic**
   - Use Wireshark or tcpdump to monitor Kad UDP traffic
   - Check if search requests are being sent
   - Check if responses are being received

4. **Compare with ED2K Search**
   - Does ED2K search work?
   - If yes, the issue is Kad-specific
   - If no, the issue might be in the search UI

## Reporting Results

When reporting the issue, include:

1. Complete debug log from search start to finish
2. Kad network status (connected/not connected, number of contacts)
3. Search term used
4. Any error messages in the log
5. Whether ED2K search works (if ED2K is enabled)

## Next Steps Based on Findings

### If Kad is not connected:
- Fix Kad connection issues first
- Ensure Kad is properly bootstrapped

### If search packets are not being sent:
- Check Kad network configuration
- Verify firewall settings
- Check UDP port forwarding

### If search packets are sent but no responses:
- Kad network might be sparse
- Try different search terms
- Wait longer for results

### If responses are received but not displayed:
- Check SearchResultRouter registration
- Check SearchControllerBase::handleResult
- Check UI update mechanism
