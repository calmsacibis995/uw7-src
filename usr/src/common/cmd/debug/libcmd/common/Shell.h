#ident	"@(#)debugger:libcmd/common/Shell.h	1.2"

#ifndef	Shell_h
#define	Shell_h

#include <sys/types.h>

class PtyInfo;

// Invoke the shell to run a UNIX system command
extern	pid_t	Shell( char * cmd, int redir, PtyInfo *& );

// for termination message ---
extern	char *	sig_message[];

#endif	/* Shell_h */
