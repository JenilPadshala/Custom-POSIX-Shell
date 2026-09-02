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

// handle SIGINT (Ctrl+C)
void sigint_handler(int) {
    // check if there is a valid fg prcs running
    if(current_fg_pid != -1) {
        // send SIGINT to the current fg prcs
        kill(current_fg_pid, SIGINT);
        write(1, "\n", 1);
    } else {
        // if no prcs running, just print a new line and a fresh prompt
        write(1, "\n", 1);
        display_prompt();
        fflush(stdout);
    }
}

//!!! handle SIGTSTP (Ctrl+Z)
void sigtstp_handler(int) {
    // check if there is a valid fg prcs running
    if(current_fg_pid != -1) {
        // send SIGTSTP to suspend the current fg prcs
        kill(current_fg_pid, SIGTSTP);
        write(1, "\n", 1);
    } else {
        // if no prcs running, just print a new line and a fresh prompt
        write(1, "\n", 1);
        display_prompt();
        fflush(stdout);
    }
}

// initialize prcs handling
void init_process_handling() {
    // create separate sigaction structs for each signal
    struct sigaction sa_chld, sa_int, sa_tstp;

    // setup SIGCHLD handler
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if(sigaction(SIGCHLD, &sa_chld, nullptr) == -1){
        perror("Error: sigaction failed");
    }

    // setup SIGINT (Ctrl+C) handler
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, nullptr) == -1) {
        perror("Error: sigaction failed for SIGINT");
    }

    // setup SIGTSTP (Ctrl+Z) handler
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    if (sigaction(SIGTSTP, &sa_tstp, nullptr) == -1) {
        perror("Error: sigaction failed for SIGTSTP");
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

    // mask SIGCHLD
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    // fork a new process
    pid_t pid = fork();

    if(pid < 0) {
        perror("Error: fork failed");
        return;
    }
    if (pid == 0) {
        // child process code
        // unblock SIGCHLD
        sigprocmask(SIG_UNBLOCK, &mask, nullptr);
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
            // unblock SIGCHLD
            sigprocmask(SIG_UNBLOCK, &mask, nullptr);
        }
        else {
            // set current_fg_pid to the pid of the foreground process
            current_fg_pid = pid;
            // unblock SIGCHLD
            sigprocmask(SIG_UNBLOCK, &mask, nullptr);
            int status;
            waitpid(pid, &status, WUNTRACED);
            // reset current_fg_pid once the fg process is terminated
            current_fg_pid = -1;
        }
    }
}