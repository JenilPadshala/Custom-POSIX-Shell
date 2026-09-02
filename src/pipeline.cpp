#include "../include/pipeline.h"
#include "../include/utils.h"
#include "../include/redirection.h"
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstdio>
#include <new>
#include <signal.h>

void execute_pipeline(char* raw_command) {
    int pipe_count = 0;
    // tokenize by the pipe symbol
    char** pipe_segments = tokenize(raw_command, "|", pipe_count);

    if (pipe_segments == nullptr || pipe_count == 0) return;

    // NO PIPELINE CASE: execute normally in the main shell prcs
    if (pipe_count == 1) {
        int arg_count = 0;
        char** args = tokenize(pipe_segments[0], " \t", arg_count);
        if (args != nullptr && arg_count > 0) {
            execute_with_redirection(args, arg_count);
        }
        if (args != nullptr) delete[] args;
        delete[] pipe_segments;
        return;
    }

    // PIPELINE CASE: need (N-1) pipes for N commands
    int num_pipes = pipe_count - 1;

    // allocate memory for the pipe fds to avoid VLA issues
    int* pipefds = new (std::nothrow) int[2 * num_pipes];
    if (pipefds == nullptr) {
        perror("Error: memory allocation for pipes failed");
        delete[] pipe_segments;
        return;
    }

    // initialize all the pipe fds
    for (int i = 0; i < num_pipes; ++i) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("Error: pipe creation failed");
            delete[] pipe_segments;
            return;
        }
    }

    // mask SIGCHLD
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, nullptr);


    pid_t pid;
    // iterate through each command segment in the pipeline
    for (int i = 0; i < pipe_count; ++i) {
        pid = fork();
        
        if (pid == 0) {
            // CHILD prcs
            
            // if this is NOT the first command, grab stdin from the previous pipe's read end
            if (i > 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }

            // if this is NOT the last command, route stdout to the next pipe's write end
            if (i < pipe_count - 1) {
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);
            }

            // IMP: if not closed by the child, the program will hang forever waiting for EOF
            for (int j = 0; j < 2 * num_pipes; ++j) {
                close(pipefds[j]);
            }

            // tokenize this specific segment by spaces and execute it
            int arg_count = 0;
            char** args = tokenize(pipe_segments[i], " \t", arg_count);
            
            if (args != nullptr && arg_count > 0) {
                // now handle redirection within pipes
                execute_with_redirection(args, arg_count);
            }
            
            if (args != nullptr) delete[] args;
            
            // terminate the child prcs so it doesn't return to the main shell loop
            exit(0);
            
        } else if (pid < 0) {
            perror("Error: fork failed during piping");
        }
    }

    // PARENT prcs (shell)
    // must also close all pipe fds so that the children dont hang
    for (int i = 0; i < 2 * num_pipes; ++i) {
        close(pipefds[i]);
    }

    // wait for every child prcs in the pipeline to finish
    for (int i = 0; i < pipe_count; ++i) {
        wait(nullptr);
    }

    // unblock SIGCHLD
    sigprocmask(SIG_UNBLOCK, &mask, nullptr);

    delete[] pipe_segments;
    delete[] pipefds;
}