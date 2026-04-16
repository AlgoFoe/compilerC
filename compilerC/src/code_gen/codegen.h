#ifndef CODEGEN_H
#define CODEGEN_H

void init_codegen(void);
void finish_codegen(void);

void declare_variable(const char *name);

/* arithmetic */
void gen_mov(const char *dest, const char *src_val);
void gen_add(const char *res,  const char *a, const char *b);
void gen_sub(const char *res,  const char *a, const char *b);
void gen_mul(const char *res,  const char *a, const char *b);
void gen_div(const char *res,  const char *a, const char *b);

/* assignment to existing variable (re-assign) */
void gen_assign(const char *dest, const char *src_val);

/* comparison: op is "==" "!=" "<" ">" "<=" ">=" */
void gen_cmp(const char *res, const char *a, const char *b, const char *op);

/* control flow */
void gen_label(const char *label);
void gen_jump(const char *label);
void gen_jump_if_false(const char *cond_var, const char *label);

/* output */
void gen_print(const char *var);

/* label factory – returns unique label string */
char *new_label(const char *prefix);

#endif