#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"
#include "helper.h"
#include "translate.h"
#include <stdlib.h>

/*Lab5: Your implementation of lab5.*/

struct expty 
{
	Tr_exp exp; 
	Ty_ty ty;
};

struct expty expTy(Tr_exp exp, Ty_ty ty)
{
	struct expty e;

	e.exp = exp;
	e.ty = ty;

	return e;
}


Ty_ty actual_ty(Ty_ty ty)
{
	while (ty && ty->kind == Ty_name){
		ty = ty->u.name.ty;
	}
	return ty;
}

bool cmpty(Ty_ty ty1,Ty_ty ty2)
{
	Ty_ty actual1 = actual_ty(ty1);
	Ty_ty actual2 = actual_ty(ty2);
	
	if(actual1->kind == actual2->kind) {
		if(actual1->kind == Ty_record) {
			if(actual1->u.record == actual2->u.record) return 1;
			else return 0;
		}
		if (actual1->kind == Ty_array){
			if(actual1->u.array == actual2->u.array) return 1;
			else return 0;
		}
		return 1;
	}
	else {
		if((actual1->kind == Ty_nil && actual2->kind == Ty_record)
			|| (actual1->kind == Ty_record && actual2->kind == Ty_nil))
			return 1;
		else return 0;
	}
}


struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label label) {
	switch (v->kind)
	{
	case A_simpleVar:
		return transSimpleVar(venv, tenv, v, l, label);
	case A_subscriptVar:
		return transSubscriptVar(venv, tenv, v, l, label);
	case A_fieldVar:
		return transFieldVar(venv, tenv, v, l, label);
	default:
		assert(0);
	}
}

struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	switch(a->kind)
	{
		case A_varExp: 
			return transVar(venv, tenv, a->u.var, l, label);
		case A_nilExp: 
			return transNilexp(venv, tenv, a, l, label);
		case A_intExp: 
			return transIntexp(venv, tenv, a, l, label);
		case A_stringExp:
			return transStringexp(venv, tenv, a, l, label);
		case A_callExp:
			return transCallexp(venv, tenv, a, l, label);
		case A_opExp:
			return transOpexp(venv, tenv, a, l, label);
		case A_recordExp:
			return transRecordexp(venv, tenv, a, l, label);
		case A_seqExp:
			return transSeqexp(venv, tenv, a, l, label);
		case A_assignExp:
			return transAssignexp(venv, tenv, a, l, label);
		case A_ifExp:
			return transIfexp(venv, tenv, a, l, label);
		case A_whileExp:
			return transWhileexp(venv,tenv,a, l, label);
		case A_forExp:	
			return transForexp(venv,tenv,a, l, label);
		case A_breakExp:
			return transBreakexp(venv, tenv, a, l, label);
		case A_letExp:
			return transLetexp(venv,tenv,a, l, label);
		case A_arrayExp:
			return transArrayexp(venv,tenv,a, l, label);
		default:
			assert(0);
	}
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label label) {
	switch (d->kind)
	{
	case A_functionDec:
		return transFunctionDec(venv, tenv, d, l, label);
	case A_varDec:
		return transVarDec(venv, tenv, d, l, label);
	case A_typeDec:
		return transTypeDec(venv, tenv, d, l, label);
	default:
		assert(0);
	}
}

Ty_ty transTy(S_table tenv, A_ty a)
{
	switch(a->kind)
	{
		case A_nameTy:
		{
			Ty_ty namety = S_look(tenv,a->u.name);
			if(!namety)
			{
				EM_error(a->pos,"undefined type %s",S_name(a->u.name));
			}
			return Ty_Name(a->u.name,namety);
		}
		case A_recordTy:
		{
			
			Ty_fieldList Ty_recordty=Ty_FieldList(NULL,NULL);
			Ty_fieldList ty_tail=Ty_recordty;
			A_fieldList A_recordty = a->u.record;
			while(A_recordty)
			{
				Ty_ty fieldtyp = S_look(tenv,A_recordty->head->typ);
				if(!fieldtyp)
				{
					EM_error(a->pos,"undefined type %s",S_name(A_recordty->head->typ));
					fieldtyp = Ty_Int();
				}
				
				Ty_field ty_field= Ty_Field(A_recordty->head->name,fieldtyp);
				ty_tail->tail = Ty_FieldList(ty_field,NULL);
				ty_tail=ty_tail->tail;

				A_recordty = A_recordty->tail;
			}
			Ty_fieldList nullhead = Ty_recordty;
			Ty_recordty=Ty_recordty->tail;
			free(nullhead);
			return Ty_Record(Ty_recordty);
			
		}
		case A_arrayTy:
		{
			Ty_ty Ty_array = S_look(tenv,a->u.array);
			if(!Ty_array)
			{
				EM_error(a->pos,"undefined type %s",S_name(a->u.array));
				return Ty_Array(NULL);
			}
			else
				return Ty_Array(Ty_array);
		}
	}
}



