# Build Notes

## Platform-Specific Information

### macOS

The code uses macOS-specific features:
- `/dev/rdiskX` for raw device access (faster than `/dev/diskX`)
- `diskutil` for disk listing and information
- `sys/disk.h` for ioctl operations (`DKIOCGETBLOCKSIZE`, `DKIOCGETBLOCKCOUNT`)

Build as usual:
```bash
cmake -B build -S .
cmake --build build
sudo ./build/disk_eraser
```

### Linux

The code adapts to Linux automatically using preprocessor directives:
- Uses `/dev/sdX` directly (no separate raw device needed)
- Uses `lsblk` for disk listing
- Uses `linux/fs.h` for ioctl operations (`BLKGETSIZE64`)
- Uses standard `umount` for unmounting

Build requirements:
```bash
# Install build essentials
sudo apt-get install build-essential cmake

# Build
cmake -B build -S .
cmake --build build

# Run (requires root)
sudo ./build/disk_eraser
```

## Tested Platforms

- **macOS 14+** (Darwin 25.1.0) - Full support
- **Linux** - Should work on modern distributions (Ubuntu 20.04+, Debian 11+, etc.)

## Platform Detection

The code automatically detects the platform at compile time using:
- `__APPLE__` for macOS
- `__linux__` for Linux

Different implementations are compiled based on the detected platform.

## Compatibility Notes

### Linux Considerations

1. **Device naming**: Linux uses `/dev/sdX` (SCSI/SATA), `/dev/nvmeXnY` (NVMe), etc.
2. **Permissions**: Requires root (`sudo`) to access block devices
3. **Dependencies**: Uses standard utilities (`lsblk`, `umount`, `mount`)
4. **System disk detection**: Checks if disk contains root partition

### macOS Considerations

1. **Device naming**: macOS uses `/dev/diskX` (buffered) and `/dev/rdiskX` (raw)
2. **Permissions**: Requires root (`sudo`) and may need SIP consideration
3. **Dependencies**: Uses `diskutil` which is standard on macOS
4. **System disk detection**: Uses `diskutil info` to check system disk status

## Cross-Compilation

The code is written in portable C11 with platform-specific sections isolated using preprocessor directives. It should be straightforward to cross-compile, though you'll need the appropriate target system headers.

## Future Platforms

To add support for other Unix-like systems (FreeBSD, OpenBSD, etc.), add appropriate `#elif` sections in `disk_ops.c` for:
- Device listing
- System disk detection
- Device unmounting
- Raw device path handling
- Block device size ioctl

## Known Issues

- **VLA Warning**: The progress bar uses a variable-length array which is folded to constant by the compiler. This is harmless and can be ignored.
