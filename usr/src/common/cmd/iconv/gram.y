/*		copyright	"%c%" 	*/


%{
#ident	"@(#)gram.y	1.4"
#ident  "$Header$"

#include <stdio.h>
#include <pfmt.h>
#include "symtab.h"
#include "kbd.h"
char *strsave(), *spack();
struct node *sortroot();

extern int nerrors;
extern unsigned char oneone[];	/* for one-one mapping */
extern int oneflag;
struct kbd_map maplist[30];
int curmap = -1;		/* CURRENT map entry */
struct sym *curparent;
char *childlist[5];	/* max args is 5 (not used!) */
int childp;			/* pointer to next in childlist */
int inamap = 0;
int fullflag = 0;
int timeflag = 0;
extern char textline[];
extern int linnum;	/* current line number */
extern struct node *root;
extern int numnode;	/* number of nodes in current map */
static const char
	undef_func[] = ":41:Undefined function \"%s\", line %d\n",
	args_to[] = ":42:Arguments to `%s', line %d\n",
	badlen[] = ":43:`%s' string length unequal, line %d\n";
%}

/*
 * kbdcomp/gram.y	1.0 AT&T UNIX PACIFIC 1988/08
 *
 *	Statements:
 *
 *		map STYLE (name) {	<-- top level statement
 *			<expressions for the map>
 *		}
 *
 *		STYLE is "full" or "sparse" type
 *
 *		define(WORD STRING)	define WORD to produce STRING
 *
 *		WORD(x y)	func WORD + x yields y (many-many mapping)
 *
 *		keylist(x y)	bytes in x[i] become y[i] (one-one mapping)
 *				Done by lookup table.
 *
 *		string(x y)	NOT PART OF GRAMMAR.  Replaces "x" with "y".
 *				x & y may be one or more bytes.  The word
 *				"string" is pre-defined to be a NULL prefix.
 *				See "sym.c", the "s_init" routine.
 *
 *		strlist(x y)	as for "keylist", x[i] produces y[i]. The
 *				diff is that "strlist" does by tree; keylist
 *				does by lookup table.
 *
 *		error( string )	When module gets error, print string.
 *				(i.e., when a partial match does not end
 *				up succeeding.)
 *
 *		timed		Use timeout algorithm on the table.
 *
 *		link (STRING)	(top level statement) produces a link spec
 *
 *		extern (STRING)	Algorithm is external [RESERVED for future
 *				plans].
 */
%token MAP SPARSE FULL DEFINE STRING KEYLIST STRLIST TIMED NERROR LINK XTERN
%start prog

%%
prog		: /* empty */
		| map_list
		;

map_list	: map_list map
		| map
		;

