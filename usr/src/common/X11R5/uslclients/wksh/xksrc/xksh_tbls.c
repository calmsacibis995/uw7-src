#ident	"@(#)wksh:xksrc/xksh_tbls.c	1.1"

/*	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T    */
/*	and UNIX System Laboratories, Inc.			*/
/*	The copyright notice above does not evidence any       */
/*	actual or intended publication of such source code.    */

#include <stdio.h>
#include <string.h>
#include "sh_config.h" /* which includes sys/types.h */
/*#include <sys/types.h>*/
#include <ctype.h>
#include "xksh.h"

extern int _Prdebug;
static int _Delim;
static int strglen;
static int struct_size;

extern int Pr_format;
int Pr_tmpnonames = 0;

static const char *Str_close_curly = "}";
static const char *Str_open_curly = "{";

#define MALMEMBERS	(4)	/* initial number of members of malloc'ed array to malloc */

#define UPPER(C) (islower(C) ? toupper(C) : (C))
#define isvarchar(C) (isalnum(C) || ((C) == '_'))

/*
 * Some of our test programs use xk_parse() to parse cdata from command
 * lines.  This has the drawback that anys or externals will get a malloc'ed
 * buffer for their char * on the send side, which would not normally be
 * freed, because xk_free() knows that such any's usually point into the
 * ubuf rather than being malloced.  On the receive side, this would be
 * true even in our test program.  So, on the send side, we keep a table
 * of any any or external char * that is malloc'ed, and xk_free checks
 * this table before free'ing the given item.  After being freed, the
 * stack is decremented for efficiency.
 */

#define ANYTBLINC	(4)

static char **Anytbl = NULL;
static int Nanytbl = 0;
static int Maxanytbl = 0;

struct special {
	char *name;
	int (*free)();
	int (*parse)();
	int (*print)();
};

#define SPEC_FREE	0
#define SPEC_PARSE	1
#define SPEC_PRINT	2

static struct special *Special = NULL;
static int Nspecs = 0;

static char **Dont = NULL;
static int Ndont, Sdont;
int (*
find_special(type, name))()
int type;
char *name;
{
	int i;

	if (!Special)
		return(NULL);
	for (i = 0; i < Nspecs; i++) {
		if (strcmp(Special[i].name, name) == 0) {
			switch(type) {
			case SPEC_PRINT:
				return(Special[i].print);
			case SPEC_FREE:
				return(Special[i].free);
			case SPEC_PARSE:
				return(Special[i].parse);
			}
		}
	}
	return(NULL);
}

int
set_special(name, free, parse, print)
char *name;
int (*free)();
int (*parse)();
int (*print)();
{
	int i;

	for (i = 0; i < Nspecs; i++)
		if (strcmp(Special[i].name, name) == 0)
			break;
	if (i == Nspecs) {
		if (!Special) {
			Special = (struct special *) malloc(sizeof(struct special));
			Special[0].name = strdup(name);
			Nspecs = 1;
		}
		else {
			Special = (struct special *) realloc(Special, (Nspecs + 1) * sizeof(struct special));
			Special[i].name = strdup(name);
			Nspecs++;
		}
	}
	if (!Special)
		return(FAIL);
	Special[i].free = free;
	Special[i].parse = parse;
	Special[i].print = print;
	return(SUCCESS);
}

/*
 * xk_parse:  Takes a pointer to a structure member table, a pointer
 * to a buffer containing an ascii representation of the structure
 * represented by the table pointer, and the number of pointers saved
 * from previous recursive calls to this routine, and parses the
 * buf into p.
 *
 * Increments buf to the last point at which it read a character,
 * and returns SUCCESS or FAIL.
 */

