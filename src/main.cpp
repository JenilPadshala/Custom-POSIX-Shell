#include "../include/prompt.h"
#include "../include/utils.h"
#include "../include/pipeline.h"
#include "../include/processes.h"
#include "../include/redirection.h"
#include "../include/history.h"
#include "../include/raw_input.h"
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
        if(!read_raw_input(input_buffer, sizeof(input_buffer))){
            write(1, "\n", 1);
            break;
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
        }
        delete[] commands;
    }

    return 0;
}
