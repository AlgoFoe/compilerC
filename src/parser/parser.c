/*
 * parser.c
 *
 * Grammar supported:
 *
 *   program     → statement*
 *
 *   statement   → declaration
 *               | assignment
 *               | if_stmt
 *               | while_stmt
 *               | print_stmt
 *
 *   declaration → 'int' ID '=' expr ';'
 *   assignment  → ID '=' expr ';'          (variable must already exist)
 *   if_stmt     → 'if' '(' expr ')' block ( 'else' block )?
 *   while_stmt  → 'while' '(' expr ')' block
 *   print_stmt  → 'print' '(' ID ')' ';'
 *   block       → '{' statement* '}'
 *
 *   expr        → term ( ('+' | '-') term )*
 *   term        → factor ( ('*' | '/') factor )*
 *   factor      → NUM | ID | '(' expr ')'
 *
 *   comparison  → expr ( '==' | '!=' | '<' | '>' | '<=' | '>=' ) expr
 *               | expr
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lexer/lexer.h"
#include "parser.h"
#include "../code_gen/codegen.h"
#include "../symbol_table/symbol_table.h"

/* ── state ──────────────────────────────────────────────────────── */

static Token cur;           /* current lookahead token */
static int   temp_id = 0;  /* counter for compiler-generated temporaries */

/* ── helpers ────────────────────────────────────────────────────── */

static void advance(void) {
    cur = get_next_token();
}

static void eat(TokenType expected) {
    if (cur.type == expected) {
        advance();
    } else {
        fprintf(stderr, "Syntax error: expected token type %d but got '%s'\n",
                expected, cur.value);
        /* error recovery: skip one token and continue */
        advance();
    }
}

/* allocate a fresh temporary variable name */
static void fresh_temp(char *buf) {
    sprintf(buf, "__t%d", temp_id++);
    declare_variable(buf);
}

/* is the current token an arithmetic operator? */
static int is_addop(void)  { return cur.type == TOKEN_PLUS  || cur.type == TOKEN_MINUS; }
static int is_mulop(void)  { return cur.type == TOKEN_STAR  || cur.type == TOKEN_SLASH; }
static int is_cmpop(void)  {
    return cur.type == TOKEN_EQ  || cur.type == TOKEN_NEQ ||
           cur.type == TOKEN_LT  || cur.type == TOKEN_GT  ||
           cur.type == TOKEN_LEQ || cur.type == TOKEN_GEQ;
}

/* ── expression parsing (writes result name into out_var) ───────── */

/* forward declarations */
static void parse_expr(char *out_var);
static void parse_term(char *out_var);
static void parse_factor(char *out_var);

/*
 * factor → NUM | ID | '(' expr ')'
 */
static void parse_factor(char *out_var) {
    if (cur.type == TOKEN_NUM) {
        strcpy(out_var, cur.value);
        advance();
    } else if (cur.type == TOKEN_ID) {
        if (!exists(cur.value)) {
            const char *suggestion = find_closest_match(cur.value);
            fprintf(stderr, "Error: variable '%s' used before declaration\n", cur.value);
            if (suggestion) {
                fprintf(stderr, "  did you mean: '%s'?\n", suggestion);
            }
        }
        strcpy(out_var, cur.value);
        advance();
    } else if (cur.type == TOKEN_LPAREN) {
        eat(TOKEN_LPAREN);
        parse_expr(out_var);
        eat(TOKEN_RPAREN);
    } else {
        fprintf(stderr, "Error: unexpected token '%s' in expression\n", cur.value);
        strcpy(out_var, "0");
        advance();
    }
}

/*
 * term → factor ( ('*' | '/') factor )*
 */
static void parse_term(char *out_var) {
    parse_factor(out_var);

    while (is_mulop()) {
        char op[4];
        strcpy(op, cur.value);
        advance();

        char right[64];
        parse_factor(right);

        char tmp[64];
        fresh_temp(tmp);

        if (strcmp(op, "*") == 0) gen_mul(tmp, out_var, right);
        else                       gen_div(tmp, out_var, right);

        strcpy(out_var, tmp);
    }
}

/*
 * expr → term ( ('+' | '-') term )*
 */
static void parse_expr(char *out_var) {
    parse_term(out_var);

    while (is_addop()) {
        char op[4];
        strcpy(op, cur.value);
        advance();

        char right[64];
        parse_term(right);

        char tmp[64];
        fresh_temp(tmp);

        if (strcmp(op, "+") == 0) gen_add(tmp, out_var, right);
        else                       gen_sub(tmp, out_var, right);

        strcpy(out_var, tmp);
    }
}

/*
 * condition → expr ( cmpop expr )?
 * If no comparison operator follows, treat the expr itself as the condition.
 */
static void parse_condition(char *out_var) {
    char left[64];
    parse_expr(left);

    if (is_cmpop()) {
        char op[4];
        strcpy(op, cur.value);
        advance();

        char right[64];
        parse_expr(right);

        char tmp[64];
        fresh_temp(tmp);
        gen_cmp(tmp, left, right, op);
        strcpy(out_var, tmp);
    } else {
        /* bare expression used as boolean */
        strcpy(out_var, left);
    }
}

