#ident	"@(#)wksh:xksrc/docall.c	1.1"
/*	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T    */
/*	and UNIX System Laboratories, Inc.			*/
/*	The copyright notice above does not evidence any       */
/*	actual or intended publication of such source code.    */

#undef printf

#include <stdio.h>
#include "sh_config.h" /* which includes sys/types.h */
/*#include <sys/types.h>*/
#include <sys/param.h>
#include <string.h>
#include <varargs.h>
#include <search.h>
#include <ctype.h>
#include "xksh.h"

static long get_prdebug(), set_prdebug();
memtbl_t *all_tbl_find();
memtbl_t *all_tbl_search();
memtbl_t *asl_find();

static memtbl_t Null_tbl = { NULL };

const static char use[] = "0x%x";
const static char use2[] = "%s=0x%x";
extern char STR_ulong[];
extern char STR_string_t[];
extern int Pr_tmpnonames;

#define TREAT_SIMPLE(TBL) ((TBL)->ptr || IS_SIMPLE(TBL))

int Xk_errno = 0;

static VOID *Hashnams;

int Xkdebug = 0;
extern memtbl_t T_ulong[], T_string_t[];
static struct memtbl **Dynmem = NULL;
static int Ndynmem = 0;
static int Sdynmem = 0;

static struct symarray *Dyndef = NULL;
static int Ndyndef = 0;
static int Sdyndef = 0;

#define NOHASH		1
#define TYPEONLY	2
#define STRUCTONLY	4

char xk_ret_buffer[100];
char *xk_ret_buf = xk_ret_buffer;
struct Bfunction xk_prdebug = { get_prdebug, set_prdebug };

struct deflist {
	char *prefix;
	int size;
	struct symarray *defs;
};

struct deflist *Deflist = NULL;
int Ndeflist;

struct structlist {
	char *prefix;
	int id;
	int size;
	struct memtbl **mem;
};

struct structlist *Structlist = NULL;
int Nstructlist;

struct symlist *Symlist = NULL;
int Nsymlist;

void
altperror(s)
char *s;
{
	if (s)
		altfprintf(2, "%s: ", s);
#ifdef HAS_STRERROR
	altfputs(2, strerror(Xk_errno));
#else /* NO strerror() */
#ifdef HAS_SYS_ERRLIST
	{
		extern char *sys_errlist[];
		extern int sys_nerr;

		if (Xk_errno < sys_nerr)
			altfputs(2, sys_errlist[Xk_errno]);
		else
			altfprintf(2, "Unknown Error: %d\n", Xk_errno);
	}
#else /* NO sys_errlist either */
	altfprintf(2, "Unknown Error: %d\n", Xk_errno);
#endif
#endif 
}

int
symcomp(sym1, sym2)
const VOID *sym1, *sym2;
{
	return(strcmp(((struct symarray *) sym1)->str, ((struct symarray *) sym2)->str));
}

#ifndef STRTOUL_AVAILABLE

#define NPTR (nptr ? nptr : &ptr)

unsigned long
strtoul(str, nptr, base)
register const char *str;
char **nptr;
int base;
{
	register int c;
	int neg = 0;

	if (!isalnum(c = *str)) {
		while (isspace(c))
			c = *++str;
		switch (c) {
		case '-':
			neg++;
			/* FALLTHROUGH */
		case '+':
			c = *++str;
		}
	}
	/* If the number is negative, we can just call strtol */
	if (neg)
		return(-strtol(str, nptr, base));
	if (c != '0') {
		if (!base)
			base = 10;
	}
	else if (str[1] == 'x' || str[1] == 'X') {
		if (!base || (base == 16)) {
			str += 2;
			base = 16;
		}
	}
	else {
		if (!base || (base == 8)) {
			base = 8;
			str++;
		}
	}

	if (!str[0]) {
		if (nptr)
			*nptr = (char *) str;
		return(0);
	}
	/*
	** Short buffers are fine, quick, cheap optimization
	*/
	if (!str[1] || !str[2] || !str[3])
		return(strtol(str, nptr, base));
	/*
	** Break the buffer into all of the characters but the last and the
	** last character.  The result is the first part multiplied by the
	** base plus the last part.
	*/
	{
		char *last_spot;
		unsigned long part;
		char *ptr;

		last_spot = (char *) str + strlen(str) - 1;
		c = *last_spot;
		*last_spot = '\0';
		part = strtol(str, NPTR, base);
		*last_spot = c;
		if (*NPTR == last_spot)
			return(part * base + strtol(last_spot, NPTR, base));
		return(part);
	}
}
#endif /* not STRTOUL_AVAILABLE */

VOID *
getaddr(str)
char *str;
{
	if (isdigit(str[0]))
		return((VOID *) strtoul(str, NULL, 0));
	else
		return((VOID *) fsym(str, -1));
}

static
memtbl_t *
ffind(tbl, fld, pptr)
memtbl_t *tbl;
char *fld;
char **pptr;
{
	static memtbl_t tbluse[2];
	memtbl_t *tbl2;
	char *p, *q, op;
	unsigned int len, sub;

	if (!fld || !(*fld))
		return(tbl);
	tbl2 = tbluse;
	tbluse[0] = *tbl;
	tbluse[1] = Null_tbl;
	q = fld;
	while (tbl2 && q && *q) {
		p = q;
		if (*q == '[') {
			if (!tbl2->ptr)
				return(NULL);
			q++;
			xk_par_int(&q, &sub, NULL);
			if (*q != ']')
				return(NULL);
			*pptr = ((char **) (*pptr))[0];
			*pptr += sub * tbl2->size;
			q++;
			tbluse[0].ptr--;
			continue;
		}
		if ((len = strcspn(p, "[.")) < strlen(p)) {
			q = p + len;
			op = *q;
			*q = '\0';
		}
		else
			q = NULL;
		tbl2 = asl_find(NULL, tbluse, p, pptr);
		if (tbl2 && (tbl2 != tbluse)) {
			/* A field should not be a subfield of itself */

			tbluse[0] = *tbl2;
			tbl2 = tbluse;
			tbl2->name = ".";
		}
		if (q) {
			if (op == '.')
				*q++ = op;
			else
				*q = op;
		}
	}
	return(tbl2);
}

