#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

// implemented by BOZ

// =====================construction functions=====================
Live_moveList Live_MoveList(Live_move head, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->head = head;
	lm->tail = tail;
	return lm;
}

Live_move Live_Move(G_node src, G_node dst) {
	Live_move m = (Live_move) checked_malloc(sizeof(*m));
	m->src = src;
	m->dst = dst;
	return m;
}



// =========================util functions=========================
static Temp_tempList Join(Temp_tempList a, Temp_tempList b, bool *dirty) {
	Temp_tempList s = Temp_tempComplement(b, a);
	if (s) {
		if (dirty) {
			*dirty = TRUE;
		}
		return Temp_tempSplice(a, s);
	} else {
		return a;
	}
}

double getCount(G_table useCount, G_node n) {
	double *p = G_look(useCount, n);
	if (p) return *p;
	return 0.0;
}

void enterCount(G_table useCount, G_node n, double count) {
	double* dp = G_look(useCount, n);
	if (!dp) {
		dp = checked_malloc(sizeof(double));
		G_enter(useCount, n, dp);
	}
	*dp = count;
}

G_table buildLiveMap(G_graph flow) {
	G_table livein = G_empty();
	G_table liveout = G_empty();
	G_nodeList flowList = G_rnodes(flow);

	bool dirty;
	do {
		dirty = FALSE;
		for (G_nodeList fl = flowList; fl; fl = fl->tail) {
			G_node node = fl->head;

			Temp_tempList in_old = G_look(livein, node);
			Temp_tempList out_old = G_look(liveout, node);
			Temp_tempList use = FG_use(node);
			Temp_tempList def = FG_def(node);
			G_nodeList succ = G_succ(node);

			Temp_tempList out_new = NULL, in_new = NULL;

			for (; succ; succ=succ->tail) {
				G_node s_node = succ->head;
				Temp_tempList s_in = G_look(livein, s_node);
				out_new = Temp_tempUnion(out_new, s_in);
			}
			
			out_new = Join(out_old, out_new, &dirty);
			in_new = Join(in_old, Temp_tempUnion(use, Temp_tempComplement(out_new, def)), &dirty);

			G_enter(livein, node, in_new);
			G_enter(liveout, node, out_new);
		}

	} while(dirty);
	
	return liveout;
}



// ======================main function implement=======================
struct Live_graph Live_liveness(G_graph flow) {
	// --------------calculate liveMap---------------
	G_table liveout = buildLiveMap(flow);

	// --------------init temp variables-------------
	G_graph g = G_Graph();
	Live_moveList ml = NULL;
	G_table useCount = G_empty();
	TAB_table temp_table = TAB_empty();

	// ----init graph with registers in flowgraph----
	G_nodeList flowList = G_nodes(flow);
	for (G_nodeList fl = flowList; fl; fl = fl->tail) {
		G_node f = fl->head;
		Temp_tempList defs = FG_def(f);
		Temp_tempList uses = FG_use(f);
		Temp_tempList nodes = Temp_tempUnion(defs, uses);

		for (; nodes; nodes = nodes->tail) {
			Temp_temp node = nodes->head;
			if (TAB_look(temp_table, node)) continue;
			G_node gnode = G_Node(g, node);
			TAB_enter(temp_table, node, gnode);
		}
	}

	// add conflict edges according to Livemap
	for (G_nodeList fl = flowList; fl; fl = fl->tail) {
		G_node f = fl->head;
		Temp_tempList defs = FG_def(f);
		Temp_tempList uses = FG_use(f);
		Temp_tempList outs = G_look(liveout, f);

		// if is a MOVE instr, add into MOVE list, and remove use nodes from outs
		if (FG_isMove(f)) {
			G_node src = TAB_look(temp_table, uses->head);
			G_node dst = TAB_look(temp_table, defs->head);
			outs = Temp_tempComplement(outs, uses);
			ml = Live_MoveList(Live_Move(src, dst), ml);
		}

		// add edges between defs and out
		for (; defs; defs = defs->tail) {
			G_node defnode = TAB_look(temp_table, defs->head);
			enterCount(useCount, defnode, getCount(useCount, defnode) + 1);

			for (Temp_tempList outss = outs; outss; outss = outss->tail) {
				if (defs->head == outss->head) continue;
				G_node outnode = TAB_look(temp_table, outss->head);
				G_addEdge(defnode, outnode);
				G_addEdge(outnode, defnode);
				enterCount(useCount, outnode, getCount(useCount, outnode) + 1);
			}
		}
	}

	struct Live_graph lg;
	lg.graph = g;
	lg.moves = ml;
	lg.nodeUseCount = useCount;

	return lg;
}

Temp_temp Live_gtemp(G_node n) {return (Temp_temp) G_nodeInfo(n);}



// ==================util functions for live_move=======================
bool Live_moveIn(Live_moveList ml, Live_move m) {
	for (; ml; ml=ml->tail) {
		if (ml->head->src == m->src && ml->head->dst == m->dst) {
			return TRUE;
		}
	}
	return FALSE;
}

Live_moveList Live_moveRemove(Live_moveList ml, Live_move m) {
	Live_moveList prev = NULL;
	Live_moveList origin = ml;
	for (; ml; ml=ml->tail) {
		if (ml->head->src == m->src && ml->head->dst == m->dst) {
			if (prev) {
				prev->tail = ml->tail;
				return origin;
			} else {
				return ml->tail;
			}
		}
		prev = ml;
	}
	return origin;
}

Live_moveList Live_moveComplement(Live_moveList in, Live_moveList notin) {
	Live_moveList res = NULL;
	for (; in; in=in->tail) {
		if (!Live_moveIn(notin, in->head)) {
			res = Live_MoveList(in->head, res);
		}
	}
	return res;
}

Live_moveList Live_moveSplice(Live_moveList a, Live_moveList b) {
  if (a==NULL) return b;
  return Live_MoveList(a->head, Live_moveSplice(a->tail, b));
}

Live_moveList Live_moveUnion(Live_moveList a, Live_moveList b) {
	Live_moveList s = Live_moveComplement(b, a);
	return Live_moveSplice(a, s);
}

Live_moveList Live_moveIntersect(Live_moveList a, Live_moveList b) {
	Live_moveList res = NULL;
	for (; a; a=a->tail) {
		if (Live_moveIn(b, a->head)) {
			res = Live_MoveList(a->head, res);
		}
	}
	return res;
}

Live_moveList Live_moveAppend(Live_moveList ml, Live_move m) {
	if (Live_moveIn(ml, m))
		return ml;
	else 
		return Live_MoveList(m, ml);
}