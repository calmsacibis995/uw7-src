/*		copyright	"%c%" 	*/

#ident	"@(#)devmgmt:common/cmd/devmgmt/putdev/main.c	1.6.18.1"
#ident  "$Header$"

/***************************************************************************
 * Command: putdev
 * Inheritable Privileges: P_MACWRITE,P_SETFLEVEL,P_OWNER
 *       Fixed Privileges: None
 * Notes: 
 *
 ***************************************************************************/


/*
 *  G L O B A L   R E F E R E N C E S
 */

/*
 * Header files needed:
 *	<sys/types.h>	Standard UNIX(r) types
 *	<stdio.h>	Standard I/O definitions
 *	<string.h>	String handling definitions
 *	<ctype.h>	Character types & macros
 *	<stdlib.h>	Storage alloc functions
 *	<errno.h>	Error codes
 *	<unistd.h>	Standard Identifiers
 *	<fmtmsg.h>	Standard Message Generation definitions
 *	<signal.h>	Signal handling
 *	<devmgmt.h>	Device Management definitions
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<unistd.h>
#include	<fmtmsg.h>
#include	<signal.h>
#include	<devmgmt.h>


/*
 * Device Management Globals referenced:
 *	adddevrec()		Add a device definition to the Device Database
 *	moddevrec()		Modify a device definition in Device Database
 *	remdevrec()		Remove a device from the Device Database
 *	rmdevattrs()		Remove device attrs of specified device from DDB
 *      appattrlist()		Appends to value-list of specified attribute.
 *      remattrlist()		Removes values from value-list of attribute.
 *	valid_alias()		Checks that alias has only allowed characters
 *	lock_ddb()		Locks DDB files
 *	unlock_ddb()		Unlocks DDB files
 */
#if defined(__STDC__)
int	adddevrec(char *, char **, int);
int	moddevrec(char *, char **, int);
int	remdevrec(char *, int);
int	rmdevattrs(char *, char **, int);
int	appattrlist(char *, char *, int);
int	remattrlist(char *, char *, int);
int	getaliasmap(char *, int *, int *, char *);
void	ddb_errmsg(int, int, char *,...);
void	err_report(char *, int);
int	ddb_check();
int	ddb_create();
int	lock_ddb();
int	unlock_ddb();
#else
int	adddevrec();
int	moddevrec();
int	remdevrec();
int	rmdevattrs();
int	appattrlist();
int	remattrlist();
int	getaliasmap();
void	ddb_errmsg();
void	err_report();
int	ddb_check();
int	ddb_create();
int	lock_ddb();
int	unlock_ddb();
#endif	/* __STDC__ */
/*
 *  L O C A L   D E F I N I T I O N S
 */

/*
 * General Purpose Constants
 *	TRUE		Boolean TRUE (if not already defined)
 *	FALSE		Boolean FALSE (if not already defined)
 *	NULL		Null address (if not already defined)
 */

#ifndef	TRUE
#define	TRUE	(1)
#endif

#ifndef	FALSE
#define	FALSE	(0)
#endif

#ifndef	NULL
#define	NULL	(0)
#endif

#ifndef SUCCESS
#define SUCCESS  (0)
#endif

#ifndef FAILURE
#define FAILURE	-1
#endif

#define NOMEM	1
#define NOATTR	9999	/* If attribute being removed is not defined, then
			 * rmdevattr() returns this value. This macro 
			 * definition should always match the one defined on
			 * libadm:devtab.h 
			 */

#define TYPE_SEC 	1
#define TYPE_DSF 	2
#define TYPE_TAB 	4

#define MAXSECATTRS     12
#define MAXDSFATTRS     4
#define MAXTABATTRS     3


/* security attributes of devices */
static char	*sec_attrs[]= {
			DDB_ALIAS, DDB_RANGE, DDB_STATE, DDB_MODE,
			DDB_STARTUP, DDB_ST_LEVEL, DDB_ST_OWNER,
			DDB_ST_GROUP, DDB_ST_OTHER, DDB_UAL_ENABLE,
			DDB_USERS, DDB_OTHER
		};

/* oam(tab) attributes of devices */
static char	*tab_attrs[]= {
			DDB_ALIAS,
			DDB_SECDEV,
			DDB_PATHNAME
			/* any other OA&M attrs names can be added 
			 * to DDB_TAB without being defined here.
                         */
		};

