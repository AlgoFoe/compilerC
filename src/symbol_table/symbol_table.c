#include <stdio.h>
#include <string.h>
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