/*
 * temp.h 
 *
 */

#ifndef TEMP_H
#define TEMP_H

typedef struct Temp_temp_ *Temp_temp;
Temp_temp Temp_newtemp(void);
int Temp_int(Temp_temp);

typedef struct Temp_tempList_ *Temp_tempList;
struct Temp_tempList_ { Temp_temp head; Temp_tempList tail;};
Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t);

typedef S_symbol Temp_label;
Temp_label Temp_newlabel(void);
Temp_label Temp_namedlabel(string name);
string Temp_labelstring(Temp_label s);

typedef struct Temp_labelList_ *Temp_labelList;
struct Temp_labelList_ { Temp_label head; Temp_labelList tail;};
Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t);

typedef struct Temp_map_ *Temp_map;
Temp_map Temp_empty(void);
Temp_map Temp_layerMap(Temp_map over, Temp_map under);
void Temp_enter(Temp_map m, Temp_temp t, string s);
string Temp_look(Temp_map m, Temp_temp t);
void Temp_dumpMap(FILE *out, Temp_map m);

Temp_map Temp_name(void);

//--------------set operation----------------
bool Temp_tempIn(Temp_tempList tl, Temp_temp t);
Temp_tempList Temp_tempComplement(Temp_tempList in, Temp_tempList notin);
Temp_tempList Temp_tempSplice(Temp_tempList a, Temp_tempList b);
Temp_tempList Temp_tempUnion(Temp_tempList a, Temp_tempList b);

void Temp_tempReplace(Temp_tempList l, Temp_temp origin, Temp_temp newTemp);

bool Temp_labelIn(Temp_labelList ll, Temp_label label);

Temp_tempList Temp_tempAppend(Temp_tempList tl, Temp_temp t);
Temp_tempList Temp_tempRemove(Temp_tempList tl, Temp_temp t);
Temp_tempList Temp_tempCopy(Temp_tempList list);
int Temp_tempLength(Temp_tempList list);
bool Temp_tempEqual(Temp_tempList a, Temp_tempList b);

//-------------debug------------------
int Temp_getTempnum(Temp_temp t);

#endif
