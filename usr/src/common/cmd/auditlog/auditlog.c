/*		copyright	"%c%" 	*/

#ident	"@(#)auditlog.c	1.4"
#ident  "$Header$"
/***************************************************************************
 * Command: auditlog
 * Inheritable Privileges: P_SETPLEVEL,P_AUDIT
 *       Fixed Privileges: None
 *
 * Notes:	No options gets the current status of auditing,
 *				name of the current event log,
 *				current audit buffer high water mark,
 *				current maximum size for the event log,
 *				name of the alternate event log,
 *				and name of program to be run after log switch.
 *		[-P path] = specification of the primary log file path.
 *		[-p node] = a 7 character node name can be appended to log file.
 *		[-v high_water] = frequency of buffer writes to disk, 0 = directly.
 *		[-x max_size] = maximum size of an audit log file in 512 bytes blocks.
 *		[-s] = shutdown system to maintenance mode when audit log full.
 *		[-d] = disable auditing and continue operation when audit log full.
 *		[-A next_path] = alternate log file to be used when audit log full.
 *		[-a next_node] = a 7 character node name for alternate audit log file.
 *		[-n pgm] = a program may also be run during audit log file switch.
 *
 ***************************************************************************/

/*LINTLIBRARY*/
#include	<stdio.h>
#include	<errno.h>
#include	<string.h>
#include	<deflt.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<audit.h>
#include	<mac.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<stdlib.h>

/*	FDR1 moved to auditon(1M)	*/
/*	FDR4 comes from AUDIT module	*/
/*	FDR9 comes from init(1M)	*/

/* Fault Detection and Recovery */ 
#define FDR2	":9:invalid max_size value specified\n"
#define FDR3	":10:max_size value applies only to regular files\n"

#define FDR5	":11:invalid high water mark specified\n"
#define FDR6	":12:\"-%s\" option not allowed while auditing enabled\n"
#define FDR7	":13:cannot open/access path or device %s\n"
#define FDR8	":14:\"%s\" is not an executable file\n"

#define USAGE1	":15:usage: auditlog [-P path][-p node][-v high_water][-x max_size]\n"
#define USAGE2	":16:\t\t\t\t[-s|-d|[[-A next_path][-a next_node][-n pgm]]]\n"

#define NOPERM ":17:Permission denied\n"
#define BADARGV ":18:argvtostr() failed\n"
#define BDMALOC ":19:unable to allocate space\n"
#define BADSTAT ":20:auditctl() failed ASTATUS, errno = %d\n"
#define BADLGET ":21:auditlog() failed ALOGGET, errno = %d\n"
#define BADBGET ":22:auditbuf() failed ABUFGET, errno = %d\n"
#define BADLSET ":23:auditlog() failed ALOGSET, errno = %d\n"
#define BADBSET ":24:auditbuf() failed ABUFSET, errno = %d\n"
#define MORE2	":25:Audit Log File Size Must Be >= %d (%d byte)blocks\n"
#define MORE5	":26:Audit Buffer High Water Mark Must Be >= 0 and <= %d bytes\n"
#define BIGNODE	":27:event log node must be < %d characters\n"
#define NOSLASH	":28:event log node may not contain a slash\n"
#define FULPATH	":29:full pathname not specified\n"
#define BIGPATH	":30:pathname component to long\n"
#define NOTREGF	":31:\"%s\" is not a regular file\n"
#define NACCESS	":32:cannot access %s\n"
#define CHKDFLT	":33:check the value of the %s parameter in the /etc/default/audit file\n" 
#define NOPKG	":34:system service not installed\n"
#define LVLOPER	":35:%s() failed, errno = %d\n"

#define BSIZE   512      /* file system block size */
#define BIGP 	"P"
#define LTLP	"p"
#define FORMAT_MSG	100
#define ALTERNATE	1
#define PRIMARY		2
#define IGNORE		3
#define ADT_DMASK	0x1
#define ADT_UPMASK	0x2
#define ADT_LPMASK	0x4
#define ADT_VMASK	0x8
#define ADT_SMASK	0x10
#define ADT_XMASK	0x20
#define ADT_NMASK	0x40
#define ADT_UAMASK	0x80
#define ADT_LAMASK	0x100

extern char *argvtostr();
static	void	usage(), adumprec(), obt_dflts(), prtdflt();
static	int	chk_path(), chk_node(), chk_pgm(), chk_size(), set_water();
static int prntlog(), satoi();

abuf_t 	abuf;		/* auditbuf(2) structure */
actl_t	actl;		/* auditctl(2) structure */
alog_t 	alog;		/* auditlog(2) structure */

static	int	flags = 0;
static	int	onfull = 0;
static int optmask = 0;
static char nullchar = '\0';
static char *argvp;

