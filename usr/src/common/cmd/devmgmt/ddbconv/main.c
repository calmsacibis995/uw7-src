/*		copyright	"%c%" 	*/


#ident	"@(#)devmgmt:common/cmd/devmgmt/ddbconv/main.c	1.3.9.3"
#ident  "$Header$"

/*
 * main.c
 *
 *	Implements the "ddbconv" command.
 */

/*
 *  G L O B A L   R E F E R E N C E S
 */

/*
 * Header files needed:
 *	<sys/types.h>	Standard UNIX(r) types
 *	<sys/stat.h>	Standard UNIX(r) types
 *	<stdio.h>	Standard I/O definitions
 *	<string.h>	String handling definitions
 *	<ctype.h>	Character types & macros
 *	<stdlib.h>	Storage alloc functions
 *	<errno.h>	Error codes
 *	<unistd.h>	Standard Identifiers
 *	<fmtmsg.h>	Standard Message Generation definitions
 *	<mac.h>		MAC definitions
 *	<devmgmt.h>	Device Management definitions
 */

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<unistd.h>
#include	<fmtmsg.h>
#include	<mac.h>
#include	<devmgmt.h>



/*
 * Device Management Globals referenced:
 *	moddevrec()		Modify a device definition in Device Database
 *	rmsecdevs()		Remove secdev attr from DDB_TAB & DDB_DSFMAP
 *	read_ddbrec()		Reads record from specified DDB file
 *	getfield()		Gets ptr to next field separated by delimiter
 *	getquoted()		Gets string enclosed in ""
 *	ddb_errget()		Gets error code set by libadm functions
 *	ddb_errmsg()		Saves error message in buffer
 *	err_report()		Writes error message on stderr
 */
#if defined(__STDC__)
int	moddevrec(char *, char **, int);
int	rmsecdevs();
char	*read_ddbrec(FILE *);
char	*getfield(char *, char *, char **);
char	*skpfield(char *, char *);
char	*getquoted(char *, char **);
int	ddb_errget();
void	err_report(char *, int);
#else
int	moddevrec();
int	rmsecdevs();
char	*read_ddbrec();
char	*getfield();
char	*skpfield();
char	*getquoted();
int	ddb_errget();
void	ddb_errmsg();
void	err_report();
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

#ifndef	DEV_SECDEV
#define	DEV_SECDEV	3
#endif

#ifndef	DEV_SECALL
#define	DEV_SECALL	4
#endif

#define SUCCESS		0
#define FAILURE		-1

/* Returned by chk_tab_dsf */
#define NO_TAB		1	/* No device.tab file found  */
#define NO_DSFMAP	2	/* No ddb_dsfmap file found  */
#define	NO_MATCH	3	/* Magic numbers don't match */

#define DDB_UMASK	0117	        /* rw-rw---- perm on DDB_SEC */

#define OLD_TAB	"/etc/old.tab"		/* OAM device.tab renamed as OLD_TAB*/

#define DDBMAGICLEN	80
/*
 *  Local, Static data
 */

typedef struct attrent {
	struct attrent	*next;	/* ptr to next list item      */
	char		*attrval; /* attr-value string        */
} attr_ent;

typedef struct {
	attr_ent	*head;	/* head of attr-value list*/
	int		count;	/* no. of attrs in list   */
} attr_list;

typedef struct devent {
	struct devent	*next;	/* ptr to next device in list */
	char		*alias;	/* device alias               */
	char		*typev;	/* value of attr "type"       */
	int		chg;	/* attrs to be changed        */
} dev_ent;

typedef struct {
	dev_ent		*head;	/* head of device list    */
	int		count;	/* no. of devices in list */
} dev_list;

typedef struct {
	char	*tval;		/* "type" attr-value      */
	char	*hilevel;	/* "range" hilevel value  */
	char	*lolevel;	/* "range" lolevel value  */
	char	*state;		/* "state" value          */
} def_attrs;

/*
 * dev_attrs[] => array that contains DEFAULT SECURITY ATTRS
 *	for different "type" of devices recognized by ddbconv.
 *	If "type" in DDB_TAB, is not one of those defined in dev_attrs[],
 *	the default security attr-values chosen are --
 *		range="SYS_PRIVATE-SYS_PUBLIC" state=DDB_PRIVATE.
 *
 * Default Security attribute values used by ddbconv -s.
 * The values for attrs RANGE and STATE of a device alias
 * are selected from this array, based on the value of TYPE attr.
 * 	For example, for a device alias which defined type="tty",
 * 	the default security attrs range="SYS_RANGE_MAX-USER_LOGIN"
 * 	and state=DDB_PUB_PRIV.
 * This command assumes that the level names used below, are already
 * defined in the LTDB. If they are not defined, DDBCONV will
 * fail. Any changes to the default RANGE and STATE attr-values must 
 * be made to this dev_attrs[] array. THIS IS ONLY A QUICK-FIX.
 *
 * FUTURE ENHANCEMENT: An option could be added to ddbconv, to get
 * these default attr-values from a external data file.
 */
