#!/bin/bash

# Trunk-Recorder Log Rotation Script
# This script safely rotates control channel CSV logs using HUP signal handling

set -euo pipefail

# Configuration
TRUNK_RECORDER_PID_FILE="/var/run/trunk-recorder.pid"
LOG_DIR="/var/log/trunk-recorder"
ARCHIVE_DIR="/var/log/trunk-recorder/archive"
RETENTION_DAYS=30

# Function to print usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    -p, --pid-file FILE     Path to trunk-recorder PID file (default: $TRUNK_RECORDER_PID_FILE)
    -d, --log-dir DIR       Directory containing log files (default: $LOG_DIR)
    -a, --archive-dir DIR   Directory to store archived logs (default: $ARCHIVE_DIR)
    -r, --retention DAYS    Days to retain old logs (default: $RETENTION_DAYS)
    -s, --system SYSTEM     Rotate logs for specific system only
    -h, --help              Show this help message

Examples:
    $0                                    # Rotate all logs
    $0 -s "MySystem"                      # Rotate logs for specific system
    $0 -d /custom/log/path                # Use custom log directory
    $0 -r 7                               # Keep logs for 7 days only

EOF
}

# Function to log messages
log_message() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1" >&2
}

# Function to get trunk-recorder PID
get_trunk_recorder_pid() {
    if [[ -f "$TRUNK_RECORDER_PID_FILE" ]]; then
        local pid=$(cat "$TRUNK_RECORDER_PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            echo "$pid"
            return 0
        else
            log_message "WARNING: PID file exists but process $pid is not running"
        fi
    fi
    
    # Try to find by process name
    local pid=$(pgrep -f "trunk-recorder" | head -1)
    if [[ -n "$pid" ]]; then
        log_message "Found trunk-recorder process: $pid"
        echo "$pid"
        return 0
    fi
    
    return 1
}

# Function to rotate logs for a specific system
rotate_system_logs() {
    local system_name="$1"
    local log_file="$LOG_DIR/control_channel_${system_name}.log"
    local timestamp=$(date '+%Y%m%d_%H%M%S')
    local archive_file="$ARCHIVE_DIR/control_channel_${system_name}_${timestamp}.log"
    
    if [[ ! -f "$log_file" ]]; then
        log_message "WARNING: Log file not found: $log_file"
        return 1
    fi
    
    # Create archive directory if it doesn't exist
    mkdir -p "$ARCHIVE_DIR"
    
    # Move the current log file to archive
    log_message "Rotating log: $log_file -> $archive_file"
    mv "$log_file" "$archive_file"
    
    # Compress the archived log
    if command -v gzip &> /dev/null; then
        log_message "Compressing archived log: $archive_file"
        gzip "$archive_file"
        archive_file="${archive_file}.gz"
    fi
    
    # Signal trunk-recorder to create new log file
    local pid=$(get_trunk_recorder_pid)
    if [[ -n "$pid" ]]; then
        log_message "Sending HUP signal to trunk-recorder (PID: $pid)"
        kill -HUP "$pid"
        
        # Wait a moment for new log file to be created
        sleep 2
        
        if [[ -f "$log_file" ]]; then
            log_message "SUCCESS: New log file created: $log_file"
        else
            log_message "WARNING: New log file not yet created: $log_file"
        fi
    else
        log_message "ERROR: Could not find trunk-recorder process"
        return 1
    fi
    
    return 0
}

# Function to find all system names from existing log files
find_systems() {
    find "$LOG_DIR" -name "control_channel_*.log" -type f | \
    sed 's/.*control_channel_\(.*\)\.log/\1/' | \
    sort -u
}

# Function to clean up old archived logs
cleanup_old_logs() {
    log_message "Cleaning up logs older than $RETENTION_DAYS days"
    
    if [[ -d "$ARCHIVE_DIR" ]]; then
        find "$ARCHIVE_DIR" -name "control_channel_*.log*" -type f -mtime +$RETENTION_DAYS -delete
        local deleted_count=$(find "$ARCHIVE_DIR" -name "control_channel_*.log*" -type f -mtime +$RETENTION_DAYS | wc -l)
        log_message "Cleaned up $deleted_count old log files"
    fi
}

# Function to verify trunk-recorder is running
verify_trunk_recorder() {
    local pid=$(get_trunk_recorder_pid)
    if [[ -z "$pid" ]]; then
        log_message "ERROR: trunk-recorder is not running"
        echo "Please start trunk-recorder before rotating logs."
        exit 1
    fi
    log_message "Found trunk-recorder running with PID: $pid"
}

# Parse command line arguments
SPECIFIC_SYSTEM=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--pid-file)
            TRUNK_RECORDER_PID_FILE="$2"
            shift 2
            ;;
        -d|--log-dir)
            LOG_DIR="$2"
            shift 2
            ;;
        -a|--archive-dir)
            ARCHIVE_DIR="$2"
            shift 2
            ;;
        -r|--retention)
            RETENTION_DAYS="$2"
            shift 2
            ;;
        -s|--system)
            SPECIFIC_SYSTEM="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            log_message "ERROR: Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

# Main execution
main() {
    log_message "Starting trunk-recorder log rotation"
    log_message "Log directory: $LOG_DIR"
    log_message "Archive directory: $ARCHIVE_DIR"
    log_message "Retention period: $RETENTION_DAYS days"
    
    # Verify trunk-recorder is running
    verify_trunk_recorder
    
    # Create directories if they don't exist
    mkdir -p "$LOG_DIR" "$ARCHIVE_DIR"
    
    # Rotate logs
    if [[ -n "$SPECIFIC_SYSTEM" ]]; then
        log_message "Rotating logs for system: $SPECIFIC_SYSTEM"
        rotate_system_logs "$SPECIFIC_SYSTEM"
    else
        log_message "Finding all systems to rotate..."
        local systems=($(find_systems))
        
        if [[ ${#systems[@]} -eq 0 ]]; then
            log_message "WARNING: No control channel log files found in $LOG_DIR"
            exit 0
        fi
        
        log_message "Found ${#systems[@]} system(s): ${systems[*]}"
        
        for system in "${systems[@]}"; do
            rotate_system_logs "$system"
        done
    fi
    
    # Clean up old logs
    cleanup_old_logs
    
    log_message "Log rotation completed successfully"
}

# Run main function
main "$@"