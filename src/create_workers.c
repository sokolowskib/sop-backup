#include "create_workers.h"
#include "create_copy.h"

volatile sig_atomic_t keep_running = 1;
void react_sigterm(int sig) { keep_running = 0; }
extern Job jobs[MAX_WORKERS];

void set_worker_signal()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = react_sigterm;
    sigaction(SIGTERM, &sa, NULL);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);
}

void create_workers(char* src_target, char* dest_target)
{
    pid_t pid;
    int target = is_valid_target(dest_target);
    if (target == 0)
    {
        printf("BŁĄD: Folder docelowy istnieje i nie jest pusty\n");
        return;
    }
    if (target == -1)
    {
        printf("BŁĄÐ: Nie można sprawdzić pliku docelowego\n");
        return;
    }
    if ((pid = fork()) < 0)
        ERR("fork fail");
    if (pid == 0)
    {
        work(src_target, dest_target);
        exit(EXIT_SUCCESS);
    }
    if (pid > 0)
    {
        add_job_to_list(pid, src_target, dest_target);
    }
}

void work(char* src_prefix, char* dest_prefix)
{
    int inotify_fd;
    if ((inotify_fd = inotify_init()) < 0)
        ERR("inotify fail");
    WatchMap map;
    map_init(&map, inotify_fd);
    copy_dir(src_prefix, dest_prefix, &map);
    struct stat st;
    set_worker_signal();
    set_abs_path(src_prefix, dest_prefix);
    while (keep_running)
    {
        char buf[sizeof(struct inotify_event) * DEFAULT_CAPACITY];
        ssize_t len = read(inotify_fd, buf, sizeof(buf));

        if (len < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            ERR("read fail");
        }
        ssize_t i = 0;

        while (i < len)
        {
            struct inotify_event* event = (struct inotify_event*)&buf[i];
            char* changed_src_path = get(&map, event->wd);
            char full_src_path[PATH_MAX];   // sciezka bezwzgledna do tego co sie zmienilo
            char full_dest_path[PATH_MAX];  // sciezka bezwzgledna do tego co zaraz zmienie

            if (event->mask & IN_IGNORED)
            {
                i += sizeof(struct inotify_event) + event->len;
                continue;
            }
            if (changed_src_path == NULL)
            {
                i += sizeof(struct inotify_event) + event->len;
                continue;
            }

            if (strcmp(event->name, "4913") == 0 || event->name[event->len - 1] == '~')
            {
                i += sizeof(struct inotify_event) + event->len;
                continue;
            }

            snprintf(full_src_path, PATH_MAX, "%s/%s", changed_src_path, event->name);
            int prefix_len = strlen(src_prefix);
            char* suffix = full_src_path + prefix_len;
            snprintf(full_dest_path, PATH_MAX, "%s/%s", dest_prefix, suffix);

            // omijanie swap file, potrzebne do mozliwosci edycji w edytorach tekstu jak vim

            if (event->mask & IN_CREATE)
            {
                if (lstat(full_src_path, &st) != -1)
                {
                    if (event->mask & IN_ISDIR)
                    {
                        mkdir(full_dest_path, st.st_mode);
                        copy_dir(full_src_path, full_dest_path, &map);
                    }
                    else
                    {
                        if (S_ISLNK(st.st_mode))
                            copy_symlink(full_src_path, full_dest_path);
                        else
                            copy_file(full_src_path, full_dest_path);
                    }
                }
            }
            if (event->mask & IN_CLOSE_WRITE)
            {
                copy_file(full_src_path, full_dest_path);
            }
            if (event->mask & IN_DELETE)
            {
                if (event->mask & IN_ISDIR)
                {
                    recursive_delete(full_dest_path);
                    map_remove(&map, full_src_path);
                }
                else
                {
                    unlink(full_dest_path);
                }
            }
            if (event->mask & IN_MOVED_FROM)
            {
                if (event->mask & IN_ISDIR)
                {
                    recursive_delete(full_dest_path);
                    map_remove(&map, full_src_path);
                }
                else
                {
                    unlink(full_dest_path);
                }
            }

            if (event->mask & IN_MOVED_TO)
            {
                if (lstat(full_src_path, &st) != -1)
                {
                    if (event->mask & IN_ISDIR)
                    {
                        mkdir(full_dest_path, st.st_mode);
                        copy_dir(full_src_path, full_dest_path, &map);
                    }
                    else if (S_ISLNK(st.st_mode))
                    {
                        copy_symlink(full_src_path, full_dest_path);
                    }
                    else
                    {
                        copy_file(full_src_path, full_dest_path);
                    }
                }
            }
            if (event->mask & IN_ATTRIB)
            {
                if (event->mask & IN_ISDIR)
                {
                    if (lstat(full_src_path, &st) != -1) {
                        chmod(full_dest_path, st.st_mode);
                    }
                }
                else
                {
                    copy_file(full_src_path, full_dest_path);
                }
            }

            if (event->mask & IN_MOVE_SELF)
            {
                printf("Zmieniono nazwe katalogu zrodlowego, koncze proces \n");
                exit(EXIT_FAILURE);
            }

            i += sizeof(struct inotify_event) + event->len;
        }
    }
    map_free(&map);
}

void init_jobs()
{
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        jobs[i].active = false;
    }
}

void add_job_to_list(pid_t pid, char* src_path, char* dest_path)
{
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        if (jobs[i].active == false)
        {
            jobs[i].pid = pid;
            jobs[i].active = true;

            jobs[i].src = strdup(src_path);
            jobs[i].dst = strdup(dest_path);
            return;
        }
    }
    ERR("all jobs busy");
}
int is_job_running(char* src_path, char* dest_path)
{
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        if (!jobs[i].active)
            continue;

        if (strcmp(jobs[i].src, src_path) == 0 && strcmp(jobs[i].dst, dest_path) == 0)
        {
            return 1;
        }
    }
    return 0;
}
pid_t find_and_turn_off_jobs(char* src_path, char* dest_path)
{
    for (int i = 0; i < MAX_WORKERS; i++)
    {
        if (!jobs[i].active)
            continue;
        if (strcmp(jobs[i].src, src_path) == 0 && strcmp(jobs[i].dst, dest_path) == 0)
        {
            jobs[i].active = false;
            if (jobs[i].src)
                free(jobs[i].src);
            if (jobs[i].dst)
                free(jobs[i].dst);
            return jobs[i].pid;
        }
    }
    return 0;
}
