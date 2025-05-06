#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>     // <== pentru sigaction, SA_RESTART etc.
#include <dirent.h>     // <== pentru struct dirent și DT_DIR
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>   // <== pentru waitpid și struct rusage dacă e folosit


volatile sig_atomic_t got_command = 0;

void sigusr1_handler(int sig) {
    got_command = 1;
}

void list_hunts() {
    DIR *dir = opendir(".");
    struct dirent *entry;

    if (!dir) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {

            int count = 0;
            char path[512];
            snprintf(path, sizeof(path), "./%s", entry->d_name);

            DIR *subdir = opendir(path);
            struct dirent *subentry;

            if (subdir) {
                while ((subentry = readdir(subdir)) != NULL) {
                    if (strcmp(subentry->d_name, ".") != 0 && strcmp(subentry->d_name, "..") != 0) {
                        count++;
                    }
                }
                closedir(subdir);
            }

            printf("Hunt: %s, Treasures: %d\n", entry->d_name, count);
        }
    }

    closedir(dir);
}

void list_treasures(const char *huntID) {
    char path[256];
    snprintf(path, sizeof(path), "%s", huntID);
    DIR *dir = opendir(path);

    if (!dir) {
        printf("Hunt '%s' does not exist.\n", huntID);
        return;
    }

    struct dirent *entry;
    printf("Treasures in hunt '%s':\n", huntID);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
            printf("- %s\n", entry->d_name);
        }
    }

    closedir(dir);
}

void view_treasure(const char *huntID, const char *ID) {
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", huntID, ID);

    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Treasure '%s' not found in hunt '%s'.\n", ID, huntID);
        return;
    }

    char line[256];
    printf("Details of treasure '%s' in hunt '%s':\n", ID, huntID);
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }

    fclose(f);
}

void handle_command(const char *command) {
    char cmd[50], arg1[50], arg2[50];
    int args = sscanf(command, "%s %s %s", cmd, arg1, arg2);

    if (strcmp(cmd, "list_hunts") == 0) {
        list_hunts();
    } else if (strcmp(cmd, "list_treasures") == 0 && args >= 2) {
        list_treasures(arg1);
    } else if (strcmp(cmd, "view_treasure") == 0 && args == 3) {
        view_treasure(arg1, arg2);
    } else if (strcmp(cmd, "stop") == 0) {
        printf("Stopping monitor...\n");
        usleep(500000);  // întârziere pentru testare (0.5 secunde)
        exit(0);
    } else {
        printf("Unknown or malformed command: '%s'\n", command);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigusr1_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    printf("Monitor running with PID %d\n", getpid());

    while (1) {
        if (got_command) {
            got_command = 0;

            FILE *f = fopen("monitor_command.cmd", "r");
            if (!f) {
                perror("Could not open command file");
                continue;
            }

            char line[256];
            if (fgets(line, sizeof(line), f)) {
                line[strcspn(line, "\n")] = 0;  // elimină newline
                handle_command(line);
            }

            fclose(f);
        }

        usleep(100000);  // evita busy-wait
    }

    return 0;
}
