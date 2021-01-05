#include <stdio.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "liveness.h"
#include "color.h"
#include "table.h"

#define INFINITE 100000


// =====================global constants======================

static int K;
static Temp_tempList precolored;
static G_graph interferenceGraph;
static G_nodeList simplifyWorklist, freezeWorklist, spillWorklist, 
		spilledNodes, coalescedNodes, coloredNodes, selectStack;
static Live_moveList coalescedMoves, constrainedMoves, frozenMoves,
		worklistMoves, activeMoves;
static G_table degree, moveList, alias, colors, spillCost;


// ===================function declearation===================

//-------------main functions-------------
void init(G_graph ig, Temp_tempList regs, Live_moveList moves, G_table useCount);
void build();
void makeWorkList();
void simplify();
void coalesce();
void freeze();
void selectSpill();
void assignColors();
struct COL_result getResults(Temp_map initial);

//-------------util functions-------------
bool isPrecolored(G_node n);
bool isMoveRelated(G_node n);
bool inAdjSet(G_node u, G_node v);
Live_moveList nodeMoves(G_node n);
G_node getAlias(G_node n);

// -----for tables-----
void enterDegreeTable(G_node n, int d);
void enterMoveTable(G_node n, Live_moveList move);
void enterAliasTable(G_node n, G_node alia);
void enterColorTable(G_node n, Temp_temp color);
void enterCostTable(G_node n, double cost);
void updateDegreeTable(G_node n, int d);
int lookDegreeTable(G_node);
Live_moveList lookMoveTable(G_node);
G_node lookAliasTable(G_node);
Temp_temp lookColorTable(G_node);
double lookupCostTable(G_node n);

// -----for simplify-----
G_node popSelectStack();
void pushSelectStack(G_node node);
G_nodeList adjacent(G_node node);
void decrementDegree(G_node node);
void enableMoves(G_nodeList nodes);

// -----for coalesce-----
void addWorkList(G_node u);
bool OK(G_node t, G_node v);
bool conservative(G_nodeList list);
void combine(G_node u, G_node v) ;
void addEdge(G_node u, G_node v);

// -----for freeze-----
void freezeMoves(G_node u);

// ----for select spill----
G_node select();

// ------------for debugging---------------
void printInterferenceGraph(char *message);
void printTempList(Temp_tempList temps, char *message);
void printMoveList(Live_moveList moves, char *message);


// ===================function implementation===================

//-------------core function-------------
struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs, Live_moveList moves, G_table cost) {
	struct COL_result ret;
	init(ig, regs, moves, cost);
	build();
	makeWorkList();

	do {
		if (simplifyWorklist) simplify();
		else if (worklistMoves) coalesce();
		else if (freezeWorklist) freeze();
		else if (spillWorklist) selectSpill();
	} while (simplifyWorklist || worklistMoves || freezeWorklist || spillWorklist);

	assignColors();

	return getResults(initial);
}


//-------------main functions-------------
void init(G_graph ig, Temp_tempList regs, Live_moveList moves, G_table useCount) {
	
	K = F_regNum;
	precolored = regs;
	interferenceGraph = ig;

	simplifyWorklist = NULL;
	freezeWorklist = NULL;
	spillWorklist = NULL;
	spilledNodes = NULL;
	coalescedNodes = NULL;
	coloredNodes = NULL;
	selectStack = NULL;

	coalescedMoves = NULL;
	constrainedMoves = NULL;
	frozenMoves = NULL;
	worklistMoves = moves;
	activeMoves = NULL;

	degree = G_empty();			// to be calculated
	moveList = G_empty();		// to be calculated
	spillCost = useCount;		// to be calculated
	alias = G_empty();
	colors = G_empty();


	// calculate degree && cost 
	G_nodeList nodes = G_nodes(ig);
	for (; nodes; nodes=nodes->tail) {
		G_node n = nodes->head;
		if (!isPrecolored(n)) {
			int d = G_degree(n) / 2;
			enterDegreeTable(n, d);
		//	enterCostTable(n, lookupCostTable(n) / d);
		} else {
			enterDegreeTable(n , INFINITE);
		//	enterCostTable(n, INFINITE);
		}
	}

	// calculate moveList
	for (; moves; moves = moves->tail) {
        Live_move move = moves->head;
		enterMoveTable(move->src, Live_MoveList(move, lookMoveTable(move->src)));
		enterMoveTable(move->dst, Live_MoveList(move, lookMoveTable(move->dst)));
	}

}

