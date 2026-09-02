#ifndef RAW_INPUT_H
#define RAW_INPUT_H

// reads input character by character, handling arrows and backspaces
bool read_raw_input(char* buffer, int max_len);
// redraws the current terminal line and reprints the prompt + buffer
void redraw_line(const char* buffer); 

#endif