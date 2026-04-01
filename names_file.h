#ifndef NAMES_FILE_H
#define NAMES_FILE_H

/*
 * names_file.h - Persistent roommate names file management
 *
 * Saves and loads a list of names to/from a plain-text file so
 * users don't have to retype names every session.
 *
 * The array element size must match MAX_NAME_LEN from people.h.
 * MAX_NAME_LEN is 64, so callers pass char[][MAX_NAME_LEN].
 */

#include "people.h"   /* for MAX_NAME_LEN and MAX_PEOPLE */

/*
 * Save names to a file, one name per line.
 * Returns 1 on success, 0 on failure.
 */
int save_names(const char *filepath,
               char names[][MAX_NAME_LEN], int count);

/*
 * Load names from a file, one per line, up to MAX_PEOPLE entries.
 * Each name is stored as a NUL-terminated string of at most MAX_NAME_LEN-1
 * characters.
 * Returns the number of names actually loaded (0 if file missing/empty).
 */
int load_names(const char *filepath,
               char names[][MAX_NAME_LEN], int max_count);

#endif /* NAMES_FILE_H */
