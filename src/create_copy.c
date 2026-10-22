#include "create_copy.h"

int copy_file(char* src_path, char* dest_path)
{
    int in, out;
    char buf[MAX_BUF];
    struct stat sa;

    if (stat(src_path, &sa) == -1)
    {
        ERR("stat fail");
    }

    if ((in = open(src_path, O_RDONLY, 0777)) < 0)
    {
        fprintf(stderr, "failed to open %s", src_path);
        exit(EXIT_FAILURE);
    }
    if ((out = open(dest_path, O_CREAT | O_WRONLY | O_TRUNC, 0777)) < 0)
    {
        ERR("open fail");
    }
    int count;
    while ((count = read(in, buf, MAX_BUF)) > 0)
    {
        if (write(out, buf, count) < count)
            ERR("write fail");
    }
    if (fchmod(out, sa.st_mode) == -1)
    {
        ERR("fchmod failed");
    }

    if (close(in) < 0)
        ERR("close fail");
    if (close(out) < 0)
        ERR("close fail");
    return 1;
}

void set_abs_path(char* src_path, char* dest_path)
{
    if (realpath(src_path, abs_src_path) == NULL)
        ERR("realpath failed");

    if (dest_path[0] == '/')
        strncpy(abs_dest_path, dest_path, MAX_PATH);
    else
    {
        char cwd[MAX_PATH];
        if (getcwd(cwd, MAX_PATH) == NULL)
            ERR("getcwd failed");

        snprintf(abs_dest_path, sizeof(abs_dest_path), "%s/%s", cwd, dest_path);
    }
}

void copy_symlink(char* src_path, char* dest_path)
{
    char link_target[MAX_PATH];
    char new_target[MAX_PATH * 2 + 1];
    ssize_t len;
    if ((len = readlink(src_path, link_target, sizeof(link_target) - 1)) < 0)
        ERR("readlink failed");

    link_target[len] = '\0';

    if (link_target[0] == '/' && strncmp(link_target, abs_src_path, strlen(abs_src_path)) == 0)
    {
        char* suffix = link_target + strlen(abs_src_path);

        snprintf(new_target, sizeof(new_target), "%s/%s", abs_dest_path, suffix);
    }
    else
    {
        strncpy(new_target, link_target, MAX_PATH);
    }
    if (symlink(new_target, dest_path) == -1)
        ERR("symlink fail");
}

void copy_dir(char* src_path, char* dest_path, WatchMap* map)
{
    DIR* dir;
    struct dirent* ent;
    struct stat statbuf;
    char full_src_path[MAX_PATH];
    char full_dest_path[MAX_BUF];

    if ((dir = opendir(src_path)) == NULL)
        ERR("nie da sie otworzyc folderu");

    if (lstat(src_path, &statbuf) == -1)
        ERR("lstat failed");

    if (map != NULL)
    {
        map_add(map, src_path);
    }

    if (mkdir(dest_path, statbuf.st_mode) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkdir failed");
            exit(EXIT_FAILURE);
        }
    }
    chmod(dest_path, statbuf.st_mode);

    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        snprintf(full_src_path, sizeof(full_src_path), "%s/%s", src_path, ent->d_name);
        snprintf(full_dest_path, sizeof(full_dest_path), "%s/%s", dest_path, ent->d_name);

        if (lstat(full_src_path, &statbuf) == -1)
            ERR("lstat failed");

        if (S_ISDIR(statbuf.st_mode))
        {
            copy_dir(full_src_path, full_dest_path, map);
        }
        if (S_ISREG(statbuf.st_mode))
        {
            copy_file(full_src_path, full_dest_path);
        }
        if (S_ISLNK(statbuf.st_mode))
        {
            copy_symlink(full_src_path, full_dest_path);
        }
    }
    closedir(dir);
}

void recursive_delete(char* path)
{
    DIR* dir;
    struct dirent* ent;
    struct stat statbuf;
    char full_path[MAX_PATH];

    if ((dir = opendir(path)) == NULL)
        ERR("nie da sie otworzyc folderu");

    while ((ent = readdir(dir)) != NULL)
    {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);

        if (lstat(full_path, &statbuf) == -1)
            ERR("lstat failed");
        if (S_ISDIR(statbuf.st_mode))
        {
            recursive_delete(full_path);
        }
        if (S_ISREG(statbuf.st_mode))
        {
            unlink(full_path);
        }
        if (S_ISLNK(statbuf.st_mode))
        {
            unlink(full_path);
        }
    }
    closedir(dir);
    if (rmdir(path) == -1)
        ERR("rmdir failed");
}

int is_valid_target(char* path)
{
    struct stat st;
    if (lstat(path, &st) == -1)
    {
        if (errno == ENOENT)
            return 1;
        perror("lstat failed");
        return -1;
    }
    if (S_ISDIR(st.st_mode))
    {
        DIR* d = opendir(path);
        if (d == NULL)
        {
            perror("opendir failed");
            return -1;
        }
        int hasfile = 0;
        struct dirent* ent;
        while ((ent = readdir(d)) != NULL)
        {
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0)
                hasfile = 1;
        }
        closedir(d);
        if (hasfile == 0)
            return 1;
        return 0;
    }
    perror("ścieżka docelowa istnieje oraz nie jest pustym folderem, skonsultuj się z lekarzem lub farmaceutą\n");
    return 0;
}
