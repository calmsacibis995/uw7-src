/*		copyright	"%c%" 	*/

#ident	"@(#)auditon.c	1.3"
#ident  "$Header$"

/***************************************************************************
 * Command: auditon
 * Inheritable Privileges: P_DACREAD,P_MACWRITE,P_SETPLEVEL,P_AUDIT
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

/*
 * Usage:	auditon
 *
 * Level:	resides at SYS_PRIVATE
 * 		executes at SYS_AUDIT
 *
 * Files:	/etc/default/audit
 *
 * Notes:	If not previously set, initialize the following
 *		default conditions: AUDIT_LOGERR, AUDIT_LOGFULL,
 *		AUDIT_PGM, AUDIT_NODE and AUDIT_DEFPATH.
 *		Determine the offset in seconds from GMT for the
 *		local TIMEZONE and store in internal structure.
 *		Enable the Auditing subsystem and execute the
 *		auditmap command.
*/
/*LINTLIBRARY*/
#include	<stdio.h>
#include	<errno.h>
#include	<string.h>
#include	<sys/types.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<audit.h>
#include	<deflt.h>
#include	<time.h>
#include	<mac.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<stdlib.h>
#include	<sys/wait.h>

/* Fault Detection and Recovery */
#define USAGE	":65:usage: auditon\n"
#define FDR1	":66:cannot access event log"
#define FDR2	":67:Auditing already enabled\n"
#define FDR3	":68:Auditing enabled %s\n"
#define FDR4	":69:the maximum (999) number of audit event log files for a given day exist\n"
#define FDR5	":70:exec of %s failed\n"

#define NOPERM ":17:Permission denied\n"
#define BADARGV ":18:argvtostr() failed\n"
#define NOERR	":71:none or invalid AUDIT_LOGERR=value found in %s\n" 
#define NACCESS	":32:cannot access %s\n"
#define NOFULL	":72:none or invalid AUDIT_LOGFULL=value found in %s\n" 
#define MISPATH	":73:none or invalid AUDIT_DEFPATH=value found in %s\n" 
#define BADFORK	":74:fork() failed\n"
#define BDMALOC	":75:unable to malloc space\n"

#define BADSTAT	":20:auditctl() failed ASTATUS, errno = %d\n"
#define BADLGET	":21:auditlog() failed ALOGGET, errno = %d\n"
#define BADLSET	":23:auditlog() failed ALOGSET, errno = %d\n"
#define UNKNOWN	":76:Internal error, errno = %d\n"
#define NOPKG   ":34:system service not installed\n"
#define A_TERMINATE ":79:auditing abnormally terminated %s\n"
#define LVLOPER ":35:%s() failed, errno = %d\n"


#define FORMAT_MSG	100

extern char *argvtostr();
static	void	adumprec(), getdeflts(), getgmtoff(), vset_dflts();
static	int	obt_dflts(),v_path(),v_node(),v_pgm();

static actl_t	actl;		/* auditctl(2) structure */
static alog_t	alog;		/* auditlog(2) structure */
static int dflt_special;	/* AUDIT_DEFPATH is special character device*/
static char *argvp;
static char deflt_file[MAXPATHLEN+1];

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
                 auditlog(2): None
                 sprintf: None
                 execv: None
 *
 * Notes:	  Initialize and enable auditing.