static
growdef()
{
	if (!(Dyndef = (struct symarray *) realloc(Dyndef, (Sdyndef + 20) * sizeof(struct symarray))))
		return(SH_FAIL);
	Deflist->defs = Dyndef;
	Sdyndef += 20;
}

do_define(argc, argv)
int argc;
char **argv;
{
	int i, argstart, redo;
	char *name;
	struct symarray *found, dummy;

	if (C_PAIR(argv[1], '-', 'R')) {
		redo = 0;
		argstart = 2;
	}
	else {
		argstart = 1;
		redo = 1;
	}
	if (!argv[argstart] || !argv[argstart + 1]) {
		altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		XK_USAGE(argv[0]);
	}
	name = argv[argstart++];
	dummy.str = name;
	found = (struct symarray *) bsearch((char *) &dummy, Dyndef, Ndyndef, sizeof(struct symarray), symcomp);
	if (found) {
		if (!redo)
			return(SH_SUCC);
		i = found - Dyndef;
	}
	else {
		if (Sdyndef == Ndyndef)
			growdef();
		Ndyndef++;
		if (Ndyndef > 1)
			for (i = Ndyndef - 1; i > 0; i--) {
				if (strcmp(name, Dyndef[i - 1].str) >= 0)
					break;
				Dyndef[i] = Dyndef[i - 1];
			}
		else
			i = 0;
		Dyndef[i].str = strdup(name);
		Deflist->size++;
	}
	RIF(xk_par_int(argv + argstart, &Dyndef[i].addr, NULL));
	return(SH_SUCC);
}

do_field_get(argc, argv)
int argc;
char **argv;
{
	char buf[10 * BUFSIZ], *p, *bufstart;
	char *fld, *type, *ptr, *ptr2;
	memtbl_t tbl[2], *tbl2;
	int i;
	char *targvar;
	char fail = 0;

	i = 1;
	if (argv[i] && argv[i + 1] && C_PAIR(argv[1], '-', 'v')) {
		targvar = argv[i + 1];
		i += 2;
	}
	else
		targvar = NULL;
	type = argv[i++];
	ptr = (char *) getaddr(argv[i++]);
	tbl[1] = Null_tbl;
	if (!type || !ptr || (parse_decl(tbl, type, 1) == FAIL)) {
		if (!type || !ptr)
			altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		else
			altprintf((const char *) "Cannot parse %s\n", type);
		XK_USAGE(argv[0]);
	}
	Pr_tmpnonames = 1;
	p = buf;
	if (targvar) {
		strcpy(p, targvar);
		p += strlen(p);
		*p++ = '=';
		bufstart = p;
	}
	else
		bufstart = buf;
	while (fld = argv[i++]) {
		if (p != bufstart)
			*p++ = targvar ? ' ' : '\n';
		tbl2 = tbl;
		ptr2 = ptr;
		if (!C_PAIR(fld, '.', '\0'))
			tbl2 = ffind(tbl, fld, &ptr2);
		if (!tbl2) {
			altprintf((const char *) "Cannot find %s\n", fld);
			fail = 1;
			break;
		}
		if (xk_print(tbl2, &p, ptr2, 0, 0, NULL, all_tbl_find) == FAIL) {
			altprintf((const char *) "Cannot print %s\n", fld);
			fail = 1;
			break;
		}
	}
	if (!fail) {
		*p = '\0';
		if (targvar)
			env_set(buf);
		else
			ALTPUTS(buf);
	}
	Pr_tmpnonames = 0;
	return(fail ? SH_FAIL : SH_SUCC);
}

static
freemem(mem)
struct memtbl *mem;
{
	free(mem->name);
	/*
	int i;

	** Because structures and typedefs now inherit fields (i.e. copy
	** the memtbl entry) we must keep the fields of a structure
	** around permanently, (unless we implement a reference count).
	** Let's keep the code handy in case we do.
		if (mem->kind == K_STRUCT) {
			struct memtbl *fmem;

			fmem = Dynmem[mem->tbl];
			for (i = 0; fmem[i].name; i++) {
				free(fmem[i].name);
				if (fmem[i].tname)
					free(fmem[i].tname);
			}
		}
	*/
	free(mem);
}

static
growmem()
{
	if (!(Dynmem = (struct memtbl **) realloc(Dynmem, (Sdynmem + 20) * sizeof(memtbl_t *))))
		return(SH_FAIL);
	chg_structlist(Dynmem, DYNMEM_ID);
	Sdynmem += 20;
}

do_struct(argc, argv)
int argc;
char **argv;
{
	struct memtbl *mem, *fmem;
	int i, j, argstart, redo;
	char *name, *fname;
	char *p;

	if (C_PAIR(argv[1], '-', 'R')) {
		redo = 0;
		argstart = 2;
	}
	else {
		argstart = 1;
		redo = 1;
	}
	if (!argv[argstart] || !argv[argstart + 1]) {
		altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		XK_USAGE(argv[0]);
	}
	name = argv[argstart++];
	for (i = 0; i < Ndynmem; i++)
		if (!(Dynmem[i]->flags & F_FIELD) && (strcmp(name, Dynmem[i]->name) == 0))
			break;
	if ((i < Ndynmem) && !redo) {
		if (!redo)
			return(SH_SUCC);
		if (Sdynmem - Ndynmem < 1)
			growmem();
	}
	else if (Sdynmem - Ndynmem < 2)
		growmem();
	/*
	** Number of memtbls needed: two for structure table and one for
	** each field plus one for null termination.  The number of
	** fields is argc - 2.
	*/
	if (!(mem = (struct memtbl *) malloc(2 * sizeof(struct memtbl))))
		return(SH_FAIL);
	if (!(fmem = (struct memtbl *) malloc((argc - 1) * sizeof(struct memtbl))))
		return(SH_FAIL);
	memset(mem, '\0', 2 * sizeof(struct memtbl));
	memset(fmem, '\0', (argc - 1) * sizeof(struct memtbl));
	if (i < Ndynmem) {
		mem->tbl = Ndynmem++;
		freemem(Dynmem[i]);
		xkhash_override(Hashnams, name, mem);
	}
	else {
		Ndynmem += 2;
		mem->tbl = i + 1;
	}
	Dynmem[i] = mem;
	Dynmem[mem->tbl] = fmem;
	mem->flags = F_TBL_IS_PTR;
	mem->id = DYNMEM_ID;
	mem->name = strdup(name);
	mem->kind = K_STRUCT;
	for (j = argstart; argv[j]; j++) {
		if (p = strchr(argv[j], ':')) {
			fname = malloc(p - argv[j] + 1);
			strncpy(fname, argv[j], p - argv[j]);
			fname[p - argv[j]] = '\0';
			parse_decl(fmem + j - argstart, p + 1, 0);
		}
		else {
			fname = strdup(argv[j]);
			fmem[j - argstart] = T_ulong[0];
		}
		fmem[j - argstart].name = fname;
		fmem[j - argstart].flags |= F_FIELD;
		fmem[j - argstart].offset = mem->size;
		mem->size += (fmem[j - argstart].ptr) ? sizeof(VOID *) : fmem[j - argstart].size;
	}
	return(SH_SUCC);
}

