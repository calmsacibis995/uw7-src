#ident "@(#)exec.c,v 6.10 1995/02/21 12:38:58 prem Exp - STREAMware TCP/IP source"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Legent Corporation
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <stdio.h>
#include <strings.h>
#include "defs.h"
#include "proto.h"

int	noexit = 0;
int	globerr = 0;

void push_fn(char *);
void pop_fn(char *);
char *print_fn(void);

/*
 * xerr - report execution error & exit
 */
#ifdef __STDC__
xerr(struct finst *fi, ...)
#else
xerr(va_alist)
va_dcl
#endif
{
	va_list         args;
#ifndef __STDC__
	struct finst   *fi;
#endif
	struct cmd     *c;
	int             flags;
	char           *fmt;
	char            buf[256];

#ifdef __STDC__
	va_start(args,fi);
#else
	va_start(args);
	fi = va_arg(args, struct finst *);
#endif
	c = va_arg(args, struct cmd *);
	flags = va_arg(args, int);
	fmt = va_arg(args, char *);
	sprintf(buf, "Function \"%s\", command %d: ", fi->func->name, c->cmdno);
	vsprintf(&buf[strlen(buf)], fmt, args);
	va_end(args);
	if (noexit) {
		error(flags, buf);
		globerr = E_FATAL;
	} else
		error(flags | E_FATAL, buf);
}

/*
 * showval - print value on stderr
 */
showval(v)
	struct val     *v;
{
	extern int dounlink;

	switch (v->vtype) {
	case V_NONE:
		fprintf(stderr, " <NONE>");
		break;
	case V_STR:
		fprintf(stderr, " %s", v->u.sval);
		break;
	case V_FD:
		fprintf(stderr, " <FD %d>", v->u.val);
		break;
	case V_MUXID:
		if (dounlink) {
			fprintf(stderr, " <PUNLINK %d>", v->u.val);
		} else {
			fprintf(stderr, " <{P,}LINK %d>", v->u.val);
		}
		break;
	}
}

/*
 * makeargv - make argument vector
 */
struct val     *
makeargv(fi, c)
	struct finst   *fi;
	struct cmd     *c;
{
	struct val     *argv;
	struct arg     *a;
	int             i;

	if (c->nargs) {
		argv = (struct val *) xmalloc(c->nargs * sizeof(struct val));
		for (a = c->args, i = 0; i < c->nargs; a++, i++) {
			switch (a->type) {
			case A_VAR:
				argv[i] = fi->vars[a->u.var->index];
				break;
			case A_PARAM:
				if (a->u.param >= fi->nargs) {
					xerr(fi, c, 0, "$%d not defined",
					     a->u.param + 1);
				}
				argv[i] = fi->args[a->u.param];
				break;
			case A_STR:
				argv[i] = *a->u.strval;
				break;
			}
			if (verbose)
				showval(&argv[i]);
		}
		if (verbose)
			putc('\n', stderr);
		return argv;
	} else {
		if (verbose)
			putc('\n', stderr);
		return (struct val *) 0;
	}
}

/*
 * chkargs - check arguments to builtin function
 */
chkargs(fi, c, bf, argc, argv)
	struct finst   *fi;
	struct cmd     *c;
	struct bfunc   *bf;
	int             argc;
	struct val     *argv;
{
	int             i;
	int             atype;

	if (argc < bf->minargs || argc > bf->maxargs)
		xerr(fi, c, 0, "Incorrect argument count");
	for (i = 0; i < argc; i++, argv++) {
		if (argv->vtype != bf->argtypes[i])
			xerr(fi, c, 0, "Incorrect argument type, arg %d", i);
	}
}

/*
 * docmd - execute a command
 */
docmd(fi, c, rval)
	struct finst   *fi;
	struct cmd     *c;
	struct val     *rval;
{
	struct val     *argv;
	struct fntab   *f;
	struct val     *cval;
	struct val     *userfunc();
	extern struct val val_none;
	extern int Gflag;

	f = c->func;
	if (verbose)
		fprintf(stderr, "%s", f->name);
	if (Gflag) {
		printf("\t/* function %s */\n",f->name);
		push_fn(f->name);
		printf("\t__ksl_strcf_fn = \"%s\";\n", print_fn());
	}
	argv = makeargv(fi, c);
	switch (f->type) {
	case F_UNDEF:
		xerr(fi, c, 0, "Undefined function called");
	case F_BUILTIN:
		chkargs(fi, c, f->u.bfunc, c->nargs, argv);
		cval = (f->u.bfunc->func) (fi, c, c->nargs, argv);
		break;
	case F_USER:
		cval = userfunc(f->u.ufunc, c->nargs, argv);
		break;
	case F_RETURN:
		if (c->nargs != 1)
			xerr(fi, c, 0, "Incorrect argument count");
		*rval = argv[0];
		cval = &val_none;
		break;
	}
	if (verbose && cval->vtype != V_NONE) {
		fprintf(stderr, "%s returns", f->name);
		showval(cval);
		putc('\n', stderr);
	}
	if (c->rvar)
		fi->vars[c->rvar->index] = *cval;
	if (argv)
		free(argv);
	if (Gflag) {
	    pop_fn(f->name);
	}
}

/*
 * userfunc - execute a user function
 */
struct val     *
userfunc(f, argc, argv)
	struct func    *f;
	int             argc;
	struct val     *argv;
{
	struct finst   *fi;
	struct cmd     *c;
	static struct val rval;

	fi = ALLOC(struct finst);
	fi->func = f;
	fi->vars = (struct val *) xmalloc(f->nvars * sizeof(struct val));
	fi->nargs = argc;
	fi->args = argv;
	rval.vtype = V_NONE;
	for (c = f->cmdhead; c; c = c->next) {
		docmd(fi, c, &rval);
		if (globerr == E_FATAL) {
			globerr = 0;
			break;
		}
	}
	free(fi->vars);
	free(fi);
	return &rval;
}

#define MAXDEPTH 1024 /* max number of chars in function `stack' */
static char name[MAXDEPTH];

typedef struct fnstack {
    struct fnstack *next;
    struct fnstack *prev;
    char *name;
} fnstack_t;

fnstack_t *fn_head = NULL;
fnstack_t *fn_tail = NULL;

void
push_fn(char *name)
{
    fnstack_t *new;

    new = ALLOC(fnstack_t);
    new->next = fn_head;
    new->prev = NULL;
    new->name = strdup(name);
    if (fn_head)
	fn_head->prev = new;
    else
	fn_tail = new;
    fn_head = new;
}

void
pop_fn(char *name)
{
    fnstack_t *old;

    old = fn_head;
    fn_head = old->next;
    if (fn_head)
	fn_head->prev = NULL;
    free(old);
}

char *
print_fn()
{
    fnstack_t *l;

    name[0] = '\0';
    l = fn_tail;

    while (l) {
	strcat(name, l->name);
	l = l->prev;
	if (l)
	    strcat(name, "->");
    }
}
