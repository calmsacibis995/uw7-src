/*		copyright	"%c%" 	*/

#ident	"@(#)auditcnv.c	1.3"


/***************************************************************************
 * Command: auditcnv
 * Inheritable Privileges: none
 *	 Fixed Privileges: none
 *
 * Usage:	auditcnv
 *
 * Level:	USER_PUBLIC
 *
 * Files:	/etc/default/useradd
 *		/etc/passwd
 *		/etc/shadow
 *		/etc/security/ia/audit
 *
 * Notes:	Must be able to access useradd and passwd files
 *		and the audit file must not previously exist.
 *		Create an entry in the audit file for each user
 *		in the passwd file with the AUDIT_MASK value
 *		found in the useradd file. The level of this
 *		audit file must be SYS_PRIVATE for defadm.
 **************************************************************************/
/*LINTLIBRARY*/
#include	<pwd.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<ia.h>
#include	<shadow.h>
#include	<errno.h>
#include	<string.h>
#include	<audit.h>
#include	<deflt.h>
#include	<mac.h>
#include	<pfmt.h>
#include	<locale.h>
#include	<stdlib.h>
#include	<sys/param.h>

/* Fault Detection and Recovery */
#define USAGE	":1:usage: auditcnv\n"
#define	FDR1	":2:file %s does not exist\n"
#define	FDR2	":3:cannot access file %s\n"
#define	FDR3	":4:cannot create audit mask file\n"
#define	FDR4	":5:audit mask file already exists\n"
#define	FDR5	":6:/etc/security/ia/audit created\n"
#define BADSTAT ":7:unable to stat() %s, errno = %d\n"
#define NONEMASK ":8:a default audit mask of none was set for all users\n"

extern int cremask(), putadtent();
extern struct	passwd *getpwent(void);
aevt_t	aevt;

/*
 * Procedure:     main
 *
 * Restrictions:  none
 */
