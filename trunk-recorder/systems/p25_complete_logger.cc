#include "p25_complete_logger.h"
#include <sstream>
#include <iomanip>
#include <map>

P25CompleteLogger::P25CompleteLogger(const std::string &log_file_path) 
    : log_filename(log_file_path), logging_enabled(true) {
    
    log_file.open(log_filename, std::ios::app);
    if (!log_file.is_open()) {
        BOOST_LOG_TRIVIAL(error) << "P25CompleteLogger: Failed to open log file: " << log_filename;
        logging_enabled = false;
        return;
    }
    
    // Write header to log file
    log_file << "\n=== P25 Complete Control Channel Logger Started ===" << std::endl;
    log_file << "Timestamp,Message_Type,Opcode,MFRID,Description,All_Fields" << std::endl;
    log_file.flush();
    
    BOOST_LOG_TRIVIAL(info) << "P25CompleteLogger: Comprehensive logging enabled to " << log_filename;
    
    // Commented out for future JSON/CSV implementation
    /*
    // Initialize JSON output
    json_root = Json::Value(Json::arrayValue);
    
    // Initialize CSV headers
    csv_headers = {
        "timestamp", "message_type", "opcode", "mfrid", "source_id", "target_id",
        "group_address", "tx_channel", "rx_channel", "tx_frequency", "rx_frequency",
        "emergency", "encrypted", "duplex", "priority", "wacn", "system_id",
        "rfss_id", "site_id", "nac", "raw_hex", "description"
    };
    */
}

P25CompleteLogger::~P25CompleteLogger() {
    if (log_file.is_open()) {
        log_file << "\n=== P25 Complete Control Channel Logger Stopped ===" << std::endl;
        log_file.close();
    }
    
    // Commented out for future implementation
    /*
    flush_json();
    flush_csv();
    */
}

void P25CompleteLogger::enable_logging(bool enable) {
    logging_enabled = enable;
    BOOST_LOG_TRIVIAL(info) << "P25CompleteLogger: Logging " << (enable ? "enabled" : "disabled");
}

void P25CompleteLogger::set_log_file(const std::string &filename) {
    if (log_file.is_open()) {
        log_file.close();
    }
    
    log_filename = filename;
    log_file.open(log_filename, std::ios::app);
    if (!log_file.is_open()) {
        BOOST_LOG_TRIVIAL(error) << "P25CompleteLogger: Failed to open new log file: " << log_filename;
        logging_enabled = false;
    }
}

