/*
 * main.c - Expense Splitter
 *
 * Entry point.  Drives the main menu loop:
 *   1. New split
 *   2. View ledger
 *   3. Summary report
 *   4. Saved roommates
 *   5. Exit
 *
 * Data files (created in the current working directory):
 *   ledger.txt     - cumulative expense history (appended each split)
 *   roommates.txt  - saved roommate names (optional; reused across runs)
 *
 * Compatibility: C99, GCC 4.8+ (CentOS 7 / Linux)
 */

#include "input.h"
#include "ledger.h"
#include "names_file.h"
#include "people.h"

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* File paths                                                           */
/* ------------------------------------------------------------------ */

#define LEDGER_FILE   "ledger.txt"
#define NAMES_FILE    "roommates.txt"

/* ------------------------------------------------------------------ */
/* Bill categories                                                      */
/* ------------------------------------------------------------------ */

static const char *CATEGORIES[] = {
    "Rent",
    "Groceries",
    "Utilities",
    "Other"
};
#define NUM_CATEGORIES  4

/* ------------------------------------------------------------------ */
/* UI helpers                                                           */
/* ------------------------------------------------------------------ */

static void print_banner(void) {
    printf("\n");
    printf("  +==========================================+\n");
    printf("  |           $ Expense Splitter $           |\n");
    printf("  |  Split bills fairly. Track every cent.  |\n");
    printf("  +==========================================+\n");
    printf("\n");
}

static void print_divider(void) {
    printf("  ------------------------------------------\n");
}

static void print_main_menu(void) {
    printf("  Main Menu\n");
    print_divider();
    printf("  1. New split\n");
    printf("  2. View ledger\n");
    printf("  3. Summary report\n");
    printf("  4. Saved roommates\n");
    printf("  5. Exit\n");
    printf("\n");
}

/* ------------------------------------------------------------------ */
/* New split                                                            */
/* ------------------------------------------------------------------ */

static void do_split(void) {
    int i;

    printf("\n");
    print_divider();
    printf("  New Bill\n");
    print_divider();

    /* --- Bill description --- */
    char bill_label[128];
    printf("\n");
    read_name("  Description (e.g. Monthly Rent, Dinner): ", bill_label, sizeof(bill_label));

    /* --- Category --- */
    printf("\n  Category:\n");
    for (i = 0; i < NUM_CATEGORIES; i++) {
        printf("    %d. %s\n", i + 1, CATEGORIES[i]);
    }
    printf("\n");
    int cat_choice = read_menu_choice(NUM_CATEGORIES);
    const char *category = CATEGORIES[cat_choice - 1];

    /* --- Total amount --- */
    printf("\n");
    double total_bill = read_positive_double("  Total amount ($): ");

    /* --- Number of people --- */
    int num_people = read_int_range("  Number of people (1-50): ", 1, MAX_PEOPLE);

    /* --- Collect names --- */
    Person people[MAX_PEOPLE];
    memset(people, 0, sizeof(people));
    collect_names(people, num_people, NAMES_FILE);

    /* --- Split method --- */
    printf("\n  Split method:\n");
    printf("    1. Equal split\n");
    printf("    2. Weighted split\n");
    printf("\n");
    int method_choice = read_menu_choice(2);
    SplitMethod method = (method_choice == 1) ? SPLIT_EQUAL : SPLIT_WEIGHTED;

    /* --- Collect weights (if weighted) --- */
    if (method == SPLIT_WEIGHTED) {
        collect_weights(people, num_people);
    }

    /* --- Calculate --- */
    calculate_shares(people, num_people, total_bill, method);

    /* --- Display results --- */
    printf("\n");
    print_divider();
    printf("  Results: %s\n", bill_label);
    printf("  Category: %-16s  Split: %s\n",
           category,
           method == SPLIT_EQUAL ? "Equal" : "Weighted");
    printf("  People: %-2d                Total: $%.2f\n",
           num_people, total_bill);
    print_divider();
    print_results(people, num_people, total_bill);

    /* --- Append to ledger --- */
    if (ledger_append(LEDGER_FILE, bill_label, category,
                      total_bill, people, num_people, method)) {
        printf("  [OK] Saved to %s.\n", LEDGER_FILE);
    } else {
        printf("  [!!] Could not write to ledger.\n");
    }

    /* --- Offer to save names --- */
    printf("\n  Save these names for next time?\n");
    printf("    1. Yes\n    2. No\n\n");
    int save_choice = read_menu_choice(2);
    if (save_choice == 1) {
        char names[MAX_PEOPLE][MAX_NAME_LEN];
        for (i = 0; i < num_people; i++) {
            memcpy(names[i], people[i].name, MAX_NAME_LEN);
        }
        if (save_names(NAMES_FILE, names, num_people)) {
            printf("  [OK] Names saved to %s.\n", NAMES_FILE);
        } else {
            printf("  [!!] Could not write roommates file.\n");
        }
    }

    printf("\n");
}

