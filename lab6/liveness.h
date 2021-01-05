#ifndef LIVENESS_H
#define LIVENESS_H
#include "graph.h"
#include "temp.h"

typedef struct Live_move_ *Live_move;
struct Live_move_ {
	G_node src, dst;
};

typedef struct Live_moveList_ *Live_moveList;
struct Live_moveList_ {
	Live_move head;
	Live_moveList tail;
};

Live_moveList Live_MoveList(Live_move head, Live_moveList tail);
Live_move Live_Move(G_node src, G_node dst);

struct Live_graph {
	G_graph graph;
	Live_moveList moves;
	G_table nodeUseCount;
};

struct Live_graph Live_liveness(G_graph flow);
Temp_temp Live_gtemp(G_node n);

//-----------------util functions for live_move------------------
bool Live_moveIn(Live_moveList ml, Live_move m);
Live_moveList Live_moveRemove(Live_moveList ml, Live_move m);
Live_moveList Live_moveComplement(Live_moveList in, Live_moveList notin);
Live_moveList Live_moveSplice(Live_moveList a, Live_moveList b);
Live_moveList Live_moveUnion(Live_moveList a, Live_moveList b);
Live_moveList Live_moveIntersect(Live_moveList a, Live_moveList b);
Live_moveList Live_moveAppend(Live_moveList ml, Live_move m);

#endif
