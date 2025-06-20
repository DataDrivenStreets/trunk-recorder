# Trunk-Recorder Log Rotation

This implementation provides seamless CSV log rotation for trunk-recorder control channel logs using HUP signal handling.

## Features

- ✅ **Seamless rotation** - No message loss during rotation
- ✅ **Automatic compression** - Gzip compression for archived logs
- ✅ **Configurable retention** - Automatic cleanup of old logs
- ✅ **Multi-system support** - Handles multiple radio systems
- ✅ **Safe operation** - Verifies trunk-recorder is running before rotation

## Quick Start

### Manual Log Rotation

```bash
# Rotate all system logs
./rotate_logs.sh

# Rotate logs for specific system
./rotate_logs.sh -s "MySystemName"

# Custom configuration
./rotate_logs.sh -d /custom/log/path -r 7
```

### Automatic Rotation with Systemd

1. Copy files to system locations:
```bash
sudo cp rotate_logs.sh /opt/trunk-recorder/
sudo cp systemd/trunk-recorder-logrotate.* /etc/systemd/system/
```

2. Enable and start the timer:
```bash
sudo systemctl daemon-reload
sudo systemctl enable trunk-recorder-logrotate.timer
sudo systemctl start trunk-recorder-logrotate.timer
```

3. Check status:
```bash
sudo systemctl status trunk-recorder-logrotate.timer
sudo systemctl list-timers | grep trunk-recorder
```

### Automatic Rotation with Cron

1. Copy the script:
```bash
sudo cp rotate_logs.sh /opt/trunk-recorder/
```

2. Add to crontab:
```bash
sudo crontab -e
# Add line for daily rotation at 2:30 AM:
30 2 * * * /opt/trunk-recorder/rotate_logs.sh >> /var/log/trunk-recorder-rotate.log 2>&1
```

## Script Options

| Option | Description | Default |
|--------|-------------|---------|
| `-p, --pid-file` | Path to trunk-recorder PID file | `/var/run/trunk-recorder.pid` |
| `-d, --log-dir` | Directory containing log files | `/var/log/trunk-recorder` |
| `-a, --archive-dir` | Directory for archived logs | `/var/log/trunk-recorder/archive` |
| `-r, --retention` | Days to retain old logs | `30` |
| `-s, --system` | Rotate specific system only | (all systems) |
| `-h, --help` | Show help message | |

## How It Works

1. **Move active log**: `mv control_channel_SystemName.log control_channel_SystemName_timestamp.log`
2. **Signal trunk-recorder**: `kill -HUP <pid>` triggers log file reopening
3. **Archive and compress**: Move to archive directory and gzip compress
4. **Cleanup old logs**: Remove files older than retention period

## Log File Naming

- **Active logs**: `control_channel_SystemName.log`
- **Archived logs**: `control_channel_SystemName_YYYYMMDD_HHMMSS.log.gz`

## Directory Structure

```
/var/log/trunk-recorder/
├── control_channel_System1.log          # Active log
├── control_channel_System2.log          # Active log
└── archive/
    ├── control_channel_System1_20240620_143000.log.gz
    ├── control_channel_System1_20240619_143000.log.gz
    └── control_channel_System2_20240620_143000.log.gz
```

## CSV Format

The control channel logs use the following CSV format:

```csv
# Control Channel Event Log for System: SystemName
# Format: timestamp,message_type,talkgroup,source,frequency,emergency,encrypted,priority,channel_id,tdma_slot,bandwidth,system_id,wacn,nac,rfss,site_id,opcode,description
2024-06-20 14:30:15,GRANT,12345,67890,851012500,0,0,3,1234,0,12.5,0x1,0xBEE00,0x293,1,1,,"Radio 67890 granted talkgroup 12345 on 851.0125 MHz (CH:1234 BW:12.5kHz)"
```

## Troubleshooting

### Check if trunk-recorder is running:
```bash
pgrep -f trunk-recorder
```

### Verify HUP signal handling:
```bash
# Send HUP signal manually
kill -HUP $(pgrep -f trunk-recorder)

# Check if new log file was created
ls -la /var/log/trunk-recorder/control_channel_*.log
```

### Debug script execution:
```bash
# Run script with verbose output
bash -x ./rotate_logs.sh

# Check systemd logs
sudo journalctl -u trunk-recorder-logrotate.service -f
```

### Common Issues

1. **Permission denied**: Ensure script has execute permissions
2. **PID not found**: Verify trunk-recorder is running and PID file location
3. **Log files not found**: Check log directory path configuration
4. **New log not created**: Verify HUP signal handler is working in trunk-recorder

## Configuration Examples

### High-frequency rotation (every hour):
```bash
# Cron entry
0 * * * * /opt/trunk-recorder/rotate_logs.sh
```

### Custom retention (keep logs for 7 days):
```bash
./rotate_logs.sh -r 7
```

### Multiple systems with different schedules:
```bash
# System1: Rotate every 4 hours
0 */4 * * * /opt/trunk-recorder/rotate_logs.sh -s "System1"

# System2: Rotate daily
0 2 * * * /opt/trunk-recorder/rotate_logs.sh -s "System2"
```

## Integration with Log Processing

After rotation, process the archived logs:

```bash
#!/bin/bash
# Process archived logs
for log_file in /var/log/trunk-recorder/archive/*.log.gz; do
    if [[ -f "$log_file" && ! -f "${log_file}.processed" ]]; then
        # Decompress and process
        zcat "$log_file" | your_log_processor.py
        
        # Mark as processed
        touch "${log_file}.processed"
    fi
done
```