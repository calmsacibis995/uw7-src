#ifndef	NOIDENT
#ident	"@(#)oldattlib:Xargs.c	1.2"
#endif
/*
 Xargs.c (C source file)
	Acc: 575322193 Fri Mar 25 14:43:13 1988
	Mod: 575321561 Fri Mar 25 14:32:41 1988
	Sta: 575321561 Fri Mar 25 14:32:41 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	author:
		Ross Hilbert
		AT&T 12/14/87
************************************************************************/

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include "Xargs.h"

typedef struct
{
	int	priority;
	char *	value;
}
	OptionValue;

static char * ExtractArg (name, argc, argv)
char *		name;
int *		argc;
char **		argv;
{
	int	n = *argc;
	int	i;
	char *	p;
	char *	arg = (char *) 0;

	for (i = 1, p = argv[1]; i < n; p = argv[++i])
	{
		if (!strcmp (name, p))
		{
			if (++i < n)
			{
				arg = argv[i];
				*argc -= 2;
			}
			break;
		}
	}
	for (++i; i < n; ++i)
		argv[i-2] = argv[i];
	return arg;
}

char * ExtractDisplay (argc, argv)
int *		argc;
char **		argv;
{
	return ExtractArg ("-display", argc, argv);
}

char * ExtractGeometry (argc, argv)
int *		argc;
char **		argv;
{
	return ExtractArg ("-geometry", argc, argv);
}

static char *	YES = "yes";

static void pass1 (dpy, opts, optv, argc, argv)
Display *	dpy;
Option *	opts;
OptionValue *	optv;
int *		argc;
char **		argv;
{
	while (opts->vptr)
	{
		if (opts->popt)
		{
			optv->value = opts->popt;
			optv->priority = 1;
		}
		else
		{
			optv->value = (char *) 0;
			optv->priority = 0;
		}
		++opts;
		++optv;
	}
}

static void pass2 (dpy, opts, optv, argc, argv)
Display *	dpy;
Option *	opts;
OptionValue *	optv;
int *		argc;
char **		argv;
{
	char * p;

	while (opts->vptr)
	{
		if (	opts->xopt
		&&	(p = XGetDefault (dpy, argv[0], opts->xopt)))
		{
			optv->value = p;
			optv->priority = 2;
		}
		++opts;
		++optv;
	}
}

#define MAX_BUF		128

static void pass3 (dpy, opts, optv, argc, argv)
Display *	dpy;
Option *	opts;
OptionValue *	optv;
int *		argc;
char **		argv;
{
	char * opt;
	int i, n, k, len;

	while (opts->vptr)
	{
		if (opts->copt)
		{
			opt = opts->copt;
			len = strlen (opt);

			if (opt[len-1] == ':')
			{
				k = 1;
				opt[len-1] = 0;
			}
			else
				k = 0;

			for (i = 1, n = *argc; i < n; ++i)
			{
				if (!strcmp (opt, argv[i]))
				{
					if (k && ++i < n)

						optv->value = argv[i];
					else
						optv->value = YES;

					optv->priority = 3;
					*argc -= (k+1);
					break;
				}
			}
			if (k)
				opt[len-1] = ':';

			for (++i; i < n; ++i)
				argv[i-(k+1)] = argv[i];
		}
		++opts;
		++optv;
	}
}

static void do_error (opt, priority, value)
Option *	opt;
int		priority;
char *		value;
{
	int len;

	fprintf (stderr, "bad value: ");

	switch (priority)
	{
		case 1:
			fprintf (stderr, "default program option");
			break;
		case 2:
			fprintf (stderr, ".Xdefaults option \"%s\"", opt->xopt);
			break;
		case 3:
			len = strlen (opt->copt);
			if (opt->copt[len-1] == ':')
				--len;
			fprintf (stderr, "command line option \"%.*s\"", len, opt->copt);
			break;
	}
	fprintf (stderr, ", value = \"%s\"\n", value);
}

#define ERROR	1
#define OK	0

int ExtractOptions (dpy, opts, argc, argv)
Display *	dpy;
Option *	opts;
int *		argc;
char **		argv;
{
#ifndef MEMUTIL
	extern char *	malloc ();
#endif /* MEMUTIL */

	Option *	x = opts;
	Option *	xo;
	OptionValue *	optv;
	OptionValue *	optp;
	OptionValue *	v;
	char *		vptr;
	char *		xv;
	int		xp;
	int		errcnt = 0;

	while (x->vptr)
		++x;

	optp = optv = (OptionValue*)malloc((x-opts)*sizeof(OptionValue));

	pass1 (dpy, opts, optv, argc, argv);
	pass2 (dpy, opts, optv, argc, argv);
	pass3 (dpy, opts, optv, argc, argv);

	while (opts->vptr)
	{
		if (optv->priority)
		{
			xp = 0;
			x = opts;
			v = optv;
			vptr = x->vptr;

			while (x->vptr)
			{
				if (x->vptr == vptr)
				{
					if (xp < v->priority)
					{
						xo = x;
						xv = v->value;
						xp = v->priority;
					}
					v->priority = 0;
				}
				++x;
				++v;
			}
			if ((*xo->f)(dpy, xo, xv) == ERROR)
			{
				do_error (xo, xp, xv);
				++errcnt;
			}
		}
		++opts;
		++optv;
	}
	free (optp);
	return errcnt;
}

