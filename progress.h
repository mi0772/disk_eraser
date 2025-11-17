#ifndef PROGRESS_H
#define PROGRESS_H

#include <stddef.h>
#include <time.h>

typedef struct {
    size_t total_bytes;
    size_t written_bytes;
    time_t start_time;
    time_t last_update;
    double speed_mbps;
} progress_info_t;

void progress_init(progress_info_t *info, size_t total_bytes);
void progress_update(progress_info_t *info, size_t bytes_written);
void progress_display(const progress_info_t *info);
void progress_finish(const progress_info_t *info);

#endif // PROGRESS_H
