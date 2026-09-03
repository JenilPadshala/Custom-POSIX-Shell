
#include "../include/autocomplete.h"
#include "../include/raw_input.h"
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <unistd.h>

// custom function to sort matches alphabetically
void sort_matches(char matches[][256], int count) {
    char temp[256];
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (std::strcmp(matches[j], matches[j + 1]) > 0) {
                std::strcpy(temp, matches[j]);
                std::strcpy(matches[j], matches[j + 1]);
                std::strcpy(matches[j + 1], temp);
            }
        }
    }
}

// custom function to handle autocomplete
void handle_autocomplete(char* buffer, int& pos, int max_len) {
    int word_start = pos;
    while (word_start > 0 && buffer[word_start - 1] != ' ') word_start--;
    // determine if the word is a command or a file/directory
    bool is_command = (word_start == 0);
    int token_len = pos - word_start;
    if (token_len == 0) return;

    char token[256];
    std::strncpy(token, &buffer[word_start], token_len);
    token[token_len] = '\0';

    char matches[100][256];
    int match_count = 0;

    // For non-commands, separate the directory path from the file prefix
    char search_dir[256] = ".";
    const char* file_prefix = token;

    if (!is_command) {
        char* last_slash = std::strrchr(token, '/');
        if (last_slash != nullptr) {
            int dir_len = last_slash - token;
            if (dir_len == 0) {
                // Root directory: e.g., "/pr"
                std::strcpy(search_dir, "/");
            } else {
                std::strncpy(search_dir, token, dir_len);
                search_dir[dir_len] = '\0';
            }
            file_prefix = last_slash + 1;
        }

        int prefix_len = std::strlen(file_prefix);

        DIR* dir = opendir(search_dir);
        if (dir != nullptr) {
            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr && match_count < 100) {
                if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue;
                if (std::strncmp(entry->d_name, file_prefix, prefix_len) == 0) {
                    std::strcpy(matches[match_count], entry->d_name);
                    match_count++;
                }
            }
            closedir(dir);
        }
    } else {
        int prefix_len = token_len;
        const char* path_env = std::getenv("PATH");
        if (path_env != nullptr) {
            char path_copy[4096];
            std::strncpy(path_copy, path_env, 4095);
            path_copy[4095] = '\0';

            char* saveptr;
            char* dir_path = strtok_r(path_copy, ":", &saveptr);

            while (dir_path != nullptr) {
                DIR* dir = opendir(dir_path);
                if (dir != nullptr) {
                    struct dirent* entry;
                    while ((entry = readdir(dir)) != nullptr && match_count < 100) {
                        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue;
                        if (std::strncmp(entry->d_name, token, prefix_len) == 0) {
                            bool duplicate = false;
                            for (int i = 0; i < match_count; i++) {
                                if (std::strcmp(matches[i], entry->d_name) == 0) {
                                    duplicate = true;
                                    break;
                                }
                            }
                            if (!duplicate) {
                                std::strcpy(matches[match_count], entry->d_name);
                                match_count++;
                            }
                        }
                    }
                    closedir(dir);
                }
                dir_path = strtok_r(nullptr, ":", &saveptr);
            }
        }

        const char* builtins[] = {"cd", "history", "search", "pinfo", "echo", "pwd", "ls", "exit"};
        int num_builtins = 8;
        for (int i = 0; i < num_builtins && match_count < 100; i++) {
            if (std::strncmp(builtins[i], token, prefix_len) == 0) {
                bool duplicate = false;
                for (int j = 0; j < match_count; j++) {
                    if (std::strcmp(matches[j], builtins[i]) == 0) duplicate = true;
                }
                if (!duplicate) {
                    std::strcpy(matches[match_count], builtins[i]);
                    match_count++;
                }
            }
        }
    }

    if (match_count == 0) return;

    // Use file_prefix for argument completion and token for commands
    int active_prefix_len = is_command ? token_len : std::strlen(file_prefix);

    if (match_count == 1) {
        int missing_len = std::strlen(matches[0]) - active_prefix_len;
        if (pos + missing_len < max_len - 1) {
            std::strcpy(&buffer[pos], matches[0] + active_prefix_len);
            pos += missing_len;
            redraw_line(buffer);
        }
    } else {
        int lcp_len = std::strlen(matches[0]);
        for (int i = 1; i < match_count; i++) {
            int j = 0;
            int current_len = std::strlen(matches[i]);
            while (j < lcp_len && j < current_len && matches[0][j] == matches[i][j]) {
                j++;
            }
            lcp_len = j;
        }

        if (lcp_len > active_prefix_len) {
            int missing_len = lcp_len - active_prefix_len;
            if (pos + missing_len < max_len - 1) {
                std::strncpy(&buffer[pos], matches[0] + active_prefix_len, missing_len);
                pos += missing_len;
                buffer[pos] = '\0';
                redraw_line(buffer);
            }
        } else {
            write(STDOUT_FILENO, "\n", 1);
            sort_matches(matches, match_count);

            for (int i = 0; i < match_count; i++) {
                write(STDOUT_FILENO, matches[i], std::strlen(matches[i]));
                write(STDOUT_FILENO, "  ", 2);
            }
            write(STDOUT_FILENO, "\n", 1);
            redraw_line(buffer);
        }
    }
}