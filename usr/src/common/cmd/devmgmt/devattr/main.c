/*	copyright	"%c%"	*/

#ident	"@(#)devmgmt:common/cmd/devmgmt/devattr/main.c	1.7.13.1"

/*
 *  main.c
 *
 *  Contains the following:
 *	devattr		Command that returns [specific] attributes for
 *			a device.
 */

/*
 *  devattr [-v] device [attr [...]]
 *
 *	This command searches the Device Database for the device specified.
 *	If it finds the device (matched either by alias or pathname to
 *	device special file), it extracts the attribute(s) specified and writes 
 *	that value to the standard output stream (stdout).
 *
 *	The command writes the values of the attributes to stdout one per 
 *	line, in the order that they were requested.  If the -v option is 
 *	requested, it writes the attributes in the form <attr>='<value>' where
 *	<attr> is the name of the attribute and <value> is the value of that 
 *	attribute.
 *
 *  Returns:
 *	0	The command succeeded
 *	1	The command syntax is incorrect,
 *		An invalid option was used,
 *	2	The Device Database files could not be opened for reading.
 *	2	The Device Database files are not in consistent state
 *	3	The requested device was not found in the Device Database
 *	4	A requested attribute was not defined for the device
 *		No diagnostic is issued in this last case.
 */

/*
 *  Header files referenced:
 *	<sys/types.h>	UNIX System Data Types
 *	<stdio.h>	C standard I/O definitions
 *	<string.h>	C string manipulation definitions
 *	<errno.h>	Error-code definitions
 *	<stdlib.h>	Storage allocation functions
 *	<fmtmsg.h>	Standard message display definitions
 *	<devmgmt.h>	Device management headers
 *	<mac.h>		Mandatory Access Control definitions
 *	<pwd.h>		Password file definitions
 *	<grp.h>		Group file definitions
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<mac.h>
#include	<devmgmt.h>
#include	<mac.h>
#include	<pwd.h>
#include	<grp.h>
#include	<ctype.h>
#include	<pfmt.h>
#include	<locale.h>
/*
 *  External functions and variables referenced (not defined by header files)
 *
 *	optind		index to next arg on the command line (from getopt())
 *	opterr		FLAG, TRUE tells getopt() to suppress error messages
 *	optarg		Ptr to current option's argument
 *	devattr()	gets value of specified attr from Device Database
 * 	listdev()	lists the attributes defined for device
 *	getattrttype()	returns the attribute type. 
 *			Types are TYPE_SEC, TYPE_DSF, and TYPE_TAB
 * 	getlistcnt()	returns the number of comma-separated items in the string
 */

extern	int	optind;
extern	int	opterr;
extern	char   *optarg;
extern	char   *devattr();
extern	char   **listdev();
extern  int	getattrtype();
extern  int	getlistcnt();

/*
 *  Local constant definitions
 *	TRUE		Boolean TRUE
 *	FALSE		Boolean FALSE
 *	NULL		Null address
 */

#ifndef	TRUE
#define	TRUE	1
#endif

#ifndef	FALSE
#define	FALSE	0
#endif

#ifndef	NULL
#define	NULL	0
#endif

#ifndef	FAILURE
#define	FAILURE	-1
#endif

/* Used to check the value returned by getattrtype() */
#ifndef	TYPE_SEC	
#define TYPE_SEC        1
#endif

#ifndef LOGNAMEMAX
#define LOGNAMEMAX    32
#endif

/*
 *  Messages
 *	M_USAGE		Usage error
 *	M_CONSTY	Device Database files not in consistent state
 *	M_NODEV		Device not found in the device table
 *	M_DEVTAB	Can't open the device table
 * 	M_NOMEM 	Insufficient memory
 */

#define M_USAGE		":813:syntax incorrect, invalid options\n\
Usage: devattr [-v] device [attribute [...]]\n"
#define	M_CONSTY	":814:Device Database in inconsistent state - notify administrator\n"

#define	M_NODEV		":815:requested device not found in Device Database\n"

#define	M_DEVTAB	":816:Device Database could not be opened for reading\n"

#define	M_NOMEM 	":817:Insufficient memory\n"


/* 
 * Exit codes:
 */

#define	EX_OK 		0
#define	EX_USAGE	1
#define EX_ERROR	1		/* no memory left */
#define	EX_DDBPROB	2
#define EX_NODEV	3
#define	EX_NOATTR	4

/*
 *  Local static data
 *	txt		Buffer for the text of messages
 */

#define	MM_MXTXTLN	256
static char	txt[MM_MXTXTLN+1];

