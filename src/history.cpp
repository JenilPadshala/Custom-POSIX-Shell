#include "../include/history.h"
#include "../include/prompt.h" // We need this to get shell_home
#include <cstdio>
#include <cstring>
#include <cstdlib>



// helper to get the absolute path to shell_history
void get_history_file_path(char* path, size_t max_len) {
    std::snprintf(path, max_len, "%s/.shell_history", shell_home);
}

// helper to add command to shell_history
void add_to_history(const char* command) {
    // avoid logging empty commands
    if (command == nullptr || std::strlen(command) == 0) return;

    char path[1024];
    get_history_file_path(path, sizeof(path));

    // open in append mode to add to the end of shell_history
    FILE* file = std::fopen(path, "a");
    if (file) {
        std::fprintf(file, "%s\n", command);
        std::fclose(file);
    }
}

// function to display the history of commands
void execute_history(char** args, int arg_count) {
    int num_to_display = 10; 

    // parse the optional <num> argument
    if (arg_count == 2) {
        char* endptr = nullptr;
        num_to_display = std::strtol(args[1], &endptr, 10);
        
        // validate that it is a +ve number
        if (*endptr != '\0' || num_to_display < 0) {
            std::fprintf(stderr, "history: invalid number '%s'\n", args[1]);
            return;
        }
    } else if (arg_count > 2) {
        std::fprintf(stderr, "history: too many arguments\n");
        return;
    }

    char path[1024];
    get_history_file_path(path, sizeof(path));

    FILE* file = std::fopen(path, "r");
    if (!file) return; // no history file yet

    // read into circular buff
    char lines[MAX_HISTORY][2048];
    int total_lines = 0;
    char buffer[2048];

    while (std::fgets(buffer, sizeof(buffer), file)) {
        size_t len = std::strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        std::strcpy(lines[total_lines % MAX_HISTORY], buffer);
        total_lines++;
    }
    std::fclose(file);

    // calculate the slice of history to print
    int available_lines = (total_lines < MAX_HISTORY) ? total_lines : MAX_HISTORY;
    
    // cap the request to what we actually have in the buffer
    if (num_to_display > available_lines) {
        num_to_display = available_lines;
    }

    // determine where the oldest line is stored in the buff
    int buffer_start = (total_lines < MAX_HISTORY) ? 0 : (total_lines % MAX_HISTORY);
    
    // skip older lines that we dont want to show
    int print_start_offset = available_lines - num_to_display; 

    // print the requested number of recent commands
    for (int i = 0; i < num_to_display; ++i) {
        int idx = (buffer_start + print_start_offset + i) % MAX_HISTORY;
        std::printf("%s\n", lines[idx]);
    }
}

// function to load history into an array and return the no. of commands loaded
int get_history_list(char out_lines[][2048]){
    char path[1024];
    get_history_file_path(path, sizeof(path));

    FILE* file = std::fopen(path, "r");
    // return 0 if no history file is found
    if (!file) return 0;

    char lines[MAX_HISTORY][2048];
    int total_lines = 0;
    char buffer[2048];

    while (std::fgets(buffer, sizeof(buffer), file)) {
        size_t len = std::strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';
        std::strcpy(lines[total_lines % MAX_HISTORY], buffer);
        total_lines++;
    }
    std::fclose(file);

    int num_avail = (total_lines < MAX_HISTORY) ? total_lines : MAX_HISTORY;
    int start_idx = (total_lines < MAX_HISTORY) ? 0 : (total_lines % MAX_HISTORY);

    for (int i = 0; i < num_avail; ++i) {
        std::strcpy(out_lines[i], lines[(start_idx + i) % MAX_HISTORY]);
    }
    
    return num_avail;
}