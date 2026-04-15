#ifndef LEXER_H
#define LEXER_H

typedef enum {
    /* literals */
    TOKEN_NUM,
    TOKEN_ID,

    /* types */
    TOKEN_INT,

    /* keywords */
    TOKEN_PRINT,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,

    /* arithmetic operators */
    TOKEN_PLUS,     /* +  */
    TOKEN_MINUS,    /* -  */
    TOKEN_STAR,     /* *  */
    TOKEN_SLASH,    /* /  */

    /* comparison operators */
    TOKEN_EQ,       /* == */
    TOKEN_NEQ,      /* != */
    TOKEN_LT,       /* <  */
    TOKEN_GT,       /* >  */
    TOKEN_LEQ,      /* <= */
    TOKEN_GEQ,      /* >= */

    /* assignment */
    TOKEN_ASSIGN,   /* =  */

    /* punctuation */
    TOKEN_SEMI,     /* ;  */
    TOKEN_LPAREN,   /* (  */
    TOKEN_RPAREN,   /* )  */
    TOKEN_LBRACE,   /* {  */
    TOKEN_RBRACE,   /* }  */

    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char      value[64];
} Token;

void  init_lexer(const char *input);
Token get_next_token(void);

#endif