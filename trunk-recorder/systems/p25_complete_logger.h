#ifndef P25_COMPLETE_LOGGER_H
#define P25_COMPLETE_LOGGER_H

#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <boost/dynamic_bitset.hpp>
#include <boost/log/trivial.hpp>
// #include <json/json.h>  // Commented out - for future JSON support
// #include <csv-parser/csv.hpp>  // Commented out - for future CSV export

/**
 * P25 Complete Control Channel Logger
 * 
 * This class provides comprehensive logging of all P25 control channel messages
 * as defined in TIA-102.AABC-B-2005 standard. It extracts and logs every available
 * field from every message type, providing complete visibility into P25 control
 * channel activity.
 * 
 * Integration with existing trunk-recorder:
 * - Hooks into the existing parse_message() pipeline in P25Parser
 * - Uses the same bitset manipulation utilities as p25_parser.cc
 * - Maintains compatibility with existing TrunkMessage structure
 * - Adds comprehensive logging without disrupting current functionality
 */

// Extended message types covering all TIA-102 standard messages
enum P25MessageCategory {
    VOICE_SERVICE_ISP = 100,    // Section 4.1 - Inbound Signaling Packets
    VOICE_SERVICE_OSP = 200,    // Section 4.2 - Outbound Signaling Packets  
    DATA_SERVICE_ISP = 300,     // Section 6.1 - Data Service ISPs
    DATA_SERVICE_OSP = 400,     // Section 6.2 - Data Service OSPs
    SYSTEM_SERVICE = 500,       // System-level messages
    VENDOR_SPECIFIC = 600       // Vendor extensions (Motorola, Harris, etc.)
};

// Complete field structure for all TIA-102 message fields (Section 2.3)
struct P25MessageFields {
    // Header fields
    unsigned long opcode;
    unsigned long mfrid;
    bool last_block;
    bool protected_flag;
    
    // Service Options (Section 2.3.24)
    bool emergency;
    bool encrypted; 
    bool duplex;
    bool mode;
    unsigned int priority;
    
    // Addressing fields
    unsigned long source_id;        // Section 2.3.27 - Source Address
    unsigned long target_id;        // Section 2.3.33 - Target Address
    unsigned long group_address;    // Section 2.3.16 - Group Address
    unsigned long unit_address;     // Section 2.3.30 - Subscriber Unit Address
    unsigned long announcement_group; // Section 2.3.3
    
    // Channel information
    unsigned long tx_channel;       // Transmit channel
    unsigned long rx_channel;       // Receive channel
    double tx_frequency;
    double rx_frequency;
    unsigned int tdma_slot;
    bool phase2_tdma;
    double bandwidth;
    
    // System identification
    unsigned long wacn;             // Section 2.3.37 - WACN ID
    unsigned long system_id;        // Section 2.3.31 - System ID
    unsigned long rfss_id;          // Section 2.3.23 - RF Sub-System ID
    unsigned long site_id;          // Section 2.3.26 - Site ID
    unsigned long nac;              // Network Access Code
    unsigned long home_system;      // Section 2.3.18
    
    // Call management
    unsigned long call_timer;       // Section 2.3.8
    unsigned int answer_response;   // Section 2.3.4
    unsigned int reason_code;       // Section 2.3.22
    
    // Data fields
    unsigned long message_id;       // Section 2.3.21
    unsigned int data_format;
    unsigned long data_content;
    unsigned int service_type;      // Section 2.3.25
    unsigned long data_access_control; // Section 2.3.11
    
    // Authentication fields
    unsigned long authentication_value;
    unsigned long key_id;
    unsigned long algorithm_id;
    
    // Location fields
    unsigned long location_area;    // Section 2.3.20 - LRA
    
    // Frequency table fields  
    unsigned int identifier;        // Section 2.3.19
    unsigned long base_frequency;   // Section 2.3.5
    unsigned long channel_spacing;  // Section 2.3.10
    long transmit_offset;           // Section 2.3.35
    unsigned int bandwidth_value;   // Section 2.3.6
    
