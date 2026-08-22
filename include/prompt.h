#ifndef PROMPT_H
#define PROMPT_H

void init_shell();
void display_prompt();

// global var so that other commands can use it to get shell home dir
extern char shell_home[1024];

#endif