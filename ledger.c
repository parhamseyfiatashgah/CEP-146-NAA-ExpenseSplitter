/*
 * ledger.c - Persistent expense ledger
 *
 * Each split is appended to a plain-text file in a human-readable
 * format.  The record includes a timestamp, category, bill label,
 * total amount, split method, and one line per person showing name,
 * weight/percentage, and amount owed.
 *
 * The file is designed to be easily inspected with cat, less, or grep.
 *
 * The ledger_compute_summary() function parses the same file to
 * produce cumulative totals (bill count, grand total, per-person debt).
 */

#include "ledger.h"
#include "people.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Width of separator lines written to the ledger */
#define SEP_WIDTH   60
/* Fixed name-column width used when writing person rows */
#define NAME_COL    24
/* Prefix used on the "Total" header line (must match exactly) */
#define TOTAL_PREFIX        "Total      : $"
#define TOTAL_PREFIX_LEN    14

/* ------------------------------------------------------------------ */
/* ledger_append                                                        */
/* ------------------------------------------------------------------ */

int ledger_append(const char *filepath,
                  const char *bill_label,
                  const char *category,
                  double total_bill,
                  const Person people[],
                  int num_people,
                  SplitMethod method) {
    int i;
    if (!filepath) return 0;

    FILE *fp = fopen(filepath, "a");
    if (!fp) {
        perror("  [ledger] Cannot open ledger file");
        return 0;
    }

    /* Timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Header block */
    for (i = 0; i < SEP_WIDTH; i++) fputc('=', fp);
    fputc('\n', fp);

    fprintf(fp, "Date       : %s\n",    timestamp);
    fprintf(fp, "Expense    : %s\n",    bill_label);
    fprintf(fp, "Category   : %s\n",    category ? category : "Other");
    fprintf(fp, "Total      : $%.2f\n", total_bill);
    fprintf(fp, "Split type : %s\n",
            method == SPLIT_EQUAL ? "Equal" : "Weighted");
    fprintf(fp, "People     : %d\n",    num_people);

    /* Column header */
    for (i = 0; i < SEP_WIDTH; i++) fputc('-', fp);
    fputc('\n', fp);
    fprintf(fp, "  %-*s  %8s  %9s  %9s\n",
            NAME_COL, "Name", "Weight", "Share (%)", "Owes ($)");
    for (i = 0; i < SEP_WIDTH; i++) fputc('-', fp);
    fputc('\n', fp);

    /* One row per person */
    for (i = 0; i < num_people; i++) {
        fprintf(fp, "  %-*s  %8.4f  %8.2f%%  %9.2f\n",
                NAME_COL,
                people[i].name,
                people[i].weight,
                people[i].share_pct,
                people[i].amount_owed);
    }

    /* Footer with TOTAL */
    for (i = 0; i < SEP_WIDTH; i++) fputc('-', fp);
    fputc('\n', fp);
    fprintf(fp, "  %-*s  %8s  %9s  %9.2f\n",
            NAME_COL, "TOTAL", "", "", total_bill);
    for (i = 0; i < SEP_WIDTH; i++) fputc('=', fp);
    fprintf(fp, "\n\n");

    fclose(fp);
    return 1;
}

/* ------------------------------------------------------------------ */
/* ledger_print                                                         */
/* ------------------------------------------------------------------ */

int ledger_print(const char *filepath) {
    if (!filepath) return 0;

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        printf("\n  (No ledger found at \"%s\". Split a bill first.)\n",
               filepath);
        return 0;
    }

    char line[256];
    printf("\n");
    while (fgets(line, sizeof(line), fp)) {
        printf("  %s", line);
    }
    printf("\n");

    fclose(fp);
    return 1;
}

/* ------------------------------------------------------------------ */
/* ledger_compute_summary helpers                                       */
/* ------------------------------------------------------------------ */

/*
 * Strip trailing whitespace (newline, carriage-return, spaces) in place.
 */
static void strip_trailing(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || s[n-1] == ' ')) {
        s[--n] = '\0';
    }
}

/*
 * Return 1 if the line consists only of the same repeated character c.
 * The line must have at least min_len occurrences of c.
 */
static int is_all_char(const char *line, char c, int min_len) {
    int n = 0;
    while (*line == c) { n++; line++; }
    return (*line == '\0' && n >= min_len);
}

/*
 * Try to find or create a PersonSummary entry for `name`.
 * Adds `amount` to their running total.
 */
static void accum_person(LedgerSummary *s, const char *name, double amount) {
    int i;
    int n;

    for (i = 0; i < s->num_people; i++) {
        if (strcmp(s->people[i].name, name) == 0) {
            s->people[i].total_owed += amount;
            return;
        }
    }

    /* New person */
    if (s->num_people >= MAX_SUMMARY_PEOPLE) return;
    n = 0;
    while (n < MAX_NAME_LEN - 1 && name[n] != '\0') {
        s->people[s->num_people].name[n] = name[n];
        n++;
    }
    s->people[s->num_people].name[n] = '\0';
    s->people[s->num_people].total_owed = amount;
    s->num_people++;
}

/*
 * Parse one person row from the ledger and accumulate into `s`.
 *
 * Person-row format (same as written by ledger_append):
 *   "  %-24s  %8.4f  %8.2f%%  %9.2f\n"
 *
 * - Characters [2..2+NAME_COL-1] = name field (24 chars, left-padded)
 * - Last whitespace-separated token = amount owed
 *
 * Skip rows whose name is "TOTAL" or "Name" (column-header guard).
 */