    // Extended fields
    unsigned long extended_function; // Section 2.3.15
    unsigned long additional_info;   // Section 2.3.1
    bool additional_info_valid;      // Section 2.3.2
    unsigned int digit_count;        // Section 2.3.14
    std::string digits;              // Section 2.3.13
    unsigned long tone_signal;       // Section 2.3.38
    
    // Patch fields
    unsigned long supergroup;
    unsigned long patch_group_1;
    unsigned long patch_group_2;
    unsigned long patch_group_3;
    
    // Vendor-specific fields
    unsigned long vendor_data_1;
    unsigned long vendor_data_2;
    unsigned long vendor_data_3;
    std::string vendor_info;
    
    // Raw data for debugging
    std::string raw_hex;
    std::string raw_binary;
    
    // Message metadata
    std::chrono::system_clock::time_point timestamp;
    P25MessageCategory category;
    std::string message_name;
    std::string description;
    int system_number;
};

class P25CompleteLogger {
private:
    std::ofstream log_file;
    std::string log_filename;
    bool logging_enabled;
    
    // Commented out for future implementation
    // std::ofstream json_file;
    // std::ofstream csv_file;
    // Json::Value json_root;
    // CSVWriter csv_writer;
    
    // Utility functions from p25_parser.cc
    unsigned long bitset_shift_mask(boost::dynamic_bitset<> &data, int shift, unsigned long long mask);
    unsigned long bitset_shift_left_mask(boost::dynamic_bitset<> &data, int shift, unsigned long long mask);
    
    // Field extraction functions for each message type
    void parse_voice_service_fields(boost::dynamic_bitset<> &data, P25MessageFields &fields);
    void parse_data_service_fields(boost::dynamic_bitset<> &data, P25MessageFields &fields);
    void parse_system_service_fields(boost::dynamic_bitset<> &data, P25MessageFields &fields);
    
