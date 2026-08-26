#include "../include/redirection.h"
#include "../include/builtins.h"
#include "../include/processes.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

// every command is first handled to check for redirection operators now.
void execute_with_redirection(char** args, int arg_count) {
    if (args == nullptr || arg_count == 0) return;

    char* input_file = nullptr;
    char* output_file = nullptr;
    bool append = false;

    // Array to hold the command and arguments, stripped of redirection operators
    char* clean_args[256];
    int clean_count = 0;

    // 1. parse the arguments for redirection operators
    for (int i = 0; i < arg_count; ++i) {
        if (std::strcmp(args[i], "<") == 0) {
            // get the input filename in case of <
            if (i + 1 < arg_count) {
                input_file = args[++i];
            } else {
                std::fprintf(stderr, "Syntax error: missing input file\n");
                return;
            }
        } 
        // get output filename in case of >
        else if (std::strcmp(args[i], ">") == 0) {
            if (i + 1 < arg_count) {
                output_file = args[++i]; // get the output filename
                append = false;
            } else {
                std::fprintf(stderr, "Syntax error: missing output file\n");
                return;
            }
        }
        // get output filename in case of >> and set append to true
        else if (std::strcmp(args[i], ">>") == 0) {
            if (i + 1 < arg_count) {
                output_file = args[++i];
                append = true;
            } else {
                std::fprintf(stderr, "Syntax error: missing output file\n");
                return;
            }
        } else {
            // keep actual command arguments
            clean_args[clean_count++] = args[i];
        }
    }
    clean_args[clean_count] = nullptr; // null-terminate for execvp

    // if there is no command left (e.g., just "> file.txt"), return
    if (clean_count == 0) return;

    // 2. save original stdin and stdout so we can restore them later
    int orig_stdin = dup(STDIN_FILENO);
    int orig_stdout = dup(STDOUT_FILENO);

    // 3. handle input redirection
    if (input_file != nullptr) {
        int fd_in = open(input_file, O_RDONLY);
        if (fd_in < 0) {
            perror("Error opening input file");
            close(orig_stdin);
            close(orig_stdout);
            return;
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }

    // 4. handle output redirection
    if (output_file != nullptr) {
        // set flag for output file (separating it so that I can toggle between append and truncate)
        int flag = (append ? O_APPEND : O_TRUNC);
        
        
        int fd_out = open(output_file, O_WRONLY | O_CREAT | flag, 0644); // file permissions are 0644
        if (fd_out < 0) {
            perror("Error opening output file");
            // Restore stdin if it was modified
            dup2(orig_stdin, STDIN_FILENO);
            close(orig_stdin);
            close(orig_stdout);
            return;
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }

    // 5. execute the command (builtin / system commands)
    if (!execute_builtin(clean_args, clean_count)) {
        execute_system_command(clean_args, clean_count);
    }

    // 6. restore original stdin and stdout
    dup2(orig_stdin, STDIN_FILENO);
    dup2(orig_stdout, STDOUT_FILENO);
    
    // close the duplicates to prevent fd leaks
    close(orig_stdin);
    close(orig_stdout);
}