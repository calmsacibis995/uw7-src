/*		copyright	"%c%" 	*/

#ident	"@(#)awk:tran.c	2.15.3.2"
#ident  "$Header$"


#define	DEBUG
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "awk.h"
#include "y.tab.h"
#include <pfmt.h>

#define	FULLTAB	2	/* rehash when table gets this x full */
#define	GROWTAB 4	/* grow table by this factor */

Array	*symtab;	/* main symbol table */

uchar	**FS;		/* initial field sep */
uchar	**RS;		/* initial record sep */
uchar	**OFS;		/* output field sep */
uchar	**ORS;		/* output record sep */
uchar	**OFMT;		/* output format for numbers */
uchar	**CONVFMT;	/* generic format for numbers->strings */
Awkfloat *NF;		/* number of fields in current record */
Awkfloat *NR;		/* number of current record */
Awkfloat *FNR;		/* number of current record in current file */
uchar	**FILENAME;	/* current filename argument */
Awkfloat *ARGC;		/* number of arguments from command line */
uchar	**SUBSEP;	/* subscript separator for a[i,j,k]; default \034 */
Awkfloat *RSTART;	/* start of re matched with ~; origin 1 (!) */
Awkfloat *RLENGTH;	/* length of same */

Cell	*recloc;	/* location of record */
Cell	*nrloc;		/* NR */
Cell	*nfloc;		/* NF */
Cell	*fsloc;		/* FS */
Cell	*fnrloc;	/* FNR */
Array	*ARGVtab;	/* symbol table containing ARGV[...] */
Array	*ENVtab;	/* symbol table containing ENVIRON[...] */
Cell	*rstartloc;	/* RSTART */
Cell	*rlengthloc;	/* RLENGTH */
Cell	*symtabloc;	/* SYMTAB */

Cell	*nullloc;
Node	*nullnode;	/* zero&null, converted into a node for comparisons */

extern Node *valtonode();
extern Cell fldtab[];

static	const	char
	assigntovid[] = ":80",
	assigntov[] = "assign to";

const char
	readvofid[] = ":81",
	readvof[] = "read value of",
	outofspace[] = ":82:Out of space in %s",
	nlstring[] = ":83:Newline in string %.10s ...";

syminit()
{
	symtab = makesymtab(NSYMTAB);
	setsymtab("0", "0", 0.0, NUM|STR|CON|DONTFREE, symtab);
	/* this is used for if(x)... tests: */
	nullloc = setsymtab("$zero&null", "", 0.0, NUM|STR|CON|DONTFREE, symtab);
	nullnode = valtonode(nullloc, CCON);
	/* recloc = setsymtab("$0", record, 0.0, REC|STR|DONTFREE, symtab); */
	recloc = &fldtab[0];
	fsloc = setsymtab("FS", " ", 0.0, STR|DONTFREE, symtab);
	FS = &fsloc->sval;
	RS = &setsymtab("RS", "\n", 0.0, STR|DONTFREE, symtab)->sval;
	OFS = &setsymtab("OFS", " ", 0.0, STR|DONTFREE, symtab)->sval;
	ORS = &setsymtab("ORS", "\n", 0.0, STR|DONTFREE, symtab)->sval;
	OFMT = &setsymtab("OFMT", "%.6g", 0.0, STR|DONTFREE, symtab)->sval;
	CONVFMT = &setsymtab("CONVFMT", "%.6g", 0.0, STR|DONTFREE, symtab)->sval;
	FILENAME = &setsymtab("FILENAME", "-", 0.0, STR|DONTFREE, symtab)->sval;
	nfloc = setsymtab("NF", "", 0.0, NUM, symtab);
	NF = &nfloc->fval;
	nrloc = setsymtab("NR", "", 0.0, NUM, symtab);
	NR = &nrloc->fval;
	fnrloc = setsymtab("FNR", "", 0.0, NUM, symtab);
	FNR = &fnrloc->fval;
	SUBSEP = &setsymtab("SUBSEP", "\034", 0.0, STR|DONTFREE, symtab)->sval;
	rstartloc = setsymtab("RSTART", "", 0.0, NUM, symtab);
	RSTART = &rstartloc->fval;
	rlengthloc = setsymtab("RLENGTH", "", 0.0, NUM, symtab);
	RLENGTH = &rlengthloc->fval;
	symtabloc = setsymtab("SYMTAB", "", 0.0, ARR, symtab);
	symtabloc->sval = (uchar *) symtab;
}