#define DEF_LEVELS	7		/* no. of def. levels          */
#define DEF_SECATTRS	7		/* no. of def. security attrs  */
#define DEF_DEVTYPE	7		/* no. of def. device types    */

static char *def_level[]={	SYS_RANGE_MIN,
				SYS_PUBLIC,
				SYS_PRIVATE,
				SYS_RANGE_MAX,
				USER_PUBLIC,
				USER_LOGIN,
				SYS_AUDIT };
				
static def_attrs dev_attrs[DEF_DEVTYPE]= {
	{"cons", SYS_RANGE_MAX, SYS_RANGE_MIN, DDB_PUB_PRIV},
	{"tty", SYS_RANGE_MAX, USER_LOGIN, DDB_PUB_PRIV},
	{"disk", SYS_PRIVATE, SYS_PUBLIC, DDB_PRIVATE},
	{"dpart", SYS_RANGE_MAX, SYS_RANGE_MIN, DDB_PRIVATE},
	{"diskette", SYS_RANGE_MAX, SYS_RANGE_MIN, DDB_PRIVATE},
	{"ctape", SYS_RANGE_MAX, SYS_RANGE_MIN, DDB_PUB_PRIV},
	{"qtape", SYS_RANGE_MAX, SYS_RANGE_MIN, DDB_PUB_PRIV} 
};


/*
 * Local Static functions:
 */
static char		msgbuf[64];	/* error message buffer */
static void err_usage();
static void freeattrlist();
static void freedevlist();
static attr_ent *make_attrent();
static char **make_tabattrs();
static char *typeval();
static int getaliaslist();
static char **bldsecattrs();
static int chk_tab_dsf();

/*
 * Error messages and exit codes:
 */
#define E_USAGE1	"syntax incorrect, invalid options\n"
#define	E_USAGE2	"USAGE: ddbconv [-v]\n\
USAGE: ddbconv [-v] -s\n\
USAGE: ddbconv [-v] -u\n"
#define E_ACCESS	"Device Database could not be accessed or created\n"
#define E_CONSTY	"Device Database in inconsistent state - notify administrator\n"
#define E_UPGRAD	"Must execute 'ddbconv [-v]' first, before 'ddbconv [-v] -s'\n"
#define E_DNGRAD	"Must execute 'ddbconv -u [-v]' on system without Enhanced Security installed\n"
#define E_LEVEL		"Device Database not converted; invalid level='%s'\n"
#define E_NOPKG		"system service not installed.\n"
#define E_NOMEM		"insufficient memory\n"

/*
 * Verbose messages displayed
 */
#define BASE_MSG	"Defined attributes in new format for following aliases:\n"
#define ENH_MSG		"Created security attrs and secdev for following aliases:\n"
#define UNENH_MSG	"Removed security attrs and secdev for following aliases:\n"
#define EX_USAGE	1

#define SEV_ERROR	1
#define ACT_QUIT	1

/*
 * Internal Static routines:
 */
static void err_usage();

/*
 * COMMAND: ddbconv
 *
 * ddbconv [-v]
 * ddbconv [-v] -s
 * ddbconv [-v] -u
 *
 * 	This command converts the OA&M Device Table of SVR4.0, into the
 *	Device Database files on the SVR4ES base and enhanced security
 *	packages.
 *
 * Options:
 *	none	coverts the OA&M device table in /etc/device.tab into
 *		2 files- /etc/device.tab & /etc/security/ddb/ddb_dsfmap
 *		in the format recognized by the SVR4ES base package.
 *	-s	upgrades the Device Database from SVR4ES base package,
 *		to SVR4ES enhanced security package. It maps every
 *		Device <alias> defined in the database, to a <secdev>
 *		with the same alias name. It creates default security
 *		attributes using the values defined in dev_attrs[] array, 
 *		for each <secdev> alias created. It also 
 *		maps every dsf in /etc/security/ddb/ddb_dsfmap to the
 *		same <secdev> alias created.
 * 
 *	-u	used to remove all the security attributes defined
 *		for each device alias. It also removes the
 *		<secdev> attribute defined for each device alias,
 *		defined in DDB_TAB & DDB_SEC files.
 *
 *	-v	it displays the names of device aliases, whose
 *		device attributes were modified.
 * Exit values:
 *	Set by the extrenal function err_report().
 */

