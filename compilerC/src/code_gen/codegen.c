#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/* state  */

static FILE *out;                   /* temp instruction stream         */
static char  vars[256][64];         /* all declared variable names     */
static int   var_count = 0;
static int   label_id  = 0;        /* counter for unique labels       */

/*  helpers  */

static int is_number(const char *s) {
    if (!s || !*s) return 0;
    int i = (s[0] == '-') ? 1 : 0;
    if (!s[i]) return 0;
    for (; s[i]; i++)
        if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}

/* emit: load operand into register (eax by default) */
static void load(const char *reg, const char *operand) {
    if (is_number(operand))
        fprintf(out, "    mov %s, %s\n", reg, operand);
    else
        fprintf(out, "    mov %s, [%s]\n", reg, operand);
}

/*  lifecycle  */

void init_codegen(void) {
    out = fopen("temp.asm", "w");
    if (!out) { perror("Cannot open temp.asm"); exit(1); }
    var_count = 0;
    label_id  = 0;
}

/*  variable management  */

void declare_variable(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(vars[i], name) == 0) return;
    strncpy(vars[var_count++], name, 63);
}

/*  label factory  */

char *new_label(const char *prefix) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "%s_%d", prefix, label_id++);
    return buf;
}

/*  instruction emitters */

void gen_mov(const char *dest, const char *src_val) {
    declare_variable(dest);
    if (is_number(src_val))
        fprintf(out, "    mov dword [%s], %s\n", dest, src_val);
    else {
        fprintf(out, "    mov eax, [%s]\n", src_val);
        fprintf(out, "    mov [%s], eax\n", dest);
    }
}

void gen_assign(const char *dest, const char *src_val) {
    /* same as gen_mov but does NOT re-declare (variable already exists) */
    if (is_number(src_val))
        fprintf(out, "    mov dword [%s], %s\n", dest, src_val);
    else {
        fprintf(out, "    mov eax, [%s]\n", src_val);
        fprintf(out, "    mov [%s], eax\n", dest);
    }
}

void gen_add(const char *res, const char *a, const char *b) {
    declare_variable(res);
    load("eax", a);
    if (is_number(b)) fprintf(out, "    add eax, %s\n", b);
    else               fprintf(out, "    add eax, [%s]\n", b);
    fprintf(out, "    mov [%s], eax\n", res);
}

void gen_sub(const char *res, const char *a, const char *b) {
    declare_variable(res);
    load("eax", a);
    if (is_number(b)) fprintf(out, "    sub eax, %s\n", b);
    else               fprintf(out, "    sub eax, [%s]\n", b);
    fprintf(out, "    mov [%s], eax\n", res);
}

void gen_mul(const char *res, const char *a, const char *b) {
    declare_variable(res);
    load("eax", a);
    if (is_number(b)) {
        fprintf(out, "    mov ebx, %s\n", b);
    } else {
        fprintf(out, "    mov ebx, [%s]\n", b);
    }
    fprintf(out, "    imul eax, ebx\n");
    fprintf(out, "    mov [%s], eax\n", res);
}

void gen_div(const char *res, const char *a, const char *b) {
    declare_variable(res);
    load("eax", a);
    fprintf(out, "    cdq\n");                 /* sign-extend eax into edx */
    if (is_number(b)) {
        fprintf(out, "    mov ebx, %s\n", b);
    } else {
        fprintf(out, "    mov ebx, [%s]\n", b);
    }
    fprintf(out, "    idiv ebx\n");            /* eax = quotient           */
    fprintf(out, "    mov [%s], eax\n", res);
}

/*
 * gen_cmp  –  evaluate (a op b), store 1 or 0 into res
 * Uses setcc so no branching in the emitted code.
 */
void gen_cmp(const char *res, const char *a, const char *b, const char *op) {
    declare_variable(res);
    load("eax", a);
    if (is_number(b)) fprintf(out, "    cmp eax, %s\n", b);
    else               fprintf(out, "    cmp eax, [%s]\n", b);

    const char *setcc;
    if      (strcmp(op, "==") == 0) setcc = "sete";
    else if (strcmp(op, "!=") == 0) setcc = "setne";
    else if (strcmp(op, "<")  == 0) setcc = "setl";
    else if (strcmp(op, ">")  == 0) setcc = "setg";
    else if (strcmp(op, "<=") == 0) setcc = "setle";
    else                             setcc = "setge";   /* >= */

    fprintf(out, "    %s al\n", setcc);
    fprintf(out, "    movzx eax, al\n");
    fprintf(out, "    mov [%s], eax\n", res);
}

void gen_label(const char *label) {
    fprintf(out, "%s:\n", label);
}

void gen_jump(const char *label) {
    fprintf(out, "    jmp %s\n", label);
}

