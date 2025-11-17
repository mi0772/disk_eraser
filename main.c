#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include "disk_ops.h"
#include "progress.h"
#include "utils.h"

#define VERSION "1.0"

// Variabile globale per gestire interruzioni
volatile sig_atomic_t interrupted = 0;

void signal_handler(int signum) {
    (void)signum; // Evitare warning unused parameter
    interrupted = 1;
}

void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void print_header(void) {
    printf("\n");
    printf("====================================\n");
    printf("    Disk Erase Tool v%s\n", VERSION);
    printf("====================================\n");
}

void print_warning(const char *disk_path, size_t disk_size) {
    char size_str[64];
    format_bytes(disk_size, size_str, sizeof(size_str));

    printf("\n");
    printf("!!! WARNING !!!\n");
    printf("================\n");
    printf("This will permanently erase ALL data on:\n");
    printf("  Device: %s\n", disk_path);
    printf("  Size: %s\n", size_str);
    printf("\nThis operation CANNOT be undone!\n");
    printf("ALL data will be PERMANENTLY lost!\n\n");
}

int get_disk_selection(char *disk_path, size_t path_size) {
#ifdef __APPLE__
    printf("\nEnter disk to erase (e.g., disk2 or /dev/disk2), or 'QUIT' to exit: ");
#elif __linux__
    printf("\nEnter disk to erase (e.g., sda or /dev/sda), or 'QUIT' to exit: ");
#else
    printf("\nEnter disk to erase, or 'QUIT' to exit: ");
#endif
    fflush(stdout);

    if (fgets(disk_path, path_size, stdin) == NULL) {
        return -1;
    }

    // Rimuovere newline
    size_t len = strlen(disk_path);
    if (len > 0 && disk_path[len - 1] == '\n') {
        disk_path[len - 1] = '\0';
    }

    // Controllare se l'utente vuole uscire
    if (strcasecmp(disk_path, "QUIT") == 0 || strcasecmp(disk_path, "EXIT") == 0) {
        return -2; // Codice speciale per uscita
    }

    // Se l'utente ha inserito solo il nome del device senza /dev/, aggiungerlo
    if (strncmp(disk_path, "/dev/", 5) != 0) {
        char temp[256];
        strncpy(temp, disk_path, sizeof(temp) - 1);
        snprintf(disk_path, path_size, "/dev/%s", temp);
    }

    return 0;
}

int confirm_disk_selection(const char *disk_path) {
    char input[128];

    // Estrarre il nome del disco per la conferma
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

    printf("Type '%s' to confirm: ", disk_name);
    fflush(stdout);

    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }

    // Rimuovere newline
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
    }

    return (strcmp(input, disk_name) == 0);
}

int main(void) {
    char disk_path[256];
    int fd = -1;
    ssize_t disk_size;
    progress_info_t progress;

    print_header();

    // 1. Verificare permessi di root
    if (!is_root()) {
        fprintf(stderr, "\nERROR: This program must be run as root (use sudo)\n\n");
        log_message("Failed: not running as root");
        return 1;
    }

    log_message("Program started");

    // 2. Mostrare lista dischi
    printf("\n");
    if (list_disks() != 0) {
        fprintf(stderr, "ERROR: Failed to list disks\n");
        log_message("Failed to list disks");
        return 1;
    }

    // 3. Chiedere quale disco cancellare
    int selection_result = get_disk_selection(disk_path, sizeof(disk_path));
    if (selection_result == -2) {
        printf("\nOperation cancelled by user.\n");
        log_message("User chose to quit");
        return 0;
    } else if (selection_result != 0) {
        fprintf(stderr, "ERROR: Invalid input\n");
        log_message("Invalid disk selection");
        return 1;
    }

    log_message("Selected disk: %s", disk_path);

    // 4. Verificare il disco
    if (!verify_disk(disk_path)) {
        log_message("Disk verification failed: %s", disk_path);
        return 1;
    }

    // 5. Aprire il disco per ottenere le informazioni
    printf("\nGetting disk information...\n");
    fd = open_disk_raw(disk_path);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open disk\n");
        log_message("Failed to open disk: %s", disk_path);
        return 1;
    }

    disk_size = get_disk_size(fd);
    if (disk_size < 0) {
        fprintf(stderr, "ERROR: Cannot get disk size\n");
        close(fd);
        log_message("Failed to get disk size");
        return 1;
    }

    close(fd);

    // 6. Mostrare warning e richiedere prima conferma
    print_warning(disk_path, disk_size);

    if (!confirm_action("Type 'YES' to continue")) {
        printf("\nOperation cancelled.\n");
        log_message("Operation cancelled by user (first confirmation)");
        return 0;
    }

    // 7. Richiedere seconda conferma
    printf("\n");
    if (!confirm_disk_selection(disk_path)) {
        printf("\nOperation cancelled.\n");
        log_message("Operation cancelled by user (second confirmation)");
        return 0;
    }

    log_message("User confirmed operation");

    // 8. Unmount del disco
    printf("\n");
    unmount_disk(disk_path);

    // 9. Aprire device raw per scrittura
    printf("\n");
    fd = open_disk_raw(disk_path);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open disk for writing\n");
        log_message("Failed to open disk for writing");
        return 1;
    }

    // 10. Setup signal handlers
    setup_signal_handlers();

    // 11. Inizializzazione progress tracker
    printf("\nStarting secure erase...\n\n");
    progress_init(&progress, disk_size);
    log_message("Starting wipe operation - size: %zu bytes", disk_size);

    // 12. Loop di scrittura con progress display
    int result = wipe_disk(fd, disk_size, &progress);

    // 13. Chiusura
    close(fd);

    // 14. Report finale
    if (result == 0) {
        progress_finish(&progress);
        log_message("Operation completed successfully");
    } else if (result == -2) {
        printf("\nOperation was interrupted.\n");
        printf("Disk may be partially erased.\n");
        log_message("Operation interrupted by user");
        return 2;
    } else {
        fprintf(stderr, "\nERROR: Operation failed\n");
        log_message("Operation failed with error");
        return 1;
    }

    return 0;
}