main(argc, argv)
	int	argc;			/* Argument count */
	char	*argv[];		/* Argument list */
{
	/* Automatic data */
	FILE		*tabfp,		/* file ptr to DDB_TAB file      */
			*dsffp,		/* file ptr to DDB_DSFMAP file   */
			*secfp;		/* file ptr to DDB_SEC file      */
	FILE		*newtabfp;	/* file prt to DDB_TAB. 	 */
	struct stat	sbuf;		/* file stat structure           */
	mode_t		old_umask;	/* old umask - saved here        */
	char		*alias;		/* device alias                  */
	char		*typeval;	/* value of attr "type"          */
	char		*secdev;	/* secure device alias           */
	char		*tabrec,	/* record from DDB_TAB file      */
			*secrec;	/* record from DDB_SEC file      */
	char		**tabattrs;	/* OA&M attr-value list          */
	char		**secattrs;	/* security attr-value list      */
	char		*cmdnm;		/* command name                  */
	char		*p;		/* Temp ptr to char              */
	char 		buf[DDBMAGICLEN]; /* Where file's first line is 
					 * stored. The value is not used */	
	int		cmp;		/* result of string compare      */
	int		err;		/* FLAG, FALSE if all's well     */
	int		optchar;	/* Option extracted              */
	int		v_on,s_on,u_on; /* TRUE if opt seen on cmd-line  */
	int		ddblock = FALSE; 
	dev_ent		*dev;		/* device list entry             */
	dev_list	devlist;	/* device alias list             */
	level_t		level;		/* temp level                    */
	char		errmsg[64];	/* error message buffer          */
	register int	cnt, readnext, i, ddb_state;
	unsigned long	magic, secmagic;
	char		magic_tok[10];

	/* save command name */
	cmdnm = argv[0];

	/* Extract arguments - validate usage */
	err = FALSE;
	v_on = s_on = u_on = FALSE;
	opterr = FALSE;
	while ((!err)&&((optchar=getopt(argc, argv, "suv")) != EOF)) {
	    switch (optchar) {
		case 's':
		    if (!(s_on || u_on)) {
			s_on = TRUE;
		    } else err = TRUE;
		    break;
		case 'u':
		    if (!(u_on || s_on)) {
			u_on = TRUE;
		    } else err = TRUE;
		    break;
		case 'v':
		    if (!v_on) {
			v_on = TRUE;
		    } else err = TRUE;
		    break;
		case '?':
		default:
		    err = TRUE;
		    break;
	    }	/* end switch */
	}	/* end while */

	/* Write a usage message if we've seen a blatant error */
	if(err || argc > optind ) {
	    err_usage(cmdnm);
	}


	/* if -s|-u NOT specified, build DDB for SVR4.0ES BASE system */
	if (!(s_on|u_on)) {

	    /* check if DDB_DSFMAP has been created already by ddbconv */
	    if(stat(DDB_DSFMAP, &sbuf) == 0) {
		/* Yes, then exit normally with no action!! *
		 * ddbconv is assumed to have run already.  */
		exit(0);
	    }

	    if (v_on) {
		printf("%s", BASE_MSG);
	    }

	    /* move SVR4.0 Device Table to Old DDB_TAB file */
	    if(rename(DDB_TAB, OLD_TAB)<0) {
		    ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    err_report(cmdnm, ACT_QUIT);
	    }

	    /* create new DDB_TAB & DDB_DSFMAP files */
	    if (ddb_create()<0) {
		err_report(cmdnm, ACT_QUIT);
	    }

	    /* create new entries for each device alias *
	     * defined in the SVR4.0 Device Table       */
	    if (tabfp=fopen(OLD_TAB,"r")) {

		while (tabrec=read_ddbrec(tabfp)) {

			/* If comment or empty line,
			 * copy it to the new DDB_TAB 
			 */
			if (*tabrec == '#' || isspace(*tabrec)) {

				/* No locking needed because this command
				 * should be executed while installing the 
				 * ES package.
				 */
				newtabfp = fopen(DDB_TAB,"a+");
				if (newtabfp == (FILE *)NULL){
					free(tabrec);
					fclose(tabfp);
		    			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    			err_report(cmdnm, ACT_QUIT);
	    			}

				if (fseek(newtabfp,0L,SEEK_END) == -1) {
					free(tabrec);
					fclose(tabfp);
		    			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    			err_report(cmdnm, ACT_QUIT);
	    			}

				if (write_ddbrec(newtabfp,tabrec) < 0) {
					/* error trying to write */
					free(tabrec);
					fclose(tabfp);
		    			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    			err_report(cmdnm, ACT_QUIT);
	    			}
				fclose(newtabfp);
				free(tabrec);
				continue;
			}
			
		    	/* convert record into attr-value list */
		    	if((tabattrs=make_tabattrs(tabrec))== NULL) {
				err_report(cmdnm, ACT_QUIT);
		    	}

			/* extract device alias name */
			alias = *tabattrs;
	
			/* -v option, then print alias names */
			if (v_on) {
				printf("%s\n", alias);
			}

			tabattrs++;

			/* add attr-values to new Device Database */
			if (adddevrec(alias, tabattrs) < 0) {
		    		err_report(cmdnm, ACT_QUIT);
			}

		}	/* end while */

		if (ddb_errget()) {
		    ddb_errmsg(SEV_ERROR, 4, E_NOMEM);
		    err_report(cmdnm, ACT_QUIT);
		}

		fclose(tabfp);
	    }

	} else if (s_on) {

	     /* Check if device.tab and ddb_dsfmap exists
	      * and have the same magicno */
	      ddb_state=chk_tab_dsf(&magic) ;

	      /* ddb_state == 0 means SUCCESS */
	      if (ddb_state != 0)
	      	switch(ddb_state) {
	      		case NO_TAB:
	        		/* DDB_TAB  file is not present/accessible */
	    			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
	    			err_report(cmdnm, ACT_QUIT);
				break;
			case NO_DSFMAP:
				/* DDB_DSFMAP file is not present/accessible
			 	* must upgrade DDB files to new format */
				ddb_errmsg(SEV_ERROR, 4, E_UPGRAD);
				err_report(cmdnm, ACT_QUIT);
				break;
	      		case NO_MATCH:
	   			/* magic-nos do not match */
	    			ddb_errmsg(SEV_ERROR, 4, E_CONSTY);
	    			err_report(cmdnm, ACT_QUIT);
				break;
			default:
				/* FAILURE is returned which is the 
				 * case when an error occurred when openning
				 * the ddb file
				 */
	    			ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
	    			err_report(cmdnm, ACT_QUIT);
				break;
	      	}

	    /* convert DDB from 4.0ES BASE to Enhanced Security */
	    /* check if enhanced security package is installed */
	    if(!_mac_installed()) {
		ddb_errmsg(SEV_ERROR, 3, E_NOPKG);
		err_report(cmdnm, ACT_QUIT);
	    }

	    /* check if default levels defined and valid */
	    for (i=0 ; i<DEF_LEVELS ; i++) {
		if (lvlin(def_level[i], &level)<0) {
		    /* level undefined/invalid; exit without converting DDB */
		    ddb_errmsg(SEV_ERROR, 4, E_LEVEL, def_level[i]);
		    err_report(cmdnm, ACT_QUIT);
		}
	    }

	    /* get list of aliases from DDB_TAB file */
	    if (getaliaslist(&devlist)==FAILURE) {
		err_report(cmdnm, ACT_QUIT);
	    }

	    /* check if previously created DDB_SEC file exists */
	    if (secfp=fopen(DDB_SEC,"r")) {

		/* skip the  magic number
		if (fgets(buf,DDBMAGICLEN,secfp) == (char *)NULL) {
		    ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    err_report(cmdnm, ACT_QUIT);
		}

		/* check the magic number in ddb_sec with the one
		 * on ddb_dsfmap and device.tab, obtained from chk_tab_dsf()
		 */
		getmagicno(secfp, magic_tok, &secmagic);
		if (magic != secmagic) {
	    		ddb_errmsg(SEV_ERROR, 4, E_CONSTY);
	    		err_report(cmdnm, ACT_QUIT);
		}

	        if (v_on) {
		    printf("%s", ENH_MSG);
	        }
		/* retain sec. attrs defined in old DDB_SEC file *
		 * for every alias defined in DDB_TAB file       */
		cnt = devlist.count;
		readnext = TRUE;
		for (dev=devlist.head; cnt > 0 ; cnt--) {
		    /* extract next alias name and type from devlist */
		    alias = dev->alias;

		    /* read next old DDB_SEC record */
		    while (readnext) {
			if (secrec=read_ddbrec(secfp)) {
			    /* extract secdev alias from old DDB_SEC record */
			    secdev = getfield(secrec,":",&secrec);

			    /* if alias > secdev; ignore record in DDB_SEC *
			     * Secdev not present in new DDB_TAB file      */
			    if (alias && secdev && ((cmp=strcmp(alias,secdev))<=0)) {
				/* alias <= secdev; build sec. attrs       *
				 * Alias is new, or sec. attrs defined.    */
				break;
			    }
			} else if (ddb_errget()) {
			    /* error in reading DDB_SEC record */
			    ddb_errmsg(SEV_ERROR, 4, E_NOMEM);
			    err_report(cmdnm, ACT_QUIT);
			} else {
			    /* no more records; break out of loop */
			    readnext = FALSE;
			}
		    }
		    if (secrec) {
			/* alias <= secdev; DDB_SEC contains more records */
			if (alias && secdev && (strcmp(alias,secdev)==0)) {
			   /* alias=secdev; security attrs defined in DDB_SEC*
			    * Only secdev attr to be defined in DDB_TAB.     */
			   dev->chg = DEV_SECDEV;
			   readnext = TRUE;
			} else {
			   /* alias<secdev; sec. attrs not defined in DDB_SEC*
			    * Essential sec. attrs & secdev to be defined    */
			   dev->chg = DEV_SECALL;
			   readnext = FALSE;
			}
		    } else {
			/* if no more secdevs in DDB_SEC :     *
			 * Rest of aliases in devlist are new. */
			dev->chg = DEV_SECALL;
		    }
		    /* go to next device alias */
		    dev = dev->next;
		}	/* end for */

		/* Build security attrs for each device alias in devlist *
		 * based on dev->typev & dev->chg using bldsecattrs().   *
		 * Modify existing device alias, by defining these attrs.*/
		cnt = devlist.count;
		for (dev=devlist.head; cnt>0 ; cnt--) {
		    /* -v option, then print alias names for which *
		     * the default security attrs are being defined*/
		    if ((v_on)&&(dev->chg==DEV_SECALL)) {
			printf("%s\n", dev->alias);
		    }
		    if((secattrs=bldsecattrs(dev->alias, dev->typev, dev->chg))
					== (char **)NULL) {
			ddb_errmsg(SEV_ERROR, 4, E_NOMEM);
			      err_report(cmdnm, ACT_QUIT);
		    }
		    if(moddevrec(dev->alias, secattrs, ddblock) < 0) {
			err_report(cmdnm, ACT_QUIT);
		    }
		    /* next device alias */
		    dev = dev->next;
		}	/* end for */
	    } else {
	    
		/* DDB_SEC not present; create new DDB_SEC file. */
		cr_magicno();
		old_umask = umask((mode_t)DDB_UMASK);

		if ((stat(DDB_TAB, &sbuf)) == -1) {
		    (void)umask(old_umask);
		    ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    err_report(cmdnm, ACT_QUIT);
		}

		if ((secfp=fopen(DDB_SEC,"w")) != (FILE *)NULL) {

		    setmagicno(secfp);
		    fclose(secfp);
		    (void)umask(old_umask);

		    /* Set the uid, gid, and level of the new DDB_SEC file
		     * using DDB_TAB as "model".
		     */
		    if ((setddbinfo(DDB_SEC, sbuf)) == FAILURE) {
		    	(void)umask(old_umask);
		    	err_report(cmdnm, ACT_QUIT);
		    }

		} else {
			/* fopen of DDB_SEC failed */
			(void)umask(old_umask);
		    	ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		    	err_report(cmdnm, ACT_QUIT);
		}

	        if (v_on) {
		    printf("%s", ENH_MSG);
	        }
		/* Create sec attrs for each alias in DDB_TAB file*/
		cnt = devlist.count;
		for (dev=devlist.head; cnt>0 ; cnt--) {

		    alias = dev->alias;
		    typeval = dev->typev;

		    /* -v option, then print alias names */
		    if (v_on) {
			printf("%s\n", alias);
		    }

		    if((secattrs=bldsecattrs(alias, typeval, DEV_SECALL))==
							(char **)NULL) {
		      ddb_errmsg(SEV_ERROR, 4, E_NOMEM);
		      err_report(cmdnm, ACT_QUIT);
		    }

		    if(moddevrec(alias, secattrs, ddblock) < 0) {
			err_report(cmdnm, ACT_QUIT);
		    }
		    /* next device alias */
		    dev = dev->next;
		}
	    }

	} else if (u_on) {

	    /* convert DDB from 4.0ES Enhanced Security to 4.0ES BASE */
	    /* check if enhanced security package is running */
	    if(mac_running()) {
		/* yes, then ddbconv -u must NOT be executed */
		ddb_errmsg(SEV_ERROR, 4, E_DNGRAD);
		err_report(cmdnm, ACT_QUIT);
	    }

	    /* -v option, then print alias names */
	    if (v_on) {
		printf("%s", UNENH_MSG);
	    }

	    /* remove all security attributes for all aliases defined */
	    if (rmsecdevs(v_on)<0) {
		err_report(cmdnm, ACT_QUIT);
	    }

	} else {
	    err_usage(cmdnm);
	}
	/* Completed successfuly */
	exit(0);
#ifdef	lint
	return(0);
#endif
}  /* main() */



