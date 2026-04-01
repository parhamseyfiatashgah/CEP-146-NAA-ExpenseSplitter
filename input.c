/*
 * input.c - Safe input reading and validation utilities
 *
 * Provides functions for reading validated user input without
 * buffer overflows or silent failures.
 */

#include "input.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read one line from stdin into buf (max_len includes NUL terminator).
 * Returns 1 on success, 0 on EOF or error. */
int read_line(char *buf, size_t max_len) {
    if (!buf || max_len == 0) return 0;

    if (!fgets(buf, (int)max_len, stdin)) {
        buf[0] = '\0';
        return 0;
    }

    /* Strip trailing newline / carriage return */
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }

    /* If the line was too long, flush the rest of the line */
    if (len == max_len - 1) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
    }

    return 1;
}

/* Read and validate a positive double (value > 0).
 * Keeps prompting until the user provides a valid number.
 * On EOF, prints a message and calls exit(1). */
double read_positive_double(const char *prompt) {
    char buf[128];
    double value;
    char *endptr;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!read_line(buf, sizeof(buf))) {
            if (feof(stdin)) {
                printf("\n  [Unexpected end of input. Exiting.]\n");
                exit(1);
            }
            printf("  [Error reading input. Please try again.]\n");
            continue;
        }

        if (buf[0] == '\0') {
            printf("  [Input cannot be empty. Please enter a number.]\n");
            continue;
        }

        errno = 0;
        value = strtod(buf, &endptr);

        /* endptr must point to the end of the string (no trailing garbage) */
        while (*endptr == ' ' || *endptr == '\t') endptr++;

        if (endptr == buf || *endptr != '\0') {
            printf("  [Invalid number: \"%s\". Please try again.]\n", buf);
            continue;
        }
        if (errno == ERANGE) {
            printf("  [Number out of range. Please try again.]\n");
            continue;
        }
        if (value <= 0.0) {
            printf("  [Value must be greater than zero. Please try again.]\n");
            continue;
        }

        return value;
    }
}

/* Read an integer in [min_val, max_val], inclusive.
 * On EOF, prints a message and calls exit(1). */
int read_int_range(const char *prompt, int min_val, int max_val) {
    char buf[64];
    long value;
    char *endptr;

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!read_line(buf, sizeof(buf))) {
            if (feof(stdin)) {
                printf("\n  [Unexpected end of input. Exiting.]\n");
                exit(1);
            }
            printf("  [Error reading input. Please try again.]\n");
            continue;
        }

        if (buf[0] == '\0') {
            printf("  [Input cannot be empty.]\n");
            continue;
        }

        errno = 0;
        value = strtol(buf, &endptr, 10);

        while (*endptr == ' ' || *endptr == '\t') endptr++;

        if (endptr == buf || *endptr != '\0') {
            printf("  [Invalid integer: \"%s\". Please try again.]\n", buf);
            continue;
        }
        if (errno == ERANGE) {
            printf("  [Number out of range. Please try again.]\n");
            continue;
        }
        if (value < min_val || value > max_val) {
            printf("  [Please enter a number between %d and %d.]\n",
                   min_val, max_val);
            continue;
        }

        return (int)value;
    }
}

/* Read a non-empty name string; trims leading and trailing whitespace.
 * On EOF, prints a message and calls exit(1). */
int read_name(const char *prompt, char *buf, size_t max_len) {
    char raw[256];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        if (!read_line(raw, sizeof(raw))) {
            if (feof(stdin)) {
                printf("\n  [Unexpected end of input. Exiting.]\n");
                exit(1);
            }
            return 0;
        }

        /* Trim leading whitespace */
        char *start = raw;
        while (*start && isspace((unsigned char)*start)) start++;

        /* Trim trailing whitespace */
        char *end = start + strlen(start);
        while (end > start && isspace((unsigned char)*(end - 1))) {
            *(--end) = '\0';
        }

        if (*start == '\0') {
            printf("  [Name cannot be empty. Please try again.]\n");
            continue;
        }

        strncpy(buf, start, max_len - 1);
        buf[max_len - 1] = '\0';
        return 1;
    }
}

/* Prompt the user to choose from a numbered menu (1..num_choices). */
int read_menu_choice(int num_choices) {
    return read_int_range("Enter your choice: ", 1, num_choices);
}
