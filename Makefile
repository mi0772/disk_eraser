CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
TARGET = disk_eraser

SRCS = main.c disk_ops.c progress.c utils.c
OBJS = $(SRCS:.c=.o)
HEADERS = disk_ops.h progress.h utils.h

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

# Clean
clean:
	rm -f $(TARGET) $(OBJS)

# Clean and rebuild
rebuild: clean all

# Install (optional)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

.PHONY: all clean rebuild install uninstall
