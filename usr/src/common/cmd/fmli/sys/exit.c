/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/exit.c	1.8.3.4"

#include	<stdio.h>
#include	<string.h>
#ifndef PRE_CI5_COMPILE		/* abs s14 */
#include 	<siginfo.h>	/* abs s13 */
#endif				/* abs s14 */
#include	<unistd.h>	/* abs s13 */
#include	"wish.h"
#include	"var_arrays.h"
#include	"retcodes.h"	/* abs */

/* a pointer declared for use by the new macro */
char	*_tmp_ptr;
/* the home directory of the user in the current FACE session */
char	*Home;
/* the name of the filecabinet for the current session */
char	*Filecabinet;
/* the path to the current users Wastebasket */
char	*Wastebasket;
/* the location of FACE's system files */
char	*Filesys;
/* the value of the OASYS variable in the invoking environment */
char	*Oasys;
/* the name of the current FMLI process */
char	*Progname;
/* an array of temporary filenames that should be removed when FMLI is exited */
char	**Remove;
/* an array of functions to call when FMLI is exited */
int	(**Onexit)();
/* a global definition of the nil character string */
char	nil[] = "";
/* a flag that indicates that debug mode is active */
int	_Debug;
static bool	Killed_by_sig = FALSE;	/* abs s13 */

void
exit(n)
int	n;
{
	register int	i;
	int	lcv;

	if (n == R_BAD_CHILD)	/* abs */
	    _exit(n);
	else
	{
	    lcv = array_len(Onexit);
	    for (i = 0; i < lcv; i++)
		(*Onexit[i])(n);
	    lcv = array_len(Remove);
	    for (i = 0; i < lcv; i++)
		unlink(Remove[i]);
	    _cleanup();
	    _exit(n);
	}
}


/* print an error message on stderr indicating the session was aborted 
 * because of a signal received.  The message includes the name the 
 * program (application) was invoked with and a description of the signal.
 * abs s13
 */
void
sig_err_msg(signum)
int signum;
{
    if (Killed_by_sig == FALSE)
	return;
    if (Progname != NULL)
	(void)write(2, Progname, strlen(Progname));
#ifndef PRE_CI5_COMPILE					/* abs s14 */
    psignal(signum, gettxt(":237"," terminated by signal: ") );
#else							/* abs s14 */
    (void)write(2, gettxt(":238"," terminated by signal "), 22); 	/* abs s14 */
#endif							/* abs s14 */
}



/* sig_exit is called upon catching a signal that causes FMLI to 
 * terminate, for example after a SIGILL caused by an Illegal 
 * Instruction.  
 * 
 * Sets a flag to remember that a signal was received
 * and then calls exit.  sig_err_msg will report the error.
 * abs s13
 */

void
sig_exit(signum)
int signum;
{
    Killed_by_sig = TRUE;
    exit(signum);		/* fmli's exit not the C library call */
}

