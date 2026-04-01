#ifndef INPUT_H
#define INPUT_H

/*
 * input.h - Safe input reading and validation utilities
 */

#include <stddef.h>

/* Read a line of text safely (strips trailing newline) */
int read_line(char *buf, size_t max_len);

/* Read and validate a positive double (> 0.0) */
double read_positive_double(const char *prompt);

/* Read and validate a positive integer (>= min_val, <= max_val) */
int read_int_range(const char *prompt, int min_val, int max_val);

/* Read a non-empty string (trims leading/trailing whitespace) */
int read_name(const char *prompt, char *buf, size_t max_len);

/* Read an integer from 1..num_choices inclusive */
int read_menu_choice(int num_choices);

#endif /* INPUT_H */
