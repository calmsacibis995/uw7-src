/*		copyright	"%c%" 	*/

#ident	"@(#)devmgmt:common/cmd/devmgmt/getdev/main.c	1.5.13.1"

/*
 *  getdev.c
 *
 *  Contains
 *	getdev	Writes on the standard output stream a list of devices
 *		that match certain criteria.
 */

/*
 *  Header Files Referenced
 *	<sys/types.h>	Standard UNIX data types
 *	<stdio.h>	Standard I/O 
 *	<errno.h>	Error handling
 *	<stdlib.h>	Storage allocation functions
 *	<string.h>	String handling
 *	<pfmt.h>	Standard message generation 
 *	<locale.h>	
 *	<devmgmt.h>	Device management
 */
#include	<sys/types.h>
#include	<stdio.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<string.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<devmgmt.h>

/*
 *  External References:
 *	getopt()	Parse command-line options
 *	opterr		Error flag for getopt()
 *	optarg		Argument to just parsed option (from getopt())
 *	getdev()	Get the devices that match criteria
 *	exit()		Exit the command
 *	putenv()	Modify the environment
 */
extern int	getopt();
extern int	opterr;	
extern int	optind;
extern char	*optarg;
extern char	**getdev();
extern void	exit();		
extern int	putenv();

/*
 *  Local Definitions
 *	TRUE		Boolean TRUE value
 *	FALSE		Boolean FALSE value
 *	EX_OK		Exit Value if all went well
 *	EX_ERROR	Exit Value if usage error occurred
 *	EX_OTHER	Exit Value for all other errors.
 */
#ifndef	TRUE
#define	TRUE		(1)
#endif

#ifndef	FALSE
#define FALSE		(0)
#endif

#define	EX_OK		0
#define	EX_ERROR	1
#define EX_DDBPROB	2

/*
 *  Messages:
 *	M_USAGE		Usage error
 *	M_CONSTY	Device Database files not in consistent state
 *	M_DEVTAB	Can't open the device table
 */
#define	M_USAGEMSG	":1:Incorrect usage\n"
#define	M_USAGE	  ":818:Usage: getdev [-ae] [criterion [...]] [device [...]]\n"
#define	M_CONSTY  ":814:Device Database in inconsistent state - notify administrator\n"
#define	M_DEVTAB  ":816:Device Database could not be opened for reading\n"

/*
 *  Local (Static) Definitions and macros
 *	buildcriterialist()	Builds the criteria list from the command-line
 *	builddevlist()		Builds the device list from the command-line
 */
static	char  **buildcriterialist();
static	char  **builddevlist();

/*
 *  getdev [-ae] [criterion [...]] [device [...]]
 *
 *	This command generates a list of devices that match the
 *	specified criteria.
 *  
 *  Options:
 *	-a		A device must meet all of the criteria to be
 *			included in the generated list instead of just
 *			one of the criteria (the "and" flag)
 *	-e		Exclude the devices mentioned from the generated
 *			list.  If this flag is omitted, the devices in the
 *			list are selected from the devices mentioned.
 *
 *  Arguments:
 *	criterion	An <attr><op><value> expression that describes
 *			a device attribute.  
 *			<attr>	is a device attribute
 *			<op> 	may be = != : !: indicating equals, does not
 *				equal, is defined, and is not defined 
 *				respectively
 *			<value>	is the attribute value.  Currently, the only
 *				value supported for the : and !: operators
 *				is *
 *	device		A device to select for or exclude from the generated
 *			list
 *
 *  Exit values:
 *	EX_OK		All went well
 *	EX_ERROR	An error (syntax, internal, or resource) occurred
 *	EX_DDBPROB	Device Database could not be opened for reading or
 *			Device Database files are in inconsistent state
 */
