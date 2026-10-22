#ifndef SOP_PROJEKT1_WATCH_MAP_H
#define SOP_PROJEKT1_WATCH_MAP_H
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))
#define DEFAULT_CAPACITY 32

typedef struct
{
    int wd;
    char* path;
} Watcher;

typedef struct
{
    Watcher* watchers;
    int count;
    int capacity;
    int inotify_fd;
} WatchMap;

void map_init(WatchMap* map, int inotify_fd);
void map_add(WatchMap* map, char* path);
char* get(WatchMap* map, int wd);
void map_free(WatchMap* map);
void map_remove(WatchMap* map, char* path);

#endif  // SOP_PROJEKT1_WATCH_MAP_H

