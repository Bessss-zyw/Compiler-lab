#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "escape.h"
#include "table.h"

// implemented by BOZ

void Esc_findEscape(A_exp exp) {
	//your code here
    printf("-----------begin Esc_findEscape----------\n");
	S_table e = S_empty();
	tranverseExp(e, 0, exp);
    printf("-----------end Esc_findEscape----------\n");
}

static E_escentry E_Escentry(int d, bool *escape) {
	E_escentry entry = checked_malloc(sizeof(*entry));
	entry->d = d;
	entry->e = escape;
	return entry;
}


static void tranverseExp(S_table e, int d, A_exp exp) {
	switch (exp->kind) {
		case A_varExp:
			tranverseVar(e, d, exp->u.var);
			break;
		case A_nilExp: case A_intExp: case A_stringExp: case A_breakExp:
			break;
		case A_callExp: 
		{	
			A_expList args = exp->u.call.args;
			for (; args; args = args->tail)
				tranverseExp(e, d, args->head);
			break;
		}
		case A_opExp:
			tranverseExp(e, d, exp->u.op.left);
			tranverseExp(e, d, exp->u.op.right);
			break;
		case A_recordExp:
		{	
			A_efieldList fields = exp->u.record.fields;
			for (; fields; fields = fields->tail)
				tranverseExp(e, d, fields->head->exp);
			break;
		}
		case A_seqExp:
		{	
			A_expList seqs = exp->u.seq;
			for (; seqs; seqs = seqs->tail)
				tranverseExp(e, d, seqs->head);
			break;
		}
		case A_assignExp:	
			tranverseVar(e, d, exp->u.assign.var);
			tranverseExp(e, d, exp->u.assign.exp);
			break;
		case A_ifExp:
			tranverseExp(e, d, exp->u.iff.test);
			tranverseExp(e, d, exp->u.iff.then);
			if (exp->u.iff.elsee) tranverseExp(e, d, exp->u.iff.elsee);
			break;
		case A_whileExp:
			tranverseExp(e, d, exp->u.whilee.test);
			tranverseExp(e, d, exp->u.whilee.body);
			break;
		case A_forExp:
			tranverseExp(e, d, exp->u.forr.lo);
			tranverseExp(e, d, exp->u.forr.hi);
			S_enter(e, exp->u.forr.var, E_Escentry(d, &(exp->u.forr.escape)));
			tranverseExp(e, d, exp->u.forr.body);
			break;
		case A_letExp:
		{	
			A_decList decs = exp->u.let.decs;
			for (; decs; decs = decs->tail)
				tranverseDec(e, d, decs->head);
			tranverseExp(e, d, exp->u.let.body);
			break;
		}
		case A_arrayExp:
			tranverseExp(e, d, exp->u.array.size);
			tranverseExp(e, d, exp->u.array.init);
			break;
		default:
			assert(0);
	}
}


static void tranverseVar(S_table e, int d, A_var var) {
	switch (var->kind) {
		case A_simpleVar:
		{	
			E_escentry entry = S_look(e, var->u.simple);
			if (entry->d < d) *(entry->e) = TRUE;
			break;
		}
		case A_fieldVar:
			tranverseVar(e, d, var->u.field.var);
			break;
		case A_subscriptVar:
			tranverseVar(e, d, var->u.subscript.var);
			tranverseExp(e, d, var->u.subscript.exp);
			break;
		default:
			assert(0);
	}
}


static void tranverseDec(S_table e, int d, A_dec dec) {
	switch (dec->kind) {
		case A_functionDec:
		{
			A_fundecList funcList = dec->u.function;
			for (; funcList; funcList = funcList->tail) {
				S_beginScope(e);
				
				// add formals into env
				A_fundec func = funcList->head;
				A_fieldList fieldList = func->params;
				for (; fieldList; fieldList = fieldList->tail) {
					A_field field = fieldList->head;
					field->escape = FALSE;
					S_enter(e, field->name, E_Escentry(d + 1, &(field->escape)));
				}
				
				// tranverse func body
				tranverseExp(e, d + 1, func->body);
				
				S_endScope(e);
			}
			break;
		}	
		case A_varDec:
		{
			tranverseExp(e, d, dec->u.var.init);
			dec->u.var.escape = FALSE;
			S_enter(e, dec->u.var.var, E_Escentry(d, &(dec->u.var.escape)));
			break;
		}
		case A_typeDec:
			break;
		default:
			assert(0);
	}
}