// Main TSBK message logging entry point - hooks into existing p25_parser.cc pipeline
void P25CompleteLogger::log_tsbk_message(boost::dynamic_bitset<> &tsbk, unsigned long nac, int sys_num) {
    if (!logging_enabled) return;
    
    P25MessageFields fields = {};
    fields.timestamp = std::chrono::system_clock::now();
    fields.nac = nac;
    fields.system_number = sys_num;
    fields.raw_hex = to_hex_string(tsbk);
    fields.raw_binary = to_binary_string(tsbk);
    
    // Extract opcode (same logic as p25_parser.cc)
    fields.opcode = bitset_shift_mask(tsbk, 88, 0x3f);
    fields.mfrid = bitset_shift_mask(tsbk, 80, 0xff);
    
    // Get message metadata
    fields.message_name = get_message_name(fields.opcode, fields.mfrid);
    fields.description = get_message_description(fields.opcode, fields.mfrid);
    fields.category = get_message_category(fields.opcode, fields.mfrid);
    
    // Parse based on opcode - comprehensive coverage of all TIA-102 messages
    switch (fields.opcode) {
        // Voice Service Messages (Section 4)
        case 0x00:  // Group Voice Channel Grant
            if (fields.mfrid == 0x90) {
                parse_motorola_patch_add(tsbk, fields);
            } else {
                parse_grp_v_ch_grant(tsbk, fields);
            }
            break;
            
        case 0x02:  // Group Voice Channel Grant Update
            if (fields.mfrid == 0x90) {
                parse_motorola_patch_add(tsbk, fields);  // Motorola patch variant
            } else {
                parse_grp_v_ch_grant_updt(tsbk, fields);
            }
            break;
            
        case 0x03:  // Group Voice Channel Grant Update - Explicit
            if (fields.mfrid == 0x90) {
                parse_motorola_patch_add(tsbk, fields);  // Motorola patch variant
            } else {
                parse_grp_v_ch_grant_updt_exp(tsbk, fields);
            }
            break;
            
        case 0x04:  // Unit-to-Unit Voice Service Channel Grant
            parse_uu_v_ch_grant(tsbk, fields);
            break;
            
        case 0x05:  // Unit-to-Unit Answer Request
            if (fields.mfrid == 0x90) {
                // Motorola Traffic Channel ID
                fields.vendor_info = "MOTOROLA_OSP_TRAFFIC_CHANNEL_ID";
                parse_system_service_fields(tsbk, fields);
            } else {
                parse_uu_ans_req(tsbk, fields);
            }
            break;
            
        case 0x06:  // Unit-to-Unit Voice Channel Grant Update
            parse_uu_v_ch_grant_updt(tsbk, fields);
            break;
            
        case 0x08:  // Telephone Interconnect Voice Channel Grant
            parse_tele_int_ch_grant(tsbk, fields);
            break;
            
        case 0x09:  // Telephone Interconnect Voice Channel Grant Update
            if (fields.mfrid == 0x90) {
                parse_motorola_sys_loading(tsbk, fields);
            } else {
                parse_tele_int_ch_grant_updt(tsbk, fields);
            }
            break;
            
        case 0x0A:  // Telephone Interconnect Answer Request
            parse_tele_int_ans_req(tsbk, fields);
            break;
            
        // Data Service Messages
        case 0x14:  // SNDCP Data Channel Grant
            parse_data_service_fields(tsbk, fields);
            fields.description = "SNDCP Data Channel Grant";
            break;
            
        case 0x15:  // SNDCP Data Page Request
            parse_data_service_fields(tsbk, fields);
            fields.description = "SNDCP Data Page Request";
            break;
            
        case 0x16:  // SNDCP Data Channel Announcement - Explicit
            parse_data_service_fields(tsbk, fields);
            fields.description = "SNDCP Data Channel Announcement - Explicit";
            break;
            
        case 0x18:  // Status Update (6.2.18)
            parse_sts_updt(tsbk, fields);
            break;
            
        case 0x1A:  // Status Query (6.2.17)
            parse_sts_q(tsbk, fields);
            break;
            
        case 0x1C:  // Message Update
            parse_data_service_fields(tsbk, fields);
            fields.description = "Message Update";
            break;
            
        case 0x1D:  // Radio Unit Monitor Command
            parse_data_service_fields(tsbk, fields);
            fields.description = "Radio Unit Monitor Command";
            break;
            
        case 0x1F:  // Call Alert (6.2.4)
            parse_call_alrt(tsbk, fields);
            break;
            
        case 0x20:  // Acknowledge Response (6.1.1/6.2.1)
            parse_ack_rsp_fne(tsbk, fields);
            break;
            
        case 0x21:  // Extended Function Command
            parse_data_service_fields(tsbk, fields);
            fields.description = "Extended Function Command";
            break;
            
        case 0x24:  // Extended Function Command (variant)
            parse_data_service_fields(tsbk, fields);
            fields.description = "Extended Function Command (variant)";
            break;
            
        case 0x27:  // Deny Response
            parse_data_service_fields(tsbk, fields);
            fields.description = "Deny Response";
            break;
            
        case 0x28:  // Unit Group Affiliation Response
            parse_data_service_fields(tsbk, fields);
            fields.description = "Unit Group Affiliation Response";
            break;
            
        case 0x29:  // Secondary Control Channel Broadcast - Explicit
            parse_secondary_cc_bcst(tsbk, fields);
            break;
            
        case 0x2A:  // Group Affiliation Query
            parse_data_service_fields(tsbk, fields);
            fields.description = "Group Affiliation Query";
            break;
            
        case 0x2B:  // Location Registration Response (6.2.23)
            parse_loc_reg_rsp(tsbk, fields);
            break;
            
        case 0x2C:  // Unit Registration Response
            parse_data_service_fields(tsbk, fields);
            fields.description = "Unit Registration Response";
            break;
            
        case 0x2D:  // Authentication Command (6.2.3)
            parse_auth_cmd(tsbk, fields);
            break;
            
        case 0x2E:  // De-Registration Acknowledge
            parse_data_service_fields(tsbk, fields);
            fields.description = "De-Registration Acknowledge";
            break;
            
        case 0x2F:  // Unit De-Registration Acknowledge
            parse_data_service_fields(tsbk, fields);
            fields.description = "Unit De-Registration Acknowledge";
            break;
            
        case 0x30:  // TDMA Synchronization Broadcast / M/A-COM Patch
            if (fields.mfrid == 0xA4) {
                parse_harris_patch_add(tsbk, fields);
            } else {
                parse_system_service_fields(tsbk, fields);
                fields.description = "TDMA Synchronization Broadcast";
            }
            break;
            
        case 0x31:  // Authentication Demand
            parse_auth_cmd(tsbk, fields);
            break;
            
        case 0x32:  // Authentication Response (6.1.3)
            parse_auth_rsp(tsbk, fields);
            break;
            
        case 0x33:  // Identifier Update - TDMA
            parse_iden_up_tdma(tsbk, fields);
            break;
            
        case 0x34:  // Identifier Update - VHF/UHF
            parse_iden_up_vhf_uhf(tsbk, fields);
            break;
            
        case 0x35:  // Time and Date Announcement
            parse_time_date_ann(tsbk, fields);
            break;
            
        case 0x36:  // Roaming Address Command
            parse_system_service_fields(tsbk, fields);
            fields.description = "Roaming Address Command";
            break;
            
        case 0x37:  // Roaming Address Update
            parse_system_service_fields(tsbk, fields);
            fields.description = "Roaming Address Update";
            break;
            
        case 0x38:  // System Service Broadcast (6.2.19)
            parse_sys_srv_bcst(tsbk, fields);
            break;
            
        case 0x39:  // Secondary Control Channel Broadcast (duplicate of 0x29)
            parse_secondary_cc_bcst(tsbk, fields);
            break;
            
        case 0x3A:  // RFSS Status Broadcast (6.2.15)
            parse_rfss_sts_bcst(tsbk, fields);
            break;
            
        case 0x3B:  // Network Status Broadcast
            parse_net_sts_bcst(tsbk, fields);
            break;
            
        case 0x3C:  // Adjacent Status Broadcast (6.2.2)
            parse_adj_sts_bcst(tsbk, fields);
            break;
            
        case 0x3D:  // Identifier Update
            parse_iden_up(tsbk, fields);
            break;
            
        default:
            // Unknown message type - still log with basic fields
            parse_system_service_fields(tsbk, fields);
            fields.description = "Unknown Message Type";
            break;
    }
    
    // Log the complete message
    log_message(fields);
    
    // Commented out for future implementation
    /*
    write_json_record(fields);
    write_csv_record(fields);
    */
}

