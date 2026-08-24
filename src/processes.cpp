#include "../include/prompt.h"
#include <unistd.h>
#include <sys/wait.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <signal.h>

pid_t current_fg_pid = -1;

// handle SIGCHLD signal (child process termination)
void sigchld_handler(int) {
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // if the currently terminate prcs was an fg, do not print anything
        if(pid == current_fg_pid){
            continue;
        }
        printf("\nTerminated bg process with PID: [%d]\n", pid);
        display_prompt();
        // flush stdout to ensure prompt is displayed
        fflush(stdout);
    }
}

// initialize process handling
void init_process_handling() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if(sigaction(SIGCHLD, &sa, nullptr) == -1){
        perror("Error: sigaction failed");
    }
}

// execute a system command
void execute_system_command(char** args, int argc) {
    if(argc == 0 || args[0] == nullptr)
        return;

    bool is_background = false;

    // check if the last argument is & (background process)
    if(strcmp(args[argc - 1], "&") == 0) {
        is_background = true;
        args[argc - 1] = nullptr; // remove the & from the arguments so that it is not passed to execvp
    }
    // fork a new process
    pid_t pid = fork();

    if(pid < 0) {
        perror("Error: fork failed");
        return;
    }
    if (pid == 0) {
        // child process code

        // if it is background process, put it in its own process group
        if(is_background) {
            setpgid(0, 0);
        }

        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "Error: command not found: '%s'\n", args[0]);
            exit(1);
        }
    }
    else {
        // parent process code
        if(is_background) {
            // print pid of background process
            printf("Started bg process with PID: [%d]\n", pid);
        }
        else {
            // set current_fg_pid to the pid of the foreground process
            current_fg_pid = pid;
            int status;
            waitpid(pid, &status, WUNTRACED);
            // reset current_fg_pid once the fg process is terminated
            current_fg_pid = -1;
        }
    }
}