main(argc, argv)
	int	argc;		/* Number of items on the command line */
	char  **argv;		/* List of pointers to the arguments */
{

	/* 
	 *  Automatic data
	 */

	char	      **arglist;	/* List of arguments */
	char	      **criterialist;	/* List of criteria */
	char	      **devicelist;	/* List of devices to search/ignore */
	char	      **fitdevlist;	/* List of devices that fit criteria */
	char	       *cmdname;	/* Simple command name */
	char	       *device;		/* Device name in the list */
	FILE	       *tabfp;		/* File ptr to DDB_TAB */
	int		exitcode;	/* Value to return to the caller */
	int		sev;		/* Message severity */
	int		optchar;	/* Option character (from getopt()) */
	int		andflag;	/* TRUE if criteria are to be anded */
	int		excludeflag;	/* TRUE if exclude "devices" lists */
	int		options;	/* Options to pass to getdev() */
	int		usageerr;	/* TRUE if syntax error */
	int		ddb_state;	/* state of DDB files   */


	(void)setlocale(LC_ALL,"");
	(void)setcat("uxcore");
	(void)setlabel("UX:getdev");

	/* 
	 *  Parse the command line:
	 *	- Options
	 *	- Selection criteria
	 *	- Devices to include or exclude
	 */

	/* 
	 *  Extract options from the command line 
	 */

	/* Initializations */
	andflag = FALSE;	/* No -a -- Or criteria data */
	excludeflag = FALSE;	/* No -e -- Include only mentioned devices */
	usageerr = FALSE;	/* No errors on the command line (yet) */

	/* 
	 *  Loop until all of the command line options have been parced 
	 */
	opterr = FALSE;			/* Don't let getopt() write messages */
	while ((optchar = getopt(argc, argv, "ae")) != EOF) switch (optchar) {

	/* -a  List devices that fit all of the criteria listed */
	case 'a': 
	    if (andflag) usageerr = TRUE;
	    else andflag = TRUE;
	    break;

	/* -e  Exclude those devices mentioned on the command line */
	case 'e':
	    if (excludeflag) usageerr = TRUE;
	    else excludeflag = TRUE;
	    break;

	/* Default case -- command usage error */
	case '?':
	default:
	    usageerr = TRUE;
	    break;
	}

	/* If there is a usage error, write an appropriate message and exit */
	if (usageerr) {
	    pfmt(stderr, MM_ERROR, M_USAGEMSG);
	    pfmt(stderr, MM_ACTION, M_USAGE);
	    exit(EX_ERROR);
	}

	/* Check if Device Database exists and in Consistent state */
	if ((ddb_state=ddb_check()) < 0) {
	   /* Not all the DDB files present/accessible.    */
	    pfmt(stderr, MM_ERROR, M_DEVTAB);
	    exit(EX_DDBPROB);
	} else if(ddb_state==0) {
	   /* DDB files corrupted - magic-nos do not match */
	    pfmt(stderr,MM_ERROR, M_CONSTY);
	    exit(EX_DDBPROB);
	}

	/* Open the device file (if there's one to be opened) */
	if ((tabfp=fopen(DDB_TAB,"r"))==(FILE *)NULL) {
	    pfmt(stderr, MM_ERROR, M_DEVTAB);
	    exit(EX_DDBPROB);
	}

	/* Build the list of criteria and devices */
	arglist = argv + optind;
	criterialist = buildcriterialist(arglist);
	devicelist = builddevlist(arglist);
	options = (excludeflag?DTAB_EXCLUDEFLAG:0)|(andflag?DTAB_ANDCRITERIA:0);

	/* 
	 *  Get the list of devices that meets the criteria requested.  If we 
	 *  got a list (that might be empty), write that list to the standard 
	 *  output file (stdout).
	 */
	exitcode = 0;
	if (!(fitdevlist = getdev(devicelist, criterialist, options))) {
	    exitcode = 1;
	}
	else for (device = *fitdevlist++ ; device ; device = *fitdevlist++)
		(void) puts(device);

	/* Finished */
	exit(exitcode);

#ifdef	lint
	return(exitcode);
#endif
}

/*
 *  char **buildcriterialist(arglist)
 *	char  **arglist
 *
 *	Build a list of pointers to the criterion on the command-line
 *
 *  Arguments:
 *	arglist		The list of arguments on the command-line
 *	
 *  Returns:  char **
 *	The address of the first item of the list of criterion on the
 *	command-line.  This is a pointer to malloc()ed space.
 */
static char  **
buildcriterialist(arglist) 
	char  **arglist;	/* Pointer to the list of argument pointers */
{
	/*
	 *  Automatic data
	 */
	char  **pp;			/* Pointer to a criteria */
	char  **allocbuf;		/* Pointer to the allocated data */
	int	ncriteria;		/* Number of criteria found */


	/*
	 *  Search the argument list, looking for the end of the list or 
	 *  the first thing that's not a criteria.  (A criteria is a 
	 *  character-string that contains a colon (':') or an equal-sign ('=')
	 */
	pp = arglist;
	ncriteria = 0;
	while (*pp && (strchr(*pp, '=') || strchr(*pp, ':'))) {
	    ncriteria++;
	    pp++;
	}

	if (ncriteria > 0) {

	    /* Allocate space for the list of criteria pointers */
	    allocbuf = (char **) malloc((unsigned)((ncriteria+1)*sizeof(char **)));

	    /* 
	     *  Build the list of criteria arguments 
	     */
	    pp = allocbuf;	/* Beginning of the list */
	    while (*arglist &&			/* If there's more to do ... */
		   (strchr(*arglist, '=') ||	/* and it's a = criterion ... */
		    strchr(*arglist, ':')))	/* or it's a : criterion ... */
			*pp++ = *arglist++;	/* Include it in the list */
	    *pp = (char *) NULL;	/* Terminate the list */

	} else allocbuf = (char **) NULL;	/* NO criteria */
	

	return (allocbuf);
}

/*
 *  char **builddevlist(arglist)
 *	char  **arglist
 *
 *	Builds a list of pointers to the devices mentioned on the command-
 *	line and returns the address of that list.
 *
 *  Arguments:
 *	arglist		The address of the list of arguments to the
 *			getdev command.
 *
 *  Returns:  char **
 *	A pointer to the first item in the list of pointers to devices
 *	specified on the command-line
 */
static char  **
builddevlist(arglist) 
	char  **arglist;	/* Pointer to the list of pointers to args */
{
	/*
	 *  Automatic data
	 */

	/*
	 *  Search the argument list, looking for the end of the list or the 
	 *  first thing that's not a criteria.  It is the first device in the 
	 *  list of devices (if any).
	 */
	while (*arglist && (strchr(*arglist, '=') || strchr(*arglist, ':'))) arglist++;

	/* Return a pointer to the argument list. */
	return(*arglist?arglist:(char **) NULL);
}