// Main MBT message logging entry point
void P25CompleteLogger::log_mbt_message(unsigned long opcode, boost::dynamic_bitset<> &header, 
                                       boost::dynamic_bitset<> &mbt_data, unsigned long link_id, 
                                       unsigned long nac, int sys_num) {
    if (!logging_enabled) return;
    
    P25MessageFields fields = {};
    fields.timestamp = std::chrono::system_clock::now();
    fields.opcode = opcode;
    fields.nac = nac;
    fields.system_number = sys_num;
    fields.source_id = link_id;
    
    // Combine header and data for hex representation
    boost::dynamic_bitset<> combined_data = header;
    combined_data <<= mbt_data.size();
    combined_data |= mbt_data;
    
    fields.raw_hex = to_hex_string(combined_data);
    fields.raw_binary = to_binary_string(combined_data);
    fields.mfrid = bitset_shift_mask(header, 72, 0xff);
    
    fields.message_name = "MBT_" + get_message_name(opcode, fields.mfrid);
    fields.description = "Multi-Block Trunking: " + get_message_description(opcode, fields.mfrid);
    fields.category = get_message_category(opcode, fields.mfrid);
    
    // Parse MBT messages based on opcode
    switch (opcode) {
        case 0x0:  // Group Voice Channel Grant (MBT format)
            parse_grp_v_ch_grant(combined_data, fields);
            break;
            
        case 0x02: // Group Regroup Voice Channel Grant  
            if (fields.mfrid == 0x90) {
                parse_motorola_patch_add(combined_data, fields);
            } else {
                parse_grp_v_ch_grant_updt(combined_data, fields);
            }
            break;
            
        case 0x04: // Unit-to-Unit Voice Service Channel Grant - Extended
            parse_uu_v_ch_grant(combined_data, fields);
            break;
            
        case 0x28: // Group Affiliation Response (MBT format)
            parse_data_service_fields(combined_data, fields);
            fields.description = "MBT Group Affiliation Response";
            break;
            
        case 0x3A: // RFSS Status (MBT format)
            parse_rfss_sts_bcst(combined_data, fields);
            break;
            
        case 0x3B: // Network Status (MBT format)
            parse_net_sts_bcst(combined_data, fields);
            break;
            
        case 0x3C: // Adjacent Status (MBT format)
            parse_adj_sts_bcst(combined_data, fields);
            break;
            
        default:
            parse_system_service_fields(combined_data, fields);
            fields.description = "Unknown MBT Message";
            break;
    }
    
    log_message(fields);
    
    // Commented out for future implementation
    /*
    write_json_record(fields);
    write_csv_record(fields);
    */
}

// Utility function from p25_parser.cc
unsigned long P25CompleteLogger::bitset_shift_mask(boost::dynamic_bitset<> &data, int shift, unsigned long long mask) {
    boost::dynamic_bitset<> bitmask(data.size(), mask);
    return ((data >> shift) & bitmask).to_ulong();
}

unsigned long P25CompleteLogger::bitset_shift_left_mask(boost::dynamic_bitset<> &data, int shift, unsigned long long mask) {
    boost::dynamic_bitset<> bitmask(data.size(), mask);
    return ((data << shift) & bitmask).to_ulong();
}

// Voice Service Field Parsers
void P25CompleteLogger::parse_voice_service_fields(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // Extract common voice service fields (Section 4)
    fields.emergency = (bool)bitset_shift_mask(data, 72, 0x80);
    fields.encrypted = (bool)bitset_shift_mask(data, 72, 0x40);
    fields.duplex = (bool)bitset_shift_mask(data, 72, 0x20);
    fields.mode = (bool)bitset_shift_mask(data, 72, 0x10);
    fields.priority = bitset_shift_mask(data, 72, 0x07);
}

void P25CompleteLogger::parse_data_service_fields(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // Extract common data service fields (Section 6)  
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.target_id = bitset_shift_mask(data, 40, 0xffffff);
    fields.group_address = bitset_shift_mask(data, 40, 0xffff);
}

void P25CompleteLogger::parse_system_service_fields(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // Extract system-level fields
    fields.wacn = bitset_shift_mask(data, 52, 0xfffff);
    fields.system_id = bitset_shift_mask(data, 40, 0xfff);
    fields.rfss_id = bitset_shift_mask(data, 48, 0xff);
    fields.site_id = bitset_shift_mask(data, 40, 0xff);
}

// Specific message type parsers (implementing full TIA-102 standard)

void P25CompleteLogger::parse_grp_v_ch_grant(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.1 - Group Voice Channel Grant
    parse_voice_service_fields(data, fields);
    
    fields.tx_channel = bitset_shift_mask(data, 56, 0xffff);
    fields.group_address = bitset_shift_mask(data, 40, 0xffff);
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    
    // Additional fields specific to group voice channel grant
    fields.call_timer = bitset_shift_mask(data, 8, 0xff);
    fields.service_type = bitset_shift_mask(data, 0, 0xff);
    
    fields.message_name = "GRP_V_CH_GRANT";
    fields.description = "Group Voice Channel Grant";
    fields.category = VOICE_SERVICE_OSP;
}