int
xk_parse(tbl, buf, p, nptr, sub, pass, tbl_find)
memtbl_t *tbl;
char **buf;		/* ascii representation of the structure */
char *p;		/* pointer to the structure to be filled in */
int nptr;		/* levels of indirection from previous calls */
int sub;		/* subscript from prev calls, or 0 if none */
VOID *pass; /* name for index into symbolics table */
memtbl_t *(*tbl_find)();
{
	memtbl_t *ntbl;
	register int i;
	int skind, delim_type;
	long val = 0;		/* used for choice selection */
	char *np;
	int delim = _Delim;
	int nmal;		/* number of members malloc'ed arrays */
	char *pp;
	int (*spec_parse)();

	if (tbl == NULL) {
		if (_Prdebug)
			fprintf(stderr, "PARSE: NULL type table!!!\n");
		return(FAIL);
	}
	xk_skipwhite(buf);
	/*
	 * If this is supposed to be a pointer, and we have a string that
	 * starts with "P" and a number then we should take the pointer
	 * itself. This is done by stripping off the 'p' and parsing it as a
	 * ulong.
	 *
	 * Further, if it starts with an '&', then we want the address of
	 * a variable, use fsym() to find it.
	 */
	if (((tbl->flags & F_TYPE_IS_PTR) || ((tbl->ptr + nptr) > 0)) &&
		(((UPPER((*buf)[0]) == 'P') && isdigit((*buf)[1])) || ((*buf)[0] == '&'))) {
		if ((*buf)[0] == '&') {
			char *start;

			(*buf)++;
			for (start = *buf; isvarchar(*buf[0]); (*buf)++)
				;
			if ((((ulong *) p)[0] = fsym(start, -1)) == NULL)
				return(FAIL);
		}
		else {
			(*buf)++;
			RIF(xk_par_int(buf, (long *)p, pass));
		}
		if (Ndont == Sdont) {
			if (Dont)
				Dont = (char **) realloc(Dont, (Sdont + 20) * sizeof(char *));
			else
				Dont = (char **) malloc((Sdont + 20) * sizeof(char *));
			if (!Dont) {
				ALTPUTS("Out of space, exiting");
				exit(1);
			}
			Sdont += 20;
		}
		Dont[Ndont++] = ((char **) p)[0];
		return(SUCCESS);
	}
	if (tbl->tname && (spec_parse = find_special(SPEC_PARSE, tbl->tname)))
		return(spec_parse(tbl, buf, p, nptr, sub, pass, tbl_find));
	if (tbl->name && (spec_parse = find_special(SPEC_PARSE, tbl->name)))
		return(spec_parse(tbl, buf, p, nptr, sub, pass, tbl_find));
	nptr += tbl->ptr;
	if (sub > 0 && tbl->subscr > 0) {
		if (_Prdebug)
			fprintf(stderr, "PARSE: WARNING: Multiple array subscripts not handled in %s\n", tbl->name);
		return(FAIL);
	}
	/*
	 * If there is exactly one pointer associated with this
	 * member, and no length delimiters, and no subscripts,
	 * or there are multiple pointers,
	 * then malloc space for the structure and call ourself
	 * recursively.
	 */
	if ((nptr > 1 && tbl->delim != 0) || (nptr == 1 && tbl->delim == 0 && tbl->subscr == 0)) {
		if (PARPEEK(buf, ",") || PARPEEK(buf, Str_close_curly)) {
			((char **)p)[0] = NULL;
			if (_Prdebug)
				fprintf(stderr, "PARSE:\tSetting field '%s' to NULL\n", tbl->name);
			return(SUCCESS);
		}
			if (xk_parpeek(buf, "NULL")) {
				RIF(xk_parexpect(buf, "NULL"));
				((char **)p)[0] = NULL;
				if (_Prdebug)
					fprintf(stderr, "PARSE:\tSetting field '%s' to NULL\n", tbl->name);
				return(SUCCESS);
			}

		if ((((char **)p)[0] = malloc(tbl->size)) == NULL) {
			return(FAIL);
		}
		if (_Prdebug)
			fprintf(stderr, "PARSE:\tSetting %s to malloced address 0x%x size %d\n", tbl->name, ((char **)p)[0], tbl->size);
		return(xk_parse(tbl, buf, ((char **)p)[0], nptr-1-tbl->ptr, sub, pass, tbl_find));
	}
	/*
	 * If there is exactly one pointer level, or one subscripting level,
	 * and there is a delimiter,
	 * and no subscript, then we are a length delimited malloced array.
	 */
	xk_skipwhite(buf);
	if (tbl->delim != 0 && ((nptr == 1 && tbl->subscr == 0) ||
		(nptr == 0 && tbl->subscr != 0 && tbl->kind != K_STRING))) {

		if (tbl->subscr == 0) {
			if (PARPEEK(buf, ",") || PARPEEK(buf, Str_close_curly)) {
				((char **)p)[0] = NULL;
				if (_Prdebug)
					fprintf(stderr, "PARSE: malloc'ed array '%s' set to NULL\n", tbl->name);
				return(SUCCESS);
			}
			if (xk_parpeek(buf, "NULL")) {
				RIF(xk_parexpect(buf, "NULL"));
				((char **)p)[0] = NULL;
				if (_Prdebug)
					fprintf(stderr, "PARSE:\tSetting field '%s' to NULL\n", tbl->name);
				return(SUCCESS);
			}
			nmal = MALMEMBERS;
			if ((np = malloc(nmal*tbl->size)) == NULL) {
				return(FAIL);
			}
			((char **)p)[0] = np;
			if (_Prdebug)
				fprintf(stderr, "PARSE:\tmember %s set to malloced pointer 0x%x size %d\n", tbl->name, np, 4*tbl->size);
		} else {
			np = p;
		}
		xk_skipwhite(buf);
		RIF(PAREXPECT(buf, Str_open_curly));
		*buf += 1;
		i = 0;
		xk_skipwhite(buf);
		while (PARPEEK(buf, Str_close_curly) == FALSE) {
			if (tbl->subscr == 0 && i >= nmal) {
				nmal += MALMEMBERS;
				if((np = realloc(np, nmal*tbl->size)) == NULL) {
					return(FAIL);
				}
				((char **)p)[0] = np;
				if (_Prdebug) {
					fprintf(stderr, "PARSE: array '%s' overflowed, realloced to %d members total size %d\n", tbl->name, nmal, nmal*tbl->size);
				}
			} else if (tbl->subscr > 0 && i > tbl->subscr) {
				if (_Prdebug)
					fprintf(stderr, "PARSE: Array %s overflowed at element number %d\n", tbl->name, i);
			}
			if (_Prdebug)
				fprintf(stderr, "PARSE:\tparsing array element [%d] of '%s' into address 0x%x\n", i, tbl->name, &np[i*tbl->size]);
			if (i) {
				xk_skipwhite(buf);
				if (PARPEEK(buf, ",") == FALSE) {
					if (PARPEEK(buf, Str_close_curly) == FALSE) {
						RIF(PAREXPECT(buf, ","));
						*buf += 1;
					}
				}
				else {
					RIF(PAREXPECT(buf, ","));
					*buf += 1;
				}
			}
			RIF(xk_parse(tbl, buf, &np[i*tbl->size], nptr ? -1 : 0, 0, -1, tbl_find));
			i++;
			struct_size = i;
			xk_skipwhite(buf);
		}
		RIF(PAREXPECT(buf, Str_close_curly));
		*buf += 1;
		return(SUCCESS);
	}
	/*
	 * If there is no delimiter, and there are two levels of pointer,
	 * then we are a NULL terminated array of pointers
	 */
	if (tbl->delim == 0 &&
		((nptr == 2 && sub == 0) || (sub == 1 && nptr == 1))) {
		/*
		 * malloc a few members, realloc as needed
		 */
		nmal = MALMEMBERS;
		if ((((char **)p)[0] = malloc(nmal*tbl->size)) == NULL) {
			return(FAIL);
		}
		xk_skipwhite(buf);
		RIF(PAREXPECT(buf, Str_open_curly));
		*buf += 1;
		xk_skipwhite(buf);
		while (PARPEEK(buf, Str_close_curly) == FALSE) {
			if (i >= nmal) {
				nmal += MALMEMBERS;
				if ((((char **)p)[0] = realloc(((char **)p)[0], nmal*tbl->size)) == NULL) {
					return(FAIL);
				}
				if (_Prdebug) {
					fprintf(stderr, "PARSE: array '%s' overflowed, realloced to %d members total size %d\n", tbl->name, nmal, nmal*tbl->size);
				}
			}
			if (_Prdebug)
				fprintf(stderr, "PARSE:\tparsing array element number %d of %s\n", i, tbl->name);
			if (i) {
				RIF(PAREXPECT(buf, ","));
				*buf += 1;
			}
			RIF(xk_parse(tbl, buf, &p[i*tbl->size], nptr == 2 ? -2 : -1, 0, -1, tbl_find));

			xk_skipwhite(buf);
		}
		RIF(PAREXPECT(buf, Str_close_curly));
		*buf++;
		((char **)p)[i*tbl->size] = NULL;
		return(SUCCESS);
	}

	switch(tbl->kind) {
	case K_CHAR:
		RIF(xk_par_int(buf, &val, pass));
		((unchar *)p)[0] = val;
		break;
	case K_SHORT:
		RIF(xk_par_int(buf, &val, pass));
		((ushort *)p)[0] = val;
		break;
	case K_INT:
		RIF(xk_par_int(buf, &val, pass));
		((int *)p)[0] = val;
		break;
	case K_LONG:
		RIF(xk_par_int(buf, (long *)p, pass));
		break;
	case K_STRING:
		if (tbl->subscr) {
			val = tbl->subscr;
			RIF(xk_par_chararr(buf, (char *)p, &val));
			if (tbl->delim <= 0 && val > -1) {
				p[val] = '\0';
			}
		} else {
			val = 0;
			RIF(xk_par_charstr(buf, (char **)p, &val));
			/* If this is not a delimited char string,
			 * then it must be null terminated
			 */
			if (tbl->delim <= 0 && val > -1) {
				((char **) p)[0][val] = '\0';
			}
			strglen = val;
		}
		break;
	case K_TYPEDEF:
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		RIF(xk_parse(ntbl, buf, p, nptr, 0, pass, tbl_find));
		return(SUCCESS);
	case K_STRUCT:
		xk_skipwhite(buf);
		RIF(PAREXPECT(buf, Str_open_curly));
		*buf += 1;
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		pp = NULL;
		for (i = 0; ntbl[i].name != NULL; i++) {
			_Delim = xk_get_pardelim(&ntbl[i], p);
			if (ntbl[i].kind >= K_DSHORT) {
				skind = ntbl[i].kind;
				pp = p + ntbl[i].offset;
				struct_size = 0;
			}
			if (ntbl[i].delim) {
				delim_type = ntbl[i].kind;
			}
			if (i && ntbl[i-1].kind < K_DSHORT) {
				xk_skipwhite(buf);
				if (PARPEEK(buf, ",") == FALSE) {
					if (PARPEEK(buf, Str_close_curly) == FALSE) {
						RIF(PAREXPECT(buf, ","));
						*buf += 1;
					}
				}
				else  {
					RIF(PAREXPECT(buf, ","));
					*buf += 1;
				}
			}
			if (_Prdebug)
				fprintf(stderr, "PARSE:\tparsing member %s into location 0x%x\n", ntbl[i].name, p + ntbl[i].offset);
			if (xk_parse(&ntbl[i], buf, p+ntbl[i].offset, nptr, sub, pass, tbl_find) == FAIL) {
				if (_Prdebug)
					fprintf(stderr, "PARSE: Failure occured in the '%s' member\n", ntbl[i].name);
				return(FAIL);
			}
		}
		if (pp != NULL) {
			switch(skind) {
				case K_DSHORT:
					if (delim_type == K_STRING)
						((short *)pp)[0] = strglen;
					else
						((short *)pp)[0] = struct_size;
					break;
				case K_DINT:
					if (delim_type == K_STRING)
						((int *)pp)[0] = strglen;
					else
						((int *)pp)[0] = struct_size;
					break;
				case K_DLONG:
					if (delim_type == K_STRING)
						((long *)pp)[0] = strglen;
					else
						((long *)pp)[0] = struct_size;
					break;
				default:
					break;
			}
		}
		xk_skipwhite(buf);
		RIF(PAREXPECT(buf, Str_close_curly));
		*buf += 1;
		break;
	case K_UNION:
		if (strncmp(tbl[-1].name, "ch_", 3) != 0) {
			if (_Prdebug)
				fprintf(stderr, "PARSE: Can not determine choice in %s\n", tbl->name);
			return(FAIL);
		}
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		xk_skipwhite(buf);
		RIF(PAREXPECT(buf, Str_open_curly));
		*buf += 1;
		for (i = 0; ntbl[i].name != NULL; i++) {
			if (xk_parpeek(buf, ntbl[i].name) == TRUE) {
				RIF(xk_parexpect(buf, ntbl[i].name));
				((long *)(p - sizeof(long)))[0] = ntbl[i].choice;
				if (_Prdebug)
					fprintf(stderr, "PARSE:\tparsing union member %s into location 0x%x\n", ntbl[i].name, p + ntbl[i].offset);
				if (xk_parse(&ntbl[i], buf, p, nptr, sub, pass, tbl_find) == FAIL) {
				if (_Prdebug)
					fprintf(stderr, "PARSE: Failure occured in the '%s' member\n", ntbl[i].name);
					return(FAIL);
				}
				break;
			}
		}
		xk_skipwhite(buf);
		RIF(PAREXPECT(buf, Str_close_curly));
		*buf += 1;
		break;
	case K_DSHORT:
	case K_DINT:
	case K_DLONG:
		break;
	default:
		return(FAIL);
	}
	return(SUCCESS);
}