struct expty transSimpleVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label label){
	E_enventry sim = S_look(venv, v->u.simple);
	
	if (sim && sim->kind == E_varEntry)
		return expTy(Tr_simpleVar(sim->u.var.access, l), actual_ty(sim->u.var.ty));
	else
		EM_error(v->pos, "undefined variable %s!", S_name(v->u.simple));
	return expTy(NULL, Ty_Int());
}

struct expty transSubscriptVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label label) {
	struct expty var = transVar(venv, tenv, v->u.subscript.var, l, label);
	struct expty exp = transExp(venv, tenv, v->u.subscript.exp, l, label);
	if(actual_ty(var.ty)->kind == Ty_array) {
		if (actual_ty(exp.ty)->kind == Ty_int)
			return expTy(Tr_subscriptVar(var.exp, exp.exp),actual_ty(var.ty->u.array));
		EM_error(v->pos,"index type is not int");
	}
	else 
		EM_error(v->pos,"array type required");
	return expTy(NULL,Ty_Int());
}

struct expty transFieldVar(S_table venv, S_table tenv, A_var v, Tr_level l, Temp_label label) {
	struct expty f = transVar(venv, tenv, v->u.field.var, l, label);
	Ty_ty ty = actual_ty(f.ty);
	if (ty->kind == Ty_record) {
		int serial = 0;
		for (Ty_fieldList l = ty->u.record; l; l = l->tail, serial++)
			if (l->head->name == v->u.field.sym) {
				return expTy(Tr_fieldVar(f.exp, serial), actual_ty(l->head->ty));
			}
		EM_error(v->u.field.var->pos, "field %s doesn't exist", S_name(v->u.field.sym));
	}
	else EM_error(v->pos, "not a record type");
	
	return expTy(NULL, Ty_Int());
}




struct expty transNilexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	return expTy(Tr_nil(), Ty_Nil());
}

struct expty transIntexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	return expTy(Tr_int(a->u.intt), Ty_Int());
}

struct expty transStringexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	return expTy(Tr_string(a->u.stringg), Ty_String());
}

struct expty transCallexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	S_symbol name = a->u.call.func;
	A_expList args = a->u.call.args;
	E_enventry func = S_look(venv, name);
	if(func == NULL || func->kind != E_funEntry) {
		EM_error(a->pos,"undefined function %s",S_name(name));
		return expTy(NULL,Ty_Void());
	}
	else {
		Ty_tyList formals= func->u.fun.formals;
		Tr_expList expList = Tr_ExpList(NULL, NULL), expListTail = expList;
		while (formals && args){
			struct expty exp = transExp(venv, tenv, args->head, l, label);
			if (!cmpty(exp.ty, formals->head)) EM_error(args->head->pos, "para type mismatch");
			expListTail->tail = Tr_ExpList(exp.exp, NULL);
			expListTail = expListTail->tail;

			formals = formals->tail;
			args = args->tail;
		}
		
		// for(;args && formals;args=args->tail,formals=formals->tail)
		// {
		// 	arg = args->head;
		// 	formal = formals->head;
		// 	struct expty exp=transExp(venv,tenv,arg);
		// 	if(!cmpty(exp.ty,formal))
		// 	{
		// 		EM_error(arg->pos,"para type mismatch");
		// 	}
		// }
		if(args != NULL)
			EM_error(a->pos,"too many params in function %s",S_name(a->u.call.func));
		
		Ty_ty ret;
		if (func->u.fun.result == NULL) ret = Ty_Void();
		else ret = actual_ty(ret);

		Tr_exp exp = Tr_call(func->u.fun.label, expList->tail, l, func->u.fun.level);

		return expTy(exp, ret);
	}
}

