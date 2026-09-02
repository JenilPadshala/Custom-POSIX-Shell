#include "../include/autocomplete.h"
#include "../include/raw_input.h"
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <unistd.h>

// Custom function to sort matches alphabetically without using <algorithm>
void sort_matches(char matches[][256], int count) {
    char temp[256]; // Temporary buffer for swapping strings
    for (int i = 0; i < count - 1; i++) { // Outer loop for bubble sort
        for (int j = 0; j < count - i - 1; j++) { // Inner loop to compare adjacent elements
            if (std::strcmp(matches[j], matches[j + 1]) > 0) { // If the current string is alphabetically greater
                std::strcpy(temp, matches[j]); // Copy current to temp
                std::strcpy(matches[j], matches[j + 1]); // Overwrite current with next
                std::strcpy(matches[j + 1], temp); // Overwrite next with temp
            }
        }
    }
}

void handle_autocomplete(char* buffer, int& pos, int max_len) {
    // look for the beginning of the current word
    int word_start = pos;
    while(word_start > 0 && buffer[word_start - 1] != ' ') word_start--;        // traverse backwards until a space is found

    bool is_command = (word_start == 0); // if no space found, the word is a command
    int prefix_len = pos - word_start; // len of the prefix that user has typed so
    if(prefix_len == 0) return; // if no prefix, do nothing and return

    // store the prefix in a char array
    char prefix[256]; 
    std::strncpy(prefix, &buffer[word_start], prefix_len); 
    prefix[prefix_len] = '\0';

    // store matching filenames/dir_names/commands in an array
    char matches[100][256];
    int match_count = 0;

    if(!is_command){
        // if the user is typing a filename/dir_name
        DIR* dir = opendir(".");
        if(dir != nullptr){
            struct dirent* entry;
            while((entry = readdir(dir)) != nullptr && match_count < 100){
                if(std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue; // skip hidden "." and ".." directories
                if(std::strncmp(entry->d_name, prefix, prefix_len) == 0){ // if filename/dir_name starts with prefix
                    std::strcpy(matches[match_count], entry->d_name); // copy the filename/dir_name to the matches array
                    match_count++;
                }
            }
            closedir(dir);
        }
    } else { // If the user is typing a command
        const char* path_env = std::getenv("PATH"); // Fetch the PATH environment variable
        if (path_env != nullptr) { // Ensure PATH is not null
            char path_copy[4096]; // Allocate a buffer to copy PATH (since strtok modifies the string)
            std::strncpy(path_copy, path_env, 4095); // Safely copy PATH into the buffer
            path_copy[4095] = '\0'; // Null-terminate the copied PATH
            
            char* saveptr; // Pointer required for reentrant strtok_r
            char* dir_path = strtok_r(path_copy, ":", &saveptr); // Tokenize the PATH string by colon delimiter
            
            while (dir_path != nullptr) { // Loop through each directory in the PATH
                DIR* dir = opendir(dir_path); // Open the current PATH directory
                if (dir != nullptr) { // If the directory exists and opened successfully
                    struct dirent* entry; // Declare a directory entry pointer
                    while ((entry = readdir(dir)) != nullptr && match_count < 100) { // Loop through files in this directory
                        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) continue; // Skip hidden dirs
                        if (std::strncmp(entry->d_name, prefix, prefix_len) == 0) { // If the executable matches the prefix
                            bool duplicate = false; // Flag to check if we already recorded this command
                            for (int i = 0; i < match_count; i++) { // Loop through existing matches
                                if (std::strcmp(matches[i], entry->d_name) == 0) { // Compare names
                                    duplicate = true; // Mark as duplicate if found
                                    break; // Stop searching inner loop
                                }
                            }
                            if (!duplicate) { // If it is not a duplicate match
                                std::strcpy(matches[match_count], entry->d_name); // Add it to the matches array
                                match_count++; // Increment match counter
                            }
                        }
                    }
                    closedir(dir); // Close the current PATH directory
                }
                dir_path = strtok_r(nullptr, ":", &saveptr); // Grab the next directory from the tokenized PATH
            }
        }
        
        const char* builtins[] = {"cd", "history", "search", "pinfo", "echo", "pwd", "ls", "exit"}; // Array of internal shell commands
        int num_builtins = 8; // Hardcode the number of builtins in the array
        for (int i = 0; i < num_builtins && match_count < 100; i++) { // Loop through builtins
            if (std::strncmp(builtins[i], prefix, prefix_len) == 0) { // Check if builtin matches the prefix
                bool duplicate = false; // Duplicate flag
                for (int j = 0; j < match_count; j++) { // Check if already stored (unlikely, but safe)
                    if (std::strcmp(matches[j], builtins[i]) == 0) duplicate = true; // Mark duplicate
                }
                if (!duplicate) { // If unique
                    std::strcpy(matches[match_count], builtins[i]); // Add builtin to matches
                    match_count++; // Increment match counter
                }
            }
        }
    }

    if (match_count == 0) return; // If absolutely nothing matched, return and do nothing

    if (match_count == 1) { // If there is exactly one perfect match
        int missing_len = std::strlen(matches[0]) - prefix_len; // Calculate how many characters need to be added
        if (pos + missing_len < max_len - 1) { // Check for buffer overflow prevention
            std::strcpy(&buffer[pos], matches[0] + prefix_len); // Append only the missing characters to the buffer
            pos += missing_len; // Move the cursor position forward
            redraw_line(buffer); // Refresh the terminal line to show the completed word
        }
    } else { // If there are multiple matches, calculate the Longest Common Prefix (LCP)
        int lcp_len = std::strlen(matches[0]); // Assume the first match is the longest possible common prefix
        for (int i = 1; i < match_count; i++) { // Loop through all other matches
            int j = 0; // Initialize character comparison index
            int current_len = std::strlen(matches[i]); // Get length of the current match being compared
            while (j < lcp_len && j < current_len && matches[0][j] == matches[i][j]) { // Compare character by character
                j++; // Advance index while characters match
            }
            lcp_len = j; // Update the LCP length to where the mismatch occurred
        }

        if (lcp_len > prefix_len) { // If the shared LCP is longer than what the user typed
            int missing_len = lcp_len - prefix_len; // Calculate how many shared characters can be added
            if (pos + missing_len < max_len - 1) { // Prevent buffer overflow
                std::strncpy(&buffer[pos], matches[0] + prefix_len, missing_len); // Append the shared characters
                pos += missing_len; // Update cursor tracker
                buffer[pos] = '\0'; // Manually null-terminate the updated buffer
                redraw_line(buffer); // Redraw the terminal to show the extended prefix
            }
        } else { // If the user has already typed the full LCP (requires double tab to display options)
            write(STDOUT_FILENO, "\n", 1); // Move to a new line below the prompt
            sort_matches(matches, match_count); // Alphabetize the matches array using custom bubble sort
            
            for (int i = 0; i < match_count; i++) { // Loop through all sorted matches
                write(STDOUT_FILENO, matches[i], std::strlen(matches[i])); // Print the match to the screen
                write(STDOUT_FILENO, "  ", 2); // Print two spaces as separation padding
            }
            write(STDOUT_FILENO, "\n", 1); // Print a final newline after listing all options
            redraw_line(buffer); // Redraw the prompt and the current command state seamlessly below the list
        }
    }
}