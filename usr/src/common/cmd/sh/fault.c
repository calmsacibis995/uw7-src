/*		copyright	"%c%" 	*/

#ident	"@(#)sh:common/cmd/sh/fault.c	1.13.29.8"
#ident  "$Header$"
/*
 * UNIX shell
 */
#include	<pfmt.h>
#include	<unistd.h>
#include	"defs.h"
#include	<sys/procset.h>

void done();
void stdsigs();
void oldsigs();
void chktrap();

extern int str2sig();

static void fault();
static BOOL sleeping = 0;
static unsigned char *trapcom[MAXTRAP];
static BOOL trapflg[MAXTRAP] =
{
	0,
	0,	/* hangup */
	0,	/* interrupt */
	0,	/* quit */
	0,	/* illegal instr */
	0,	/* trace trap */
	0,	/* IOT */
	0,	/* EMT */
	0,	/* float pt. exp */
	0,	/* kill */
	0, 	/* bus error */
	0,	/* memory faults */
	0,	/* bad sys call */
	0,	/* bad pipe call */
	0,	/* alarm */
	0, 	/* software termination */
	0,	/* unassigned */
	0,	/* unassigned */
	0,	/* death of child */
	0,	/* power fail */
	0,	/* window size change */
	0,	/* urgent IO condition */
	0,	/* pollable event occured */
	0,	/* stopped by signal */
	0,	/* stopped by user */
	0,	/* continued */
	0,	/* stopped by tty input */
	0,	/* stopped by tty output */
	0,	/* virtual timer expired */
	0,	/* profiling timer expired */
	0,	/* exceeded cpu limit */
	0,	/* exceeded file size limit */
};

static void (*(
sigval[]))() = 
{
	0,
	done, 	/* hangup */
	fault,	/* interrupt */
	fault,	/* quit */
	done,	/* illegal instr */
	done,	/* trace trap */
	done,	/* IOT */
	done,	/* EMT */
	done,	/* floating pt. exp */
	0,	/* kill */
	done, 	/* bus error */
	fault,	/* memory faults */
	done, 	/* bad sys call */
	done,	/* bad pipe call */
	done,	/* alarm */
	fault,	/* software termination */
	done,	/* unassigned */
	done,	/* unassigned */
	0,	/* death of child */
	done,	/* power fail */
	0,	/* window size change */
	done,	/* urgent IO condition */
	done,	/* pollable event occured */
	0,	/* uncatchable stop */
	0,	/* foreground stop */
	0,	/* stopped process continued */
	0,	/* background tty read */
	0,	/* background tty write */
	done,	/* virtual timer expired */
	done,	/* profiling timer expired */
	done,	/* exceeded cpu limit */
	done,	/* exceeded file size limit */
};

static int
ignoring(i)
register int i;
{
	struct sigaction act;
	if (trapflg[i] & SIGIGN)
		return 1;
	/*
	** O isn't a real signal
	*/
	if(i == 0)	{
		return 0;
	}
	(void)sigaction(i, 0, &act);
	if (act.sa_handler == SIG_IGN) {
		trapflg[i] |= SIGIGN;
		return 1;
	}
	return 0;
}

static void
clrsig(i)
int	i;
{
	if (trapcom[i] != 0) {
		free(trapcom[i]);
		trapcom[i] = 0;
	}
	if (trapflg[i] & SIGMOD) {
		trapflg[i] &= ~SIGMOD;
		handle(i, sigval[i]);
	}
}

void
done(sig)
{
	register unsigned char	*t = trapcom[0];

	if (t)
	{
		trapcom[0] = 0;
		execexp(t, 0);
		free(t);
	}
	else
		chktrap();

	rmtemp(0);
	rmfunctmp();

#ifdef ACCT
	doacct();
#endif
	(void)endjobs(0);
	if (sig) {
		sigset_t set;
		(void)sigemptyset(&set);
		(void)sigaddset(&set, sig);
		(void)sigprocmask(SIG_UNBLOCK, &set, 0);
		handle(sig, SIG_DFL);
		kill(mypid, sig);
	}
	exit(exitval);
}

static void 
fault(sig)
register int	sig;
{
	register int flag;
	
	switch (sig) {
		case SIGSEGV:
			if (setbrk(brkincr) == (unsigned char *)-1)
				error(0, nospace, nospaceid);
			return;
		case SIGALRM:
			if (sleeping)
				return;
			break;
	}

	if (trapcom[sig])
		flag = TRAPSET;
	else
		flag = SIGSET;
	/* ignore SIGTERM, SIGQUIT, and SIGINT when "-i" option is used. */
	if ( (flags&intflg)!=intflg || (sig!=SIGTERM && sig!=SIGQUIT && sig!=SIGINT))
		trapnote |= flag;
	trapflg[sig] |= flag;
}

