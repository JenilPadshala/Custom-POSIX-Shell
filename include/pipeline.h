#ifndef PIPELINE_H
#define PIPELINE_H

// parses a raw command string for pipe operators, sets up pipe fds, and executes
void execute_pipeline(char* raw_command);

#endif