arginit(ac, av)
	int ac;
	uchar *av[];
{
	Cell *cp;
	Array *makesymtab();
	int i;
	uchar temp[5];

	for (i = 1; i < ac; i++)	/* first make FILENAME first real argument */
		if (!isclvar(av[i])) {
			setsval(lookup("FILENAME", symtab), av[i]);
			break;
		}
	ARGC = &setsymtab("ARGC", "", (Awkfloat) ac, NUM, symtab)->fval;
	cp = setsymtab("ARGV", "", 0.0, ARR, symtab);
	ARGVtab = makesymtab(NSYMTAB);	/* could be (int) ARGC as well */
	cp->sval = (uchar *) ARGVtab;
	for (i = 0; i < ac; i++) {
		sprintf((char *)temp, "%d", i);
		setsymtab(temp, *av, 0.0, STR|CANBENUM, ARGVtab);
		av++;
	}
}

envinit(envp)
	uchar *envp[];
{
	Cell *cp;
	Array *makesymtab();
	uchar *p;

	cp = setsymtab("ENVIRON", "", 0.0, ARR, symtab);
	ENVtab = makesymtab(NSYMTAB);
	cp->sval = (uchar *) ENVtab;
	for ( ; *envp; envp++) {
		if ((p = (uchar *) strchr((char *) *envp, '=')) == NULL)	/* index() on bsd */
			continue;
		*p++ = 0;	/* split into two strings at = */
		setsymtab(*envp, p, 0.0, STR|CANBENUM, ENVtab);
		p[-1] = '=';	/* restore in case env is passed down to a shell */
	}
}

Array *makesymtab(n)
	int n;
{
	Array *ap;
	Cell **tp;

	ap = (Array *) malloc(sizeof(Array));
	tp = (Cell **) calloc(n, sizeof(Cell *));
	if (ap == NULL || tp == NULL)
		error(MM_ERROR, outofspace, "makesymtab");
	ap->nelem = 0;
	ap->size = n;
	ap->tab = tp;
	return(ap);
}

freesymtab(ap)	/* free symbol table */
	Cell *ap;
{
	Cell *cp, *temp;
	Array *tp;
	int i;

	if (!isarr(ap))
		return;
	tp = (Array *) ap->sval;
	if (tp == NULL)
		return;
	for (i = 0; i < tp->size; i++) {
		for (cp = tp->tab[i]; cp != NULL; cp = temp) {
			xfree(cp->nval);
			if (freeable(cp))
				xfree(cp->sval);
			temp = cp->cnext;	/* avoids freeing then using */
			free(cp);
		}
	}
	free(tp->tab);
	free(tp);
}

freeelem(ap, s)		/* free elem s from ap (i.e., ap["s"] */
	Cell *ap;
	uchar *s;
{
	Array *tp;
	Cell *p, *prev = NULL;
	int h;
	
	tp = (Array *) ap->sval;
	h = hash(s, tp->size);
	for (p = tp->tab[h]; p != NULL; prev = p, p = p->cnext)
		if (strcmp((char *) s, (char *) p->nval) == 0) {
			if (prev == NULL)	/* 1st one */
				tp->tab[h] = p->cnext;
			else			/* middle somewhere */
				prev->cnext = p->cnext;
			if (freeable(p))
				xfree(p->sval);
			free(p->nval);
			free(p);
			tp->nelem--;
			return;
		}
}

