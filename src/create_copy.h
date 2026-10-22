#ifndef SOP_PROJEKT1_CREATE_COPY_H
#define SOP_PROJEKT1_CREATE_COPY_H
#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "watch_map.h"
#define MAX_PATH 1024
#define MAX_BUF 4096
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

extern char abs_src_path[MAX_PATH];
extern char abs_dest_path[MAX_PATH * 2];

void set_abs_path(char*, char*);
int copy_file(char*, char*);
void copy_dir(char*, char*, WatchMap*);
void copy_symlink(char*, char*);
void recursive_delete(char*);
int is_valid_target(char*);
#endif  // SOP_PROJEKT1_CREATE_COPY_H

