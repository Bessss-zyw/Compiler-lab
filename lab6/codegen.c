#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"
#include "string.h"

static AS_instrList iList=NULL, last=NULL;
static void emit(AS_instr inst) {
    if (last != NULL)
        last = last->tail = AS_InstrList(inst, NULL);
    else last = iList = AS_InstrList(inst, NULL);
}

static Temp_temp munchExp(T_exp e);
static void munchStm(T_stm s);

static Temp_tempList munchArgs(int i, T_expList args, int *stack_size) {
    if (!args) return NULL;
    Temp_tempList recur = munchArgs(i+1, args->tail, stack_size);
    Temp_temp cur = munchExp(args->head);
    Temp_temp para = NULL;
    if (i >= F_formalRegNum) {
        emit(AS_Oper("\tpush `s0", Temp_TempList(F_SP(), NULL), Temp_TempList(cur, Temp_TempList(F_SP(), NULL)), NULL));
        *stack_size += F_wordSize;
    } else {
        Temp_tempList argReg = F_argReg();
        for (int j = 0; j < i; j++) {
            argReg = argReg->tail;
        }
        para = argReg->head;
        emit(AS_Move("\tmov `s0, `d0", Temp_TempList(para, NULL), Temp_TempList(cur, NULL)));
    }
    if (para) {
        return Temp_TempList(para, recur);
    } else {
        return recur;
    }
}

static Temp_temp munchExp(T_exp e) {
    char buf[100];
    switch (e->kind) {
        case T_BINOP: {
            Temp_temp r = Temp_newtemp();
            Temp_temp left = munchExp(e->u.BINOP.left);
            Temp_temp right = munchExp(e->u.BINOP.right);
            switch (e->u.BINOP.op) {
                case T_plus: {
                    emit(AS_Move("\tmov `s0, `d0", Temp_TempList(r, NULL), Temp_TempList(left, NULL)));
                    emit(AS_Oper("\tadd `s0, `d0", Temp_TempList(r, NULL), Temp_TempList(right, Temp_TempList(r, NULL)), NULL));
                    return r;
                }
                case T_minus: {
                    emit(AS_Move("\tmov `s0, `d0", Temp_TempList(r, NULL), Temp_TempList(left, NULL)));
                    emit(AS_Oper("\tsub `s0, `d0", Temp_TempList(r, NULL), Temp_TempList(right, Temp_TempList(r, NULL)), NULL));
                    return r;
                }
                case T_mul: {
                    emit(AS_Move("\tmov `s0, `d0", F_X86MUL(), Temp_TempList(left, NULL)));
                    emit(AS_Oper("\timul `s0, `d0", F_X86MUL(), Temp_TempList(right, F_X86MUL()), NULL));
                    emit(AS_Move("\tmov `s0, `d0", Temp_TempList(r, NULL), F_X86MUL()));
                    return r;
                }
                case T_div: {
                    emit(AS_Move("\tmov `s0, `d0", F_X86DIV(), Temp_TempList(left, NULL)));
                    emit(AS_Oper("\tcltd", F_X86DIV(), F_X86DIV(), NULL));
                    emit(AS_Oper("\tidiv `s0", F_X86DIV(), Temp_TempList(right, F_X86DIV()), NULL));
                    emit(AS_Move("\tmov `s0, `d0", Temp_TempList(r, NULL), F_X86DIV()));
                    return r;
                }
            }
        }
        case T_MEM: {
            Temp_temp r = Temp_newtemp();
            T_exp mem = e->u.MEM;
            if (mem->kind == T_CONST) {
                sprintf(buf, "\tmov %d, `d0", mem->u.CONST);
                emit(AS_Oper(String(buf), Temp_TempList(r, NULL), NULL, NULL));
                return r;
            } else if (mem->kind == T_BINOP) {
                if (mem->u.BINOP.op == T_plus) {
                    if (mem->u.BINOP.right->kind == T_CONST) {
                        Temp_temp b = munchExp(mem->u.BINOP.left);
                        sprintf(buf, "\tmov %d(`s0), `d0", mem->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(buf), Temp_TempList(r, NULL), Temp_TempList(b, NULL), NULL));
                        return r;
                    } else if (mem->u.BINOP.right->kind == T_BINOP && mem->u.BINOP.right->u.BINOP.op == T_mul && 
                    mem->u.BINOP.right->u.BINOP.right->kind == T_CONST && mem->u.BINOP.right->u.BINOP.right->u.CONST == F_wordSize) {
                        Temp_temp b = munchExp(mem->u.BINOP.left);
                        Temp_temp i = munchExp(mem->u.BINOP.right->u.BINOP.left);
                        sprintf(buf, "\tmov (`s0,`s1,%d), `d0", F_wordSize);
                        emit(AS_Oper(String(buf), Temp_TempList(r, NULL), Temp_TempList(b, Temp_TempList(i, NULL)), NULL));
                        return r;
                    }
                }
            }
            Temp_temp s = munchExp(mem);
            emit(AS_Oper("\tmov (`s0), `d0", Temp_TempList(r, NULL), Temp_TempList(s, NULL), NULL));
            return r;
        }
        case T_TEMP: {
            return e->u.TEMP;
        }
        case T_ESEQ: {
            munchStm(e->u.ESEQ.stm);
            return munchExp(e->u.ESEQ.exp);
        }
        case T_NAME: {
            Temp_temp r = Temp_newtemp();
            // sprintf(buf, "\tmov $.%s, `d0", Temp_labelstring(e->u.NAME));
            sprintf(buf, "\tleaq .%s(%s), `d0", Temp_labelstring(e->u.NAME), "\%rip");
            emit(AS_Oper(String(buf), Temp_TempList(r, NULL), NULL, NULL));
            return r;
        }
        case T_CONST: {
            Temp_temp r = Temp_newtemp();
            sprintf(buf, "\tmov $%d, `d0", e->u.CONST);
            emit(AS_Oper(String(buf), Temp_TempList(r, NULL), NULL, NULL));
            return r;
        }
        case T_CALL: {
            Temp_temp r = F_RV();
            int stack_size = 0;
            Temp_tempList l = munchArgs(0, e->u.CALL.args, &stack_size);
            sprintf(buf, "\tcall %s", Temp_labelstring(e->u.CALL.fun->u.NAME));
            emit(AS_Oper(String(buf), Temp_TempList(F_RV(), F_callerSavedReg()), l, NULL));

            if (stack_size > 0) {
                memset(buf, 0, sizeof(buf));
                sprintf(buf, "\tadd $%d, `d0", stack_size);
                emit(AS_Oper(String(buf), Temp_TempList(F_SP(), NULL), NULL, NULL));
            }
            return r;
        }
    }
}

