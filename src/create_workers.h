#ifndef SOP_PROJEKT1_CREATE_WORKERS_H
#define SOP_PROJEKT1_CREATE_WORKERS_H
#include "create_copy.h"
#include "watch_map.h"
#define MAX_WORKERS 128
extern volatile sig_atomic_t keep_running;
typedef struct
{
    pid_t pid;
    char* src;
    char* dst;
    bool active;
} Job;

void create_workers(char*, char*);
void work(char*, char*);
void add_job_to_list(pid_t, char*, char*);
pid_t find_and_turn_off_jobs(char*, char*);
int is_job_running(char*, char*);

#endif  // SOP_PROJEKT1_CREATE_WORKERS_H

