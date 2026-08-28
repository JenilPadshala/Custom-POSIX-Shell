#include "../include/prompt.h"
#include "../include/utils.h"
#include "../include/pipeline.h"
#include "../include/processes.h"
#include "../include/redirection.h"
#include "../include/history.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <new>

int main() {
    init_shell();
    init_process_handling();

    // buffer for user input
    char input_buffer[4096];

    while (true) {
        display_prompt();
        // get user input
        // if logout, print a newline and break (Ctrl+D)
        if (std::fgets(input_buffer, sizeof(input_buffer), stdin) == nullptr) {
            write(1, "\n", 1);
            break;
        }
        
        // strip trailing '\n' so that tokenization does not treat \n as part of the last argument.
        size_t len = std::strlen(input_buffer);
        if (len > 0 && input_buffer[len - 1] == '\n') {
            input_buffer[len - 1] = '\0';
        }

        // if empty line, reprint the prompt
        if (std::strlen(input_buffer) == 0) {
            continue;
        }

        // add the command to the history
        add_to_history(input_buffer);

        // split on ';' first so that "cmd1; cmd2" runs as two independent commands in one line.
        int command_count = 0;
        char** commands = tokenize(input_buffer, ";", command_count);

        // if no commands, continue
        if (commands == nullptr) continue;

        // process each command
        for (int i = 0; i < command_count; ++i) {
            execute_pipeline(commands[i]);
            // int arg_count = 0;
            // // split each command on spaces/tabs so extra whitespace is ignored (e.g. "ls    -l" still becomes argv ["ls", "-l"]).
            // char** args = tokenize(commands[i], " \t", arg_count);

            // // if command is not empty, process it
            // if (args != nullptr && arg_count > 0) {

            //     // TEMPORARY: print parsed command so tokenizer can be checked
            //     // char debug_msg[1024];
            //     // int debug_len = std::snprintf(debug_msg, sizeof(debug_msg), "Command to execute: %s (Total args: %d)\n", args[0], arg_count);
            //     // if (debug_len > 0) write(1, debug_msg, static_cast<size_t>(debug_len));
            //     // try to execute builtin CMDs
            //     // if(!execute_builtin(args, arg_count)){
            //     //     // not a builtin cmd, so use redirection to handle the command; execute_system_command moved into redirection.cpp
            //     //     execute_with_redirection(args, arg_count);
            //     // }
            //     execute_with_redirection(args, arg_count);
            // }

            // // free the args array returned by tokenize
            // if (args != nullptr) {
            //     delete[] args;
            // }
        }
        delete[] commands;
    }

    return 0;
}
