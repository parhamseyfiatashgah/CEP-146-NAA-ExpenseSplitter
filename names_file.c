/*
 * names_file.c - Persistent roommate names file management
 *
 * Reads and writes one name per line to a plain-text file.
 * The format is intentionally human-readable so users can
 * view or edit it with any text editor.
 */

#include "names_file.h"
#include "people.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* save_names                                                           */
/* ------------------------------------------------------------------ */

int save_names(const char *filepath,
               char names[][MAX_NAME_LEN], int count) {
    if (!filepath || count < 0) return 0;

    FILE *fp = fopen(filepath, "w");
    if (!fp) return 0;

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s\n", names[i]);
    }

    fclose(fp);
    return 1;
}

/* ------------------------------------------------------------------ */
/* load_names                                                           */
/* ------------------------------------------------------------------ */

int load_names(const char *filepath,
               char names[][MAX_NAME_LEN], int max_count) {
    if (!filepath) return 0;

    FILE *fp = fopen(filepath, "r");
    if (!fp) return 0;

    int count = 0;
    char line[MAX_NAME_LEN * 2]; /* generous buffer for reading */

    while (count < max_count && fgets(line, sizeof(line), fp)) {
        /* Strip trailing newline/carriage-return */
        char *p = line + strlen(line);
        while (p > line && (*(p - 1) == '\n' || *(p - 1) == '\r')) {
            *(--p) = '\0';
        }

        /* Trim leading whitespace */
        char *start = line;
        while (*start && isspace((unsigned char)*start)) start++;

        if (*start == '\0') continue; /* skip blank lines */

        /* Copy at most MAX_NAME_LEN-1 characters */
        int n = 0;
        while (n < MAX_NAME_LEN - 1 && start[n] != '\0') {
            names[count][n] = start[n];
            n++;
        }
        names[count][n] = '\0';
        count++;
    }

    fclose(fp);
    return count;
}