struct expty transOpexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	struct expty left = transExp(venv, tenv, a->u.op.left, l, label);
	struct expty right = transExp(venv, tenv, a->u.op.right, l, label);
	switch (a->u.op.oper)
	{
	case A_plusOp: case A_minusOp: case A_timesOp: case A_divideOp:
		if(actual_ty(left.ty)->kind != Ty_int)
			EM_error(a->u.op.left->pos,"integer required");
		if(actual_ty(right.ty)->kind != Ty_int)
			EM_error(a->u.op.right->pos,"integer required");
		return expTy(Tr_BinOp(left.exp, right.exp, a->u.op.oper), Ty_Int());
	case A_ltOp: case A_leOp: case A_gtOp: case A_geOp: case A_eqOp: case A_neqOp:
		if (!cmpty(left.ty,right.ty))
			EM_error(a->pos, "same type required!");
		return expTy(Tr_Condition(left.exp, right.exp, a->u.op.oper), Ty_Int());
	default:
		assert(0);
	}
}

struct expty transRecordexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	Ty_ty record_ty = actual_ty(S_look(tenv, a->u.record.typ));
	if(!record_ty) {
		EM_error(a->pos,"undefined type %s",S_name(a->u.record.typ));
		return expTy(NULL,Ty_Int());
	}
	else if(record_ty->kind != Ty_record) {
		EM_error(a->pos,"not a record type");
		return expTy(NULL, record_ty);
	}

	A_efieldList efields = a->u.record.fields;
	Ty_fieldList fields = record_ty->u.record;
	Tr_expList expfields = Tr_ExpList(NULL,NULL), exptail = expfields;
	while(efields){
		if(!fields) {
			EM_error(a->pos,"Too many efields in %s", S_name(a->u.record.typ));
			break;
		}
	
		struct expty exp = transExp(venv, tenv, efields->head->exp, l, label);
		if(!cmpty(exp.ty, fields->head->ty))
			EM_error(a->pos,"record type unmatched");
		
		exptail->tail = Tr_ExpList(exp.exp, NULL);
		exptail = exptail->tail;
		efields = efields->tail;
		fields = fields->tail;
	}

	if(fields != NULL)
		EM_error(a->pos,"Too little efields in %s",S_name(a->u.record.typ));

	return expTy(Tr_record(expfields->tail), record_ty);
}

struct expty transSeqexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	if(!a->u.seq) return expTy(NULL,Ty_Void());
	
	struct expty et;
	for (A_expList e = a->u.seq; e; e = e->tail){
		et = transExp(venv, tenv, e->head, l, label);
	}
	return et;
}

struct expty transAssignexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	A_var v = a->u.assign.var;
	if(v->kind == A_simpleVar) {
		E_enventry x = S_look(venv,v->u.simple);
		if(x->readonly == 1)
			EM_error(a->pos,"loop variable can't be assigned");
	}
	A_exp e = a->u.assign.exp;
	struct expty var = transVar(venv, tenv, v, l, label);
	struct expty exp = transExp(venv, tenv, e, l, label);
	if(!cmpty(var.ty, exp.ty))
		EM_error(a->pos,"unmatched assign exp");
	return expTy(Tr_Assign(var.exp, exp.exp), actual_ty(var.ty));
}

struct expty transIfexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	struct expty test = transExp(venv, tenv, a->u.iff.test, l, label);
	struct expty then = transExp(venv, tenv, a->u.iff.then, l, label);
	Tr_exp elsetr = NULL;
	if(a->u.iff.elsee->kind != A_nilExp) {
		struct expty elsee = transExp(venv, tenv, a->u.iff.elsee, l, label);
		if(!cmpty(then.ty,elsee.ty))
			EM_error(a->pos,"then exp and else exp type mismatch");
		elsetr = elsee.exp;
	}
	else if(then.ty->kind != Ty_void) 
		EM_error(a->pos,"if-then exp's body must produce no value");
	return expTy(Tr_If(test.exp, then.exp, elsetr),actual_ty(then.ty));
}

struct expty transWhileexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	Temp_label done = Temp_newlabel();
	struct expty test = transExp(venv, tenv, a->u.whilee.test, l, label);
	struct expty body = transExp(venv, tenv, a->u.whilee.body, l, done);
	if(body.ty->kind != Ty_void)
	{
		EM_error(a->pos,"while body must produce no value");
	}
	return expTy(Tr_While(test.exp, body.exp, done),actual_ty(body.ty));
}

