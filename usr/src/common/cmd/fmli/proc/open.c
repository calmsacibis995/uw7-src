/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:proc/open.c	1.11.3.5"

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include "inc.types.h"		/* abs s14 */
#include "token.h"
#include "slk.h"
#include "actrec.h"
#include "proc.h"
#include "terror.h"
#include	"moremacros.h"
#include "wish.h"

/* dmd TCB
struct actrec *Proc_list = NULL;
*/
extern struct proc_rec PR_all[];

proc_open(flags, title, path, va_alist)
int flags;
char *title, *path;
va_dcl
{
	char *argv[MAX_ARGS+2];
	register int i;
	va_list list;

	va_start(list);
	for (i = 0; i < MAX_ARGS+1 && (argv[i] = va_arg(list, char *)); i++)
		;
	argv[MAX_ARGS+1] = NULL;

	return(proc_openv(flags, title, path, argv));
}

int
proc_opensys(flags, title, path, arg)
int flags;
char *title, *path;
char *arg;
{
	return(proc_open(flags, title, path,  "/bin/sh", "-c", arg, NULL));
}

int
proc_openv(flags, title, path, argv)
int flags;
char *title, *path;
char *argv[];
{
	struct actrec a, *rec;
	extern struct slk No_slks[];
	int proc_close(), proc_current(), proc_noncurrent(), proc_ctl();
	struct actrec *ar_create(), *path_to_ar();

	/* if no path is specified, consider all the arguments put together
	 * to be the path.
	 *
	 * the buf here used to be fixed length.  It is now malloc'd
	 * so there is no problem with overflow.  dmd s15
	 */

	if (path == NULL) {
		char *buf;
		register int i, len;

	/*
	 * first figure out how long the args total up to. we add
	 * one to allow for the tab char between each one.
	 * we add a one in the malloc to allow for the terminal NULL
	 * dmd s15
	 */
		for (i = len = 0; argv[i]; i++)
			len += (int)strlen(argv[i]) + 1;

		if ( (buf = malloc(len+1)) == NULL )
			fatal(NOMEM,nil);

		for (i = len = 0; argv[i]; i++)
		{
                        if (len + (int)strlen(argv[i]) >= BUFSIZ) {
                        	fprintf(stderr,"\nbuffer overflow\n");
                        	return(FAIL);
                        }
                        len += sprintf(buf + len,"%s\t",argv[i]);
                }
		a.path = buf;
	} else
		a.path = strsave(path);

	if ((rec = path_to_ar(a.path)) != NULL) {
		free(a.path);
		return(ar_current(rec, TRUE)); /* abs k15 */
	}

	a.odptr = title?strsave(title):NULL;

	a.fcntbl[AR_CLOSE] = proc_close;
	a.fcntbl[AR_REREAD] = AR_NOP;
	a.fcntbl[AR_REINIT] = AR_NOP;
	a.fcntbl[AR_CURRENT] = proc_current;
/*	a.fcntbl[AR_TEMP_CUR] = proc_current; /* abs k15 optimize later */
	a.fcntbl[AR_TEMP_CUR] = AR_NOP; /* miked */
	a.fcntbl[AR_NONCUR] = proc_noncurrent;
	a.fcntbl[AR_CTL] = proc_ctl;
	a.fcntbl[AR_HELP] = AR_NOHELP;
	a.fcntbl[AR_ODSH] = AR_NOP;
	a.id = proc_default(flags, argv);
	if (a.id == FAIL)
		return(FAIL);
	a.lifetime = AR_LONGTERM;
	a.flags = AR_SKIP;
	a.slks = No_slks;

/* dmd TCB
	if (Proc_list)
		(void) ar_close(Proc_list, FALSE);
*/

	PR_all[a.id].ar = ar_create(&a);

	return(ar_current(PR_all[a.id].ar, FALSE)?SUCCESS:FAIL); /* abs k15 */
}
