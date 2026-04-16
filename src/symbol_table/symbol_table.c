#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "symbol_table.h"

char table[100][50];
int count = 0;

void add_symbol(const char *name) {
    if (!exists(name)) {
        strcpy(table[count++], name);
    }
}

int exists(const char* name){
    for(int i = 0; i < count; i++) {
        if(strcmp(table[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Calculate Levenshtein distance between two strings */
static int levenshtein_distance(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    /* Allocate distance matrix */
    int **d = malloc((len1 + 1) * sizeof(int*));
    for (int i = 0; i <= len1; i++) {
        d[i] = malloc((len2 + 1) * sizeof(int));
    }

    /* Initialize base cases */
    for (int i = 0; i <= len1; i++) d[i][0] = i;
    for (int j = 0; j <= len2; j++) d[0][j] = j;

    /* Fill the distance matrix */
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            int min_val = d[i-1][j] + 1;      /* deletion */
            if (d[i][j-1] + 1 < min_val) min_val = d[i][j-1] + 1;  /* insertion */
            if (d[i-1][j-1] + cost < min_val) min_val = d[i-1][j-1] + cost;  /* substitution */
            d[i][j] = min_val;
        }
    }

    int result = d[len1][len2];

    /* Free memory */
    for (int i = 0; i <= len1; i++) free(d[i]);
    free(d);

    return result;
}

/* Count matching characters at the same positions */
static int count_matching_chars(const char *s1, const char *s2) {
    int matches = 0;
    int len = strlen(s1) < strlen(s2) ? strlen(s1) : strlen(s2);
    for (int i = 0; i < len; i++) {
        if (s1[i] == s2[i]) matches++;
    }
    return matches;
}

/* Find the closest match in the symbol table (max distance 2) */
const char* find_closest_match(const char *name) {
    int min_distance = 3;  /* Only suggest if distance <= 2 */
    int best_match_score = -1;
    const char *best_match = NULL;

    for (int i = 0; i < count; i++) {
        int dist = levenshtein_distance(name, table[i]);
        if (dist < min_distance) {
            min_distance = dist;
            best_match_score = count_matching_chars(name, table[i]);
            best_match = table[i];
        } else if (dist == min_distance) {
            /* Tiebreaker: prefer symbol with more matching chars at same positions */
            int match_score = count_matching_chars(name, table[i]);
            if (match_score > best_match_score) {
                best_match_score = match_score;
                best_match = table[i];
            }
        }
    }

    return best_match;
}