/*		copyright	"%c%" 	*/

#ident	"@(#)auditoff.c	1.3"
#ident  "$Header$"
/***************************************************************************
 * Command: auditoff
 * Inheritable Privileges: P_SETPLEVEL,P_AUDIT
 *       Fixed Privileges: None
 * Notes:	Disable the Auditing subsystem
 *
 ***************************************************************************/

/*LINTLIBRARY*/
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <audit.h>
#include <mac.h>
#include <pfmt.h>
#include <stdio.h>
#include <locale.h>
#include <stdlib.h>

/* Fault Detection and Recovery */
#define	FDR1		":61:Auditing disabled\n"
#define	FDR2		":62:Auditing already disabled\n"

#define NOPERM		":17:Permission denied\n"
#define BADARGV 	":18:argvtostr() failed\n"
#define LVLOPER 	":35:%s() failed, errno = %d\n"
#define BADSTAT 	":20:auditctl() failed ASTATUS, errno = %d\n"
#define BADOFF 		":63:auditctl() failed AUDITOFF, errno = %d\n"
#define USAGE		":64:usage: auditoff\n"
#define NOPKG		":34:system service not installed\n"

extern char *argvtostr();
static void adumprec();

/*
 * Procedure:     main
 *
 * Restrictions:  
                 auditevt(2): None
                 lvlproc(2): None
                 auditctl(2): None
 *
 * Notes:	  Disable auditing
 */
main(argc,argv)
int argc;
char **argv;
{
	char *argvp;
	level_t mylvl, audlvl;
	actl_t	actl;		/* auditctl(2) structure */


	/* Initialize locale information */
	(void)setlocale(LC_ALL, "");

        /* Initialize message label */
	(void)setlabel("UX:auditoff");

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
		if (lvlin(SYS_AUDIT, &audlvl) == -1) {
			(void)pfmt(stderr, MM_ERROR, LVLOPER, "lvlin", errno);
			exit(ADT_LVLOPER);
		}
		if (lvlequal(&audlvl, &mylvl) == 0){
                	/* SET level if not SYS_AUDIT */
                        if (lvlproc(MAC_SET, &audlvl) == -1) {
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

	/* save command line arguments */
	if (( argvp = (char *)argvtostr(argv)) == NULL) {
		(void)pfmt(stderr, MM_ERROR, BADARGV);
		adumprec(ADT_AUDIT_CTL,ADT_MALLOC,strlen(argv[0]),argv[0]);
                exit(ADT_MALLOC);
        }

	/* get current status of auditing */				
	if (auditctl(ASTATUS, &actl, sizeof(actl_t)) != 0) {		
		(void)pfmt(stderr, MM_ERROR, BADSTAT, errno);
		adumprec(ADT_AUDIT_CTL,ADT_BADSTAT,strlen(argvp),argvp);
		exit(ADT_BADSTAT);		
	}					
     
	/* command does not take any arguments */
	if (argc>1) {
		(void) pfmt(stderr, MM_ACTION, USAGE);
		adumprec(ADT_AUDIT_CTL,ADT_BADSYN,strlen(argvp),argvp);
		exit(ADT_BADSYN);
	}

	/* auditing already disabled */
	if (!actl.auditon) {
		(void)pfmt(stdout, MM_WARNING, FDR2);
		exit(ADT_SUCCESS);
	}
	
	/* disable auditing */
	adumprec(ADT_AUDIT_CTL,ADT_SUCCESS,strlen(argvp),argvp);		
	if (auditctl(AUDITOFF,0, sizeof(actl_t))) {
		(void)pfmt(stderr, MM_ERROR, BADOFF, errno);
		adumprec(ADT_AUDIT_CTL,ADT_INVALID,strlen(argvp),argvp);
		exit(ADT_INVALID);
	} else 
		(void)pfmt(stdout, MM_INFO, FDR1);

	exit(ADT_SUCCESS);
/*NOTREACHED*/
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