/* dsf attributes of devices */
static char	*dsf_attrs[]= {
			DDB_CDEVICE, DDB_BDEVICE, DDB_CDEVLIST, DDB_BDEVLIST };

struct {
	char sec[MAXSECATTRS];
	char tab[MAXTABATTRS];
	char dsf[MAXDSFATTRS];
} attr_flag = {0};


/*
 * Error messages and exit codes:
 */
#define E_USAGE1 "incorrect usage\n"
#define	E_USAGE2 "USAGE: putdev -a alias [secdev=alias] [attribute=value [...]]\n\
       putdev -m device attribute=value [attribute=value [...]]\n \
       putdev -d device [attribute[...]]\n \
       putdev -p device attribute=value[,value...]\n \
       putdev -r device attribute=value[,value...]\n"

#define E_ACCESS	"Device Database could not be accessed or created\n"
#define E_AEXIST        "%s already exists in Device Database\n"
#define E_AGAIN         "Device Database in use - Try again later\n"
#define E_CONSTY	"Device Database in inconsistent state - notify administrator\n"
#define E_DINVAL	"invalid alias or invalid pathname \"%s\"\n"
#define E_ENODEV         "%s does not exist in Device Database\n"
#define E_MULTDEFS	"multiple definitions of an attribute are not allowed\n"
#define E_NOATTR	"\"%s\" not defined for %s\n"
#define E_NOMEM		"insufficient memory\n"


#define EX_USAGE	1	/* usage error */
#define EX_ERROR	1	/* internal errors, like no memory left */
#define EX_ACCESS	2	/* DDB cannot be accessed or created */
#define EX_CONSTY	2	/* DDB is in inconsistent state */
#define	EX_AEXIST	3	/* alias is already defined in DDB */
#define EX_ENODEV	3	/* device is not defined in DDB */
#define EX_NOATTR	4	/* attribute not defined for device */
#define EX_OTHER	6	/* For error msgs related to ES */
#define OTHER		EX_OTHER

