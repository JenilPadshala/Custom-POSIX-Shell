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
    // tokenize the string
    char* token = std::strtok(str, delim);
    while (token != nullptr && token_count < MAX_TOKENS - 1) {
        // store pointers into str, not copies so the caller must keep str alive.
        tokens[token_count++] = token;
        token = std::strtok(nullptr, delim);
    }

    // trailing nullptr so the list is execvp-ready because execvp requires nullptr to terminate the list.
    tokens[token_count] = nullptr;
    return tokens;
}