void P25CompleteLogger::parse_grp_v_ch_grant_updt(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.2 - Group Voice Channel Grant Update
    fields.tx_channel = bitset_shift_mask(data, 64, 0xffff);
    fields.group_address = bitset_shift_mask(data, 48, 0xffff);
    fields.rx_channel = bitset_shift_mask(data, 32, 0xffff);
    
    fields.message_name = "GRP_V_CH_GRANT_UPDT";
    fields.description = "Group Voice Channel Grant Update";
    fields.category = VOICE_SERVICE_OSP;
}

void P25CompleteLogger::parse_grp_v_ch_grant_updt_exp(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.3 - Group Voice Channel Grant Update - Explicit
    parse_voice_service_fields(data, fields);
    
    fields.tx_channel = bitset_shift_mask(data, 48, 0xffff);
    fields.group_address = bitset_shift_mask(data, 16, 0xffff);
    
    fields.message_name = "GRP_V_CH_GRANT_UPDT_EXP";
    fields.description = "Group Voice Channel Grant Update - Explicit";
    fields.category = VOICE_SERVICE_OSP;
}

void P25CompleteLogger::parse_uu_v_ch_grant(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.5 - Unit-to-Unit Voice Service Channel Grant
    parse_voice_service_fields(data, fields);
    
    fields.tx_channel = bitset_shift_mask(data, 64, 0xffff);
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.target_id = bitset_shift_mask(data, 40, 0xffffff);
    
    fields.message_name = "UU_V_CH_GRANT";
    fields.description = "Unit-to-Unit Voice Service Channel Grant";
    fields.category = VOICE_SERVICE_OSP;
}

void P25CompleteLogger::parse_uu_ans_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.4 - Unit-to-Unit Answer Request
    parse_voice_service_fields(data, fields);
    
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.target_id = bitset_shift_mask(data, 40, 0xffffff);
    
    fields.message_name = "UU_ANS_REQ";
    fields.description = "Unit-to-Unit Answer Request";
    fields.category = VOICE_SERVICE_OSP;
}

void P25CompleteLogger::parse_uu_v_ch_grant_updt(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.8 - Unit-to-Unit Voice Channel Grant Update
    fields.tx_channel = bitset_shift_mask(data, 64, 0xffff);
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.target_id = bitset_shift_mask(data, 40, 0xffffff);
    
    fields.message_name = "UU_V_CH_GRANT_UPDT";
    fields.description = "Unit-to-Unit Voice Channel Grant Update";
    fields.category = VOICE_SERVICE_OSP;
}

void P25CompleteLogger::parse_tele_int_ch_grant(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.6 - Telephone Interconnect Voice Channel Grant
    parse_voice_service_fields(data, fields);
    
    fields.tx_channel = bitset_shift_mask(data, 64, 0xffff);
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.call_timer = bitset_shift_mask(data, 8, 0xff);
    
    fields.message_name = "TELE_INT_CH_GRANT";
    fields.description = "Telephone Interconnect Voice Channel Grant";
    fields.category = VOICE_SERVICE_OSP;
}

void P25CompleteLogger::parse_tele_int_ans_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.7 - Telephone Interconnect Answer Request
    parse_voice_service_fields(data, fields);
    
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.call_timer = bitset_shift_mask(data, 8, 0xff);
    
    fields.message_name = "TELE_INT_ANS_REQ";
    fields.description = "Telephone Interconnect Answer Request";
    fields.category = VOICE_SERVICE_OSP;
}

void P25CompleteLogger::parse_tele_int_ch_grant_updt(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 4.2.9 - Telephone Interconnect Channel Grant Update
    fields.tx_channel = bitset_shift_mask(data, 64, 0xffff);
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    
    fields.message_name = "TELE_INT_CH_GRANT_UPDT";
    fields.description = "Telephone Interconnect Channel Grant Update";
    fields.category = VOICE_SERVICE_OSP;
}

// Data Service Message Parsers

