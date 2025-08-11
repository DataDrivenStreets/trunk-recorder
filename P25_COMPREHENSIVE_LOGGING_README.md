# P25 Comprehensive Control Channel Logging

## Overview

This repository now includes comprehensive P25 control channel logging that is **ENABLED BY DEFAULT**. Every P25 control channel transmission is automatically logged with complete field extraction according to the TIA-102.AABC-B-2005 standard.

## Default Behavior

✅ **Automatic Logging**: When trunk-recorder starts with P25 systems, comprehensive logging begins immediately
✅ **Default Log File**: `p25_control_complete.log` (created in the working directory)
✅ **Complete Coverage**: Every control channel message is logged with all available fields
✅ **Zero Configuration**: No setup required - works out of the box

## What Gets Logged

### Every Message Type Including:
- **Voice Service Messages**: Group/Unit-to-Unit channel grants, updates, requests
- **Data Service Messages**: Status updates, authentication, alerts, registrations  
- **System Service Messages**: RFSS status, network status, frequency updates
- **Vendor Extensions**: Motorola patch commands, Harris/M/A-COM extensions
- **Raw Data**: Complete hex and binary representation for forensic analysis

### Complete Field Extraction:
- Source/Target/Group addresses
- Channel assignments and frequencies
- Emergency/Encrypted/Duplex flags
- System identifiers (WACN, System ID, RFSS, Site)
- Authentication and security parameters
- Patch group configurations
- Timing and location data
- Vendor-specific information

## Log File Format

```
Timestamp,Message_Type,Opcode,MFRID,Description,All_Fields
2024-01-15 14:30:15.123,GRP_V_CH_GRANT,0x00,0x00,Group Voice Channel Grant,Opcode=0x00,MFRID=0x00,NAC=0x293,SysNum=1,SrcID=12345,GrpAddr=54321,TxCh=0x1234,Emrg=N,Enc=Y,Dup=N,Pri=3,RawHex=00293ABC1234...
```

## Control Options

### Disable Logging (if needed):
```cpp
P25Parser* parser = /* your parser instance */;
parser->disable_complete_logging();
```

### Change Log File:
```cpp
parser->enable_complete_logging("my_custom_p25_log.txt");
```

### Check Status:
```cpp
// Logging is enabled by default, but you can check:
if (parser->complete_logger && parser->complete_logger->is_logging_enabled()) {
    // Logging is active
}
```

## File Locations

The comprehensive logging system consists of:

### Core Implementation:
- `trunk-recorder/systems/p25_complete_logger.h`
- `trunk-recorder/systems/p25_complete_logger.cc`

### Integration Points:
- `trunk-recorder/systems/p25_parser.h` (modified)
- `trunk-recorder/systems/p25_parser.cc` (modified)
- `CMakeLists.txt` (modified)

## Technical Details

### Integration Architecture:
The comprehensive logger hooks into the existing message parsing pipeline at two critical points:

1. **TSBK Messages** (`decode_tsbk()`): Captures single-block trunking messages
2. **MBT Messages** (`decode_mbt_data()`): Captures multi-block trunking messages

### Performance Impact:
- **Minimal**: Field extraction uses existing bitset operations
- **Safe**: Exception handling prevents crashes
- **Non-invasive**: Does not affect existing trunk-recorder functionality

### Standards Compliance:
- **Complete TIA-102.AABC-B Coverage**: All 60+ message types
- **Vendor Extensions**: Motorola, Harris/M/A-COM proprietary messages
- **Field Completeness**: Every field defined in Section 2.3 of the standard

## Viewing Logs in Real-Time

```bash
# Watch comprehensive logs
tail -f p25_control_complete.log

# Filter for specific message types
grep "GRP_V_CH_GRANT" p25_control_complete.log

# View emergency traffic only
grep "Emrg=Y" p25_control_complete.log

# Monitor encrypted communications
grep "Enc=Y" p25_control_complete.log
```

## Benefits

1. **Complete Visibility**: See every control channel transmission, not just the subset parsed by the original parser
2. **Forensic Analysis**: Complete raw data capture for detailed investigation
3. **System Understanding**: Full insight into P25 system behavior and configuration
4. **Troubleshooting**: Detailed logs help diagnose system issues
5. **Research**: Complete dataset for P25 protocol analysis
6. **Compliance**: Full audit trail of all control channel activity

## Future Enhancements

The logging system includes commented-out code for:
- **JSON Export**: Structured data output for automated processing
- **CSV Export**: Spreadsheet-compatible format
- **Database Integration**: Direct logging to SQL databases

These features can be enabled by uncommenting the relevant code sections and adding the required dependencies.

---

**Note**: This comprehensive logging captures significantly more data than the original p25_parser.cc. Log files will be larger but provide complete visibility into P25 control channel activity.