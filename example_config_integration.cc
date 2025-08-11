// Example showing how to integrate P25 Complete Logger into trunk-recorder
// This demonstrates the integration points and configuration options

// In main.cc or system configuration code:

#include "trunk-recorder/systems/p25_parser.h"

void configure_p25_comprehensive_logging(P25Parser* parser, const std::string& config_value) {
    if (!config_value.empty() && config_value != "false") {
        if (config_value == "true") {
            // Use default log file name
            parser->enable_complete_logging();
            BOOST_LOG_TRIVIAL(info) << "P25 comprehensive logging enabled with default file";
        } else {
            // Use custom log file name from config
            parser->enable_complete_logging(config_value);
            BOOST_LOG_TRIVIAL(info) << "P25 comprehensive logging enabled: " << config_value;
        }
    }
}

// Example JSON configuration that could be added to system config:
/*
{
  "systems": [
    {
      "type": "p25",
      "shortName": "MySystem",
      "comprehensive_logging": "p25_detailed_log.txt",
      // ... other system config
    }
  ]
}
*/

// Configuration parsing example (in config.cc):
/*
void parse_p25_system_config(json system_config, P25Parser* parser) {
    // Existing configuration parsing...
    
    // Add comprehensive logging configuration
    if (system_config.contains("comprehensive_logging")) {
        std::string log_config = system_config["comprehensive_logging"];
        configure_p25_comprehensive_logging(parser, log_config);
    }
}
*/

// The integration hooks are automatically called whenever messages are parsed:
// 
// Integration Point 1: In P25Parser::decode_tsbk() 
//   - Every TSBK (Trunking System Block) message triggers complete_logger->log_tsbk_message()
//   - This captures ALL control channel messages including those not fully parsed by current code
//   - Extracts every available field according to TIA-102 standard
//
// Integration Point 2: In P25Parser::decode_mbt_data()
//   - Every MBT (Multi-Block Trunking) message triggers complete_logger->log_mbt_message() 
//   - Captures extended/complex messages with additional data blocks
//   - Provides complete field extraction for all MBT message variants
//
// The logging is:
// - Non-invasive: Does not change existing functionality
// - ENABLED BY DEFAULT: Automatically starts logging when P25Parser is created
// - Comprehensive: Logs every single control channel transmission
// - Detailed: Extracts every field defined in TIA-102 standard
// - Safe: Exception handling prevents crashes if logging fails

// Log Output Format Example:
/*
2024-01-15 14:30:15.123,GRP_V_CH_GRANT,0x00,0x00,Group Voice Channel Grant,Opcode=0x00,MFRID=0x00,NAC=0x293,SysNum=1,SrcID=12345,GrpAddr=54321,TxCh=0x1234,Emrg=N,Enc=Y,Dup=N,Pri=3,RawHex=00293ABC1234...

2024-01-15 14:30:16.456,RFSS_STS_BCST,0x3A,0x00,RFSS Status Broadcast,Opcode=0x3A,MFRID=0x00,NAC=0x293,SysNum=1,SysID=0x123,RFSS=1,Site=2,TxCh=0x5678,RawHex=3A123456...

2024-01-15 14:30:17.789,MOT_PATCH_ADD,0x00,0x90,Motorola Patch Add Command,Opcode=0x00,MFRID=0x90,NAC=0x293,SysNum=1,SG=1001,PG1=2001,PG2=2002,PG3=2003,VendorInfo=Motorola Group Regroup,RawHex=009012345...
*/