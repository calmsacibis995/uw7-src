#ident	"@(#)truss:i386/cmd/truss/syscall.c	1.1.3.1"
#ident	"$Header$"

#include <stdio.h>
#include <errno.h>

#include "pcontrol.h"
#include "ramdata.h"
#include "proto.h"
#include "machdep.h"

int
Pgetsysnum(P)		/* determine which syscall number we are at */
register process_t *P;
{
	register int syscall = -1;

	if (Pgetareg(P,R_0) == 0)
		syscall = P->REG[R_0] & 0xffff;
	return syscall;
}

int
Psetsysnum(P, syscall)	/* we are at a syscall trap, prepare to issue syscall */
register process_t *P;
register int syscall;
{
	P->REG[R_0] = syscall;
	if (Pputareg(P,R_0))
		syscall = -1;

	return syscall;
}

struct sysret		/* perform system call in controlled process */
Psyscall(P, sysindex, nargs, argp)
register process_t *P;	/* process control structure */
int sysindex;		/* system call index */
register int nargs;	/* number of arguments to system call */
struct argdes *argp;	/* argument descriptor array */
{
	register struct argdes *adp;	/* pointer to argument descriptor */
	struct sysret rval;		/* return value */
	register int i;			/* general index value */
	register int Perr = 0;		/* local error number */
	int sexit;			/* old value of stop-on-syscall-exit */
	ulong_t sp;			/* adjusted stack pointer */
	ulong_t ap;			/* adjusted argument pointer */
	gregset_t savedreg;		/* remembered registers */
	int arglist[MAXARGS+2];		/* syscall arglist */
	int why = P->why.pr_lwp.pr_why;	/* reason for stopping */
	int what = P->why.pr_lwp.pr_what; /* detailed reason (syscall, signal) */

	/* block (hold) all signals for the duration. */
	sigset_t block, unblock;

	(void) sigfillset(&block);
	(void) sigemptyset(&unblock);
	(void) sigprocmask(SIG_BLOCK, &block, &unblock);

	rval.rerrno = 0;		/* initialize return value */
	rval.r0 = 0;
	rval.r1 = 0;

	premptyset(&psigs);	/* no saved signals yet */

	if (sysindex <= 0 || sysindex > PRMAXSYS ||	/* programming error */
	    nargs < 0 || nargs > MAXARGS)
		goto bad1;

	if (P->state != PS_STOP ||		/* check state of process */
	    (P->why.pr_lwp.pr_flags & PR_ASLEEP) ||
	    Pgetregs(P) != 0)
		goto bad2;

	for (i = 0; i < NGREG; i++)		/* remember registers */
		savedreg[i] = P->REG[i];

	if (checksyscall(P))			/* bad text ? */
		goto bad3;


	/* validate arguments and compute the stack frame parameters --- */

	sp = savedreg[R_SP];	/* begin with the current stack pointer */
	for (i=0, adp=argp; i<nargs; i++, adp++) {	/* for each argument */
		rval.r0 = i;		/* in case of error */
		switch (adp->type) {
		default:			/* programming error */
			goto bad4;
		case AT_BYVAL:			/* simple argument */
			break;
		case AT_BYREF:			/* must allocate space */
			switch (adp->inout) {
			case AI_INPUT:
			case AI_OUTPUT:
			case AI_INOUT:
				if (adp->object == NULL)
					goto bad5;	/* programming error */
				break;
			default:		/* programming error */
				goto bad6;
			}
			/* allocate stack space for BYREF argument */
			if (adp->len <= 0 || adp->len > MAXARGL)
				goto bad7;	/* programming error */

			/* adjust sp down to word boundary */
			sp = (sp - adp->len) & ~(sizeof (int) - 1);
			adp->value = sp;	/* stack address for object */
			break;
		}
	}
	rval.r0 = 0;			/* in case of error */
	sp -= sizeof (int)*(nargs+2);	/* space for arg list + CALL parms */
	ap = sp;			/* address of arg list */


	/* point of no return */

	/* special treatment of stopped-on-syscall-entry */
	/* move the process to the stopped-on-syscall-exit state */
	if (why == PR_SYSENTRY) {
		/* arrange to reissue sys call */
		savedreg[R_PC] -= SYSCALL_OFF;

		sexit = Psysexit(P, what, TRUE);  /* catch this syscall exit */

		if (Psetrun(P, 0, PRSABORT) != 0 ||	/* abort sys call */
		    Pwait(P) != 0 ||
		    P->state != PS_STOP ||
		    P->why.pr_lwp.pr_why != PR_SYSEXIT ||
		    P->why.pr_lwp.pr_what != what ||
		    Pgetareg(P, R_PS) != 0 ||
		    Pgetareg(P, 0) != 0 ||
		    (P->REG[R_PS] & ERRBIT) == 0 ||
		    P->REG[R_0] != EINTR) {
			(void) fprintf(stderr,
				       "Psyscall(): cannot abort sys call\n");
			(void) Psysexit(P, what, sexit);
			goto bad9;
		}

		(void) Psysexit(P, what, sexit);/* restore previous exit trap */
	}