    // Individual message parsers for complete TIA-102 coverage
    void parse_grp_v_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);           // 4.1.1
    void parse_uu_v_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);            // 4.1.2
    void parse_uu_ans_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields);         // 4.1.3
    void parse_tele_int_dial_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);  // 4.1.4
    void parse_tele_int_pstn_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);  // 4.1.5
    void parse_tele_int_ans_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields);   // 4.1.6
    
    void parse_grp_v_ch_grant(boost::dynamic_bitset<> &data, P25MessageFields &fields);     // 4.2.1
    void parse_grp_v_ch_grant_updt(boost::dynamic_bitset<> &data, P25MessageFields &fields); // 4.2.2
    void parse_grp_v_ch_grant_updt_exp(boost::dynamic_bitset<> &data, P25MessageFields &fields); // 4.2.3
    void parse_uu_ans_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);          // 4.2.4
    void parse_uu_v_ch_grant(boost::dynamic_bitset<> &data, P25MessageFields &fields);      // 4.2.5
    void parse_tele_int_ch_grant(boost::dynamic_bitset<> &data, P25MessageFields &fields);  // 4.2.6
    void parse_tele_int_ans_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);   // 4.2.7
    void parse_uu_v_ch_grant_updt(boost::dynamic_bitset<> &data, P25MessageFields &fields); // 4.2.8
    void parse_tele_int_ch_grant_updt(boost::dynamic_bitset<> &data, P25MessageFields &fields); // 4.2.9
    
    void parse_ack_rsp_u(boost::dynamic_bitset<> &data, P25MessageFields &fields);          // 6.1.1
    void parse_auth_q(boost::dynamic_bitset<> &data, P25MessageFields &fields);             // 6.1.2
    void parse_auth_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields);           // 6.1.3
    void parse_call_alrt_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);      // 6.1.4
    void parse_emrg_alrm_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);      // 6.1.6
    void parse_sts_q_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);          // 6.1.13
    void parse_sts_q_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields);          // 6.1.14
    void parse_sts_updt_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);       // 6.1.15
    void parse_loc_reg_req(boost::dynamic_bitset<> &data, P25MessageFields &fields);        // 6.1.18
    
    void parse_ack_rsp_fne(boost::dynamic_bitset<> &data, P25MessageFields &fields);        // 6.2.1
    void parse_adj_sts_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields);       // 6.2.2
    void parse_auth_cmd(boost::dynamic_bitset<> &data, P25MessageFields &fields);           // 6.2.3
    void parse_call_alrt(boost::dynamic_bitset<> &data, P25MessageFields &fields);          // 6.2.4
    void parse_que_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields);            // 6.2.14
    void parse_rfss_sts_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields);      // 6.2.15
    void parse_sts_q(boost::dynamic_bitset<> &data, P25MessageFields &fields);              // 6.2.17
    void parse_sts_updt(boost::dynamic_bitset<> &data, P25MessageFields &fields);           // 6.2.18
    void parse_sys_srv_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields);       // 6.2.19
    void parse_loc_reg_rsp(boost::dynamic_bitset<> &data, P25MessageFields &fields);        // 6.2.23
    
    // System service message parsers
    void parse_iden_up_tdma(boost::dynamic_bitset<> &data, P25MessageFields &fields);       // TSBK 0x33
    void parse_iden_up_vhf_uhf(boost::dynamic_bitset<> &data, P25MessageFields &fields);    // TSBK 0x34
    void parse_iden_up(boost::dynamic_bitset<> &data, P25MessageFields &fields);            // TSBK 0x3d
    void parse_net_sts_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields);       // TSBK 0x3b
    void parse_secondary_cc_bcst(boost::dynamic_bitset<> &data, P25MessageFields &fields);  // TSBK 0x29/0x39
    void parse_time_date_ann(boost::dynamic_bitset<> &data, P25MessageFields &fields);      // TSBK 0x35
    
    // Vendor-specific parsers
    void parse_motorola_patch_add(boost::dynamic_bitset<> &data, P25MessageFields &fields);
    void parse_motorola_patch_delete(boost::dynamic_bitset<> &data, P25MessageFields &fields);
    void parse_motorola_sys_loading(boost::dynamic_bitset<> &data, P25MessageFields &fields);
    void parse_harris_patch_add(boost::dynamic_bitset<> &data, P25MessageFields &fields);
    void parse_harris_patch_delete(boost::dynamic_bitset<> &data, P25MessageFields &fields);
    
    // Logging functions
    void log_message(const P25MessageFields &fields);
    std::string format_timestamp(const std::chrono::system_clock::time_point &timestamp);
    std::string fields_to_string(const P25MessageFields &fields);
    
    // Commented out for future implementation
    // void write_json_record(const P25MessageFields &fields);
    // void write_csv_record(const P25MessageFields &fields);
    // std::string fields_to_json(const P25MessageFields &fields);
    // std::vector<std::string> fields_to_csv_row(const P25MessageFields &fields);
    
    // Utility functions
    std::string get_message_name(unsigned long opcode, unsigned long mfrid);
    std::string get_message_description(unsigned long opcode, unsigned long mfrid);
    P25MessageCategory get_message_category(unsigned long opcode, unsigned long mfrid);
    std::string to_hex_string(const boost::dynamic_bitset<> &data);
    std::string to_binary_string(const boost::dynamic_bitset<> &data);

public:
    P25CompleteLogger(const std::string &log_file_path = "p25_control_complete.log");
    ~P25CompleteLogger();
    
    // Main logging interface - integrates with existing p25_parser.cc pipeline
    void log_tsbk_message(boost::dynamic_bitset<> &tsbk, unsigned long nac, int sys_num);
    void log_mbt_message(unsigned long opcode, boost::dynamic_bitset<> &header, 
                        boost::dynamic_bitset<> &mbt_data, unsigned long link_id, 
                        unsigned long nac, int sys_num);
    
    // Configuration
    void enable_logging(bool enable = true);
    void set_log_file(const std::string &filename);
    
    // Commented out for future implementation
    // void enable_json_output(const std::string &json_filename);
    // void enable_csv_output(const std::string &csv_filename);
    // void flush_json();
    // void flush_csv();
    
    bool is_logging_enabled() const { return logging_enabled; }
    std::string get_log_filename() const { return log_filename; }
};

#endif // P25_COMPLETE_LOGGER_H