do_typedef(argc, argv)
int argc;
char **argv;
{
	struct memtbl *mem;
	int i, redo;
	char *name, *decl;

	if (C_PAIR(argv[1], '-', 'R')) {
		redo = 0;
		name = argv[3];
		decl = argv[2];
	}
	else {
		name = argv[2];
		decl = argv[1];
		redo = 1;
	}
	if (!name || !decl) {
		altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		XK_USAGE(argv[0]);
	}
	for (i = 0; i < Ndynmem; i++)
		if (!(Dynmem[i]->flags & F_FIELD) && (strcmp(name, Dynmem[i]->name) == 0))
			break;
	if ((i < Ndynmem) && !redo) {
		if (!redo)
			return(SH_SUCC);
	}
	else if (Sdynmem - Ndynmem < 1)
		growmem();
	if (!(mem = (struct memtbl *) malloc(2 * sizeof(struct memtbl))))
		return(SH_FAIL);
	mem[1] = Null_tbl;
	if (i < Ndynmem) {
		freemem(Dynmem[i]);
		xkhash_override(Hashnams, name, mem);
	}
	else
		Ndynmem++;
	Dynmem[i] = mem;
	parse_decl(mem, decl, 0);
	mem->name = strdup(name);
	return(SH_SUCC);
}

static char *
endtok(start)
char *start;
{
	while(*start && !isspace(*start))
		start++;
	return(start);
}

static
parse_decl(mem, decl, tst)
struct memtbl *mem;
char *decl;
int tst;
{
	struct memtbl *tbl;
	char *p, *end;
	char hold;
	int flag = 0, done;

	end = decl;
	do {
		p = end;
		xk_skipwhite(&p);
		end = endtok(p);
		hold = *end;
		*end = '\0';
		done = ((strcmp(p, (const char *) "struct") != 0) &&
			(strcmp(p, (const char *) "const") != 0) &&
			(strcmp(p, (const char *) "unsigned") != 0) &&
			(strcmp(p, (const char *) "signed") != 0) &&
			(strcmp(p, (const char *) "union") != 0));
		*end = hold;
	} while (!done && hold);
	if (!p[0]) {
		if (tst) {
			altprintf((const char *) "Cannot parse %s\n", decl);
			return(FAIL);
		}
		altprintf((const char *) "Cannot parse %s, using ulong\n", decl);
		mem[0] = T_ulong[0];
		return(SUCCESS);
	}
	hold = *end;
	*end = '\0';
	tbl = all_tbl_search(p, flag|NOHASH);
	*end = hold;
	if (!tbl) {
		if (tst) {
			altprintf((const char *) "Cannot parse %s\n", decl);
			return(FAIL);
		}
		altprintf((const char *) "Cannot parse %s, using ulong\n", decl);
		mem[0] = T_ulong[0];
		return(SUCCESS);
	}
	mem[0] = tbl[0];
	for (p = end; *p; p++) {
		switch(*p) {
		case '[':
			{
				char *q = strchr(p, ']');

				if (!q) {
					altprintf((const char *) "Cannot find ]\n");
					continue;
				}
				p++;
				xk_par_int(&p, &(mem->subscr), NULL);
				mem->flags &= ~(F_SIMPLE);
				if (mem->subscr)
					mem->size *= mem->subscr;
				p = q;
				break;
			}
		case '*':
			if ((mem->kind == K_CHAR) && !(mem->ptr)) {
				char *name;

				name = mem->name;
				mem[0] = T_string_t[0];
				mem->name = name;
			}
			else {
				mem->ptr++;
				mem->flags &= ~(F_SIMPLE);
			}
			break;
		}
	}
	return(SUCCESS);
}

/*
** There is an implicit dirty trick going on here.  It is effective,
** efficient and will work anywhere but it is tricky.  A memtbl has
** strings in it.  The fact that a byte-by-byte comparison is being
** done on a memtbl means that pointers are being compared.  This
** means that EVERY UNIQUE MEMTBL SHOULD HAVE SOME UNIQUE FIELD (i.e.
** in this case, the string for the name field).  If somebody uses
** an algorithm in do_struct() that saves string space (by seeing if
** the same string is lying around) this code will break and an ID
** field will be necessary to maintain uniqueness.
*/
struct symlist *
fsymbolic(tbl, sym)
struct memtbl *tbl;
struct symlist *sym;
{
	int i;

	for (i = 0; i < Nsymlist; i++)
		if (memcmp(tbl, &Symlist[i].tbl, sizeof(struct memtbl)) == 0)
			return(Symlist + i);
	return(NULL);
}

