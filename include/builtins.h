#ifndef BUILTINS_H
#define BUILTINS_H

// Executes built-in commands. Returns true if the command was a built-in and handled, false otherwise.
bool execute_builtin(char** args, int arg_count);

#endif