/*
 *  Internal functions
 */
static char * conv_level();
static void conv_secattr();

/*
 *  main()
 *
 *	Implements the command "devattr".   This function parses the command 
 *	line, then calls the devattr() function looking for the specified 
 *	device and the requested attribute.  It writes the information to
 *	the standard output file in the requested format.
 *
 */

main(argc, argv)
	int	argc;		/* Number of arguments */
	char   *argv[];		/* Pointer to arguments */
{

	/* Automatic data */
	char	*device;		/* Pointer to device name */
	char	*attr;			/* Pointer to current attribute */
	char	*value;			/* Pointer to current attr value */
	char	*p;			/* Temporary character pointer */
	char	**argptr;		/* Pointer into argv[] list */
	int	syntaxerr;		/* TRUE if invalid option seen */
	int	noerr;			/* TRUE if all's well in processing */
	int	v_seen;			/* TRUE if -v is on the command-line */
	int	exitcode;		/* Value to return */
	int	c;			/* Temp char value */
	int	ddb_state;
	int     etype;                  /* attribute's type */
	int     fieldno;                /* security attribute */

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxcore");
	(void)setlabel("UX:devattr");

	/*
	 *  Parse the command-line.
	 */

	syntaxerr = FALSE;
	v_seen = FALSE;

	/* Extract options */
	opterr = FALSE;
	while ((c = getopt(argc, argv, "v")) != EOF) switch(c) {

	    /* -v option:  No argument, may be seen only once */
	    case 'v':
		if (!v_seen) v_seen = TRUE;
		else syntaxerr = TRUE;
		break;

	    /* Unknown option */
	    default:
		syntaxerr = TRUE;
	    break;
	}

	/* 
	 * Check for a usage error 
	 *  - invalid option
	 *  - arg count < 2
	 *  - arg count < 3 && -v used 
	 */

	if (syntaxerr || (argc < (optind+1))) {
	    pfmt(stderr, MM_ACTION, M_USAGE);
	    exit(EX_USAGE);
	}

	/* Check if Device Database exists and in Consistent state */
	if ((ddb_state=ddb_check()) < 0) {
	   /* Not all the DDB files present/accessible.    */
	    pfmt(stderr, MM_ERROR, M_DEVTAB);
	    exit(EX_DDBPROB);
	} else if(ddb_state==0) {
	   /* DDB files corrupted - magic-nos do not match */
	    pfmt(stderr, MM_ERROR, M_CONSTY);
	    exit(EX_DDBPROB);
	}

	/* 
	 *  Get the list of known attributes for the device.  This does 
	 *  two things.  First, it verifies that the device is known in the 
	 *  device table.  Second, it gets the attributes to list just in 
	 *  case no attributes were specified.  Then, set a pointer to the 
	 *  list of attributes to be extracted and listed...  
	 */

	device = argv[optind];


	if ((argptr = listdev(device)) == (char **) NULL) {
	    switch (errno) {
	    case (ENODEV):
		strcpy(txt, M_NODEV);
		exitcode = EX_NODEV;
		break;
	    default:
		strcpy(txt, M_DEVTAB);
		exitcode = EX_DDBPROB;
		break;
	    }
	    pfmt(stderr, MM_ERROR, txt);
	    exit(exitcode);
	}

	if (argc > (optind+1)) argptr = &argv[optind+1];

	/*
	 *  List attributes.  
	 */

	exitcode = EX_OK;
	noerr = TRUE;
	while (noerr && ((attr = *argptr++) != (char *) NULL)) {
	    if ((value = devattr(device, attr))==(char *)NULL) {
		noerr = FALSE;
		switch (errno) {
		case (ENODEV):
		    strcpy(txt, M_NODEV);
		    exitcode = EX_NODEV;
		    break;
		case (EINVAL):
		    /* 
		     * If the device doesn't have the attribute defined
		     * continue with next attribute, without any diagnostic
		     * To be conforming with 4.0 the exit code is 4.
		     */
		    noerr = TRUE;
		    exitcode = EX_NOATTR;
		    continue;
		    break;
		default:
		    strcpy(txt, M_DEVTAB);
		    exitcode = EX_DDBPROB;
		    break;
		}
		if (!noerr) {
		    pfmt(stderr, MM_ERROR, txt);
		    exit(exitcode);
		}
	    }
	    /* If getattrtype() returns TYPE_SEC, call conv_secattr().
	     * This function translates the security attributes that
	     * have an internal representation different from 
	     * the external representation. In the case where the translation
	     * fails, the ID is displayed.
	     */
	    etype = getattrtype(attr, &fieldno);
	    if (etype == TYPE_SEC)
		conv_secattr(&value, fieldno);
		
	    if (noerr && v_seen) {
		(void) fputs(attr, stdout);
		(void) fputs("='", stdout);
		for (p = value ; *p ; p++) {
		    (void) putc(*p, stdout);
		    if (*p == '\'') (void) fputs("\"'\"'", stdout);
		}
		(void) fputs("'\n", stdout);
	    } else if (noerr) {
		(void) fputs(value, stdout);
		(void) putc('\n', stdout);
	    }
	}

	/* Exit */
	exit(exitcode);
#ifdef	lint
	return(0);
#endif
}