void munchStm(T_stm s) {
    char buf[100];
    switch (s->kind) {
        case T_SEQ: {
            munchStm(s->u.SEQ.left);
            munchStm(s->u.SEQ.right);
            break;
        }
        case T_LABEL: {
            sprintf(buf, ".%s", Temp_labelstring(s->u.LABEL));
            emit(AS_Label(String(buf), s->u.LABEL));
            break;
        }
        case T_JUMP: {
            emit(AS_Oper("\tjmp .`j0", NULL, NULL, AS_Targets(s->u.JUMP.jumps)));
            break;
        }
        case T_CJUMP: {
            Temp_temp left = munchExp(s->u.CJUMP.left);
            Temp_temp right = munchExp(s->u.CJUMP.right);
            emit(AS_Oper("\tcmp `s1, `s0", NULL, Temp_TempList(left, Temp_TempList(right, NULL)), NULL));
            char *op;
            switch (s->u.CJUMP.op) {
                case T_eq: {
                    op = "je";
                    break;
                }
                case T_ne: {
                    op = "jne";
                    break;
                }
                case T_lt: {
                    op = "jl";
                    break;
                }
                case T_gt: {
                    op = "jg";
                    break;
                }
                case T_le: {
                    op = "jle";
                    break;
                }
                case T_ge: {
                    op = "jge";
                    break;
                }
            }
            sprintf(buf, "\t%s .`j0", op);
            emit(AS_Oper(String(buf), NULL, NULL, AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
            break;
        }
        case T_MOVE: {
            T_exp dst = s->u.MOVE.dst, src = s->u.MOVE.src;
            Temp_temp t = munchExp(src);
            if (dst->kind == T_MEM) {
                T_exp mem = dst->u.MEM;
                if (mem->kind == T_BINOP) {
                    if (mem->u.BINOP.op == T_plus) {
                        if (mem->u.BINOP.right->kind == T_CONST) {
                            Temp_temp b = munchExp(mem->u.BINOP.left);
                            sprintf(buf, "\tmov `s0, %d(`s1)", mem->u.BINOP.right->u.CONST);
                            emit(AS_Oper(String(buf), NULL, Temp_TempList(t, Temp_TempList(b, NULL)), NULL));
                            break;
                        } else if (mem->u.BINOP.right->kind == T_BINOP && mem->u.BINOP.right->u.BINOP.op == T_mul 
                                    && mem->u.BINOP.right->u.BINOP.right->kind == T_CONST 
                                    && mem->u.BINOP.right->u.BINOP.right->u.CONST == F_wordSize) {
                            Temp_temp b = munchExp(mem->u.BINOP.left);
                            Temp_temp i = munchExp(mem->u.BINOP.right->u.BINOP.left);
                            sprintf(buf, "\tmov `s0, (`s1,`s2,%d)", F_wordSize);
                            emit(AS_Oper(String(buf), NULL, Temp_TempList(t, Temp_TempList(b, Temp_TempList(i, NULL))), NULL));
                            break;
                        }
                    }
                }
                Temp_temp d = munchExp(mem);
                emit(AS_Oper("\tmov `s0, (`s1)", NULL, Temp_TempList(t, Temp_TempList(d, NULL)), NULL));
                break;
            } else if (dst->kind == T_TEMP) {
                Temp_temp d = munchExp(dst);
                emit(AS_Move("\tmov `s0, `d0", Temp_TempList(d, NULL), Temp_TempList(t, NULL)));
                break;
            }
            break;
        }
		case T_EXP: {
            munchExp(s->u.EXP);
            break;
        }
        default:
            break;
    }
}

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList list;

    for (T_stmList sl = stmList; sl; sl=sl->tail) {
        munchStm(sl->head);
    }

    list = iList;
    iList = last = NULL;

    return F_procEntryExit2(list);
}
