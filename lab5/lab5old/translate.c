#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

//LAB5: you can modify anything you want.
static struct F_fragList_ fragList_ = {NULL, NULL}; 
F_fragList fragList = &fragList_, fragtail = &fragList_;

struct Tr_access_ {
	Tr_level level;
	F_access access;
};
struct Tr_level_ {
	F_frame frame;
	Tr_level parent;
};
struct patchList_ 
{
	Temp_label *head; 
	patchList tail;
};
struct Cx 
{
	patchList trues; 
	patchList falses; 
	T_stm stm;
};
struct Tr_exp_ {
	enum {Tr_ex, Tr_nx, Tr_cx} kind;
	union {T_exp ex; T_stm nx; struct Cx cx; } u;
};


static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);

T_stm Tr_mk_record_array(Tr_expList fields,Temp_temp r,int offset,int size);


Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
	Tr_expList list = (Tr_expList)checked_malloc(sizeof(struct Tr_expList_));
	list->head = head;
	list->tail = tail;
	return list;
}

Tr_access Tr_Access(Tr_level level, F_access access) {
	Tr_access acc = (Tr_access)checked_malloc(sizeof(struct Tr_access_));
	acc->level = level;
	acc->access = access;
	return acc;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
	Tr_accessList list = (Tr_accessList)checked_malloc(sizeof(struct Tr_accessList_));
	list->head = head;
	list->tail = tail;
	return list;
}

static patchList PatchList(Temp_label *head, patchList tail)
{
	patchList list;

	list = (patchList)checked_malloc(sizeof(struct patchList_));
	list->head = head;
	list->tail = tail;
	return list;
}



void doPatch(patchList tList, Temp_label label)
{
	for(; tList; tList = tList->tail)
		*(tList->head) = label;
}

patchList joinPatch(patchList first, patchList second)
{
	if(!first) return second;
	for(; first->tail; first = first->tail);
	first->tail = second;
	return first;
}



static Tr_exp Tr_Ex(T_exp exp) {
	Tr_exp ex = (Tr_exp)checked_malloc(sizeof(ex));
	ex->kind = Tr_ex;
	ex->u.ex = exp;
	return ex;
}

static Tr_exp Tr_Nx(T_stm stm) {
	Tr_exp nx = (Tr_exp)checked_malloc(sizeof(nx));
	nx->kind = Tr_nx;
	nx->u.nx = stm;
	return nx;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
	Tr_exp cx = (Tr_exp)checked_malloc(sizeof(cx));
	cx->kind = Tr_ex;
	cx->u.cx.trues = trues;
	cx->u.cx.falses = falses;
	cx->u.cx.stm = stm;
	return cx;
}



static T_exp unEx(Tr_exp e) {
	switch (e->kind)
	{
	case Tr_ex:
		return e->u.ex;
	case Tr_nx:
		return T_Eseq(e->u.nx, T_Const(0));
	case Tr_cx: 
		{
			Temp_temp r = Temp_newtemp();
			Temp_label t = Temp_newlabel(), f = Temp_newlabel();
			doPatch(e->u.cx.trues, t);
			doPatch(e->u.cx.falses, f);
			return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
							T_Eseq(e->u.cx.stm, 
								T_Eseq(T_Label(f),
									T_Eseq(T_Move(T_Temp(r), T_Const(0)),
										T_Eseq(T_Label(t),
											T_Temp(r))))));
		}
	default:
		assert(0);
	}
}

static T_stm unNx(Tr_exp e) {
	switch (e->kind)
	{
	case Tr_ex:
		return T_Exp(e->u.ex);
	case Tr_nx:
		return e->u.nx;
	case Tr_cx: 
		{
			Temp_label l = Temp_newlabel();
			doPatch(e->u.cx.trues, l);
			doPatch(e->u.cx.falses, l);
			return T_Seq(e->u.cx.stm, T_Label(l));
		}
	default:
		assert(0);
	}
}

static struct Cx unCx(Tr_exp e) {
	switch (e->kind)
	{
	case Tr_ex:
		{
			struct Cx cx;
			return cx;
		}
	case Tr_cx:
		return e->u.cx;
	case Tr_nx:
	default:
		assert(0);
	}
}

// below need future check
Tr_level Tr_outermost(void) {
	static struct Tr_level_ outermost;
	outermost.frame = F_newFrame(Temp_namedlabel("main"),NULL);
	outermost.parent = NULL;
	return &outermost; 
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
	Tr_level level = checked_malloc(sizeof(*level));
	U_boolList link_added_formals = U_BoolList(TRUE,formals);
	F_frame frame = F_newFrame(name,link_added_formals);
	level->frame = frame;
	level->parent = parent;
	return level;
}

