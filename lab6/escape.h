#ifndef ESCAPE_H
#define ESCAPE_H
#include "symbol.h"
#include "absyn.h"

typedef struct E_escentry_ *E_escentry;

struct E_escentry_ {
	int d;      // the depth of the function this variable is in
    bool *e;
};

static E_escentry E_Escentry(int d, bool *escape);

static void tranverseExp(S_table e, int d, A_exp exp);
static void tranverseVar(S_table e, int d, A_var var);
static void tranverseDec(S_table e, int d, A_dec dec);

void Esc_findEscape(A_exp exp);

#endif
