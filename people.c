/*
 * people.c - Person data structure and operations
 *
 * Handles name collection (with optional saved-names file), weight
 * collection, share calculation, and result display.
 */

#include "people.h"
#include "input.h"
#include "names_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Name collection                                                      */
/* ------------------------------------------------------------------ */

/*
 * Offer to load names from the saved names file.
 * Returns the number of names loaded (0 if not used or file missing).
 */
static int try_load_saved_names(Person people[], int num_people,
                                 const char *filepath) {
    int i;
    char saved[MAX_PEOPLE][MAX_NAME_LEN];
    int  count = load_names(filepath, saved, MAX_PEOPLE);

    if (count == 0) return 0;

    printf("\n  Saved roommates found (%d):\n", count);
    for (i = 0; i < count; i++) {
        printf("    %d. %s\n", i + 1, saved[i]);
    }
    printf("\n  Use saved names? (1=Yes, 2=No): ");
    int choice = read_int_range("", 1, 2);

    if (choice == 2) return 0;

    /* Copy as many saved names as we need */
    int to_copy = (count < num_people) ? count : num_people;
    for (i = 0; i < to_copy; i++) {
        memcpy(people[i].name, saved[i], MAX_NAME_LEN);
    }

    if (to_copy < num_people) {
        printf("\n  Only %d saved name(s); please enter the remaining %d:\n\n",
               to_copy, num_people - to_copy);
        for (i = to_copy; i < num_people; i++) {
            char prompt[64];
            snprintf(prompt, sizeof(prompt), "  Person %d name: ", i + 1);
            read_name(prompt, people[i].name, MAX_NAME_LEN);
        }
    }

    return num_people;
}

int collect_names(Person people[], int num_people,
                  const char *saved_names_file) {
    int i;

    /* Try to reuse saved names */
    if (saved_names_file) {
        int loaded = try_load_saved_names(people, num_people, saved_names_file);
        if (loaded == num_people) {
            printf("\n  Names loaded from file.\n");
            return num_people;
        }
    }

    /* Manual entry */
    printf("\n  Enter each person's name:\n");
    for (i = 0; i < num_people; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "    Person %d: ", i + 1);
        read_name(prompt, people[i].name, MAX_NAME_LEN);
    }

    return num_people;
}

/* ------------------------------------------------------------------ */
/* Weight collection                                                    */
/* ------------------------------------------------------------------ */

int collect_weights(Person people[], int num_people) {
    int i;

    printf("\n  Weighted split options:\n");
    printf("    1. Raw weights  (e.g. 1, 2, 3 - proportional)\n");
    printf("    2. Percentages  (must sum to 100)\n");
    printf("\n");
    int mode = read_menu_choice(2);

    if (mode == 1) {
        /* Raw weights */
        printf("\n  Enter a positive weight for each person.\n");
        printf("  (Shares are divided proportionally to these values.)\n\n");

        for (i = 0; i < num_people; i++) {
            char prompt[96];
            snprintf(prompt, sizeof(prompt),
                     "  Weight for %-20s: ", people[i].name);
            people[i].weight = read_positive_double(prompt);
        }
    } else {
        /* Percentages - must sum to exactly 100 */
        printf("\n  Enter a percentage (0-100) for each person.\n");
        printf("  They must total exactly 100%%.\n\n");

        for (;;) {
            double total_pct = 0.0;
            for (i = 0; i < num_people; i++) {
                char prompt[96];
                snprintf(prompt, sizeof(prompt),
                         "  %% for %-22s: ", people[i].name);
                double pct = read_positive_double(prompt);
                people[i].weight = pct;
                total_pct += pct;
            }

            /* Allow +/-0.001 rounding tolerance */
            double diff = total_pct - 100.0;
            if (diff < 0.0) diff = -diff;
            if (diff > 0.001) {
                printf("\n  [Percentages sum to %.2f%%, not 100%%."
                       " Please re-enter.]\n\n",
                       total_pct);
                continue;
            }
            break;
        }
    }

    return 1;
}

/* ------------------------------------------------------------------ */
/* Share calculation                                                    */
/* ------------------------------------------------------------------ */

void calculate_shares(Person people[], int num_people,
                      double total_bill, SplitMethod method) {
    int i;

    if (method == SPLIT_EQUAL) {
        for (i = 0; i < num_people; i++) {
            people[i].weight = 1.0;
        }
    }

    /* Sum all weights */
    double weight_sum = 0.0;
    for (i = 0; i < num_people; i++) {
        weight_sum += people[i].weight;
    }

    /* Compute each person's share */
    double running_total = 0.0;
    for (i = 0; i < num_people - 1; i++) {
        people[i].share_pct   = (people[i].weight / weight_sum) * 100.0;
        people[i].amount_owed = (people[i].weight / weight_sum) * total_bill;
        running_total        += people[i].amount_owed;
    }

    /* Last person gets the remainder - eliminates floating-point drift */
    int last = num_people - 1;
    people[last].share_pct   = (people[last].weight / weight_sum) * 100.0;
    people[last].amount_owed = total_bill - running_total;
}

/* ------------------------------------------------------------------ */
/* Result display                                                       */
/* ------------------------------------------------------------------ */

void print_results(const Person people[], int num_people, double total_bill) {
    int i;

    /* Compute name column width */
    int name_col = 14;
    for (i = 0; i < num_people; i++) {
        int len = (int)strlen(people[i].name);
        if (len > name_col) name_col = len;
    }
    name_col += 2;

    /* "Share (%)" = 10, "Owes ($)" = 10, plus 4 spaces separators */
    int row_width = 2 + name_col + 2 + 10 + 2 + 10;

    /* Print header row */
    printf("  ");
    for (i = 0; i < row_width - 2; i++) printf("-");
    printf("\n");
    printf("  %-*s  %10s  %10s\n", name_col, "  Person", "Share (%)", "Owes ($)");
    printf("  ");
    for (i = 0; i < row_width - 2; i++) printf("-");
    printf("\n");

    /* Print one row per person */
    for (i = 0; i < num_people; i++) {
        printf("  %-*s  %9.2f%%  %10.2f\n",
               name_col,
               people[i].name,
               people[i].share_pct,
               people[i].amount_owed);
    }

    /* Print footer with total */
    printf("  ");
    for (i = 0; i < row_width - 2; i++) printf("-");
    printf("\n");
    printf("  %-*s  %10s  %10.2f\n", name_col, "  TOTAL", "", total_bill);
    printf("  ");
    for (i = 0; i < row_width - 2; i++) printf("-");
    printf("\n\n");
}