/* ------------------------------------------------------------------ */
/* View ledger                                                          */
/* ------------------------------------------------------------------ */

static void do_view_ledger(void) {
    printf("\n");
    print_divider();
    printf("  Expense Ledger  (%s)\n", LEDGER_FILE);
    print_divider();
    ledger_print(LEDGER_FILE);
}

/* ------------------------------------------------------------------ */
/* Summary report                                                       */
/* ------------------------------------------------------------------ */

static void do_summary(void) {
    LedgerSummary summary;
    printf("\n");
    print_divider();
    ledger_compute_summary(LEDGER_FILE, &summary);
    ledger_print_summary(&summary, LEDGER_FILE);
}

/* ------------------------------------------------------------------ */
/* Saved roommates                                                      */
/* ------------------------------------------------------------------ */

static void do_roommates(void) {
    int i;

    printf("\n");
    print_divider();
    printf("  Saved Roommates  (%s)\n", NAMES_FILE);
    print_divider();

    char saved[MAX_PEOPLE][MAX_NAME_LEN];
    int count = load_names(NAMES_FILE, saved, MAX_PEOPLE);

    printf("\n");
    if (count == 0) {
        printf("  No names saved yet.\n\n");
    } else {
        printf("  Saved names (%d):\n", count);
        for (i = 0; i < count; i++) {
            printf("    %d. %s\n", i + 1, saved[i]);
        }
        printf("\n");
    }

    printf("  Options:\n");
    printf("    1. Replace saved names\n");
    printf("    2. Clear saved names\n");
    printf("    3. Back\n");
    printf("\n");

    int choice = read_menu_choice(3);

    if (choice == 1) {
        int n = read_int_range("  How many names to save (1-50): ", 1, MAX_PEOPLE);
        char names[MAX_PEOPLE][MAX_NAME_LEN];
        printf("\n");
        for (i = 0; i < n; i++) {
            char prompt[64];
            snprintf(prompt, sizeof(prompt), "    Name %d: ", i + 1);
            read_name(prompt, names[i], MAX_NAME_LEN);
        }
        if (save_names(NAMES_FILE, names, n)) {
            printf("\n  [OK] %d name(s) saved.\n\n", n);
        } else {
            printf("\n  [!!] Could not write names file.\n\n");
        }
    } else if (choice == 2) {
        FILE *fp = fopen(NAMES_FILE, "w");
        if (fp) {
            fclose(fp);
            printf("\n  [OK] Saved names cleared.\n\n");
        } else {
            printf("\n  [!!] Could not clear names file.\n\n");
        }
    }
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void) {
    print_banner();

    for (;;) {
        print_main_menu();

        int choice = read_menu_choice(5);

        switch (choice) {
            case 1:
                do_split();
                break;
            case 2:
                do_view_ledger();
                break;
            case 3:
                do_summary();
                break;
            case 4:
                do_roommates();
                break;
            case 5:
                printf("\n  Goodbye!\n\n");
                return 0;
            default:
                printf("  [Invalid choice.]\n\n");
                break;
        }
    }
}