int
handle(sig, func)
	int sig; 
	void (*func)();
{
	struct sigaction act, oact;
	if (func == SIG_IGN && (trapflg[sig] & SIGIGN))
		return 0;
	(void)sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = func;
	(void)sigaction(sig, &act, &oact);
	if (func == SIG_IGN)
		trapflg[sig] |= SIGIGN;
	else
		trapflg[sig] &= ~SIGIGN;
	return (func != oact.sa_handler);
}

/*
 * Called from main() to set up standard signal dispositions.
 *
 * Most ignored or masked signals remain ignored or masked.
 * However, we must be able to catch SIGSEGV in order for
 * automatic memory allocation to work correctly -- see fault().
 */

void
stdsigs()
{
	register int	i;

	for (i = 1; i < MAXTRAP; i++) {
		if (sigval[i] == 0)
			continue;
		if (i != SIGSEGV && ignoring(i))
			continue;
		handle(i, sigval[i]);
	}

	sigrelse(SIGSEGV);
}

void
oldsigs()
{
	register int	i;
	register unsigned char	*t;

	i = MAXTRAP;
	while (i--)
	{
		t = trapcom[i];
		if (t == 0 || *t)
			clrsig(i);
		trapflg[i] = 0;
	}
	trapnote = 0;
}

/*
 * check for traps
 */

void
chktrap()
{
	register int	i = MAXTRAP;
	register unsigned char	*t;

	trapnote &= ~TRAPSET;
	while (--i)
	{
		if (trapflg[i] & TRAPSET)
		{
			trapflg[i] &= ~TRAPSET;
			t = trapcom[i];
			if (t)
			{
				int	savxit = exitval;
				execexp(t, 0);
				exitval = savxit;
				exitset();
			}
		}
	}
}

void
systrap(argc,argv)
int argc;
char **argv;
{
	int sig;

	if (argc == 1) {
		for (sig = 0; sig < MAXTRAP; sig++) {
			if (trapcom[sig]) {
				prn_buff(sig);
				prs_buff(gettxt(colonid, colon));
				prs_buff(trapcom[sig]);
				prc_buff(NL);
			}
		}
	} else {
		char *a1 = *(argv+1);
		char *cka1 = a1;
		BOOL noa1;

		while (digit(*cka1))
			cka1++;
		noa1=((a1!=cka1&&(*cka1==0)))||((a1==cka1)&&(str2sig(a1,&sig)==0));
		if (noa1 == 0)
			++argv;
		(void) str2sig(a1,&sig);
		while (*++argv) {
			if (str2sig(*argv,&sig) < 0 ||
			  sig >= MAXTRAP || sig < MINTRAP || 
			  sig == SIGSEGV || sig == SIGCHLD)
				error_fail(SYSTRAP, badtrap, badtrapid);
			else if (sig == SIGALRM)
				warning(SYSTRAP, badtrap, badtrapid);
			else if (noa1)
				clrsig(sig);
                        else if (*a1) {
				if (trapflg[sig] & SIGMOD || !ignoring(sig)) {
					handle(sig, fault);
					trapflg[sig] |= SIGMOD;
					replace(&trapcom[sig], a1);
				}
			} else if (handle(sig, SIG_IGN)) {
				trapflg[sig] |= SIGMOD;
				replace(&trapcom[sig], a1);
			}
		}
	}
}

void
mysleep(ticks)
int ticks;
{
	sigset_t set, oset;
	struct sigaction act, oact;

	/*
	 * add SIGALRM to mask
	 */

	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGALRM);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);

	/*
	 * catch SIGALRM
	 */

	(void)sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = fault;
	(void)sigaction(SIGALRM, &act, &oact);

	/*
	 * start alarm and wait for signal
	 */

	set = oset;
	(void)sigdelset(&set, SIGALRM);
	(void)alarm(ticks);
	sleeping = 1;
	(void)sigsuspend(&set);
	sleeping = 0;

	/*
	 * reset alarm, catcher and mask
	 */

	(void)alarm(0); 
	(void)sigaction(SIGALRM, &oact, NULL);
	(void)sigprocmask(SIG_SETMASK, &oset, 0);
}
