/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"

struct Temp_temp_ {int num;};

int Temp_int(Temp_temp t)
{
	return t->num;
}

string Temp_labelstring(Temp_label s)
{return S_name(s);
}

static int labels = 0;

Temp_label Temp_newlabel(void)
{char buf[100];
 sprintf(buf,"L%d",labels++);
 return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s)
{return S_Symbol(s);
}

static int temps = 100;

Temp_temp Temp_newtemp(void)
{Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));
 p->num=temps++;
 {char r[16];
  sprintf(r, "%d", p->num);
  Temp_enter(Temp_name(), p, String(r));
 }
 return p;
}



struct Temp_map_ {TAB_table tab; Temp_map under;};


Temp_map Temp_name(void) {
 static Temp_map m = NULL;
 if (!m) m=Temp_empty();
 return m;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
  Temp_map m = checked_malloc(sizeof(*m));
  m->tab=tab;
  m->under=under;
  return m;
}

Temp_map Temp_empty(void) {
  return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
  if (over==NULL)
      return under;
  else return newMap(over->tab, Temp_layerMap(over->under, under));
}

void Temp_enter(Temp_map m, Temp_temp t, string s) {
  assert(m && m->tab);
  TAB_enter(m->tab,t,s);
}

string Temp_look(Temp_map m, Temp_temp t) {
  string s;
  assert(m && m->tab);
  s = TAB_look(m->tab, t);
  if (s) return s;
  else if (m->under) return Temp_look(m->under, t);
  else return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t) 
{Temp_tempList p = (Temp_tempList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

static FILE *outfile;
void showit(Temp_temp t, string r) {
  fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
  outfile=out;
  TAB_dump(m->tab,(void (*)(void *, void*))showit);
  if (m->under) {
     fprintf(out,"---------\n");
     Temp_dumpMap(out,m->under);
  }
}

//------------set operation-----------------------

bool Temp_tempIn(Temp_tempList tl, Temp_temp t) {
	for (; tl; tl=tl->tail) {
		if (tl->head == t) {
			return TRUE;
		}
	}
	return FALSE;
}

Temp_tempList Temp_tempComplement(Temp_tempList in, Temp_tempList notin) {
	// Temp_tempList res = NULL;
	// for (; in; in=in->tail) {
	// 	if (!Temp_tempIn(notin, in->head)) {
	// 		res = Temp_TempList(in->head, res);
	// 	}
	// }
	// return res;
  	Temp_tempList newList = NULL;
	for (Temp_tempList tl = in; tl; tl = tl->tail)
	{
		if (!Temp_tempIn(notin, tl->head))
			newList = Temp_TempList(tl->head, newList);
	}
	return newList;
}

Temp_tempList Temp_tempSplice(Temp_tempList a, Temp_tempList b) {
  if (a==NULL) return b;
  return Temp_TempList(a->head, Temp_tempSplice(a->tail, b));
}

Temp_tempList Temp_tempUnion(Temp_tempList a, Temp_tempList b) {
	// Temp_tempList s = Temp_tempComplement(b, a);
	// return Temp_tempSplice(a, s);
  Temp_tempList newList = a;
	for (Temp_tempList tl = b; tl; tl = tl->tail)
	{
		if (!Temp_tempIn(newList, tl->head))
			newList = Temp_TempList(tl->head, newList);
	}
	return newList;
}

void Temp_tempReplace(Temp_tempList l, Temp_temp origin, Temp_temp newTemp) {
  for (; l; l=l->tail) {
    if (l->head == origin) {
      l->head = newTemp;
    }
  }
}

bool Temp_labelIn(Temp_labelList ll, Temp_label label) {
  for (; ll; ll=ll->tail) {
    if (ll->head == label) {
      return TRUE;
    }
  }
  return FALSE;
}

Temp_tempList Temp_tempAppend(Temp_tempList tl, Temp_temp t) {
  if (Temp_tempIn(tl, t)) {
    return tl;
  } else {
    return Temp_TempList(t, tl);
  }
}

Temp_tempList Temp_tempRemove(Temp_tempList ml, Temp_temp m) {
	Temp_tempList prev = NULL;
	Temp_tempList origin = ml;
	for (; ml; ml=ml->tail) {
		if (ml->head == m) {
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

Temp_tempList Temp_tempCopy(Temp_tempList list) {
	Temp_tempList head = Temp_TempList(NULL, NULL), tail = head;
	for (Temp_tempList tl = list; tl; tl = tl->tail) {
		tail->tail = Temp_TempList(tl->head, NULL);
		tail = tail->tail;
	}
	return head->tail;
}

bool Temp_tempEqual(Temp_tempList a, Temp_tempList b) {
	int la = Temp_tempLength(a);
	int lb = Temp_tempLength(b);
	if (la != lb) return FALSE;
	if (Temp_tempComplement(a, b)) return FALSE;
	return TRUE;
}

int Temp_tempLength(Temp_tempList list) {
	int length = 0;
	for (Temp_tempList tl = list; tl; tl = tl->tail)
		length++;
	return length;
}

//---------------debug--------------------
int Temp_getTempnum(Temp_temp t) {
  return t->num;
}