Tr_accessList Tr_formals(Tr_level level) {
	Tr_accessList accesslst = Tr_AccessList(NULL,NULL);
	Tr_accessList tail = accesslst;
	for(F_accessList faccesslst= level->frame->formals;faccesslst;faccesslst=faccesslst->tail)
	{
		tail->tail = Tr_AccessList(Tr_Access(level, faccesslst->head),NULL);
		tail = tail->tail;
	}
	accesslst = accesslst->tail;
	return accesslst;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
	return Tr_Access(level, F_allocLocal(level->frame, escape));
}

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals) {
	T_stm stm = T_Move(T_Temp(F_RAX()), unEx(body));
	F_frag head = F_ProcFrag(stm,level->frame);
	//The added frag is the head of the new frags. 
	fragtail->tail = F_FragList(head,NULL);
	fragtail = fragtail->tail;
}

F_fragList Tr_getResult(void) {
	fragList = fragList->tail;
	return fragList;
}
// up need future check



Tr_exp Tr_simpleVar(Tr_access access, Tr_level level) {
	T_exp frame_ptr = T_Temp(F_FP());
	while (level != access->level) {
		frame_ptr = T_Mem(T_Binop(T_plus, frame_ptr, T_Const(-wordsize)));
		level = level->parent;
	}
	return Tr_Ex(F_accessExp(access->access, frame_ptr));
}

Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp serial) {
	if (base->kind != Tr_ex || serial->kind != Tr_ex) 
		printf("Error: subscriptVar's loc or subscript must be an expression\n");
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), 
				T_Binop(T_mul, unEx(serial), T_Const(wordsize)))));
}

Tr_exp Tr_fieldVar(Tr_exp base, int serial) {
	if (base->kind != Tr_ex) 
		printf("Error: fieldVar's loc must be an expression\n");
	return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), 
				T_Binop(T_mul, T_Const(serial), T_Const(wordsize)))));
}

Tr_exp Tr_nil() {
	return Tr_Ex(T_Const(0));
}

Tr_exp Tr_int(int value) {
	return Tr_Ex(T_Const(value));
}

Tr_exp Tr_string(string value) {
	F_frag frag =  F_StringFrag(Temp_newlabel(), value);
	fragtail->tail = F_FragList(frag, NULL);
	fragtail = fragtail->tail;
	return Tr_Ex(T_Name(frag->u.stringg.label));
}

Tr_exp Tr_call(Temp_label func, Tr_expList args, Tr_level cur_level, Tr_level func_level) {
	// get static link
	T_exp slink = T_Temp(F_FP());
	while (cur_level != func_level->parent) {
		slink = T_Mem(T_Binop(T_plus, slink, T_Const(wordsize)));
		cur_level = cur_level->parent;
	}

	// make argList
	T_expList exps = T_ExpList(slink, NULL), exptail = exps;
	while (args) {
		exptail->tail = T_ExpList(unEx(args->head), NULL);
		exptail = exptail->tail;
	}
	
	return Tr_Ex(T_Call(T_Name(func), exps));
}

Tr_exp Tr_BinOp(Tr_exp left, Tr_exp right, A_oper op) {
	switch (op)
	{
	case A_plusOp: 
		return Tr_Ex(T_Binop(T_plus, unEx(left), unEx(right)));
	case A_minusOp: 
		return Tr_Ex(T_Binop(T_minus, unEx(left), unEx(right)));
	case A_timesOp: 
		return Tr_Ex(T_Binop(T_mul, unEx(left), unEx(right)));
	case A_divideOp:
		return Tr_Ex(T_Binop(T_div, unEx(left), unEx(right)));
	default:
		assert(0);
	}
}

Tr_exp Tr_Condition(Tr_exp left, Tr_exp right, A_oper op) {
	T_stm stm;
	switch (op)
	{
	case A_ltOp: 
		stm = T_Cjump(T_lt, unEx(left), unEx(right), NULL, NULL);
	case A_leOp: 
		stm = T_Cjump(T_le, unEx(left), unEx(right), NULL, NULL);
	case A_gtOp: 
		stm = T_Cjump(T_gt, unEx(left), unEx(right), NULL, NULL);
	case A_geOp: 
		stm = T_Cjump(T_ge, unEx(left), unEx(right), NULL, NULL);
	case A_eqOp: 
		stm = T_Cjump(T_eq, unEx(left), unEx(right), NULL, NULL);
	case A_neqOp:
		stm = T_Cjump(T_ne, unEx(left), unEx(right), NULL, NULL);
	default:
		assert(0);
	}

	patchList t = PatchList(&stm->u.CJUMP.true, NULL);
	patchList f = PatchList(&stm->u.CJUMP.false, NULL);
	return Tr_Cx(t, f, stm);
}

Tr_exp Tr_record(Tr_expList efieldList) {
	int field_count = 0;
	Tr_expList temp = efieldList;
	while (temp) {
		field_count++;
		temp = temp->tail;
	}

	Temp_temp r = Temp_newtemp();
	T_stm call = T_Move(
					T_Temp(r), 
					F_externalCall("malloc", 
						T_ExpList(T_Binop(T_mul, T_Const(field_count), T_Const(wordsize)), NULL)
					)
				);
	call = T_Seq(call, Tr_mk_record_array(efieldList, r, 0, field_count));
	return Tr_Ex(T_Eseq(call, T_Temp(r)));
}

