/*		copyright	"%c%" 	*/

#ident	"@(#)err.h	1.2"
#ident  "$Header$"
#ifndef ERR
#define	ERR	1	/*Flag to say we have been here*/

#define	ERR_NONE	-1
#define	ERR_WARN	MM_WARNING
#define	ERR_ERR		MM_ERROR

#define	ERR_UNKNOWN	0
#define	ERR_CONTINUE	1
#define	ERR_QUIT	2

#define	ERRMAX	1024

struct	msg {
	int	sev;
	int	act;
	char	text[ERRMAX];
	char	args[4][1024];
};

#endif