Cell *setsymtab(n, s, f, t, tp)
	uchar *n, *s;
	Awkfloat f;
	unsigned t;
	Array *tp;
{
	register int h;
	register Cell *p;
	Cell *lookup();

	if (n != NULL && (p = lookup(n, tp)) != NULL) {
		dprintf( ("setsymtab found %o: n=%s", p, p->nval) );
		dprintf( (" s=\"%s\" f=%g t=%o\n", p->sval? p->sval : tostring(""), p->fval, p->tval) );
		return(p);
	}
	p = (Cell *) malloc(sizeof(Cell));
	if (p == NULL)
		error(MM_ERROR, ":84:Symbol table overflow at %s", n);
	p->nval = tostring(n);
	p->sval = s ? tostring(s) : tostring("");
	p->fval = f;
	p->tval = t & ~CANBENUM;
	p->csub = 0;
	if (t & CANBENUM)
		(void)is2number(0, p);
	tp->nelem++;
	if (tp->nelem > FULLTAB * tp->size)
		rehash(tp);
	h = hash(n, tp->size);
	p->cnext = tp->tab[h];
	tp->tab[h] = p;
	dprintf( ("setsymtab set %o: n=%s", p, p->nval) );
	dprintf( (" s=\"%s\" f=%g t=%o\n", p->sval? p->sval : tostring(""), p->fval, p->tval) );
	return(p);
}

hash(s, n)	/* form hash value for string s */
	register uchar *s;
	int n;
{
	register unsigned hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = (*s + 31 * hashval);
	return hashval % n;
}

rehash(tp)	/* rehash items in small table into big one */
	Array *tp;
{
	int i, nh, nsz;
	Cell *cp, *op, **np;

	nsz = GROWTAB * tp->size;
	np = (Cell **) calloc(nsz, sizeof(Cell *));
	if (np == NULL)
		error(MM_ERROR, outofspace, "rehash");
	for (i = 0; i < tp->size; i++) {
		for (cp = tp->tab[i]; cp; cp = op) {
			op = cp->cnext;
			nh = hash(cp->nval, nsz);
			cp->cnext = np[nh];
			np[nh] = cp;
		}
	}
	free(tp->tab);
	tp->tab = np;
	tp->size = nsz;
}

Cell *lookup(s, tp)	/* look for s in tp */
	register uchar *s;
	Array *tp;
{
	register Cell *p, *prev = NULL;
	int h;

	h = hash(s, tp->size);
	for (p = tp->tab[h]; p != NULL; prev = p, p = p->cnext)
		if (strcmp((char *) s, (char *) p->nval) == 0)
			return(p);	/* found it */
	return(NULL);			/* not found */
}

Awkfloat setfval(vp, f)
	register Cell *vp;
	Awkfloat f;
{
	if ((vp->tval & (NUM | STR)) == 0) 
		funnyvar(vp, gettxt(assigntovid, assigntov));
	if (vp->tval & FLD) {
		donerec = 0;	/* mark $0 invalid */
		if (vp-fldtab > *NF)
			newfld(vp-fldtab);
		dprintf( ("setting field %d to %g\n", vp-fldtab, f) );
	} else if (vp->tval & REC) {
		donefld = 0;	/* mark $1... invalid */
		donerec = 1;
	}
	vp->tval &= ~STR;	/* mark string invalid */
	vp->tval |= NUM;	/* mark number ok */
	dprintf( ("setfval %o: %s = %g, t=%o\n", vp, vp->nval ? vp->nval : tostring(""), f, vp->tval) );
	return vp->fval = f;
}

funnyvar(vp, rw)
	Cell *vp;
	char *rw;
{
	if (vp->tval & ARR)
		error(MM_ERROR, ":85:Cannot %s %s; it's an array name.",
			rw, vp->nval);
	if (vp->tval & FCN)
		error(MM_ERROR, ":86:Cannot %s %s; it's a function.",
			rw, vp->nval);
	error(MM_ERROR, ":87:Funny variable %o: n=%s s=\"%s\" f=%g t=%o",
		vp, vp->nval, vp->sval, vp->fval, vp->tval);
}

uchar *setsval(vp, s)
register Cell *vp;
uchar *s;
{
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, gettxt(assigntovid, assigntov));
	if (vp->tval & FLD) {
		donerec = 0;	/* mark $0 invalid */
		if (vp-fldtab > *NF)
			newfld(vp-fldtab);
		dprintf( ("setting field %d to %s\n", vp-fldtab, s) );
	} else if (vp->tval & REC) {
		donefld = 0;	/* mark $1... invalid */
		donerec = 1;
	} else if (vp == fsloc && donefld == 0) {
		/*
		* Because POSIX.2 requires that awk act as if it always
		* splits the current input line immediately after reading,
		* we force it to be split into fields just before a change
		* to FS if we haven't needed to do so yet.
		*/
		fldbld();
	}
	vp->tval &= ~NUM;
	vp->tval |= STR;
	s = tostring(s); /* moved to here since "s" can be "vp->sval" */
	if (freeable(vp))
		xfree(vp->sval);
	vp->tval &= ~DONTFREE;
	dprintf( ("setsval %o: %s = \"%s\", t=%o\n", vp, vp->nval, s, vp->tval) );
	return(vp->sval = s);
}