*/
main(argc,argv)
char **argv;
{
	extern int errno;
	int pid,status;
	level_t mylvl, lvl;
	static int CHILD=0;
	char disp_line[FORMAT_MSG];
	char *pgmp[2];
	char *linep;
	char logfile[MAXPATHLEN + 1];
	char savepath[MAXPATHLEN + 1];

	/* Initialize locale information */
 	(void)setlocale(LC_ALL, "");

	/* Initialize message label */
	(void)setlabel("UX:auditon");

	/* Initialize catalog */
	(void)setcat("uxaudit");

        /* make process EXEMPT */
	if (auditevt(ANAUDIT, NULL, sizeof(aevt_t)) == -1){
         	if (errno == ENOPKG){
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
		if (lvlin(SYS_AUDIT, &lvl) == -1) {
			(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlin", errno);
			exit(ADT_LVLOPER);
		}
		if (lvlequal(&lvl, &mylvl) == 0){
                	/* SET level if not SYS_AUDIT */
                        if (lvlproc(MAC_SET, &lvl) == -1) {
				if (errno == EPERM) {
                 			(void)pfmt(stderr, MM_ERROR, NOPERM);
                        		exit(ADT_NOPERM);
				}
				(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlproc", errno);
				exit(ADT_LVLOPER);
			}
		}
	}else
             	if (errno != ENOPKG) {
			(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlproc", errno);
			exit(ADT_LVLOPER);
		}

	if (( argvp = (char *)argvtostr(argv)) == NULL) {
		(void)pfmt(stderr, MM_ERROR, BADARGV);
		adumprec(ADT_AUDIT_CTL,ADT_MALLOC,strlen(argv[0]),argv[0]);
                exit(ADT_MALLOC);
        }

	/* no command line arguments allowed */
	if (argc > 1) {
		(void) pfmt(stderr, MM_ACTION, USAGE);
		adumprec(ADT_AUDIT_CTL,ADT_BADSYN,strlen(argvp),argvp);
		exit(ADT_BADSYN);
	}

	/* get status of auditing */
	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADSTAT, errno);
		adumprec(ADT_AUDIT_CTL,ADT_BADSTAT,strlen(argvp),argvp);
		exit(ADT_BADSTAT);		
	} 
	if (actl.auditon) {
		(void)pfmt(stdout, MM_WARNING, FDR2);
		adumprec(ADT_AUDIT_CTL,ADT_ENABLED,strlen(argvp),argvp);
		exit(ADT_SUCCESS);		
	}

	/* allocate space for the ppathp, apathp, progp and defpathp */
	/* defnodep and defpgmp.                                    */
	if (((alog.ppathp=(char *)calloc(MAXPATHLEN+1,sizeof(char)))==NULL)
          ||((alog.apathp=(char *)calloc(MAXPATHLEN+1,sizeof(char)))==NULL)
          ||((alog.progp=(char *)calloc(MAXPATHLEN+1,sizeof(char)))==NULL)
          ||((alog.defpathp=(char *)calloc(MAXPATHLEN+1,sizeof(char)))==NULL)
          ||((alog.defnodep=(char *)calloc(ADT_NODESZ+1,sizeof(char)))==NULL)
          ||((alog.defpgmp=(char *)calloc(MAXPATHLEN+1,sizeof(char)))==NULL)) {
		(void)pfmt(stderr, MM_ERROR, BDMALOC);
		exit(ADT_MALLOC);
	}

	/* get the current audit log file attributes */
	if (auditlog(ALOGGET, &alog, sizeof(alog_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADLGET, errno);
		exit(ADT_BADLGET);
	}

	/*format the pathname of the default audit file*/
        sprintf(deflt_file,"%s/%s",DEFLT,ADT_DEFLTFILE);

	/* read /etc/default/audit default parameters.*/
	getdeflts();

	/* set new audit log file attributes */
	if (auditlog(ALOGSET, &alog, sizeof(alog_t)) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADLSET, errno);
		exit(ADT_BADLSET);
	}

	/* Get the GMT offset in seconds */
	getgmtoff();

	/* enable auditing */
	if (auditctl(AUDITON, &actl, sizeof(actl_t)) == -1) {
		/* get the audit log control structure again */
		if (auditlog(ALOGGET, &alog, sizeof(alog_t)) == -1) {
			(void)pfmt(stderr, MM_ERROR, BADLGET, errno);
			exit(ADT_BADLGET);		
		} 
		switch(errno) {
		case EACCES:
		case ENOENT:
			if (*alog.ppathp == NULL)
			{
				(void)sprintf(disp_line,"%s %%s\n",FDR1);
				(void)pfmt(stderr, MM_ERROR, disp_line, ADT_DEFPATH);
			}
			else {
				if (!(alog.flags & PSPECIAL)) {
					(void)sprintf(disp_line,"%s %%.%ds/%%.%ds%%.%ds%%.0%dd%%.%ds\n",FDR1,
						strlen(alog.ppathp),ADT_DATESZ,ADT_DATESZ,ADT_SEQSZ,ADT_NODESZ);
					(void)pfmt(stderr, MM_ERROR, disp_line, alog.ppathp,
						alog.mmp, alog.ddp, alog.seqnum, alog.pnodep); 
				}
				else {
					(void)sprintf(disp_line,"%s %%s\n",FDR1);
					(void)pfmt(stderr, MM_ERROR, disp_line, alog.ppathp);
				}
			}
			break;
		case EINVAL:
			(void)pfmt(stdout, MM_WARNING, FDR2);
			adumprec(ADT_AUDIT_CTL,ADT_ENABLED,strlen(argvp),argvp);
			exit(ADT_ENABLED);		
			break;
		case EEXIST:
			(void)pfmt(stderr, MM_ERROR, FDR4);
			break;
		default:
			(void)pfmt(stderr, MM_ERROR, UNKNOWN, errno);
			break;
		}
		exit(ADT_INVALID);
	} 

	/* exec /usr/sbin/auditmap program */
	if ((pid = fork()) != -1) {
		if (pid == CHILD) {
			linep=strrchr(ADT_MAPPGM,'/');
			pgmp[0]= linep++;
			pgmp[1]='\0';
			(void)execv(ADT_MAPPGM,(char **)pgmp);
			(void)pfmt(stderr,MM_ERROR,FDR5,ADT_MAPPGM);
			adumprec(ADT_AUDIT_MAP,ADT_BADEXEC,strlen(argvp),argvp);
			exit(ADT_BADEXEC);
		}
	}else {
       		(void)pfmt(stderr, MM_ERROR, BADFORK);
		adumprec(ADT_AUDIT_CTL,ADT_BADFORK,strlen(argvp),argvp);
		exit(ADT_BADFORK);
	}

	(void)wait(&status);

	/* check status of auditing again, in this case a cmd of ASTATUS*/
	/* will not return non-zero.                                    */
	auditctl(ASTATUS, &actl, sizeof(actl_t));

	/*Auditing may have been disabled by auditoff, logfull or logerr*/
	/*condition of DISABLE                                          */
	if (actl.auditon) {
		/* make process EXEMPT */
		auditevt(ANAUDIT, NULL, sizeof(aevt_t)); 


		/*If the audit log file is a regular file then change the*/
		/*owner & group equal to that of its directory.          */
		if (!(alog.flags & PSPECIAL)) {
			/*Save the log file path - in case auditlog(ALOGGET,..) fails*/
			(void)strcpy(savepath,alog.ppathp);

			/*Retrieve the audit log file attributes - need the*/
                        /*sequence number of the current log file.         */
			if (auditlog(ALOGGET, &alog, sizeof(alog_t)) == -1) {
				(void)pfmt(stdout, MM_WARNING, BADLGET, errno);
				adumprec(ADT_AUDIT_LOG,ADT_BADLGET,strlen(argvp),argvp);
				(void)pfmt(stdout, MM_INFO, FDR3, savepath);
			}
			else {
				/*Construct the absolute pathname of the log file*/
				(void)sprintf(disp_line,"%%.%ds/%%.%ds%%.%ds%%.0%dd%%.%ds",strlen(alog.ppathp),
					ADT_DATESZ,ADT_DATESZ,ADT_SEQSZ,ADT_NODESZ);
				(void)sprintf(logfile,disp_line,alog.ppathp,alog.mmp,alog.ddp,alog.seqnum,alog.pnodep);

				(void)pfmt(stdout, MM_INFO, FDR3, logfile);
			}
		}else
			(void)pfmt(stdout, MM_INFO, FDR3, alog.ppathp);

		adumprec(ADT_AUDIT_CTL,ADT_SUCCESS,strlen(argvp),argvp);		
	}
	else {
		if (!(alog.flags & PSPECIAL)) {
			/*Save the log file path - in case auditlog(ALOGGET,..) fails*/
			(void)strcpy(savepath,alog.ppathp);

			/*Retrieve the audit log file attributes - need the*/
                        /*sequence number of the last log file used.       */
			if (auditlog(ALOGGET, &alog, sizeof(alog_t)) == -1) {
				(void)pfmt(stdout, MM_WARNING, BADLGET, errno);
				adumprec(ADT_AUDIT_LOG,ADT_BADLGET,strlen(argvp),argvp);
				(void)pfmt(stderr, MM_ERROR, A_TERMINATE, savepath);
			}
			else {
				(void)sprintf(disp_line,"%%.%ds/%%.%ds%%.%ds%%.0%dd%%.%ds",strlen(alog.ppathp),
					ADT_DATESZ,ADT_DATESZ,ADT_SEQSZ,ADT_NODESZ);
				(void)sprintf(logfile,disp_line,alog.ppathp,alog.mmp,alog.ddp,alog.seqnum,alog.pnodep);
				(void)pfmt(stderr, MM_ERROR, A_TERMINATE, logfile);
			}
		}else
			(void)pfmt(stderr, MM_ERROR, A_TERMINATE, alog.ppathp);

		exit(ADT_INVALID);
	}
	exit(ADT_SUCCESS);
/*NOTREACHED*/
}


/*
 * Procedure:     adumprec
 *
 * Restrictions:  auditdmp(2): None
 *
 * Notes:	USER level interface to auditdmp(2)
 * 		for USER level audit event records
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
 * Procedure:     getgmtoff
 *
 * Restrictions: 
                 ctime: none
                 localtime: none
 *
 * Notes:	get the seconds of GMT
*/
void
getgmtoff()
{
	static	time_t	clock;
	extern long timezone, altzone;
	extern int daylight;

	(void)time(&clock);
	(void *)ctime(&clock);
	if (daylight)
	{
		if(localtime(&clock)->tm_isdst)
			actl.gmtsecoff=altzone;
		else
			actl.gmtsecoff=timezone;
	}
	else
		actl.gmtsecoff=timezone;
	return;
}

/*
 * Procedure:     getdeflts
 *
 * Restrictions:  none
 *
 * Notes:	read, verify and set the audit default values.
*/
void
getdeflts()
{
	defaults_t dflts;
	
	(void)memset(&dflts,NULL,sizeof(defaults_t));

	/*obtain audit default values from /var/default/audit*/
	if (obt_dflts(&dflts) == 1)
	{
		/*Unable to open /etc/default/audit file*/
		/*Set kernel defaults: defpathp and defonfull with hardcoded default values.*/
		/*Set kernel defaults: defnodep and defpgmp to NULL.                        */
		(void)strcpy(alog.defpathp,ADT_DEFPATH);
		alog.defonfull=ADISA;
		alog.defnodep=NULL;
		alog.defpgmp=NULL;

		/*Set the LOGERR condition to the hardcoded default value of disable*/
		alog.onerr = ADISA;
	}
	else
		/*verify default values in /var/default/audit*/
		vset_dflts(&dflts);


	/*If the user set the logfull condtion (auditlog -s|-d|-A|a) since auditing*/
	/*has been disabled, do NOT override the new settings.                     */
	if ((alog.onfull == 0) || ((alog.onfull == AALOG) && !(alog.flags & APATH)))
	{
		alog.onfull=alog.defonfull;

		/*If the default file did not exist or could not be read, the onfull*/
		/*condition is ADISA.                                               */
		if (alog.onfull == AALOG)
		{
			/*Set the alternate path and node name*/
			(void)strcpy(alog.apathp, alog.defpathp);
			alog.flags |= APATH;

			if (!dflt_special)
			{
				if (alog.defnodep != NULL)
				{
					(void)strncpy(alog.anodep,alog.defnodep,ADT_NODESZ);
					alog.flags |= ANODE;
				}
			}
			else
				alog.flags |= ASPECIAL;

			/*Set the alternate program*/
			if (alog.defpgmp != NULL)
			{
				(void)strcpy(alog.progp,alog.defpgmp);
				alog.onfull |= APROG;
			}
		}
	}
		
	/*If the user set the primary path (auditlog -P|-p) since auditing*/
	/*has been disabled, do NOT override the new settings.                     */
	if (!(alog.flags & PPATH))
	{
		alog.flags |= PPATH;
		(void)strcpy(alog.ppathp, alog.defpathp);

		if (!dflt_special)
		{
			if (alog.defnodep != NULL)
			{
				(void)strncpy(alog.pnodep,alog.defnodep,ADT_NODESZ);
				alog.flags |= PNODE;
			}
		
		}
		else
			alog.flags |= PSPECIAL;
	}
}

/*
 * Procedure:     obt_dflts
 *
 * Restrictions: 
                 defopen:  none
 *
 * Notes:         obtain AUDIT default values
*/
int
obt_dflts(dflts)
defaults_t *dflts;
{
	FILE *def_fp;

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
		return(0);
	}
	else 
	{
		/* No default file */
		(void)pfmt(stdout, MM_WARNING, NACCESS, deflt_file);
		return(1);
	}
}

/*
 * Procedure:     vset_dflts
 *
 * Restrictions:  none
 *
 * Notes:         validate and set AUDIT default values
*/
void
vset_dflts(dflts)
defaults_t *dflts;
{

	/*Validate the default parameters in /etc/default/audit*/

	/*Validate AUDIT_LOGERR and populate alog.onerr*/
	if (dflts-> logerrp && *(dflts->logerrp) && !strcmp("SHUTDOWN", dflts->logerrp))
		alog.onerr = ASHUT;
	else 
	{
		if (dflts-> logerrp && *(dflts->logerrp) && !strcmp("DISABLE", dflts->logerrp))
			alog.onerr = ADISA;
		else {
			(void)pfmt(stdout, MM_WARNING, NOERR, deflt_file);
			/* No AUDIT_LOGERR value */
			alog.onerr = ADISA; 
		}
	}


	/*Validate AUDIT_LOGFULL and populate kernel default defonfull*/
	if (dflts->logfullp && *(dflts->logfullp) && !strcmp("SHUTDOWN", dflts->logfullp))
		alog.defonfull = ASHUT;
	else
	{
		if (dflts->logfullp && *(dflts->logfullp) && !strcmp("DISABLE", dflts->logfullp))
			alog.defonfull = ADISA;
		else 
		{
			if (dflts->logfullp && *(dflts->logfullp) && !strcmp("SWITCH", dflts->logfullp))
				alog.defonfull = AALOG;
			else
			{
				(void)pfmt(stdout, MM_WARNING, NOFULL, deflt_file);
			 	/* No AUDIT_LOGFULL value */
				alog.defonfull = ADISA;
			}
		
		}
	}

	/*Validate AUDIT_DEFPATH and populate kernel default defpathp.*/
	if (dflts->defpathp && *(dflts->defpathp))
	{
		if (v_path(dflts->defpathp) == 1)
		{
			(void)pfmt(stdout, MM_WARNING, MISPATH, deflt_file);
			(void)strcpy(alog.defpathp,ADT_DEFPATH);
		}
		else
			(void)strcpy(alog.defpathp,dflts->defpathp);
	}
	else
	{
		(void)pfmt(stdout, MM_WARNING, MISPATH, deflt_file);
		(void)strcpy(alog.defpathp,ADT_DEFPATH);
	}

	/*If the default path (AUDIT_DEFPATH) is a special character device ignore the */
	/*default parameter AUDIT_NODE and defnodep is NULL.                           */
	if (!dflt_special && dflts->nodep && *(dflts->nodep) &&
	    (v_node(dflts->nodep) == 0)) {
		(void)strcpy(alog.defnodep,dflts->nodep);
	} else {
		alog.defnodep=NULL;
	}

	if (dflts->pgmp && *(dflts->pgmp))
	{
		if (v_pgm(dflts->pgmp) == 1) 
			alog.defpgmp=NULL;
		else {
			(void)strcpy(alog.defpgmp,dflts->pgmp);
			if (alog.progp == NULL) {
				(void)strcpy(alog.progp,dflts->pgmp);
				alog.defonfull |= APROG;
			}
		}
	}
	else
		alog.defpgmp=NULL;

	return;
}
/*
 * Procedure:     v_path
 *
 * Restrictions:  stat(2): none
 *
 * Notes:         validate AUDIT_DEFPATH value
*/
int
v_path(pathp)
char *pathp;
{
	struct stat statpath;
	
	/* absolute pathnames required */
	if (*pathp != '/') 
		return(1);

	if ((int)strlen(pathp) > ADT_MAXPATHLEN)
		return(1);

	/* pathname must exist */
	if (stat(pathp, &statpath))
		return(1);
	else {
		/* a valid path is a directory or character special device */
		if ((statpath.st_mode & S_IFMT) == S_IFDIR)
			return(0);
		else 
		{
                        if ((statpath.st_mode & S_IFMT) == S_IFCHR)
			{
				dflt_special = 1;
				return(0);
			}
			else
				/*The default path is not a pecial character device or directory*/
				return(1);
		}
	}
}
/*
 * Procedure:     v_node
 *
 * Restrictions:  none
 *
 * Notes:         validate AUDIT_NODE value
*/
int
v_node(nodep)
char *nodep;
{
	int i;

	if ((int)strlen(nodep) > ADT_NODESZ)
		return(1);

	/* node name may not contain a SLASH */
	for (i=0; i<ADT_NODESZ; i++,nodep++) {
		if (*nodep == '/')
			return(1);
	}
	return(0);
}
/*
 * Procedure:     v_pgm
 *
 * Restrictions:  stat(2): none
 *
 * Notes:         validate AUDIT_PGM value
*/
int
v_pgm(pgmp)
char *pgmp;
{
	struct stat statpgm;
	
	/* absolute pathnames required */
	if (*pgmp != '/') 
		return(1);

	/*Program must exist*/
	/*If pathname is greater than 1024 characters, stat will return*/
	/*ENAMETOOLONG.                                                */
	if (stat(pgmp, &statpgm)) 
		return(1);
        else 
        {
		/* pgm must be executable by owner */
		if ((statpgm.st_mode & S_IFMT) == S_IFREG) {
			if (statpgm.st_mode & S_IXUSR) 
				return(0);
			else
				return(1);
		}else 
			return(1);
	}
}