/*
 * conv_secattr converts the security attributes that
 * are stored as IDs:
 *
 * 	range, startup level, startup owner, startup group and users.
 *
 * conv_secattr() will traslate the attributes' internal representation (ID)
 * to their external representation. 
 * In the case of range and startup level
 * from LID to alias name or fully qualified names;
 * in the case of startup owner  and users from user IDs to lognames;
 * and, in the case for startup group from group ID to group name.
 * If the translation fails, the origial string is returned in the
 * argument 'value'.
 * 
 *	 value contains the original attribute'svalue.
 *    	 If the value can be converted from to an external representation,
 *	 value will contain the result of the conversion.
 * 
 * 	 filedno contains an decimal value that associates the
 *       security attribute with the translation steps needed.
 * 
 */
void
conv_secattr(value, fieldno)
char **value;
int fieldno;
{
	static char lvlname[LVL_NAMESIZE]; 	/* level name */
	static char savevalue[32];		/* attr's original value */
	static char ownership[LOGNAMEMAX + 5];  /* gid>rwx or uid>rwx */
	static char 	*users;	    	/* users logname and permissions */
	static char 	*saveusers; 		/* Original user's string */

	int	user_cnt = 0;
	char  	*str1, *str2, *p;       /* Temporary charater pointers */
	level_t level_1, level_2;	/* contain range or startup level */

	uid_t	uid;			/* startup owner's uid or users */
	gid_t	gid;			/* startup group id */
	struct 	passwd *pw_ent;
	struct  group  *grp_ent;

	switch(fieldno) {
	case (1):	/* range */
		/* getfield modifies the string passed as argument,
		 * so save the range's value in case the translation to
	 	 * alias or fully qualified name fails 
		 */
		strcpy(savevalue, *value);

		/* get the hi and lo strings' value from
	 	 * the range contained in the DDB. 
		 */
		str1 = (char *) getfield(*value,"-",&str2);

		/* Convert ACSCII representation of LIDs to number */
		level_1  = (level_t)strtol(str1,(char **)NULL,0);
		level_2  = (level_t)strtol(str2,(char **)NULL,0);

		/* Convert LID to level alias name or fully qualified */
		str1 = str2 = (char *)NULL;
		str1 = conv_level(level_1);
		str2 = conv_level(level_2);

		if (str1 != (char *)NULL && str2 != (char *)NULL) {
			strcpy(lvlname,str1);
			strcat(lvlname,"-");
			strcat(lvlname,str2);
			*value = lvlname;
    		} else {
			*value = savevalue;
		}
		break;

	case (2):	/* state */
	case (3):	/* mode */
	case (4):	/* startup */
		break;

	case (5): 	/* startup level */
		level_1 = (level_t)strtol(*value,(char **)NULL,0);
		str1 = conv_level(level_1);
		if (str1 != (char *)NULL) {
			strcpy(lvlname, str1);
			*value = lvlname;
    		}
    		break;

	case (6): 	/* startup owner */
		strcpy(savevalue, *value);
		p =  *value;

		/* get user id */
		str1 = (char *) getfield(p,">",&p);
		uid  = (uid_t) strtol(str1, (char **)NULL,0);

		/* get user's entry from /etc/passwd */
		pw_ent = getpwuid(uid);
		if (pw_ent != (struct passwd *)NULL) {
			strcpy(ownership, pw_ent->pw_name);
			strcat(ownership,">");
			strcat(ownership, p);
			*value = ownership;
		} else {
			*value = savevalue;
		}
		break;

	case (7): 	/* startup group */
		strcpy(savevalue, *value);
		p =  *value;

		/* get group id */
		str1 = (char *) getfield(p,">",&p);
		gid = (gid_t) strtol(str1, (char **)NULL,0);

		/* get group entry from file /etc/group */
		grp_ent = getgrgid(gid);
		if (grp_ent != (struct group *)NULL) {
			strcpy(ownership, grp_ent->gr_name);
			strcat(ownership,">");
			strcat(ownership, p);
			*value = ownership;
		} else {
			*value = savevalue;
		}
		break;

	case (8):	/* starup other */
	case (9):	/* ual_enable */
		break;
	case (10):	/* users */
		/* allocate space to save the string that contains the ual */
		saveusers = (char *)malloc(strlen(*value) + 1);
		if (saveusers == (char *)NULL) {
			free(saveusers);
			return;
		}
		strcpy(saveusers, *value);	

		/* get a count on the number of items in the list */
		user_cnt = getlistcnt(*value);
		if (!user_cnt) {
			free(saveusers);
			return;
		}
		/* 
		 * allocate space for string that 
		 * will contain ual listed by logname
		 */
		users = (char *)malloc(user_cnt * (LOGNAMEMAX + 3) + 1);
		if (users == (char *)NULL) {
			free(saveusers);
			return;
		}
		*users = '\0';
		p = *value;

		while (*p != '\0' && user_cnt-- > 1) {

			/* get uid and permissions for that uid */
			str1 = (char *) getfield(p,">",&p);
			str2 = (char *) getfield(p,",",&p);

			/* Convert ACSCII representation of uid to number */
			uid  = (uid_t)strtol(str1,(char **)NULL,0);

			pw_ent = getpwuid(uid);
			if (pw_ent != (struct passwd *)NULL &&
				str2 != (char *)NULL) {
				if (*users == '\0')
					strcpy(users,pw_ent->pw_name);
				else
					strcat(users,pw_ent->pw_name);
			} else {
				/* the uid is not defined in /etc/passwd.
				 * keep the information as uid
				 */
				if (*users == '\0') 
					strcpy(users,str1);
				else
					strcat(users,str1);
				
			}
			strcat(users,">");
			strcat(users,str2);
			strcat(users,",");
		}
		if (*p == '\0') {
			*value = saveusers;
			return;
		}
		/* last uid in the list */
		str1 = (char *) getfield(p,">",&p);
		uid  = (uid_t) strtol(str1,(char **)NULL,0);

		pw_ent = getpwuid(uid);
		if (pw_ent != (struct passwd *)NULL && (*p == 'n'||*p == 'y')){
			strcat(users,pw_ent->pw_name);
		} else {
			strcat(users,str1);
		}
		strcat(users,">");
		strcat(users,p);
		*value = users;
		break;
	
	case (11):	/* other */
		break;
	default:
		break;
    	} /* end of switch */
}
/*
 * Subroutine: conv_level()
 * Restrictions:
 *		lvlout(2)	None
 * Notes:
 *    conv_level() attempts to convert the LID to an alias name.
 *    If that translation fails, then it tries to convert the LID
 *    to a fully qualified name. If that attempt also fails, 
 *    (char *)NULL is returned. Otherwise, a pointer to the string
 *    representing the alias name or the fully qualified name id returned.
 * 	
 */