main(argc,argv)
/*ARGSUSED*/
int argc;
char **argv;
{
	register char *Amask=NULL;
	struct  passwd  *pwdp;
	struct	adtuser	*adt, a_usrnamep;	/* default entry */
	struct stat buf;
	FILE	*adt_fp;
	FILE	*def_fp;
	int end_of_file = 0;
	uid_t adt_uid, adt_gid;
	level_t adt_level;
	int mcnt;
	int deflterr=0;		
	static char *Usrdefault = "useradd";
	char deflt_file[MAXPATHLEN+1];
	
	/* Initialize locale information */
	(void)setlocale(LC_ALL, "");

        /* Initialize message label */
	(void)setlabel("UX:auditcnv");

        /* Initialize catalog */
	(void)setcat("uxaudit");						

	/* No argument can be passed to the command*/
	if (argc > 1) {
		(void)pfmt(stderr, MM_ACTION, USAGE);
		exit(ADT_BADSYN);
	}

	/*format the pathname of the /etc/default/useradd file*/
	sprintf(deflt_file,"%s/%s",DEFLT,Usrdefault);

	/* check status of the /etc/default/useradd file */
	if (stat(deflt_file, &buf) == -1) {
		switch(errno) {
			case ENOENT:
				(void)pfmt(stderr, MM_ERROR, FDR1, deflt_file);
				break;
			case EACCES:
				(void)pfmt(stderr, MM_ERROR, FDR2, deflt_file);
				break;
			default:
				(void)pfmt(stderr, MM_ERROR, BADSTAT, deflt_file, errno);
				break;
		}
		exit(ADT_FMERR);
	} 
	/*
	 * check  if the "/etc/passwd" file exists.  This is
	 * where we get the names of all the users from.  If
	 * it doesn't exist, then it's an error.
	*/
	if (stat(PASSWD, &buf) < 0) {
		switch(errno) {
			case ENOENT:
				(void)pfmt(stderr, MM_ERROR, FDR1, PASSWD);
				break;
			case EACCES:
				(void)pfmt(stderr, MM_ERROR, FDR2, PASSWD);
				break;
			default:
				(void)pfmt(stderr, MM_ERROR, BADSTAT, PASSWD, errno);
				break;
		}
		exit(ADT_FMERR);
	} 
	/*
	 * stat the file /etc/shadow since these are the attributes we
	 * want for the new audit file.
	*/
	if (stat(SHADOW, &buf) < 0) {
		switch(errno) {
			case ENOENT:
				(void)pfmt(stderr, MM_ERROR, FDR1, SHADOW);
				break;
			case EACCES:
				(void)pfmt(stderr, MM_ERROR, FDR2, SHADOW);
				break;
			default:
				(void)pfmt(stderr, MM_ERROR, BADSTAT, SHADOW, errno);
				break;
		}
		exit(ADT_FMERR);
	} 
 	/*
	 * mode for the audit file should be 400
	*/
	(void) umask(~(buf.st_mode & S_IRUSR));

	if (access(AUDITMASK,0) == 0) {
		(void)pfmt(stderr, MM_ERROR, FDR4);
		exit(ADT_FMERR);
	}

	/*The /etc/security/ia/audit file will have the DAC and MAC*/
        /*of the /etc/shadow file.                                 */
	adt_level = buf.st_level;
        adt_gid = buf.st_gid;
	adt_uid = buf.st_uid;

	/*
	 * open audit mask file
	*/
	if ((adt_fp = fopen(AUDITMASK, "w")) == NULL) {
		(void)pfmt(stderr, MM_ERROR, FDR3);
		exit(ADT_FMERR);
	}

	/* read /etc/default/useradd */
	if ((def_fp = defopen(Usrdefault)) != (FILE *)NULL) {
		if((Amask=defread(def_fp, "AUDIT_MASK"))!=NULL) {
	    		Amask = strdup(Amask);
			/* create initial mask for each user */
	   		if (!Amask || !(*Amask) || cremask(ADT_UMASK,Amask,aevt.emask))
				deflterr++;
		}else 
			deflterr++;
		(void) defclose(def_fp);
	}else 
		deflterr++;

	/*For each entry in the passwd file write an entry (username and*/
	/*mask) to the /etc/security/ia/audit file.                     */
	while (!end_of_file) {
		if ((pwdp = getpwent()) != NULL) {
			/* get each user's name */
			adt = &a_usrnamep;
			(void)sprintf(adt->ia_name,"%s", pwdp->pw_name);
			/*
			 * assign initial audit mask
			*/
			if (deflterr)
			{
				for (mcnt=0; mcnt<ADT_EMASKSIZE; mcnt++) 
		      			adt->ia_amask[mcnt] = 0;
			}
			else
				for (mcnt=0; mcnt<ADT_EMASKSIZE; mcnt++) 
		     			adt->ia_amask[mcnt]=
						aevt.emask[mcnt]; 
			/*
			 * write an entry to audit mask file
			*/
			if ((putadtent(adt,adt_fp)) != 0 ) 
			{
				(void)fclose(adt_fp);
				(void)unlink(AUDITMASK);
				(void)pfmt(stderr, MM_ERROR, FDR3);
				exit(ADT_FMERR);
			}
		}
		else
			end_of_file = 1;
	}
	/*
	 * close the newly created audit file
	*/
	(void) fclose(adt_fp);

	/*If a zero length audit file was created, due to getpwent() failure*/
	/*or empty passwd file, remove the newly created audit file */
	if (stat(AUDITMASK, &buf) == -1) {
		(void)pfmt(stderr, MM_ERROR, BADSTAT, AUDITMASK, errno);
		exit(ADT_FMERR);
	} 
	if (buf.st_size == 0)
	{
		(void)unlink(AUDITMASK);
		(void)pfmt(stderr, MM_ERROR, FDR3);
		exit(ADT_FMERR);
	}

	/*Set the level of the audit file to the level of the /etc/shadow file*/
	(void) lvlfile(AUDITMASK, MAC_SET, &adt_level);

	/*
	 * change the group of the temporary audit mask file
	*/
        if (chown(AUDITMASK, adt_uid, adt_gid) < 0) {
		(void) unlink(AUDITMASK);
		exit(ADT_FMERR);
	}

	/*Warn the user that a default mask of none was set if*/
	/*	-can not open /etc/default/useradd            */
	/*	-AUDIT_MASK parameter missing                 */
	/*	-AUDIT_MASK=value  where value is invalid     */
	/*	-AUDIT_MASK=value  where value is blank       */
	if (deflterr)
		(void)pfmt(stdout, MM_WARNING, NONEMASK);

	(void)pfmt(stdout, MM_INFO, FDR5);
	exit(ADT_SUCCESS);
/*NOTREACHED*/
}