void build() {
	printf("----------color build init4---------\n");
}

void makeWorkList() {
	G_nodeList nodes = G_nodes(interferenceGraph);
	for (; nodes; nodes=nodes->tail) {
		G_node n = nodes->head;
		if (isPrecolored(n)) continue;
		if (lookDegreeTable(n) >= K) {
			spillWorklist = G_nodeAppend(spillWorklist, n);
		} else if (isMoveRelated(n)) {
			freezeWorklist = G_nodeAppend(freezeWorklist, n);
		} else {
			simplifyWorklist = G_nodeAppend(simplifyWorklist, n);
		}
	}
}

void simplify() {
	G_node n = simplifyWorklist->head;
	simplifyWorklist = simplifyWorklist->tail;
	pushSelectStack(n);
	G_nodeList nl = adjacent(n);
	for (; nl; nl=nl->tail) {
		G_node m = nl->head;
		decrementDegree(m);
	}
}

void coalesce() {
	assert(worklistMoves != NULL);
	Live_move m = worklistMoves->head;

	G_node x = getAlias(m->src);
	G_node y = getAlias(m->dst);
	G_node u, v;
	if (isPrecolored(y)) {
		u = y;
		v = x;
	} else {
		u = x;
		v = y;
	}
	worklistMoves = Live_moveRemove(worklistMoves, m);
	if (u == v) {
		coalescedMoves = Live_moveAppend(coalescedMoves, m);
		addWorkList(u);
	} else if (isPrecolored(v) || G_goesTo(u, v)) {
		constrainedMoves = Live_moveAppend(constrainedMoves, m);
		addWorkList(u);
		addWorkList(v);
	} else {
		G_nodeList nl = adjacent(v);
		bool flag = TRUE;
		for (; nl; nl=nl->tail) {
			G_node t = nl->head;
			if (!OK(t, u)) {
				flag = FALSE;
				break;
			}
		}
		if (isPrecolored(u) && flag || !isPrecolored(u) && conservative(G_nodeUnion(adjacent(u), adjacent(v)))) {
			coalescedMoves = Live_moveAppend(coalescedMoves, m);
			combine(u, v);
			addWorkList(u);
		} else {
			activeMoves = Live_moveAppend(activeMoves, m);
		}
	}
}

void freeze() {
	G_node u = freezeWorklist->head;
	freezeWorklist = G_nodeRemove(freezeWorklist, u);
	simplifyWorklist = G_nodeAppend(simplifyWorklist, u);
	freezeMoves(u);
}

void selectSpill() {
	G_node n = select(); 
	spillWorklist = G_nodeRemove(spillWorklist, n);
	simplifyWorklist = G_nodeAppend(simplifyWorklist, n);
	freezeMoves(n);
}

void assignColors() {
	for (G_nodeList nl = G_nodes(interferenceGraph); nl; nl=nl->tail) {
		G_node n = nl->head;
		if (isPrecolored(n)) {
			enterColorTable(n, Live_gtemp(n));
			coloredNodes = G_nodeAppend(coloredNodes, n);
		}
	}
	while (selectStack) {
		G_node n = popSelectStack();
		if (G_nodeIn(coloredNodes, n)) continue;
		Temp_tempList okColors = precolored;
		G_nodeList adj = G_adj(n);
		for (; adj; adj=adj->tail) {
			G_node w = adj->head;
			if (G_nodeIn(coloredNodes, getAlias(w)) || isPrecolored(getAlias(w))) {
				okColors = Temp_tempComplement(okColors, Temp_TempList(lookColorTable(getAlias(w)), NULL));
			}
		}
		if (!okColors) {
			spilledNodes = G_nodeAppend(spilledNodes, n);
		} else {
			coloredNodes = G_nodeAppend(coloredNodes, n);
			enterColorTable(n, okColors->head);
		}
	}
	G_nodeList nl = coalescedNodes;
	for (; nl; nl=nl->tail) {
		G_node n = nl->head;
		enterColorTable(n, lookColorTable(getAlias(n)));
	}
}

