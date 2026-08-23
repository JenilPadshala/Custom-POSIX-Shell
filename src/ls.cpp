#include "../include/ls.h"
#include "../include/prompt.h" // For resolving '~'
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <new>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>
#include <unistd.h>

#define MAX_TARGETS 128
#define MAX_FILES 1024

// get permissions for a file or directory (file/directory, user, group, others)
void get_permissions(mode_t mode, char* perms) {
    std::strcpy(perms, "----------");
    if (S_ISDIR(mode)) perms[0] = 'd';
    if (S_ISLNK(mode)) perms[0] = 'l';
    
    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';
    
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';
    
    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';
}

int compare_strings(const void* a, const void* b) {
    return std::strcmp(*(const char**)a, *(const char**)b);
}

// print long format for a file or directory (permissions, links, owner, group, size, date, name)
void print_long_format(const char* full_path, const char* display_name) {
    struct stat file_stat;

    if (stat(full_path, &file_stat) == -1) {
        perror("stat");
        return;
    }

    char perms[11];
    get_permissions(file_stat.st_mode, perms);

    struct passwd* pw = getpwuid(file_stat.st_uid);
    struct group* gr = getgrgid(file_stat.st_gid);
    const char* owner = pw ? pw->pw_name : "unknown";
    const char* group = gr ? gr->gr_name : "unknown";

    char time_buf[256];
    struct tm* tm_info = std::localtime(&file_stat.st_mtime);
    std::strftime(time_buf, sizeof(time_buf), "%b %e %H:%M", tm_info);

    std::printf("%s %ld %s %s %5ld %s %s\n", 
                perms, 
                (long)file_stat.st_nlink, 
                owner, 
                group, 
                (long)file_stat.st_size, 
                time_buf, 
                display_name);
}

void execute_ls(char** args, int arg_count) {
    bool flag_a = false;
    bool flag_l = false;
    const char* targets[MAX_TARGETS];
    int target_count = 0;

    // parse arguments for flags and target files/directories
    for (int i = 1; i < arg_count; ++i) {
        if (args[i][0] == '-') {
            for (size_t j = 1; j < std::strlen(args[i]); ++j) {
                if (args[i][j] == 'a') flag_a = true;
                else if (args[i][j] == 'l') flag_l = true;
                else {
                    std::fprintf(stderr, "ls: invalid option -- '%c'\n", args[i][j]);
                    return;
                }
            }
        }
        // not a flag, add to target array if space available
        else {
            if (target_count < MAX_TARGETS)
                targets[target_count++] = args[i];
        }
    }

    // default to current directory if no target provided
    if (target_count == 0) {
        targets[target_count++] = ".";
    }

    // process each target (file or directory)
    for (int t = 0; t < target_count; ++t) {
        const char* current_target = targets[t];
        // if '~', home directory
        if (std::strcmp(current_target, "~") == 0) {
            current_target = shell_home;
        }

        struct stat path_stat;
        if (stat(current_target, &path_stat) == -1) {
            std::fprintf(stderr, "ls: cannot access '%s': ", current_target);
            perror("");
            continue;
        }

        // Check if the target is a directory
        if (S_ISDIR(path_stat.st_mode)) {
            if (target_count > 1) {
                std::printf("%s:\n", current_target);
            }

            DIR* dir = opendir(current_target);
            if (dir == nullptr) {
                perror("ls");
                continue;
            }

            char* files[MAX_FILES];
            int file_count = 0;
            struct dirent* entry;

            while ((entry = readdir(dir)) != nullptr && file_count < MAX_FILES) {
                if (!flag_a && entry->d_name[0] == '.') {
                    continue;
                }
                files[file_count] = new (std::nothrow) char[std::strlen(entry->d_name) + 1];
                if (files[file_count]) {
                    std::strcpy(files[file_count], entry->d_name);
                    file_count++;
                }
            }
            closedir(dir);

            // sort files alphabetically
            qsort(files, file_count, sizeof(char*), compare_strings);

            for (int i = 0; i < file_count; ++i) {
                if (flag_l) {
                    // if -l flag, print long format
                    char full_path[2048];
                    std::snprintf(full_path, sizeof(full_path), "%s/%s", current_target, files[i]);
                    print_long_format(full_path, files[i]);
                } else {
                    std::printf("%s\n", files[i]);
                }
                delete[] files[i];
            }
            if (t < target_count - 1) std::printf("\n");

        } else {
            // target is a file, not a directory
            if (flag_l) {
                print_long_format(current_target, current_target);
            } else {
                std::printf("%s\n", current_target);
            }
        }
    }
}