/* ── statement parsers ──────────────────────────────────────────── */

static void parse_statement(void);   /* forward */

/*
 * block → '{' statement* '}'
 */
static void parse_block(void) {
    eat(TOKEN_LBRACE);
    while (cur.type != TOKEN_RBRACE && cur.type != TOKEN_EOF)
        parse_statement();
    eat(TOKEN_RBRACE);
}

/*
 * declaration → 'int' ID '=' expr ';'
 */
static void parse_declaration(void) {
    eat(TOKEN_INT);

    char name[64];
    strcpy(name, cur.value);
    eat(TOKEN_ID);

    if (exists(name)) {
        fprintf(stderr, "Warning: '%s' already declared\n", name);
    }
    add_symbol(name);
    declare_variable(name);

    eat(TOKEN_ASSIGN);

    char result[64];
    parse_expr(result);

    /* store expression result into the named variable */
    if (strcmp(result, name) != 0)
        gen_mov(name, result);

    eat(TOKEN_SEMI);
}

/*
 * assignment--> ID '=' expr ';'
 */
static void parse_assignment(void) {
    char name[64];
    strcpy(name, cur.value);
    eat(TOKEN_ID);

    if (!exists(name)) {
        const char *suggestion = find_closest_match(name);
        fprintf(stderr, "Error: '%s' not declared\n", name);
        if (suggestion) {
            fprintf(stderr, "  did you mean: '%s'?\n", suggestion);
        }
    }

    eat(TOKEN_ASSIGN);

    char result[64];
    parse_expr(result);

    if (strcmp(result, name) != 0)
        gen_assign(name, result);

    eat(TOKEN_SEMI);
}

/*
 * if_stmt → 'if' '(' condition ')' block ( 'else' block )?
 *
 * Generated assembly layout:
 *
 *   <evaluate condition into cond_tmp>
 *   jump_if_false cond_tmp, else_label
 *   <then block>
 *   jmp end_label
 *   else_label:
 *   <else block>   (may be empty)
 *   end_label:
 */
static void parse_if(void) {
    eat(TOKEN_IF);
    eat(TOKEN_LPAREN);

    char cond[64];
    parse_condition(cond);
    eat(TOKEN_RPAREN);

    char else_lbl[64], end_lbl[64];
    strcpy(else_lbl, new_label("else"));
    strcpy(end_lbl,  new_label("end_if"));

    gen_jump_if_false(cond, else_lbl);

    parse_block();                       /* then-branch */

    gen_jump(end_lbl);
    gen_label(else_lbl);

    if (cur.type == TOKEN_ELSE) {
        eat(TOKEN_ELSE);
        parse_block();                   /* else-branch */
    }

    gen_label(end_lbl);
}

/*
 * while_stmt → 'while' '(' condition ')' block
 *
 * Generated assembly layout:
 *
 *   loop_label:
 *   <evaluate condition into cond_tmp>
 *   jump_if_false cond_tmp, end_label
 *   <body>
 *   jmp loop_label
 *   end_label:
 */
static void parse_while(void) {
    eat(TOKEN_WHILE);

    char loop_lbl[64], end_lbl[64];
    strcpy(loop_lbl, new_label("while_loop"));
    strcpy(end_lbl,  new_label("while_end"));

    gen_label(loop_lbl);

    eat(TOKEN_LPAREN);
    char cond[64];
    parse_condition(cond);
    eat(TOKEN_RPAREN);

    gen_jump_if_false(cond, end_lbl);

    parse_block();

    gen_jump(loop_lbl);
    gen_label(end_lbl);
}

/*
 * print_stmt → 'print' '(' ID ')' ';'
 */
static void parse_print(void) {
    eat(TOKEN_PRINT);
    eat(TOKEN_LPAREN);

    if (!exists(cur.value)) {
        const char *suggestion = find_closest_match(cur.value);
        fprintf(stderr, "Error: '%s' not declared\n", cur.value);
        if (suggestion) {
            fprintf(stderr, "  did you mean: '%s'?\n", suggestion);
        }
    }
    char name[64];
    strcpy(name, cur.value);
    eat(TOKEN_ID);

    eat(TOKEN_RPAREN);
    eat(TOKEN_SEMI);

    gen_print(name);
}

/*
 * statement dispatcher
 */
static void parse_statement(void) {
    switch (cur.type) {
        case TOKEN_INT:    parse_declaration(); break;
        case TOKEN_IF:     parse_if();          break;
        case TOKEN_WHILE:  parse_while();       break;
        case TOKEN_PRINT:  parse_print();       break;
        case TOKEN_ID:     parse_assignment();  break;
        default:
            fprintf(stderr, "Error: unexpected token '%s'\n", cur.value);
            advance();
            break;
    }
}

/* ── entry point ────────────────────────────────────────────────── */

void parse(void) {
    init_codegen();
    advance();                          /* prime the lookahead */

    while (cur.type != TOKEN_EOF)
        parse_statement();

    finish_codegen();
}