#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

void add_symbol(const char *name);
int exists(const char* name);
const char* find_closest_match(const char *name);

#endif