#ident	"@(#)OSRcmds:csh/sh.wait.c	1.1"
#pragma comment(exestr, "@(#) sh.wait.c 25.1 94/02/18 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1994 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/* #ident	"@(#)csh:sh.wait.c	1.2" */

/*
 *	@(#) sh.wait.c 1.1 88/03/29 csh:sh.wait.c
 */

/* Copyright (c) 1980 Regents of the University of California */
/*	MODIFICATION HISTORY
 *
 *	L000   scol!panosd 18 Mar 1991
 *	- make it not complain if HZ env variable is not set.
 *	  Neither sh nor ksh do.
 *	- hughd: and cut out the else, proceed to the printf,
 *	  that was the whole point of taking HZ from param.h
 *	L001	scol!gregw	17 Feb 1994
 *	- Renamed calloc to Calloc to avoid possible conflict with libc.
 */
#include "sh.h"
#include <sys/param.h>
#ifdef VMUNIX
#include <sys/vtimes.h>
struct	vtimes zvms;
#endif

/*
 * C Shell
 */

static struct tbuffer {
#ifdef V6
	int 	put, pst;
#else
	time_t	put, pst;
#endif
	time_t	cut, cst;
} times0;
time_t	time0;

long
secs(bef, aft)
	struct tbuffer *bef, *aft;
{

	return ((aft->cut - bef->cut + aft->cst - bef->cst));
}

settimes()
{

	time(&time0);
	times(&times0);
}

dotime(v, kp)
	register char **v;
	struct command *kp;
{
	struct tbuffer timeszer, timesdol, *tzp;
	time_t timezer, timedol;
#ifdef VMUNIX
	struct vtimes vm0, vm1, vmme;
#endif

	if (v[1] != 0) {
		time(&timezer), times(&timeszer);
#ifdef VMUNIX
		vtimes(0, &vm0);
#endif
		lshift(v, 1);
		if (func(kp) == 0) {
			timflg = 1;
			return (0);
		}
		tzp = &timeszer;
#ifdef VMUNIX
		vmme = zvms;
#endif
	} else {
		timezer = time0, tzp = &times0;
#ifdef VMUNIX
		vm0 = zvms;
		vtimes(&vmme, 0);
#endif
	}
	time(&timedol);
	times(&timesdol);
#ifdef VMUNIX
	vtimes(0, &vm1);
	vmsadd(&vm1, &vmme);
#else
	ptimes(timedol - timezer, tzp, &timesdol);
#endif
#ifdef VMUNIX
	pvtimes(&vm0, &vm1, timedol - timezer);
#endif
	return (1);
}

#ifdef VMUNIX
vmsadd(vp, wp)
	register struct vtimes *vp, *wp;
{

	vp->vm_utime += wp->vm_utime;
	vp->vm_stime += wp->vm_stime;
	vp->vm_nswap += wp->vm_nswap;
	vp->vm_idsrss += wp->vm_idsrss;
	vp->vm_ixrss += wp->vm_ixrss;
	if (vp->vm_maxrss < wp->vm_maxrss)
		vp->vm_maxrss = wp->vm_maxrss;
	vp->vm_majflt += wp->vm_majflt;
	vp->vm_minflt += wp->vm_minflt;
	vp->vm_inblk += wp->vm_inblk;
	vp->vm_oublk += wp->vm_oublk;
}
#endif

donice(v)
	register char **v;
{
	register char *cp;

	v++, cp = *v++;
	if (cp == 0) {
/*
		nice(20);
		nice(-10);
*/
		nice(4);
		return (1);
	}
	if (*v == 0 && any(cp[0], "+-")) {
/*
		nice(20);
		nice(-10);
*/
		nice(getn(cp));
		return (1);
	}
	return (0);
}

struct	tbuffer bef, aft;
#ifdef VMUNIX
struct	vtimes vms;
#endif
time_t	btim, atim;

pwait(i)
	register int i;
{
	register int p, e;
	char *name;
	int s;

	if (i == 0)
		return;
	time(&btim);
	do {
		times(&bef);
#ifndef VMUNIX
		p = wait(&s);
#else
		p = vwait(&s, &vms);
#endif
		if (p == -1)
			return;
		times(&aft);
		if (p == getn(value("child")))
			unsetv("child");
		time(&atim);
		e = s & TRIM;
		if (e > 0 && (e > 15 || mesg[e])) {
			if (p != i)
				printf("%d: ", p);
			if (name = cname(p))
				printf("%s: ", name);
			if (e <= 15)
				printf(mesg[e]);
			else
				printf("Sig %d", e);
			if (s & 0200)
				printf(" -- Core dumped");
			printf("\n");
		}
		if (e != 0 && i == p) {
			cdone(p);
			if (e == SIGINT && setintr && (!gointr || !eq(gointr, "-")))
				pintr();
			error(0);
		}
		if (i == p) {
			set("status", putn(e ? e | QUOTE : (s >> 8) & 0377));
			if (exiterr && !eq(value("status"), "0")) {
				if (e == 0) {
					if (name = cname(p))
						printf("%s: ", name);
					printf("Exit status %s\n", value("status"));
				}
				exitstat();
			}
			cdone(p);
			break;
		}
		cdone(p);
	} while (i != p);
	if (Hz <= 0 && (Hz = gethz()) <= 0)
		Hz = HZ;				/* L000 */
	if (timflg ||
	(!child && adrof("time") && secs(&bef, &aft) / Hz >= getn(value("time")))) {
		timflg = 0;
#ifndef VMUNIX
		ptimes(atim - btim, &bef, &aft);
#else
		pvtimes(&zvms, &vms, atim- btim);
#endif
	}
}

