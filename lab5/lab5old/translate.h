#ifndef TRANSLATE_H
#define TRANSLATE_H

#include "util.h"
#include "absyn.h"
#include "temp.h"
#include "frame.h"

/* Lab5: your code below */

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_expList_ *Tr_expList;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

typedef struct Tr_level_ *Tr_level;

typedef struct patchList_ *patchList;

struct Tr_expList_
{
    Tr_exp head;
    Tr_expList tail;
};

struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;	
};

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);
Tr_access Tr_Access(Tr_level level, F_access access);
Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);
Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);
Tr_accessList Tr_formals(Tr_level level);
Tr_access Tr_allocLocal(Tr_level level, bool escape);
void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);
F_fragList Tr_getResult(void);

Tr_exp Tr_simpleVar(Tr_access access, Tr_level level);
Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp serial);
Tr_exp Tr_fieldVar(Tr_exp base, int serial);
Tr_exp Tr_nil();
Tr_exp Tr_int(int value);
Tr_exp Tr_string(string value);
Tr_exp Tr_break(Temp_label done);
Tr_exp Tr_call(Temp_label func, Tr_expList args, Tr_level cur_level, Tr_level func_level);
Tr_exp Tr_BinOp(Tr_exp left, Tr_exp right, A_oper op);
Tr_exp Tr_Condition(Tr_exp left, Tr_exp right, A_oper op);
Tr_exp Tr_record(Tr_expList efieldList);

// need future check
Tr_exp Tr_Assign(Tr_exp var,Tr_exp exp);
Tr_exp Tr_If(Tr_exp test,Tr_exp then,Tr_exp elsee);
Tr_exp Tr_While(Tr_exp test,Tr_exp body,Temp_label done);
Tr_exp Tr_For(Tr_access loopv,Tr_exp lo, Tr_exp hi, Tr_exp body,Tr_level l ,Temp_label done);
Tr_exp Tr_Break(Temp_label done);
Tr_exp Tr_Array(Tr_exp size,Tr_exp init);
Tr_exp Tr_Seq(Tr_exp,Tr_exp);

#endif