struct COL_result getResults(Temp_map initial) {
	struct COL_result ret;
	ret.spills = NULL;

	if (!spilledNodes) {
		Temp_map coloring = Temp_empty();
		G_nodeList nl = G_nodes(interferenceGraph);
		for (; nl; nl=nl->tail) {
			G_node node = nl->head;
			Temp_temp color = lookColorTable(node);
			if (colors) {
				Temp_enter(coloring, Live_gtemp(node), Temp_look(initial, color));
			}
		}
		ret.coloring = Temp_layerMap(coloring, initial);
	}
	else {
		Temp_tempList spills = NULL;
		for (G_nodeList nl = spilledNodes; nl; nl=nl->tail) {
			G_node n = nl->head;
			spills = Temp_TempList(Live_gtemp(n), spills);
		}
		ret.spills = spills;
	}
	
	return ret;
}


//-------------util functions-------------
bool isPrecolored(G_node n) {
	return Temp_tempIn(precolored, Live_gtemp(n));
}

bool isMoveRelated(G_node n) {
	return nodeMoves(n) != NULL;
}

bool inAdjSet(G_node u, G_node v) {
	return (G_goesTo(u, v) || G_goesTo(v, u));
}

Live_moveList nodeMoves(G_node n) {
	return Live_moveIntersect(lookMoveTable(n), 
			Live_moveUnion(activeMoves, worklistMoves));
}

G_node getAlias(G_node n) {
	// G_node alia = lookAliasTable(n);
	// if (alia) return alia;
	// else return n;
	if (G_nodeIn(coalescedNodes, n)) 
		return getAlias(lookAliasTable(n));
	return n;
}


// -----for tables-----
void enterDegreeTable(G_node n, int d) {
	// int *p = (int *)checked_malloc(sizeof(int));
	// *p = d;
	G_enter(degree, n, (void *)d);
}

void enterMoveTable(G_node n, Live_moveList moves) {
	G_enter(moveList, n, moves);
}

void enterAliasTable(G_node n, G_node alia) {
	G_enter(alias, n, alia);
}

void enterColorTable(G_node n, Temp_temp color) {
	G_enter(colors, n, color);
}

void enterCostTable(G_node n, double cost) {
	double* p = G_look(spillCost, n);
	if (!p) {
		p = checked_malloc(sizeof(double));
		G_enter(spillCost, n, p);
	}
	*p = cost;
}

int lookDegreeTable(G_node n) {
	return (int)G_look(degree, n);
}

void updateDegreeTable(G_node n, int d) {
	// int *p = (int *)G_look(degrees, n);
	// *p = d;
    enterDegreeTable(n, d);
}

Live_moveList lookMoveTable(G_node n) {
	return (Live_moveList)G_look(moveList, n);
}

G_node lookAliasTable(G_node n) {
	return (G_node)G_look(alias, n);
}

Temp_temp lookColorTable(G_node n) {
	return (Temp_temp)G_look(colors, n);
}

double lookupCostTable(G_node n) {
	return *(double*)G_look(spillCost, n);
}


// -----for simplify-----
void pushSelectStack(G_node n) {
	selectStack = G_NodeList(n, selectStack);
}

G_node popSelectStack() {
	if (!selectStack) return NULL;
	G_node n = selectStack->head;
	selectStack = selectStack->tail;
	return n;
}

G_nodeList adjacent(G_node n) {
	return G_nodeComplement(G_succ(n), G_nodeUnion(selectStack, coalescedNodes));
}

void decrementDegree(G_node m) {
	int d = lookDegreeTable(m);
	enterDegreeTable(m, d-1);
	if (d == K && !isPrecolored(m)) { 
		enableMoves(G_nodeAppend(adjacent(m), m));
		spillWorklist = G_nodeRemove(spillWorklist, m);
		if (isMoveRelated(m)) {
			freezeWorklist = G_nodeAppend(freezeWorklist, m);
		} else {
			simplifyWorklist = G_nodeAppend(simplifyWorklist, m);
		}
	}
}

