# Assignment 2: POSIX Shell Implementation
Roll number: 2026202010

## Brief Overview:
In this assignment I have implemented a custom POSIX Shell in C++. The shell supports the following features:
- Built-in commands: cd, echo, pwd, ls with flags support
- System commands using execvp (with option to run them in background or foreground)
- pinfo: command to print information about a process
- search: command to search for a file/directory under the current directory recursively.
- I/O redirection: <, >, >>
- Piping: |
- Redirection with Piping
- Signals: Ctrl+Z, Ctrl+C, Ctrl+D
- Autocomplete using Tab key
- History: history (persistent across sessions)
- Up/Down arrows to recall and edit history
## File Mapping:
Following are the files corresponding to each of the features:
| Sr No | Feature                                                                                | Files                                                                                                                                   |
| ----- | -------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| 1     | Prompt (`username@hostname:cwd>`), `~` for shell home, extra spaces/tabs               | `include/prompt.h`, `src/prompt.cpp`; tokenization in `include/utils.h`, `src/utils.cpp`; REPL loop and `;` splitting in `src/main.cpp` |
| 2     | Built-ins `cd`, `echo`, `pwd`                                                          | `include/builtins.h`, `src/builtins.cpp`                                                                                                |
| 3     | Built-in `ls`                                                                          | `include/ls.h`, `src/ls.cpp` (dispatched from `builtins.cpp`)                                                                           |
| 4     | Foreground and background system commands (`fork` / `execvp` / `waitpid`)              | `include/processes.h`, `src/processes.cpp`                                                                                              |
| 5     | `pinfo` / `pinfo <pid>`                                                                | `include/pinfo.h`, `src/pinfo.cpp` (routed through `builtins.cpp`)                                                                      |
| 6     | `search`                                                                               | `include/search.h`, `src/search.cpp`(routed through `builtins.cpp`)                                                                    |
| 7     | I/O redirection `<`, `>`, `>>`                                                         | `include/redirection.h`, `src/redirection.cpp`(dispatched from `pipeline.cpp`)                                                         |
| 8     | Pipelines                                                                              | `include/pipeline.h`, `src/pipeline.cpp`                                                                                                |
| 9     | Redirection with pipeline                                                              | `src/pipeline.cpp` calls `execute_with_redirection()` per segment (`src/redirection.cpp`)                                               |
| 10    | Signals: Ctrl+Z (stop FG), Ctrl+C (`SIGINT` to FG), Ctrl+D (logout of this shell only) | Ctrl+Z / Ctrl+C: `src/processes.cpp`; Ctrl+D: `src/raw_input.cpp`                                                                       |
| 11    | TAB autocomplete for commands and names in the current directory                       | `include/raw_input.h`, `src/raw_input.cpp`                                                                                              |
| 12    | `history`                                                                              | `include/history.h`, `src/history.cpp`; arrow keys in `src/raw_input.cpp`                                                               |
## Features Implementation Overview:
### 1. Prompt:
This module retrieves the system hostname and username dynamically using `gethostname()` and `getpwuid(getuid())`. 
- It tracks the directory from which the shell was invoked and uses it as the base "home" directory, substituting this path with `~` in the prompt output for a cleaner interface. 
- Multiple commands separated by `;` are parsed and handled sequentially.
- Sample output: `<jenilpadshala@Jenils-MacBook-Pro-3.local:~>`
### 2. Built-ins:
This module implements the built-in commands `cd`, `echo`, `pwd`.
- `cd`: Implemented using `chdir()` system call. It handles absolute paths, relative paths, and special cases like `cd ~`, `cd -`, and `cd ..`.
- `echo`: Iterates through the tokenized arguments and prints them to the console (fd = 1 /  STDOUT_FILENO).
- `pwd`: Uses `getcwd()` to retrieve the current working directory and prints it to the console.
### 3. ls Command:
Implemented using `<dirent.h>` and `<sys/stat.h>`. It opens directories using `opendir()` and reads entries with `readdir()`.
- `-a` **flag**: Bypasses the default filter that ignores hidden files (files starting with `.`).
- `-l` **flag**: Uses `stat()` to fetch detailed file metadata (permissions, links, owner, group, size, and modification time). 
- Dispatched through `execute_builtin()` in `builtins.cpp`.
### 4. System commands (background/foreground), with and without arguments:
Non-builtin commands are dispatched and executed using `fork()` and `execvp()`.
- Foreground processes: `waitpid()` is used to wait for the process to complete.
- Background processes: Placed in a separate process group using `setpgid(0, 0)`.
- Prints the PID of the process when run in background.
- Background termination is managed asynchronously via a `SIGCHLD` signal handler.
- Multiple background jobs are supported.
### 5. pinfo Command:
Because macOS lacks the linux `/proc` virtual filesystem, in this module I have utilized `<libproc.h>` C API to fetch process data dynamicallyl.
- It uses `proc_pidinfo()` with `PROC_PIDTASKALLINFO` to get the process state and virtual memory size.
- It uses `proc_pidpath()` to resolve and fetch the absolute path of the executable.
- If no PID is provided, it uses `getpid()` to get the PID of the current instance of the shell and display its information.
### 6. search Command:
Implemented using `opendir()` and `readdir()` to traverse the directory tree recursively.
- If a subdirectory is found, it recursively calls itself to search for the file/directory.
- It prints `True` if the file/directory is found, otherwise `False`.
### 7. I/O Redirection:
It uses `open()`,`close()`, `dup()`, and `dup2()` to manipulate file descriptors before executing a command.
- `<`: Opens the specified file in `O_RDONLY` mode and duplicates it to STDIN_FILENO.
- `>`: Opens the specified file in `O_WRONLY | O_CREAT | O_TRUNC` mode and duplicates it to STDOUT_FILENO.
- `>>`: Opens the specified file in `O_WRONLY | O_CREAT | O_APPEND` mode and duplicates it to STDOUT_FILENO.
### 8. Pipelines & 9. Redirection with pipeline:
The pipeline manager splits the input by `|`. It iterates through each command segment, creating pipes using the `pipe()` system call.
- `dup2()` is used to chain the `STDOUT_FILENO` of the current process to the write-end of the pipe, and the `STDIN_FILENO` of the next process to the read-end.
- Because redirection is evaluated per-segment, commands like `cat < in.txt | grep "A" > out.txt` are handled without any issues by applying I/O redirection sequentially within the child processes before `execvp()`.
### 10. Signals:
I have used `sigaction()` to override the default behaviour of the following signals:
- `Ctrl+C`(`SIGINT`) & `Ctrl+Z`(`SIGTSTP`): Intercepted by the shell to prevent it from dying.
  - `SIGINT` is sent to interrupt the current foreground process.
  - `SIGTSTP` is sent to push the current foreground process to the background.
`Ctrl+D` (`EOF`): Logs out of the shell only. Captured within the raw input loop in `src/raw_input.cpp` as an ASCII value of 4.
### 11. TAB autocomplete:
The shell operates in Non-Canonical (RAW) mode via `<termios.h>` to intercept keypresses instantly.
So when the user presses the TAB key (ASCII value 9):
- The shell extracts the current string prefix.
- It scans the current directory using `opendir()` and collects matches.
- If utilizes Longest Common Prefix (LCP) logic to auto-fill the text. If multiple matches exist, it lists them for the user to choose from.
### 12. History & Arrow Key Navigation:
- Commands are logged to a `.shell_history` file using `fopen()`, `fprintf()`, and `fclose()` (stores at max 20 commands).
- The `history <num>` command is implemented to display the last `num` commands from the history (capped at max 10).
- Up/Down Arrows: 
  - The raw mode engine listens for ANSI escape sequences (`\033[A` and `\033[B`). 
  - It cycles through a loaded array of recent commands, utilizing the `\33[2K\r` escape sequence to dynamically clear the terminal line and redraw the prompt with the requested history state without generating new terminal lines.