#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/


//varibales
struct F_access_ {
	enum {inFrame, inReg} kind;
	union {
		int offset; //inFrame
		Temp_temp reg; //inReg
	} u;
};


// registers
static Temp_temp rbp;
static Temp_temp rsp;
static Temp_temp rax;
static Temp_temp rdi;
static Temp_temp rsi;
static Temp_temp rdx;
static Temp_temp rcx;
static Temp_temp r8;
static Temp_temp r9;



F_access InReg(Temp_temp reg) {
	F_access acc = checked_malloc(sizeof(*acc));
	acc->kind = inReg;
	acc->u.reg = reg;
	return acc;
}

F_access InFrame(int offset) {
	F_access acc = checked_malloc(sizeof(*acc));
	acc->kind = inFrame;
	acc->u.offset = offset;
	return acc;
}

static F_frame F_Frame(Temp_label name, F_accessList formals, F_accessList locals, T_stmList view_shift, int s_size) {
	F_frame frame = checked_malloc(sizeof(*frame));
	frame->name = name;
	frame->formals = formals;
	frame->locals = locals;
	frame->view_shift = view_shift;
	frame->s_size = s_size;
	return frame;
}

static Temp_temp paramReg(int num) {
	switch (num)
	{
	case 0: return F_RDI();
	case 1: return F_RSI();
	case 2: return F_RDX();
	case 3: return F_RCX();
	case 4: return F_R8();
	case 5: return F_R9();
	default: return NULL;
	}
}



Temp_label F_name(F_frame f)
{
	return f->name;
}

F_accessList F_formals(F_frame f)
{
	return f->formals;
}



F_frag F_StringFrag(Temp_label label, string str) {   
	int size = strlen(str);
	char *data = checked_malloc(size + 1);
	strncmp(data, str, size);
	str[size] = '\0';

	F_frag frag = checked_malloc(sizeof(*frag));
	frag->kind = F_stringFrag;
	frag->u.stringg.label = label;
	frag->u.stringg.len = size;
	frag->u.stringg.str = data;
	
	return frag;                                      
}                                                     
                                                      
F_frag F_ProcFrag(T_stm body, F_frame frame) {        
	F_frag frag = checked_malloc(sizeof(*frag));
	frag->kind = F_procFrag;
	frag->u.proc.body = body;
	frag->u.proc.frame = frame;
	
	return frag;                                       
}                                                     



F_fragList F_FragList(F_frag head, F_fragList tail) { 
	F_fragList list = checked_malloc(sizeof(*list));
	list->head = head;
	list->tail = tail;
	return list;                                      
}  

F_accessList F_AccessList(F_access head, F_accessList tail) {
	F_accessList list = checked_malloc(sizeof(*list));
	list->head = head;
	list->tail = tail;
	return list;
}

T_exp F_externalCall(string s, T_expList args) {
	return T_Call(T_Name(Temp_namedlabel(s)), args);
}



F_access F_allocLocal(F_frame frame, bool escape) {
	// alloc in frame
	if (escape) {	
		frame->s_size -= wordsize;
		return InFrame(frame->s_size);
	}
	// alloc in register
	else {			
		return InReg(Temp_newtemp());
	}
}

F_frame F_newFrame(Temp_label label, U_boolList escapes) {
	F_accessList formals = F_AccessList(NULL, NULL), ftail = formals;
	T_stmList view_shift = T_StmList(NULL, NULL), vtail = view_shift;
	int s_size = 0, pnum = 0, formal_off = 8;

	while (escapes) {
		// alloc in frame
		if (escapes->head) {
			if (pnum >= 6) {
				s_size -= wordsize;
				vtail->tail = T_StmList(T_Move(
					T_Mem(T_Binop(T_plus, T_Temp(F_FP()), T_Const(s_size))), 
					T_Temp(paramReg(pnum))), NULL);
				ftail->tail = F_AccessList(InFrame(s_size), NULL);
				vtail = vtail->tail;
				ftail = ftail->tail;
			}
			else {
				ftail->tail = F_AccessList(InFrame(formal_off), NULL);
				ftail = ftail->tail;
				formal_off += wordsize;
			}
			
		}
		// alloc in register
		else {
			if (pnum >= 6){
				printf("Frame: the 7-nth formal should be passed on frame.\n");
			}
			else{
				Temp_temp temp = Temp_newtemp();
				vtail->tail = T_StmList(T_Move(T_Temp(temp), T_Temp(paramReg(pnum))), NULL);
				ftail->tail = F_AccessList(InReg(temp), NULL);
				vtail = vtail->tail;
				ftail = ftail->tail;
			}
		}
		
		escapes = escapes->tail;
		pnum++;
	}
	
	return F_Frame(label, formals->tail, NULL, view_shift->tail, s_size);
}

T_exp F_exp(F_access access, T_exp frame_ptr) {
	if (access->kind == inReg) 
		return T_Temp(access->u.reg);
	else
		return T_Mem(T_Binop(T_plus, frame_ptr, T_Const(access->u.offset)));
}

T_stm F_procEntryExit1(F_frame frame,T_stm stm)
{
	return stm;
}



Temp_temp F_FP() {
	if (!rbp) rbp = Temp_newtemp();
	return rbp;
}

Temp_temp F_RBP() {
	if (!rbp) rbp = Temp_newtemp();
	return rbp;
}

Temp_temp F_RSP() {
	if (!rsp) rsp = Temp_newtemp();
	return rsp;
}

Temp_temp F_RDI() {
	if (!rdi) rdi = Temp_newtemp();
	return rdi;
}

Temp_temp F_RSI() {
	if (!rsi) rsi = Temp_newtemp();
	return rsi;
}

Temp_temp F_RDX() {
	if (!rdx) rdx = Temp_newtemp();
	return rdx;
}

Temp_temp F_RCX() {
	if (!rcx) rcx = Temp_newtemp();
	return rcx;
}

Temp_temp F_RV() {
	if (!rax) rax = Temp_newtemp();
	return rax;
}

Temp_temp F_R8() {
	if (!r8) r8 = Temp_newtemp();
	return r8;
}

Temp_temp F_R9() {
	if (!r9) r9 = Temp_newtemp();
	return r9;
}