#define QUIT_ATTRLIST(list)	{\
			freeattrlist(list);\
			return((char **)NULL);\
			}
/*
 * char **make_tabattrs(rec);
 * char 	*rec;
 *
 *	Makes attr-value strings from the <rec>, which is assumed
 *	to be in the OAM Device Table (SVR4.0) format. It returns
 *	a list of pointers to the attr-value strings.
 */
char **
make_tabattrs(rec)
char	*rec;
{
	attr_ent	*ent, *curr;	/* attr-value entry     */
	char		*attr;		/* attr name            */
	char		*value;		/* value buffer ptr     */
	char		*recptr;	/* Temp ptr to rec      */
	char		**rtnlist;	/* Ptr to alloc'd list  */
	int		i;		/* Temp counter         */
	attr_list list = {(attr_ent *)NULL, 0}; /* attr list    */
				

	/*  
	 *  Accumulate all the attributes defined for the device
	 *  in an attribute list <rtnlist>.
	 */

	recptr = rec;

	/* attributes defined in /etc/device.tab record */

	/* extract device alias name */
	if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
	    ent->attrval = getfield(recptr,":",&recptr);
	    ent->next = (attr_ent *)NULL;
	    /* link new entry & increment count */
	    list.head = ent;
	    list.count++;
	} else QUIT_ATTRLIST(&list);
	
	/* extract cdevice value */
	if (value=getfield(recptr,":",&recptr)) {
	    if(*value) {
		if (curr=make_attrent("cdevice", value)) {
		    ent->next = curr;
		    ent = curr;
		    list.count++;
		} else {
		    QUIT_ATTRLIST(&list);
		}
	    }
	}

	/* extract bdevice value */
	if (value=getfield(recptr,":",&recptr)) {
	    if(*value) {
		if (curr=make_attrent("bdevice", value)) {
		    ent->next = curr;
		    ent = curr;
		    list.count++;
		} else {
		    QUIT_ATTRLIST(&list);
		}
	    }
	}

	/* extract pathname value */
	if (value=getfield(recptr,":",&recptr)) {
	    if(*value) {
		if (curr=make_attrent("pathname", value)) {
		    ent->next = curr;
		    ent = curr;
		    list.count++;
		} else
		    QUIT_ATTRLIST(&list);
	    }
	}
		
	/*  Other attributes, if any  */
	while ((*recptr)&&(*recptr!='\n')) {
	    if ((attr=getfield(recptr,"=",&recptr)) &&
		(value=getquoted(recptr,&recptr))) {
		if (curr=make_attrent(attr,value)) {
		    ent->next = curr;
		    ent = curr;
		    list.count++;
		} else {
		    QUIT_ATTRLIST(&list);
		}
	    }
	} 	/* end while */
	
	/* allocate memory for pointer list */
	rtnlist = (char **)NULL;
	if (rtnlist=(char **)malloc((list.count+1)*sizeof(char *))) {
	    ent = list.head;	
	    /* copy attrs from namelist into output list */
	    for (i=0 ; i<list.count ; i++) {
		rtnlist[i] = ent->attrval;
		ent = ent->next;
	    }
	    rtnlist[i] = (char *)NULL;	/* end list with NULL */
	} else QUIT_ATTRLIST(&list);

	/* free memory for namelist */
	freeattrlist(&list);

	/* Fini */
	return(rtnlist);
}