Awkfloat r_getfval(vp)
register Cell *vp;
{
	/* if (vp->tval & ARR)
		ERROR "Illegal reference to array %s", vp->nval FATAL;
		return 0.0; */
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, gettxt(readvofid, readvof));
	if ((vp->tval & FLD) && donefld == 0)
		fldbld();
	else if ((vp->tval & REC) && donerec == 0)
		recbld();
	if (!isnum(vp)) {	/* not marked as a number */
		vp->fval = atof((char *)vp->sval);	/* best guess */
		if (is2number(vp->sval, 0) && !(vp->tval&CON))
			vp->tval |= NUM;	/* make NUM only sparingly */
	}
	dprintf( ("getfval %o: %s = %g, t=%o\n", vp, vp->nval, vp->fval, vp->tval) );
	return(vp->fval);
}

uchar *r_getsval(vp)
register Cell *vp;
{
	uchar s[100];

	/* if (vp->tval & ARR)
		ERROR "Illegal reference to array %s",
			vp->nval FATAL;
		return ""; */
	if ((vp->tval & (NUM | STR)) == 0)
		funnyvar(vp, gettxt(readvofid, readvof));
	if ((vp->tval & FLD) && donefld == 0)
		fldbld();
	else if ((vp->tval & REC) && donerec == 0)
		recbld();
	if ((vp->tval & STR) == 0) {
		if (!(vp->tval&DONTFREE))
			xfree(vp->sval);
		if ((long)vp->fval == vp->fval) {
			sprintf((char *)s, "%ld", (long)vp->fval);
			vp->tval |= STR;
		} else {
			sprintf((char *)s, (char *)*CONVFMT, vp->fval);
			/*
			* In case CONVFMT is changed by the program,
			* we leave the string value uncached for non-
			* integer numeric constants.  Ugh.
			*/
			if (!(vp->tval & CON))
				vp->tval |= STR;
		}
		vp->sval = tostring(s);
		vp->tval &= ~DONTFREE;
	}
	dprintf( ("getsval %o: %s = \"%s\", t=%o\n", vp, vp->nval ? vp->nval : tostring(""), vp->sval ? vp->sval : tostring(""), vp->tval) );
	return(vp->sval);
}

uchar *tostring(s)
register uchar *s;
{
	register uchar *p;

	p = malloc(strlen((char *) s)+1);
	if (p == NULL)
		error(MM_ERROR, ":88:Out of space in tostring on %s", s);
	strcpy((char *) p, (char *) s);
	return(p);
}

uchar *qstring(s, delim)	/* collect string up to delim */
	uchar *s;
	int delim;
{
	uchar *q;
	int c, n;

	for (q = cbuf; (c = *s) != delim; s++) {
		if (q >= cbuf + CBUFLEN - 1)
			vyyerror(":89:String %.10s ... too long", cbuf);
		else if (c == '\n')
			vyyerror(nlstring, cbuf);
		else if (c != '\\')
			*q++ = c;
		else	/* \something */	
			switch (c = *++s) {
			case '\\':	*q++ = '\\'; break;
			case 'n':	*q++ = '\n'; break;
			case 't':	*q++ = '\t'; break;
			case 'b':	*q++ = '\b'; break;
			case 'f':	*q++ = '\f'; break;
			case 'r':	*q++ = '\r'; break;
			default:
				if (!isdigit(c)) {
					*q++ = c;
					break;
				}
				n = c - '0';
				if (isdigit(s[1])) {
					n = 8 * n + *++s - '0';
					if (isdigit(s[1]))
						n = 8 * n + *++s - '0';
				}
				*q++ = n;
				break;
			}
	}
	*q = '\0';
	return cbuf;
}