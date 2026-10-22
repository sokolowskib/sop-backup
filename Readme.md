# sop-backup

A live directory backup tool for Linux, written in C. It is an interactive shell
that starts one worker process per backup. Each worker mirrors a source
directory into a target directory and keeps the target in sync by watching
the source tree with **inotify**.

Written for the Operating Systems course (SOP1) at the Faculty of Mathematics and
Information Science, Warsaw University of Technology.

## Features

- **Live synchronization**: create, modify, delete, rename/move and permission
  changes in the source tree show up in the backup right away.
- **Recursive watching**: every subdirectory gets its own inotify watch. A map
  of watch descriptors to paths is kept up to date as directories appear and
  disappear.
- **Multiple backups at once**: each `add` forks its own worker process, and one
  source can be backed up to several targets.
- **Symlink handling**: absolute links that point inside the source tree are
  rewritten to point at the matching place in the backup. Other links are
  copied unchanged.
- **Restore**: the source is rebuilt from a stopped backup.
- **Signal handling**: `SIGCHLD` cleans up dead workers. `SIGINT`/`SIGTERM`
  shut down all workers cleanly. Other signals are ignored.
- **Editor-friendly**: vim swap/backup files (`4913`, `*~`) are skipped.

## Build

Needs Linux (inotify) and a C17 compiler.

```sh
make            # debug build with ASan/UBSan
make CI=1       # release-style build, warnings as errors
```

## Usage

```sh
./sop-backup
```

| Command | Description |
|---|---|
| `add <src> <dst> [dst...]` | Copy `src` into each `dst` (which must be empty or missing) and start syncing it live |
| `end <src> <dst> [dst...]` | Stop syncing the given backups |
| `restore <src> <dst>` | Replace `src` with the contents of the stopped backup `dst` |
| `list` | Show the active workers (PID, source, target) |
| `exit` | Stop all workers and quit |

Paths with spaces can be wrapped in double quotes.

## How it works

```
             ┌──────────────┐
  stdin ───► │ main process │  parses commands, tracks jobs, reaps children (SIGCHLD)
             └──────┬───────┘
          fork()    │    fork()
        ┌───────────┴───────────┐
        ▼                       ▼
  ┌───────────┐           ┌───────────┐
  │  worker   │           │  worker   │   one inotify instance each;
  │ src → dst │           │ src → dst2│   blocking read() on the event queue
  └───────────┘           └───────────┘
```

1. The worker copies the whole source tree into the target and adds an inotify
   watch on every directory along the way (`IN_CREATE`, `IN_CLOSE_WRITE`,
   `IN_DELETE`, `IN_MOVED_FROM/TO`, `IN_ATTRIB`, `IN_MOVE_SELF`).
2. It then blocks on `read()` from the inotify fd. For each event it looks up the
   watch descriptor's path, works out the matching path in the target, and
   applies the change.
3. A directory created or moved in is copied recursively and watched. A
   directory deleted or moved out is removed recursively and unwatched.
4. If the source directory itself is renamed, the worker exits.

## Source layout

| File | Responsibility |
|---|---|
| `src/main.c` | Command loop, job table, signal handlers |
| `src/parse_command.c` | Input reading and tokenizing (with quotes) |
| `src/create_workers.c` | Forking workers, inotify event loop |
| `src/create_copy.c` | Recursive copy/delete of files, directories and symlinks |
| `src/watch_map.c` | Map from watch descriptors to paths |

## Testing

`test.sh` runs a series of file operations on `source/` and then uses
`diff -r` to compare it with `backup/`:

```sh
# terminal 1
./sop-backup

# terminal 2: recreates source/, then waits 3 s
./test.sh

# terminal 1, during those 3 s:
> add source backup
```