do_symbolic(argc, argv)
int argc;
char **argv;
{
	int i, nsyms, isflag;
	ulong j;
	struct symarray syms[50];
	struct memtbl *tbl;
	char *p;
	char *type;

	nsyms = 0;
	isflag = 0;
	for (i = 1; argv[i]; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j]; j++) {
				switch(argv[i][j]) {
				case 'm':
					isflag = 1;
					break;
				case 't':
					if (argv[i][j + 1])
						type = argv[i] + j + 1;
					else
						type = argv[++i];
					j = strlen(argv[i]) - 1;
					break;
				}
			}
		}
		else {
			syms[nsyms++].str = argv[i];
			if (nsyms == 50)
				break;
		}
	}
	if (!nsyms) {
		altprintf((const char *) "Symbolics must be given\n");
		XK_USAGE(argv[0]);
	}
	if (p = strchr(type, '.')) {
		*p = '\0';
		if ((tbl = all_tbl_search(type, 0)) == NULL) {
			*p = '.';
			altprintf((const char *) "Cannot find %s\n", type);
			XK_USAGE(argv[0]);
		}
		if ((tbl = ffind(tbl, p + 1, NULL)) == NULL) {
			*p = '.';
			altprintf((const char *) "Cannot find %s\n", type);
			XK_USAGE(argv[0]);
		}
		*p = '.';
	}
	else if ((tbl = all_tbl_search(type, 0)) == NULL) {
		altprintf((const char *) "Cannot find %s\n", type);
		XK_USAGE(argv[0]);
	}
		
	for (i = 0; i < nsyms; i++) {
		if (!fdef(syms[i].str, &j)) {
			altprintf((const char *) "Cannot find %s\n", syms[i].str);
			XK_USAGE(argv[0]);
		}
		syms[i].str = strdup(syms[i].str);
		syms[i].addr = j;
	}
	add_symbolic(isflag, tbl, syms, nsyms);
	return(SH_SUCC);
}

add_symbolic(isflag, tbl, syms, nsyms)
int isflag;
struct memtbl *tbl;
struct symarray *syms;
int nsyms;
{
	struct symlist *symptr;

	if ((symptr = fsymbolic(tbl)) == NULL) {
		if (!Symlist)
			Symlist = (struct symlist *) malloc((Nsymlist + 1) * sizeof(struct symlist));
		else
			Symlist = (struct symlist *) realloc(Symlist, (Nsymlist + 1) * sizeof(struct symlist));
		if (!Symlist)
			return(SH_FAIL);
		symptr = Symlist + Nsymlist;
		Nsymlist++;
	}
	else
		free(symptr->syms);
	symptr->tbl = *tbl;
	symptr->nsyms = nsyms;
	symptr->isflag = isflag;
	symptr->syms = (struct symarray *) malloc(nsyms * sizeof(struct symarray));
	memcpy(symptr->syms, syms, nsyms * sizeof(struct symarray));
}

fdef(str, val)
char *str;
ulong *val;
{
	struct symarray *found, dummy;
	int i;

	dummy.str = str;
	if (!Deflist)
		return(0);
	for (i = 0; i < Ndeflist; i++) {
		if (Deflist[i].defs) {
			if (Deflist[i].size < 0)
				found = (struct symarray *) lfind((char *) &dummy, Deflist[i].defs, (unsigned int *) &Deflist[i].size, sizeof(struct symarray), symcomp);
			else
				found = (struct symarray *) bsearch((char *) &dummy, Deflist[i].defs, Deflist[i].size, sizeof(struct symarray), symcomp);
			if (found != NULL) {
				*val = found->addr;
				return(1);
			}
		}
	}
	return(0);
}

do_structlist(argc, argv)
int argc;
char **argv;
{
	int i, j, id = 0;
	char *prefix = NULL;
	struct memtbl **memptr;

	for (i = 1; argv[i]; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j]; j++) {
				switch(argv[i][j]) {
				case 'i':
					if (argv[i][j + 1])
						fdef(argv[i] + j + 1, &id);
					else
						fdef(argv[++i], &id);
					j = strlen(argv[i]) - 1;
					break;
				case 'p':
					if (argv[i][j + 1])
						prefix = argv[i] + j + 1;
					else
						prefix = argv[++i];
					j = strlen(prefix) - 1;
					break;
				default:
					altprintf((const char *) "Illegal option to structlist: %s", argv[i]);
					XK_USAGE(argv[0]);
				}
			}
		}
		else {
			if ((memptr = (memtbl_t **) getaddr(argv[i])) == NULL) {
				altfprintf(2, "Cannot find %s\n", argv[i]);
				XK_USAGE(argv[0]);
			}
		}
	}
	for (i = 0; i < Nstructlist; i++)
		if ((Structlist[i].mem == memptr) && (!prefix || (strcmp(Structlist[i].prefix, prefix) == 0)) && (!id || (Structlist[i].id == id)))
			return(SH_SUCC);
	add_structlist(memptr, prefix, id);
}

static
chg_structlist(memptr, id)
struct memtbl **memptr;
int id;
{
	int i;

	for (i = 0; i < Nstructlist; i++)
		if (Structlist[i].id == id) {
			Structlist[i].mem = memptr;
			return;
		}
}

add_structlist(memptr, prefix, id)
struct memtbl **memptr;
char *prefix;
int id;
{
	int i;

	if (!Structlist)
		Structlist = (struct structlist *) malloc((Nstructlist + 1) * sizeof(struct structlist));
	else
		Structlist = (struct structlist *) realloc(Structlist, (Nstructlist + 1) * sizeof(struct structlist));
	if (!Structlist)
		return(SH_FAIL);
	Structlist[Nstructlist].mem = memptr;
	Structlist[Nstructlist].id = id;
	Structlist[Nstructlist].prefix = strdup(prefix);
	for (i = 1; memptr[i] && memptr[i][0].name && memptr[i][0].name[0]; i++)
		if (strcmp(memptr[i][0].name, memptr[i - 1][0].name) < 0)
			break;
	if (!(memptr[i] && memptr[i][0].name && memptr[i][0].name[0]))
		Structlist[Nstructlist].size = i - 1;
	else
		Structlist[Nstructlist].size = -1;
	Nstructlist++;
	return(SH_SUCC);
}

do_deflist(argc, argv)
int argc;
char **argv;
{
	int i, j;
	char *prefix = NULL;
	struct symarray *defptr;

	for (i = 1; argv[i]; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j]; j++) {
				switch(argv[i][j]) {
				case 'p':
					if (argv[i][j + 1]) {
						prefix = argv[i] + j;
						j += strlen(prefix) - 2;
					}
					else {
						prefix = argv[++i];
						j = strlen(prefix) - 1;
					}
				}
			}
		}
		else {
			if ((defptr = (struct symarray *) getaddr(argv[i])) == NULL) {
				altfprintf(2, "Cannot find %s\n", argv[i]);
				XK_USAGE(argv[0]);
			}
		}
	}
	for (i = 0; i < Ndeflist; i++)
		if ((Deflist[i].defs == defptr) && (!prefix || (strcmp(Deflist[i].prefix, prefix) == 0)))
			return(SH_SUCC);
	return(add_deflist(defptr, prefix));
}

