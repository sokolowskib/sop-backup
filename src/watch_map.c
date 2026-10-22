#include "watch_map.h"

void map_init(WatchMap* map, int inotify_fd)
{
    map->inotify_fd = inotify_fd;
    map->count = 0;
    map->capacity = DEFAULT_CAPACITY;

    map->watchers = malloc(sizeof(Watcher) * map->capacity);
    if (!map->watchers)
        ERR("malloc fail");
}

void map_add(WatchMap* map, char* path)
{
    if (map->count >= map->capacity)
    {
        map->capacity *= 2;
        map->watchers = realloc(map->watchers, map->capacity * sizeof(Watcher));
    }
    int wd;
    if ((wd = inotify_add_watch(map->inotify_fd, path,
                                IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB | IN_CLOSE_WRITE |
                                    IN_MOVE_SELF | IN_DELETE_SELF)) == -1)
    {
        ERR("inotify_add_watch failed");
    }
    map->watchers[map->count].path = strdup(path);
    map->watchers[map->count++].wd = wd;
}

char* get(WatchMap* map, int wd)
{
    for (int i = 0; i < map->count; i++)
    {
        if (map->watchers[i].wd == wd)
        {
            return map->watchers[i].path;
        }
    }
    return NULL;  // jesli niczego nie znajdzie zwraca nulla, do obslugi
}

void map_free(WatchMap* map)
{
    for (int i = 0; i < map->count; i++)
    {
        if (map->watchers[i].path)
            free(map->watchers[i].path);
    }
    free(map->watchers);
}

bool check_prefix(char* prefix, char* path)
{
    if (strncmp(path, prefix, strlen(prefix)) == 0 && (path[strlen(prefix)] == '/' || path[strlen(prefix)] == '\0'))
    {
        return true;
    }
    return false;
}

void map_remove(WatchMap* map, char* path)
{
    for (int i = 0; i < map->count; i++)
    {
        if (check_prefix(path, map->watchers[i].path))
        {
            free(map->watchers[i].path);
            inotify_rm_watch(map->inotify_fd, map->watchers[i].wd);
            map->watchers[i] = map->watchers[map->count - 1];
            map->count--;
            i--;
        }
    }
}
