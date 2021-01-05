#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "helper.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/


typedef void* Tr_exp;
struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

//In Lab4, the first argument exp should always be **NULL**.
struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}


Ty_ty actual_ty(Ty_ty ty)
{
	if (ty->kind == Ty_name) return ty->u.name.ty;
	else return ty;
}


struct expty transVar(S_table venv, S_table tenv, A_var v)
{
	switch (v->kind)
	{
	case A_simpleVar: {
		E_enventry sim = S_look(venv, v->u.simple);
		if (sim && sim->kind == E_varEntry)
			return expTy(NULL, actual_ty(sim->u.var.ty));
		else if (!sim)
			EM_error(v->pos, "undefined variable %s!", S_name(v->u.simple));
		else 
			EM_error(v->pos, "\"%s\" is not a var type!", S_name(v->u.simple));
		
		return expTy(NULL, Ty_Int());
	}

	case A_fieldVar: {
		struct expty f = transVar(venv, tenv, v->u.field.var);
		if (f.ty->kind == Ty_record) {
			for (Ty_fieldList l = f.ty->u.record; l; l = l->tail)
				if (l->head->name == v->u.field.sym)
					return expTy(NULL, actual_ty(l->head->ty));
			EM_error(v->u.field.var->pos, "field %s does not exist!", S_name(v->u.field.sym));
		}
		else EM_error(v->pos, "%s is not a record type!", S_name(v->u.field.sym));
		
		return expTy(NULL, Ty_Int());
	}

	case A_subscriptVar: {
		struct expty sub = transVar(venv, tenv, v->u.subscript.var);
		struct expty exp = transExp(venv, tenv, v->u.subscript.exp);
		if (sub.ty->kind == Ty_array) {
			if (exp.ty == Ty_Int())
				return expTy(NULL, actual_ty(exp.ty->u.array));
			EM_error(v->u.field.var->pos, "int type required for array index!", S_name(v->u.field.sym));
		}
		else EM_error(v->pos, "\"%s\" is not an array type!", S_name(v->u.subscript.var->u.simple));
		
		return expTy(NULL, Ty_Int());
	}
	
	default:
		assert(0);
	}
}

	       
struct expty transExp(S_table venv, S_table tenv, A_exp a)
{
	switch (a->kind)
	{
	case A_varExp:
		return transVar(venv, tenv, a->u.var);
	
	case A_nilExp:
		return expTy(NULL, Ty_Nil());
	
	case A_intExp:
		return expTy(NULL, Ty_Int());
	
	case A_stringExp:
		return expTy(NULL, Ty_String());		
	
	case A_callExp: {
		printf("A_callExp %s\n", S_name(a->u.call.func));
		E_enventry func = S_look(venv, a->u.call.func);
		printf("A_callExp %s get S_look %d\n", S_name(a->u.call.func), func->u.fun.formals == NULL);
		if (func && func->kind == E_funEntry) {
			// check parameters
			Ty_tyList required; A_expList actual;
			for (required = func->u.fun.formals, actual = a->u.call.args; required; 
					required = required->tail, actual = actual->tail){
				printf("%s param %d %d\n", S_name(a->u.call.func), required->head == NULL, required->tail == NULL);
				printf("%s arg %d %d\n", S_name(a->u.call.func), actual->head == NULL, actual->tail == NULL);
				if (!actual) {
					EM_error(a->pos, "expect more parameter!");
					return expTy(NULL, Ty_Int());
				}

				struct expty exp = transExp(venv, tenv, actual->head);
				printf("arg exp explored\n");
				if (exp.ty != required->head) {
					EM_error(a->pos, "parameter mismatch!");
					return expTy(NULL, Ty_Int());
				}
			}
			printf("out of loop\n");
			if (actual != NULL) {
				EM_error(a->pos, "too much parameter!");
				return expTy(NULL, Ty_Int());
			}
			return expTy(NULL, actual_ty(func->u.fun.result));
		}
		else if (!func) EM_error(a->pos, "undefined function %s!", S_name(a->u.call.func));
		else EM_error(a->pos, "\"%s\" is not a function!", S_name(a->u.call.func));
		return expTy(NULL, Ty_Int());
	}

	case A_opExp: {
		struct expty left = transExp(venv, tenv, a->u.op.left);
		struct expty right = transExp(venv, tenv, a->u.op.right);
		switch (a->u.op.oper)
		{
		case A_plusOp: case A_minusOp: case A_timesOp: case A_divideOp:
			case A_ltOp: case A_leOp: case A_gtOp: case A_geOp:
			if (left.ty->kind != Ty_int) 
				EM_error(a->u.op.left->pos, "same type required");
			if (right.ty->kind != Ty_int) 
				EM_error(a->u.op.right->pos, "same type required");
			return expTy(NULL, Ty_Int());
		case A_eqOp: case A_neqOp:
			if (left.ty != right.ty)
				EM_error(a->pos, "same type required!");
			return expTy(NULL, Ty_Int());
		default:
			assert(0);
		}
	}

	case A_recordExp: {
		Ty_ty ty = S_look(tenv, a->u.record.typ);
		if (ty) {
			// check fields
			// Ty_fieldList fields; A_efieldList efields;
			// for (fields = ty->u.record, efields = a->u.record.fields; fields; 
			// 		fields = fields->tail, efields = efields->tail){
			// 	if (!efields) {
			// 		EM_error(a->pos, "expect more fields!");
			// 		return expTy(NULL, Ty_Int());
			// 	}
			// 	else if (efields->head->name != fields->head->name) {
			// 		EM_error(a->pos, "field name mismatch!");
			// 		return expTy(NULL, Ty_Int());
			// 	}

			// 	struct expty exp = transExp(venv, tenv, efields->head->exp);
			// 	if (exp.ty != fields->head->ty) {
			// 		EM_error(a->pos, "field type mismatch!");
			// 		return expTy(NULL, Ty_Int());
			// 	}
			// }

			// if (efields != NULL) {
			// 	EM_error(a->pos, "too much fields!");
			// 	return expTy(NULL, Ty_Int());
			// }
			
			return expTy(NULL, ty);
		}
		else EM_error(a->pos, "undefined record %s!", S_name(a->u.record.typ));
		return expTy(NULL, Ty_Int());
	}

	case A_seqExp: {
		struct expty et;
		for (A_expList e = a->u.seq; e; e = e->tail){
			et = transExp(venv, tenv, e->head);
		}
		return et;
	}

	case A_assignExp: {
		struct expty var = transVar(venv, tenv, a->u.assign.var);
		struct expty exp = transExp(venv, tenv, a->u.assign.exp);
		if (var.ty != exp.ty) {
			EM_error(a->pos, "require same type while assigning!");
		}

		// check loop variable
		A_var v = a->u.assign.var;
		if (v->kind == A_simpleVar){
			E_enventry var = S_look(venv, v->u.simple);
			if (var && var->kind == E_varEntry) 
				if (var->u.var.ty->kind == Ty_name)
				//  && var->u.var.ty->u.name.sym == v->u.simple)
					EM_error(a->pos, "loop variable can't be assigned");
		}

		return expTy(NULL, Ty_Void());
	}

	case A_ifExp: {
		transExp(venv, tenv, a->u.iff.test);
		struct expty ethen = transExp(venv, tenv, a->u.iff.then);
		struct expty eelse = transExp(venv, tenv, a->u.iff.elsee);
		if (eelse.ty == Ty_Nil()) {
			if (ethen.ty != Ty_Void())
				EM_error(a->u.iff.then->pos, "if-then exp's body must produce no value");
			return expTy(NULL, Ty_Void());
		}
		if (ethen.ty !=  eelse.ty) {
			EM_error(a->u.iff.elsee->pos, "require same type for then exp and else exp!");
			return expTy(NULL, Ty_Void());
		}
		return expTy(NULL, eelse.ty);
	}

	case A_whileExp: {
		transExp(venv, tenv, a->u.whilee.test);
		struct expty body = transExp(venv, tenv, a->u.whilee.body);
		if (body.ty != Ty_Void())
			EM_error(a->u.whilee.body->pos, "while body must produce no value");
		return expTy(NULL, Ty_Void());
	}

	case A_forExp: {
		// check range exp
		struct expty lo = transExp(venv, tenv, a->u.forr.lo);
		if (lo.ty != Ty_Int())
			EM_error(a->u.forr.lo->pos, "for exp's range type is not integer");
		struct expty hi = transExp(venv, tenv, a->u.forr.hi);
		if (hi.ty != Ty_Int())
			EM_error(a->u.forr.hi->pos, "for exp's range type is not integer");
		
		// check body exp
		S_beginScope(venv);
		S_enter(venv, a->u.forr.var, E_VarEntry(Ty_Name(a->u.forr.var, Ty_Int())));
		struct expty body = transExp(venv, tenv, a->u.forr.body);
		if (body.ty != Ty_Void()) 
			EM_error(a->u.forr.body->pos, "require void type for \"for\" body exp!");
		S_endScope(venv);
		return expTy(NULL, Ty_Void());
	}

	case A_breakExp:
		// check break position?
		return expTy(NULL, Ty_Void());
		
	case A_letExp: {
		struct expty exp;
		A_decList list;
		S_beginScope(venv);
		S_beginScope(tenv);
		for (list = a->u.let.decs; list; list = list->tail)
			transDec(venv, tenv, list->head);
		exp = transExp(venv, tenv, a->u.let.body);
		S_endScope(tenv);
		S_endScope(venv);
		return exp;
	}

	case A_arrayExp: {
		Ty_ty ty = S_look(tenv, a->u.array.typ);
		struct expty size = transExp(venv, tenv, a->u.array.size);
		if (size.ty != Ty_Int())
			EM_error(a->u.array.size->pos, "require Int for array size!");
		struct expty init = transExp(venv, tenv, a->u.array.init);
		if (init.ty != actual_ty(ty)) 
			EM_error(a->u.array.init->pos, "same type required");
		return expTy(NULL, ty);
	}

	default:
		assert(0);
	}
}


