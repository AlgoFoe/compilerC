#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"

static const char *src;
static int pos = 0;

void init_lexer(const char *input) {
    src = input;
    pos = 0;
}

/* true if char at (pos+offset) is NOT alphanumeric/underscore (word boundary) */
static int at_word_boundary(int len) {
    char c = src[pos + len];
    return !(isalnum(c) || c == '_');
}

Token get_next_token(void) {
    Token tok;
    tok.value[0] = '\0';

    /* skip whitespace */
    while (src[pos] && (src[pos] == ' '  || src[pos] == '\t' ||
                        src[pos] == '\n' || src[pos] == '\r'))
        pos++;

    if (src[pos] == '\0') {
        tok.type = TOKEN_EOF;
        return tok;
    }

    /* ── two-character operators (must check before single-char) ── */
    if (src[pos] == '=' && src[pos+1] == '=') { pos += 2; tok.type = TOKEN_EQ;  strcpy(tok.value, "=="); return tok; }
    if (src[pos] == '!' && src[pos+1] == '=') { pos += 2; tok.type = TOKEN_NEQ; strcpy(tok.value, "!="); return tok; }
    if (src[pos] == '<' && src[pos+1] == '=') { pos += 2; tok.type = TOKEN_LEQ; strcpy(tok.value, "<="); return tok; }
    if (src[pos] == '>' && src[pos+1] == '=') { pos += 2; tok.type = TOKEN_GEQ; strcpy(tok.value, ">="); return tok; }

    /* ── single-character tokens ─────────────────────────────────── */
    switch (src[pos]) {
        case '=': pos++; tok.type = TOKEN_ASSIGN; strcpy(tok.value, "=");  return tok;
        case '+': pos++; tok.type = TOKEN_PLUS;   strcpy(tok.value, "+");  return tok;
        case '-': pos++; tok.type = TOKEN_MINUS;  strcpy(tok.value, "-");  return tok;
        case '*': pos++; tok.type = TOKEN_STAR;   strcpy(tok.value, "*");  return tok;
        case '/': pos++; tok.type = TOKEN_SLASH;  strcpy(tok.value, "/");  return tok;
        case '<': pos++; tok.type = TOKEN_LT;     strcpy(tok.value, "<");  return tok;
        case '>': pos++; tok.type = TOKEN_GT;     strcpy(tok.value, ">");  return tok;
        case ';': pos++; tok.type = TOKEN_SEMI;   strcpy(tok.value, ";");  return tok;
        case '(': pos++; tok.type = TOKEN_LPAREN; strcpy(tok.value, "(");  return tok;
        case ')': pos++; tok.type = TOKEN_RPAREN; strcpy(tok.value, ")");  return tok;
        case '{': pos++; tok.type = TOKEN_LBRACE; strcpy(tok.value, "{");  return tok;
        case '}': pos++; tok.type = TOKEN_RBRACE; strcpy(tok.value, "}");  return tok;
    }

    /* ── keywords (longest match first) ─────────────────────────── */
    struct { const char *word; int len; TokenType type; } kw[] = {
        { "while", 5, TOKEN_WHILE },
        { "print", 5, TOKEN_PRINT },
        { "else",  4, TOKEN_ELSE  },
        { "int",   3, TOKEN_INT   },
        { "if",    2, TOKEN_IF    },
        { NULL,    0, 0           }
    };
    for (int k = 0; kw[k].word; k++) {
        int len = kw[k].len;
        if (strncmp(&src[pos], kw[k].word, len) == 0 && at_word_boundary(len)) {
            pos += len;
            tok.type = kw[k].type;
            strcpy(tok.value, kw[k].word);
            return tok;
        }
    }

    /* ── identifier ──────────────────────────────────────────────── */
    if (isalpha(src[pos]) || src[pos] == '_') {
        int i = 0;
        while (isalnum(src[pos]) || src[pos] == '_')
            tok.value[i++] = src[pos++];
        tok.value[i] = '\0';
        tok.type = TOKEN_ID;
        return tok;
    }

    /* ── integer literal ─────────────────────────────────────────── */
    if (isdigit(src[pos])) {
        int i = 0;
        while (isdigit(src[pos]))
            tok.value[i++] = src[pos++];
        tok.value[i] = '\0';
        tok.type = TOKEN_NUM;
        return tok;
    }

    /* unknown character – skip and continue */
    fprintf(stderr, "Lexer warning: unknown character '%c'\n", src[pos]);
    pos++;
    return get_next_token();
}