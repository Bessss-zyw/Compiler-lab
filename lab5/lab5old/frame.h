
/*Lab5: This header file is not complete. Please finish it with more definition.*/

#ifndef FRAME_H
#define FRAME_H

#include "tree.h"
#define wordsize 8

typedef struct F_frame_ *F_frame;

typedef struct F_access_ *F_access;
typedef struct F_accessList_ *F_accessList;

struct F_accessList_ {F_access head; F_accessList tail;};


/* declaration for fragments */
struct F_frame_
{
	Temp_label name;
	F_accessList formals;
	F_accessList locals;
	T_stmList view_shift;
	int s_size;
};

typedef struct F_frag_ *F_frag;
struct F_frag_ {enum {F_stringFrag, F_procFrag} kind;
			union {
				struct {Temp_label label; string str; int len;} stringg;
				struct {T_stm body; F_frame frame;} proc;
			} u;
};

F_frag F_StringFrag(Temp_label label, string str);
F_frag F_ProcFrag(T_stm body, F_frame frame);

typedef struct F_fragList_ *F_fragList;
struct F_fragList_ 
{
	F_frag head; 
	F_fragList tail;
};

F_fragList F_FragList(F_frag head, F_fragList tail);
F_accessList F_AccessList(F_access head, F_accessList tail);

// other functions
F_access F_allocLocal(F_frame frame, bool escape);
F_frame F_newFrame(Temp_label label, U_boolList escapes);
T_exp F_accessExp(F_access access, T_exp frame_ptr);
T_exp F_externalCall(string s,T_expList args);

Temp_temp F_FP();
Temp_temp F_RBP();
Temp_temp F_RSP();
Temp_temp F_RDI();
Temp_temp F_RSI();
Temp_temp F_RDX();
Temp_temp F_RCX();
Temp_temp F_RAX();
Temp_temp F_R8();
Temp_temp F_R9();


#endif
