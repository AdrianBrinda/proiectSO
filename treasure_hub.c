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


pid_t monitor_pid = -1;
int monitor_running = 0;
int monitor_exiting = 0;

// Handler pentru terminarea monitorului
void sigchld_handler(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid > 0) {
        printf("Monitor process terminated with status %d\n", WEXITSTATUS(status));
        monitor_running = 0;
        monitor_exiting = 0;
        monitor_pid = -1;
    }
}

// Scrie comanda în fișierul partajat și trimite semnal
void send_command_to_monitor(const char *command) {
    FILE *f = fopen("monitor_command.cmd", "w");
    if (f == NULL) {
        perror("Failed to write command file");
        return;
    }

    fprintf(f, "%s\n", command);
    fclose(f);

    kill(monitor_pid, SIGUSR1);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char input[100];

    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitor already running with PID %d\n", monitor_pid);
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./treasure_manager", "./treasure_manager", NULL);
                perror("Failed to start monitor");
                exit(1);
            } else if (monitor_pid > 0) {
                monitor_running = 1;
                printf("Monitor started with PID %d\n", monitor_pid);
            } else {
                perror("Fork failed");
            }

        } else if (strcmp(input, "list_hunts") == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
                continue;
            }
            if (monitor_exiting) {
                printf("Monitor is shutting down. Please wait...\n");
                continue;
            }
            send_command_to_monitor("list_hunts");

        } else if (strncmp(input, "list_treasures", 14) == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
                continue;
            }
            if (monitor_exiting) {
                printf("Monitor is shutting down. Please wait...\n");
                continue;
            }
            send_command_to_monitor(input);  // ex: list_treasures Hunt42

        } else if (strncmp(input, "view_treasure", 13) == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
                continue;
            }
            if (monitor_exiting) {
                printf("Monitor is shutting down. Please wait...\n");
                continue;
            }
            send_command_to_monitor(input);  // ex: view_treasure Hunt42 007

        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
                continue;
            }
            send_command_to_monitor("stop");
            monitor_exiting = 1;

        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Error: Cannot exit while monitor is running. Use stop_monitor first.\n");
                continue;
            }
            break;

        } else {
            printf("Unknown command: %s\n", input);
        }
    }

    return 0;
}
