#ifndef LEDGER_H
#define LEDGER_H

/*
 * ledger.h - Persistent expense ledger
 *
 * Appends each completed split to a plain-text file so the debt history
 * accumulates between program runs.  The file can be opened in any
 * text editor or inspected with cat, less, grep, etc.
 */

#include "people.h"

/* ------------------------------------------------------------------ */
/* Summary report types                                                 */
/* ------------------------------------------------------------------ */

#define MAX_SUMMARY_PEOPLE  200  /* max unique names across all entries */

/* Per-person totals accumulated from all ledger entries */
typedef struct {
    char   name[MAX_NAME_LEN];
    double total_owed;
} PersonSummary;

/* Aggregate statistics read from the ledger file */
typedef struct {
    int           bill_count;               /* number of split records     */
    double        grand_total;              /* sum of all bill totals       */
    PersonSummary people[MAX_SUMMARY_PEOPLE]; /* per-person cumulative owes */
    int           num_people;              /* entries used in people[]     */
} LedgerSummary;

/* ------------------------------------------------------------------ */
/* Function declarations                                                */
/* ------------------------------------------------------------------ */

/*
 * Append one split record to the ledger file.
 *
 * Parameters:
 *   filepath    - path to the ledger text file
 *   bill_label  - short description of the expense (e.g. "Monthly Rent")
 *   category    - bill category (e.g. "Rent", "Groceries", "Utilities")
 *   total_bill  - total dollar amount
 *   people      - array of Person structs (filled after calculate_shares)
 *   num_people  - number of people in the array
 *   method      - SPLIT_EQUAL or SPLIT_WEIGHTED
 *
 * Returns 1 on success, 0 on I/O failure.
 */
int ledger_append(const char *filepath,
                  const char *bill_label,
                  const char *category,
                  double total_bill,
                  const Person people[],
                  int num_people,
                  SplitMethod method);

/*
 * Print the full contents of the ledger file to stdout.
 * Returns 1 on success, 0 if the file cannot be opened.
 */
int ledger_print(const char *filepath);

/*
 * Parse the ledger file and populate a LedgerSummary.
 * Accumulates bill count, grand total, and per-person totals.
 * Returns 1 on success, 0 if the file cannot be opened.
 */
int ledger_compute_summary(const char *filepath, LedgerSummary *out);

/*
 * Print a formatted summary report to stdout.
 */
void ledger_print_summary(const LedgerSummary *s, const char *filepath);

#endif /* LEDGER_H */