#define WARNING		999	/* If warning was issued from adddevrec
				 * or moddevrec, this value will be returned
				 * from these functions (provided no other
				 * error was encountered
				 */

/* flags for err_report */
#define SEV_ERROR	1
#define ACT_QUIT	1	
#define ACT_CONT	2

/*
 * Internal Static routines and variables:
 */
static void err_usage();
static int  check_attrs();
static int  nattrs;		/* Number of attributes on command */


/*
 * COMMAND: putdev
 *
 * putdev -a alias [attribute=value [...]]
 * putdev -m device [secdev=value] attribute=value [attribute=value [...]]
 * putdev -d device [attribute [...]]
 * putdev -p device attribute=value[,value...]
 * putdev -r device attribute=value[,value...]
 *
 * 	This command is used to add/modify/remove device definitions to
 *	the Device Database.
 *
 * Options:
 *	-a		Add an alias and associated attr values to the
 *			Device Database.
 *	-m		Modify an existing device definition.
 *	-d		If no attributes attributes are specified, then
 *			it removes the specified device from Device Database.
 *			If attributes are specified, then it removes those
 *			attribute definitions(values) from the specified
 *			device.
 *	-p		Appends the list of values to the specified attribute,
 *			in an existing device definition.
 *	-r		Removes the list of values from the specified
 *			attribute, in an existing device definition.
 * Note:
 *	secdev		used for mapping aliases to a Secure Device alias,
 *			that defines security attributes. By this mapping,
 *			many aliases can share one set of security attrs.
 *	-p|-r		options can be used ONLY on attributes that take
 *			a comma separated list of values --
 *			    cdevlist, bdevlist, users.
 *			
 * Exit values:
 *	Set by the external function err_report().
 */

static int ddblock = FALSE;	/* flag for libadm functions that try to lock
				 * DDB letting them know not to lock it*/
				
/*
 * Procedure:     main
 *
 * Restrictions:
                 getopt: None
                 ddb_check: None
                 ddb_errmsg: None
                 err_report: None
                 ddb_create: None!
                 lock_ddb: None!
                 unlock_ddb: None
                 getaliasmap: None
                 adddevrec: None!
                 moddevrec: None
                 remdevrec: None
                 rmdevattrs: None
                 appattrlist: None
                 remattrlist: None
*/

main(argc, argv)
	int	argc;			/* Argument count */
	char	*argv[];		/* Argument list */
{
	/* Automatic data */
	char		*alias;		/* <alias> on command-line       */
	char		*device;	/* <device> on command-line      */
	char		*cmdnm;		/* command name                  */
	char		*value;         /* value of attr. multiply defined */
	char		**save;		/* contain copy of attributes */
	char 		*attr;		/* attribute multiply defined, if any */
	int		noerr;		/* FLAG, TRUE if all's well */
	int		optchar;	/* Option extracted */
	int		a_on,m_on,d_on, /* TRUE if opt seen on cmd-line    */
			p_on,r_on;
	int		ddb_state;	/* state of DDB files: present(1)  */
					/* corrupted(0), not present(-1)   */

	int 		ret = SUCCESS, i;
	char 		*cp;
	
	/* save command name */
	cmdnm = cp = argv[0];
	if ((cp = strrchr(cp, '/')) != NULL) {
		cmdnm = ++cp;
	}

	/* Extract arguments - validate usage */
	noerr = TRUE;
	a_on = m_on = d_on = p_on = r_on = FALSE;
	opterr = FALSE;
	while ((optchar=getopt(argc, argv, "a:d:m:p:r:")) != EOF) {
	    switch (optchar) {
		case 'a':
		    if (!(a_on || m_on || d_on || p_on || r_on)) {
			a_on = TRUE;
			alias = optarg;
		    } else noerr = FALSE;
		    break;
		case 'd':
		    if (!(a_on || m_on || d_on || p_on || r_on)) {
			d_on = TRUE;
			device = optarg;
		    } else noerr = FALSE;
		    break;
		case 'm':
		    if (!(a_on || m_on || d_on || p_on || r_on)) {
			m_on = TRUE;
			device = optarg;
		    } else noerr = FALSE;
		    break;
		case 'p':
		    if (!(a_on || m_on || d_on || p_on || r_on)) {
			p_on = TRUE;
			device = optarg;
		    } else noerr = FALSE;
		    break;
		case 'r':
		    if (!(a_on || m_on || d_on || p_on || r_on)) {
			r_on = TRUE;
			device = optarg;
		    } else noerr = FALSE;
		    break;
		case '?':
		default:
		    noerr = FALSE;
		    break;
	    }	/* end switch */
	}	/* end while */

	/* Write a usage message if we've seen a blatant error */
	if (!(a_on || m_on || d_on || p_on || r_on) || !noerr) {
	    err_usage(cmdnm);
	}

	/* Set up */
	nattrs = argc - optind;

	/* lock DDB  before using it 
	 * Call unlock_ddb() before exiting putdev(1M)
	 * except when returning from the libadm's functions that modify
	 * the DDB (adddevrec, moddevrec, ...)
	 */
	if (lock_ddb() == FAILURE) {
		switch (errno) {
		case (EACCES):
		case (EAGAIN):
		    /* error, cannot lock DDB */
		    ddb_errmsg(SEV_ERROR, EX_OTHER, E_AGAIN);
		    err_report(cmdnm, ACT_QUIT);
		    break;
		default:
		    /* DDB files could not be accessed */
		    ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		    err_report(cmdnm, ACT_QUIT);
		    break;
		}
	}
	ddblock = TRUE;


	/* Check if Device Database exists?      */
	if ((ddb_state=ddb_check()) < 0) {
	    if (errno) {
		/* Not all the DDB files present/accessible.    */
		ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
		err_report(cmdnm, ACT_QUIT);
	    } else {
		/* It could be the first time a device is added. Create
		 * the files now since a lock is set on the DDB file
		 * later on */
		if (a_on) {
			if (ddb_create() < 0) 
				err_report(cmdnm, ACT_QUIT);
		} else {
			ddb_errmsg(SEV_ERROR, EX_ACCESS, E_ACCESS);
			err_report(cmdnm, ACT_QUIT);
		}
	    }
	} else if(ddb_state==0) {
	   /* DDB files corrupted - magic-nos do not match */
	   ddb_errmsg(SEV_ERROR, EX_CONSTY, E_CONSTY);
	   err_report(cmdnm, ACT_QUIT);
	}

	/* For 4.0 compatibility and options -a and -m
	 *  1. check that the alias is valid,
	 *  2. check that alias in not used as attribute
	 *  3. check that no attributes are repeated in the command line
	 */

	if (a_on || m_on) {

		if (a_on) {
			if(!valid_alias(alias)) {
				ddb_errmsg(SEV_ERROR,EX_ERROR,E_DINVAL,alias);
	    			err_report(cmdnm, ACT_QUIT);
			}
	   	} else {
			if(!(valid_alias(device)||valid_path(device))){
				ddb_errmsg(SEV_ERROR,EX_ERROR,E_DINVAL,device);
	    			err_report(cmdnm, ACT_QUIT);
			}
		}

	   	/* Checking that attributes are not repeated */
		/* save the arguments  */
		save = (char **) malloc(sizeof(char **) * (nattrs + 1));
		if (save == (char **)NULL) {
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOMEM);
	    		err_report(cmdnm, ACT_QUIT);
		}
			
		for (i=0; i <= nattrs; i++) {
			if (i < nattrs) {
				save[i] = (char *)malloc(strlen(argv[optind + i]) + 1);
				if (save[i] == (char *)NULL) {
					ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOMEM);
	    				err_report(cmdnm, ACT_QUIT);
				}
				strcpy(save[i], argv[optind + i]);
			}
			else {
				save[i] = (char *)malloc(sizeof(char *));

				/* last element points to NULL*/
				save[i] = (char *)NULL;
			}
		}
		ret = check_attrs(save,&attr,&value);
		free(save);

		switch(ret) {
		case (FAILURE):
			ddb_errmsg(SEV_ERROR,EX_ERROR,E_MULTDEFS);
		        err_report(cmdnm, ACT_QUIT);
			break;
		case (NOMEM):
			ddb_errmsg(SEV_ERROR, EX_ERROR, E_NOMEM);
	    		err_report(cmdnm, ACT_QUIT);
			break;
		case (OTHER):
			err_usage(cmdnm);
			break;
		}
	}

	/* ignore all the signals */

	for (i = 1; i < NSIG; i++)
		if (i != SIGSEGV )
			(void) sigset(i, SIG_IGN);

 	/* Clear the errno. It is set because SIGKILL can not be ignored. */

	errno = 0;

	/*
	/*  putdev -a alias [attr=value [...]] */
	if (a_on) {

	    /* Check usage */
	    /* args to getaliasmap() */
	    /* declared at block level because they are not really
	     * needed outside this block */
	    char secdev[DDB_MAXALIAS];	
	    int  mapcnt, atype;

	    if (nattrs < 0) {
		unlock_ddb();
		err_usage(cmdnm);
	    } else {

		/* For 4.0 compatibility: 
		 * check that the alias is not defined in the DDB by
		 * calling getaliasmap which returns 0 if the alias
		 * is found in the DDB. 
		 * If alias is defined in DDB, exit with status 3.
		 */
		if (getaliasmap(alias,&atype, &mapcnt, secdev) == TRUE) {
			unlock_ddb();
			ddb_errmsg(SEV_ERROR,EX_AEXIST,E_AEXIST,alias);
		    	err_report(cmdnm, ACT_QUIT);
		}

		/* Attempt to add the new alias */
		ret = adddevrec(alias, &argv[optind], ddblock);
		switch(ret) {
			case(FAILURE):
		    		/* Attempt failed.  Report error message. 
		     		 * No need to unlock DDB since adddevrec()
		     		 * calls unlock_ddb() before returning */
		    		err_report(cmdnm, ACT_QUIT);
				break;
			case(WARNING):
				exit(EX_OTHER);
				break;
		}
	    }
	    /* End -a case */

	} else if (m_on) {
	    /* putdev -m device attr=value [...] */

	    /* Check usage */
	    if (nattrs < 0) {
		unlock_ddb();
		err_usage(cmdnm);
	    } else {

		/* Attempt to modify the definition for device */
		ret  = moddevrec(device, &argv[optind], ddblock);
		switch(ret) {
			case FAILURE:
		    		/* Attempt failed.  Report error message 
		     	 	 * No need to unlock DDB since moddevrec()
		     	 	 * calls unlock_ddb() before returning */
		    		err_report(cmdnm, ACT_QUIT);
				break;
			case WARNING:
				exit(EX_OTHER);
		    		break;
		}
	    }
	    /* End -m case */

	} else if (d_on) {
	    /* putdev -d device [attr [...]] */

	    /* Check usage */
	    if (nattrs < 0) {
		unlock_ddb();
		err_usage(cmdnm);
	    } else {

		if (nattrs == 0) {
			/* putdev -d device */
		    	/* Attempt failed.  Report error message 
		     	 * No need to unlock DDB since remdevrec()
		     	 * calls unlock_ddb() before returning */
			if ((remdevrec(device, ddblock)) < 0) {
			    /* Attempt failed.  Report error message */
			    err_report(cmdnm, ACT_QUIT);
			}
		} else {
		    /* putdev -d device attr [attr [...]] */
		    ret = rmdevattrs(device, &argv[optind], ddblock);
		    if (ret == FAILURE) {
			 /* Attempt failed*/
			 err_report(cmdnm, ACT_QUIT);
		    } else if (ret == NOATTR)
			exit(EX_NOATTR);

		}   /* End "putdev -d device attr [...]" case */
	    }
	    /* End -d case */
	} else if(p_on) {
	    /* putdev -p device attribute=value,.... */
	    if (nattrs != 1) {
		unlock_ddb();
		err_usage(cmdnm);
	    } else {
		if(appattrlist(device, argv[optind], ddblock) < 0) {
		    err_report(cmdnm, ACT_QUIT);
		}
	    }
	} else if(r_on) {
	    /* putdev -r device attribute=value,.... */
	    if (nattrs != 1) {
		unlock_ddb();
		err_usage(cmdnm);
	    } else {
		if(remattrlist(device, argv[optind], ddblock) < 0) {
		    err_report(cmdnm, ACT_QUIT);
		}
	    }
	}

	/* Completed successfuly */
	exit(0);