void gen_jump_if_false(const char *cond_var, const char *label) {
    fprintf(out, "    mov eax, [%s]\n", cond_var);
    fprintf(out, "    test eax, eax\n");
    fprintf(out, "    jz %s\n", label);
}

void gen_print(const char *var) {
    fprintf(out, "    mov eax, [%s]\n", var);
    fprintf(out, "    call print_int\n");
}

/* ── finish: assemble final.asm ─────────────────────────────────── */

void finish_codegen(void) {
    if (!out) return;
    fclose(out);

    FILE *final = fopen("final.asm", "w");
    if (!final) { perror("Cannot open final.asm"); return; }

    /* DATA section – one dd per variable */
    fprintf(final, "section .data\n");
    for (int i = 0; i < var_count; i++)
        fprintf(final, "    %s dd 0\n", vars[i]);

    /* TEXT section */
    fprintf(final, "\nsection .text\n");
    fprintf(final, "global _start\n\n");
    fprintf(final, "_start:\n");

    /* paste temp instructions */
    FILE *tmp = fopen("temp.asm", "r");
    if (tmp) {
        int ch;
        while ((ch = fgetc(tmp)) != EOF) fputc(ch, final);
        fclose(tmp);
    }

    /* exit syscall */
    fprintf(final, "\n    mov eax, 1\n");
    fprintf(final, "    xor ebx, ebx\n");
    fprintf(final, "    int 0x80\n");

    /* ── print_int subroutine ───────────────────────────────────────
     * Receives value in eax.
     * Converts to decimal string and writes it to stdout via sys_write,
     * followed by a newline.
     * ---------------------------------------------------------------- */
    fprintf(final, "\nprint_int:\n");
    fprintf(final, "    push ebx\n");
    fprintf(final, "    push esi\n");
    fprintf(final, "    push edi\n");
    fprintf(final, "    sub  esp, 16\n");
    fprintf(final, "    mov  esi, eax\n");          /* value to print        */
    /* handle zero explicitly */
    fprintf(final, "    test esi, esi\n");
    fprintf(final, "    jnz  .pi_nonzero\n");
    fprintf(final, "    mov  byte [esp], '0'\n");
    fprintf(final, "    mov  byte [esp+1], 10\n");
    fprintf(final, "    mov  eax, 4\n");
    fprintf(final, "    mov  ebx, 1\n");
    fprintf(final, "    lea  ecx, [esp]\n");
    fprintf(final, "    mov  edx, 2\n");
    fprintf(final, "    int  0x80\n");
    fprintf(final, "    add  esp, 16\n");
    fprintf(final, "    pop  edi\n");
    fprintf(final, "    pop  esi\n");
    fprintf(final, "    pop  ebx\n");
    fprintf(final, "    ret\n");
    fprintf(final, ".pi_nonzero:\n");
    fprintf(final, "    mov  edi, 14\n");           /* write index in buffer */
    fprintf(final, "    mov  byte [esp+15], 10\n"); /* newline at position 15*/
    fprintf(final, "    mov  ebx, 10\n");           /* divisor               */
    fprintf(final, ".pi_loop:\n");
    fprintf(final, "    xor  edx, edx\n");
    fprintf(final, "    mov  eax, esi\n");
    fprintf(final, "    div  ebx\n");
    fprintf(final, "    mov  esi, eax\n");
    fprintf(final, "    add  dl, '0'\n");
    fprintf(final, "    mov  [esp+edi], dl\n");
    fprintf(final, "    dec  edi\n");
    fprintf(final, "    test esi, esi\n");
    fprintf(final, "    jnz  .pi_loop\n");
    fprintf(final, "    inc  edi\n");               /* edi → first digit     */
    fprintf(final, "    lea  ecx, [esp+edi]\n");
    fprintf(final, "    mov  edx, 16\n");
    fprintf(final, "    sub  edx, edi\n");          /* length incl. newline  */
    fprintf(final, "    mov  eax, 4\n");
    fprintf(final, "    mov  ebx, 1\n");
    fprintf(final, "    int  0x80\n");
    fprintf(final, "    add  esp, 16\n");
    fprintf(final, "    pop  edi\n");
    fprintf(final, "    pop  esi\n");
    fprintf(final, "    pop  ebx\n");
    fprintf(final, "    ret\n");

    fclose(final);

    printf("Assembling...\n");
    if (system("nasm -f elf32 final.asm -o output.o") != 0) {
        fprintf(stderr, "Error: assembly failed.\n"); return;
    }
    printf("Assembly generated: final.asm\n");

    printf("Linking...\n");
    if (system("ld -m elf_i386 output.o -o program") != 0) {
        fprintf(stderr, "Error: linking failed.\n"); return;
    }
    printf("Executable generated: ./program\n");
}