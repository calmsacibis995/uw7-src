/*		copyright	"%c%" 	*/

#ident	"@(#)privlib.c	1.2"
#ident  "$Header$"
/*
** This library contains routines which are useful to routines which
** will work with privileges.
*/

#include <sys/types.h>
#include <priv.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <pfmt.h>
#include "err.h"

extern	char	*strcpy();
extern	char	*privname();
extern	int	privnum();
extern	int	strcmp();
extern	void	tfm_report();

/*
** The buffer msg is used internally to store error messages to be printed
** by the privilege message routine.  The text of the message is set
** up by any routine that fails, along with the severity and action
** code.  The severity can be ERR_NONE, ERR_MSG, ERR_WARN, or
** ERR_ERROR, the action code can be ERR_QUIT or ERR_CONTINUE. 
** Setting the severity to ERR_NONE clears the error condition and
** causes the error reporting routine to suppress printing and
** return 0 to indicate that no message was posted by any privilege
** routine.  Once a message has been reported, the severity is set
** to ERR_NONE to prevent the error from being reported twice.
*/

static	struct	msg	msgbuf = {ERR_NONE,ERR_CONTINUE,{0},{{0},{0},{0},{0}}};

/*
**The following error message strings are used to report errors encountered
**by the privilege library routines.
*/
#define BADPRIV		":22:Invalid process privilege: \"%s\"\n"
#define DUPPRIV		":23:Duplicate process privilege: \"%s\"\n"
#define BADPNUM		":24:Unrecognized privilege number: \"%d\"\n"
#define MALLOC		":2:Memory allocation failed\n"

#define pm_allon	((1 << NPRIVS) - 1)

/*
** Function: privadd()
**
** Notes:
** Given a privilege name, find the correct privilege vector bit and
** turn it on in the privilege vector supplied in 'result.'  If the
** supplied string is not the name of a privilege, do nothing.
*/
int
privadd(result,str)
unsigned	*result;
char		*str;
{
	register int		pbit;
	register unsigned	mask;

	pbit = privnum(str);
	if(pbit < 0){
		(void)strcpy(msgbuf.text,BADPRIV);
		(void)strcpy(msgbuf.args[0],str);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	if((pbit != P_ALLPRIVS) && (pbit > NPRIVS)){
		(void)strcpy(msgbuf.text,BADPRIV);
		(void)strcpy(msgbuf.args[0],str);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	if(pbit == P_ALLPRIVS){
		mask = pm_allon;
	} else {
		mask = 1 << pbit;
	}
	if((*result) & mask){
		(void)strcpy(msgbuf.text,DUPPRIV);
		(void)strcpy(msgbuf.args[0],str);
		msgbuf.sev = ERR_ERR;
		msgbuf.act = ERR_QUIT;
		return(-1);
	}
	(*result) |= mask;
	return(0);
}

/*
** Function: hasprivs()
**
** Notes:
** This routine determines whether a sample privilege vector 'sample'
** has all of the privileges in a test privilege vector 'tst.'  If 'sample'
** has the all privileges in 'tst,' the routine returns a 1 (true)
** otherwise it returns a 0 (false).
*/
int
hasprivs(tst,sample)
unsigned	tst;
unsigned	sample;
{
	return((tst & sample) == tst);
}

/*
** Function: misspriv()
**
** Notes:
** This routine searches from one greater than the privilege specified
** in 'pos' until it finds a privilege that is turned on in the
** privilege vector 'tst' and not turned on in the privilege
** vector 'sample.'  It then returns the privilege number of the mismatch. 
** If no mismatch is found, this routine returns a -1.  By feeding the
** return value from a call to this function back in as the 'pos'
** parameter the calling program can sequentially read out the privileges
** missing from 'tst.'
**
** IMPORTANT: The first 'pos' value fed into this function determines
** where it will start looking.  To scan the entire vector, pass -1 as
** the first position.
*/
int
misspriv(tst,sample,pos)
unsigned	tst;
unsigned	sample;
int	pos;
{
	unsigned	mask;

	for(++pos; pos < sizeof(unsigned) * CHAR_BIT; ++pos){
		mask = 1 << pos;
		if((tst & mask) && (!(sample & mask))){
			return(pos);
		}
	}
	return(-1);
}

/*
** Function: priv_err()
**
** Notes:
** This routine sets up messages posted by the privilege routines
** for printing.  The 'cmdnm' argument is the name of the calling
** program (derived from argv[0]) and the 'action' argument is the
** requested action to be taken after the message has been posted.
** This action can be ERR_CONTINUE, ERR_QUIT or ERR_UNKNOWN. If it
** is ERR_CONTINUE, this routine will return a 1 after printing the
** message, if ERR_QUIT this routine will exit with a 1 after
** printing the message.  If the action is ERR_UNKNOWN the action
** specified by the failing routine will be taken.  If no error was
** posted by any privilege routine, this routine returns a 0 to
** indicate that there was no message pending and prints nothing.
*/
int
priv_err(action)
int	action;
{
	if(msgbuf.act == ERR_NONE){
		return(0);		/*no error in this library*/
	}
	if(action != ERR_UNKNOWN){
		msgbuf.act = action;
	}
	tfm_report(&msgbuf);
	return(1);			/*error posted by this library*/
}

/*
** Function: showprivs()
**
** Notes:
** This routine prints out the list of privilege names corresponding to the
** privilege vector specified in 'p'.  If an unknown privilege bit is turned
** on, the routine prints out <unknown>.  It is up to the calling
** routine to supply the newline.
*/
void
showprivs(p)
unsigned	p;
{
static	char	pname[BUFSIZ];
	int	b=0;

	for(b = 0; p ; p >>= 1, ++b){
		if(p & 1){
			if(privname(pname, b)){
				(void)printf("%s ",pname);
			} else {
				(void)pfmt(stdout,MM_NOSTD,":25:<unknown> ");
			}
		}
	}
}