void enableMoves(G_nodeList nl) {
	for (; nl; nl=nl->tail) {
		G_node n = nl->head;
		Live_moveList ml = nodeMoves(n);
		for (; ml; ml=ml->tail) {
			Live_move m = ml->head;
			if (Live_moveIn(activeMoves, m)) {
				activeMoves = Live_moveRemove(activeMoves, m);
				worklistMoves = Live_moveAppend(worklistMoves, m);
			}
		}
	}
}


// -----for coalesce-----
void addWorkList(G_node u) {
	if (!isPrecolored(u) && !isMoveRelated(u) && lookDegreeTable(u) < K) {
		freezeWorklist = G_nodeRemove(freezeWorklist, u);
		simplifyWorklist = G_nodeAppend(simplifyWorklist, u);
	}
}

bool OK(G_node t, G_node v) {
	return lookDegreeTable(t) < K || isPrecolored(t) || G_goesTo(t, v);
}

bool conservative(G_nodeList nl) {
	int k = 0;
	for (; nl; nl=nl->tail) {
		G_node node = nl->head;
		if (lookDegreeTable(node) >= K) {
			k = k + 1;
		}
	}
	return k < K;
}

void combine(G_node u, G_node v) {
	if (G_nodeIn(freezeWorklist, v)) {
		freezeWorklist = G_nodeRemove(freezeWorklist, v);
	} else {
		spillWorklist = G_nodeRemove(spillWorklist, v);
	}
	coalescedNodes = G_nodeAppend(coalescedNodes, v);
	enterAliasTable(v, u);
	Live_moveList uml = lookMoveTable(u);
	Live_moveList vml = lookMoveTable(v);
	enterMoveTable(u, Live_moveUnion(uml, vml));
	enableMoves(G_NodeList(v, NULL));
	G_nodeList nl = adjacent(v);
	for (; nl; nl=nl->tail) {
		G_node t = nl->head;
		addEdge(t, u);
		decrementDegree(t);
	}
	if (lookDegreeTable(u) >= K && G_nodeIn(freezeWorklist, u)) {
		freezeWorklist = G_nodeRemove(freezeWorklist, u);
		spillWorklist = G_nodeAppend(spillWorklist, u);
	}
}

void addEdge(G_node u, G_node v) {
	if (!G_goesTo(u, v)) {
		G_addEdge(u, v);
		G_addEdge(v, u);
		enterDegreeTable(u, lookDegreeTable(u) + 1);
		enterDegreeTable(v, lookDegreeTable(v) + 1);
	}	
}


// -----for freeze-----
void freezeMoves(G_node u) {
	Live_moveList ml = nodeMoves(u);
	for (; ml; ml=ml->tail) {
		Live_move m = ml->head;
		G_node x = m->src;
		G_node y = m->dst;
		G_node v;
		if (getAlias(y) == getAlias(u)) {
			v = getAlias(x);
		} else {
			v = getAlias(y);
		}
		activeMoves = Live_moveRemove(activeMoves, m);
		frozenMoves = Live_moveAppend(frozenMoves, m);
		if (!nodeMoves(v) && lookDegreeTable(v) < K) {
			freezeWorklist = G_nodeRemove(freezeWorklist, v);
			simplifyWorklist = G_nodeAppend(simplifyWorklist, v);
		}
	}
}

// ----for select spill----
G_node select() {
	double min = 100000.0;
	G_node minNode = NULL;
	for (G_nodeList nl = spillWorklist; nl; nl=nl->tail) {
		G_node n = nl->head;
		assert(!isPrecolored(n));
		double cost = lookupCostTable(n);
		if (cost < min) {
			min = cost;
			minNode = n;
		}
	}
	return minNode;
}

//-----debug function implementation-----
void printInterferenceGraph(char *message) {
	printf("----------begin printInterferenceGraph----------\n");
	if (message != NULL) printf("message: %s\n", message);
	printf("-----------end printInterferenceGraph-----------\n");
}

void printTempList(Temp_tempList temps, char *message) {
	printf("---------------begin printTempList---------------\n");
	if (message != NULL) printf("message: %s\n", message);
	printf("----------------end printTempList----------------\n");
}

void printMoveList(Live_moveList moves, char *message) {
	printf("---------------begin printMoveList---------------\n");
	if (message != NULL) printf("message: %s\n", message);
	printf("----------------end printMoveList----------------\n");
}
