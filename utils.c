#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

char* format_bytes(size_t bytes, char *buffer, size_t buffer_size) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        unit_index++;
    }

    snprintf(buffer, buffer_size, "%.1f %s", size, units[unit_index]);
    return buffer;
}

char* format_time(time_t seconds, char *buffer, size_t buffer_size) {
    if (seconds < 60) {
        snprintf(buffer, buffer_size, "%lds", (long)seconds);
    } else if (seconds < 3600) {
        long mins = seconds / 60;
        long secs = seconds % 60;
        snprintf(buffer, buffer_size, "%ldm %lds", mins, secs);
    } else if (seconds < 86400) {
        long hours = seconds / 3600;
        long mins = (seconds % 3600) / 60;
        snprintf(buffer, buffer_size, "%ldh %ldm", hours, mins);
    } else {
        long days = seconds / 86400;
        long hours = (seconds % 86400) / 3600;
        snprintf(buffer, buffer_size, "%ldd %ldh", days, hours);
    }

    return buffer;
}

int confirm_action(const char *message) {
    char input[128];

    printf("%s: ", message);
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }

    // Rimuovere newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }

    return (strcmp(input, "YES") == 0);
}

void log_message(const char *format, ...) {
    // Aprire file di log in append mode
    FILE *log_file = fopen("disk_erase.log", "a");
    if (!log_file) {
        return; // Ignorare errori di logging
    }

    // Ottenere timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Scrivere timestamp
    fprintf(log_file, "[%s] ", timestamp);

    // Scrivere messaggio
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);

    fprintf(log_file, "\n");
    fclose(log_file);
}

int is_root(void) {
    return (geteuid() == 0);
}