/*
 * static attr_ent *
 * make_attrent(attr,value)
 *	Makes <attr_ent> structure using the input parameters
 *	<attr> & <value>. The <attr_ent> struct is initialized as follows:
 *	name_ent->attrval = '<attr>="<value>"'
 *	name_ent->next  = NULL
 */
static attr_ent *
make_attrent(attr, value)
char	*attr;		/* attribute name */
char	*value;		/* attr value     */
{
	attr_ent	*ent;		/* ptr to attr_ent struct */
	char		*valbuf;	/* value buffer           */

	if (ent=(attr_ent *)malloc(sizeof(attr_ent))) {
	    if (valbuf=(char *)malloc(strlen(attr)+strlen(value)+2)) {
		strcpy(valbuf,attr);
		strcat(valbuf,"=");
		strcat(valbuf,value); 
		/* initialize name_ent structure */
		ent->attrval = valbuf;
		ent->next = (attr_ent *)NULL;
	    } else {
		free(ent);
		return((attr_ent *)NULL);
	    }
	} else {
	    return((attr_ent *)NULL);
	}
	return(ent);
}

/*
 * char
 * *get_typeval(rec)
 * char		*rec;
 *
 *	Returns the value of attribute <type> from character string <rec>.
 *	If <type> attr is not defined, it return (char *)NULL.
 */
