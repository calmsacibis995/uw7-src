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
#ident	"@(#)fmli:proc/pcurrent.c	1.17.3.6"

#include <stdio.h>
#include <signal.h>
#include "inc.types.h"		/* abs s14 */
#include <errno.h>
#include "wish.h"
#include "token.h"
#include "slk.h"
#include "actrec.h"
#include "proc.h"
#include "procdefs.h"
#include "terror.h"
#include "sizes.h"

#include <unistd.h> 

#ifdef	SIGTSTP
void  on_suspend();
#endif

extern struct proc_rec PR_all[];
extern bool Suspend_interupt;
extern char *Suspend_window;


/* extern int errno;    EFT abs k16 */

int
proc_current(rec)
struct actrec *rec;
{
    int p = rec->id;
    pid_t pid, w;	 /* EFT abs k16 */
    int status;
    extern int _Debug;
    void sigcatch();
    void (*oldquit) (), (*oldint) ();
    void (*oldttin) (), (*oldttoui) ();
    int set_ret_val();
    
    if (PR_all[p].status == ST_DEAD)
	return(FAIL);					/* abs s18 */

    /* if process is not already forked, fork it else resume it */

    vt_before_fork();
    fork_clrscr();
    if (PR_all[p].pid == NOPID) {
#ifdef _DEBUG
	_debug(stderr, "NEW PROCESS FORKING\n");
#endif
	switch (pid = fork()) {
	case FAIL:
#ifdef _DEBUG
	    _debug(stderr, "Fork failed with errno=%d\n", errno);
#endif
	    error(MISSING, gettxt(":190","process fork failed") );
	    return(FAIL);
	case 0:			/* child */
#ifdef _DEBUG
	    if (_Debug)
		(void) freopen("/dev/tty", "w+", stderr);
#endif
	    sigset(SIGINT, SIG_DFL);
	    sigset(SIGQUIT, SIG_DFL);
	    sigset(SIGTTIN, SIG_DFL);
            sigset(SIGTTOU, SIG_DFL);
	    execvp(PR_all[p].name, PR_all[p].argv);
	    error_exec(errno);
	    child_error(NOEXEC, PR_all[p].name); /* abs k15 */
	    _exit(255);
	default:
	    oldquit = sigset(SIGQUIT, SIG_IGN); /* changed from..  */
	    oldint  = sigset(SIGINT, SIG_IGN); /* ..signal()  abs */
	    oldttin = signal(SIGTTIN,SIG_DFL);
            oldttoui = signal(SIGTTOU,SIG_DFL);
	    PR_all[p].pid = PR_all[p].respid = pid;
	    break;
	}
    } else {			/* resume */
	pid = PR_all[p].pid;
#ifdef _DEBUG
	_debug(stderr, "resuming pid %d by signaling %d\n", pid, PR_all[p].respid);
#endif
	if (PR_all[p].flags & PR_CLOSING) {
	    fflush(stdout);
	    fflush(stderr);

            printf( gettxt(":191","You are returning to a suspended activity. This activity\r\nmust be ended before you can complete logging out.\r\nPlease take whatever steps are necessary to end this\r\nactivity.\r\n\n") );

	    fflush(stdout);
	    sleep(3);
	} else {
	    fflush(stdout);
	    fflush(stderr);
            
	    printf( gettxt(":192","You are returning to a suspended activity. \r\n") );
	    fflush(stdout);
	    sleep(3);
	}

	if (kill(PR_all[p].respid, SIGUSR1) == FAIL) {
#ifdef _DEBUG
	    _debug(stderr, "RESUME SIGNAL FAILED WITH ERRNO=%d\n", errno);
#endif
	    return(FAIL);
	}
    }
    PR_all[p].status = ST_RUNNING;

#ifdef _DEBUG
    _debug(stderr, "Waiting for pid %d\n", pid);
#endif
    status = 0;
    Suspend_interupt = FALSE;

#ifdef	SIGTSTP
	signal(SIGTSTP, SIG_DFL);
#endif

    while ((w = wait(&status)) != pid) {
	if ((w == FAIL && errno != EINTR) || Suspend_interupt) {
#ifdef _DEBUG
	    _debug(stderr, "Woken while waiting for %d\n", pid);
#endif
	    break;
	}
    }

#ifdef	SIGTSTP
	signal(SIGTSTP, on_suspend);
#endif
    if (Suspend_interupt) {
#ifdef _DEBUG
	_debug(stderr, "Process %d suspended, making non-current\n", pid);
#endif
	if (Suspend_window == NULL)
	    ar_backup();	/* go back to previous activation record */
	else {
	    objop("OPEN", NULL, Suspend_window, NULL);
	    free(Suspend_window);
	    Suspend_window = NULL;
	}
    } else {
	(void) set_ret_val(status);
#ifdef _DEBUG
	_debug(stderr, "Process terminated, closing actrec\n");
#endif
	PR_all[p].pid = PR_all[p].respid = NOPID;
	if ((PR_all[p].flags == PR_ERRPROMPT && status>>8 != 0) ||
	    (PR_all[p].flags & ~PR_CLOSING) == 0) {
	    char buf[PATHSIZ];

	    printf( gettxt(":193","\r\nPress ENTER to continue") );
	    fflush(stdout);
	    fgets(buf, PATHSIZ, stdin);
	}
	if ((PR_all[p].flags & PR_CLOSING) == 0)
	    ar_close(rec, FALSE);
    }
    /*	signal(SIGINT, oldint);
	signal(SIGQUIT, oldquit);
	abs */
    sigset(SIGINT, oldint);
    sigset(SIGQUIT, oldquit);
    signal(SIGTTIN, oldttin);       /* must use signal() and not sigset() */
    vt_after_fork();
    return(SUCCESS);
}

void sigcatch(sig)
int sig;
{
/*	signal(sig, SIG_IGN);
	signal(sig, sigcatch);
abs */
        sigignore(sig);
	sigset(sig, sigcatch);
}
