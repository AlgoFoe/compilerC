#include <stdio.h>
#include <stdlib.h>
#include "lexer/lexer.h"
#include "parser/parser.h"

/*
 * Read the entire contents of a file into a heap-allocated string.
 * Caller must free() the result.
 */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    if (!buf) { fprintf(stderr, "Out of memory\n"); exit(1); }

    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char *argv[]) {
    const char *input_file = "input.c";          /* default source file */

    if (argc == 2) {
        input_file = argv[1];                     /* optional: ./cumpileher myfile.c */
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [source_file]\n", argv[0]);
        return 1;
    }

    printf("=== Compiler ===\n");
    printf("Source: %s\n\n", input_file);

    char *source = read_file(input_file);

    init_lexer(source);
    parse();

    free(source);
    return 0;
}