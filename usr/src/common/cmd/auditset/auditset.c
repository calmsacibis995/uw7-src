/*		copyright	"%c%" 	*/

#ident	"@(#)auditset.c	1.3"
#ident  "$Header$"

/***************************************************************************
 * Command: auditset
 * Inheritable Privileges: P_AUDIT,P_SETPLEVEL,P_DACREAD,P_MACREAD
 *       Fixed Privileges: none
 * 
 * Notes:	No options will display the System Audit Criteria:
 *					    User Audit Criteria: 
 *					    Object Level Audit Criteria 
 *		[-d] = display the System Audit Criteria
 *		[-m] = display the Object Level Audit Criteria
 *		[-u user[,...]] = select one or more active users
 *		[-a] =	select all active users
 *		[-e[operator]event[,...]] = set the User Audit Criteria
 *			operators are + - !
 *		[-s[operator]event[,...]] = set the System Audit Criteria
 *			operators are + - !
 *		[-o[operator]event[,...]] = set the Object Audit Criteria
 *			operators are + - !
 *		[-l[operator]level] = set the Level Audit Criteria
 *			operators are + - 
 *		[-r[operator]levelmin-levelmax] = set the Level Range Audit Criteria
 *			operators are  - 
 *
 ***************************************************************************/


/*LINTLIBRARY*/
#include	<sys/types.h>
#include	<stdio.h>
#include	<errno.h>
#include	<pwd.h>
#include	<string.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<sys/param.h>
#include	<sys/procfs.h>
#include 	<audit.h>
#include 	<mac.h>
#include	<dirent.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<ctype.h>
#include	<utmpx.h>

/* Fault Detection and Recovery */
#define FDR1a	":140:invalid options -o, -l, -r\n"
#define FDR2	":141:invalid or inactive user \"%s\" specified\n"
#define FDR2a	":142:invalid or inactive user \"%d\" specified\n"

#define FDR5	":143:auditing is not enabled\n"
#define FDR6	":144:no current object levels in effect\n"
#define FDR7	":145:invalid security level \"%s\" specified\n"
#define FDR8 	":146:maximum security level does not dominate minimum security level \"%s-%s\"\n"
#define FDR10	":147:system defined level limit exceeded, level \"%s\" not set\n"
#define FDR11	":148:no object level event type(s) or class(es) in effect\n"
#define FDR12	":149:level %s is not currently in effect.\n"

#define NOLTDB	":150:LTDB is inaccessible\n"
#define BDLVLIN	":151:lvlin() error, error = %d\n"
#define BDLVLOUT ":152:lvlout() error, error = %d\n"
#define BDMALOC	":19:unable to allocate space\n"
#define NOPERM ":17:Permission denied\n"
#define BADARGV ":18:argvtostr() failed\n"
#define BADSTAT ":20:auditctl() failed ASTATUS, errno = %d\n"
#define BADGSYS ":153:auditevt() failed AGETSYS, errno = %d\n"
#define BADSSYS ":154:auditevt() failed ASETSYS, errno = %d\n"
#define BADGUSR ":155:auditevt() failed AGETUSR, errno = %d\n"
#define BADSUSR ":156:auditevt() failed ASETUSR, errno = %d\n"
#define BADCLVL ":157:auditevt() failed ACNTLVL, errno = %d\n"
#define BADGLVL ":158:auditevt() failed AGETLVL, errno = %d\n"
#define BADSLVL ":159:auditevt() failed ASETLVL, errno = %d\n"
#define BDOPENDIR ":160:opendir() failed for directory /proc \n"
#define B_USAGE_D ":161:usage: auditset [-d [-u user[,...] | -a]]\n"
#define B_USAGE_S ":162:\n auditset [-s [operator]event[,...]] [[-u user[,...]  | -a] -e[operator]event[,...]]\n"
#define E_USAGE_D ":163:usage: auditset [-d [-u user[,...] | -a] [-m]]\n"
#define E_USAGE_S1 ":162:\n auditset [-s [operator]event[,...]] [[-u user[,...]  | -a] -e[operator]event[,...]]\n"
#define E_USAGE_S2 ":164:          [-o [operator]event[,...]] [-l [operator]level] [-r [operator]levelmin-levelmax]\n"
#define NOPKG	":34:system service not installed\n"
#define LVLOPER ":35:%s() failed, errno = %d\n"

#define MXUID	100	/* maximum number of arguments to the -u option */
#define LVLMIN	0	/* minimum object level range to be audited */
#define LVLMAX	1	/* maximum object level range to be audited */
#define LEVEL	2	/* single object level to be audited */


extern char *argvtostr();
extern int satoi(), cremask();
extern struct	passwd	*getpwnam(), *getpwuid();
static	void	usage(),prsys(),prusr(),prlvl(),
		set(),display(), prallusr(),get_actuids(),
		prcom(),setsys(),setlvl(),setevts(),
		parse_range(), verify_lvl(), adumprec(), parsuid(),
		zsetsys(), zprsys();


static int optmask = 0;	/* option mask */
static actl_t	actl;		/* auditctl(2) structure */
static aevt_t	sys;		/* auditevt(2) structure for SYStem events */
static aevt_t	lvl;		/* auditevt(2) structure for OLA */
static char	lvlmintbl[LVL_MAXNAMELEN + 1];
static char	lvlmaxtbl[LVL_MAXNAMELEN + 1];
static char	lvloutbuf[LVL_MAXNAMELEN + 1];
static level_t *lvlp, lvlmin, lvlmax;
static 	int  errflag = 0;
static 	int  mac = 0;
static	int uidnum=0;		/* count of user entered uids */
static	int uidname=0;		/* count of user entered logins */
static	int del_range=0;	/* indicate if operator used for -r option */
static	char	*procdir = "/proc";	/* standard /proc directory */
static char *sys_evtsp;		/* System  Events */
static char *usr_evtsp;		/* List of User Events */
static char *lvl_evtsp;		/* List of Object Level Events */
static char *trap_evtsp;	/* List of Trusted Application Events */
static char *argvp;
static char  *u_logins[MXUID];  /* user entered logins */
static uid_t  u_uids[MXUID]; 	/* user entered uids */
static int  actuidsz = MXUID;