T_stm Tr_mk_record_array(Tr_expList fields,Temp_temp r,int offset,int size)
{
	if(size > 1)
		if(offset < size-2) 
			return T_Seq(
				T_Move(T_Binop(T_plus,T_Temp(r),T_Const(offset*wordsize)), unEx(fields->head)),
				Tr_mk_record_array(fields->tail,r,offset+1,size)
			);
		else 
			return T_Seq(
				T_Move(T_Binop(T_plus,T_Temp(r),T_Const(offset*wordsize)), unEx(fields->head)),
				T_Move(T_Binop(T_plus,T_Temp(r),T_Const((offset+1)*wordsize)), unEx(fields->tail->head))
			);
	else
		return T_Move(T_Binop(T_plus,T_Temp(r),T_Const(offset*wordsize)), unEx(fields->head));
}



// below need future check
Tr_exp Tr_Assign(Tr_exp var,Tr_exp exp)
{
	return Tr_Nx(T_Move(unEx(var),unEx(exp)));
}

Tr_exp Tr_If(Tr_exp test,Tr_exp then,Tr_exp elsee)
{
	struct Cx test_ = unCx(test);
	Temp_temp r = Temp_newtemp();
	Temp_label truelabel = Temp_newlabel();
	Temp_label falselabel = Temp_newlabel();
	doPatch(test_.trues,truelabel);
	doPatch(test_.falses,falselabel);
	if(elsee)
	{
		Temp_label meeting = Temp_newlabel();
		T_exp e = 
		T_Eseq(test_.stm,
			T_Eseq(T_Label(truelabel),
				T_Eseq(T_Move(T_Temp(r),unEx(then)),
					T_Eseq(T_Jump(T_Name(meeting),Temp_LabelList(meeting,NULL)),
						T_Eseq(T_Label(falselabel),
							T_Eseq(T_Move(T_Temp(r),unEx(elsee)),
								T_Eseq(T_Jump(T_Name(meeting),Temp_LabelList(meeting,NULL)),
									T_Eseq(T_Label(meeting),
										T_Temp(r)))))))));
		return Tr_Ex(e);
	}
	else
	{
		T_stm s = 
		T_Seq(test_.stm,
			T_Seq(T_Label(truelabel),
				T_Seq(unNx(then),T_Label(falselabel))));
		return Tr_Nx(s);
	}
}

Tr_exp Tr_While(Tr_exp test,Tr_exp body,Temp_label done)
{
	Temp_label bodyy = Temp_newlabel(), tst = Temp_newlabel();
	struct Cx test_ = unCx(test);
	doPatch(test_.trues,bodyy);
	doPatch(test_.falses,done);
	T_stm s = T_Seq(T_Label(tst),
	T_Seq(test_.stm,
		T_Seq(T_Label(bodyy),
			T_Seq(unNx(body),
				T_Seq(T_Jump(T_Name(tst),Temp_LabelList(tst,NULL)),
					T_Label(done))))));
	return Tr_Nx(s);
}

Tr_exp Tr_For(Tr_access loopv,Tr_exp lo,Tr_exp hi,Tr_exp body,Tr_level l,Temp_label done)
{
	//Caution:check lo<=hi FIRST!
	//check if i<hi before i++;
	Temp_label bodylabel = Temp_newlabel();
	Temp_label incloop_label = Temp_newlabel();

	//Stm makes i++
	T_stm incloop;//Stm that makes i++;
	T_exp loopvar = F_accessExp(loopv->access,T_Temp(F_FP()));
	incloop = T_Move(loopvar,
		T_Binop(T_plus,loopvar,T_Const(1)));

	/*if(i < hi) {i++; goto body;}*/
	T_stm test = T_Seq(T_Cjump(T_le,unEx(Tr_simpleVar(loopv,l)),unEx(hi),incloop_label,done),
	T_Seq(T_Label(incloop_label),
		T_Seq(incloop,
			T_Jump(T_Name(bodylabel),Temp_LabelList(bodylabel,NULL)))));

	//Test if lo<=hi;
	T_stm checklohi = T_Cjump(T_le,unEx(lo),unEx(hi),bodylabel,done);

	//Concatenate together.
	T_stm forr = T_Seq(checklohi,
	T_Seq(T_Label(bodylabel),
		T_Seq(unNx(body),
			T_Seq(test,T_Label(done)))));
	
	return Tr_Nx(forr);
}

Tr_exp Tr_Break(Temp_label done)
{
	return Tr_Nx(T_Jump(T_Name(done),Temp_LabelList(done,NULL)));
}

Tr_exp Tr_Array(Tr_exp size,Tr_exp init)
{
	//Call initArray to create an array.
	T_exp callinitArray = F_externalCall("initArray",
	T_ExpList(unEx(size),
		T_ExpList(unEx(init),NULL)));
	return Tr_Ex(callinitArray);
}

Tr_exp Tr_Seq(Tr_exp left, Tr_exp right) {
	T_exp e;
	if (right)
		e = T_Eseq(unEx(left), unEx(right));
	else 
		e = T_Eseq(unEx(left), T_Const(0));
	return Tr_Ex(e);
}

// up need future check

