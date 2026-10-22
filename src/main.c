#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "create_copy.h"
#include "create_workers.h"
#include "parse_command.h"
#include "watch_map.h"
char abs_src_path[MAX_PATH];
char abs_dest_path[MAX_PATH * 2];
Job jobs[MAX_WORKERS];

void handle_signal(int sig) { keep_running = 0; }
// hangle sigchld
void handle_sigchld(int sig)
{
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
    {
        for (int i = 0; i < MAX_WORKERS; i++)
        {
            if (jobs[i].active && jobs[i].pid == pid)
            {
                jobs[i].active = false;
                break;
            }
        }
    }
}

void clean_after_child()
{  // zeby nie bylo race condition musze ustawic sigchld normalnie, inaczej sobie wylacze sprawdzanie smierci
    // dziecka powyzsza funkcja
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigaction(SIGCHLD, &sa, NULL);
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        if (jobs[i].active)
        {
            kill(jobs[i].pid, SIGTERM);
            waitpid(jobs[i].pid, NULL, 0);

            if (jobs[i].src)
                free(jobs[i].src);
            if (jobs[i].dst)
                free(jobs[i].dst);
            jobs[i].active = false;
        }
    }
}

void set_handler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;

    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    struct sigaction child_death;
    memset(&child_death, 0, sizeof(child_death));
    child_death.sa_handler = handle_sigchld;
    child_death.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &child_death, NULL);

    struct sigaction ignore;
    memset(&ignore, 0, sizeof(ignore));
    ignore.sa_handler = SIG_IGN;

    for (int i = 0; i < NSIG; i++)
    {
        if (i != SIGTERM && i != SIGINT && i != SIGCHLD)
        {
            sigaction(i, &ignore, NULL);
        }
    }
}

void usage()
{
    printf("\n");
    printf("\tadd <src path> <target path>\n");
    printf("\tend <src path> <target path>\n");
    printf("\trestore <src path> <target path>\n");
    printf("\tlist\n");
    printf("\texit\n");
}

int main(int argc, char** argv)
{
    char* command;
    char* command_args[MAX_COMMAND_ARRAY];
    int count;
    set_handler();

    while (keep_running)
    {
        printf("> ");
        fflush(stdout);
        command = wait_for_input(MAX_CLI_INPUT);
        if (command == NULL)
        {
            if (errno == EINTR)
                continue;
            keep_running = 0;
        }

        if ((count = create_command_array(command, command_args)) <= 0)
        {
            free(command);
            continue;
        }

        if (strcmp(command_args[0], "add") == 0)
        {
            if (count < 3)
                usage();
            else
            {
                char abs_src[MAX_PATH + 1];
                bool verify = true;
                if (realpath(command_args[1], abs_src) == NULL)
                {
                    perror("Sciezka zrodlowa nie istnieje\n");
                    verify = false;
                }
                char** raw_targets = command_args + 2;
                make_path_absolute(command_args[1], abs_src);

                struct stat st;
                if (lstat(abs_src, &st) == 0)
                {
                    if (!S_ISDIR(st.st_mode))
                    {
                        perror("Sciezka zrodlowa musi byc folderem a nie plikiem\n");
                        verify = false;
                    }
                }
                else
                {
                    perror("lstat sie nie powiodl\n");
                    verify = false;
                }
                if (verify)
                {
                    for (int i = 0; i < count - 2; i++)
                    {
                        char abs_target[MAX_PATH + 1];
                        make_path_absolute(raw_targets[i], abs_target);

                        if (strncmp(abs_src, abs_target, strlen(abs_src)) == 0)
                        {
                            char check = abs_target[strlen(abs_src)];
                            if (check == '/' || check == '\0')
                            {
                                printf("Nie mozesz stworzyc kopii w katalogu zrodlowym\n");
                                continue;
                            }
                        }

                        if (is_job_running(abs_src, abs_target) == 1)
                        {
                            printf("%s juz monitoruje %s\n", abs_target, abs_src);
                            continue;
                        }

                        create_workers(abs_src, abs_target);
                    }
                }
            }
        }
        else if (strcmp(command_args[0], "list") == 0)
        {
            printf("Aktywne procesy:\n");
            for (int i = 0; i < MAX_WORKERS; i++)
            {
                if (jobs[i].active)
                    printf("[%d]\t%s ---->%s\n", jobs[i].pid, jobs[i].src, jobs[i].dst);
            }
        }
        else if (strcmp(command_args[0], "end") == 0)
        {

            char abs_src[MAX_PATH];
            make_path_absolute(command_args[1], abs_src);
            char** target = command_args + 2;

            for (int i = 0; i < count - 2; i++)
            {
                char abs_target[MAX_PATH];
                make_path_absolute(target[i], abs_target);
                pid_t pid = find_and_turn_off_jobs(abs_src, abs_target);
                if (pid == 0)
                {
                    perror("nie znaleziono podanego procesu\n");
                    continue;
                }
                if (pid != -1)
                {
                    if (kill(pid, SIGTERM) < 0)
                    {
                        perror("Nie udalo sie wyslac sygnalu\n");
                        continue;
                    }
                }
            }
        }
        else if (strcmp(command_args[0], "restore") == 0)
        {
            for (int i = 0; i < count - 2; i++)
            {
                char** target = command_args + 2;
                if (is_job_running(command_args[1], target[i]) == 0)
                {
                    recursive_delete(command_args[1]);
                    copy_dir(target[i], command_args[1], NULL);
                }
                else
                {
                    printf("Nie mozesz przywrocic aktywnej kopii, wpierw wykonaj end %s %s\n", command_args[1],
                           target[i]);
                }
            }
        }
        else if (strcmp(command_args[0], "exit") == 0)
        {
            keep_running = 0;
        }
        else
        {
            printf("Twoja komenda jest niepoprawna, instrukcja obslugi:\n");
            usage();
        }
        free(command);
        command = NULL;
    }
    clean_after_child();
    if (command)
        free(command);
    return 0;
}