	/* perform the system call entry, adjusting %sp */
	/* this moves the process to the stopped-on-syscall-entry state */
	/* just before the arguments to the sys call are fetched */

	(void) Psetsysnum(P, sysindex);
	P->REG[R_SP] = sp;
	P->REG[R_PC] = P->sysaddr;	/* address of syscall */
	(void) Pputregs(P);

	if (execute(P, sysindex) != 0 || /* execute the syscall instruction */
	    P->REG[R_PC] != P->sysaddr+SYSCALL_OFF)
		goto bad10;


	/* stopped at syscall entry; copy arguments to stack frame */

	for (i=0, adp=argp; i<nargs; i++, adp++) {	/* for each argument */
		rval.r0 = i;		/* in case of error */
		if (adp->type != AT_BYVAL &&
		    adp->inout != AI_OUTPUT) {
			/* copy input byref parameter to process */
			if (Pwrite(P, (long)adp->value, adp->object, adp->len)
			    != adp->len)
				goto bad17;
		}
		arglist[i+1] = adp->value;
	}
	rval.r0 = 0;			/* in case of error */
	arglist[0] = savedreg[R_PC];	/* CALL parameters */
	arglist[nargs+1] = 0;
	if (Pwrite(P, ap, &arglist[0], sizeof (int)*(nargs+2)) !=
	    sizeof (int)*(nargs+2))
		goto bad18;


	/* complete the system call */
	/* this moves the process to the stopped-on-syscall-exit state */

	sexit = Psysexit(P, sysindex, TRUE);	/* catch this syscall exit */
	do {		/* allow process to receive signals in sys call */
		if (Psetrun(P, 0, 0) == -1)
			goto bad21;
		while (P->state == PS_RUN)
			(void) Pwait(P);
	} while (P->state == PS_STOP && P->why.pr_lwp.pr_why == PR_SIGNALLED);
	(void) Psysexit(P, sysindex, sexit);	/* restore original setting */

	if (P->state != PS_STOP ||
	    P->why.pr_lwp.pr_why != PR_SYSEXIT)
		goto bad22;
	if (P->why.pr_lwp.pr_what != sysindex)
		goto bad23;
	if (P->REG[R_PC] != P->sysaddr+SYSCALL_OFF)
		goto bad24;


	/* fetch output arguments back from process */

	if (Pread(P, ap, arglist, sizeof (int)*(nargs+2)) !=
	    sizeof (int)*(nargs+2))
		goto bad25;
	for (i=0, adp=argp; i<nargs; i++, adp++) {	/* for each argument */
		rval.r0 = i;		/* in case of error */
		if (adp->type != AT_BYVAL &&
		    adp->inout != AI_INPUT) {
			/* copy output byref parameter from process */
			if (Pread(P, adp->value, adp->object, adp->len)
			    != adp->len)
				goto bad26;
		}
		adp->value = arglist[i+1];
	}


	/* get the return values from the syscall */

	if (P->REG[R_PS] & ERRBIT) {	/* error */
		rval.rerrno = P->REG[R_0];
		rval.r0 = -1;
	} else {				/* normal return */
		rval.r0 = P->REG[R_0];
		rval.r1 = P->REG[R_1];
	}


	goto good;

bad26:	Perr++;
bad25:	Perr++;
bad24:	Perr++;
bad23:	Perr++;
bad22:	Perr++;
bad21:	Perr++;
	Perr++;
	Perr++;
bad18:	Perr++;
bad17:	Perr++;
	Perr++;
	Perr++;
	Perr++;
	Perr++;
	Perr++;
	Perr++;
bad10:	Perr++;
bad9:	Perr++;
	Perr += 8;
	rval.rerrno = -Perr;	/* local errors are negative */

good:
	/* restore process to its previous state (almost) */

	for (i = 0; i < NGREG; i++)	/* restore remembered registers */
		P->REG[i] = savedreg[i];
	(void) Pputregs(P);

	if (why == PR_SYSENTRY &&	/* special treatment */
	    execute(P, what) != 0) {	/* get back to the syscall */
		(void) fprintf(stderr,
			"Psyscall(): cannot reissue sys call\n");
		if (Perr == 0)
			rval.rerrno = -27;
	}

	P->why.pr_lwp.pr_why = why;
	P->why.pr_lwp.pr_what = what;

	goto out;

bad7:	Perr++;
bad6:	Perr++;
bad5:	Perr++;
bad4:	Perr++;
bad3:	Perr++;
bad2:	Perr++;
bad1:	Perr++;
	rval.rerrno = -Perr;	/* local errors are negative */

out:
	/* unblock (release) all signals before returning */
	(void) sigprocmask(SIG_SETMASK, &unblock, (sigset_t *)NULL);

	return rval;
}

