#ifndef	NOIDENT
#ident	"@(#)oldattlib:expr.c	1.1"
#endif
/*
 expr.c (C source file)
	Acc: 575043153 Tue Mar 22 09:12:33 1988
	Mod: 570731707 Mon Feb  1 11:35:07 1988
	Sta: 573929807 Wed Mar  9 11:56:47 1988
	Owner: 2011
	Group: 1985
	Permissions: 664
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#define abs(arg) (arg < 0? -arg : arg)

#define DELIMITER 1
#define VARIABLE  2
#define NUMBER    3

extern char *prog;
extern char *pgmname;
char Token[80];
char TokType;

extern float Variables[26];

void GetExp();
void Level1();
void Level2();
void Level3();
void Level4();
void Level5();
void Level6();
void Primitive();
void GetToken();
void Arith();
void Unary();
void PutBack();
float FindVar();
void ExpError();

void GetExp(result)
float *result;
{
GetToken();
if (!*Token)
   {
   ExpError(2);
   return;
   }
Level1(result);
} /* end of GetExp */

void Level1(result)
float *result;
{
int slot;
int TmpTokType;
char TmpToken[80];

if (TokType == VARIABLE)
   {
   strcpy(TmpToken,Token);
   TmpTokType = TokType;
   slot = toupper(*Token) - 'A';
   GetToken();
   if (*Token != '=')
      {
      PutBack();
      strcpy(Token,TmpToken);
      TokType = TmpTokType;
      }
   else
      {
      GetToken();
      Level2(result);
      Variables[slot] = *result;
      return;
      }
   }
Level2(result);

} /* end of Level1 */

void Level2(result)
float *result;
{
register op;
float hold;

Level3(result);
while ((op = *Token) == '+' || op == '-')
   {
   GetToken();
   Level3(&hold);
   Arith(op,result,&hold);
   }
} /* end of Level2 */

void Level3(result)
float *result;
{
register op;
float hold;

Level4(result);
while ((op = *Token) == '*' || op == '/' || op == '%')
   {
   GetToken();
   Level4(&hold);
   Arith(op,result,&hold);
   }
} /* end of Level3 */

void Level4(result)
float *result;
{
float hold;

Level5(result);
while (*Token == '^')
   {
   GetToken();
   Level4(&hold);
   Arith('^',result,&hold);
   }
} /* end of Level4 */

void Level5(result)
float * result;
{
register char op;

op = 0;
if ((TokType == VARIABLE) && !strcmp(Token,"sin") ||
                             !strcmp(Token,"cos") ||
                             !strcmp(Token,"tan") ||
                             !strcmp(Token,"root") ||
                             !strcmp(Token,"abs"))
   {
   op = *Token;
   GetToken();
   }
if ((TokType == DELIMITER) && *Token == '~' ||
                              *Token == '+' ||
                              *Token == '-')
   {
   op = *Token;
   GetToken();
   }
Level6(result);
if (op) Unary(op,result);
} /* end of Level 5*/

void Level6(result)
float *result;
{
if ((*Token == '(') && (TokType == DELIMITER))
   {
   GetToken();
   Level1(result);
   if (*Token != ')')  ExpError(1);
   GetToken();
   }
else
   Primitive(result);
} /* end of Level6 */

void Primitive(result)
float *result;
{
switch(TokType)
   {
   case VARIABLE: *result = FindVar(Token);
                  GetToken();
                  return;
   case NUMBER:   *result = atof(Token);
                  GetToken();
                  return;
   default:       ExpError(0);
   }
} /* end of Primitive */

void Arith(o,r,h)
char o;
float *r;
float *h;
{

switch(o)
   {
   case '-': *r = *r - *h; break;
   case '+': *r = *r + *h; break;
   case '*': *r = *r * *h; break;
   case '/': *r = *r / *h; break;
   case '%': *r = (int) *r % (int) *h; break;
   case '^': if (*h == 0) *r = 1;
             else
                {
                float tmp = *r;
                int power = *h - 1;
                for (power = *h - 1; power > 0; --power)
                   *r = *r * tmp;
                }
             break;
   }
} /* end of Arith */

void Unary(o,r)
char o;
float *r;
{
switch (o)
   {
   case 's': *r = (float) sin((double)(*r)); break;
   case 'c': *r = (float) cos((double)(*r)); break;
   case 't': *r = (float) tan((double)(*r)); break;
   case 'a': *r = abs((*r));                 break;
   case '-': *r = -(*r);                     break;
   case 'r':
   case '~': *r = (float)pow((double)abs((*r)),(double).5); break;
   default : break;
   }
} /* end of Unary */

void PutBack()
{
char *t;
t = Token;
for (;*t;t++) prog--;
} /* end of putback */

float FindVar(s)
char *s;
{
if (!isalpha(*s))
   {
   ExpError(1);
   return 0;
   }
return Variables[toupper(*Token) - 'A'];
} /* end of FindVar */

void ExpError(error)
int error;
{
static char *e[] = {"syntax error",
                    "unbalanced parenthesis",
                    "no expression present"};
fprintf (stderr,"%s: %s\n",pgmname,e[error]);
} /* end of ExpError */

void GetToken()
{
register char *tmp;
TokType = 0;
tmp = Token;

while(iswhite(*prog)) ++prog;

if (is_in(*prog,"+-~*/%^=()"))
   {
   TokType = DELIMITER;
   *tmp++ = *prog++;
   }
else
  if(isalpha(*prog))
     {
      while(!isdelim(*prog)) *tmp++ = *prog++;
      TokType = VARIABLE;
      }
   else
      if(isdigit(*prog))
         {
         while(!isdelim(*prog)) *tmp++ = *prog++;
         TokType = NUMBER;
         }
*tmp = 0;
} /* end of GetToken */

iswhite(c)
char c;
{
if (c == ' ' || c == 9) return 1; else return 0;
} /* end of iswhite */

isdelim(c)
char c;
{
if (is_in(c," +-~/*%^=()") || c == 9 || c == '\r' || c == 0)
   return 1;
else
   return 0;
} /* end of isdelim */

is_in(ch,s)
char ch;
char *s;
{
while(*s) if (*s++ == ch) return 1;
return 0;
} /* end of is_in */