static char *
get_typeval(rec)
char	*rec;
{
	char	*value, *tmp, *next;

	if (tmp=strstr(rec, "type=")) {
	    tmp = skpfield(tmp,"=");
	    value = getquoted(tmp, &next);
	    return(value);
	} else {
	    return((char *)NULL);
	}
}

/*
 * int
 * getaliaslist(list)
 * dev_list	*list;
 *
 *	Returns a list of aliases defined in DDB_TAB file,
 *	and their "type" attr-value on the SVR4.0ES BASE or 
 *	ENHANCED system.
 */
static int
getaliaslist(list)
dev_list	*list;
{
	FILE		*tabfp;		/* DDB_TAB file ptr     */
	dev_ent	*ent,	*curr;		/* dev_ent entry        */
	char		*alias;		/* device alias name    */
	char		*value;		/* value buffer ptr     */
	char		*recptr;	/* Temp ptr to rec      */
	int		i;		/* Temp counter         */
	char 		buf[DDBMAGICLEN]; /* Where TAB's first line 
					   * stored. Value is ignored */

	/* open DDB_TAB to get all aliases defined */
	if ( (tabfp=fopen(DDB_TAB,"r")) == (FILE *)NULL) {
	    /* could not access DDB_TAB file */
	    ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
	    return(FAILURE);
	}

	/* skip magic number */
	if (fgets(buf,DDBMAGICLEN,tabfp) == (char *)NULL) {
		ddb_errmsg(SEV_ERROR, 4, E_ACCESS);
		fclose(tabfp);
		return(FAILURE);
	}

	/* initialize list head */
	list->head = (dev_ent *)NULL;
	list->count = 0;

	while (recptr = read_ddbrec(tabfp)) {
		/* skip comments and empty lines */
	    	if (*recptr == '#' || isspace(*recptr)) {
			free(recptr);
			continue;
	    	} else {
	    		/* extract alias name from DDB_TAB record */
	    		if (ent=(dev_ent *)malloc(sizeof(dev_ent))) {
				ent->alias=getfield(recptr,":",&recptr);
				ent->typev = get_typeval(recptr);
				ent->next = (dev_ent *)NULL;
				/* link new entry & increment count */
				list->head = ent;
				list->count++;
	     		} else {
				freedevlist(list);
				return(FAILURE);
	     		}
			break;
		}
	}

	/* read records from DDB_TAB file */
	while (recptr=read_ddbrec(tabfp)) {

	    /* skip comments and empty lines */
	    if ( *recptr == '#' || isspace(*recptr)) {
		free(recptr);
		continue;
	    }

	    /* extract next alias from DDB_TAB record */
	    alias = getfield(recptr,":",&recptr);
	    if (curr=(dev_ent *)malloc(sizeof(dev_ent))) {
		if (curr->alias=(char *)malloc(strlen(alias)+1)) {
		    strcpy(curr->alias,alias);
		    curr->typev = get_typeval(recptr);
		    curr->next = (dev_ent *)NULL;
		    list->count++;
		    /* move entry ptr to curr */
		    ent->next = curr;
		    ent = curr;
		}
	    } else {
		freedevlist(list);
		return(FAILURE);
	    }
	} 	/* end while */
	
	/* Fini */
	return(SUCCESS);
}

