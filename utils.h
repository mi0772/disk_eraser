#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <time.h>

char* format_bytes(size_t bytes, char *buffer, size_t buffer_size);
char* format_time(time_t seconds, char *buffer, size_t buffer_size);
int confirm_action(const char *message);
void log_message(const char *format, ...);
int is_root(void);

#endif // UTILS_H