static
add_deflist(defptr, prefix)
struct symarray *defptr;
char *prefix;
{
	int i;

	if (!Deflist)
		Deflist = (struct deflist *) malloc((Ndeflist + 1) * sizeof(struct deflist));
	else
		Deflist = (struct deflist *) realloc(Deflist, (Ndeflist + 1) * sizeof(struct deflist));
	if (!Deflist)
		return(SH_FAIL);
	Deflist[Ndeflist].defs = defptr;
	Deflist[Ndeflist].prefix = strdup(prefix);
	for (i = 1; defptr[i].str && defptr[i].str[0]; i++)
		if (symcomp((VOID *) (defptr + i), (VOID *) (defptr + i - 1)) < 0)
			break;
	if (!(defptr[i].str && defptr[i].str[0]))
		Deflist[Ndeflist].size = i;
	else
		Deflist[Ndeflist].size = -1;
	
	Ndeflist++;
	return(SH_SUCC);
}

int Prfailpoint = 0;

strparse(tbl, pbuf, val)
memtbl_t *tbl;
char **pbuf, *val;
{
	char *p, *phold;
	int ret;

	if (!IS_SIMPLE(tbl) && !tbl->ptr && !(tbl->flags & F_TYPE_IS_PTR))
		tbl->ptr = 1;
	phold = p = strdup(val);
	ret = xk_parse(tbl, &p, pbuf, 0, 0, NULL, all_tbl_find);
	if ((ret == FAIL) && Prfailpoint)
		altprintf((const char *) "WARNING: extra after parse: '%s'\n", p);
	free(phold);
	return(ret != FAIL);
}

ulong
strprint(va_alist)
va_dcl
{
	va_list ap;
	char *arg;
	char *variable = NULL;
	memtbl_t tbl;
	char *p;
	char buf[5 * BUFSIZ];
	char *name;
	VOID *val;
	char always_ptr;
	int nonames = 0;
	int ret;

	va_start(ap);
	always_ptr = 0;
	while ((arg = (char *) va_arg(ap, ulong)) && (arg[0] == '-')) {
		int i;

		for (i = 1; arg[i]; i++) {
			switch (arg[i]) {
			case 'v':
				variable = va_arg(ap, char *);
				i = strlen(arg) - 1;
				break;
			case 'p':
				always_ptr = 1;
				break;
			case 'N':
				nonames = 1;
			}
		}
	}
	name = arg;
	if (!arg) {
		altprintf((const char *) "Insufficient arguments to strprint\n");
		va_end(ap);
		return(SH_FAIL);
	}
	val = (VOID *) va_arg(ap, ulong);
	va_end(ap);
	if (parse_decl(&tbl, name, 1) == FAIL)
		return(SH_FAIL);
	/*if (tbl.name == STR_string_t) {*/
		/*ALTPUTS(val);*/
		/*return(SH_SUCC);*/
	/*}*/
	if (variable)
		p = buf + lsprintf(buf, "%s=", variable);
	else
		p = buf;
	if ((always_ptr || !IS_SIMPLE(&tbl)) && !tbl.ptr && !(tbl.flags & F_TYPE_IS_PTR))
		tbl.ptr = 1;
	if (!val && (tbl.ptr || (tbl.flags & F_TYPE_IS_PTR))) {
		altprintf((const char *) "NULL value argument to strprint\n");
		return(SH_FAIL);
	}
	if (always_ptr && (tbl.flags & F_TYPE_IS_PTR))
		val = *((VOID **) val);
	else while (tbl.ptr > 1) {
		val = *((VOID **) val);
		tbl.ptr--;
	}
	Pr_tmpnonames = nonames;
	ret = xk_print(&tbl, &p, (VOID *) &val, 0, 0, NULL, all_tbl_find);
	Pr_tmpnonames = 0;
	if (ret == FAIL)
		return(SH_FAIL);
	if (variable)
		env_set(buf);
	else
		ALTPUTS(buf);
	return(SH_SUCC);
}

strfree(buf, type)
char *buf;
char *type;
{
	memtbl_t tbl;

	if (parse_decl(&tbl, type, 1) == FAIL)
		return(SH_FAIL);
	if (!IS_SIMPLE(&tbl) && !tbl.ptr && !(tbl.flags & F_TYPE_IS_PTR))
		tbl.ptr = 1;
	if (xk_free(&tbl, &buf, 0, 0, all_tbl_find) == FAIL)
		return(SH_FAIL);
	return(SH_SUCC);
}

static
allprint(pargs, tbls)
ulong *pargs;
memtbl_t *tbls;
{
	char buf[10 * BUFSIZ], *p;
	int i;

	for (i = 0; tbls[i].name; i++) {
		altprintf((const char *) "Argument %d (type %s):\n\t", i + 1, tbls[i].name);
		p = buf;
		/*if (tbls[i].name == STR_string_t)*/
			/*ALTPUTS(pargs[i]);*/
		/*else {*/
			xk_print(tbls + i, &p, pargs + i, 0, 0, NULL, all_tbl_find);
			ALTPUTS(buf);
		/*}*/
	}
}

static
pp_usage()
{
	altprintf((const char *) "Please enter p(rint), s(end) or field=val\n");
}

static
call_postprompt(pargs, tbls, freeit)
ulong *pargs;
memtbl_t *tbls;
int *freeit;
{
	char buf[BUFSIZ];

	for ( ; ; ) {
		myprompt((const char *) "Postprompt: ");
		buf[0] = 'q';
		buf[1] = '\0';
		if (!altgets(buf) || (xk_Strncmp(buf, (const char *) "q", 2) == 0)) {
			altprintf((const char *) "Warning: command will not be executed\n");
			return(0);
		}
		else if (xk_Strncmp(buf, (const char *) "p", 2) == 0)
			allprint(pargs, tbls);
		else if (xk_Strncmp(buf, (const char *) "s", 2) == 0)
			return(1);
		else if (!strchr(buf, '=') || (asl_set(tbls, buf, pargs) == SH_FAIL))
			pp_usage();
	}
}

