#include "../include/search.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>

// helper function to recursively search directories
bool search_recursive(const char* current_path, const char* target) {
    DIR* dir = opendir(current_path);
    // if can open dir, print error and return false
    if (dir == nullptr) {
        std::fprintf(stderr, "search: cannot open directory: %s\n", current_path);
        return false;
    }

    // read each entry in the directory
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // skip "." and ".." to prevent infinite recursive loops
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // check if the current entry matches the target name
        if (std::strcmp(entry->d_name, target) == 0) {
            closedir(dir);
            return true;
        }

        // construct the full path of the current entry to check if it's a directory
        char next_path[2048];
        int len = std::snprintf(next_path, sizeof(next_path), "%s/%s", current_path, entry->d_name);
        if (len < 0 || (size_t)len >= sizeof(next_path)) {
            continue; // skip if the path is too long for our buffer
        }

        struct stat statbuf;
        if (stat(next_path, &statbuf) == 0) {
            // if the entry is a directory, recurse into it
            if (S_ISDIR(statbuf.st_mode)) {
                if (search_recursive(next_path, target)) {
                    closedir(dir);
                    return true;
                }
            }
        }
    }

    closedir(dir);
    return false;
}

void execute_search(char** args, int arg_count) {
    if (arg_count != 2) {
        std::fprintf(stderr, "search: invalid arguments\n");
        return;
    }

    const char* target = args[1];
    
    // start the search from the current directory (".")
    bool found = search_recursive(".", target);

    if (found) {
        std::printf("True\n");
    } else {
        std::printf("False\n");
    }
}