struct expty transForexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	S_beginScope(venv);
	struct expty lo = transExp(venv, tenv, a->u.forr.lo, l, label);
	struct expty hi = transExp(venv, tenv, a->u.forr.hi, l, label);
	if(!cmpty(lo.ty,Ty_Int()) || !cmpty(hi.ty,Ty_Int()))
		EM_error(a->u.forr.lo->pos,"for exp's range type is not integer");
	
	Tr_access access = Tr_allocLocal(l, a->u.forr.escape);
	E_enventry i = E_VarEntry(access, lo.ty);
	i->readonly=1;
	S_enter(venv, a->u.forr.var, i);
	
	Temp_label done	= Temp_newlabel();
	struct expty body = transExp(venv, tenv, a->u.forr.body, l, done);
	if(body.ty->kind != Ty_void)
		EM_error(a->pos,"for body must produce no value");
	Tr_exp forr = Tr_For(access, lo.exp, hi.exp, body.exp, l, done);
	S_endScope(venv);
	return expTy(forr,actual_ty(body.ty));
}

struct expty transBreakexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	return expTy(Tr_Break(label), Ty_Void());
}

struct expty transLetexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	A_decList d;
	S_beginScope(venv);
	S_beginScope(tenv);
	/*Caution:semant.c shouldn't include tree.h module. Modify this later */
	Tr_exp trSeq = Tr_nil();//An empty stm
	for(d=a->u.let.decs ;d ; d=d->tail)
	{
		trSeq = Tr_Seq(trSeq,transDec(venv,tenv,d->head,l,label));
	}
	struct expty letbody=transExp(venv,tenv,a->u.let.body,l,label);
	Tr_exp trlet = Tr_Seq(trSeq,letbody.exp);
	S_endScope(venv);
	S_endScope(tenv);
	return expTy(trlet,actual_ty(letbody.ty));
}

struct expty transArrayexp(S_table venv, S_table tenv, A_exp a, Tr_level l, Temp_label label) {
	Ty_ty ty_array = actual_ty(S_look(tenv,a->u.array.typ));
	if(!ty_array)
	{
		EM_error(a->pos,"undefined type %s",S_name(a->u.array.typ));
		return expTy(NULL,Ty_Int());
	}
	if(ty_array->kind!=Ty_array)
	{
		EM_error(a->pos,"not array type");
	}
	struct expty size = transExp(venv,tenv,a->u.array.size,l,label);
	if(size.ty->kind != Ty_int)
	{
		EM_error(a->u.array.size->pos,"type of size expression should be int");
	}
	struct expty init = transExp(venv,tenv,a->u.array.init,l,label);
	if(!cmpty(init.ty,ty_array->u.array))
	{
		EM_error(a->u.array.size->pos,"type mismatch");
	}

	Tr_exp trarray = Tr_Array(size.exp,init.exp);
	return expTy(trarray,ty_array);
}



Ty_tyList makeFormalTyList(S_table tenv,A_fieldList params)
{
	A_field field;
	Ty_tyList tyList = Ty_TyList(NULL,NULL);
	Ty_tyList tail = tyList;
	Ty_ty cur;
	for(;params;params = params->tail)
	{
		field = params->head;
		cur = S_look(tenv,field->typ);
		if(!cur)
		{
			EM_error(field->pos,"undefined type %s",S_name(field->typ));
			continue;
		}
		tail->tail = Ty_TyList(cur,NULL);
		tail = tail->tail;
	}
	Ty_tyList old = tyList;
	tyList = tyList->tail;
	free(old);
	return tyList;
}

