%{
/* implemented by BOZ */
/* Lab2 Attention: You are only allowed to add code in this file and start at Line 26.*/
#include <string.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "y.tab.h"

int charPos=1;

int yywrap(void)
{
 charPos=1;
 return 1;
}

void adjust(void)
{
 EM_tokPos=charPos;
 charPos+=yyleng;
}

/*
* Please don't modify the lines above.
* You can add C declarations of your own below.
*/

int commentLevel = 0;
int bufLength = 0;
string buf;

void adjustStr(void)
{
  charPos+=yyleng;
}

void appendStr(string s)
{
  if (bufLength++ == 0) buf = s;
  else strcat(buf, s);
}

void appendStr_c(char c)
{
  char *temp = (char *)malloc(2);
  temp[0] = c; temp[1] = 0;
  if (bufLength++ == 0) buf = temp;
  else strcat(buf, temp);
}

/* @function: getstr
 * @input: a string literal
 * @output: the string value for the input which has all the escape sequences 
 * translated into their meaning.
 */
char *getstr(const char *str)
{
	//optional: implement this function if you need it
	return NULL;
}

%}
  /* You can add lex definitions here. */
digits [0-9]+
%Start STRINGBUF COMMENT
%%
  /* 
  * Below is an example, which you can wipe out
  * and write reguler expressions and actions of your own.
  */ 

<INITIAL>"\n" {adjust(); EM_newline(); continue;}
<INITIAL>[\t ]+ {adjust();}

  /* symbols */
<INITIAL>"," {adjust(); return COMMA;}
<INITIAL>":" {adjust(); return COLON;}
<INITIAL>";" {adjust(); return SEMICOLON;}
<INITIAL>"(" {adjust(); return LPAREN;}
<INITIAL>")" {adjust(); return RPAREN;}
<INITIAL>"[" {adjust(); return LBRACK;}
<INITIAL>"]" {adjust(); return RBRACK;}
<INITIAL>"{" {adjust(); return LBRACE;}
<INITIAL>"}" {adjust(); return RBRACE;}
<INITIAL>"." {adjust(); return DOT;}
<INITIAL>"+" {adjust(); return PLUS;}
<INITIAL>"-" {adjust(); return MINUS;}
<INITIAL>"*" {adjust(); return TIMES;}
<INITIAL>"/" {adjust(); return DIVIDE;}
<INITIAL>"=" {adjust(); return EQ;}
<INITIAL>"<>" {adjust(); return NEQ;}
<INITIAL>"<" {adjust(); return LT;}
<INITIAL>"<=" {adjust(); return LE;}
<INITIAL>">" {adjust(); return GT;}
<INITIAL>">=" {adjust(); return GE;}
<INITIAL>"&" {adjust(); return AND;}
<INITIAL>"|" {adjust(); return OR;}
<INITIAL>":=" {adjust(); return ASSIGN;}

  /* reserved words */
<INITIAL>"if" {adjust(); return IF;}
<INITIAL>"then" {adjust(); return THEN;}
<INITIAL>"else" {adjust(); return ELSE;}
<INITIAL>"while" {adjust(); return WHILE;}
<INITIAL>"for" {adjust(); return FOR;}
<INITIAL>"to" {adjust(); return TO;}
<INITIAL>"do" {adjust(); return DO;}
<INITIAL>"let" {adjust(); return LET;}
<INITIAL>"in" {adjust(); return IN;}
<INITIAL>"end" {adjust(); return END;}
<INITIAL>"of" {adjust(); return OF;}
<INITIAL>"break" {adjust(); return BREAK;}
<INITIAL>"nil" {adjust(); return NIL;}
<INITIAL>"function" {adjust(); return FUNCTION;}
<INITIAL>"var" {adjust(); return VAR;}
<INITIAL>"type" {adjust(); return TYPE;}
<INITIAL>"array" {adjust(); return ARRAY;}

  /* id and int */
<INITIAL>[a-zA-Z][a-zA-Z0-9_]* {adjust(); yylval.sval = String(yytext); return ID;}
<INITIAL>{digits} {adjust(); yylval.ival = atoi(yytext); return INT;}

  /* handle strings */
<INITIAL>\" {adjust(); buf = (char *)malloc(1); buf[0] = 0; bufLength = 0;BEGIN STRINGBUF;}
<STRINGBUF>\" {adjustStr(); yylval.sval = buf; BEGIN 0; return STRING;}
<STRINGBUF>\\\" {adjustStr(); appendStr_c('\"');}
<STRINGBUF>\\n {adjustStr(); appendStr_c('\n');}
<STRINGBUF>\\t {adjustStr(); appendStr_c('\t');}
<STRINGBUF>\\\\ {adjustStr(); appendStr_c('\\');}
<STRINGBUF>\\[ \n\t\f]+\\ {adjustStr();}
<STRINGBUF>\\{digits}{3} {adjustStr(); char c = (char)atoi(yytext + 1);appendStr_c(c);}
<STRINGBUF>\\\^[A-Z] {adjustStr(); char c = (char)(yytext[2] - 'A' + 1);appendStr_c(c);}
<STRINGBUF>. {adjustStr(); appendStr(String(yytext));}
<STRINGBUF><EOF> {EM_error(EM_tokPos, "error string %s with no ending!\n", buf);}

  /* handle comments */
<INITIAL>"/*" {adjust(); commentLevel = 1; BEGIN COMMENT;}
<COMMENT>"*/" {adjust(); if (--commentLevel == 0) BEGIN 0;}
<COMMENT>"/*" {adjust(); commentLevel++;}
<COMMENT>.|\n {adjust();}
<COMMENT><EOF> {EM_error(EM_tokPos, "error comment with no ending!\n");}

<INITIAL>. {adjustStr(); EM_error(EM_tokPos, "error token %s\n", yytext);}
.|\n {BEGIN 0; yyless(1);}
