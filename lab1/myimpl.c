#include "prog1.h"
#include <stdio.h>
#include <stdlib.h>
#define MAX(x, y) x > y? x: y

int maxargs(A_stm stm);
int maxargs_exp(A_exp exp);
int maxargs(A_stm stm);

int maxargs_exp(A_exp exp){
	switch (exp->kind) {
		case A_opExp:
			return MAX(maxargs_exp(exp->u.op.left) , maxargs_exp(exp->u.op.right));
		case A_eseqExp:
			return MAX(maxargs_exp(exp->u.eseq.exp) , maxargs(exp->u.eseq.stm));
		case A_idExp: case A_numExp: default:
			return 0;
	}

	return 0;
}

int maxargs_expList(A_expList list){
	A_expList p = list;
	int max = 0;
	while (list->kind != A_lastExpList){
		max = MAX(max, maxargs_exp(list->u.pair.head));
		list = list->u.pair.tail;
	}

	return MAX(max, maxargs_exp(list->u.last));
}

int maxargs(A_stm stm){
	A_expList list;
	int i;
	
	switch (stm->kind) {
		case A_compoundStm:
			return MAX(maxargs(stm->u.compound.stm1), maxargs(stm->u.compound.stm2));
		case A_assignStm:
			return maxargs_exp(stm->u.assign.exp);
		case A_printStm:
			list = stm->u.print.exps;
			i = 1;
			while (list->kind != A_lastExpList){
				i++; list = list->u.pair.tail;
			}
			return MAX(maxargs_expList(stm->u.print.exps) , i);
		default:
			return 0;
	}

	return 0;
}



typedef struct table *Table_;
struct table{string id; int value; Table_ tail;};

Table_ Table(string id, int value, struct table *tail){
	Table_ t = malloc(sizeof(*t));
	t->id = id; t->value = value; t->tail = tail;
	return t;
}

Table_ search(Table_ head, string key){
	Table_ t = head;
	while (t != NULL){
		if (t->id == key) break;
		else t = t->tail;
	}
	
	return t;
}

Table_ update(Table_ t, string key, int newValue){
	Table_ target = search(t, key);
	if (target == NULL) return Table(key, newValue, t);

	target->value = newValue;
	return t;
}

int lookup(Table_ t, string key){
	Table_ target = search(t, key);
	if (target == NULL) return 0;

	return target->value;
}



Table_ interp_stm(A_stm stm, Table_ t);
Table_ interp_exp(A_exp exp, Table_ t, int *value);

Table_ interp_stm(A_stm stm, Table_ t){
	A_expList list;
	Table_ res = t;
	int value;
	switch (stm->kind) {
		case A_compoundStm:
			res = interp_stm(stm->u.compound.stm1, res);
			res = interp_stm(stm->u.compound.stm2, res);
			break;
		case A_assignStm:
			res = interp_exp(stm->u.assign.exp, res, &value);
			res = update(t, stm->u.assign.id, value);
			break;
		case A_printStm:
			list = stm->u.print.exps;
			while (list->kind != A_lastExpList){
				res = interp_exp(list->u.pair.head, res, &value);
				printf("%d ", value);
				list = list->u.pair.tail;
			}
			res = interp_exp(list->u.pair.head, res, &value);
			printf("%d\n", value);
			break;
		default:
			break;
	}

	return res;
}

Table_ interp_exp(A_exp exp, Table_ t, int *value){
	Table_ res = t;
	int val1, val2;

	switch (exp->kind) {
		case A_idExp:
			*value = lookup(res, exp->u.id);
			break;
		case A_numExp:
			*value = exp->u.num;
			break;
		case A_opExp:
			res = interp_exp(exp->u.op.left, res, &val1);
			res = interp_exp(exp->u.op.right, res, &val2);
			switch (exp->u.op.oper){
				case A_plus: *value = val1 + val2; break;
				case A_minus: *value = val1 - val2; break;
				case A_times: *value = val1 * val2; break;
				case A_div: *value = val1 / val2; break;
				default: break;
			}
			break;
		case A_eseqExp:
			res = interp_stm(exp->u.eseq.stm, t);
			res = interp_exp(exp->u.eseq.exp, t, &val1);
			*value = val1;
			break;
		default:
			return 0;
	}

	return res;
}

void interp(A_stm stm)
{
	Table_ t = NULL;

	interp_stm(stm, t);

}