void P25CompleteLogger::parse_ack_rsp_fne(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.1 - Acknowledge Response - FNE
    fields.group_address = bitset_shift_mask(data, 40, 0xffff);
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.additional_info = bitset_shift_mask(data, 48, 0xff);
    
    fields.message_name = "ACK_RSP_FNE";
    fields.description = "Acknowledge Response - FNE";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_adj_sts_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.2 - Adjacent Status Broadcast
    fields.rfss_id = bitset_shift_mask(data, 48, 0xff);
    fields.site_id = bitset_shift_mask(data, 40, 0xff);
    fields.tx_channel = bitset_shift_mask(data, 24, 0xffff);
    
    fields.message_name = "ADJ_STS_BCST";
    fields.description = "Adjacent Status Broadcast";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_auth_cmd(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.3 - Authentication Command
    fields.target_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.authentication_value = bitset_shift_mask(data, 40, 0xffffff);
    fields.key_id = bitset_shift_mask(data, 64, 0xffff);
    fields.algorithm_id = bitset_shift_mask(data, 72, 0xff);
    
    fields.message_name = "AUTH_CMD";
    fields.description = "Authentication Command";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_call_alrt(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.4 - Call Alert
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.target_id = bitset_shift_mask(data, 40, 0xffffff);
    
    fields.message_name = "CALL_ALRT";
    fields.description = "Call Alert";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_rfss_sts_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.15 - RFSS Status Broadcast
    fields.system_id = bitset_shift_mask(data, 56, 0xfff);
    fields.rfss_id = bitset_shift_mask(data, 48, 0xff);
    fields.site_id = bitset_shift_mask(data, 40, 0xff);
    fields.tx_channel = bitset_shift_mask(data, 24, 0xffff);
    
    fields.message_name = "RFSS_STS_BCST";
    fields.description = "RFSS Status Broadcast";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_net_sts_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.16 - Network Status Broadcast
    fields.wacn = bitset_shift_mask(data, 52, 0xfffff);
    fields.system_id = bitset_shift_mask(data, 40, 0xfff);
    fields.tx_channel = bitset_shift_mask(data, 24, 0xffff);
    
    fields.message_name = "NET_STS_BCST";
    fields.description = "Network Status Broadcast";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_sts_q(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.17 - Status Query
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.target_id = bitset_shift_mask(data, 40, 0xffffff);
    
    fields.message_name = "STS_Q";
    fields.description = "Status Query";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_sts_updt(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.18 - Status Update
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.target_id = bitset_shift_mask(data, 40, 0xffffff);
    fields.additional_info = bitset_shift_mask(data, 64, 0xffff);
    
    fields.message_name = "STS_UPDT";
    fields.description = "Status Update";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_sys_srv_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.19 - System Service Broadcast
    fields.wacn = bitset_shift_mask(data, 52, 0xfffff);
    fields.system_id = bitset_shift_mask(data, 40, 0xfff);
    fields.service_type = bitset_shift_mask(data, 32, 0xff);
    
    fields.message_name = "SYS_SRV_BCST";
    fields.description = "System Service Broadcast";
    fields.category = DATA_SERVICE_OSP;
}

void P25CompleteLogger::parse_loc_reg_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TIA-102.AABC-B Section 6.2.23 - Location Registration Response
    fields.group_address = bitset_shift_mask(data, 56, 0xffff);
    fields.source_id = bitset_shift_mask(data, 16, 0xffffff);
    fields.location_area = bitset_shift_mask(data, 40, 0xffff);
    
    fields.message_name = "LOC_REG_RSP";
    fields.description = "Location Registration Response";
    fields.category = DATA_SERVICE_OSP;
}

// System Service Parsers

void P25CompleteLogger::parse_iden_up_tdma(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TSBK 0x33 - Identifier Update - TDMA
    fields.identifier = bitset_shift_mask(data, 76, 0xf);
    unsigned long channel_type = bitset_shift_mask(data, 72, 0xf);
    fields.transmit_offset = (long)bitset_shift_mask(data, 58, 0x3fff);
    fields.channel_spacing = bitset_shift_mask(data, 48, 0x3ff);
    fields.base_frequency = bitset_shift_mask(data, 16, 0xffffffff);
    
    // Process transmit offset sign
    unsigned long toff_sign = (fields.transmit_offset >> 13) & 1;
    fields.transmit_offset = fields.transmit_offset & 0x1fff;
    if (toff_sign == 0) {
        fields.transmit_offset = 0 - fields.transmit_offset;
    }
    
    fields.message_name = "IDEN_UP_TDMA";
    fields.description = "Identifier Update - TDMA";
    fields.category = SYSTEM_SERVICE;
}

void P25CompleteLogger::parse_iden_up_vhf_uhf(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TSBK 0x34 - Identifier Update - VHF/UHF
    fields.identifier = bitset_shift_mask(data, 76, 0xf);
    fields.bandwidth_value = bitset_shift_mask(data, 72, 0xf);
    fields.transmit_offset = (long)bitset_shift_mask(data, 58, 0x3fff);
    fields.channel_spacing = bitset_shift_mask(data, 48, 0x3ff);
    fields.base_frequency = bitset_shift_mask(data, 16, 0xffffffff);
    
    // Process transmit offset sign
    unsigned long toff_sign = (fields.transmit_offset >> 13) & 1;
    fields.transmit_offset = fields.transmit_offset & 0x1fff;
    if (toff_sign == 0) {
        fields.transmit_offset = 0 - fields.transmit_offset;
    }
    
    fields.message_name = "IDEN_UP_VHF_UHF";
    fields.description = "Identifier Update - VHF/UHF";
    fields.category = SYSTEM_SERVICE;
}

void P25CompleteLogger::parse_iden_up(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TSBK 0x3d - Identifier Update
    fields.identifier = bitset_shift_mask(data, 76, 0xf);
    fields.bandwidth_value = bitset_shift_mask(data, 67, 0x1ff);
    fields.transmit_offset = (long)bitset_shift_mask(data, 58, 0x1ff);
    fields.channel_spacing = bitset_shift_mask(data, 48, 0x3ff);
    fields.base_frequency = bitset_shift_mask(data, 16, 0xffffffff);
    
    // Process transmit offset sign
    unsigned long toff_sign = (fields.transmit_offset >> 8) & 1;
    fields.transmit_offset = fields.transmit_offset & 0xff;
    if (toff_sign == 0) {
        fields.transmit_offset = 0 - fields.transmit_offset;
    }
    
    fields.message_name = "IDEN_UP";
    fields.description = "Identifier Update";
    fields.category = SYSTEM_SERVICE;
}

void P25CompleteLogger::parse_secondary_cc_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TSBK 0x29/0x39 - Secondary Control Channel Broadcast
    fields.rfss_id = bitset_shift_mask(data, 72, 0xff);
    fields.site_id = bitset_shift_mask(data, 64, 0xff);
    fields.tx_channel = bitset_shift_mask(data, 48, 0xffff);
    fields.rx_channel = bitset_shift_mask(data, 24, 0xffff);
    
    fields.message_name = "SECONDARY_CC_BCST";
    fields.description = "Secondary Control Channel Broadcast";
    fields.category = SYSTEM_SERVICE;
}

void P25CompleteLogger::parse_time_date_ann(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // TSBK 0x35 - Time and Date Announcement
    // Extract time/date fields - this is a complex structure with multiple date/time components
    fields.additional_info = bitset_shift_mask(data, 16, 0xffffffff);  // Time/date data
    
    fields.message_name = "TIME_DATE_ANN";
    fields.description = "Time and Date Announcement";
    fields.category = SYSTEM_SERVICE;
}

// Vendor-Specific Parsers

void P25CompleteLogger::parse_motorola_patch_add(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // Motorola Group Regroup Add Command
    fields.supergroup = bitset_shift_mask(data, 64, 0xffff);
    fields.patch_group_1 = bitset_shift_mask(data, 48, 0xffff);
    fields.patch_group_2 = bitset_shift_mask(data, 32, 0xffff);
    fields.patch_group_3 = bitset_shift_mask(data, 16, 0xffff);
    
    fields.message_name = "MOT_PATCH_ADD";
    fields.description = "Motorola Patch Add Command";
    fields.category = VENDOR_SPECIFIC;
}

void P25CompleteLogger::parse_motorola_sys_loading(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // Motorola System Loading
    unsigned long mk = bitset_shift_mask(data, 76, 0xf);
    unsigned long ms = bitset_shift_mask(data, 70, 0xff);
    fields.vendor_data_1 = bitset_shift_mask(data, 64, 0xffff);
    
    fields.message_name = "MOT_SYS_LOADING";
    fields.description = "Motorola System Loading";
    fields.category = VENDOR_SPECIFIC;
    
    std::ostringstream oss;
    oss << "Scan Marker: " << mk << " " << ms << " microslots";
    fields.vendor_info = oss.str();
}

void P25CompleteLogger::parse_harris_patch_add(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    // M/A-COM (Harris) Group Request Patch
    unsigned long grg_g = bitset_shift_mask(data, 28, 0x1);
    unsigned long grg_a = bitset_shift_mask(data, 77, 0x01);
    fields.supergroup = bitset_shift_mask(data, 56, 0xffff);
    fields.patch_group_1 = bitset_shift_mask(data, 16, 0xffffff) & 0xffff;
    
    fields.message_name = "HARRIS_PATCH_ADD";
    fields.description = "Harris/M/A-COM Patch Add Command";
    fields.category = VENDOR_SPECIFIC;
    
    std::ostringstream oss;
    oss << "Group: " << (grg_g ? "Yes" : "No") << ", Active: " << (grg_a ? "Yes" : "No");
    fields.vendor_info = oss.str();
}

// Additional ISP message stubs (not typically seen on control channel but included for completeness)
void P25CompleteLogger::parse_grp_v_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "GRP_V_REQ";
    fields.description = "Group Voice Service Request";
    fields.category = VOICE_SERVICE_ISP;
}

void P25CompleteLogger::parse_uu_v_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "UU_V_REQ";
    fields.description = "Unit-to-Unit Voice Service Request";
    fields.category = VOICE_SERVICE_ISP;
}