Ty_tyList transFormals(S_table tenv, A_fundec func) {
	A_fieldList flist;
	Ty_tyList tlist = NULL, tlist_tail = NULL;

	for (flist = func->params; flist; flist = flist->tail) {
		Ty_ty ty =  S_look(tenv, flist->head->typ);
		if (!ty) {
			EM_error(flist->head->pos, "undefined type!");;
			ty = Ty_Int();
		}

		if (tlist == NULL)
			tlist = tlist_tail = Ty_TyList(ty, NULL);
		else {
			tlist_tail->tail = Ty_TyList(ty, NULL);
			tlist_tail = tlist_tail->tail;
		}
	}
	
	return tlist;
}


void transDec(S_table venv, S_table tenv, A_dec d){
	switch (d->kind)
	{
	case A_functionDec: {
		A_fundecList flist;
		A_fundec func;
		for (flist = d->u.function; flist; flist = flist->tail) {
			func = flist->head;

			// check duplicate function name
			for (A_fundecList flist2 = d->u.function; flist2 != flist; flist2 = flist2->tail) {
				if (flist2->head->name == func->name) {
					EM_error(func->pos, "duplicate function name!");;
					break;
				}
			}

			// trans formals and results
			Ty_tyList formals = transFormals(tenv, func);
			Ty_ty result;
			if (func->result) {
				result = S_look(tenv, func->result);
				if (!result) {
					EM_error(func->pos, "undefined return type!");;
					result = Ty_Int();
				}
			}
			else result = Ty_Void();

			// add to value table
			S_enter(venv, flist->head->name, E_FunEntry(formals, result));

			// check function body
			A_fieldList fields;
			Ty_tyList tys;
			S_beginScope(venv);
			for (fields = func->params, tys = formals; fields; fields = fields->tail, tys = tys->tail)
				S_enter(venv, fields->head->name, E_VarEntry(tys->head));
			struct expty exp = transExp(venv, tenv, func->body);
			if (exp.ty != result)
				EM_error(func->pos, "function return type mismatch!");
			// other return type check?
			S_endScope(venv);
		}
		break;
	}
		
	case A_varDec: {
		struct expty exp = transExp(venv, tenv, d->u.var.init);
		Ty_ty ty;
		if (d->u.var.typ){	
			ty = S_look(tenv, d->u.var.typ);

			// check if type exist
			if (!ty){
				EM_error(d->pos, "undefined type %s!", S_name(d->u.var.typ));
				ty = Ty_Int();
			}

			// check if type consist with init value
			if (exp.ty != ty) {
				EM_error(d->u.var.init->pos, "type mismatch with declear type!");
				ty = exp.ty;
			}
		}
		else if (exp.ty == Ty_Nil()){
			EM_error(d->pos, "init value cannot be nil in short var declear!");
			ty = Ty_Int();
		}
		S_enter(venv, d->u.var.var, E_VarEntry(ty));
		break;
	}
	
	case A_typeDec: {
		// add type names first
		A_nametyList nlist, nlist2;
		for (nlist = d->u.type; nlist; nlist = nlist->tail){
			// check dup name
			for (nlist2 = d->u.type; nlist2 != nlist; nlist2 = nlist2->tail)
				if (nlist2->head->name == nlist->head->name){
					EM_error(d->pos, "duplicate type name!");
					break;
				}
			
			S_enter(tenv, nlist->head->name, Ty_Name(nlist->head->name, NULL));
		}
		
		// fill type define
		A_namety namety;
		for (nlist = d->u.type; nlist; nlist = nlist->tail){
			Ty_ty ty = S_look(tenv, nlist->head->name);
			ty->u.name.ty = transTy(tenv, nlist->head->ty);
		}

		// check for type cycle
		bool cycle = 0;
		for (nlist = d->u.type; nlist; nlist = nlist->tail){
			Ty_ty ty = S_look(tenv, nlist->head->name), ty2 = ty;
			while (ty2->kind == Ty_name){
				ty2 = ty->u.name.ty;
				if (ty2 == ty) {
					EM_error(d->pos, "type cycle detected!");
					cycle = 1;
					break;
				}
			}
			if (cycle) break;
		}

		break;
	}
		
	default:
		assert(0);
	}
}


Ty_ty transTy (S_table tenv, A_ty a){
	switch (a->kind)
	{
	case A_nameTy: {
		Ty_ty ty = S_look(tenv, a->u.name);
		if (!ty) {
			EM_error(a->pos, "undefined type %s!", S_name(a->u.name));
			return Ty_Int();
		}
		else return ty;
	}
		
	case A_recordTy: {
		A_fieldList flist;
		Ty_fieldList tlist = NULL;
		for (flist = a->u.record; flist; flist = flist->tail){
			Ty_ty t = S_look(tenv, flist->head->typ);
			if (!t) {
				EM_error(a->pos, "undefined type %s!", S_name(flist->head->name));
				t = Ty_Int();
			}
			tlist = Ty_FieldList(Ty_Field(flist->head->name, t), tlist);
		}
		return Ty_Record(tlist);
	}
		
	case A_arrayTy: {
		Ty_ty ty = S_look(tenv, a->u.array);
		if (!ty) {
			EM_error(a->pos, "undefined type %s!", S_name(a->u.array));
			ty = Ty_Int();
		}
		return Ty_Array(ty);
	}
		
	default:
		break;
	}
}

void SEM_transProg(A_exp exp){
	transExp(E_base_venv(), E_base_tenv(), exp);
}