int
xk_get_delim(tbl, p)
memtbl_t *tbl;
char *p;
{
	memtbl_t *dtbl = &tbl[tbl->delim];

	if (tbl->delim == 0) {
		return(-1);
	}
	if (_Prdebug)
		fprintf(stderr, "\tdelimiter for field %s is field %s\n", tbl->name, dtbl->name);
	p += dtbl->offset;
	switch (dtbl->kind) {
	case K_DLONG:
	case K_LONG:
		return(((long *)p)[0]);
	case K_CHAR:
		return(((char *)p)[0]);
	case K_DSHORT:
	case K_SHORT:
		return(((short *)p)[0]);
	case K_DINT:
	case K_INT:
		return(((int *)p)[0]);
	default:
	if (_Prdebug)
		fprintf(stderr, "\tcan not find delimiter value in %s\n", tbl->name);
		return(0);
	}
}
int
xk_get_pardelim(tbl, p)
memtbl_t *tbl;
char *p;
{
	memtbl_t *dtbl = &tbl[tbl->delim];

	if (tbl->delim == 0) {
		return(-1);
	}
	if (_Prdebug)
		fprintf(stderr, "\tdelimiter for field %s is field %s\n", tbl->name, dtbl->name);
	p += dtbl->offset;
	switch (dtbl->kind) {
	case K_DLONG:
		return(-1);
	case K_LONG:
		return(((long *)p)[0]);
	case K_DSHORT:
		return(-1);
	case K_CHAR:
		return(((char *)p)[0]);
	case K_SHORT:
		return(((short *)p)[0]);
	case K_DINT:
		return(-1);
	case K_INT:
		return(((int *)p)[0]);
	default:
	if (_Prdebug)
		fprintf(stderr, "\tcan not find delimiter value in %s\n", tbl->name);
		return(0);
	}
}

