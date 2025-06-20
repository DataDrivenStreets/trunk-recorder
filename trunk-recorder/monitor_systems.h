#ifndef MONITOR_SYSTEMS_H
#define MONITOR_SYSTEMS_H
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <ctime>

#include "./global_structs.h"
#include "call.h"
#include "config.h"
#include "source.h"
#include "systems/p25_parser.h"
#include "systems/p25_trunking.h"
#include "systems/smartnet_parser.h"
#include "systems/smartnet_trunking.h"
#include "systems/system.h"
#include <gnuradio/top_block.h>

int monitor_messages(Config &config, gr::top_block_sptr &tb, std::vector<Source *> &sources, std::vector<System *> &systems, std::vector<Call *> &calls);
void retune_system(System *sys, gr::top_block_sptr &tb, std::vector<Source *> &sources);

// Enhanced signal handling functions
void setup_signal_handlers();
void signal_handler(int sig);
void handle_config_reload();
void handle_status_report(std::vector<Source *> &sources, std::vector<System *> &systems, std::vector<Call *> &calls);
void handle_debug_toggle();
void exit_interupt(int sig);
void hup_handler(int sig);
void handle_log_rotation();
void print_status(std::vector<Source *> &sources, std::vector<System *> &systems, std::vector<Call *> &calls);
#endif