static void parse_person_row(const char *line, LedgerSummary *s) {
    int n;
    char name[MAX_NAME_LEN];
    const char *p;
    const char *tok_end;
    char amount_str[32];
    int alen;
    char *endptr;
    double amount;

    /* Must have at least 2 leading spaces */
    if (line[0] != ' ' || line[1] != ' ') return;

    /* Extract name from the fixed-width column [2 .. 2+NAME_COL-1] */
    n = 0;
    p = line + 2;
    while (n < NAME_COL && *p != '\0') {
        name[n++] = *p++;
    }
    name[n] = '\0';

    /* Trim trailing spaces from name */
    while (n > 0 && name[n-1] == ' ') name[--n] = '\0';

    if (n == 0)                     return; /* empty name */
    if (strcmp(name, "TOTAL") == 0) return; /* skip TOTAL row */
    if (strcmp(name, "Name")  == 0) return; /* skip column header */

    /* Find the last whitespace-separated token (the amount owed) */
    p = line + strlen(line);         /* point past the NUL */
    /* Skip trailing whitespace already stripped, but be safe */
    while (p > line && (*(p-1) == ' ' || *(p-1) == '\t')) p--;
    tok_end = p;
    /* Walk backwards over the token */
    while (p > line && *(p-1) != ' ' && *(p-1) != '\t') p--;
    if (p == tok_end) return;        /* no token found */

    alen = (int)(tok_end - p);
    if (alen <= 0 || alen >= (int)sizeof(amount_str)) return;
    memcpy(amount_str, p, alen);
    amount_str[alen] = '\0';

    errno = 0;
    amount = strtod(amount_str, &endptr);
    /* Reject if nothing was parsed or the token contains non-numeric chars */
    if (endptr == amount_str || *endptr != '\0' || errno == ERANGE) return;

    accum_person(s, name, amount);
}

/* ------------------------------------------------------------------ */
/* ledger_compute_summary                                               */
/* ------------------------------------------------------------------ */

int ledger_compute_summary(const char *filepath, LedgerSummary *out) {
    FILE *fp;
    char line[256];
    int in_block;
    int sep_count;
    int in_person_rows;

    if (!filepath || !out) return 0;
    memset(out, 0, sizeof(*out));

    fp = fopen(filepath, "r");
    if (!fp) return 0;

    in_block      = 0;
    sep_count     = 0;
    in_person_rows = 0;

    while (fgets(line, sizeof(line), fp)) {
        strip_trailing(line);

        /* Block boundary: all '=' (at least SEP_WIDTH chars) */
        if (is_all_char(line, '=', SEP_WIDTH)) {
            if (!in_block) {
                in_block      = 1;
                sep_count     = 0;
                in_person_rows = 0;
            } else {
                in_block      = 0;
                in_person_rows = 0;
            }
            continue;
        }

        if (!in_block) continue;

        /* Row separator: all '-' */
        if (is_all_char(line, '-', SEP_WIDTH)) {
            sep_count++;
            in_person_rows = (sep_count == 2); /* person rows follow sep #2 */
            continue;
        }

        /* Total line: "Total      : $X.XX" */
        if (strncmp(line, TOTAL_PREFIX, TOTAL_PREFIX_LEN) == 0) {
            char *endptr;
            double amount = strtod(line + TOTAL_PREFIX_LEN, &endptr);
            if (endptr != line + TOTAL_PREFIX_LEN) {
                out->grand_total += amount;
                out->bill_count++;
            }
            continue;
        }

        /* Person rows appear only between separator #2 and separator #3 */
        if (in_person_rows) {
            parse_person_row(line, out);
        }
    }

    fclose(fp);
    return 1;
}

/* ------------------------------------------------------------------ */
/* ledger_print_summary                                                 */
/* ------------------------------------------------------------------ */

void ledger_print_summary(const LedgerSummary *s, const char *filepath) {
    int i;

    printf("\n");
    printf("  ------------------------------------------\n");
    printf("  Summary Report  (%s)\n",
           filepath ? filepath : "ledger.txt");
    printf("  ------------------------------------------\n\n");

    if (s->bill_count == 0) {
        printf("  No entries found. Split a bill first (option 1).\n\n");
        return;
    }

    printf("  Bills recorded  : %d\n",     s->bill_count);
    printf("  Grand total     : $%.2f\n\n", s->grand_total);

    if (s->num_people == 0) {
        printf("  (No individual rows found in ledger.)\n\n");
        return;
    }

    /* Determine name column width */
    int name_col = 14;
    for (i = 0; i < s->num_people; i++) {
        int len = (int)strlen(s->people[i].name);
        if (len > name_col) name_col = len;
    }
    name_col += 2;

    int row_width = name_col + 14;

    printf("  Cumulative amounts owed per person:\n");
    printf("  ");
    for (i = 0; i < row_width; i++) printf("-");
    printf("\n");
    printf("  %-*s  %10s\n", name_col, "  Roommate", "Total ($)");
    printf("  ");
    for (i = 0; i < row_width; i++) printf("-");
    printf("\n");

    for (i = 0; i < s->num_people; i++) {
        printf("  %-*s  %10.2f\n",
               name_col,
               s->people[i].name,
               s->people[i].total_owed);
    }

    printf("  ");
    for (i = 0; i < row_width; i++) printf("-");
    printf("\n\n");
}