/*
 * xk_print:  Takes a pointer to a structure member table, a pointer
 * to a buffer big enough to hold an ascii representation of the structure
 * represented by the table pointer, and a pointer to a structure to
 * be filled in, and the number of pointers saved
 * from previous recursive calls to this routine, and prints the
 * buf into p.
 *
 * Increments buf to the last point at which it wrote a character,
 * and returns SUCCESS or FAIL.
 */


int
xk_print(tbl, buf, p, nptr, sub, pass, tbl_find)
memtbl_t *tbl;
char **buf;		/* ascii representation of the structure */
char *p;		/* pointer to the structure to be printed */
int nptr;		/* levels of indirection from previous calls */
int sub;		/* subscript from previous calls, or 0 if none */
VOID *pass; /* name for index into env table */
memtbl_t *(*tbl_find)();
{
	memtbl_t *ntbl;
	register int i;
	long val;		/* used for choice selection */
	char *np;
	int delim = _Delim;
	int (*spec_print)();

	if (p == NULL) {
		*buf += lsprintf(*buf, "NULL");
		return(SUCCESS);
	}
	if (tbl == NULL) {
		if (_Prdebug)
			fprintf(stderr, "PRINT: NULL type table!!!\n");
		return(FAIL);
	}
	if (tbl->tname && (spec_print = find_special(SPEC_PRINT, tbl->tname)))
		return(spec_print(tbl, buf, p, nptr, sub, pass, tbl_find));
	if (tbl->name && (spec_print = find_special(SPEC_PRINT, tbl->name)))
		return(spec_print(tbl, buf, p, nptr, sub, pass, tbl_find));
	nptr += tbl->ptr;
	if (sub > 0 && tbl->subscr > 0) {
	if (_Prdebug)
		fprintf(stderr, "PRINT: Multiple array subscripts not handled in %s\n", tbl->name);
		return(FAIL);
	}
	/*
	 * If there is exactly one pointer associated with this
	 * member, and no length delimiters, and no subscripts,
	 * or there are multiple pointers,
	 * then dereference the structure and call ourself
	 * recursively.
	 */
	if ((nptr > 1 && tbl->delim != 0) || (nptr == 1 && tbl->delim == 0 && tbl->subscr == 0)) {
		if (_Prdebug)
			fprintf(stderr, "PRINT: Dereferencing %s to address 0x%x\n", tbl->name, ((char **)p)[0]);
		return(xk_print(tbl, buf, ((char **)p)[0], nptr-1-tbl->ptr, sub, pass, tbl_find));
	}
	/*
	 * If there is exactly one pointer level, or one subscripting level,
	 * and there is a delimiter,
	 * and no subscript, then we are a length delimited array.
	 */
	if (tbl->delim != 0 && ((nptr == 1 && tbl->subscr == 0) ||
		nptr == 0 && tbl->subscr != 0 && tbl->kind != K_STRING)) {

		if (_Prdebug)
			fprintf(stderr, "PRINT: DELIM of %s: %d\n", tbl->name, delim);
		if (tbl->subscr == 0) {
			np = ((char **)p)[0];
			if (_Prdebug)
				fprintf(stderr, "PRINT:\tusing pointer 0x%x for array\n", np);
		} else {
			np = p;
		}
		if (np == NULL) {
		 	*buf += lsprintf(*buf, "NULL");
			return(SUCCESS);
		}
		*buf += lsprintf(*buf, Str_open_curly);
		for (i = 0; i < delim; i++) {
			if (_Prdebug)
				fprintf(stderr, "PRINT:\tprinting array level %d of member %s at location 0x%x\n", i, tbl->name, &np[i*tbl->size]);
			if (i)
				*buf += lsprintf(*buf, ", ");
			RIF(xk_print(tbl, buf, &np[i*tbl->size], nptr ? -1 : 0, 0, -1, tbl_find));
		}
		*buf += lsprintf(*buf, Str_close_curly);
		return(SUCCESS);
	}
	/*
	 * If there is no delimiter, and there are two levels of pointer,
	 * then we are a NULL terminated array.
	 */
	if (tbl->delim == 0 &&
		((nptr == 2 && sub == 0) || (sub == 1 && nptr == 1))) {
		*buf += lsprintf(*buf, Str_open_curly);
		for (i = 0; ((char **)p)[i*tbl->size] != NULL; i++) {
			if (i)
				*buf += lsprintf(*buf, ", ");
			if (_Prdebug)
				fprintf(stderr, "PRINT:\tprinting array level %d of member %s\n", i, tbl->name);
			RIF(xk_print(tbl, buf, ((char **)p)[i*tbl->size], nptr-(!sub), 0, -1, tbl_find));
		}
		*buf += lsprintf(*buf, Str_close_curly);
		return(SUCCESS);
	}

	if (!Pr_tmpnonames && (Pr_format & PRNAMES)) {
		switch(tbl->kind) {
		case K_CHAR:
		case K_SHORT:
		case K_INT:
		case K_LONG:
		case K_STRING:
			*buf += lsprintf(*buf, "%s=", tbl->name);
		}
	}
	switch(tbl->kind) {
	case K_CHAR:
	case K_SHORT:
	case K_INT:
	case K_LONG:
		RIF(xk_prin_int(tbl, buf, p, pass));
		break;
	case K_STRING:
		if (delim > 0) {
			if (tbl->subscr) {
				RIF(xk_prin_hexstr(buf, (char *)p, delim));
			} else {
				RIF(xk_prin_hexstr(buf, ((char **)p)[0], delim));
			}
		} else {
			if (tbl->subscr) {
				RIF(xk_prin_nts(buf, (char *)p));
			} else {
				RIF(xk_prin_nts(buf, ((char **)p)[0]));
			}
		}
		break;
	case K_TYPEDEF:
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		return(xk_print(ntbl, buf, p, nptr, 0, pass, tbl_find));
	case K_STRUCT:
		*buf += lsprintf(*buf, Str_open_curly);
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		for (i = 0; ntbl[i].name != NULL; i++) {
			_Delim = xk_get_delim(&ntbl[i], p);
			if (_Prdebug)
				fprintf(stderr, "PRINT:\tprinting member %s at location 0x%x\n", ntbl[i].name, p+ntbl[i].offset);
			RIF(xk_print(&ntbl[i], buf, p+ntbl[i].offset, nptr, sub, pass, tbl_find));
			if (ntbl[i].kind < K_DSHORT && ntbl[i+1].name != NULL)
				*buf += lsprintf(*buf, ", ");
		}
		*buf += lsprintf(*buf, Str_close_curly);
		break;
	case K_UNION:
		if (strncmp(tbl[-1].name, "ch_", 3) != 0) {
			if (_Prdebug)
				fprintf(stderr, "PRINT: can not determine choice in %s\n", tbl->name);
			return(FAIL);
		}
		val = *((long *)(p - sizeof(long)));
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		*buf += lsprintf(*buf, Str_open_curly);
		for (i = 0; ntbl[i].name != NULL; i++) {
			if (ntbl[i].choice == val) {
				*buf += lsprintf(*buf, "%s ", ntbl[i].name);
				if (_Prdebug)
					fprintf(stderr, "PRINT:\tprinting union member %s into location 0x%x\n", ntbl[i].name, p + ntbl[i].offset);
				RIF(xk_print(&ntbl[i], buf, p, nptr, sub, pass, tbl_find));
				break;
			}
		}
		*buf += lsprintf(*buf, Str_close_curly);
		break;
	case K_DSHORT:
	case K_DINT:
	case K_DLONG:
		break;
	default:
		return(FAIL);
	}
	return(SUCCESS);
}