level_t	level;


/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: None
                 auditevt(2): None
                 pfmt: None
                 lvlproc(2): None
                 lvlin: None
                 auditctl(2): None
                 getopt: None
*/

main(argc,argv)
int argc;
char **argv;
{
	extern int optind;
	extern char *optarg;
	int c;
	level_t mylvl, audlvl;

	/* Initialize locale information */
	(void)setlocale(LC_ALL, "");

        /* Initialize message label */
	(void)setlabel("UX:auditset");

        /* Initialize catalog */
	(void)setcat("uxaudit");

	/* make process EXEMPT */
	if (auditevt(ANAUDIT, NULL, sizeof(aevt_t)) == -1) {
         	if (errno == ENOPKG) {
                 	(void)pfmt(stderr, MM_ERROR, NOPKG);
                        exit(ADT_NOPKG);
		} else
			if (errno == EPERM) {
                   		(void)pfmt(stderr, MM_ERROR, NOPERM);
                                exit(ADT_NOPERM);
			}
	}

	/* Get the current level of this process */
	if (lvlproc(MAC_GET, &mylvl) == 0) {
		if (lvlin(SYS_AUDIT, &audlvl) == -1) {
			(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlin", errno);
			exit(ADT_LVLOPER);
		}
		if (lvlequal(&audlvl, &mylvl) == 0) {
			/* SET level if not SYS_AUDIT */
			if (lvlproc(MAC_SET, &audlvl) == -1) {
				if (errno == EPERM) {
                                        (void)pfmt(stderr, MM_ERROR, NOPERM);
                                        exit(ADT_NOPERM);
                                }
                                (void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlproc", errno);
                                exit(ADT_LVLOPER);
			}
			mac=1;
		}
	} else 
             	if (errno != ENOPKG) {
			(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlproc", errno);
			exit(ADT_LVLOPER);
		}
	
	/* save entire command line for audit record */
	if (( argvp = (char *)argvtostr(argv)) == NULL) {
		(void)pfmt(stderr, MM_ERROR, BADARGV);
		adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argv[0]),argv[0]);
		exit(ADT_MALLOC);
	}
	
	/* get current status of auditing */
	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADSTAT, errno);
		adumprec(ADT_AUDIT_CTL,ADT_BADSTAT,strlen(argvp),argvp);
		exit(ADT_BADSTAT);
	}
	
	/* get current the system wide event mask */
	if (auditevt(AGETSYS, &sys, sizeof(aevt_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADGSYS, errno);
		adumprec(ADT_AUDIT_EVT,ADT_BADEGET,strlen(argvp),argvp);
		exit(ADT_BADEGET);
	}
	
	if (mac) {
		/* get the size of the object level table */
		if (auditevt(ACNTLVL, &lvl, sizeof(aevt_t)) == -1) {
			(void)pfmt(stderr, MM_ERROR, BADCLVL, errno);
			adumprec(ADT_AUDIT_EVT,ADT_BADEGET,strlen(argvp),argvp);
			exit(ADT_BADEGET);
		}

		if ((lvl.lvl_tblp=(level_t *)calloc(lvl.nlvls, sizeof(level_t)))==NULL){
			(void)pfmt(stderr, MM_ERROR, BDMALOC);
			adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
			exit(ADT_MALLOC);
		}
		lvlp=lvl.lvl_tblp;

		/* allocate space for the lvl_minp and lvl_maxp range limits */
		if ((lvl.lvl_minp=(level_t *)malloc(sizeof(level_t)))==NULL){
			(void)pfmt(stderr, MM_ERROR, BDMALOC);
			adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
			exit(ADT_MALLOC);
		}
		if ((lvl.lvl_maxp=(level_t *)malloc(sizeof(level_t)))==NULL){
			(void)pfmt(stderr, MM_ERROR, BDMALOC);
			adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
			exit(ADT_MALLOC);
		}
		
		*lvl.lvl_minp=0;
		*lvl.lvl_maxp=0;

		/* get the current object event mask, level and level range*/
		if (auditevt(AGETLVL, &lvl, sizeof(aevt_t))) {
			(void)pfmt(stderr, MM_ERROR, BADGLVL, errno);
			adumprec(ADT_AUDIT_EVT,ADT_BADEGET,strlen(argvp),argvp);
			exit(ADT_BADEGET);
		}
	}
	
	/* display current audit criteria */
	if (argc==1) {		
		prsys();
		prallusr();
		prlvl();
		adumprec(ADT_AUDIT_EVT,ADT_SUCCESS,strlen(argvp),argvp);
		exit(ADT_SUCCESS);
	}
	
	/* parse command line options */
	while ((c=getopt(argc,argv,"dmau:e:s:o:l:r:z:Z"))!=EOF) {
		switch (c) {
		case 'd':	/* display system criteria */
			if (optmask & (ADT_DMASK|ADT_SMASK|ADT_EMASK|ADT_OMASK
			    |ADT_RMASK|ADT_LMASK|ADT_DZMASK|ADT_SZMASK)) {
				errflag++;
				break;
			}
			optmask |= ADT_DMASK;
			break;
		case 'm':	/* display object criteria and levels */
			if (optmask & (ADT_SMASK|ADT_EMASK|ADT_OMASK|ADT_RMASK
			    |ADT_LMASK|ADT_DZMASK|ADT_SZMASK)) {
				errflag++;
				break;
			}
			optmask |= ADT_MMASK;
			break;
		case 'a':	/* get ALL users */
			if (optmask & (ADT_AMASK|ADT_UMASK|ADT_DZMASK
			    |ADT_SZMASK)) { 
				errflag++;
				break;
			}
			optmask |= ADT_AMASK;
			break;
		case 'u':	/* get list of user names or uids */
			if (optmask & (ADT_UMASK|ADT_AMASK|ADT_DZMASK
			    |ADT_SZMASK)) {
				errflag++;
				break;
			}
			parsuid(optarg);
			optmask |= ADT_UMASK;
			break;
		case 'e':	/* event mask for user[s] */
			if (optmask & (ADT_EMASK|ADT_DMASK|ADT_MMASK
			    |ADT_DZMASK|ADT_SZMASK)) {  
				errflag++;
				break;
			} 
			if ((usr_evtsp=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(usr_evtsp,optarg);
			optmask |= ADT_EMASK;
			break;
		case 's':	/* system wide event mask */
			if (optmask & (ADT_SMASK|ADT_DMASK|ADT_MMASK
			    |ADT_DZMASK|ADT_SZMASK)) {
				errflag++;
				break;
			}
			if ((sys_evtsp=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(sys_evtsp,optarg);
			optmask |= ADT_SMASK;
			break;
		case 'o':	/* object level event mask */
			if (optmask & (ADT_OMASK|ADT_DMASK|ADT_MMASK
			    |ADT_DZMASK|ADT_SZMASK)) {
				errflag++;
				break;
			}
			if (mac) {
				if ((lvl_evtsp=(char *)malloc(strlen(optarg) + 1))==NULL){
					(void)pfmt(stderr, MM_ERROR, BDMALOC);
					adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
					exit(ADT_MALLOC);
				}
				(void)strcpy(lvl_evtsp,optarg);
				optmask |= ADT_OMASK;
				break;
			} else {
				(void)pfmt(stderr, MM_ERROR, FDR1a);
				(void)pfmt(stderr, MM_ERROR, NOPKG);
				(void)pfmt(stderr,MM_ACTION,B_USAGE_D);
				(void)pfmt(stderr,MM_NOSTD,B_USAGE_S);
				adumprec(ADT_AUDIT_EVT,ADT_NOPKG, strlen(argvp),argvp);
				exit(ADT_NOPKG);
				break;
			}
		case 'l':	/* single level[s] to be audited */
			if (optmask & (ADT_DMASK|ADT_MMASK|ADT_DZMASK
			    |ADT_SZMASK)) {
				errflag++;
				break;
			}
			if (mac) {
				verify_lvl(optarg,LEVEL);
				optmask |= ADT_LMASK;
				break;
			} else {
				(void)pfmt(stderr, MM_ERROR, FDR1a);
				(void)pfmt(stderr, MM_ERROR, NOPKG);
				(void)pfmt(stderr,MM_ACTION,B_USAGE_D);
				(void)pfmt(stderr,MM_NOSTD,B_USAGE_S);
				adumprec(ADT_AUDIT_EVT,ADT_NOPKG, strlen(argvp),argvp);
				exit(ADT_NOPKG);
				break;
			}
		case 'r':	/* object level-range to be audited */
			if (optmask & (ADT_RMASK|ADT_DMASK|ADT_MMASK
			    |ADT_DZMASK|ADT_SZMASK)) {
				errflag++;
				break;
			}
			if (mac) {
				parse_range(optarg); 
				verify_lvl(lvlmintbl,LVLMIN);
				verify_lvl(lvlmaxtbl,LVLMAX);
				if (lvldom(&lvlmax, &lvlmin) <=0) {
					(void)pfmt(stderr, MM_ERROR, FDR8,
						lvlmintbl,lvlmaxtbl);
					adumprec(ADT_AUDIT_EVT,ADT_BADSYN,strlen(argvp),argvp);
					exit(ADT_BADSYN);
				}
				optmask |= ADT_RMASK;
			} else {
				(void)pfmt(stderr, MM_ERROR, FDR1a);
				(void)pfmt(stderr, MM_ERROR, NOPKG);
				(void)pfmt(stderr,MM_ACTION,B_USAGE_D);
				(void)pfmt(stderr,MM_NOSTD,B_USAGE_S);
				adumprec(ADT_AUDIT_EVT,ADT_NOPKG, strlen(argvp),argvp);
				exit(ADT_NOPKG);
			}
			break;
		case 'z': /* set extended bits, trusted applications */
			if (optmask & (ADT_SMASK|ADT_DMASK|ADT_MMASK
			    |ADT_DZMASK|ADT_DZMASK)) {
				errflag++;
				break;
			}
			if ((trap_evtsp=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(trap_evtsp,optarg);
			optmask = ADT_SZMASK;
			break;
		case 'Z': /* display extended bits, trusted applications */
			if (optmask & (ADT_SMASK|ADT_DMASK|ADT_MMASK 
			    |ADT_DZMASK|ADT_SZMASK)) {
				errflag++;
				break;
			}
			zprsys();
			exit(ADT_SUCCESS);
		default:
                        errflag++;
                        break;
		}
	}

        /* The command line is invalid if it contained an arguement */
	if (optind < argc)
		errflag++;

        /* If the -m entered without the -d option */
	if ((optmask & ADT_MMASK) && (!(optmask & ADT_DMASK)))
		errflag++;

	if (errflag > 0) {
		usage();
		adumprec(ADT_AUDIT_EVT,ADT_BADSYN,
			strlen(argvp),argvp);
		exit(ADT_BADSYN);
	}

	if (optmask & ADT_DMASK)
		display();
	else
		set();
	adumprec(ADT_AUDIT_EVT,ADT_SUCCESS,strlen(argvp),argvp);
	exit(ADT_SUCCESS);
/*NOTREACHED*/
}

/*
 * Procedure:     adumprec
 *
 * Restrictions:
                 auditdmp(2): None
 * Notes:                             
 * USER level interface to auditdmp(2)
 * for USER level audit event records 
*/
void
adumprec(rtype,status,size,argp)
int rtype;			/* event type */
int status;			/* event status */
int size;			/* size of argp */
char *argp;			/* data/arguments */
{
        arec_t rec;

        rec.rtype = rtype;
        rec.rstatus = status;
        rec.rsize = size;
        rec.argp = argp;

        auditdmp(&rec, sizeof(arec_t));
        return;
}

static	void
display()
{
	prsys();
	if (optmask & ADT_AMASK) 
		prallusr();
	if (optmask & ADT_UMASK) 
		prusr();
	if (optmask & ADT_MMASK)
		prlvl();
}


/*
 * Procedure:     get_actuids
 *
 * Restrictions:
                 opendir: none
                 open(2): none
*/
static void
get_actuids(act_uids,total)
uid_t *act_uids;
int *total;
{
	register struct	utmpx	*u;
	struct passwd *passwd_p;

	/* Find the entry for this pid in the utmp file.  */
	while ((u = getutxent()) != NULL) {
		if (u->ut_type == USER_PROCESS) {
			if ((passwd_p = getpwnam(u->ut_user)) != NULL) {
				/* list of all users logged in */
				act_uids[(*total)++] = passwd_p->pw_uid;
				if (*total > actuidsz) {
					uid_t *Nuids;
					if ((Nuids = (uid_t *)
					    malloc(actuidsz * 2)) == NULL) {
						(void)pfmt(stderr, MM_ERROR,
							BDMALOC);
						adumprec(ADT_AUDIT_EVT,
							ADT_MALLOC,
							strlen(argvp),argvp);
						exit(ADT_MALLOC);
					}
					(void)memcpy(Nuids, act_uids,
						actuidsz * sizeof(uid_t));
					(void)free(act_uids);
					act_uids = Nuids;
					actuidsz *= 2;
				}
			}
		}
	}
	endutxent();		/* close utmp file */
}

static void
parse_range(argp)
register char	*argp;
{
	register char	*lvl;
	register int	range=0;

	/* Valid operator to  -r option is '-' */
	if (*argp == '-') {
		del_range=1;
		argp++;
	}

	/* must be of the form "levelmin-levelmax" */
	while(argp && *argp) {
		lvl=argp;
		argp=strchr(argp,'-');
		if (argp)
			*argp++ ='\0';
		range++;
		if (range==1)
			(void)strcpy(lvlmintbl,lvl);
		else if (range==2)
			(void)strcpy(lvlmaxtbl,lvl);
	}
	if (range != 2) {
		usage();
		adumprec(ADT_AUDIT_EVT,ADT_BADSYN, strlen(argvp),argvp);
		exit(ADT_BADSYN);
	}
}

static	void
parsuid(sp)
char	*sp;
{
	register char *p;
	int x,fd,n;
	
	/* A user may enter either login name or uid */
	while (sp) {
	 	if (p=strchr(sp,','))
			*p++ ='\0'; 
		if (isalnum(*sp) == 0) {
			/* Is not alphanumeric */
			(void)pfmt(stdout, MM_WARNING, FDR2, sp);
			sp = p;
			continue;
		}
		if ( (n=satoi(sp)) < 0) {
			/* Check for duplicates */
			for (fd=x=0; x<uidname; x++)
				if (strcmp(sp,u_logins[x]) == 0) {
					fd=1;
					break;
				}
			/* Add login name to array */
			if (!fd) 
				u_logins[uidname++] = sp;
		} else {
			/* Check for duplicates */
			for (fd=x=0; x<uidnum; x++)
				if ( n == u_uids[x]) {
					fd=1;
					break;
				}
			/* Add uid to array */
			if (!fd) 
				u_uids[uidnum++] = n;
		}
		sp = p;
	}
	return;
}

/*
 * Procedure:     prallusr
 *
 * Restrictions:
                 getpwuid: None
                 auditevt(2): None

 * Notes: Print the user audit mask for all active users
*/

static void
prallusr()
{
	uid_t *act_uids;
	int total=0;
	short first=1;
	short x;
	struct passwd *passwd_p;


	if ((act_uids = (uid_t *)malloc(actuidsz)) == NULL) {
		(void)pfmt(stderr, MM_ERROR, BDMALOC);
		adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}

	/* Obtain uids of all active users */
	get_actuids(act_uids,&total);

	qsort(act_uids, total, sizeof(uid_t), (int(*)()) strcmp);

	/* For each active uid obtain the login name and the user mask */
	for (x=0; x<total; x++) {
		if (first) {
			(void)pfmt(stdout, MM_NOSTD,
				":166:\nUser Audit Criteria:\n");
			first=0;
		}

		if ((x > 0) && (act_uids[x] == act_uids[x - 1]))
			continue;

		if ((passwd_p = getpwuid(act_uids[x])) != NULL)
			(void)fprintf(stdout,"\t%s (%d):\t"
				,passwd_p->pw_name,act_uids[x]);
		else
			(void)pfmt(stdout, MM_NOSTD, ":167:\tUNKNOWN:\t");
		setpwent();

		sys.uid = act_uids[x];
		if (auditevt(AGETUSR, &sys, sizeof(aevt_t))) {
			(void)pfmt(stderr, MM_ERROR, BADGUSR,errno);
			adumprec(ADT_AUDIT_EVT,ADT_BADEGET,strlen(argvp),argvp);
			exit(ADT_BADEGET);
		}

		prcom();
	}
	return;
}

static void
prcom()
{
	int i;
	int allcnt = 0;

	/* Compare the retrieved mask against all possible auditable events. */ 
	for (i=1; i<=ADT_NUMOFEVTS; i++) 
		if (EVENTCHK(i,sys.emask)) {
			allcnt++;
			if (i != (allcnt))	
				i = (ADT_NUMOFEVTS + 1);
		}
	if (allcnt == 0)
		(void)pfmt(stdout, MM_NOSTD, ":168: none ");
	else if (allcnt == ADT_NUMOFEVTS)
		(void)pfmt(stdout, MM_NOSTD, ":169: all ");
	else
	     for (i=1; i<=ADT_NUMOFEVTS; i++)	/* Skip zero position */
		if (EVENTCHK(i,sys.emask)) 
			(void)fprintf(stdout," %s",adtevt[i].a_evtnamp);
	(void)fprintf(stdout,"\n\n");
}

/*
 * Procedure:     prlvl
 *
 * Restrictions:
                 lvlout: None
                 pfmt: None
 *  Notes:
 *  Print Object Level Mask Function
 */

static	void
prlvl()
{
	int i;
	int allcnt,objcnt;

	allcnt=objcnt=0;

	if (mac) {
		(void)pfmt(stdout, MM_NOSTD,
			":170:\nObject Level Audit Criteria: ");

		/* 
		 * Compare the retrieved object mask with all possible
		 * auditable object event types.
		 */
		for (i=1; i<=ADT_NUMOFEVTS; i++) {
			if (adtevt[i].a_objt) {
				objcnt++;
				if (EVENTCHK(i,lvl.emask)) {
					allcnt++;
					if (objcnt  != allcnt)	
						break;
				}
			}
		}
		if (allcnt == 0)
			(void)pfmt(stdout, MM_NOSTD, ":168: none ");
		else if (allcnt == objcnt)
			(void)pfmt(stdout, MM_NOSTD, ":169: all ");
		else
			/* Skip zero position */
			for (i=1; i<=ADT_NUMOFEVTS; i++) {
				if (adtevt[i].a_objt) {
					if (EVENTCHK(i,lvl.emask)) 
						(void)fprintf(stdout," %s",
							adtevt[i].a_evtnamp);
				}
			}
		(void)fprintf(stdout,"\n");
	
		/* If a level range exists for the object mask, display it */
		if ((*lvl.lvl_minp != 0) && (*lvl.lvl_maxp != 0)) {
			lvlout(lvl.lvl_minp, lvlmintbl, MAXNAMELEN, LVL_ALIAS);
			lvlout(lvl.lvl_maxp, lvlmaxtbl, MAXNAMELEN, LVL_ALIAS);
			(void)fprintf(stdout,"\t%s - %s\n",lvlmintbl,lvlmaxtbl);
		}
	
		/*
		 * If individual levels exists for the object mask,
		 * display them
		 */
		for (i=1; i<=lvl.nlvls; lvlp++,i++) {
			if (*lvlp != 0) {
				if (lvlout(lvlp, lvloutbuf, MAXNAMELEN, 
				    LVL_ALIAS) == -1) {
					(void)pfmt(stderr, MM_ERROR, BDLVLOUT,
					    errno);
					adumprec(ADT_AUDIT_EVT, ADT_BADLOUT,
					    strlen(argvp), argvp);
					exit(ADT_BADLOUT);
				} else
					(void)fprintf(stdout,"\t%s\n",lvloutbuf);
			}
		}
		
	}
}

/*
 *  Print System Mask Function
 */
static	void
prsys()
{
	
	(void)pfmt(stdout, MM_NOSTD, ":171:System Audit Criteria:\n\tsystem:");
	prcom();
}
	
/*
 * Procedure:     prusr
 *
 * Restrictions:
                 getpwnam: None
                 auditevt(2): None
                 getpwuid: None
 * Notes:                           
 * Print User Mask Function        
*/

static	void
prusr()
{
	uid_t *act_uids;
	uid_t val_uid;
	int total=0;
	short fd,x,y;
	short first=1;
	struct passwd *passwd_p;
	
	if ((act_uids = (uid_t *)malloc(actuidsz)) == NULL) {
		(void)pfmt(stderr, MM_ERROR, BDMALOC);
		adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}

	/* Obtain uids of all active users */
	get_actuids(act_uids,&total);

	/* For each user entered login */
	for (x=0; x<uidname; x++) {
		/* Obtain uid */
		if ((passwd_p = getpwnam(u_logins[x])) != NULL) {
			val_uid=passwd_p->pw_uid;
			setpwent();

			/*
			 * If the login name translates to a uid already 
			 * contained in the option argument, then display this 
			 * user during the evaluation of the uid(s). Duplicate 
			 * user information should not be displayed.
			 */
			for (y=fd=0; y<uidnum; y++) 
				if (val_uid == u_uids[y]){
					fd=1;
					break;
				}
			if (fd)
				continue;
		} else {
			(void)pfmt(stdout, MM_WARNING, FDR2, u_logins[x]);
			setpwent();
			continue;
		}
		/* Loop through list of unique active uids */
		for (fd=0,y=0; y<total; y++) {
			if (val_uid == act_uids[y]) {
				if (first) {
					(void)pfmt(stdout, MM_NOSTD,
					    ":166:\nUser Audit Criteria:\n");
					first=0;
				}
				(void)fprintf(stdout,"\t%s (%d):\t",
				    u_logins[x], val_uid);
				sys.uid = val_uid;
				if (auditevt(AGETUSR, &sys, sizeof(aevt_t))) {
					(void)pfmt(stderr, MM_ERROR, BADGUSR,
					    errno);
					adumprec(ADT_AUDIT_EVT, ADT_BADEGET,
					    strlen(argvp), argvp);
					exit(ADT_BADEGET);
				}
				prcom();
				fd=1;
				break;
			}
		}
		if (!fd)
			(void)pfmt(stdout, MM_WARNING, FDR2, u_logins[x]);
	}

	/* For each user entered uid */
	for (x=0; x<uidnum; x++) {
		/* Loop through list of unique active uids */
		for (fd=0,y=0; y<total; y++) {
			if (u_uids[x] == act_uids[y]) {
				if (first) {
					(void)pfmt(stdout, MM_NOSTD,
					    ":166:\nUser Audit Criteria:\n");
					first=0;
				}
				/* A match is found print the user mask */
				if ((passwd_p = getpwuid(u_uids[x])) != NULL)
					(void)fprintf(stdout,"\t%s (%d):\t",
					    passwd_p->pw_name,u_uids[x]);
				else
					(void)pfmt(stdout, MM_NOSTD,
					    ":167:\tUNKNOWN:\t");
				setpwent();
				sys.uid = u_uids[x];
				if (auditevt(AGETUSR, &sys, sizeof(aevt_t))) {
					(void)pfmt(stderr, MM_ERROR, BADGUSR,
					    errno);
					adumprec(ADT_AUDIT_EVT, ADT_BADEGET,
					    strlen(argvp), argvp);
					exit(ADT_BADEGET);
				}
				prcom();
				fd=1;
				break;
			}
		}
		if (!fd)
			(void)pfmt(stdout, MM_WARNING, FDR2a, u_uids[x]);
	}
}

static void
set()
{
	/* If the -u or -a option entered without the -e option */
	if ((optmask & (ADT_UMASK|ADT_AMASK)) && (!(optmask & ADT_EMASK))) {
		usage();
		adumprec(ADT_AUDIT_EVT,ADT_BADSYN,strlen(argvp),argvp);
		exit(ADT_BADSYN);
	}

	/* If the -e option is entered without the -u or -a option */
	if ((optmask & ADT_EMASK) && (!(optmask & (ADT_UMASK|ADT_AMASK)))) {
		usage();
		adumprec(ADT_AUDIT_EVT,ADT_BADSYN,strlen(argvp),argvp);
		exit(ADT_BADSYN);
	}

	/*
	 * The -e option is processed prior to the -s, -o, -l or -r options
	 * If unable to determine active users (read /proc) due to lack of
	 * of privilege, process exits.
	 */
	if (optmask & ADT_EMASK)
		setevts();

	if (optmask & ADT_SMASK) 
		setsys(); 

	if (optmask & (ADT_OMASK|ADT_RMASK|ADT_LMASK)) 
		setlvl(); 

	if (optmask & ADT_SZMASK) 
		zsetsys(); 
}

/*
 * Procedure:     setevts
 *
 * Restrictions:
                 getpwnam: None
                 pfmt: None
                 auditevt(2): None
                 cremask: None
*/
static	void
setlvl()
{
	int i,rc;
	uint obj_lvl_flag = 0;
	int fd_level=0;
	int fd_event=0;

	/* warn the user if -o option entered without -r or -l */
	if ((optmask & ADT_OMASK) && (!(optmask & (ADT_LMASK | ADT_RMASK)))) {
		/*
		 * If there are no current levels in effect for auditing
		 * objects, inform the user.
		 */
		if (!(lvl.flags & (ADT_LMASK | ADT_RMASK)))
			(void)pfmt(stdout, MM_WARNING, FDR6);
	}

	/* warn the user if -r or -l option entered without the -o option */
	if ((optmask & (ADT_RMASK | ADT_LMASK)) && (!(optmask & ADT_OMASK))) {
		/* If there is no object mask in effect inform the user. */
		if (!(lvl.flags & ADT_OMASK))
			(void)pfmt(stdout, MM_WARNING, FDR11);
	}
	
	if (optmask & ADT_OMASK)  
		if ((rc=cremask(ADT_OMASK,  lvl_evtsp, lvl.emask)) == -1) {
			adumprec(ADT_AUDIT_EVT,ADT_FMERR,strlen(argvp),argvp);
			exit(ADT_FMERR);
		}

	if (optmask & ADT_RMASK) {
		obj_lvl_flag |= ADT_RMASK;

		/* user entered operator '-' */
		if (del_range) {
			*lvl.lvl_minp = 0;
			*lvl.lvl_maxp = 0;
		} else {
			*lvl.lvl_minp = lvlmin;
			*lvl.lvl_maxp = lvlmax;
		}
	}

	/*
	 * If the NEW RESULTING object mask contains no object events and there 
	 * is a resulting level or level range set inform the user.
	 * Do not set "lvl.flags" to ADT_OMASK if the user inputted event list
	 * is entirely invalid.
	 */ 
	if ((optmask & ADT_OMASK) && (rc == 0)) {
		obj_lvl_flag |= ADT_OMASK;

	     	for (i=1; i<=ADT_NUMOFEVTS; i++) {	/* Skip zero position */
			if (adtevt[i].a_objt) {
				if (EVENTCHK(i,lvl.emask)) {
					fd_event=1;
					break;
				}
			}
		}
		if (!fd_event) {
			if (lvlmin)
				(void)pfmt(stdout, MM_WARNING, FDR11);
			else {
				for (i=0; i<lvl.nlvls; i++) {
					if (lvl.lvl_tblp[i] != 0) {
						(void)pfmt(stdout, MM_WARNING,
						    FDR11);
						break;
					}
				}
			}
	
		}
	}

	/*
	 * If both the resulting level range and individual levels were cleared
	 * and a object mask exists inform the user.
	 */
	if (optmask & (ADT_RMASK | ADT_LMASK)) {
		for (i=0; i<lvl.nlvls; i++) {
			if (lvl.lvl_tblp[i] != 0) {
				fd_level=1;
				break;
			}
		}
		if ((fd_level == 0) && (*lvl.lvl_minp == 0)) {
			if (obj_lvl_flag & ADT_OMASK) {
				/* Test the NEW RESULTING object mask */
	     			for (i=1; i<=ADT_NUMOFEVTS; i++) {
					if (adtevt[i].a_objt) {
						if (EVENTCHK(i,lvl.emask)) {
							(void)pfmt(stderr,
							    MM_WARNING, FDR6);
							break;
						}
					}
				}
			} else {
				/*
				 * There is no NEW RESULTING object mask 
				 * but there is an object mask currently set.
				 */
				if (lvl.flags & ADT_OMASK)
					(void)pfmt(stdout, MM_WARNING, FDR6);
			}
		}
	}

	if (optmask & ADT_LMASK)
		obj_lvl_flag |= ADT_LMASK;
	lvl.flags=obj_lvl_flag;

	
	if ((auditevt(ASETLVL, &lvl, sizeof(aevt_t))) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADSLVL, errno);
		adumprec(ADT_AUDIT_EVT,ADT_BADESET,strlen(argvp),argvp);
		exit(ADT_BADESET);
	}
}

/*
 * Function:     setevts
 * Privileges:   auditevt()   P_AUDIT
 * Restrictions: none
 */
static	void
setevts()
{
	uid_t *act_uids;
	short fd,y,x;
	struct passwd *passwd_p;
	uid_t val_uids[MXUID];
	uid_t ue_uid;
	int total,z;
	aevt_t usr;
	int rc;
	char *evt_list;

	total=z=0;

	if ((act_uids = (uid_t *)malloc(actuidsz)) == NULL) {
		(void)pfmt(stderr, MM_ERROR, BDMALOC);
		adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}

	/* Obtain uids of all active users */
	get_actuids(act_uids,&total);

	if (optmask & ADT_UMASK) {
		/* For each user entered login */
		for (x=0; x<uidname; x++) {
			/* Obtain uid */
			if ((passwd_p = getpwnam(u_logins[x])) != NULL) {
				ue_uid=passwd_p->pw_uid;
				setpwent();
			} else {
				(void)pfmt(stdout, MM_WARNING, FDR2, u_logins[x]);
				setpwent();
				continue;
			}
			/* Loop through list of unique active uids */
			for (fd=0,y=0; y<total; y++) {
				if (ue_uid == act_uids[y]) {
					val_uids[z] = act_uids[y];
					z++;
					fd=1;
					break;
				}
			}
			if (!fd)
				(void)pfmt(stdout, MM_WARNING, FDR2, u_logins[x]);
		}
	
		/* For each user entered uid */
		for (x=0; x<uidnum; x++) {
			/* Loop through list of unique active uids */
			for (fd=0,y=0; y<total; y++) {
				if (u_uids[x] == act_uids[y]) {
					/*
					 * A match is found.
					 * Save the valid and active uid.
					 */
					val_uids[z++]=u_uids[x];
					fd=1;
					break;
				}
			}
			if (!fd)
				(void)pfmt(stdout, MM_WARNING, FDR2a, u_uids[x]);
		}
		total= z;
	} else {
		/*
		 * The -a option was entered - reset the user mask
		 * for all active users.
		 */
		(void)memcpy(&val_uids,act_uids,actuidsz);
	}


	/* Maintain a copy of the user input event list */
	if (!(evt_list=strdup(usr_evtsp))) {
		(void)pfmt(stderr, MM_ERROR, BDMALOC);
		adumprec(ADT_AUDIT_EVT,ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}

	for (x=0; x<total; x++) {
		usr.uid=val_uids[x];
		if (auditevt(AGETUSR, &usr, sizeof(aevt_t))) {
			(void)pfmt(stderr, MM_ERROR, BADGUSR,errno);
			adumprec(ADT_AUDIT_EVT,ADT_BADEGET,strlen(argvp),argvp);
			exit(ADT_BADEGET);
		}

		if ((rc=cremask(ADT_UMASK, evt_list, usr.emask)) == -1) {
			adumprec(ADT_AUDIT_EVT,ADT_FMERR,strlen(argvp),argvp);
			exit(ADT_FMERR);
		}


		/*
		 * If the user inputted "event list" is entirely invalid
		 * for this user, then continue to next user.
		 */ 
		if (rc == 0) {
			if (auditevt(ASETUSR, &usr, sizeof(aevt_t))) {
				(void)pfmt(stderr, MM_ERROR, BADSUSR, errno);
				adumprec(ADT_AUDIT_EVT, ADT_BADESET,
				    strlen(argvp), argvp);
				exit(ADT_BADESET);
			}
		}

		strcpy(evt_list,usr_evtsp);
	}
	return;
}

/*
 * Procedure:     setsys
 *
 * Restrictions:
                 cremask: None
                 auditevt(2): None
                 pfmt: None
*/

static	void
setsys()
{
	int rc;

	if ((rc=cremask(ADT_SMASK, sys_evtsp, sys.emask)) == -1) {
		adumprec(ADT_AUDIT_EVT,ADT_FMERR,strlen(argvp),argvp);
		exit(ADT_FMERR);
	}
	
	/*
	 * If the user inputted event list is entirely invalid no
	 * further action is taken.
	 */
	if (rc == 0) {
		if (auditevt(ASETSYS, &sys, sizeof(aevt_t))) {
			(void)pfmt(stderr, MM_ERROR, BADSSYS, errno);
			adumprec(ADT_AUDIT_EVT,ADT_BADESET,strlen(argvp),argvp);
			exit(ADT_BADESET);
		}
	}
}

/*
 * Procedure:     zsetsys
 *
 * Restrictions:
                 cremask: None
                 auditevt(2): None
                 pfmt: None
*/

static	void
zsetsys()
{
	int rc;

	if ((rc=zcremask(trap_evtsp, sys.emask)) == -1) {
		adumprec(ADT_AUDIT_EVT,ADT_FMERR,strlen(argvp),argvp);
		exit(ADT_FMERR);
	}
	
	/*
	 * If the user inputted event list is entirely invalid no
	 * further action is taken.
	 */
	if (rc == 0) {
		if (auditevt(ASETSYS, &sys, sizeof(aevt_t))) {
			(void)pfmt(stderr, MM_ERROR, BADSSYS, errno);
			adumprec(ADT_AUDIT_EVT,ADT_BADESET,strlen(argvp),argvp);
			exit(ADT_BADESET);
		}
	}
}


static	void
usage()
{
	if (mac) {
		(void)pfmt(stderr,MM_ACTION,E_USAGE_D);
		(void)pfmt(stderr,MM_NOSTD,E_USAGE_S1);
		(void)pfmt(stderr,MM_NOSTD,E_USAGE_S2);
	} else {
		(void)pfmt(stderr,MM_ACTION,B_USAGE_D);
		(void)pfmt(stderr,MM_NOSTD,B_USAGE_S);
	}
}

/*
 * Procedure:     verify_lvl
 *
 * Restrictions:
                 lvlin: None
*/

void
verify_lvl(lvlstr,ltype)
char	*lvlstr;
int	ltype;
{
	level_t level;
	level_t *tmpp,*lp;
	char input_level[LVL_MAXNAMELEN+2];
	int levelcnt=0; /* number of current object levels being audited */
	int i,match=0;
	

	if (ltype == LEVEL) {
		if (*lvlstr == '!') {
			(void)pfmt(stderr, MM_ERROR, FDR7, lvlstr);
			adumprec(ADT_AUDIT_EVT,ADT_BADSYN,strlen(argvp),argvp);
			exit(ADT_BADSYN);
		} else {
			if ((*lvlstr == '+') || (*lvlstr == '-'))
				(void)strcpy(input_level,lvlstr+1);
			else
				(void)strcpy(input_level,lvlstr);
		}
	} else
		(void)strcpy(input_level,lvlstr);

	/* assure valid level, and get associated lid */
	if (lvlin(input_level, &level) == -1) {
		switch(errno) {
			case EINVAL:	/* invalid security level specified */
				(void)pfmt(stderr, MM_ERROR, FDR7, lvlstr);
				adumprec(ADT_AUDIT_EVT, ADT_BADSYN,
				    strlen(argvp), argvp);
				exit(ADT_BADSYN);
				break;
			case EACCES:	/* no access to LTDB */
				(void)pfmt(stderr, MM_ERROR, NOLTDB);
				adumprec(ADT_AUDIT_EVT, ADT_BADLVLIN,
				    strlen(argvp), argvp);
				exit(ADT_BADLVLIN);
				break;
			default:	/* library error */
				(void)pfmt(stderr, MM_ERROR, BDLVLIN, errno);
				adumprec(ADT_AUDIT_EVT, ADT_BADLVLIN,
				    strlen(argvp),argvp);
				exit(ADT_BADLVLIN);
		}
	}

	switch(ltype) {
	case LVLMIN:
		lvlmin=level;
		break;
	case LVLMAX:
		lvlmax=level;
		break;
	case LEVEL:
		switch(*lvlstr)
		{
			case '+':
				/* calculate number of levels being audited */
				for (i=0; i<lvl.nlvls; i++) {
					if (lvl.lvl_tblp[i] != 0)
	        				levelcnt++;
	 			}
				if (levelcnt < lvl.nlvls)
					/* Add this new level */
					lvl.lvl_tblp[levelcnt]=level;
				else
					(void)pfmt(stdout, MM_WARNING, FDR10,
					    lvlstr);
				break;
			case '-':
				/* delete this level from the current list */
				if ((lp=(level_t *)calloc(lvl.nlvls,
				    sizeof(level_t)))==NULL) {
					(void)pfmt(stderr, MM_ERROR, BDMALOC);
					adumprec(ADT_AUDIT_EVT, ADT_MALLOC,
					    strlen(argvp), argvp);
					exit(ADT_MALLOC);
				}
				tmpp=lp;
				for (i=0; i<lvl.nlvls; i++) {
					if (lvl.lvl_tblp[i] == level)
						match=1;
					else {
						*tmpp=lvl.lvl_tblp[i];
						tmpp++;
					}
	 			}
				if (match == 1)
					(void)memcpy(lvl.lvl_tblp, lp,
					     (lvl.nlvls * sizeof(level_t)));
				else
					(void)pfmt(stdout, MM_WARNING, FDR12,
					     lvlstr);
				break;
			default:
				/* replace the current auditable levels */
				(void)memset(lvl.lvl_tblp, '\0',
				    (lvl.nlvls * sizeof(level_t)));
				lvl.lvl_tblp[0]=level;
				break;
		}
		break;
	default:
		break;
	}
}

/*
 *  Print System Mask Function for Trusted Applications
 */
static	void
zprsys()
{
	int i;
	int allcnt = 0;
	
	(void)pfmt(stdout, MM_NOSTD, ":171:System Audit Criteria:\n\tsystem:");
	/* Compare the retrieved mask against all possible auditable events */
	for (i=128; i<=255; i++) 
		if (EVENTCHK(i,sys.emask)) {
			allcnt++;
			if (i != (allcnt))	
				i = (255 + 1);
		}
	if (allcnt == 0)
		(void)pfmt(stdout, MM_NOSTD, ":168: none ");
	else if (allcnt == 128)
		(void)pfmt(stdout, MM_NOSTD, ":169: all ");
	else
		for (i=128; i<=255; i++)	/* Skip zero position */
			if (EVENTCHK(i,sys.emask)) 
				(void)fprintf(stdout," %s",adtevt[i].a_evtnamp);
	(void)fprintf(stdout,"\n\n");
}
