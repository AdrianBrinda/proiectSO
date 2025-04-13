#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

typedef struct comoara
{
    char ID[10];
    char nume[20];
    double lat;
    double lng;
    char indiciu[50];
    int valoare;
} Comoara_t;

void add(char *huntID, Comoara_t comoara)
{
    int result = mkdir(huntID, 0755);
    if (result == 0)
    {
        printf("Directory created successfully.\n");
    }
    else
    {
        perror("Error creating directory");
    }
    char filename[50];
    strcpy(filename, huntID);
    strcat(filename, "/");
    strcat(filename, comoara.ID);

    FILE *f = fopen(filename, "w+");
    if (f == NULL)
        printf("nu e bine\n");

    fprintf(f, "%s\n", comoara.ID);
    fprintf(f, "%s\n", comoara.nume);
    fprintf(f, "%lf\n", comoara.lat);
    fprintf(f, "%lf\n", comoara.lng);
    fprintf(f, "%s\n", comoara.indiciu);
    fprintf(f, "%d\n", comoara.valoare);
    fclose(f);
}

int list(char *huntID)
{
    struct dirent *entry;
    DIR *dir = opendir(huntID);
    struct stat fileStat;

    if (dir == NULL)
    {
        perror("Unable to open directory");
        return 1;
    }

    long long totalSize = 0;
    time_t lastModTime = 0;

    // Trebuie să parcurgem directorul de două ori: o dată pentru statistici, apoi pentru listare
    printf("Hunt: %s\n", huntID);

    // Prima trecere – calculează dimensiunea totală și ultima modificare
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char filepath[300];
            snprintf(filepath, sizeof(filepath), "%s/%s", huntID, entry->d_name);

            if (stat(filepath, &fileStat) == 0)
            {
                totalSize += fileStat.st_size;
                if (fileStat.st_mtime > lastModTime)
                {
                    lastModTime = fileStat.st_mtime;
                }
            }
            else
            {
                perror("stat failed");
            }
        }
    }

    // Afișează statistici
    printf("Total file size: %lld bytes\n", totalSize);
    printf("Last modified: %s", ctime(&lastModTime));

    // Re-deschide directorul pentru a lista comorile
    rewinddir(dir);
    printf("Treasures:\n");

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            printf("- %s\n", entry->d_name);
        }
    }

    closedir(dir);
    return 0;
}

void view(char *huntID, char *ID)
{
    char filename[100];
    snprintf(filename, sizeof(filename), "%s/%s", huntID, ID);

    FILE *f = fopen(filename, "r");
    if (f == NULL)
    {
        perror("Eroare la deschiderea fișierului");
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), f) != NULL)
    {
        printf("%s", buffer);
    }

    fclose(f);
}

void removeTreasure(char *huntID, char *ID)
{

    char filename[100];
    snprintf(filename, sizeof(filename), "%s/%s", huntID, ID);

    if (remove(filename) == 0)
    {
        printf("File '%s' deleted successfully.\n", filename);
    }
    else
    {
        perror("remove failed");
    }
}

void removeHunt(char *huntID)
{
    DIR *dir = opendir(huntID);
    struct dirent *entry;

    if (dir == NULL)
    {
        perror("Unable to open directory");
        return;
    }

    char filepath[300];

    while ((entry = readdir(dir)) != NULL)
    {
        // Sari peste "." și ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Creează calea completă către fișier
        snprintf(filepath, sizeof(filepath), "%s/%s", huntID, entry->d_name);

        if (remove(filepath) != 0)
        {
            perror("Failed to delete file");
        }
    }

    closedir(dir);

    // Acum directorul este gol, deci îl putem șterge
    if (rmdir(huntID) == 0)
    {
        printf("Directory '%s' removed successfully.\n", huntID);
    }
    else
    {
        perror("Failed to remove directory");
    }
}

void logger(char **comanda, int argc)
{
    char filename[100];
    snprintf(filename, sizeof(filename), "%s/logged_hunt", comanda[2]);

    FILE *f = fopen(filename, "a");
    if (f == NULL)
    {
        perror("Eroare la deschiderea fișierului");
        return;
    }

    for (int i = 0; i < argc; ++i)
    {
        fprintf(f, "%s ", comanda[i]);
    }
    fprintf(f, "\n");

    fclose(f);

    char symlinkName[100];
    snprintf(symlinkName, sizeof(symlinkName), "logged_hunt-%s", comanda[2]);

    symlink(filename, symlinkName);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        exit(-1);
    }

    if (strcmp(argv[1], "--add") == 0)
    {
        Comoara_t comoara;
        printf("Introdu id, nume, lat, lng, indiciu, valoare\n");
        scanf("%9s", comoara.ID);
        scanf("%19s", comoara.nume);
        scanf("%lf", &comoara.lat);
        scanf("%lf", &comoara.lng);
        scanf("%49s", comoara.indiciu);
        scanf("%d", &comoara.valoare);
        char huntID[10];
        strcpy(huntID, argv[2]);
        add(huntID, comoara);
        logger(argv, argc);
    }

    if (strcmp(argv[1], "--list") == 0)
    {
        char huntID[10];
        strcpy(huntID, argv[2]);
        list(huntID);
        logger(argv, argc);
    }
    if (strcmp(argv[1], "--removeHunt") == 0)
    {
        char huntID[10];
        strcpy(huntID, argv[2]);
        removeHunt(huntID);
        logger(argv, argc);
    }

    if (strcmp(argv[1], "--removeTreasure") == 0)
    {
        char huntID[10], ID[10];
        strcpy(huntID, argv[2]);
        strcpy(ID, argv[3]);
        removeTreasure(huntID, ID);
        logger(argv, argc);
    }

    if (strcmp(argv[1], "--view") == 0)
    {
        char huntID[10], ID[10];
        strcpy(huntID, argv[2]);
        strcpy(ID, argv[3]);
        view(huntID, ID);
        logger(argv, argc);
    }

    return 0;
}