/*
 * static char **
 * bldsecattrs(alias, tval, chg)
 *	Builds the default security attr-value list, for specified <alias>,
 *	based on <chg>, and the value of attr "type" passed in <tval>.
 *	If chg = DEV_SECDEV, only secdev attr-val string is returned.
 *	If chg = DEV_SECALL, all default secattr-values & secdev are
 *		built based on <tval> and returned.
 *
 *	The following security attr-value strings are built --
 *		secdev = <alias>
 *		range = <default range> from dev_attrs[] based on tval.
 *		state = <default state> from dev_attrs[] based on tval.
 *		mode   = static
 *		startup= no
 *		ual_enable= yes
 *		other= >y
 */
static char **
bldsecattrs(alias, tval, chg)
char	*alias;
char	*tval;
int	chg;
{
	static char	*secdev = "secdev=%s";
	static char	*range  = "range=%s-%s";
	static char	*state  = "state=%s";
	static char	*mode   = "mode=static";
	static char	*startup= "startup=n";
	static char	*ual_enable="ual_enable=y";
	static char	*other="other=>y";
	level_t		level;
	char		devbuf[DDB_MAXALIAS];
	char		**list, **stlist;
	register int	i;

	stlist = list = (char **)NULL;
	if (chg == DEV_SECDEV) {
	    /* security attrs already exist in DDB_SEC file for <alias> *
	     * Define only the <secdev> attribute = <alias>.            */
	    /* allocate memory for pointer list */
	    if (stlist=list=(char **)malloc(2*sizeof(char *))) {
		/* build secdev=alias string */
		if (*list = (char*)malloc(strlen(secdev)+strlen(alias)+1)) {
		    sprintf (*list, secdev, alias);
		    *(++list) = (char *)NULL;
		    return(stlist);
		}
	    } else {
		return((char **)NULL);
	    }
	} else {
	    /* security attrs NOT defined in DDB_SEC for <alias>.       *
	     * Define the minimum set of default security attrs.        */
	    /* allocate memory for pointer list */
	    if (stlist=list=(char **)malloc((DEF_SECATTRS+1)*sizeof(char *))) {
		/* build secdev=alias string */
		if (*list = (char*)malloc(strlen(secdev)+strlen(alias)+1)) {
		    sprintf (*list, secdev, alias);
		    list++;
		} else
		    return((char **)NULL);
		/* build range & state based on <tval>         */
		/* get default device range for specified type *
		 * from the default secattrs[] array.          */
		i = 0;
		for ( i=0 ; i<DEF_DEVTYPE ; i++) {
		    if ((dev_attrs[i].tval) && tval && (strcmp(dev_attrs[i].tval, tval)==0)) 
			break;
		}
		if (i == DEF_DEVTYPE) {
		    /* no match on type in secattrs[] array[] */

		    /* build default device range string */
		    if (*list = (char *)malloc(strlen(range)+
				strlen(SYS_PRIVATE)+strlen(SYS_PUBLIC)+1)) {
			sprintf(*list, range, SYS_PRIVATE, SYS_PUBLIC);
			list++;
		    } else {
			return((char **)NULL);
		    }
		    /* build state=private string */
		    if (*list = (char *)malloc(strlen(state)+
					strlen(DDB_PRIVATE)+1)) {
			sprintf(*list, state, DDB_PRIVATE );
			list++;
		    } else {
			return((char **)NULL);
		    }
		} else {
		    /* type found in dev_attrs[] array   */

		    /* build default device range string */
		    if (*list = (char *)malloc(strlen(range)+
				strlen(dev_attrs[i].hilevel)+
				strlen(dev_attrs[i].lolevel)+1)) {
			sprintf(*list, range, dev_attrs[i].hilevel, 
						dev_attrs[i].lolevel);
			list++;
		    } else {
			return((char **)NULL);
		    }
		    /* build default state string */
		    if (*list = (char *)malloc(strlen(state)+
					strlen(dev_attrs[i].state)+1)) {
			sprintf(*list, state, dev_attrs[i].state );
			list++;
		    } else {
			return((char **)NULL);
		    }
		}

		/* build mode=static string */
		if (*list = (char *)malloc(strlen(mode)+1)) {
		    strcpy(*list, mode);
		    list++;
		} else
		    return((char **)NULL);
		/* build startup=n string */
		if (*list = (char *)malloc(strlen(startup)+1)) {
		    strcpy(*list, startup);
		    list++;
		} else
		    return((char **)NULL);

		/* build ual_enable=y string */
		if (*list = (char *)malloc(strlen(ual_enable)+1)) {
		    strcpy(*list, ual_enable);
		    list++;
		} else
		    return((char **)NULL);

		/* build other=>y string */
		if (*list = (char *)malloc(strlen(other)+1)) {
		    strcpy(*list, other);
		    list++;
		} else
		    return((char **)NULL);
		/* essential sec. attrs defined in list */
		/* essential sec. attrs defined in list */
		/* terminate list with NULL string ptr */
		*list = (char *)NULL;
		return(stlist);
	    } else {
		return((char **)NULL);
	    }
	}
}