map		: MAP
			{	fullflag = 0; }

		  style '(' STRING
			{
if (inamap) {
pfmt(stderr, MM_ERROR, ":44:'map' used inside a map, line %d\n", linnum);
exit(1);
}
				++curmap;
				maplist[curmap].mapname = strsave(textline);
if (! legalname(maplist[curmap].mapname)) {
pfmt(stderr, MM_ERROR, ":45:\"%s\": illegal map name, line %d\n",
	maplist[curmap].mapname, linnum);
	++nerrors;
}
				maplist[curmap].maproot = (struct node *) 0;
				maplist[curmap].mapone = NULL;
				maplist[curmap].maperr = NULL;
				maplist[curmap].map_min = 0;
				maplist[curmap].map_max = 256;
				oneflag = 0;
				timeflag = 0;
			}
		  ')'
		  	{ inamap = 1; }
		  '{' expr_list '}'
			{	inamap = 0;
				numnode = 0;
				if (fullflag)
					root = sortroot(root);	/* CAREFUL! */
				numtree(root);
				/*		showtree(root); */
				buildtbl(root);
			 }
		| LINK '(' STRING 
			{
				/*
				 * Simple linkage, just stuff it in the
				 * structure.  It will get output when
				 * the rest of them do.
				 */
				++curmap;
				maplist[curmap].mapname = LINKAGE;
				maplist[curmap].maproot = (struct node *) 0;
				maplist[curmap].mapone = NULL;
				maplist[curmap].maperr = NULL;
				maplist[curmap].map_min = 0;
				maplist[curmap].maptext = (unsigned char *) spack(textline);
				maplist[curmap].map_max = strlen((char *)maplist[curmap].maptext) + 1;
if (! legallink(maplist[curmap].maptext)) {
pfmt(stderr, MM_ERROR, ":46:\"%s\": illegal linkage spec, line %d.\n",
	maplist[curmap].maptext, linnum);
	++nerrors;
}
				buildtbl(LINKAGE);
			}
		  ')'
		| XTERN '(' STRING
			{
				/*
				 * Simple linkage, just stuff it in the
				 * structure.  It will get output when
				 * the rest of them do.
				 */
				++curmap;
				maplist[curmap].mapname = EXTERNAL;
				maplist[curmap].maproot = (struct node *) 0;
				maplist[curmap].mapone = NULL;
				maplist[curmap].maperr = NULL;
				maplist[curmap].map_min = 0;
				maplist[curmap].maptext = (unsigned char *) spack(textline);
				maplist[curmap].map_max = strlen((char *)maplist[curmap].maptext) + 1;
				buildtbl(LINKAGE);
			}
		 ')'
		;

style		:	/* empty */
		| SPARSE
			{ fullflag = 0; }
		| FULL
			{ fullflag = 1; }
		;
expr_list	: expr_list expr
		| expr
		;

expr		: DEFINE '(' STRING
			{
				if (s_find(textline)) {
				  pfmt(stderr, MM_ERROR,
				  	":48:Redefinition of %s, line %d\n",
				  textline, linnum);
				  ++nerrors;
				}
				curparent = s_lookup(textline);
				curparent->s_type = S_FUNC;
				childp = 0;
			}
		  STRING
			{
				curparent->s_value = strsave(textline);
			}
		  ')'

		| STRING
			{
/*
 * A function call:
 * For these guys, the func is the name, the "values" are the parameters.
 */
			if (!(curparent = s_find(textline))) {
				pfmt(stderr, MM_ERROR, undef_func, textline, linnum);
				++nerrors;
				curparent = s_create("orphan");
				curparent->s_type = S_FUNC;
				curparent->s_value = strsave("dummy");
			}
			childp = 0;
			}
		  '(' expr_tail
			{
			if (childp) {
				childlist[childp] =  (char *) 0;
				buildtree(curparent, &(childlist[0]));
			}
			else {
				pfmt(stderr, MM_ERROR, args_to,
					curparent->s_value, linnum);
				++nerrors;
			}
			}

		| STRLIST '('
			{
			if (!(curparent = s_find("string"))) {
				pfmt(stderr, MM_ERROR, undef_func, textline, linnum);
				++nerrors;
				curparent = s_create("orphan");
				curparent->s_type = S_FUNC;
				curparent->s_value = strsave("dummy");
			}
			childp = 0;
			}
		  expr_tail
		  	{
				do_strlist();
			}
		| KEYLIST '('
			{
			childp = 0;
			}
		  expr_tail
			{
			register int i;
			if (childp) {
				if (strlen(childlist[0]) != strlen(childlist[1])) {
					pfmt(stderr, MM_ERROR, badlen,
						"keylist", linnum);
					++nerrors;
				}
				else { /* do one-one mapping for each */
				    if (! oneflag) {
					for (i = 0; i < 256; i++)
						oneone[i] = (unsigned char) i;
					oneflag = 1;
				    }
				    while (*childlist[0]) {
					    oneone[((unsigned char)*childlist[0])] = *childlist[1];
					    ++(childlist[0]); ++(childlist[1]);
				    }
				}
			}
			else {
				pfmt(stderr, MM_ERROR, args_to, "keylist", linnum);
				++nerrors;
			}
			}

		| NERROR '(' STRING 
			{ maplist[curmap].maperr = (unsigned char *)strsave(textline); }
		  ')'
			
		| TIMED
			{
				timeflag = 1;	/* default to timed mode */
			}
		;

