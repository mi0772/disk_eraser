#define _POSIX_C_SOURCE 200809L

#include "disk_ops.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <limits.h>

#ifdef __APPLE__
#include <sys/disk.h>
#elif __linux__
#include <linux/fs.h>
#include <sys/mount.h>
#endif

// Variabile globale per gestire interruzioni
extern volatile sig_atomic_t interrupted;

int list_disks(void) {
    printf("\nAvailable disks:\n");

#ifdef __APPLE__
    // macOS: use diskutil
    FILE *fp = popen("diskutil list", "r");
    if (!fp) {
        perror("popen");
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        printf("  %s", line);
    }

    pclose(fp);
#elif __linux__
    // Linux: use lsblk
    FILE *fp = popen("lsblk -d -o NAME,SIZE,TYPE,MOUNTPOINT", "r");
    if (!fp) {
        perror("popen");
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        printf("  %s", line);
    }

    pclose(fp);
#else
    fprintf(stderr, "Unsupported operating system\n");
    return -1;
#endif

    return 0;
}

int is_system_disk(const char *disk_path) {
    // Estrarre il numero del disco
    const char *disk_name = strrchr(disk_path, '/');
    if (!disk_name) {
        disk_name = disk_path;
    } else {
        disk_name++; // Skip '/'
    }

    // Rimuovere eventuale 'r' davanti (rdisk -> disk su macOS)
    if (disk_name[0] == 'r') {
        disk_name++;
    }

#ifdef __APPLE__
    // macOS: use diskutil to check if it's a system disk
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "diskutil info %s | grep -i 'System Disk\\|boot'", disk_name);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    char line[256];
    int is_system = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "Yes") || strstr(line, "true")) {
            is_system = 1;
            break;
        }
    }

    pclose(fp);
    return is_system;
#elif __linux__
    // Linux: check if disk is mounted as root or contains root partition
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mount | grep -E '^/dev/%s[0-9]* on / ' || lsblk -no MOUNTPOINT /dev/%s | grep -E '^/$'",
             disk_name, disk_name);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        return -1;
    }

    char line[256];
    int is_system = 0;

    if (fgets(line, sizeof(line), fp)) {
        // If we got any output, it means the disk contains root partition
        is_system = 1;
    }

    pclose(fp);
    return is_system;
#else
    return -1;
#endif
}

int verify_disk(const char *disk_path) {
    // Verificare che il path esista
    struct stat st;
    if (stat(disk_path, &st) != 0) {
        fprintf(stderr, "ERROR: Disk %s does not exist\n", disk_path);
        return 0;
    }

    // Verificare che sia un device
    if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode)) {
        fprintf(stderr, "ERROR: %s is not a disk device\n", disk_path);
        return 0;
    }

    // Verificare che non sia il disco di sistema
    if (is_system_disk(disk_path)) {
        fprintf(stderr, "ERROR: Cannot erase system disk!\n");
        return 0;
    }

    return 1;
}

int unmount_disk(const char *disk_path) {
    // Estrarre il nome del disco
    const char *disk_name = strrchr(disk_path, '/');
    if (!disk_name) {
        disk_name = disk_path;
    } else {
        disk_name++;
    }

    // Rimuovere eventuale 'r' davanti
    if (disk_name[0] == 'r') {
        disk_name++;
    }

    printf("Unmounting disk...\n");

#ifdef __APPLE__
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "diskutil unmountDisk /dev/%s", disk_name);
    int ret = system(cmd);
#elif __linux__
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "umount /dev/%s* 2>/dev/null || true", disk_name);
    int ret = system(cmd);
#else
    int ret = -1;
#endif

    if (ret != 0) {
        fprintf(stderr, "WARNING: Failed to unmount disk (may already be unmounted)\n");
    }

    return 0; // Non è critico se fallisce
}

int open_disk_raw(const char *disk_path) {
    char raw_path[PATH_MAX];

#ifdef __APPLE__
    // macOS: use raw device (/dev/rdiskX) for better performance
    if (strstr(disk_path, "/dev/rdisk")) {
        strncpy(raw_path, disk_path, sizeof(raw_path) - 1);
    } else {
        const char *disk_name = strrchr(disk_path, '/');
        if (!disk_name) {
            disk_name = disk_path;
        } else {
            disk_name++;
        }

        // Se non inizia già con 'r', aggiungerlo
        if (disk_name[0] == 'r') {
            snprintf(raw_path, sizeof(raw_path), "/dev/%s", disk_name);
        } else {
            snprintf(raw_path, sizeof(raw_path), "/dev/r%s", disk_name);
        }
    }
#elif __linux__
    // Linux: just use the device path directly (no separate raw device)
    strncpy(raw_path, disk_path, sizeof(raw_path) - 1);
    raw_path[sizeof(raw_path) - 1] = '\0';
#else
    return -1;
#endif

    printf("Opening raw device: %s\n", raw_path);

    int fd = open(raw_path, O_WRONLY | O_SYNC);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    return fd;
}

ssize_t get_disk_size(int fd) {
#ifdef __APPLE__
    uint64_t block_count;
    uint32_t block_size;

    // Ottenere block size
    if (ioctl(fd, DKIOCGETBLOCKSIZE, &block_size) < 0) {
        perror("ioctl(DKIOCGETBLOCKSIZE)");
        return -1;
    }

    // Ottenere numero di blocchi
    if (ioctl(fd, DKIOCGETBLOCKCOUNT, &block_count) < 0) {
        perror("ioctl(DKIOCGETBLOCKCOUNT)");
        return -1;
    }

    return (ssize_t)(block_count * block_size);
#elif __linux__
    uint64_t size;

    // Linux: use BLKGETSIZE64 to get size in bytes
    if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
        perror("ioctl(BLKGETSIZE64)");
        return -1;
    }

    return (ssize_t)size;
#else
    return -1;
#endif
}

int wipe_disk(int fd, size_t disk_size, progress_info_t *progress) {
    const size_t BUFFER_SIZE = 1024 * 1024; // 1MB
    void *buffer;

    // Allocare buffer allineato per performance ottimali
    if (posix_memalign(&buffer, 4096, BUFFER_SIZE) != 0) {
        perror("posix_memalign");
        return -1;
    }

    // Riempire buffer con zeri
    memset(buffer, 0, BUFFER_SIZE);

    size_t total_written = 0;

    while (total_written < disk_size) {
        // Controllare se l'operazione è stata interrotta
        if (interrupted) {
            printf("\n\nOperation interrupted by user.\n");
            free(buffer);
            return -2; // Codice speciale per interruzione
        }

        size_t to_write = (disk_size - total_written > BUFFER_SIZE)
                          ? BUFFER_SIZE
                          : (disk_size - total_written);

        ssize_t written = write(fd, buffer, to_write);
        if (written < 0) {
            if (errno == EINTR) {
                continue; // Retry su interrupt
            }
            perror("write");
            free(buffer);
            return -1;
        }

        total_written += written;
        progress_update(progress, written);
    }

    free(buffer);

    // Assicurarsi che tutto sia scritto su disco
    printf("\nSyncing to disk...\n");
    if (fsync(fd) < 0) {
        perror("fsync");
        return -1;
    }

    return 0;
}