static char ** match (v, x)
char * v;
char ** x;
{
	if (!v || !x)
		return (char **) 0;

	while (*x)
	{
		if (!strcmp (v, *x))
			break;
		++x;
	}
	if (*x)
		return x;
	return (char **) 0;
}

int OptString (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	char ** p = (char **) opt->vptr;
	*p = value;
	return OK;
}

static char * vtrue[]  = { "yes", "Yes", "on",  "On",  "true",  "True",  (char *)0 };
static char * vfalse[] = { "no",  "No",  "off", "Off", "false", "False", (char *)0 };

int OptBoolean (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	int * p = (int *) opt->vptr;

	if (match (value, vtrue))
	{
		*p = 1;
		return OK;
	}
	if (match (value, vfalse))
	{
		*p = 0;
		return OK;
	}
	return ERROR;
}

int OptInverse (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	int * p = (int *) opt->vptr;

	if (match (value, vtrue))
	{
		*p = 0;
		return OK;
	}
	if (match (value, vfalse))
	{
		*p = 1;
		return OK;
	}
	return ERROR;
}


int OptInt (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	extern int	atoi ();
	int *		p = (int *) opt->vptr;
	int		v = atoi (value);
	char *		range = opt->arg;

	if (range)
	{
		char * pmin = range;
		char * pmax = strchr (range, ':');

		if (!pmax)
			return ERROR;
		*pmax++ = 0;

		if (*pmin && (v < atoi (pmin)))
			return ERROR;
		if (*pmax && (v > atoi (pmax)))
			return ERROR;
		*--pmax = ':';
	}
	*p = v;
	return OK;
}

int OptLong (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	extern long	atol ();
	long *		p = (long *) opt->vptr;
	long		v = atol (value);
	char *		range = opt->arg;

	if (range)
	{
		char * pmin = range;
		char * pmax = strchr (range, ':');

		if (!pmax)
			return ERROR;
		*pmax++ = 0;

		if (*pmin && (v < atol (pmin)))
			return ERROR;
		if (*pmax && (v > atol (pmax)))
			return ERROR;
		*--pmax = ':';
	}
	*p = v;
	return OK;
}

int OptFloat (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	extern double	atof ();
	float *		p = (float *) opt->vptr;
	double		v = atof (value);
	char *		range = opt->arg;

	if (range)
	{
		char * pmin = range;
		char * pmax = strchr (range, ':');

		if (!pmax)
			return ERROR;
		*pmax++ = 0;

		if (*pmin && (v < atof (pmin)))
			return ERROR;
		if (*pmax && (v > atof (pmax)))
			return ERROR;
		*--pmax = ':';
	}
	*p = (float) v;
	return OK;
}

int OptDouble (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	extern double	atof ();
	double *	p = (double *) opt->vptr;
	double		v = atof (value);
	char *		range = opt->arg;

	if (range)
	{
		char * pmin = range;
		char * pmax = strchr (range, ':');

		if (!pmax)
			return ERROR;
		*pmax++ = 0;

		if (*pmin && (v < atof (pmin)))
			return ERROR;
		if (*pmax && (v > atof (pmax)))
			return ERROR;
		*--pmax = ':';
	}
	*p = v;
	return OK;
}

int OptColor (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	unsigned long *	p = (unsigned long *) opt->vptr;
	int		scr = DefaultScreen (dpy);
	Colormap	cmap = DefaultColormap (dpy, scr);
	XColor		color;

	if (DisplayCells (dpy, scr) > 2)
	{
		if (	XParseColor (dpy, cmap, value, &color)
		&&	XAllocColor (dpy, cmap, &color))
		{
			*p = color.pixel;
			return OK;
		}
		else
			return ERROR;
	}
	if (!strcmp (value, "white"))
		*p = WhitePixel (dpy, scr);
	else
		*p = BlackPixel (dpy, scr);
	return OK;
}

int OptFont (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	XFontStruct ** p = (XFontStruct **) opt->vptr;

	if (*p = XLoadQueryFont (dpy, value))
		return OK;
	return ERROR;
}

int OptFILE (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	FILE ** p = (FILE **) opt->vptr;
	char * type = opt->arg ? opt->arg : "r";

	if (*p = fopen (value, type))
		return OK;
	return ERROR;
}

int OptEnum (dpy, opt, value)
Display * dpy;
Option * opt;
char * value;
{
	int * p = (int *) opt->vptr;
	char ** x = (char **) opt->arg;
	char ** v = match (value, x);

	if (v)
	{
		*p = v-x;
		return OK;
	}
	return ERROR;
}