/*
 * static void freeattrlist()
 *	Frees memory allocated for attr-list.
 */
static
void freeattrlist(list)
attr_list	*list;
{
	attr_ent	*next, *curr;

	if (curr=list->head) do {
	    next = curr->next;
	    free(curr);
	} while(curr=next);
}
/*
 * static void freedevlist()
 *	Frees memory allocated for dev-list.
 */
static
void freedevlist(list)
dev_list	*list;
{
	dev_ent	*next, *curr;

	if (curr=list->head) do {
	    next = curr->next;
	    free(curr);
	} while(curr=next);
}

/*
 * err_usage()
 *	Functions displays usage error, and exits.
 */
static void
err_usage(cmdnm)
char	*cmdnm;
{
	/* display errorand quit, exit(EX_USAGE) */
	fprintf(stderr, "UX:%s:ERROR:%s", cmdnm, E_USAGE1);
	fprintf(stderr, "%s", E_USAGE2);
	exit(EX_USAGE);
}

/* 
 * chk_tab_dsf()
 * If device.tab and ddb_dsfmap file exist, compare the magicno.
 * If magicno are the same, return SUCCESS; otherwise, return  NO_MATCH
 * If couldn't open device.tab return NO_TAB 
 * If couldn't open ddb_dsmap do not exist, then return NO_DSFMAP.
 */
static int
chk_tab_dsf(magic)
unsigned long *magic;
{

	FILE	*f_tab,*f_dsf;		/* DDB file pointers          */

	char	magictok[10];		/* buffers for magic no's      */
	unsigned long tabval, dsfval; 	/* time values that are part of
					 * the magic number */

	register int err;		/* errno save area            */

	/* check if DDB_TAB & DDB_DSFMAP accessible */
	/* Open files, compare Magic numbers */
	if (f_tab=fopen(DDB_TAB, "r")) {
	    if (f_dsf=fopen(DDB_DSFMAP, "r")) {
		/* get magic_no's */
		getmagicno (f_tab, magictok, &tabval);
		getmagicno (f_dsf, magictok, &dsfval);

		if (tabval ==  dsfval) {
		    fclose(f_tab);
		    fclose(f_dsf);
		    *magic = tabval;
		} else {
		    /* magic-nos do not match */
		    fclose(f_tab);
		    fclose(f_dsf);
		    return(NO_MATCH);
		}
	    } else {
		err = errno;
		fclose(f_tab);
		/* DDB_DSFMAP doesn't exit, must upgrade */
		if (access(DDB_DSFMAP,R_OK) < 0 && errno == ENOENT)
			return(NO_DSFMAP);
		else {
			/* errno, cannot open DDB_DSFMAP file */
			errno = err;
			return(FAILURE);
		}
	    }
	} else {
		/* error, cannot open DDB_TAB file */
		if (errno != ENOENT )
			return(FAILURE);
		return(NO_TAB);
	}

	/* Both DDB_TAB and DDB_DSFMAP are present, & magicno's matched */
        return(SUCCESS);
}