#define ZERORET		0
#define NONZERO		1
#define NONNEGATIVE	2

/* In shell, 0 is success so, ZERORET means direct return, NONZERO means
** return the opposite of its truth value and NONNEGATIVE means return
** true if the value IS negative (since FALSE is success)
*/
#define CALL_RETURN(RET) return(SET_RET(RET), ((ret_type == ZERORET) ? (RET) : ((ret_type == NONZERO) ? !(RET) : ((RET) < 0))))
#define EARLY_RETURN(RET) return(SET_RET(RET))
#define SET_RET(RET) (((int) sprintf(xk_ret_buffer, use, (RET))), (int) (xk_ret_buf = xk_ret_buffer), RET)

do_call(argc, argv)
int argc;
char **argv;
{
	VOID *pargs[10];
	memtbl_t tblarray[10];
	char freeit[10];
	ulong (*func)();
	char *p;
	char dorun, promptflag;
	unsigned char freeval, ret_type;
	register int i, j, ret;

	promptflag = 0;
	freeval = 1;
	ret_type = ZERORET;
	dorun = 1;
	if (!argv[1]) {
		altprintf((const char *) "No function to call\n");
		EARLY_RETURN(1);
	}
	for (j = 1; argv[j][0] == '-'; j++) {
		for (i = 1; argv[j][i]; i++) {
			switch(argv[j][i]) {
			case 'F':
				/* Do not free */
				freeval = 0;
				break;
			case 'r':
				/* reverse sense of return value */
				ret_type = NONZERO;
				break;
			case 'n':
				/* Non-negative return value is okay */
				ret_type = NONNEGATIVE;
				break;
			default:
				altprintf((const char *) "Unrecognized flag %c\n", argv[j][1]);
				EARLY_RETURN(1);
			}
		}
	}
	if (!argv[j]) {
		altprintf((const char *) "No function to call\n");
		CALL_RETURN(1);
	}
	memset(tblarray, '\0', 10 * sizeof(memtbl_t));
	memset(pargs, '\0', 10 * sizeof(VOID *));
	memset(freeit, '\0', 10 * sizeof(char));
	func = (ulong (*)()) fsym(argv[j], -1);
	if (!func && ((argv[j][0] != '0') || (UPP(argv[j][1]) != 'X') || !(func = (ulong (*)()) strtoul(argv[j], &p, 16)) || *p)) {
		altprintf((const char *) "No function to call %s\n", argv[j]);
		CALL_RETURN(1);
	}
	j++;
	for (i = 0; (i < 10) && argv[j]; j++, i++) {
		char *val;
		char type[100];

		if (C_PAIR(argv[j], '+', '?')) {
			promptflag = 1;
			continue;
		}
		else if (C_PAIR(argv[j], '+', '+')) {
			j++;
			break;
		}
		if (argv[j][0] == '@') {
			if (!(val = strchr(argv[j] + 1, ':'))) {
				dorun = 0;
				ret = -1;
				break;
			}
			strncpy(type, argv[j] + 1, val - argv[j] - 1);
			type[val - argv[j] - 1] = '\0';
			val++;
			/*
			**  This is no longer necessary, the integers will be
			**  parsed by the usual parser, and the pointers must
			**  have the p in front.
			**
			**  if (isdigit(val[0])) {
			**  	xk_par_int(&val, pargs + i, NULL);
			**  	parse_decl(tblarray + i, type, 0);
			**  }
			else
			*/
			if (parse_decl(tblarray + i, type, 1) == FAIL) {
				dorun = 0;
				ret = -1;
				break;
			}
			else {
				if (!strparse(tblarray + i, pargs + i, val)) {
					altprintf((const char *) "Cannot figure out %s for %s\n", val, type);
					dorun = 0;
					ret = -1;
					break;
				}
				else
					freeit[i] = freeval;
			}
		}
		else if (isdigit(argv[j][0])) {
			char *p;

			p = argv[j];
			tblarray[i] = T_ulong[0];
			xk_par_int(&p, pargs + i, NULL);
		}
		else if (strcmp(argv[j], (const char *) "NULL") == 0) {
			tblarray[i] = T_ulong[0];
			pargs[i] = NULL;
		}
		else {
			pargs[i] = (VOID *) argv[j];
			tblarray[i] = T_string_t[0];
		}
	}
	/* Process special arguments */
	while (argv[j]) {
		asl_set(tblarray, argv[j], pargs);
		j++;
	}
	if (dorun) {
		extern int errno;

		if (!promptflag || call_postprompt(pargs, tblarray, freeit))
			ret = (*func)(pargs[0], pargs[1], pargs[2], pargs[3], pargs[4], pargs[5], pargs[6], pargs[7], pargs[8], pargs[9], pargs[10]);
		else
			ret = 0;
		Xk_errno = errno;
	}
	for (i = 0; i < 10; i++) {
		if (pargs[i] && freeit[i])
			/* There is no recourse for failure */
			xk_free(tblarray + i, pargs + i, 0, 0, all_tbl_find);
	}
	CALL_RETURN(ret);
}

int _Prdebug;

static long
get_prdebug()
{
	return(_Prdebug);
}

static long
set_prdebug(n)
long n;
{
	_Prdebug = n;
}


do_sizeof(argc, argv)
int argc;
char **argv;
{
	memtbl_t *tbl;
	int i;

	i = 1;
	if (!argv[i]) {
		altprintf((const char *) "Insufficient arguments to %s\n", argv[0]);
		XK_USAGE(argv[0]);
	}
	if ((tbl = all_tbl_search(argv[i], 0)) == NULL) {
		altprintf((const char *) "Cannot find %s\n", argv[i]);
		XK_USAGE(argv[0]);
	}
	if (argv[++i]) {
		char buf[50];

		sprintf(buf, use2, argv[i], tbl->ptr ? sizeof(VOID *) : tbl->size);
		env_set(buf);
	}
	else {
		sprintf(xk_ret_buffer, use, tbl->ptr ? sizeof(VOID *) : tbl->size);
		xk_ret_buf = xk_ret_buffer;
	}
	return(SH_SUCC);
}