#ifndef VMUNIX
ptimes(sec, bef, aft)
	time_t sec;
	register struct tbuffer *bef, *aft;
{

	p60ths(aft->cut - bef->cut);
	printf("u ");
	p60ths(aft->cst - bef->cst);
	printf("s ");
	psecs(sec);
	if (Hz == 0 && (Hz = gethz()) == 0)
		Hz = HZ;				/* L000 */
	printf(" %d%%", (int) ((100 * secs(bef, aft)) / (Hz * (sec ? sec : 1))));
	putchar('\n');
}
#endif

#ifdef VMUNIX
pvtimes(v0, v1, sec)
	register struct vtimes *v0, *v1;
	time_t sec;
{
	register time_t t =
	    (v1->vm_utime-v0->vm_utime)+(v1->vm_stime-v0->vm_stime);
	register char *cp;
	register int i;
	register struct varent *vp = adrof("time");

	cp = "%Uu %Ss %E %P %X+%Dk %I+%Oio %Fpf+%Ww";
	if (vp && vp->vec[0] && vp->vec[1])
		cp = vp->vec[1];
	for (; *cp; cp++)
	if (*cp != '%')
		putchar(*cp);
	else if (cp[1]) switch(*++cp) {

	case 'U':
		p60ths(v1->vm_utime - v0->vm_utime);
		break;

	case 'S':
		p60ths(v1->vm_stime - v0->vm_stime);
		break;

	case 'E':
		psecs(sec);
		break;

	case 'P':
		printf("%d%%", (int) ((100 * t) / (60 * (sec ? sec : 1))));
		break;

	case 'W':
		i = v1->vm_nswap - v0->vm_nswap;
		printf("%d", i);
		break;

	case 'X':
		printf("%d", t == 0 ? 0 : (v1->vm_ixrss-v0->vm_ixrss)/(2*t));
		break;

	case 'D':
		printf("%d", t == 0 ? 0 : (v1->vm_idsrss-v0->vm_idsrss)/(2*t));
		break;

	case 'K':
		printf("%d", t == 0 ? 0 : ((v1->vm_ixrss+v1->vm_idsrss) -
		   (v0->vm_ixrss+v0->vm_idsrss))/(2*t));
		break;

	case 'M':
		printf("%d", v1->vm_maxrss/2);
		break;

	case 'F':
		printf("%d", v1->vm_majflt-v0->vm_majflt);
		break;

	case 'R':
		printf("%d", v1->vm_minflt-v0->vm_minflt);
		break;

	case 'I':
		printf("%d", v1->vm_inblk-v0->vm_inblk);
		break;

	case 'O':
		printf("%d", v1->vm_oublk-v0->vm_oublk);
		break;

	}
	putchar('\n');
}
#endif

void
endwait(int unused)
{

	signal(SIGINT, SIG_IGN);
	cleft();
	bferr("Interrupted");
}

await()
{
	if (setintr)
		signal(SIGINT, endwait);
	pwait(-1);
	if (setintr)
		signal(SIGINT, SIG_IGN);
}

struct	achild {
	int	pid;
	char	*cname;
	struct	achild *cnext;
} children;

char *
cname(pid)
	int pid;
{
	register struct achild *cp;

	for (cp = children.cnext; cp; cp = cp->cnext)
		if (cp->pid == pid)
			return (cp->cname);
	return (NOSTR);
}

cadd(pid, cname)
	int pid;
	char *cname;
{
								/* L001 */
	register struct achild *cp = (struct achild *) Calloc(1, sizeof (struct achild));

	cp->pid = pid;
	cp->cname = savestr(cname);
	cp->cnext = children.cnext;
	children.cnext = cp;
}

cdone(pid)
	int pid;
{
	register struct achild *cpp, *cp;

	cpp = &children;
	for (cp = cpp->cnext; cp; cp = cp->cnext) {
		if (cp->pid == pid) {
			xfree(cp->cname);
			cpp->cnext = cp->cnext;
			xfree(cp);
			return;
		}
		cpp = cp;
	}
}

cleft()
{

	register struct achild *cp;

	for (cp = children.cnext; cp; cp = cp->cnext)
		printf("%6d  %s\n", cp->pid, cp->cname);
}
