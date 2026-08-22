#include "../include/prompt.h"
#include <unistd.h>
#include <pwd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>


// global vars for prompt display
char shell_home[1024];
char username[1024];
char system_name[1024];

void init_shell() {
    // get username
    uid_t uid = geteuid();
    // get passwd struct for username
    struct passwd *pw = getpwuid(uid);
    if (pw) {
        std::strncpy(username, pw->pw_name, sizeof(username) - 1);
    } else {
        std::strncpy(username, "unknown", sizeof(username) - 1);
    }
    // get system name
    if (gethostname(system_name, sizeof(system_name) - 1) == -1) {
        std::strncpy(system_name, "unknown", sizeof(system_name) - 1);
    }   
    // get shell home dir (current dir)
    if (getcwd(shell_home, sizeof(shell_home)) == nullptr) {
        perror("Error: Failed to get shell home directory");
    }
}

void display_prompt() {
    // get current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        perror("Error: Failed to get current directory");
        return;
    }

    // display path
    char display_path[1024];
    // get length of shell home
    size_t home_len = std::strlen(shell_home);

    // if cwd is under shell_home, show '~' + the remainder (e.g. ~/src).
    // strncmp is used to compare the first home_len characters of cwd and shell_home.
    if (std::strncmp(cwd, shell_home, home_len) == 0) {
        display_path[0] = '~';
        std::strncpy(display_path + 1, cwd + home_len, sizeof(display_path) - 2);
        display_path[sizeof(display_path) - 1] = '\0';
    } 
    // if cwd is not under shell_home, show the full path
    else {
        std::strncpy(display_path, cwd, sizeof(display_path) - 1);
        display_path[sizeof(display_path) - 1] = '\0';
    }

    // display format: <user@host:path> 
    char prompt_msg[2048];
    int len = std::snprintf(prompt_msg, sizeof(prompt_msg), "<%s@%s:%s> ", username, system_name, display_path);
    if (len > 0) {
        write(1, prompt_msg, static_cast<size_t>(len));
    }
}