/*
 * xk_free:  Takes a pointer to a structure member table, and
 * free any malloc'ec elements in it at all levels.
 * Returns SUCCESS or FAIL.
 *
 * Contains an optimization that if a structure or union contains a
 * type that is a simple type and nptr is zero, does not do a recursive call.
 */

int
xk_free(tbl, p, nptr, sub, tbl_find)
memtbl_t *tbl;
char *p;		/* pointer to the structure to be filled in */
int nptr;		/* levels of indirection from previous calls */
int sub;		/* subscript from prev calls, or 0 if none */
memtbl_t *(*tbl_find)();
{
	memtbl_t *ntbl;
	register int i;
	long val;		/* used for choice selection */
	char *np;
	int delim = _Delim;
	int (*spec_free)();

	if (tbl == NULL) {
		if (_Prdebug)
			fprintf(stderr, "FREE: NULL type table!!!\n");
		return(FAIL);
	}
	if (tbl->tname && (spec_free = find_special(SPEC_FREE, tbl->tname)))
		return(spec_free(tbl, p, nptr, sub, tbl_find));
	if (tbl->name && (spec_free = find_special(SPEC_FREE, tbl->name)))
		return(spec_free(tbl, p, nptr, sub, tbl_find));
	nptr += tbl->ptr;
	if ((tbl->flags & F_TYPE_IS_PTR) || (nptr > 0)) {
		for (i = Ndont - 1; i >= 0; i--) {
			if (Dont[i] == ((char **) p)[0]) {
				for ( ; i < Ndont - 1; i++)
					Dont[i] = Dont[i + 1];
				Ndont--;
				return(SUCCESS);
			}
		}
	}
	if (sub > 0 && tbl->subscr > 0) {
		if (_Prdebug)
			fprintf(stderr, "FREE: WARNING: Multiple array subscripts not handled in %s\n", tbl->name);
		return(FAIL);
	}
	/*
	 * If there is exactly one pointer associated with this
	 * member, and no length delimiters, and no subscripts,
	 * or there are multiple pointers,
	 * then recursively call ourselves on the structure, then
	 * free the structure itself.
	 */
	if ((nptr > 1 && tbl->delim != 0) || (nptr == 1 && tbl->delim == 0 && tbl->subscr == 0)) {
		if (((char **)p)[0] != NULL) {
			RIF(xk_free(tbl, ((char **)p)[0], nptr-1-tbl->ptr, sub, tbl_find));
			free(((char **)p)[0]);
			if (_Prdebug)
				fprintf(stderr, "FREE: member '%s' at location 0x%x freed and set to NULL\n", tbl->name, ((char **)p)[0]);
			((char **)p)[0] = NULL;
		} else {
			if (_Prdebug)
				fprintf(stderr, "FREE: member '%s' is NULL, no free\n", tbl->name);
		}
		return(SUCCESS);
	}
	/*
	 * If there is exactly one pointer level, or one subscripting level,
	 * and there is a delimiter,
	 * and no subscript, then we are a length delimited malloced array.
	 * Free each element, then free the whole array.
	 */
	if (tbl->delim != 0 && ((nptr == 1 && tbl->subscr == 0) ||
		nptr == 0 && tbl->subscr != 0 && tbl->kind != K_STRING)) {
		if (_Prdebug)
			fprintf(stderr, "FREE: DELIM of %s: %d\n", tbl->name, delim);
		if (tbl->subscr == 0)
			np = ((char **)p)[0];
		else
			np = p;
		for (i = 0; i < delim; i++) {
			if (_Prdebug)
				fprintf(stderr, "FREE: Freeing array element [%d] of '%s' at address 0x%x\n", i, tbl->name, &np[i*tbl->size]);
			RIF(xk_free(tbl, &np[i*tbl->size], nptr ? -1 : 0, 0, tbl_find));
		}
		if (tbl->subscr == 0) {
			if (np != NULL) {
				if (_Prdebug)
					fprintf(stderr, "FREE: freeing pointer to array of '%s' at location 0x%x and setting to NULL\n", tbl->name, np);
				free(np);
				if (tbl->subscr == 0)
					((char **)p)[0] = NULL;
			} else if (_Prdebug)
				fprintf(stderr, "FREE: pointer to array of '%s'is NULL, no free\n", tbl->name);
		}
		return(SUCCESS);
	}

	switch(tbl->kind) {
	case K_DSHORT:
	case K_SHORT:
	case K_DINT:
	case K_INT:
	case K_DLONG:
	case K_LONG:
		break;
	case K_STRING:
		if (!tbl->subscr) {
			if (((char **)p)[0] != NULL) {
				if (_Prdebug)
					fprintf(stderr, "FREE: freeing string '%s' at location 0x%x, and setting to NULL\n", tbl->name, p);
				free(((char **)p)[0]);
				((char **)p)[0] = NULL;
			} else if (_Prdebug)
				fprintf(stderr, "FREE: string '%s' is NULL, no free\n", tbl->name, p);
		}
		break;
	case K_TYPEDEF:
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		return(xk_free(ntbl, p, nptr, 0, tbl_find));
	case K_STRUCT:
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		for (i = 0; ntbl[i].name != NULL; i++) {
			if ((ntbl[i].flags & F_SIMPLE) && nptr+ntbl[i].ptr == 0) {
				if (_Prdebug)
					fprintf(stderr, "FREE:\tsimple member %s at location 0x%x NEEDS NO FREE\n", ntbl[i].name, p + ntbl[i].offset);
				continue;
			}
			_Delim = xk_get_delim(&ntbl[i], p);
			if (_Prdebug)
				fprintf(stderr, "FREE:\tfreeing member %s at location 0x%x\n", ntbl[i].name, p + ntbl[i].offset);
			if (xk_free(&ntbl[i], p+ntbl[i].offset, nptr, sub, tbl_find) == FAIL) {
			if (_Prdebug)
				fprintf(stderr, "FREE: failure occured in the '%s' member\n", ntbl[i].name);
				return(FAIL);
			}
		}
		break;
	case K_UNION:
		if (strncmp(tbl[-1].name, "ch_", 3) != 0) {
			if (_Prdebug)
				fprintf(stderr, "FREE: can not determine choice in %s\n", tbl->name);
			return(FAIL);
		}
		val = *((long *)(p - sizeof(long)));
		ntbl = tbl_find(tbl->tname, tbl->tbl, tbl->id);
		for (i = 0; ntbl[i].name != NULL; i++) {
			if (ntbl[i].choice == val) {
				if ((ntbl[i].flags & F_SIMPLE) && nptr+ntbl[i].ptr == 0) {
					if (_Prdebug)
						fprintf(stderr, "FREE:\tsimple member %s at location 0x%x NEEDS NO FREE\n", ntbl[i].name, p + ntbl[i].offset);
					continue;
				}
				if (_Prdebug)
					fprintf(stderr, "FREE:\tfreeing union member %s into location 0x%x\n", ntbl[i].name, p + ntbl[i].offset);
				RIF(xk_free(&ntbl[i], p, nptr, sub, tbl_find));
				break;
			}
		}
		if (ntbl[i].name == NULL && _Prdebug)
			if (_Prdebug)
				fprintf(stderr, "FREE:\tno legal union choice for '%s' (value is 0x%x(%d), so no free\n", tbl->name, val, val);
		break;
	default:
		return(FAIL);
	}
	return(SUCCESS);
}
