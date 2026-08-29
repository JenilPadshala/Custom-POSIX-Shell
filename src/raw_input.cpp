#include "../include/raw_input.h"
#include "../include/history.h"
#include "../include/prompt.h"
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);     // get curr terminal settings
    struct termios raw = orig_termios;
    // disable canonical mode (line buffering) and local echo
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // apply the changes
}

// clears the current terminal line and reprints the prompt + buffer
void redraw_line(const char* buffer) {
    // \33[2K clears the entire line, \r moves cursor to the beginning
    const char* clear_seq = "\33[2K\r";
    write(STDOUT_FILENO, clear_seq, std::strlen(clear_seq));
    
    // Unbuffered system call to print prompt
    display_prompt();
    
    // Unbuffered system call to print the current command
    if (std::strlen(buffer) > 0) {
        write(STDOUT_FILENO, buffer, std::strlen(buffer));
    }
}

// function to read raw input from the terminal keystroke-by-keystroke and evaluate it instantly using a while loop
bool read_raw_input(char* buffer, int max_len) {
    enable_raw_mode(); 
    
    char history[MAX_HISTORY][2048]; // 2d array to store the history of commands
    int hist_count = get_history_list(history); // populate the history array and get the number of commands
    int hist_index = hist_count; // point just past the last command in the history
    char current_input[2048] = {0}; // save what you typed before pressing UP arrow

    // pos to track the cursor location, buffer starts as empty string, c holds each incoming byte
    int pos = 0;
    buffer[0] = '\0';
    char c;
    // infinite loop to read one byte at a time from stdin and process it
    while (read(STDIN_FILENO, &c, 1) == 1) {
        // if user presses Enter or Return
        if (c == '\n' || c == '\r') {
            write(STDOUT_FILENO, "\n", 1);
            buffer[pos] = '\0';
            break; 
        } else if (c == 4) { // Ctrl+D (EOF)
            disable_raw_mode();
            return false;
        } else if (c == 127) { // backspace
            if (pos > 0) { // only if there's something to delete
                pos--;
                buffer[pos] = '\0';
                write(STDOUT_FILENO, "\b \b", 3); // \b moves back one char, space clears it, then \b moves back again
            }
        } else if (c == 9) { // tab
            // TODO: implement auto-complete here
        } else if (c == '\033') { // escape sequence (arrows)
            // handle arrow keys
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

            if (seq[0] == '[') {
                if (seq[1] == 'A') { // UP Arrow
                    if (hist_index > 0) {
                        if (hist_index == hist_count) {
                            std::strcpy(current_input, buffer); // Save current typing
                        }
                        hist_index--; // move up one command in the history
                        std::strcpy(buffer, history[hist_index]); // copy the command to the buffer
                        pos = std::strlen(buffer);
                        redraw_line(buffer); // redraw the line with the new command
                    }
                } else if (seq[1] == 'B') { // DOWN Arrow
                    if (hist_index < hist_count) {
                        hist_index++; // move down one command in the history
                        if (hist_index == hist_count) {
                            std::strcpy(buffer, current_input); // restore saved typing
                        } else {
                            std::strcpy(buffer, history[hist_index]); // copy the command to the buffer
                        }
                        pos = std::strlen(buffer);
                        redraw_line(buffer); // redraw the line with the new command
                    }
                }
            }
        } else if (c >= 32 && c < 127) { // regular printable characters
            if (pos < max_len - 1) {
                buffer[pos++] = c;
                buffer[pos] = '\0';
                write(STDOUT_FILENO, &c, 1); // print the character to the terminal
            }
        }
    }

    disable_raw_mode(); // restore the terminal to its original state
    return true;
}