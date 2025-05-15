#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

typedef struct
{
    char user[50];
    int total;
} Player;

int find_player(Player *list, int count, const char *name)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(list[i].user, name) == 0)
            return i;
    }
    return -1;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hunt_folder>\n", argv[0]);
        return 1;
    }

    DIR *dir = opendir(argv[1]);
    if (!dir)
    {
        perror("Can't open folder");
        return 1;
    }

    Player players[100];
    int total = 0;

    struct dirent *f;
    while ((f = readdir(dir)) != NULL)
    {
        if (f->d_type != DT_REG)
            continue;
        if (strcmp(f->d_name, "logged_hunt") == 0)
            continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", argv[1], f->d_name);

        FILE *file = fopen(filepath, "r");
        if (!file)
            continue;

        char id[20], name[50], lat[20], lon[20], clue[50];
        int val;

        fscanf(file, "%s\n%s\n%s\n%s\n%s\n%d", id, name, lat, lon, clue, &val);
        fclose(file);

        int pos = find_player(players, total, name);
        if (pos == -1)
        {
            strcpy(players[total].user, name);
            players[total].total = val;
            total++;
        }
        else
        {
            players[pos].total += val;
        }
    }

    closedir(dir);

    for (int i = 0; i < total; i++)
    {
        printf("%s: %d\n", players[i].user, players[i].total);
    }

    return 0;
}
