#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/ttymon.h	1.2"
#endif

#ifndef	_TTYMON_H
#define _TTYMON_H


#define TTYMON1		"ttymon1"
#define COM1		"com1"
#define COM2		"com2"
#define TTY00		"tty00"
#define TTY01		"tty01"

#define MAX_CMD_LEN	1024


/* -data structure to hold information returned from the popen_execcmd()
**  function and supplied to the pclose_execcmd() function
*/
typedef struct pipeexec{
	FILE	*pe_fp;		/* allocated file pointer to parent side of
				** pipe.
				*/
	pid_t	pe_cpid;	/* child's process id */
	uint	pe_type;	/* type of "open", read or write */
	int	pe_savedfd;	/* file descriptor related to stdin or stdout
				** before the stdin or stdout was linked to the
				** pipe.
				*/
}pexec_t;

/* -types of pipeexec
*/
#define	PE_READ		((uint)(0))
#define	PE_WRITE	((uint)(1))


/*********************************/


/* making an argv type list, from a list of parameters, for an execv 
** systme call
**
**	-ap: pointer to first argument
**	-arg0: the basename of the command to be executed
**	-argv: pointer to list
**	-funcname: name of function. used if there is a failure
*/
#define MKARGV(ap, arg0, argv, funcname) {\
	int	numexecargs = 2;	/* number of arguments for exec. it is\
					** initialized to 2 since there must be\
					** an arg0 and a NULL pointer to\
					** terminiate the list\
					*/\
	int	i;\
\
\
	/* -count the number of command line arguments\
	** -allocate an array of count pointers (argv)\
	** -copy parameters to the argv\
	*/\
	va_start(ap);\
	while ((char *)(va_arg(ap, char*)) != (char *)NULL){\
		numexecargs++;\
	}\
	va_end(ap);\
	if ((argv = (char **)calloc((size_t)numexecargs, sizeof(char *))) ==\
	 (char **)NULL){\
		exit(0); /*NO RETURN*/\
	}\
	va_start(ap);\
	argv[0] = arg0;\
	for (i = 1; (argv[i] = (char *)va_arg(ap, char*)) != (char *)NULL; i++); /*NULL STATEMENT*/\
	va_end(ap);\
}


#endif 
