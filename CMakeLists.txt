cmake_minimum_required(VERSION 4.0)
project(disk_eraser C)

set(CMAKE_C_STANDARD 11)

# Source files
set(SOURCES
    main.c
    disk_ops.c
    progress.c
    utils.c
)

# Header files (per completezza, anche se non necessario per la compilazione)
set(HEADERS
    disk_ops.h
    progress.h
    utils.h
)

add_executable(disk_eraser ${SOURCES} ${HEADERS})

# Enable all warnings
target_compile_options(disk_eraser PRIVATE -Wall -Wextra)