Tr_exp transFunctionDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label label) {
	A_fundecList funcs = d->u.function;
	A_fundec f;
	//Set an empty S_table to check function definition duplicate.
	S_table tocheck = S_empty();

	//First loop:push function names into venv to handle recursive function
	for(;funcs;funcs = funcs->tail)
	{
		f=funcs->head;
		if(S_look(tocheck,f->name))
		{
			EM_error(f->pos,"two functions have the same name");
			continue;
		}
		S_enter(tocheck,f->name,(void *)0);
		/*Handle the type of the formals*/
		Ty_tyList formalTys = makeFormalTyList(tenv,f->params);
		
		/*Make a new level*/
		Temp_label name = Temp_newlabel();
		U_boolList args = NULL;
		for(Ty_tyList cal_arg = formalTys;cal_arg;cal_arg = cal_arg->tail)
		{
			args = U_BoolList(TRUE,args);//Assume that all params are escaped.
		}
		Tr_level newl = Tr_newLevel(l,name,args);

		/*Enter the name of the function without handling the body of it.*/
		if(f->result)
		{
			Ty_ty resultTy = S_look(tenv,f->result);
			if(!resultTy)
			{
				EM_error(f->pos,"undefined return type %s",S_name(f->result));
				continue;
			}
			S_enter(venv,f->name,E_FunEntry(newl,name,formalTys,resultTy));
		}
		else
		{
			S_enter(venv,f->name,E_FunEntry(newl,name,formalTys,Ty_Void()));
		}
	}

	//Second loop:handle 
	for(funcs = d->u.function;funcs;funcs=funcs->tail)
	{
		f=funcs->head;
		E_enventry funentry = S_look(venv,f->name);
		Ty_tyList formalTys = funentry->u.fun.formals;
		S_beginScope(venv);

		/*Enter formals into venv */
		
		A_fieldList l; 
		Ty_tyList t;

		/*Get the access of the formal from funentry*/
		Tr_accessList accesslist = Tr_formals(funentry->u.fun.level);
		for(l=f->params,t=formalTys;l;l=l->tail,t=t->tail)
		{
			S_enter(venv,l->head->name,E_VarEntry(accesslist->head,t->head));
			accesslist = accesslist->tail;
		}
		
		/*Handle the function body.*/
		struct expty body = transExp(venv,tenv,f->body,funentry->u.fun.level,funentry->u.fun.label);
		if(!cmpty(body.ty,funentry->u.fun.result))
		{
			if(actual_ty(funentry->u.fun.result)->kind == Ty_void)
			{
				EM_error(f->pos,"procedure returns value");
			}
			EM_error(f->pos,"procedure returns unexpected type");
		}
		S_endScope(venv);
		Tr_procEntryExit(funentry->u.fun.level,body.exp, NULL);
	}
	return Tr_nil();
}

Tr_exp transVarDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label label) {
	struct expty e = transExp(venv,tenv,d->u.var.init,l,label);
	S_symbol type = d->u.var.typ;
	Tr_access access;
	if(!type)
	{
		if(e.ty == Ty_Nil())
		{
			EM_error(d->pos,"init should not be nil without type specified");
		}
		access = Tr_allocLocal(l,TRUE);
		S_enter(venv,d->u.var.var,E_VarEntry(access,e.ty));
	}
	else
	{
		Ty_ty ty_var= S_look(tenv,type);
		if(!ty_var)
		{
			EM_error(d->pos,"undefined type %s",S_name(type));
			access = Tr_allocLocal(l,TRUE);
			S_enter(venv,d->u.var.var,E_VarEntry(access,actual_ty(e.ty)));
		}
		else
		{
			if(e.ty == Ty_Nil() && actual_ty(ty_var)->kind !=Ty_record)
			{
				EM_error(d->pos,"init should not be nil without type specified");
			}

			else if(!cmpty(ty_var,e.ty) && e.ty !=Ty_Nil())
			{
				EM_error(d->pos,"type mismatch");
			}
		}
		access = Tr_allocLocal(l,TRUE);
		S_enter(venv,d->u.var.var,E_VarEntry(access,actual_ty(e.ty)));
	}
	return Tr_Assign(Tr_simpleVar(access,l),e.exp);
}

Tr_exp transTypeDec(S_table venv, S_table tenv, A_dec d, Tr_level l, Temp_label label) {
	A_nametyList cur = d->u.type;
	A_namety head;
	// add type names into tenv
	A_nametyList nlist, nlist2;
	for (nlist = d->u.type; nlist; nlist = nlist->tail)
		// check dup name
		if(S_look(tenv, nlist->head->name))
			EM_error(d->pos,"two types have the same name");
		else
			S_enter(tenv, nlist->head->name,Ty_Name(nlist->head->name,NULL));
	

	// fill type
	A_namety namety;
	for (nlist = d->u.type; nlist; nlist = nlist->tail){
		Ty_ty ty = S_look(tenv, nlist->head->name);
		ty->u.name.ty = transTy(tenv, nlist->head->ty);
	}

	// check for type cycle
	for (nlist = d->u.type; nlist; nlist = nlist->tail){
		Ty_ty init = S_look(tenv, nlist->head->name);
		Ty_ty type = init;
		while(type->kind == Ty_name){	
			type = type->u.name.ty;
			if(type == init){
				EM_error(d->pos,"illegal type cycle");
				init->u.name.ty = Ty_Int();
				break;
			}
		}
	}
	return Tr_nil();
}



F_fragList SEM_transProg(A_exp exp){

	//TODO LAB5: do not forget to add the main frame
	Tr_level mainframe = Tr_outermost();
	Temp_label mainlabel = Temp_newlabel();
	struct expty mainexp = transExp(E_base_venv(),E_base_tenv(), exp, mainframe, mainlabel);
	Tr_procEntryExit(mainframe, mainexp.exp, NULL);

	return Tr_getResult();
}

