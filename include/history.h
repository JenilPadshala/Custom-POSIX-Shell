#ifndef HISTORY_H
#define HISTORY_H

// append a raw command string to the history file
void add_to_history(const char* command);

// execute the history built-in command (print the last 10-20 commands)
void execute_history(char** args, int arg_count);

#endif