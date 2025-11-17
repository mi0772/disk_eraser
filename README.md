# disk_eraser

A simple command-line utility to securely overwrite hard drives on Unix-like systems, making data recovery infeasible.

## Platform Support

This tool is designed for Unix-like operating systems:
- **macOS** (tested on macOS 14+)
- **Linux** (tested on modern distributions)

**Note:** Windows is not supported. This is a tool for serious Unix systems, not for gaming consoles or office workstations.

## Overview

`disk_eraser` writes zeros to an entire disk device, effectively wiping all data. It includes safety checks to prevent accidental data loss, real-time progress monitoring, and proper signal handling.

## Features

- Double confirmation before operations
- System disk detection and protection
- Real-time progress display (percentage, speed, ETA)
- Raw device access for optimal performance
- Signal handling (CTRL+C) for clean interruption
- Operation logging with timestamps

## Requirements

- Unix-like system (macOS or Linux)
- Root privileges
- CMake 4.0+
- Standard C11 compiler

## Building

```bash
cmake -B build -S .
cmake --build build
```

The executable will be in `build/disk_eraser`.

## Usage

```bash
sudo ./disk_eraser
```

The program will:
1. List available disks
2. Prompt for disk selection
3. Display disk information
4. Request double confirmation
5. Unmount the disk
6. Overwrite the entire disk with zeros
7. Show final statistics

### Example Session

```
====================================
    Disk Erase Tool v1.0
====================================

Available disks:
  /dev/disk0 (internal, physical):
     0: GUID_partition_scheme  *500.3 GB   disk0
  /dev/disk2 (external, physical):
     0: GUID_partition_scheme  *2.0 TB     disk2

Enter disk to erase (e.g., disk2 or /dev/disk2): disk2

!!! WARNING !!!
================
This will permanently erase ALL data on:
  Device: /dev/disk2
  Size: 2.0 TB

This operation CANNOT be undone!
ALL data will be PERMANENTLY lost!

Type 'YES' to continue: YES

Type 'disk2' to confirm: disk2

Unmounting disk...
Opening raw device: /dev/rdisk2

Starting secure erase...

[========================================] 100.0% | 2.0 TB / 2.0 TB
Speed: 125.3 MB/s | Elapsed: 4h 32m | ETA: 0s

Operation completed successfully!
```

## Architecture

```
disk_eraser/
├── main.c          # Entry point and main flow
├── disk_ops.c/h    # Disk operations (list, verify, wipe)
├── progress.c/h    # Progress tracking and display
└── utils.c/h       # Utility functions (formatting, logging)
```

### Components

**disk_ops**: Low-level disk operations
- Device listing
- System disk detection
- Device unmounting
- Raw device access
- Write loop with error handling

**progress**: Progress tracking
- Completion percentage calculation
- Real-time write speed monitoring
- ETA estimation
- Final statistics

**utils**: Support functions
- Byte formatting (B, KB, MB, GB, TB)
- Time formatting
- User confirmation prompts
- File logging

## Safety Mechanisms

1. Root privilege verification
2. System disk protection
3. Double confirmation requirement
4. Device path validation
5. Clean signal handling (SIGINT, SIGTERM)
6. Comprehensive logging to `disk_erase.log`

## Technical Details

- **Buffer size**: 1 MB aligned to 4096 bytes
- **Write pattern**: Single pass of zeros (0x00)
- **Sync mode**: `O_SYNC` flag ensures data reaches disk
- **macOS**: Uses `/dev/rdiskX` for raw access, `diskutil` for listing
- **Linux**: Uses `/dev/sdX`, standard Unix tools for listing

## Platform Differences

### macOS
- Raw devices: `/dev/rdiskX`
- Disk listing: `diskutil list`
- Disk info: `diskutil info`
- Block device ioctl: `DKIOCGETBLOCKSIZE`, `DKIOCGETBLOCKCOUNT`

### Linux
- Raw devices: `/dev/sdX` (no separate raw device)
- Disk listing: Parse `/proc/partitions` or use `lsblk`
- Block device ioctl: `BLKGETSIZE64`

## Limitations

- Requires root privileges
- Single-pass overwrite (sufficient for modern drives)
- Cannot be used on mounted root filesystem

## Logging

All operations are logged to `disk_erase.log`:

```
[2025-11-17 10:00:00] Program started
[2025-11-17 10:00:15] Selected disk: /dev/disk2
[2025-11-17 10:00:20] User confirmed operation
[2025-11-17 10:00:25] Starting wipe operation - size: 2000398934016 bytes
[2025-11-17 14:32:10] Operation completed successfully
```

## Security Note

Single-pass zero overwriting is considered sufficient for modern hard drives and SSDs. Multiple-pass overwriting (Gutmann method, DoD 5220.22-M) is largely obsolete due to increased storage density and wear-leveling in SSDs.

## Warning

**This program permanently destroys data. Use with extreme caution.**

- Always verify the target disk twice
- Backup important data before use
- Test on a sacrificial USB drive first
- Do not interrupt during operation

## License

MIT License

## Disclaimer

This software is provided "as is", without warranty of any kind. The author is not responsible for data loss or damage caused by use of this program. Use at your own risk.
