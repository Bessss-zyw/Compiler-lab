#ifndef __SEMANT_H_
#define __SEMANT_H_

#include "absyn.h"
#include "symbol.h"
#include <stdlib.h>
struct expty;

struct expty transVar(S_table venv, S_table tenv, A_var v);
struct expty transExp(S_table venv, S_table tenv, A_exp a);
void		 transDec(S_table venv, S_table tenv, A_dec d);
Ty_ty		 transTy (              S_table tenv, A_ty a);

void SEM_transProg(A_exp exp);
Ty_ty actual_ty(Ty_ty ty);
bool cmpty(Ty_ty ty1,Ty_ty ty2);

struct expty transCallexp(S_table venv, S_table tenv, A_exp a);
struct expty transOpexp(S_table venv, S_table tenv, A_exp a);
struct expty transRecordexp(S_table venv, S_table tenv, A_exp a);
struct expty transSeqexp(S_table venv, S_table tenv, A_exp a);
struct expty transAssignexp(S_table venv, S_table tenv, A_exp a);
struct expty transIfexp(S_table venv, S_table tenv, A_exp a);
struct expty transWhileexp(S_table venv, S_table tenv, A_exp a);
struct expty transForexp(S_table venv, S_table tenv, A_exp a);
struct expty transLetexp(S_table venv, S_table tenv, A_exp a);
struct expty transArrayexp(S_table venv, S_table tenv, A_exp a);


#endif
