#ident	"@(#)vars.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident "@(#)vars.c	1.11 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Variable handling stuff.
 */

static struct var	*lookup ARGS((const char name[]));

/*
 * Assign a value to a variable.
 */
void
assign(varname, newvalue)
	char varname[];
	const char newvalue[];
{
	register struct var *vp;
	register int h;

	if (varname[0]=='-')
		deassign(varname+1);
	else if (varname[0]=='n' && varname[1]=='o')
		deassign(varname+2);
	else {
		/* Special case; "ask" must be mapped to "asksub" */
		if (strcmp(varname, "ask")==0)
			varname="asksub";
		h = hash(varname);
		vp = lookup(varname);
		if (vp == NOVAR) {
			vp = (struct var *) pcalloc(sizeof *vp);
			vp->v_name = vcopy(varname);
			vp->v_link = variables[h];
			variables[h] = vp;
		} else {
			if (strcmp(varname, "REMOTEHOST") == 0)
				goto noassign;
			vfree(vp->v_value);
		}
		vp->v_value = vcopy(newvalue);
		/*
		 * for efficiency, intercept certain assignments here
		 */
		if (strcmp(varname, "prompt")==0)
			prompt = vp->v_value;
		else if (strcmp(varname, "debug")==0)
			debug = 1;
noassign:
		if (debug) fprintf(stderr, "assign(%s)=%s\n", vp->v_name, vp->v_value);
	}
}

deassign(s)
register char *s;
{
	register struct var *vp, *vp2;
	register int h;

	/* Special case; "ask" must be mapped to "asksub" */
	if (strcmp(s, "ask")==0)
		s="asksub";

	if ((vp2 = lookup(s)) == NOVAR) {
		if (!sourcing) {
			pfmt(stdout, MM_ERROR, 
				":337:\"%s\": undefined variable\n", s);
			return(1);
		}
		return(0);
	}
	if (strcmp(s, "REMOTEHOST")==0)
		return(0);
	if (debug) fprintf(stderr, "deassign(%s)\n", s);
	if (strcmp(s, "prompt")==0)
		prompt = NOSTR;
	else if (strcmp(s, "debug")==0)
		debug = 0;
	h = hash(s);
	if (vp2 == variables[h]) {
		variables[h] = variables[h]->v_link;
		vfree(vp2->v_name);
		vfree(vp2->v_value);
		free((char*)vp2);
		return(0);
	}
	for (vp = variables[h]; vp->v_link != vp2; vp = vp->v_link)
		;
	vp->v_link = vp2->v_link;
	vfree(vp2->v_name);
	vfree(vp2->v_value);
	free((char*)vp2);
	return(0);
}

/*
 * Free up a variable string.  We do not bother to allocate
 * strings whose value is "" since they are expected to be frequent.
 * Thus, we cannot free same!
 */
void
vfree(cp)
	register char *cp;
{
	if (!equal(cp, ""))
		free(cp);
}

/*
 * Copy a variable value into permanent (ie, not collected after each
 * command) space.  Do not bother to alloc space for ""
 */

char *
vcopy(str)
	const char str[];
{
	register char *toptr, *cp;
	register const char *cp2;

	if (!str)
		return("");
	if (equal(str, ""))
		return("");
	toptr = pcalloc((unsigned)(strlen(str)+1));
	cp = toptr;
	cp2 = str;
	while (*cp++ = *cp2++)
		;
	return(toptr);
}

/*
 * Get the value of a variable and return it.
 * Look in the environment if its not available locally.
 */

char *
value(varname)
	const char varname[];
{
	register struct var *vp;
	register char *cp;

	if ((vp = lookup(varname)) == NOVAR)
		cp = getenv(varname);
	else
		cp = vp->v_value;
	if (debug) fprintf(stderr, "value(%s)=%s\n", varname, (cp)?cp:"");
	return(cp);
}

char *
value_noenv(varname)
	const char varname[];
{
	register struct var *vp;
	register char *cp;

	if ((vp = lookup(varname)) != NOVAR)
		cp = vp->v_value;
	else
		cp = (char *)0;
	if (debug) fprintf(stderr, "value(%s)=%s\n", varname, (cp)?cp:"");
	return(cp);
}

/*
 * Locate a variable and return its variable
 * node.
 */

static struct var *
lookup(varname)
	const char varname[];
{
	register struct var *vp;
	register int h;

	h = hash(varname);
	for (vp = variables[h]; vp != NOVAR; vp = vp->v_link)
		if (equal(vp->v_name, varname))
			return(vp);
	return(NOVAR);
}

/*
 * Locate a group name and return it.
 */

struct grouphead *
findgroup(grpname)
	char grpname[];
{
	register struct grouphead *gh;
	register int h;

	h = hash(grpname);
	for (gh = groups[h]; gh != NOGRP; gh = gh->g_link)
		if (equal(gh->g_name, grpname))
			return(gh);
	return(NOGRP);
}

/*
 * Print a group out on stdout
 */
void
printgroup(grpname)
	char grpname[];
{
	register struct grouphead *gh;
	register struct mgroup *gp;

	if ((gh = findgroup(grpname)) == NOGRP) {
		pfmt(stdout, MM_ERROR, ":339:\"%s\": not a group\n", grpname);
		return;
	}
	printf("%s\t", gh->g_name);
	for (gp = gh->g_list; gp != NOGE; gp = gp->ge_link)
		printf(" %s", gp->ge_name);
	printf("\n");
}

/*
 * Hash the passed string and return an index into
 * the variable or group hash table.
 */

hash(grpname)
	const char grpname[];
{
	register int h;
	register const char *cp;

	for (cp = grpname, h = 0; *cp; h = (h << 2) + *cp++)
		;
	if (h < 0)
		h = -h;
	if (h < 0)
		h = 0;
	return(h % HSHSIZE);
}