expr_tail	: STRING
			{ childlist[childp++] = strsave(textline); }
		  STRING
			{ childlist[childp++] = strsave(textline); }
		  ')'
		| error
			{ pfmt(stderr, MM_ERROR, ":49:Null expression, line %d\n",
				linnum);
			  ++nerrors;
			  yyclearin; }
		;
%%

/* put functions here */

char bttmp[512];
extern struct node *root;
extern int dup_error;	/* "tree" may set on dup errors */

buildtree(par, ch)

	struct sym *par;
	char **ch;
{
	register int rval;
	register int i;
	register char *t;

/*
 * Better have 2 children!
 */
	bttmp[0] = '\0';
/*
 * "Par" is null for "key()" nodes
 */
	if (par)
		strcat(bttmp, par->s_value);
	strcat(bttmp, *ch);	/* bttmp contains prefix + suffix */
	if (strlen(bttmp) >= KBDIMAX) {
		pfmt(stderr, MM_ERROR, ":50:Input string too long: %s\n", bttmp);
		++nerrors;
	}
	++ch;	/* point at result */
	rval = tree(root, bttmp, *ch);
	if (rval == E_DUP) {
		pfmt(stderr, MM_ERROR, ":51:dup. ( ");
		t = bttmp;
		while (*t)
			prinval(*t++, 1);
		pfmt(stderr, MM_NOSTD, ":52:) on line %d", linnum);
		if (dup_error)
			pfmt(stderr, MM_NOSTD, ":53: (already defined on/near line %d)",
				dup_error);
		fprintf(stderr, "\n");
		++nerrors;
	}
}

/*
 * A map name is illegal if it contains a comma, colon, or space.
 */

legalname(s)
	char *s;
{
	while (*s) {
		if (*s == ',' || *s == ':' || *s == ' ')
			return(0);
		++s;
	}
	return(1);	/* legal */
}

/*
 * A link-string is illegal if it doesn't follow the form:
 *		s colon x comma y [ comma z ...]
 * this is a simple check, and won't catch ALL bad formats, just some.
 * Assumes that spack() has been called on the string.
 */

legallink(s)
	char *s;
{
	register int colon, comma;

	if (! *s)
		return(0);
	colon = comma = 0;
	while (*s) {
		if (*s == ',' && ! colon)
			return(0);
		if (*s == ':' && comma)
			return(0);
		if (*s == ',')
			++comma;
		if (*s == ':')
			++colon;
		++s;
	}
	if (*(s-1) == ',')
		return(0);
	if (colon + comma == 0)
		return(0);
	if (comma < 1)
		return(0);
	return(1);
}

/*
 * Subroutine to do "strlist" stuff.  Make a string out of a byte
 * of each childlist; pass these in pairs to "buildtree".  Don't
 * pass a "parent".
 */

do_strlist()

{
	unsigned char x[2], y[2]; /* input strings */
	unsigned char *s[2];	  /* input string ptr array for buildtree */

	if (childp) {
		if (strlen(childlist[0]) != strlen(childlist[1])) {
			pfmt(stderr, MM_ERROR, badlen, "strlist", linnum);
			++nerrors;
		}
		else { /* build one-one mapping nodes for each */
		    s[0] = (unsigned char *) &x[0];	/* input */
		    s[1] = (unsigned char *) &y[0];	/* result */
		    x[1] = y[1] = '\0';
		    while (*childlist[0]) {
			x[0] = *childlist[0];
			y[0] = *childlist[1];
			buildtree((struct sym *) 0, &s[0]);
			++(childlist[0]); ++(childlist[1]);
		    }
		}
	}
	else {
		pfmt(stderr, MM_ERROR, args_to, "strlist", linnum);
		++nerrors;
	}
}
