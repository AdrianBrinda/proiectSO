#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>

int pipefd[2];
pid_t manager_pid = -1;
int monitor_active = 0;

void child_exit_handler(int sig)
{
    int status;
    if (waitpid(manager_pid, &status, WNOHANG) > 0)
    {
        printf("Monitor exited (code %d)\n", WEXITSTATUS(status));
        monitor_active = 0;
    }
}

void transmit_command(const char *cmd)
{
    FILE *f = fopen("monitor_command.cmd", "w");
    if (!f)
    {
        perror("Cannot write command");
        return;
    }
    fprintf(f, "%s\n", cmd);
    fclose(f);

    kill(manager_pid, SIGUSR1);
    usleep(200000);
    char buff[1024];
    ssize_t len = read(pipefd[0], buff, sizeof(buff) - 1);
    if (len > 0)
    {
        buff[len] = '\0';
        printf("%s", buff);
    }
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = child_exit_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char input[128];

    while (1)
    {
        printf("hub_> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "start_monitor") == 0)
        {
            if (monitor_active)
            {
                printf("Monitor already running (PID %d)\n", manager_pid);
                continue;
            }

            if (pipe(pipefd) == -1)
            {
                perror("pipe");
                continue;
            }

            manager_pid = fork();
            if (manager_pid == 0)
            {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                execl("./treasure_manager", "treasure_manager", NULL);
                perror("exec");
                exit(1);
            }
            else
            {
                close(pipefd[1]);
                monitor_active = 1;
                printf("Monitor started (PID %d)\n", manager_pid);
            }
        }
        else if (strcmp(input, "stop_monitor") == 0)
        {
            if (!monitor_active)
            {
                printf("No monitor to stop.\n");
                continue;
            }
            transmit_command("stop");
        }
        else if (strncmp(input, "list_", 5) == 0 || strncmp(input, "view_", 5) == 0)
        {
            if (!monitor_active)
            {
                printf("Start the monitor first.\n");
                continue;
            }
            transmit_command(input);
        }
        else if (strcmp(input, "calculate_score") == 0)
        {
            DIR *d = opendir(".");
            if (!d)
            {
                perror("opendir");
                continue;
            }

            struct dirent *entry;
            while ((entry = readdir(d)) != NULL)
            {
                if (entry->d_type == DT_DIR &&
                    entry->d_name[0] != '.' &&
                    strcmp(entry->d_name, "vscode") != 0)
                {

                    int p[2];
                    if (pipe(p) == -1)
                    {
                        perror("pipe");
                        continue;
                    }

                    pid_t scorer = fork();
                    if (scorer == 0)
                    {
                        close(p[0]);
                        dup2(p[1], STDOUT_FILENO);
                        execl("./score_calculator", "score_calculator", entry->d_name, NULL);
                        perror("exec score_calculator");
                        exit(1);
                    }
                    else
                    {
                        close(p[1]);
                        char line[256];
                        ssize_t n;
                        printf("Scores for %s:\n", entry->d_name);
                        while ((n = read(p[0], line, sizeof(line) - 1)) > 0)
                        {
                            line[n] = '\0';
                            printf("%s", line);
                        }
                        close(p[0]);
                        waitpid(scorer, NULL, 0);
                    }
                }
            }

            closedir(d);
        }
        else if (strcmp(input, "exit") == 0)
        {
            if (monitor_active)
            {
                printf("Stop monitor first before exiting.\n");
            }
            else
            {
                break;
            }
        }
        else
        {
            printf("Unknown command: %s\n", input);
        }
    }

    return 0;
}
