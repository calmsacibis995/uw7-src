/*		copyright	"%c%" 	*/

#ident	"@(#)audit.h	1.2"
#ident  "$Header$"

#ifndef _AUDIT_H
#define	_AUDIT_H

#include "sysaudit.h"

#define ADT_CLASSFILE	"/etc/security/audit/classes"
#define ADT_DEFLTFILE	"audit"
#define ADT_MAPFILE	"/var/audit/auditmap/auditmap"
#define ADT_MAPDIR	"/var/audit/auditmap"
#define ADT_MAPPGM	"/usr/sbin/auditmap"
#define CAT		"/etc/security/mac/ltf.cat"
#define ADT_CAT		"/var/audit/auditmap/ltf.cat"
#define CLSF		"/etc/security/mac/ltf.class"
#define ADT_CLSF	"/var/audit/auditmap/ltf.class"
#define ALIASF		"/etc/security/mac/ltf.alias"
#define ADT_ALIASF	"/var/audit/auditmap/ltf.alias"
#define LIDF		"/etc/security/mac/lid.internal"
#define ADT_LIDF	"/var/audit/auditmap/lid.internal"

#define LVL_STRUCT_SIZE		sizeof(struct mac_level)

/* exit  codes for audit commands */
#define ADT_SUCCESS	0	/* succeeded */
#define ADT_BADSYN	1	/* Incorrect syntax */
#define ADT_INVARG	2	/* Valid & Invalid arguments */
#define ADT_NOPKG	3	/* Service not installed */
#define ADT_NOPERM	4	/* No permission */
#define ADT_FMERR	5	/* File manipulation error */
#define ADT_BADBGET	6       /* Get buffer attributes failed */
#define ADT_BADBSET	7       /* Set buffer attributes failed */
#define ADT_BADLGET	8       /* Get log attributes failed */
#define ADT_BADLSET	9       /* Set log attributes failed */
#define ADT_BADEGET	10      /* Get event mask error */
#define ADT_BADESET	11      /* Set event mask error */
#define ADT_BADSTAT	12      /* Get audit status failed */
#define ADT_BADTYPE	13      /* Invalid audit log file type */
#define ADT_BADVHIGH	14      /* Invalid high water mark */
#define ADT_NOLOG	15      /* No audit log file */
#define ADT_NOPGM	16      /* No audit switch program */
#define ADT_INVALID	17      /* Unable to disable/enable */
#define ADT_NOMATCH	18      /* No match found in log file */
#define ADT_MACHERR	19      /* Unable to get machine name info */
#define ADT_UIDERR	20      /* Unable to get uid info */
#define ADT_GIDERR	21      /* Unable to get gid info */
#define ADT_CLASSERR	22      /* Unable to get event classes */
#define ADT_TYPERR	23      /* Unable to get event types */
#define ADT_MALLOC	24      /* Unable to malloc memory */
#define ADT_DMPFAIL	25      /* Unsuccessful auditdmp(2) */
#define ADT_INCOMPLETE	26	/* Incomplete statement */
#define ADT_LVLOPER	27	/* MAC system call or library routine failed*/
#define ADT_BADMAP	28	/* Invalid map element */
#define ADT_PRVERR	29      /* Unable to get privilege info*/
#define ADT_SYSERR	30      /* Unable to get system name info*/
#define ADT_BADLOUT	31	/* Invalid lvlout(2) */
#define ADT_BADARCH	32	/* Log file byte order of format not readable */
#define ADT_BADEXEC	33	/* unable to execute program(auditmap) */
#define ADT_ENABLED	34	/* auditing is on*/
#define ADT_NOTENABLED	35	/* auditing not on*/
#define ADT_BADFORK	36	/* unable to execute program(auditmap) */
#define ADT_BADLVLIN	37	/* Invalid lvlin(2) */
#define ADT_BADDEFAULT	38	/* value in audit default file is invalid*/
#define ADT_XDRERR      39	/* XDR encode/decode error*/

/* Structure of Auditable Events Table */
extern struct adtevt { 	
	char	*a_evtnamp;	/* name of event type */
	int	a_evtnum;	/* event number */
	int	a_objt;		/* selectable by object level */
	int	a_fixed;	/* fixed event - can't be turned off */
} adtevt[];

/* Table for Class/Types of Events */
struct evtab {
	char *aclassp;		/* pointer to alias event */
	char *atyplistp;	/* pointer to corresponding list */
} *evtabptr;

/*Structure for audit default file*/
typedef struct defaults {
	char *logerrp;
	char *logfullp;
	char *pgmp;
	char *defpathp;
	char *nodep;
}defaults_t;

#if defined (__STDC__)
extern	int	auditbuf(int, struct abuf *, int);
extern	int	auditctl(int, struct actl *, int);
extern	int	auditdmp(struct arec *, int);
extern	int	auditevt(int, struct aevt *, int);
extern	int	auditlog(int, struct alog *, int);

#else	/* !defined (__STDC__) */

extern	int	auditbuf();
extern	int	auditctl();
extern	int	auditdmp();
extern	int	auditevt();
extern	int	auditlog();

#endif		/* defined (__STDC__) */

#endif	/* _AUDIT_H */
