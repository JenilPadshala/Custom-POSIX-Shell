#include "../include/utils.h"
#include <cstring>
#include <new>
#include <unistd.h>

// max tokens to prevent a huge line from growing the token array without bound.
#define MAX_TOKENS 256

// func to tokenize a string
char** tokenize(char* str, const char* delim, int& token_count) {
    // if str or delim is nullptr, return nullptr
    if (str == nullptr || delim == nullptr) {
        token_count = 0;
        return nullptr;
    }

    // allocate memory for tokens
    char** tokens = new (std::nothrow) char*[MAX_TOKENS];
    // if allocation fails, print error message and return nullptr
    if (tokens == nullptr) {
        const char error_msg[] = "Error: Failed to allocate memory for tokens\n";
        write(2, error_msg, sizeof(error_msg) - 1);
        token_count = 0;
        return nullptr;
    }

    token_count = 0;

    // ADDED LOGIC FOR HANDLING DOUBLE QUOTES (FOR ECHO) 
    
    bool in_quotes = false; // flag to track if we are inside quotation marks
    char* token_start = nullptr; // pointer to mark the beginning of the current token
    char* read_ptr = str; // pointer to read the original string
    char* write_ptr = str; // pointer to overwrite the string in-place
    
    // determine if we are tokenizing command arguments (spaces/tabs)
    bool is_space_delim = (std::strchr(delim, ' ') != nullptr);

    // iterate through the string until the null terminator is reached
    while (*read_ptr != '\0') {
        // toggle quote state, but DO NOT skip the character so it is printed
        if (is_space_delim && *read_ptr == '"') {
            in_quotes = !in_quotes;
            read_ptr++;
            if(token_start == nullptr) token_start = write_ptr;
            continue;
        }

        // if we hit a delimiter and we are NOT safely inside quotes
        if (!in_quotes && std::strchr(delim, *read_ptr) != nullptr) {
            if (token_start != nullptr) { // if we were tracking a valid token
                *write_ptr = '\0'; // null-terminate the token in-place
                if (token_count < MAX_TOKENS - 1) { // bounds check to prevent array overflow
                    tokens[token_count++] = token_start; // store the pointer in the array
                }
                token_start = nullptr; // reset token start for the next one
                write_ptr++; // advance write pointer past the null terminator
            }
        } else {
            // keep copying characters (now includes quotes AND locked spaces)
            if (token_start == nullptr) token_start = write_ptr; // mark start if this is a new token
            *write_ptr = *read_ptr; // copy character from read to write location
            write_ptr++; // advance write pointer
        }
        read_ptr++; // advance read pointer
    }

    // capture the final token if the string didn't end with a delimiter
    if (token_start != nullptr && token_count < MAX_TOKENS - 1) {
        *write_ptr = '\0'; // safely null-terminate the end of the string
        tokens[token_count++] = token_start; // store the final token
    }

    // trailing nullptr so the list is execvp-ready because execvp requires nullptr to terminate the list.
    tokens[token_count] = nullptr;
    return tokens;
}