#ifdef	lint
	return(0);
#endif
}  /* main() */

/*
 * Procedure:     err_usage
 *
 * Restrictions:
                 fprintf: None
 Notes: Functions displays usage error, and exits.
*/

static void
err_usage(cmdnm)
char	*cmdnm;
{
	/* display error and quit, exit(EX_USAGE) */
	fprintf(stderr,"UX:putdev:ERROR:%s",E_USAGE1);
	fprintf(stderr,"%s",E_USAGE2);
	exit(EX_USAGE);
}

/* For 4.0 compatibility: 
 * check_attr verifies that no attribute is repeated
 * in the command line
 * 	Returns 
 *		-1  if it finds an attribute repeated in the 
 *	 	    list of attributes given in the command line.
 *
 *		 0  if no attribute is repeated in the command line.
 *
 *		 1  If there is no sufficient memory to allocate the 
 *		    for the temporary arrays needed
 *
 *		 2  Error encountered. Issue usage message
 */
static int
check_attrs(attrlist, attr, value)
char *attrlist[];
char **attr, **value;
{
	char **oam_list;	/* list of oam attributes */
	char *oam_attr;		/* points to attribute added to oam_list */
	char *val;
	int atype, item;
	int i = 0, j = 0;
	int next = 0; 	/* next available entry on oam_list */
	int found = 0; 	/* if found ==1, the oa&m is repeated in the command */
	int err = 0;

	/* at the most all the attributes will be O&AM */
	oam_list = (char **)malloc(sizeof(char **) * (nattrs + 1));
	if (oam_list == (char **)NULL)
		return(1);

	while( attrlist[i] != (char *)NULL && !err)
	{

		*attr = (char * )getfield(attrlist[i], "=", value);
		if (*attr == (char *)NULL) {
			free(oam_list);
			return(OTHER);
		}

		atype = getattrtype(*attr, &item);
		switch(atype){
		case (TYPE_SEC):
			if (attr_flag.sec[item])
				err++;
			else
				attr_flag.sec[item] = 1;
			break;

		case (TYPE_DSF):
			if (attr_flag.dsf[item])
				err++;
			else
				attr_flag.dsf[item] = 1;
			break;
	
		case (TYPE_TAB):
			/* if item == -1, then it is an oam attribute */
			if (item != -1)
			{
				if (attr_flag.tab[item])
					err++;
				else
					attr_flag.tab[item] = 1;
			}
			/* must be a new OA&M attribute */
			/* check if attr defined in attrlist */
			else {
				j = 0;
				while (j < next && !found) {
					if (strcmp(*attr,oam_list[j]) == 0)
						found = 1;
					j++;
				}
				if (found)
					err++;
				else {
					oam_attr = malloc(strlen(*attr) + 1);
					if (oam_attr == (char *)NULL)
	    				 	return(1);
					strcpy(oam_attr,*attr);
					oam_list[next++] = oam_attr;
				}
			}
			break;

		default:
			err++;
			break;
		}
		++i;
	}
	free(oam_list);
	if (err)
		return(FAILURE);
	return(SUCCESS);
}
