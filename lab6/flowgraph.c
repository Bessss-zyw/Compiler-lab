#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

// implemented by BOZ

Temp_tempList FG_def(G_node n) {
	//your code here.
	AS_instr instr = G_nodeInfo(n);
	switch (instr->kind) {
		case I_OPER:
			return instr->u.OPER.dst;
		case I_LABEL:
			return NULL;
		case I_MOVE:
			return instr->u.MOVE.dst;
		default:
			assert(0);
	}
	return NULL;
}

Temp_tempList FG_use(G_node n) {
	//your code here.
	AS_instr instr = G_nodeInfo(n);
	switch (instr->kind) {
		case I_OPER:
			return instr->u.OPER.src;
		case I_LABEL:
			return NULL;
		case I_MOVE:
			return instr->u.MOVE.src;
		default:
			assert(0);
	}
	return NULL;
}

bool FG_isMove(G_node n) {
	//your code here.
	AS_instr instr = G_nodeInfo(n);
	return (instr->kind == I_MOVE);
}

G_graph FG_AssemFlowGraph(AS_instrList il) {
	//your code here.
    printf("-----------begin FG_AssemFlowGraph1----------\n");

	// make a new graph
	G_graph graph = G_Graph();
	G_node curr, prev = NULL, next;
	G_nodeList jmpNodes = NULL;		// record jxx nodes
	TAB_table table = TAB_empty();	// record label infos

	// traverse nodes one by one, add sequential instrs
	for (AS_instrList ill = il; ill; ill = ill->tail) {
		AS_instr instr = ill->head;
		
		curr = G_Node(graph, instr);
		if (prev) G_addEdge(prev, curr);
        prev = curr;

		switch (instr->kind) {
			case I_OPER:	
				if (instr->u.OPER.jumps && instr->u.OPER.jumps->labels) 	// check if jxx
					jmpNodes = G_NodeList(curr, jmpNodes);
                if (!strncmp("\tjmp", instr->u.OPER.assem, 4)) {			// check if jmp
					prev = NULL; continue;
				}
				break;
			case I_LABEL:	// add into label table
				TAB_enter(table, instr->u.LABEL.label, curr);
				break;
			case I_MOVE:
				break;
			default:
				assert(0);
		}
		
		
	}

	// traverse jmp nodes one by one, add jmp edges
	for (; jmpNodes; jmpNodes = jmpNodes->tail) {
		curr = jmpNodes->head;
		AS_instr instr = G_nodeInfo(curr);

		for (Temp_labelList list = instr->u.OPER.jumps->labels; list; list = list->tail) {
			Temp_label label = list->head;
			next = TAB_look(table, label);
			if (next) 
				G_addEdge(curr, next);
			else 
				printf("Cannot find label %s\nSee in runtime.s or undefined label\n",Temp_labelstring(label));
		}
	}
	
    printf("-----------begin FG_AssemFlowGraph1----------\n");
	return graph;
}
