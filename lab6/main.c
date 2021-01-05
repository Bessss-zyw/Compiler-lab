/*
 * main.c
 */

#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "absyn.h"
#include "errormsg.h"
#include "temp.h" /* needed by translate.h */
#include "tree.h" /* needed by frame.h */
#include "assem.h"
#include "frame.h" /* needed by translate.h and printfrags prototype */
#include "translate.h"
#include "env.h"
#include "semant.h" /* function prototype for transProg */
#include "canon.h"
#include "prabsyn.h"
#include "printtree.h"
#include "escape.h" 
#include "parse.h"
#include "codegen.h"
#include "regalloc.h"
//---------------------
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"

extern bool anyErrors;

//------debug-----------
void showInstrInfo(FILE* out, void* inst) {
  AS_print(out, (AS_instr)inst, Temp_layerMap(F_regTempMap(),Temp_name()));
}

void showTempInfo(FILE* out, void* t) {
  fprintf(out, "%s\n", Temp_look(Temp_layerMap(F_regTempMap(),Temp_name()), (Temp_temp)t));
}
//----------------------

/*Mark: Lab6: complete the function doProc
 * 1. initialize the F_tempMap
 * 2. initialize the register lists (for register allocation)
 * 3. do register allocation
 * 4. output (print) the assembly code of each function
 
 * Uncommenting the following printf can help you debugging.*/

/* print the assembly language instructions to filename.s */
static void doProc(FILE *out, FILE *logg, F_frame frame, T_stm body)
{
 AS_proc proc;
 struct RA_result allocation;
 T_stmList stmList;
 AS_instrList iList;
 struct C_block blo;

 F_tempMap = F_regTempMap();

 fprintf(logg, "doProc for function %s:\n", S_name(F_name(frame)));
 printStmList(logg, T_StmList(body, NULL));
 fprintf(logg, "-------====IR tree=====-----\n");

 stmList = C_linearize(body);
 printStmList(logg, stmList);
 fprintf(logg, "-------====Linearlized=====-----\n");

 blo = C_basicBlocks(stmList);
 C_stmListList stmLists = blo.stmLists;
 for (; stmLists; stmLists = stmLists->tail) {
 	printStmList(logg, stmLists->head);
	fprintf(logg,"------====Basic block=====-------\n");
 }

 stmList = C_traceSchedule(blo);
 printStmList(logg, stmList);
 fprintf(logg,"-------====trace=====-----\n");

 iList  = F_codegen(frame, stmList); /* 9 */
 AS_printInstrList(logg, iList, Temp_layerMap(F_tempMap, Temp_name()));
 fprintf(logg,"----======print flowgraph=======-----\n");

 G_graph fg = FG_AssemFlowGraph(iList);  /* 10.1 */
 G_show(logg, G_nodes(fg), showInstrInfo);
 fprintf(logg,"----======print interference graph=======-----\n");
 struct Live_graph lg = Live_liveness(fg);
 G_show(logg, G_nodes(lg.graph), showTempInfo);
 fprintf(logg,"----======before RA======-----\n");

 //  proc = F_procEntryExit3(frame, iList);
 //  fprintf(out, "%s", proc->prolog);
 //  AS_printInstrList (out, proc->body,
 //                        Temp_layerMap(F_regTempMap(),Temp_name()));
 //  fprintf(out, "%s", proc->epilog);

 struct RA_result ra = RA_regAlloc(frame, iList);  /* 11 */
 fprintf(logg,"----======print reg mapping======-----\n");
 Temp_dumpMap(logg, ra.coloring);
 fprintf(logg,"----======emit code======-----\n");
 proc = F_procEntryExit3(frame, ra.il);
 fprintf(out, "%s", proc->prolog);
 AS_printInstrList (out, proc->body,
                       ra.coloring);
 fprintf(out, "%s", proc->epilog);
}

void doStr(FILE *out, Temp_label label, string str) {
	fprintf(out, "\t.section .rodata\n");
	fprintf(out, ".%s:\n", S_name(label));

	int length = *(int *)str;
  fprintf(out, "\t.int %d\n", length);
	//it may contains zeros in the middle of string. To keep this work, we need to print all the charactors instead of using fprintf(str)
	fprintf(out, "\t.string \"");
	int i = 0;
  str = str + 4;
  char c;
	for (; i < length; i++) {
    c = str[i];
    if (c == '\n') { //  TODO: check later
      fprintf(out, "\\n");
    } else if (c == '\t') {
      fprintf(out, "\\t");
    } else {
		  fprintf(out, "%c", c);
    }
	}
	fprintf(out, "\"\n");

	//fprintf(out, ".string \"%s\"\n", str);
}

int main(int argc, string *argv)
{
 A_exp absyn_root;
 S_table base_env, base_tenv;
 F_fragList frags;
 char outfile[100];
 char logfile[100];
 FILE *out = stdout;
 FILE *logg = stdout;

 if (argc==2) {
   absyn_root = parse(argv[1]);
   if (!absyn_root)
     return 1;
     
#if 0
   pr_exp(out, absyn_root, 0); /* print absyn data structure */
   fprintf(out, "\n");
#endif

   //Lab 6: escape analysis
   //If you have implemented escape analysis, uncomment this
   Esc_findEscape(absyn_root); /* set varDec's escape field */

   frags = SEM_transProg(absyn_root);
   if (anyErrors) return 1; /* don't continue */

   /* convert the filename */
   sprintf(outfile, "%s.s", argv[1]);
  //  sprintf(logfile, "%s.log", argv[1]);
   out = fopen(outfile, "w");
  //  logg = fopen(logfile, "w");
   /* Chapter 8, 9, 10, 11 & 12 */
   for (;frags;frags=frags->tail)
     if (frags->head->kind == F_procFrag) {
       doProc(out, logg, frags->head->u.proc.frame, frags->head->u.proc.body);
	 }
     else if (frags->head->kind == F_stringFrag) 
	   doStr(out, frags->head->u.stringg.label, frags->head->u.stringg.str);

   fclose(out);

  // int k = 0;
	// for (; frags; frags = frags->tail) k++;
	// fprintf(stdout, "%d\n", k);
   return 0;
 }
 EM_error(0,"usage: tiger file.tig");
 return 1;
}