void P25CompleteLogger::parse_uu_ans_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "UU_ANS_RSP";
    fields.description = "Unit-to-Unit Voice Service Answer Response";
    fields.category = VOICE_SERVICE_ISP;
}

void P25CompleteLogger::parse_tele_int_dial_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "TELE_INT_DIAL_REQ";
    fields.description = "Telephone Interconnect Request - Explicit Dialing";
    fields.category = VOICE_SERVICE_ISP;
}

void P25CompleteLogger::parse_tele_int_pstn_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "TELE_INT_PSTN_REQ";
    fields.description = "Telephone Interconnect Request - Implicit Dialing";
    fields.category = VOICE_SERVICE_ISP;
}

void P25CompleteLogger::parse_tele_int_ans_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "TELE_INT_ANS_RSP";
    fields.description = "Telephone Interconnect Answer Response";
    fields.category = VOICE_SERVICE_ISP;
}

void P25CompleteLogger::parse_ack_rsp_u(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "ACK_RSP_U";
    fields.description = "Acknowledge Response - Unit";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_auth_q(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "AUTH_Q";
    fields.description = "Authentication Query";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_auth_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "AUTH_RSP";
    fields.description = "Authentication Response";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_call_alrt_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "CALL_ALRT_REQ";
    fields.description = "Call Alert Request";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_emrg_alrm_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "EMRG_ALRM_REQ";
    fields.description = "Emergency Alarm Request";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_sts_q_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "STS_Q_REQ";
    fields.description = "Status Query Request";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_sts_q_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "STS_Q_RSP";
    fields.description = "Status Query Response";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_sts_updt_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "STS_UPDT_REQ";
    fields.description = "Status Update Request";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_loc_reg_req(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "LOC_REG_REQ";
    fields.description = "Location Registration Request";
    fields.category = DATA_SERVICE_ISP;
}

void P25CompleteLogger::parse_que_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields) {
    fields.message_name = "QUE_RSP";
    fields.description = "Queued Response";
    fields.category = DATA_SERVICE_OSP;
}

// Utility functions

std::string P25CompleteLogger::get_message_name(unsigned long opcode, unsigned long mfrid) {
    if (mfrid == 0x90) {
        switch (opcode) {
            case 0x00: return "MOT_GRG_ADD_CMD";
            case 0x02: return "MOT_GRG_CN_GRANT_EXP";
            case 0x03: return "MOT_GRG_CN_GRANT_UPDT";
            case 0x05: return "MOT_OSP_TRAFFIC_CHANNEL_ID";
            case 0x09: return "MOT_OSP_SYSTEM_LOADING";
            default: return "MOT_UNKNOWN_" + std::to_string(opcode);
        }
    } else if (mfrid == 0xA4) {
        return "HARRIS_GRG_EXENC_CMD";
    }
    
    // Standard TIA-102 message names
    static const std::map<unsigned long, std::string> message_names = {
        {0x00, "GRP_V_CH_GRANT"},
        {0x02, "GRP_V_CH_GRANT_UPDT"},
        {0x03, "GRP_V_CH_GRANT_UPDT_EXP"},
        {0x04, "UU_V_CH_GRANT"},
        {0x05, "UU_ANS_REQ"},
        {0x06, "UU_V_CH_GRANT_UPDT"},
        {0x08, "TELE_INT_CH_GRANT"},
        {0x09, "TELE_INT_CH_GRANT_UPDT"},
        {0x0A, "TELE_INT_ANS_REQ"},
        {0x14, "SNDCP_DATA_CH_GRANT"},
        {0x15, "SNDCP_DATA_PAGE_REQ"},
        {0x16, "SNDCP_DATA_CH_ANN_EXP"},
        {0x18, "STS_UPDT"},
        {0x1A, "STS_Q"},
        {0x1C, "MSG_UPDT"},
        {0x1D, "RADIO_UNIT_MON_CMD"},
        {0x1F, "CALL_ALRT"},
        {0x20, "ACK_RSP"},
        {0x21, "EXT_FUNC_CMD"},
        {0x24, "EXT_FUNC_CMD_EXT"},
        {0x27, "DENY_RSP"},
        {0x28, "GRP_AFF_RSP"},
        {0x29, "SECONDARY_CC_BCST"},
        {0x2A, "GRP_AFF_Q"},
        {0x2B, "LOC_REG_RSP"},
        {0x2C, "UNIT_REG_RSP"},
        {0x2D, "AUTH_CMD"},
        {0x2E, "DEREG_ACK"},
        {0x2F, "UNIT_DEREG_ACK"},
        {0x30, "TDMA_SYNC_BCST"},
        {0x31, "AUTH_DEMAND"},
        {0x32, "AUTH_RSP"},
        {0x33, "IDEN_UP_TDMA"},
        {0x34, "IDEN_UP_VHF_UHF"},
        {0x35, "TIME_DATE_ANN"},
        {0x36, "ROAMING_ADDR_CMD"},
        {0x37, "ROAMING_ADDR_UPDT"},
        {0x38, "SYS_SRV_BCST"},
        {0x39, "SECONDARY_CC_BCST_EXP"},
        {0x3A, "RFSS_STS_BCST"},
        {0x3B, "NET_STS_BCST"},
        {0x3C, "ADJ_STS_BCST"},
        {0x3D, "IDEN_UP"}
    };
    
    auto it = message_names.find(opcode);
    return (it != message_names.end()) ? it->second : "UNKNOWN_" + std::to_string(opcode);
}

std::string P25CompleteLogger::get_message_description(unsigned long opcode, unsigned long mfrid) {
    if (mfrid == 0x90) {
        switch (opcode) {
            case 0x00: return "Motorola Group Regroup Add Command";
            case 0x02: return "Motorola Group Regroup Channel Grant - Explicit";
            case 0x03: return "Motorola Group Regroup Channel Grant Update";
            case 0x05: return "Motorola OSP Traffic Channel ID";
            case 0x09: return "Motorola OSP System Loading";
            default: return "Unknown Motorola Message";
        }
    } else if (mfrid == 0xA4) {
        return "Harris/M/A-COM Group Regroup Extended Encryption Command";
    }
    
    // Standard TIA-102 descriptions would go here...
    return "Standard P25 Message";
}

P25MessageCategory P25CompleteLogger::get_message_category(unsigned long opcode, unsigned long mfrid) {
    if (mfrid == 0x90 || mfrid == 0xA4) {
        return VENDOR_SPECIFIC;
    }
    
    if (opcode >= 0x33 && opcode <= 0x3D) {
        return SYSTEM_SERVICE;
    } else if (opcode <= 0x0A) {
        return VOICE_SERVICE_OSP;
    } else {
        return DATA_SERVICE_OSP;
    }
}

std::string P25CompleteLogger::to_hex_string(const boost::dynamic_bitset<> &data) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    
    // Convert bitset to hex string
    for (size_t i = data.size(); i > 0; i -= 4) {
        unsigned long nibble = 0;
        for (size_t j = 0; j < 4 && (i - j - 1) < data.size(); ++j) {
            if (data[i - j - 1]) {
                nibble |= (1 << j);
            }
        }
        oss << std::hex << nibble;
    }
    
    return oss.str();
}

