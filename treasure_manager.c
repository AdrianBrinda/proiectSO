#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t received_signal = 0;

void handle_signal(int sig)
{
    received_signal = 1;
}

int is_hidden(const char *name)
{
    return name[0] == '.';
}

void list_all_hunts()
{
    DIR *main_dir = opendir(".");
    if (!main_dir)
    {
        perror("Could not open current directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(main_dir)) != NULL)
    {
        if (entry->d_type == DT_DIR &&
            !is_hidden(entry->d_name) &&
            strcmp(entry->d_name, "vscode") != 0)
        {

            char sub_path[256];
            snprintf(sub_path, sizeof(sub_path), "%s", entry->d_name);

            DIR *sub_dir = opendir(sub_path);
            if (!sub_dir)
                continue;

            int count = 0;
            struct dirent *file;
            while ((file = readdir(sub_dir)) != NULL)
            {
                if (!is_hidden(file->d_name) &&
                    strcmp(file->d_name, "logged_hunt") != 0)
                    count++;
            }

            closedir(sub_dir);
            printf("Hunt: %s | Treasures: %d\n", entry->d_name, count);
        }
    }

    closedir(main_dir);
    fflush(stdout);
}

void list_treasures_in_hunt(const char *folder)
{
    DIR *dir = opendir(folder);
    if (!dir)
    {
        printf("Hunt '%s' not found.\n", folder);
        fflush(stdout);
        return;
    }

    printf("Treasures in hunt '%s':\n", folder);
    struct dirent *file;
    while ((file = readdir(dir)) != NULL)
    {
        if (!is_hidden(file->d_name) &&
            strcmp(file->d_name, "logged_hunt") != 0)
        {
            printf(" - %s\n", file->d_name);
        }
    }

    closedir(dir);
    fflush(stdout);
}

void display_treasure(const char *folder, const char *treasure)
{
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s/%s", folder, treasure);

    FILE *f = fopen(filepath, "r");
    if (!f)
    {
        printf("Could not read treasure '%s' in hunt '%s'.\n", treasure, folder);
        fflush(stdout);
        return;
    }

    printf("Contents of treasure '%s' from '%s':\n", treasure, folder);
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        printf("%s", line);
    }

    fclose(f);
    fflush(stdout);
}

void execute_command(const char *command)
{
    char cmd[64], arg1[64], arg2[64];
    int count = sscanf(command, "%s %s %s", cmd, arg1, arg2);

    if (strcmp(cmd, "list_hunts") == 0)
    {
        list_all_hunts();
    }
    else if (strcmp(cmd, "list_treasures") == 0 && count >= 2)
    {
        list_treasures_in_hunt(arg1);
    }
    else if (strcmp(cmd, "view_treasure") == 0 && count == 3)
    {
        display_treasure(arg1, arg2);
    }
    else if (strcmp(cmd, "stop") == 0)
    {
        printf("Shutting down...\n");
        fflush(stdout);
        exit(0);
    }
    else
    {
        printf("Unknown command: '%s'\n", command);
        fflush(stdout);
    }
}

int main()
{
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGUSR1, &sa, NULL);

    while (1)
    {
        if (received_signal)
        {
            received_signal = 0;

            FILE *f = fopen("monitor_command.cmd", "r");
            if (!f)
            {
                perror("Cannot open command file");
                continue;
            }

            char cmd[256];
            if (fgets(cmd, sizeof(cmd), f))
            {
                cmd[strcspn(cmd, "\n")] = '\0';
                execute_command(cmd);
            }

            fclose(f);
        }

        usleep(100000);
    }

    return 0;
}
