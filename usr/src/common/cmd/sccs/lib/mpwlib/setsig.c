#ident	"@(#)sccs:lib/mpwlib/setsig.c	6.6.1.3"
# include	"signal.h"
# include	"sys/types.h"
# include	"macros.h"
# include <pfmt.h>
# include <locale.h>
# include <sys/euc.h>
# include <limits.h>
# include <unistd.h>
# include <stdio.h>

#define ONSIG	16

/*
	General-purpose signal setting routine.
	All non-ignored, non-caught signals are caught.
	If a signal other than hangup, interrupt, or quit is caught,
	a "user-oriented" message is printed on file descriptor 2 with
	a number for help(I).
	If hangup, interrupt or quit is caught, that signal	
	is set to ignore.
	Termination is like that of "fatal",
	via "clean_up(sig)" (sig is the signal number)
	and "exit(userexit(1))".
 
	If the file "dump.core" exists in the current directory
	the function commits
	suicide to produce a core dump
	(after calling clean_up, but before calling userexit).
*/


static char	*Mesg[ONSIG]={
	0,
	0,	/* Hangup */
	0,	/* Interrupt */
	0,	/* Quit */
	"Illegal Instruction",
	"Trace/Breakpoint Trap",
	"Abort",
	"Emulation Trap",
	"Arithmetic Exception",
	"Killed",
	"Bus Error",
	"Segmentation Fault",
	"Bad system Call",
	"Broken Pipe",
	"Alarm Clock"
};
static int	Mesg_no[ONSIG]={
	0,0,0,0,8,9,10,11,12,13,14,15,16,17,18
};

static void	setsig1();

void
setsig()
{
	register int j;
	register void (*n)();

	for (j=1; j<ONSIG; j++)
		if (n=signal(j,setsig1))
			signal(j,n);
}


static char preface[]="SIGNAL: ";
static char endmsg[]=" (ut12)\n";

static void
setsig1(sig)
int sig;
{
	int userexit(), open(), write();
	unsigned int	strlen();
	void	abort(), clean_up(), exit();
	char	valuemsg[128];

	if (Mesg[sig]) {
		sprintf(valuemsg,"uxlibc:%d",Mesg_no[sig]);
		fprintf(stderr,"%s%s%s",gettxt("uxepu:251",preface),
					gettxt(valuemsg,Mesg[sig]),
					gettxt("uxepu:252",endmsg));
	}
	else
		signal(sig,SIG_IGN);
	clean_up(sig);
	if(open("dump.core",0) > 0) {
		signal(SIGIOT,0);
		abort();
	}
	exit(userexit(1));
}
