# Magnet Conversion Progress Tracking

## Overview
aMule now provides real-time progress tracking for magnet link conversions, giving users visual feedback during the magnet-to-ed2k conversion process.

## Features

### Visual Progress Indicators
- **Blue Progress Bars**: Magnet conversions show distinct blue progress bars
- **Real-time Updates**: Progress updates every 0.5-2 seconds during conversion
- **Percentage Display**: Clear progress percentage in the download list

### Conversion Stages
The conversion process goes through several stages:

1. **Parsing** (10%) - Analyzing the magnet URI structure
2. **Tracker Query** (30%) - Contacting torrent trackers for metadata  
3. **DHT Lookup** (50%) - Searching distributed hash table networks
4. **Metadata Fetch** (70%) - Retrieving file information from peers
5. **Conversion** (90%) - Creating ed2k link format
6. **Complete** (100%) - Ready for download

### Time Estimation
- **ETA Display**: Estimated time remaining for conversion completion
- **Conversion Rate**: Progress per second tracking
- **Stage Timing**: Individual stage duration measurements

## User Guide

### Adding Magnet Links
Magnet links can be added through:
- **File → Add Magnet Link** menu
- **Paste magnet URI** into main window
- **Drag and drop** magnet links onto aMule

### Monitoring Progress
- **Blue progress bar** indicates conversion status
- **Hover tooltips** show detailed stage information  
- **Progress percentage** updates in real-time
- **Error messages** appear for failed conversions

### Troubleshooting
**Common Issues:**
- Slow conversion: May indicate network connectivity issues
- Stuck at 0%: Check internet connection and firewall settings
- Conversion failed: Magnet URI may be invalid or unsupported

**Log Files:**
Conversion statistics are logged to:
- `~/.amule_magnet_stats.log` - Performance metrics
- Standard aMule logs - Debug information

## Technical Details

### Status Codes
- `PS_CONVERTING_MAGNET` (11) - Conversion in progress
- Normal download statuses apply after conversion

### Performance Metrics
The system tracks:
- Conversion duration per stage
- Overall conversion speed  
- Success/failure rates
- Resource usage during conversion

### Configuration
No user configuration required - works automatically with default settings.

## Support
For issues with magnet conversion:
1. Check network connectivity
2. Verify magnet URI format is supported
3. Check log files for detailed error information
4. Contact support if issues persist

---
*This feature enhances the user experience by providing clear feedback during magnet link processing.*