#include "../include/builtins.h"
#include "../include/prompt.h"
#include "../include/ls.h"
#include "../include/pinfo.h"
#include "../include/search.h"
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// Stores the previous directory for the 'cd -' command
char prev_dir[1024] = ""; 

// builtin echo command
void custom_echo(char** args, int arg_count) {
    // start at idx 1 to skip "echo"
    for (int i = 1; i < arg_count; ++i) {
        write(1, args[i], std::strlen(args[i]));
        // print a space between arguments, but not after the last one
        if (i < arg_count - 1) {
            write(1, " ", 1);
        }
    }
    write(1, "\n", 1);
}

// builtin pwd command
void custom_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        write(1, cwd, std::strlen(cwd));
        write(1, "\n", 1);
    } else {
        perror("pwd error");
    }
}

// builtin cd command
void custom_cd(char** args, int arg_count) {
    // if more than one argument, print an error
    if (arg_count > 2) {
        const char* err = "Invalid arguments\n";
        write(2, err, std::strlen(err));
        return;
    }

    // save current working directory before attempting to change
    char current_cwd[1024];
    if (getcwd(current_cwd, sizeof(current_cwd)) == nullptr) {
        perror("cd error: getcwd failed");
        return;
    }

    const char* target_dir = nullptr;

    // handle 'cd' with no arguments or 'cd ~'
    if (arg_count == 1 || std::strcmp(args[1], "~") == 0) {
        target_dir = shell_home;
    } 
    // handle 'cd -' to switch to previous directory
    else if (std::strcmp(args[1], "-") == 0) {
        if (std::strlen(prev_dir) == 0) {
            target_dir = shell_home;
        } else {
            target_dir = prev_dir;
        }
        // standard behavior for `cd -` is to print the directory it switches to
        write(1, target_dir, std::strlen(target_dir));
        write(1, "\n", 1);
    } 
    // Handle specific paths ('.', '..', or absolute/relative paths)
    else {
        target_dir = args[1];
    }

    if (target_dir != nullptr) {
        if (chdir(target_dir) == 0) {
            // If chdir was successful, update prev_dir
            std::strncpy(prev_dir, current_cwd, sizeof(prev_dir) - 1);
        } else {
            perror("cd");
        }
    }
}

// execute builtin command
bool execute_builtin(char** args, int arg_count) {
    if (args == nullptr || args[0] == nullptr) {
        return false;
    }

    if (std::strcmp(args[0], "echo") == 0) {
        custom_echo(args, arg_count);
        return true;
    } else if (std::strcmp(args[0], "pwd") == 0) {
        custom_pwd();
        return true;
    } else if (std::strcmp(args[0], "cd") == 0) {
        custom_cd(args, arg_count);
        return true;
    } else if (std::strcmp(args[0], "ls") == 0) {
        execute_ls(args, arg_count);
        return true;
    } else if (std::strcmp(args[0], "pinfo") == 0) {
        execute_pinfo(args, arg_count);
        return true;
    } else if (std::strcmp(args[0], "search") == 0) {
        execute_search(args, arg_count);
        return true;
    }

    return false; // if not a built-in command, return false
}