static char *
conv_level(level)
level_t level;
{
	char	*bufp;
	int	bufsz;

	/*
	 * Try converting to alias name; if conversion fails, try converting
	 * to fully qualified name. If neither succeeds, return (char *)NULL
	 */
	if ((bufsz=lvlout(&level, (char *)NULL, 0, LVL_ALIAS)) < 0)  {
		return((char *)NULL);
	}

	/*
	 * lvlout returns Ok even if there was no alias. if there was none
	 * the sting would just be numerical represntation of LID
	 * so attempt to convert it and see it is equal if it is free it
	 * and try a fully qualified.
	 */
	 

	if (bufp=(char *)malloc(bufsz)) {
		/* convert level to requested format */
	   	lvlout(&level, bufp, bufsz, LVL_ALIAS);
	} else {
	    	/* out of memory */
	        pfmt(stderr, MM_ERROR, M_NOMEM);
	    	exit(EX_ERROR); 
	}


	if ((level_t)atoi(bufp) == level) {
		/* returned LID, try fully qualified */
		free(bufp);
		if ((bufsz=lvlout(&level, (char *)NULL, 0, LVL_FULL)) < 0)  {
		    	return((char *)NULL);
		}
		else {
			if (bufp=(char *)malloc(bufsz)) {
				/* convert level to requested format */
	   			lvlout(&level, bufp, bufsz, LVL_FULL);
	    			return(bufp);
			} else {
	    			/* out of memory */
	        		pfmt(stderr, MM_ERROR, M_NOMEM);
	    			exit(EX_ERROR); 
			}
		}
	}
	else {
	    	return(bufp);
	}
}
