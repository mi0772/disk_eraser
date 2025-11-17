#ifndef DISK_OPS_H
#define DISK_OPS_H

#include <stddef.h>
#include <sys/types.h>
#include "progress.h"

// Funzioni per gestire le operazioni sul disco
int list_disks(void);
int verify_disk(const char *disk_path);
int is_system_disk(const char *disk_path);
int unmount_disk(const char *disk_path);
int open_disk_raw(const char *disk_path);
ssize_t get_disk_size(int fd);
int wipe_disk(int fd, size_t disk_size, progress_info_t *progress);

#endif // DISK_OPS_H
