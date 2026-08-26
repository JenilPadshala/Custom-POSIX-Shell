#ifndef REDIRECTION_H
#define REDIRECTION_H

// Parses redirection operators, sets up dup2, and executes the command
void execute_with_redirection(char** args, int arg_count);

#endif