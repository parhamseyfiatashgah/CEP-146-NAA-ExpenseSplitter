#ifndef PEOPLE_H
#define PEOPLE_H

/*
 * people.h - Person data structure and operations
 *
 * Manages the list of people, their names, weights/percentages,
 * and the computed amounts owed.
 */

#define MAX_PEOPLE      50
#define MAX_NAME_LEN    64

/* One participant in an expense split */
typedef struct {
    char   name[MAX_NAME_LEN];  /* Person's name                            */
    double weight;              /* Raw weight (equal=1.0 each, or user-set) */
    double share_pct;           /* Percentage share (0-100), derived        */
    double amount_owed;         /* Dollar amount this person owes            */
} Person;

/* Split method */
typedef enum {
    SPLIT_EQUAL    = 1,
    SPLIT_WEIGHTED = 2
} SplitMethod;

/*
 * Prompt for person names.  If saved_names_file is non-NULL and the file
 * exists, offer to load names from it first.
 * Returns the number of people entered (>= 1).
 */
int collect_names(Person people[], int num_people,
                  const char *saved_names_file);

/*
 * Collect weights for a weighted split.
 * Fills people[i].weight for each person.
 * Returns 1 on success, 0 if the user cannot provide valid weights.
 */
int collect_weights(Person people[], int num_people);

/*
 * Calculate amount_owed and share_pct for every person.
 * For equal splits, all weights are set to 1.0.
 */
void calculate_shares(Person people[], int num_people,
                      double total_bill, SplitMethod method);

/* Print a formatted result table to stdout */
void print_results(const Person people[], int num_people, double total_bill);

#endif /* PEOPLE_H */
