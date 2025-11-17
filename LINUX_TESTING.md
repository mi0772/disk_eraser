# Linux Testing Guide

## Testing on Linux

To test this program on Linux, you can use a USB drive or create a virtual disk image.

### Option 1: USB Drive Testing

1. Insert a USB drive (make sure it contains no important data!)
2. Identify the device:
   ```bash
   lsblk -d
   # Look for your USB drive, e.g., /dev/sdb
   ```
3. Build and run:
   ```bash
   cmake -B build -S .
   cmake --build build
   sudo ./build/disk_eraser
   ```
4. When prompted, enter the disk name (e.g., `sdb` or `/dev/sdb`)

### Option 2: Virtual Disk Testing (Safe)

Create a virtual disk file for testing without risking real hardware:

```bash
# Create a 100MB test disk image
dd if=/dev/zero of=test_disk.img bs=1M count=100

# Create a loop device
sudo losetup -f test_disk.img

# Find which loop device was created
losetup -a
# Example output: /dev/loop0: []: (/path/to/test_disk.img)

# Run disk_eraser on the loop device
sudo ./build/disk_eraser
# Enter: loop0 or /dev/loop0

# When done, detach the loop device
sudo losetup -d /dev/loop0

# Cleanup
rm test_disk.img
```

## Expected Behavior on Linux

### Disk Listing
```
Available disks:
  NAME   SIZE TYPE MOUNTPOINT
  sda  931.5G disk
  sdb    7.5G disk /mnt/usb
  nvme0n1 238.5G disk
```

### System Disk Detection

The program will prevent erasing:
- Any disk containing the root partition (`/`)
- Any disk with partitions mounted as root

### Device Names

Linux uses different naming schemes:
- **SATA/SCSI**: `/dev/sda`, `/dev/sdb`, etc.
- **NVMe**: `/dev/nvme0n1`, `/dev/nvme1n1`, etc.
- **MMC/SD**: `/dev/mmcblk0`, `/dev/mmcblk1`, etc.
- **Loop devices**: `/dev/loop0`, `/dev/loop1`, etc.

All formats should work with the program.

## Troubleshooting

### Permission Denied
```
ERROR: Cannot open disk
open: Permission denied
```
**Solution**: Run with `sudo`

### Device is Busy
```
write: Device or resource busy
```
**Solution**: Unmount all partitions first:
```bash
sudo umount /dev/sdX*
```

### Cannot Get Disk Size
```
ERROR: Cannot get disk size
ioctl(BLKGETSIZE64): Invalid argument
```
**Solution**: Make sure you're targeting the whole disk, not a partition:
- Correct: `/dev/sda`
- Wrong: `/dev/sda1`

## Differences from macOS

| Feature | macOS | Linux |
|---------|-------|-------|
| Device naming | `/dev/diskX` | `/dev/sdX`, `/dev/nvmeXnY` |
| Raw device | `/dev/rdiskX` | Same as regular device |
| List command | `diskutil list` | `lsblk` |
| Unmount command | `diskutil unmountDisk` | `umount` |
| Size ioctl | `DKIOCGETBLOCKCOUNT` | `BLKGETSIZE64` |

## Required Utilities

The program uses these standard Linux utilities:
- `lsblk` - List block devices
- `mount` - Show mounted filesystems
- `umount` - Unmount filesystems

These are part of the `util-linux` package, which is standard on all Linux distributions.

## Known Issues on Linux

1. **NVMe devices**: Naming is different (`nvme0n1` vs `sda`). The program handles this correctly.
2. **Partition detection**: The system disk check looks for root partition. If you have a complex setup (LVM, RAID, etc.), the detection might not work perfectly.
3. **Loop devices**: Work fine for testing but have different performance characteristics than real disks.

## Security Note

The system disk detection on Linux checks if:
- Any partition of the disk is mounted as `/`
- The disk appears in mount table for root filesystem

This is a safety measure but not foolproof. **Always double-check which disk you're erasing!**