/*
 * Procedure:     main
 *
 * Restrictions:
                 auditevt(2): None
                 lvlproc(2): None
                 auditctl(2): None
                 auditlog(2): None
                 auditbuf(2): None
*/
main(argc,argv)
int  argc;
char **argv;
{
	extern int optind;
	extern char *optarg;
	level_t mylvl, audlvl;
	int c;
	short rc;
	char *plogp, *alogp, *nodepp, *nodeap, *pgmp, *maxsizep, *hwaterp;
	defaults_t dflts;
	struct stat statpath;
	char path[MAXPATHLEN +1];

	/* Initialize locale information */
	(void)setlocale(LC_ALL, "");

        /* Initialize message label */
	(void)setlabel("UX:auditlog");

        /* Initialize catalog */
	(void)setcat("uxaudit");

        /* make process EXEMPT */
	if (auditevt(ANAUDIT, NULL, sizeof(aevt_t)) == -1)
	{
         	if (errno == ENOPKG)
		{
                 	(void)pfmt(stderr, MM_ERROR, NOPKG);
                        exit(ADT_NOPKG);
		}
		else
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
		if (lvlequal(&audlvl, &mylvl) == 0)
                	/* SET level if not SYS_AUDIT */
                        if (lvlproc(MAC_SET, &audlvl) == -1) {
				if (errno == EPERM) {
                                        (void)pfmt(stderr, MM_ERROR, NOPERM);
					exit(ADT_NOPERM);
                                }
				(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlproc", errno);
                                exit(ADT_LVLOPER);
			}
	}else
             	if (errno != ENOPKG) {
			(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlproc", errno);
			exit(ADT_LVLOPER);
		}
	
	if ((argvp = (char *)argvtostr(argv)) == NULL) {
		(void)pfmt(stderr, MM_ERROR, BADARGV);
                adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argv[0]),argv[0]);
		exit(ADT_MALLOC);
        }

	/* get current status of auditing */
	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADSTAT, errno);
                adumprec(ADT_AUDIT_CTL,ADT_BADSTAT,strlen(argvp),argvp);
		exit(ADT_BADSTAT);
	}					
     
	/* allocate space for the ppathp, apathp, progp*/
	if (((alog.ppathp=(char *)calloc(MAXPATHLEN+1,sizeof(char *)))==NULL)
	  ||((alog.apathp=(char *)calloc(MAXPATHLEN+1,sizeof(char *)))==NULL)
	  ||((alog.progp=(char *)calloc(MAXPATHLEN+1,sizeof(char *)))==NULL))
	{
		(void)pfmt(stderr, MM_ERROR, BDMALOC);
		adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argvp),argvp);
		exit(ADT_MALLOC);
	}

	/* get current log file attributes */
	if (auditlog(ALOGGET, &alog, sizeof(alog_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADLGET, errno);
		adumprec(ADT_AUDIT_LOG,ADT_BADLGET,strlen(argvp),argvp);
		exit(ADT_BADLGET);
	}

	alog.defpathp=NULL;
	alog.defnodep=NULL;
	alog.defpgmp=NULL;

	/* get current audit buffer attributes */
	if (auditbuf(ABUFGET, &abuf, sizeof(abuf_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADBGET, errno);
                adumprec(ADT_AUDIT_LOG,ADT_BADBGET,strlen(argvp),argvp);
		exit(ADT_BADBGET);
	}

	/*Obtain audit default values from /etc/default/audit*/
	(void)memset(&dflts,NULL,sizeof(defaults_t));
	obt_dflts(&dflts);

	/* display log file attributes */
	if (argc == 1) {
		if (prntlog(&dflts) == 1)
		{
                	adumprec(ADT_AUDIT_LOG,ADT_FMERR,strlen(argvp),argvp);
			exit(ADT_FMERR);
		}
                adumprec(ADT_AUDIT_LOG,ADT_SUCCESS,strlen(argvp),argvp);
		exit(ADT_SUCCESS);
	}

	/* parse command line arguments */
	while ((c = getopt(argc,argv,"P:p:A:a:n:x:v:sd")) != EOF) {
		switch (c) {
		case 'P':
			if (optmask & ADT_UPMASK)
				usage();

			if (actl.auditon) {
				(void)pfmt(stderr, MM_ERROR, FDR6, BIGP);
                		adumprec(ADT_AUDIT_LOG,ADT_ENABLED,strlen(argvp),argvp);
				exit(ADT_ENABLED);
			}

			if ((plogp=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(plogp,optarg);

			optmask |= ADT_UPMASK;
			break;
		case 'p':
			if (optmask & ADT_LPMASK)
				usage();
			if (actl.auditon) {
				(void)pfmt(stderr, MM_ERROR, FDR6, LTLP);
                		adumprec(ADT_AUDIT_LOG,ADT_ENABLED,strlen(argvp),argvp);
				exit(ADT_ENABLED);
			}
			if ((nodepp=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(nodepp,optarg);

			optmask |= ADT_LPMASK;
			break;
		case 'A':
			if (optmask & (ADT_UAMASK | ADT_SMASK | ADT_DMASK))
				usage();

			if ((alogp=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(alogp,optarg);

			optmask |= ADT_UAMASK;
			break;

		case 'a':
			if (optmask & (ADT_LAMASK | ADT_SMASK | ADT_DMASK))
				usage();

			if ((nodeap=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(nodeap,optarg);

			optmask |= ADT_LAMASK;
			break;
		case 'n':
			if (optmask & (ADT_NMASK | ADT_SMASK | ADT_DMASK))
				usage();

			if ((pgmp=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(pgmp,optarg);
			optmask |= ADT_NMASK;
			break;
		case 'x':
			if (optmask & ADT_XMASK)
				usage();
			if ((maxsizep=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(maxsizep,optarg);
			optmask |= ADT_XMASK;
			break;
		case 'v':
			if (optmask & ADT_VMASK)
				usage();
			if ((hwaterp=(char *)malloc(strlen(optarg) + 1))==NULL){
				(void)pfmt(stderr, MM_ERROR, BDMALOC);
				adumprec(ADT_AUDIT_LOG,ADT_MALLOC,strlen(argvp),argvp);
				exit(ADT_MALLOC);
			}
			(void)strcpy(hwaterp,optarg);
			optmask |= ADT_VMASK;
			break;
		case 's':
			if (optmask & (ADT_SMASK | ADT_DMASK | ADT_UAMASK | ADT_LAMASK | ADT_NMASK))
				usage();
			optmask |= ADT_SMASK;
			break;
		case 'd':
			if (optmask & (ADT_DMASK | ADT_SMASK | ADT_UAMASK | ADT_LAMASK | ADT_NMASK))
				usage();
			optmask |= ADT_DMASK;
			break;
		case '?':
		        usage();
		}
	}

	/*The command line is invalid if it contained an arguement*/
	if (optind < argc)
		usage();

	/*Note: The order in which the options are proccessed is critical*/
	/*      to their verification.                                   */

	/*User enter -P option*/
	if (optmask & ADT_UPMASK)
	{
		if ((chk_path(plogp,PRIMARY))) {
               		adumprec(ADT_AUDIT_LOG,ADT_BADSYN,strlen(argvp),argvp);
			exit(ADT_BADSYN);
		}

		(void)strcpy(alog.ppathp,plogp);
		flags |= PPATH;

		if (flags & PSPECIAL)
		{
			/*The primary path is a special character device*/
			/*If a previous invocation of auditlog set a node a name*/
			/*then clear it. If the -p option was entered then the  */
                        /*node name will be evaluated by the -p option code.    */
			if (alog.flags & PNODE)
				(void)memset(alog.pnodep,NULL,ADT_NODESZ);
		}
		else
		{
			/*The primary path is a directory. If a previous invocation */
                        /*set the node name no action is taken. If the -p option was*/
                        /*entered then the node name will be evaluated by the -p    */
                        /*option code. Otherwise retrieve and validate the default  */
                        /*node name AUDIT_NODE.                                     */
			if (!((optmask & ADT_LPMASK) || (alog.flags & PNODE))) 
			{
				if (*(dflts.nodep) != NULL)
				{
					/*retrieve and validate the default node name*/
					if (chk_node(dflts.nodep) == 1) 
					{
						(void)pfmt(stdout, MM_INFO, CHKDFLT, "AUDIT_NODE");
                				adumprec(ADT_AUDIT_LOG,ADT_BADDEFAULT,strlen(argvp),argvp);
						exit(ADT_BADDEFAULT);
					}
					else
					{
						(void)memset(alog.pnodep,NULL,ADT_NODESZ);
						(void)strncpy(alog.pnodep,dflts.nodep,ADT_NODESZ);
						flags |= PNODE;
					}
				}
			}
		}
		
	}
	
	/*User enter -p option*/
	if (optmask & ADT_LPMASK)
	{
		if (chk_node(nodepp) == 1) {
               		adumprec(ADT_AUDIT_LOG,ADT_BADSYN,strlen(argvp),argvp);
			exit(ADT_BADSYN);
		}
		else
		{
			if ((optmask & ADT_UPMASK) || (alog.flags & PPATH)) {
				/*The primary log has been determined to be a valid   */
				/*character special device or a regular file. If it is*/
				/*a character special device ignore the -p option.    */
				/*Otherwise, the pnode name is accepted.              */
				if (stat(alog.ppathp, &statpath)) {
					(void)pfmt(stderr, MM_ERROR, FDR7, alog.ppathp);
                			adumprec(ADT_AUDIT_LOG,ADT_FMERR,strlen(argvp),argvp);
					exit(ADT_FMERR);
				}
				if ((statpath.st_mode & S_IFMT) != S_IFCHR)
				{
					(void)memset(alog.pnodep,NULL,ADT_NODESZ);
					(void)strncpy(alog.pnodep,nodepp,ADT_NODESZ);
					flags |= PNODE;
				}
			}
			else
			{
				/*Determine if the default primary log is a regular */
				/*file or a character special device. If it is a    */
				/*character special device ignore the -p option.    */
				/*Otherwise, the pnode name is accepted.              */
				
				if (*(dflts.defpathp) != NULL)
				{
					if (chk_path(dflts.defpathp,IGNORE) == 1)
					{
						(void)pfmt(stdout, MM_INFO, CHKDFLT,"AUDIT_DEFPATH");
                				adumprec(ADT_AUDIT_LOG,ADT_BADDEFAULT,strlen(argvp),argvp);
						exit(ADT_BADDEFAULT);
					}
					(void)strcpy(path,dflts.defpathp);
				}
				else
					(void)strcpy(path,ADT_DEFPATH);

				if (stat(path, &statpath)) {
					(void)pfmt(stderr, MM_ERROR, FDR7, path);
                			adumprec(ADT_AUDIT_LOG,ADT_FMERR,strlen(argvp),argvp);
					exit(ADT_FMERR);
				}
				if ((statpath.st_mode & S_IFMT) != S_IFCHR) 
				{
					(void)memset(alog.pnodep,NULL,ADT_NODESZ);
					(void)strncpy(alog.pnodep,nodepp,ADT_NODESZ);
					flags |= PNODE;
					(void)strcpy(alog.ppathp,path);
					flags  |= PPATH;
				}
			}
		}
	}

	/*User enter -A option*/
	if (optmask & ADT_UAMASK)
	{
		if (chk_path(alogp,ALTERNATE)) {
               		adumprec(ADT_AUDIT_LOG,ADT_BADSYN,strlen(argvp),argvp);
			exit(ADT_BADSYN);
		}
		(void)strcpy(alog.apathp,alogp);
		onfull |= AALOG;
		flags |= APATH;

		if (flags & ASPECIAL)
		{
			/*The alternate log is a special character device.      */
			/*If a previous invocation of auditlog set a node a name*/
			/*then clear it. If the -a option was entered then the  */
                        /*node name will be evaluated by the -a option code.    */
			if (alog.flags & ANODE)
				(void)memset(alog.anodep,NULL,ADT_NODESZ);
		}
		else
		{
			/*The alternate path is a directory. If a previous invocation*/
                        /*set the node name no action is taken. If the -a option was */
                        /*entered then the node name will be evaluated by the -a     */
                        /*option code. Otherwise retrieve and validate the default   */
                        /*node name AUDIT_NODE.                                      */
			if (!((optmask & ADT_LAMASK) || (alog.flags & ANODE))) 
			{
				if (*(dflts.nodep) != NULL)
				{
					/*retrieve and validate the default node name*/
					if (chk_node(dflts.nodep) == 1) 
					{
						(void)pfmt(stdout, MM_INFO, CHKDFLT, "AUDIT_NODE");
                				adumprec(ADT_AUDIT_LOG,ADT_BADDEFAULT,strlen(argvp),argvp);
						exit(ADT_BADDEFAULT);
					}
					else
					{
						(void)memset(alog.anodep,NULL,ADT_NODESZ);
						(void)strncpy(alog.anodep,dflts.nodep,ADT_NODESZ);
						flags |= ANODE;
					}
				}
			}
		}
		
	}

	/*User enter -a option*/
	if (optmask & ADT_LAMASK)
	{
		if (chk_node(nodeap) == 1) {
                	adumprec(ADT_AUDIT_LOG,ADT_BADSYN,strlen(argvp),argvp);
			exit(ADT_BADSYN);
		}
		else
		{
			if ((optmask & ADT_UAMASK) || (alog.flags & APATH)) {
				/*The alternate log has been determined to be a valid */
				/*character special device or a regular file. If it is*/
				/*a character special device ignore the -a option.    */
				/*Otherwise, the anode name is accepted.              */
				if (stat(alog.apathp, &statpath)) {
					(void)pfmt(stderr, MM_ERROR, FDR7, alog.apathp);
                			adumprec(ADT_AUDIT_LOG,ADT_FMERR,strlen(argvp),argvp);
					exit(ADT_FMERR);
				}
				if ((statpath.st_mode & S_IFMT) != S_IFCHR) 
				{
					(void)memset(alog.anodep,NULL,ADT_NODESZ);
					(void)strncpy(alog.anodep,nodeap,ADT_NODESZ);
					flags |= ANODE;
					onfull |= AALOG;
				}
			}
			else
			{
				/*Determine if the default alternate log is a regular*/
				/*file or a character special device. If it is a     */
				/*character special device ignore the -a option.     */
				/*Otherwise, the anode name is accepted.             */
				
				if (*(dflts.defpathp) != NULL)
				{
					if (chk_path(dflts.defpathp,IGNORE) == 1)
					{
						(void)pfmt(stdout, MM_INFO, CHKDFLT, "AUDIT_DEFPATH");
                				adumprec(ADT_AUDIT_LOG,ADT_BADDEFAULT,strlen(argvp),argvp);
						exit(ADT_BADDEFAULT);
					}
					(void)strcpy(path,dflts.defpathp);
				}
				else
					(void)strcpy(path,ADT_DEFPATH);

				if (stat(path, &statpath)) {
					(void)pfmt(stderr, MM_ERROR, FDR7, path);
                			adumprec(ADT_AUDIT_LOG,ADT_FMERR,strlen(argvp),argvp);
					exit(ADT_FMERR);
				}

				if ((statpath.st_mode & S_IFMT) != S_IFCHR) 
				{
					(void)memset(alog.anodep,NULL,ADT_NODESZ);
					(void)strncpy(alog.anodep,nodeap,ADT_NODESZ);
					flags |= ANODE;
					(void)strcpy(alog.apathp,path);
					flags  |= APATH;
					onfull |= AALOG;
				}
			}
		}
	}

	/*User enter -n option*/
	if (optmask & ADT_NMASK)
	{
		/*The -n option must be used in conjunction with the -A/-a options*/
		/*or the default parameter ADT_LOGFULL in /etc/default/audit      */
		/*is SWITCH.                                                      */
		if ((optmask & (ADT_UAMASK | ADT_LAMASK)) || (strcmp(dflts.logfullp,"SWITCH") == 0))
		{
			if (chk_pgm(pgmp)) {
               			adumprec(ADT_AUDIT_LOG,ADT_BADSYN,strlen(argvp),argvp);
				exit(ADT_BADSYN);
			}
			(void)strcpy(alog.progp,pgmp);
			onfull |= APROG;
			onfull |= AALOG;

			/*If the current invocation of auditlog  or a previous*/
			/*invocation of auditlog did not set a alternate log  */
			/*use the defaults.                                   */
			if (!((flags & APATH) || (alog.flags & APATH))) 
			{
				/*validate the default alternate log*/
				if (*(dflts.defpathp) != NULL)
				{
					if (chk_path(dflts.defpathp,IGNORE) == 1)
					{
						(void)pfmt(stdout, MM_INFO, CHKDFLT, "AUDIT_DEFPATH");
                				adumprec(ADT_AUDIT_LOG,ADT_BADDEFAULT,strlen(argvp),argvp);
						exit(ADT_BADDEFAULT);
					}
					(void)strcpy(alog.apathp,dflts.defpathp);
				}
				else
					(void)strcpy(alog.apathp,ADT_DEFPATH);

				flags  |= APATH;

				/*Determine if the default alternate log is a regular  */
				/*file or a character special device. If it is a       */
				/*character special device ignore the default node name*/
				if (stat(alog.apathp, &statpath)) {
					(void)pfmt(stderr, MM_ERROR, FDR7, alog.apathp);
                			adumprec(ADT_AUDIT_LOG,ADT_FMERR,strlen(argvp),argvp);
					exit(ADT_FMERR);
				}

				if ((statpath.st_mode & S_IFMT) != S_IFCHR) 
				{
					/*retrieve and validate the default node name*/
					if (*(dflts.nodep) != NULL)
					{
						if (chk_node(dflts.nodep) == 1) 
						{
							(void)pfmt(stdout, MM_INFO, CHKDFLT, "AUDIT_NODE");
                					adumprec(ADT_AUDIT_LOG,ADT_BADDEFAULT,strlen(argvp),argvp);
							exit(ADT_BADDEFAULT);
						}
						else
						{
							(void)memset(alog.anodep,NULL,ADT_NODESZ);
							(void)strncpy(alog.anodep,dflts.nodep,ADT_NODESZ);
							flags |= ANODE;
						}
					}
				}
			}
		}
		else
			usage();
	}

	/*User enter -x option*/
	if (optmask & ADT_XMASK)
	{
		rc = chk_size(maxsizep,&dflts);
		if (rc == 1) {
               		adumprec(ADT_AUDIT_LOG,ADT_BADSYN,strlen(argvp),argvp);
			exit(ADT_BADSYN);
		}
		else
		{
			if (rc == 0)
				flags |= PSIZE;
		}
	}

	/*User enter -v option*/
	if (optmask & ADT_VMASK)
	{
		if (set_water(hwaterp)) {
               		adumprec(ADT_AUDIT_LOG,ADT_BADSYN,strlen(argvp),argvp);
			exit(ADT_BADSYN);
		}
	}

	/*User enter -s option*/
	if (optmask & ADT_SMASK)
	{
		onfull |= ASHUT;
		(void)memset(alog.apathp,NULL,MAXPATHLEN +1);
		(void)memset(alog.anodep,NULL,ADT_NODESZ);
		(void)memset(alog.progp,NULL,MAXPATHLEN +1);
	}

	/*User enter -d option*/
	if (optmask & ADT_DMASK)
	{
		onfull |= ADISA;
		(void)memset(alog.apathp,NULL,MAXPATHLEN +1);
		(void)memset(alog.anodep,NULL,ADT_NODESZ);
		(void)memset(alog.progp,NULL,MAXPATHLEN +1);
	}

	/*If auditing is disabled set onerr to the default value        */
	/*AUDIT_LOGERR. If auditing is disabled and the onfull condition*/
	/*is not set, set onfull to the default value AUDIT_LOGFULL.    */
        /*auditlog(ALOGSET, .....) requires a non-zero, valid value for */
	/*onerr and onfull.                                             */
	if (!actl.auditon)
	{
		/*Validate AUDIT_LOGERR and populate alog.onerr*/
		if (*(dflts.logerrp) && !strcmp("SHUTDOWN", dflts.logerrp))
			alog.onerr = ASHUT;
		else 
			/*AUDIT_LOGERR is equal to DISABLE or if AUDIT_LOGERR is null*/
                        /*or invalid the hardcoded default is DISABLE*/ 
			alog.onerr = ADISA; 

		/*If the onfull condition has not yet been set and the current*/
		/*current invocation of auditlog is NOT setting the onfull    */
		/*condition then AUDIT_LOGFULL is used.                       */
		if ((alog.onfull == 0) && (onfull == 0))
		{
			if (*(dflts.logfullp) && !strcmp("SHUTDOWN", dflts.logfullp))
				alog.onfull = ASHUT;
			else
			{
				if (*(dflts.logfullp) && !strcmp("SWITCH", dflts.logfullp))
					alog.onfull = AALOG;
				else 
					/*AUDIT_LOGFULL is equal to DISABLE or if AUDIT_LOGFULL is null*/
                       		 	/*or invalid the hardcoded default is DISABLE*/ 
					alog.onfull = ADISA;
			}
		}
	}


	/*The -A,-n,-s and -d options modify the onfull value.            */
	/*The -a option may or may not modify onfull (if alternate path is*/
	/*a special character device the -a option is ignored).           */
	if ((optmask & (ADT_UAMASK | ADT_NMASK | ADT_SMASK | ADT_DMASK)) || ((optmask & ADT_LAMASK) && (onfull != 0)))
		alog.onfull = onfull;

	alog.flags = flags;

	/* set log file attributes */
	if (auditlog(ALOGSET, &alog, sizeof(alog_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADLSET, errno);
                adumprec(ADT_AUDIT_LOG,ADT_BADLSET,strlen(argvp),argvp);
		exit(ADT_BADLSET);
	}

        adumprec(ADT_AUDIT_LOG,ADT_SUCCESS,strlen(argvp),argvp);
	exit(ADT_SUCCESS);
/*NOTREACHED*/
}

/*
 * Procedure:     chk_path
 *
 * Restrictions:  none
 *
 * Notes:	Check that a full or absolute pathname is given.
 *		It must be less than 1009 characters, exist,
 *		and be to either a regular directory or
 *		character special device. If a character special 
 *		device is specified than the size option is ignored.
 */
int
chk_path(path,log_type)
char *path;
int log_type;
{
	struct stat statpath;
	
	/* absolute pathnames required */
	if (*path != '/') {
		(void)pfmt(stderr, MM_ERROR, FULPATH);
		return(1);
	}			

	if ((int)strlen(path) > ADT_MAXPATHLEN)
	{
		(void)pfmt(stderr, MM_ERROR, BIGPATH);
		return(1);
	}

	/* pathname must exist */
	if (stat(path, &statpath)) {
		(void)pfmt(stderr, MM_ERROR, FDR7, path);
		return(1);
	}else {
		/* a valid path is a directory or character special device */
		if ((statpath.st_mode & S_IFMT) == S_IFDIR)
			return(0);
		else if ((statpath.st_mode & S_IFMT) == S_IFCHR) {
			if (log_type == PRIMARY)
			{
				/*The maximum log size is only associated with the */
				/*primary log not the alternate log. If the primary*/
				/*log is a special character device the maximum log*/
				/*size is not relevant. Inform the user. Reset any */
                                /*previous value to O and if the -x option is      */
				/*entered ignore it.                               */

				flags |= PSPECIAL;
				if (alog.flags & PSIZE || (optmask & ADT_XMASK)) {
					/*Don't want FDR3 to be displayed twice*/
					if (!(optmask & ADT_XMASK))
						(void)pfmt(stdout, MM_WARNING, FDR3);
					alog.maxsize = 0;
				}
			}
			if (log_type == ALTERNATE)
				flags |= ASPECIAL;
			
			return(0);
		}else {
			/*A directory or special character device was not entered*/
			(void)pfmt(stderr, MM_ERROR, FDR7, path);
			return(1);
		}
	}
}

/*
	 * Procedure:     chk_node
	 *
	 * Restrictions:  none
	 *
	 * Notes:	Check that the specified node name is less than
	 * 		eight characters and does not contain a '/'.
	 */
	int
chk_node(node)
char *node;
{
	int i;

	if ((int)strlen(node) > ADT_NODESZ)  {
		(void)pfmt(stderr, MM_ERROR, BIGNODE, ADT_NODESZ + 1);
		return(1);
	}

	/* node name may not contain a SLASH */
	for (i=0; i<ADT_NODESZ; i++) {
		if (*node == '/') {
			(void)pfmt(stderr, MM_ERROR, NOSLASH);
			return(1);
		}
		node++;
	}
	return(0);
}

/*
 * Procedure:     chk_pgm
 *
 * Restrictions:  none
 *
 * Notes:	Check that a full or absolute pathname to the
 *		specified program is supplied and the program
 *		is executable.
 */
int
chk_pgm(pgm)
char *pgm;
{
	struct stat statpgm;
	
	/* absolute pathnames required */
	if (*pgm != '/') {
		(void)pfmt(stderr, MM_ERROR, FULPATH);
		return(1);
	}

	/*Program must exist*/
	/*If pathname is greater than 1024 characters, stat will return*/
	/*ENAMETOOLONG.                                                */
	if (stat(pgm, &statpgm)) {
		(void)pfmt(stderr, MM_ERROR, FDR7, pgm);
		return(1);
	}else {
		/* pgm must be executable by owner */
		if ((statpgm.st_mode & S_IFMT) == S_IFREG) {
			if (statpgm.st_mode & S_IXUSR) 
				return(0);
			else {
				(void)pfmt(stderr, MM_ERROR, FDR8, pgm);
				return(1);
			}
		}else {
			(void)pfmt(stderr, MM_ERROR, NOTREGF, pgm);
			return(1);
		}
	}
}

/*
 * Procedure:     chk_size
 *
 * Restrictions:  none
 *
 * Notes:	Check that the specified log file size is either ZERO,
 *		equal or greater than the size of the audit buffer.
 */
int
chk_size(arg,dflts)
char *arg;
defaults_t *dflts;
{
	struct stat statpath;
	int cnv_maxsize;
	char path[MAXPATHLEN+1];
	short use_dflt=0;

	/*if the primary log file is a character special device ignore the -x option*/
	if ((alog.flags & PPATH) || (flags & PPATH))
		(void)strcpy(path, alog.ppathp);
	else
	{
		if (*(dflts->defpathp) != NULL)
		{
			if (chk_path(dflts->defpathp,IGNORE) == 1)
			{
				(void)pfmt(stdout, MM_INFO, CHKDFLT,"AUDIT_DEFPATH");
                		adumprec(ADT_AUDIT_LOG,ADT_BADDEFAULT,strlen(argvp),argvp);
				exit(ADT_BADDEFAULT);
			}
			(void)strcpy(path, dflts->defpathp);
		}
		else
			(void)strcpy(path, ADT_DEFPATH);

		use_dflt=1;
	}
		
	if (stat(path, &statpath)) {
		(void)pfmt(stderr, MM_ERROR, FDR7, path);
		return(1);
	}

	if ((statpath.st_mode & S_IFMT) == S_IFCHR)
	{
		(void)pfmt(stdout, MM_WARNING, FDR3);
		return(IGNORE);
	}

	/*convert input string to number*/
	if ((cnv_maxsize=satoi(arg)) < 0)
	{
		(void)pfmt(stderr, MM_ERROR, FDR2);
		(void)pfmt(stderr, MM_NOSTD, MORE2, abuf.bsize/BSIZE, BSIZE);
		return(1);
	}

	if ((cnv_maxsize == 0) || ((cnv_maxsize *BSIZE) >= abuf.bsize))
	{
		alog.maxsize=cnv_maxsize * BSIZE;
		/*If the AUDIT_DEFPATH parameter was used in evaluating the -x */
		/*set the primary path to AUDIT_DEFPATH and the primary node to*/
		/*AUDIT_NODE.                                                  */
		if (use_dflt)
		{
			(void)strcpy(alog.ppathp,path);
			flags |= PPATH;
			if (*(dflts->nodep) != NULL)
			{
				/*retrieve and validate the default node name*/
				if (chk_node(dflts->nodep) == 1) 
				{
					(void)pfmt(stdout, MM_INFO, CHKDFLT, "AUDIT_NODE");
                			adumprec(ADT_AUDIT_LOG,ADT_BADDEFAULT,strlen(argvp),argvp);
					exit(ADT_BADDEFAULT);
				}
				else
				{
					(void)memset(alog.pnodep,NULL,ADT_NODESZ);
					(void)strncpy(alog.pnodep,dflts->nodep,ADT_NODESZ);
					flags |= PNODE;
				}
			}
		}
	}
	else
	{
		(void)pfmt(stderr, MM_ERROR, FDR2);
		(void)pfmt(stderr, MM_NOSTD, MORE2, abuf.bsize/BSIZE, BSIZE);
		return(1);
	}
	return(0);
}

/**
 ** Strict atoi conversion : return a negative number if non-digits in string
 **/
int
satoi(s)
char *s;
{
	register int n;
	for (n=0; *s; s++) {
		if (*s < '0' || *s > '9')
			return(-1);
		n = 10*n + (*s - '0');
	}
	return(n);
}

/**

/*
 * Procedure:     set_water
 *
 * Restrictions:  auditbuf(2)
 *
 * Notes:	Check that the specified high water mark is either ZERO,
 *		or less than the size of the audit buffer.
 */
int
set_water(vhigh)
char *vhigh;
{
	int cnv_hwater;
	int tmp_bsize;

	/*convert input string to integer*/
	if ((cnv_hwater=satoi(vhigh)) < 0 )
	{
		(void)pfmt(stderr, MM_ERROR, FDR5);
		(void)pfmt(stderr, MM_NOSTD, MORE5, abuf.bsize);
		return(1);
	}

	/*save the buffer size previously obtained from ABUFGET*/
	tmp_bsize=abuf.bsize;

	abuf.vhigh = cnv_hwater;

	/*validate vhigh in system call*/
	if (auditbuf(ABUFSET, &abuf, sizeof(abuf_t)) == -1) 
	{
		if (errno == EINVAL)
		{
			(void)pfmt(stderr, MM_ERROR, FDR5);
			(void)pfmt(stderr, MM_NOSTD, MORE5, tmp_bsize);
			return(1);
		}
		else
		{
			(void)pfmt(stderr, MM_ERROR, BADBSET, errno);
                	adumprec(ADT_AUDIT_LOG,ADT_BADBSET,strlen(argvp),argvp);
			exit(ADT_BADBSET);
		}
	}
	return(0);
}

/*
 * Procedure:     prntlog
 *
 * Restrictions:  none
 *
 * Notes:	 Display audit log file attributes.
 *               Values retrieved from the /etc/default/audit file are
 *               not validated.
 */
int
prntlog(dflts)
defaults_t *dflts;
{
	struct stat statpath;
	short dflt_char=0;
	char disp_node[FORMAT_MSG];
	char disp_name[FORMAT_MSG];

	/*format the printf statement for the primary/alternate log file name*/
	(void)sprintf(disp_node,"%%.%ds",ADT_NODESZ);
	(void)sprintf(disp_name,"/%%.%ds%%.%ds%%.0%dd",ADT_DATESZ,ADT_DATESZ,ADT_SEQSZ);

	(void)fprintf(stdout,"\n");
	if (actl.auditon)
		(void)pfmt(stdout, MM_NOSTD, ":36:Current Status of Auditing:\t\t\tON \n");
	else
		(void)pfmt(stdout, MM_NOSTD, ":37:Current Status of Auditing:\t\t\tOFF \n");

	(void)pfmt(stdout, MM_NOSTD, ":174:Current Event Log:\t\t\t\t");
	if (alog.flags & PPATH) 
		(void)fprintf(stdout,"%s",alog.ppathp);
	else
	{
		if (*(dflts->defpathp) != NULL)
		{
			if (stat(dflts->defpathp, &statpath)) {
				(void)fprintf(stdout,"%s",ADT_DEFPATH);
			}
			else
			{
				if ((statpath.st_mode & S_IFMT) == S_IFCHR)
					dflt_char=1;
				(void)fprintf(stdout,"%s",dflts->defpathp);
			}
		}
		else
			(void)fprintf(stdout,"%s",ADT_DEFPATH);
	}

	/*If the primary event log is a regular file and:                */
	/*   auditing enabled the default AUDIT_NODE is not displayed.   */
	/*   auditing is not enabled the default AUDIT_NODE is displayed.*/
	if ((actl.auditon) && (!(alog.flags & PSPECIAL))) {
		(void)fprintf(stdout,disp_name,alog.mmp,alog.ddp,alog.seqnum);
		if (alog.flags & PNODE)
			(void)fprintf(stdout,disp_node,alog.pnodep);
	}
	else 
	{
		if ((!(alog.flags & PSPECIAL)) && (dflt_char == 0)) 
                {
			(void)fprintf(stdout,"/MMDD###");
			if (alog.flags & PNODE)
				(void)fprintf(stdout,disp_node,alog.pnodep);
			else
			{
				if (*(dflts->nodep) != NULL)
					(void)fprintf(stdout,"%s",dflts->nodep);
			}
		}
	}
	(void)fprintf(stdout,"\n");

	(void)pfmt(stdout, MM_NOSTD, ":39:Current Audit Buffer High Water Mark:\t\t");
	(void)pfmt(stdout, MM_NOSTD, ":40:%d bytes\n",abuf.vhigh);

	(void)pfmt(stdout, MM_NOSTD, ":41:Current Maximum File Size Setting:\t\t");
	if (alog.flags & PSIZE)
		(void)pfmt(stdout, MM_NOSTD, ":42:%d blocks\n",alog.maxsize/BSIZE);
	else
		(void)pfmt(stdout, MM_NOSTD, ":43:none\n");

	(void)pfmt(stdout, MM_NOSTD, ":44:Action To Be Taken Upon Full Event Log:\t\t");
	if (alog.onfull & ASHUT) 
		(void)pfmt(stdout, MM_NOSTD, ":45:system shutdown");
	else if (alog.onfull & AALOG) 
		(void)pfmt(stdout, MM_NOSTD, ":46:log switch");
	else if (alog.onfull & ADISA) 
		(void)pfmt(stdout, MM_NOSTD, ":47:auditing disabled");
	     else 
		/*Display AUDIT_LOGFULL value in /etc/default/audit*/
		/*If AUDIT_LOGFULL is invalid, missing or commented*/
		/*out display the default value.                   */
		prtdflt(dflts->logfullp);

	(void)fprintf(stdout,"\n");

	(void)pfmt(stdout, MM_NOSTD, ":48:Action To Be Taken Upon Error:\t\t\t");
	if (alog.onerr & ASHUT) 
		(void)pfmt(stdout, MM_NOSTD, ":45:system shutdown");
	else if (alog.onerr & ADISA) 
		(void)pfmt(stdout, MM_NOSTD, ":47:auditing disabled");
	else 
		/*Display AUDIT_LOGERR value in /etc/default/audit*/
		/*If AUDIT_LOGERR is invalid, missing or commented*/
		/*out display the default value.                  */
		prtdflt(dflts->logerrp);

	(void)fprintf(stdout,"\n");

	(void)pfmt(stdout, MM_NOSTD, ":49:Next Event Log To Be Used:\t\t\t"); 

	/*If the APATH flag is set then onfull is equal to "SWITCH"*/
	if (alog.flags & APATH)  {
		(void)fprintf(stdout,"%s",alog.apathp);
		if (!(alog.flags & ASPECIAL))
		{
			(void)fprintf(stdout,"/MMDD###");
			if (alog.flags & ANODE)
				(void)fprintf(stdout,disp_node,alog.anodep);
			else
			{
				if (*(dflts->nodep) != NULL)
					(void)fprintf(stdout,"%s",dflts->nodep);
			}
		}
		(void)fprintf(stdout,"\n");
	}
	else
	{
		/*If the default logfull action is "SWITCH" then display the*/
		/*default alternate path and node. Otherwise display "none" */
		if (strcmp(dflts->logfullp,"SWITCH") == 0)
		{
			dflt_char = 0;
			if (*(dflts->defpathp) != NULL)
			{
				if (stat(dflts->defpathp, &statpath)) 
					(void)fprintf(stdout,"%s",ADT_DEFPATH);
				else
				{
					if ((statpath.st_mode & S_IFMT) == S_IFCHR)
						dflt_char=1;
					(void)fprintf(stdout,"%s",dflts->defpathp);
				}
			}
			else
				(void)fprintf(stdout,"%s",ADT_DEFPATH);

			/*If the default alternate path is a directory display*/
			/*MMDD### and the node name.                          */
			if (dflt_char == 0)
			{
				(void)fprintf(stdout,"/MMDD###");
				/*The ANODE flag can not be set without the APATH flag being set.*/
				if (*(dflts->nodep) != NULL)
					(void)fprintf(stdout,"%s",dflts->nodep);
			}
			(void)fprintf(stdout,"\n");
		}
		else
			(void)pfmt(stdout, MM_NOSTD, ":43:none\n");
	}

	(void)pfmt(stdout, MM_NOSTD, ":50:Program To Run When Event Log Is Full:\t\t");
	if (alog.onfull & APROG) 
		(void)fprintf(stdout,"%s\n",alog.progp);
	else
	{ 
                if (((strcmp(dflts->logfullp,"SWITCH") == 0) || (alog.onfull & AALOG)) && (*(dflts->pgmp) != NULL))
			(void)fprintf(stdout,"%s\n",dflts->pgmp);
		else
			(void)pfmt(stdout, MM_NOSTD, ":43:none\n");
	}

	(void)fprintf(stdout,"\n");
	return(0);
}

/*
 * Procedure:     usage
 *
 * Restrictions:  none
 */
void
usage()
{
	(void)pfmt(stderr, MM_ACTION, USAGE1);
	(void)pfmt(stderr, MM_NOSTD, USAGE2);
        adumprec(ADT_AUDIT_LOG,ADT_BADSYN,strlen(argvp),argvp);
	exit(ADT_BADSYN);
}

/*
 * Procedure:	  adumprec
 *
 * Restrictions:  auditdmp(2): none
 *
 * Notes:	USER level interface to auditdmp(2)
 *		for USER level audit event records
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

/*
 * Procedure:     obtain AUDIT default values
 *
 * Restrictions:  none
 *
*/
void
obt_dflts(dflts)
defaults_t *dflts;
{
	FILE *def_fp;
	char deflt_file[MAXPATHLEN + 1];

	/*format the pathname of the default audit file*/
	sprintf(deflt_file,"%s/%s",DEFLT,ADT_DEFLTFILE);

	if ((def_fp = defopen(deflt_file)) != NULL) 
	{
		if ((dflts->logerrp = defread(def_fp, "AUDIT_LOGERR")) != NULL) 
	    		dflts->logerrp = strdup(dflts->logerrp);

		if ((dflts->logfullp = defread(def_fp, "AUDIT_LOGFULL")) != NULL) 
	    		dflts->logfullp = strdup(dflts->logfullp);

		if ((dflts->pgmp = defread(def_fp, "AUDIT_PGM")) != NULL) 
	    		dflts->pgmp = strdup(dflts->pgmp);

		if ((dflts->defpathp = defread(def_fp, "AUDIT_DEFPATH")) != NULL) 
	    		dflts->defpathp = strdup(dflts->defpathp);

		if ((dflts->nodep = defread(def_fp, "AUDIT_NODE")) != NULL) 
	    		dflts->nodep = strdup(dflts->nodep);
		(void) defclose(def_fp);
	}
	else 
		/* No default file */
		(void)pfmt(stdout, MM_WARNING, NACCESS, deflt_file);
	/* 
	 * Need to guard against missing entries, strdup failures
	 * and missing file.
	 */
	if (!dflts->nodep)
		dflts->nodep = &nullchar;
	if (!dflts->defpathp)
		dflts->defpathp = &nullchar;
	if (!dflts->pgmp)
		dflts->pgmp = &nullchar;
	if (!dflts->logfullp)
		dflts->logfullp = &nullchar;
	if (!dflts->logerrp)
		dflts->logerrp = &nullchar;
}

void
prtdflt(action_valp)
char *action_valp;
{
	/*If AUDIT_LOGFULL or AUDIT_LOGERR entry is missing or commented*/
	/*out in the /etc/default/audit display the default value.      */
	if (*action_valp == NULL)
	{
		(void)pfmt(stdout, MM_NOSTD, ":47:auditing disabled");
		return;
	}

	if (strcmp(action_valp,"SHUTDOWN") == 0)
		(void)pfmt(stdout, MM_NOSTD, ":45:system shutdown");
	else
	{
		if (strcmp(action_valp,"DISABLE") == 0)
			(void)pfmt(stdout, MM_NOSTD, ":47:auditing disabled");
		else
		{
			if (strcmp(action_valp,"SWITCH") == 0)
				(void)pfmt(stdout, MM_NOSTD, ":46:log switch");
			else
				(void)pfmt(stdout, MM_NOSTD, ":47:auditing disabled");
		}
	}
	return;
}

