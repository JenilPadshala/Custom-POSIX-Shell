#ifndef HISTORY_H
#define HISTORY_H

// max number of commands that can be stored in shell_history
#define MAX_HISTORY 20

// append a raw command string to the history file
void add_to_history(const char* command);

// execute the history built-in command (print the last 10-20 commands)
void execute_history(char** args, int arg_count);

// load history into an array and return the no. of commands loaded
int get_history_list(char out_lines[][2048]);

#endif