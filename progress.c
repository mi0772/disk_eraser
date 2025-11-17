#include "progress.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

void progress_init(progress_info_t *info, size_t total_bytes) {
    info->total_bytes = total_bytes;
    info->written_bytes = 0;
    info->start_time = time(NULL);
    info->last_update = info->start_time;
    info->speed_mbps = 0.0;
}

void progress_update(progress_info_t *info, size_t bytes_written) {
    info->written_bytes += bytes_written;

    time_t current_time = time(NULL);

    // Aggiornare il display solo ogni secondo per evitare troppi refresh
    if (current_time > info->last_update) {
        time_t elapsed = current_time - info->start_time;

        if (elapsed > 0) {
            // Calcolare velocità in MB/s
            info->speed_mbps = (double)info->written_bytes / (double)elapsed / (1024.0 * 1024.0);
        }

        info->last_update = current_time;
        progress_display(info);
    }
}

void progress_display(const progress_info_t *info) {
    // Calcolare percentuale
    double percentage = 0.0;
    if (info->total_bytes > 0) {
        percentage = ((double)info->written_bytes / (double)info->total_bytes) * 100.0;
    }

    // Creare progress bar
    const int bar_width = 40;
    int filled = (int)(percentage / 100.0 * bar_width);

    char bar[bar_width + 1];
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            bar[i] = '=';
        } else if (i == filled) {
            bar[i] = '>';
        } else {
            bar[i] = ' ';
        }
    }
    bar[bar_width] = '\0';

    // Formattare i byte
    char written_str[64], total_str[64];
    format_bytes(info->written_bytes, written_str, sizeof(written_str));
    format_bytes(info->total_bytes, total_str, sizeof(total_str));

    // Calcolare tempo trascorso
    time_t elapsed = time(NULL) - info->start_time;
    char elapsed_str[64];
    format_time(elapsed, elapsed_str, sizeof(elapsed_str));

    // Calcolare ETA
    char eta_str[64] = "calculating...";
    if (info->speed_mbps > 0 && info->written_bytes > 0) {
        size_t remaining_bytes = info->total_bytes - info->written_bytes;
        time_t eta_seconds = (time_t)((double)remaining_bytes / (info->speed_mbps * 1024.0 * 1024.0));
        format_time(eta_seconds, eta_str, sizeof(eta_str));
    }

    // Stampare progress bar (sovrascrivendo la linea precedente)
    printf("\r[%s] %.1f%% | %s / %s", bar, percentage, written_str, total_str);
    printf("\nSpeed: %.2f MB/s | Elapsed: %s | ETA: %s    \033[A",
           info->speed_mbps, elapsed_str, eta_str);
    fflush(stdout);
}

void progress_finish(const progress_info_t *info) {
    // Muovere il cursore alla linea successiva per non sovrascrivere
    printf("\n\n");

    time_t elapsed = time(NULL) - info->start_time;
    double avg_speed = 0.0;

    if (elapsed > 0) {
        avg_speed = (double)info->written_bytes / (double)elapsed / (1024.0 * 1024.0);
    }

    char total_str[64], elapsed_str[64];
    format_bytes(info->total_bytes, total_str, sizeof(total_str));
    format_time(elapsed, elapsed_str, sizeof(elapsed_str));

    // Ottenere timestamp di inizio e fine
    char start_time[64], end_time[64];
    struct tm *tm_info;

    tm_info = localtime(&info->start_time);
    strftime(start_time, sizeof(start_time), "%Y-%m-%d %H:%M:%S", tm_info);

    time_t finish_time = time(NULL);
    tm_info = localtime(&finish_time);
    strftime(end_time, sizeof(end_time), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("Operation completed successfully!\n\n");
    printf("Statistics:\n");
    printf("  Total data written: %s\n", total_str);
    printf("  Total time: %s\n", elapsed_str);
    printf("  Average speed: %.2f MB/s\n", avg_speed);
    printf("  Start: %s\n", start_time);
    printf("  End: %s\n", end_time);
    printf("\nThe disk is now ready to be formatted.\n");
}