do_deref(argc, argv)
int argc;
char **argv;
{
	unchar *ptr;
	long i, len = 0;
	short longwise = -1;
	char printit = 0;


	for (i = 1; argv[i][0] == '-'; i++) {
		if (isdigit(argv[i][1])) {
			if (longwise < 0)
				longwise = 0;
			ptr = (unchar *) argv[i] + 1;
			xk_par_int(&ptr, &len, NULL);
			if (!len) {
				altprintf((const char *) "Invalid length specifier: %s\n", argv[i]);
				XK_USAGE(argv[0]);
			}
		}
		else if (argv[i][1] == 'l')
			longwise = 1;
		else if (argv[i][1] == 'p')
			printit = 1;
	}
	if (longwise < 0)
		longwise = 1;
	if (!len)
		len = sizeof(long);
	if (!argv[i]) {
		altprintf((const char *) "Insufficient arguments to deref\n");
		XK_USAGE(argv[0]);
	}	
	ptr = (unchar *) getaddr(argv[i++]);
	if (ptr) {
		if (argv[i] || printit) {
			char *dbuf, *p;
			int totlen;
			char buf[10 * BUFSIZ];
			int incr;

			if (printit)
				totlen = len + 1 + 1;
			else
				totlen = len + strlen(argv[i]) + 1 + 1;
			dbuf = (char *) (totlen < (10 * BUFSIZ - 1)) ? buf : malloc(totlen);
			if (printit)
				strcpy(dbuf, "0x");
			else
				sprintf(dbuf, "%s=0x", argv[i]);
			p = dbuf + strlen(dbuf);
			incr = longwise ? sizeof(long) : sizeof(char);
			for (i=0; i < len; i += incr, p += 2 * incr)
				sprintf(p, "%*.*x", incr * 2, incr * 2, longwise ? *((ulong *) (ptr + i)) : (unsigned long) (ptr[i]));
			if (printit)
				ALTPUTS(dbuf);
			else
				env_set(dbuf);
			if (dbuf != buf)
				free(dbuf);
		}
		else {
			if (len > sizeof(ulong)) {
				altprintf((const char *) "The length must be less than %d to set RET\n", sizeof(ulong));
				XK_USAGE(argv[0]);
			}
			sprintf(xk_ret_buffer, use, *((ulong *) ptr));
			xk_ret_buf = xk_ret_buffer;
		}
		return(SH_SUCC);
	}
	altfprintf(2, "Cannot find %s\n", argv[--i]);
	XK_USAGE(argv[0]);
}

do_finddef(argc, argv)
int argc;
char **argv;
{
	ulong found;
	struct symarray dummy;

	if (!argv[1]) {
		altprintf((const char *) "Must give argument to finddef\n");
		XK_USAGE(argv[0]);
	}
	if (fdef(argv[1], &found)) {
		if (argv[2]) {
			char buf[50];

			sprintf(buf, use2, argv[2], found);
			env_set(buf);
		}
		else {
			sprintf(xk_ret_buffer, use, found);
			xk_ret_buf = xk_ret_buffer;
		}
		return(SH_SUCC);
	}
	altfprintf(2, (const char *) "Cannot find %s\n", argv[1]);
	XK_USAGE(argv[0]);
}

VOID *
nop(var)
VOID *var;
{
	return(var);
}

VOID *
save_alloc(var)
VOID *var;
{
	return(var);
}

asl_set(tblarray, desc, pargs)
memtbl_t *tblarray;
char *desc;
unchar **pargs;
{
	char *ptr;
	char *val;
	memtbl_t *tbl;
	memtbl_t usetbl[2];
	char op;
	char field[80], *fldp = field;
	unsigned long intval, i, newval;
	unsigned long top, bottom;

	if ((val = strchr(desc, '=')) == NULL)
		return(SH_FAIL);
	if (ispunct(val[-1]) && (val[-1] != ']')) {
		op = val[-1];
		strncpy(field, desc, val - desc - 1);
		field[val - desc - 1] = '\0';
		val++;
	}
	else {
		op = '\0';
		strncpy(field, desc, val - desc);
		field[val - desc] = '\0';
		val++;
	}
	if (isdigit(fldp[0])) {
		top = bottom = strtoul(fldp, &fldp, 0) - 1;
		if (*fldp == '.')
			fldp++;
	}
	else {
		top = 9;
		bottom = 0;
	}
	usetbl[1] = Null_tbl;
	for (i = bottom; i <= top; i++) {
		usetbl[0] = tblarray[i];
		ptr = (char *) (pargs + i);
		if (tbl = ffind(usetbl, fldp, &ptr))
			break;
	}
	if (!tbl || (i > top)) {
		altprintf((const char *) "Cannot find %s\n", fldp);
		return(SH_FAIL);
	}
	if (!op || !(tbl->flags & F_SIMPLE)) {
		if (xk_parse(tbl, &val, ptr, 0, 0, NULL, all_tbl_find) < 0)
			altprintf((const char *) "Cannot set value: %s\n", val);
	}
	else {
		xk_par_int(&val, &newval, NULL);
		switch (tbl->size) {
		case sizeof(long):
			intval = ((unsigned long *) ptr)[0];
			break;
		case sizeof(short):
			intval = ((unsigned short *) ptr)[0];
			break;
		case sizeof(char):
			intval = ((unsigned char *) ptr)[0];
			break;
		}
		switch(op) {
		case '+':
			intval += newval;
			break;
		case '-':
			intval -= newval;
			break;
		case '*':
			intval *= newval;
			break;
		case '/':
			intval /= newval;
			break;
		case '%':
			intval %= newval;
			break;
		case '&':
			intval &= newval;
			break;
		case '|':
			intval |= newval;
			break;
		case '^':
			intval ^= newval;
			break;
		}
		switch (tbl->size) {
		case sizeof(long):
			((unsigned long *) ptr)[0] = intval;
			break;
		case sizeof(short):
			((unsigned short *) ptr)[0] = intval;
			break;
		case sizeof(char):
			((unsigned char *) ptr)[0] = intval;
			break;
		}
	}
	return(SH_SUCC);
}