std::string P25CompleteLogger::to_binary_string(const boost::dynamic_bitset<> &data) {
    std::string result;
    result.reserve(data.size());
    
    for (size_t i = data.size(); i > 0; --i) {
        result += (data[i-1] ? '1' : '0');
    }
    
    return result;
}

std::string P25CompleteLogger::format_timestamp(const std::chrono::system_clock::time_point &timestamp) {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

std::string P25CompleteLogger::fields_to_string(const P25MessageFields &fields) {
    std::ostringstream oss;
    
    oss << "Opcode=0x" << std::hex << fields.opcode;
    oss << ",MFRID=0x" << std::hex << fields.mfrid;
    oss << ",NAC=0x" << std::hex << fields.nac;
    oss << ",SysNum=" << std::dec << fields.system_number;
    
    if (fields.source_id != 0) 
        oss << ",SrcID=" << fields.source_id;
    if (fields.target_id != 0) 
        oss << ",TgtID=" << fields.target_id;
    if (fields.group_address != 0) 
        oss << ",GrpAddr=" << fields.group_address;
    if (fields.tx_channel != 0) 
        oss << ",TxCh=0x" << std::hex << fields.tx_channel;
    if (fields.rx_channel != 0) 
        oss << ",RxCh=0x" << std::hex << fields.rx_channel;
    
    oss << ",Emrg=" << (fields.emergency ? "Y" : "N");
    oss << ",Enc=" << (fields.encrypted ? "Y" : "N");
    oss << ",Dup=" << (fields.duplex ? "Y" : "N");
    oss << ",Pri=" << fields.priority;
    
    if (fields.wacn != 0) 
        oss << ",WACN=0x" << std::hex << fields.wacn;
    if (fields.system_id != 0) 
        oss << ",SysID=0x" << std::hex << fields.system_id;
    if (fields.rfss_id != 0) 
        oss << ",RFSS=" << std::dec << fields.rfss_id;
    if (fields.site_id != 0) 
        oss << ",Site=" << std::dec << fields.site_id;
    
    if (fields.supergroup != 0) 
        oss << ",SG=" << fields.supergroup;
    if (fields.patch_group_1 != 0) 
        oss << ",PG1=" << fields.patch_group_1;
    if (fields.patch_group_2 != 0) 
        oss << ",PG2=" << fields.patch_group_2;
    if (fields.patch_group_3 != 0) 
        oss << ",PG3=" << fields.patch_group_3;
    
    if (!fields.vendor_info.empty()) 
        oss << ",VendorInfo=" << fields.vendor_info;
    
    oss << ",RawHex=" << fields.raw_hex;
    
    return oss.str();
}

void P25CompleteLogger::log_message(const P25MessageFields &fields) {
    if (!logging_enabled || !log_file.is_open()) return;
    
    log_file << format_timestamp(fields.timestamp) << ","
             << fields.message_name << ","
             << "0x" << std::hex << fields.opcode << ","
             << "0x" << std::hex << fields.mfrid << ","
             << fields.description << ","
             << fields_to_string(fields)
             << std::endl;
    
    log_file.flush();
}

// Commented out for future JSON implementation
/*
void P25CompleteLogger::write_json_record(const P25MessageFields &fields) {
    Json::Value record;
    record["timestamp"] = format_timestamp(fields.timestamp);
    record["message_name"] = fields.message_name;
    record["opcode"] = fields.opcode;
    record["mfrid"] = fields.mfrid;
    record["description"] = fields.description;
    record["source_id"] = fields.source_id;
    record["target_id"] = fields.target_id;
    record["group_address"] = fields.group_address;
    record["tx_channel"] = fields.tx_channel;
    record["rx_channel"] = fields.rx_channel;
    record["emergency"] = fields.emergency;
    record["encrypted"] = fields.encrypted;
    record["duplex"] = fields.duplex;
    record["priority"] = fields.priority;
    record["wacn"] = fields.wacn;
    record["system_id"] = fields.system_id;
    record["rfss_id"] = fields.rfss_id;
    record["site_id"] = fields.site_id;
    record["nac"] = fields.nac;
    record["raw_hex"] = fields.raw_hex;
    
    json_root.append(record);
}

void P25CompleteLogger::write_csv_record(const P25MessageFields &fields) {
    std::vector<std::string> row = {
        format_timestamp(fields.timestamp),
        fields.message_name,
        std::to_string(fields.opcode),
        std::to_string(fields.mfrid),
        std::to_string(fields.source_id),
        std::to_string(fields.target_id),
        std::to_string(fields.group_address),
        std::to_string(fields.tx_channel),
        std::to_string(fields.rx_channel),
        fields.emergency ? "1" : "0",
        fields.encrypted ? "1" : "0",
        fields.duplex ? "1" : "0",
        std::to_string(fields.priority),
        std::to_string(fields.wacn),
        std::to_string(fields.system_id),
        std::to_string(fields.rfss_id),
        std::to_string(fields.site_id),
        std::to_string(fields.nac),
        fields.raw_hex,
        fields.description
    };
    
    csv_writer.write_row(row);
}

void P25CompleteLogger::flush_json() {
    if (json_file.is_open()) {
        json_file << json_root << std::endl;
        json_file.close();
    }
}

void P25CompleteLogger::flush_csv() {
    if (csv_file.is_open()) {
        csv_file.close();
    }
}
*/