do_field_comp(argc, argv)
int argc;
char **argv;
{
	char *val, *type;
	VOID *ptr, *ptr2, *nuptr;
	memtbl_t tbl[2], *tbl2;
	unsigned int i;
	char pr1[5 * BUFSIZ], pr2[5 * BUFSIZ], *p1, *p2;

	i = 1;
	type = argv[i++];
	ptr = getaddr(argv[i++]);
	tbl[1] = Null_tbl;
	if (!type || !ptr || (parse_decl(tbl, type, 1) == FAIL))
		XK_USAGE(argv[0]);
	if (!IS_SIMPLE(tbl) && !tbl->ptr && !(tbl->flags & F_TYPE_IS_PTR))
		tbl->ptr = 1;
	else while (tbl->ptr > 1) {
		ptr = *((VOID **) ptr);
		tbl->ptr--;
	}
	for ( ; argv[i]; i++) {
		tbl2 = tbl;
		ptr2 = ptr;
		if (val = strchr(argv[i], '=')) {
			*val++ = '\0';
			tbl2 = ffind(tbl, argv[i], &ptr2);
			if (!tbl2) {
				altprintf((const char *) "Cannot find %s\n", argv[i]);
				XK_USAGE(argv[0]);
			}
			val[-1] = '=';
		}
		else
			val = argv[i];
		p1 = pr1;
		p2 = pr2;
		Pr_tmpnonames = 1;
		xk_print(tbl2, &p1, ptr2, 0, 0, NULL, all_tbl_find);
		if (xk_parse(tbl2, &val, &nuptr, 0, 0, NULL, all_tbl_find) < 0) {
			altprintf((const char *) "Failed to parse: ");
			ALTPUTS(argv[i]);
			XK_USAGE(argv[0]);
		}
		xk_print(tbl2, &p2, &nuptr, 0, 0, NULL, all_tbl_find);
		xk_free(tbl2, &nuptr, 0, 0, all_tbl_find);
		Pr_tmpnonames = 0;
		if (strcmp(pr1, pr2)) {
			if (env_get((const char *) "PRCOMPARE"))
				altprintf((const char *) "Comparision failed: field %s\nActual:  %s\nCompare: %s\n", argv[i], pr1, pr2);
			return(SH_FAIL);
		}
	}
	return(SH_SUCC);
}

call_init()
{
	extern struct memtbl *basemems[];
	extern struct symarray basedefs[];

	Hashnams = (VOID *) gettree(50);
	if (!(Dynmem = (struct memtbl **) malloc(20 * sizeof(struct memtbl *)))) {
		altfputs(2, "Insufficient memory\n");
		exit(1);
	}
	Dynmem[0] = NULL;
	Sdynmem = 20;
	Ndynmem = 0;
	if (!(Dyndef = (struct symarray *) malloc(20 * sizeof(struct symarray)))) {
		altfputs(2, "Insufficient memory\n");
		exit(1);
	}
	Dyndef[0].str = NULL;
	Sdyndef = 20;
	Ndyndef = 0;
	add_deflist(Dyndef, "dynamic");
	add_deflist(basedefs, "base");
	add_structlist(basemems, "base", BASE_ID);
	add_structlist(Dynmem, "dynamic", DYNMEM_ID);
}

memtbl_t *
all_tbl_find(name, tbl, id)
char *name;
int tbl;
long id;
{
	int i;

	if (tbl != -1) {
		for (i = 0; i < Nstructlist; i++)
			if (id == Structlist[i].id)
				return(Structlist[i].mem[tbl]);
		return(NULL);
	}
	return(all_tbl_search(name, TYPEONLY));
}

memtbl_t *
all_tbl_search(name, flag)
char *name;
int flag;
{
	register int i;
	VOID *found;

	if (found = (VOID *) xkhash_find(Hashnams, name))
		return((memtbl_t *) found);
	else {
		register int j;
		register memtbl_t **subtbl;

		for (i = 0; i < Nstructlist; i++) {
			if (subtbl = Structlist[i].mem)
				for (j = 0; subtbl[j]; j++)
					if (!(subtbl[j]->flags & F_FIELD) && (strcmp(name, subtbl[j]->name) == 0) && ((subtbl[j]->kind != K_TYPEDEF) || (subtbl[j]->tbl != -1))) {
						if (!(flag & NOHASH))
							xkhash_add(Hashnams, name, subtbl[j]);
						return(subtbl[j]);
					}
		}
	}
	return(NULL);
}

memtbl_t *
asl_find(ptbl, tbls, fld, pptr)
memtbl_t *ptbl;
memtbl_t *tbls;
char *fld;
char **pptr;
{
	int i;
	memtbl_t *tbl;

	if (!Structlist)
		return(NULL);
	if (!pptr && (ptbl == tbls))
		return(NULL);
	for (i = 0; tbls[i].name; i++) {
		if ((xk_Strncmp(tbls[i].name, fld, strlen(fld)) == 0) && (strlen(fld) == strlen(tbls[i].name))) {
			if (pptr && ptbl && ((ptbl->kind == K_STRUCT) || (ptbl->kind == K_ANY)))
				*pptr += tbls[i].offset;
			return(tbls + i);
		}
	}
	for (i = 0; tbls[i].name; i++) {
		if ((tbls[i].kind == K_TYPEDEF) || (tbls[i].kind == K_STRUCT) || (tbls[i].kind == K_UNION) || (tbls[i].kind == K_ANY)) {
			char *hold;

			if (!pptr) {
				if ((tbl = asl_find(tbls + i, all_tbl_find(tbls[i].tname, tbls[i].tbl, tbls[i].id), fld, pptr)) != NULL)
					return(tbl);
				continue;
			}
			hold = *pptr;
			if (tbls[i].ptr) {
				int nptr;

				nptr = tbls[i].ptr;
				/* if you hit a NULL, stop the loop */
				do {
					*pptr = *((char **) *pptr);
				} while (*pptr && --nptr);
			}
			if (*pptr) {
				if (!tbls[i].ptr)
					*pptr += tbls[i].offset;
				if ((tbl = asl_find(tbls + i, all_tbl_find(tbls[i].tname, tbls[i].tbl, tbls[i].id), fld, pptr)) != NULL)
					return(tbl);
				*pptr = hold;
			}
		}
	}
	return(NULL);
}

static
myprompt(prompt)
char *prompt;
{
	p_flush();
	p_setout(2);
	p_str(prompt, 0);
}
