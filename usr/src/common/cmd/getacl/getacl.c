/*		copyright	"%c%" 	*/

#ident	"@(#)getacl:getacl.c	1.9.2.2"
#ident  "$Header$"
/***************************************************************************
 * Command: getacl
 * Inheritable Privileges: P_DACREAD,P_MACREAD
 *       Fixed Privileges: None
 * Notes:
 *
 ***************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <acl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <priv.h>
#include <locale.h>
#include <pfmt.h>

#define AREAD		0x4	/* read permission */
#define AWRITE		0x2	/* write permission */
#define AEXEC		0x1	/* execute permission */

#define ACCESS_ACL	0x1	/* user specified "-a" */
#define DEFAULT_ACL	0x2	/* user specified "-d" */

extern int optind;		/* for getopt */
extern char *optarg;		/* for getopt */
extern int errno;
extern int sys_nerr;

#define NENTRIES	128		/* initial size of ACL buffer */
struct	acl aclbuf[NENTRIES];		/* initial ACL buffer */
int	num_entries = NENTRIES;		/* ACL buffer size */

#define BADUSAGE	":48:incorrect usage\n"
#define NOCLASSENT	":61:missing CLASS_OBJ entry for \"%s\"\n"
#define BADACLTYPE	":62:invalid ACL type for \"%s\"\n"
#define PERMDENIED	":63:permission denied for \"%s\"\n"
#define	FILENOTFOUND	":64:file \"%s\" not found\n"
#define STATFAILED	":65:stat failed for file \"%s\", %s\n"
#define MALLOCFAIL	":66:malloc failed\n"
#define NOPKG		":57:system service not installed\n"
#define ACLFAIL		":67:acl failed for file \"%s\", %s\n"
/*
 * Procedure:     main
 *
 * Restrictions:
                 getpwuid: P_MACREAD
                 getgrgid: P_MACREAD
		 pfmt: none
*/
main(argc,argv)
char 	*argv[];
{
	struct stat 	statbuf;
	struct passwd 	*getpwuid();
	struct passwd	*pwd_p;
	struct group 	*getgrgid();
	struct group	*grp_p;
	struct acl 	*aclbp = aclbuf;	/* pointer to acl call buffer */
	char 		*filep;                 /* filename */  
	void 		usage();		/* print "usage: " message */
	long		optflag = 0;
	long		mfiles  = 0;		/* indicates multiple files */
	int 		i, c, nentries;
	int		error = 0;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxes");
	(void)setlabel("UX:getacl");

	while ((c=getopt(argc,argv,"ad"))!=EOF) {
		switch(c) {
			case 'a':
				optflag |= ACCESS_ACL;
				break;
			case 'd':
				optflag |= DEFAULT_ACL;
				break;
			default:
				usage();
				exit(1);
				break;
		} /* switch */
	} /* while getopt */

	if (optind == argc) {	/* no file name given */
		pfmt(stderr, MM_ERROR, BADUSAGE);
		usage();
		exit(1);
	}
	/* check if multiple files so we can print blank line */
	if (optind < (argc - 1))
		mfiles++;
	if (optflag == 0) 
		optflag = ACCESS_ACL | DEFAULT_ACL;
	/* 
	 * loop for each file specified 
	 */
	for (i = optind; i < argc; i++) {

		/* get all existing ACL entries for the file */
		filep = argv[i];
		if (getacl(filep, &nentries, &aclbp, &statbuf) < 0) {
			error = 2;
			continue;
		}

		/*
	 	 * Print Filename, Owner & Group
	 	 */

		pfmt(stdout, MM_NOSTD, ":68:# file: %s\n", filep);
		pfmt(stdout, MM_NOSTD, ":69:# owner: ");

		procprivl(CLRPRV,MACREAD_W,(priv_t)0);
		if ((pwd_p = getpwuid(statbuf.st_uid)) != NULL)
			fprintf(stdout, "%s\n", pwd_p->pw_name);
		else
			fprintf(stdout, "%d\n",statbuf.st_uid);
		pfmt(stdout, MM_NOSTD, ":70:# group: ");
		if ((grp_p = getgrgid(statbuf.st_gid)) != NULL)
			fprintf(stdout, "%s\n", grp_p->gr_name);
		else
			fprintf(stdout, "%d\n", statbuf.st_gid);
				
		procprivl(SETPRV,MACREAD_W,(priv_t)0);
		/* Print Out ACL */
		if (printacl(aclbp, nentries, optflag, filep) < 0)
			error = 2;
		/* print space between multiple files */
		if ((mfiles) && (i < (argc - 1)))
			fprintf(stdout, "\n");
		fflush(stdout);
	} 	/* end for */

	/* if we malloc'ed the ACL buffer, free it now */
	if (aclbp != aclbuf)
		free((char *)aclbp);
	exit(error);
}


/*
 * Procedure:     printacl
 *
 * Restrictions:
                getpwuid: P_MACREAD
                getgrgid: P_MACREAD
		pfmt: none
*/
/*
 *
 *	printacl - prints the ACL entries in the buffer
 *		   based on the options specified by the
 *		   caller.
 *
 *	input -	   1. pointer the buffer of ACL entries
 *		   2. number of entries in the buffer
 *		   3. flags showing the options specified by the caller
 *		      (which may be -a and/or -d)
 *		   4. file name
 *
 *	output -   none
 *
 */
int
printacl(aclbufp, nentries, optflag, fname)
struct acl 	*aclbufp;
int		nentries;
long		optflag;
char		*fname;
{
	struct acl 	*aclp;
	struct passwd 	*getpwuid();
	struct passwd	*pwd_p;
	struct group 	*getgrgid();
	struct group	*grp_p;
	long 		i; 
	long 		entry_type; 
	mode_t		class_perms;

        /* get the file group class bits to print effective permissions */
	aclp = aclbufp;
        for (i = 0;  i < nentries; i++, aclp++) {
                if (aclp->a_type == CLASS_OBJ) {
                        class_perms = aclp->a_perm;
                        break;
                }
        }
        if (i == nentries) {
                pfmt(stderr, MM_ERROR, NOCLASSENT, fname);
		return(-1);
        }

	/*
	 * Print ACL, Based on User Options
	 */

        aclp = aclbufp;
	for (i = 0;  i < nentries; i++, aclp++) {
		if (aclp->a_type & ACL_DEFAULT) {
			/* If this is a default entry */
			if (optflag & DEFAULT_ACL) {
				/* if user requested default entries */
				pfmt(stdout, MM_NOSTD, ":71:default:");
				/*
				 * turn off default to print the 
				 * remainder of the entry type
				 */
				entry_type = aclp->a_type & ~ACL_DEFAULT;
			} else {
				/* user didn't want to print defaults */
				continue;
			}
		} else {
			/* this is a non-default entry */
			if (optflag & ACCESS_ACL) {
				/* user specified "-a" */
				entry_type = aclp->a_type;
			} else {
				/* user didn't specify "-a" */
				continue;
			}
		}
		
		switch (entry_type) {
		case USER_OBJ:
			pfmt(stdout, MM_NOSTD, ":72:user::");
			break;
		case USER:
			pfmt(stdout, MM_NOSTD, ":73:user:");
			procprivl(CLRPRV,MACREAD_W,(priv_t)0);
			if ((pwd_p = getpwuid(aclp->a_id)) != NULL)
				fprintf(stdout, "%s:", pwd_p->pw_name);
			else
				fprintf(stdout, "%d:",aclp->a_id);
			procprivl(SETPRV,MACREAD_W,(priv_t)0);
			break;
		case GROUP_OBJ:
			pfmt(stdout, MM_NOSTD, ":74:group::");
			break;
		case GROUP:
			pfmt(stdout, MM_NOSTD, ":75:group:");
			procprivl(CLRPRV,MACREAD_W,(priv_t)0);
			if ((grp_p = getgrgid(aclp->a_id)) != NULL)
				fprintf(stdout, "%s:", grp_p->gr_name);
			else
				fprintf(stdout, "%d:", aclp->a_id);
			procprivl(SETPRV,MACREAD_W,(priv_t)0);
			break;
		case CLASS_OBJ:
			pfmt(stdout, MM_NOSTD, ":76:class:");
			break;
		case OTHER_OBJ:
			pfmt(stdout, MM_NOSTD, ":77:other:");
			break;
		default:
			pfmt(stderr, MM_ERROR, BADACLTYPE, fname);
			return(-1);
			break;
		}	/* end switch */
		
		/* Now print permissions field */
		fprintf(stdout, "%c%c%c", 
			aclp->a_perm & AREAD ? 'r' : '-',
			aclp->a_perm & AWRITE ? 'w' : '-',
			aclp->a_perm & AEXEC ? 'x' : '-');

		/*
		 * use the file group class bits to determine
		 * whether or not to print the effective perms,
		 * but not for default entries (which is why
		 * we look at a_type instead of entry_type 
		 */
		if ((aclp->a_type == USER) || (aclp->a_type == GROUP_OBJ) ||
			(aclp->a_type == GROUP)) {
			if ((ushort)(class_perms & aclp->a_perm) < aclp->a_perm) {
				pfmt(stdout,MM_NOSTD,":78:\t#effective:");
				fprintf(stdout, "%c%c%c", 
					class_perms & aclp->a_perm & AREAD 
						? 'r' :'-',
					class_perms & aclp->a_perm & AWRITE 
						? 'w' :'-',
					class_perms & aclp->a_perm & AEXEC 
						? 'x' :'-');
			}	/* end "if ((ushort)(class_perms & ... " */
		}	/* end "if ((entry_type == USER) ..." */

		/* and the all important new-line */
		fprintf(stdout, "\n");

	}	/* end for */
	return(0);
}


/*
 * Procedure:     getacl
 *
 * Restrictions:
                 stat(2): none
                 acl(2): none
		 strerror: none
		 pfmt: none
*/

/*
 *
 *	getacl - get the ACL for the file specified,
 *		 and stat the file for additional information.
 *
 *	input -  1. file name
 *		 2. pointer to number of entries in the ACL
 *		 3. pointer to pointer to ACL buffer
 *		 4. pointer to the stat buffer
 *
 *	output - pointer to the buffer containing the ACL
 *
 */
int
getacl(filep, nentriesp, aclpp, statbufp)
char		*filep;
int		*nentriesp;
struct acl	**aclpp;
struct stat 	*statbufp;
{
	int		error;
	int		i;
	struct acl 	*aclp = *aclpp;
	  
	if ((error = stat(filep, statbufp)) != 0) {
		if (errno == EACCES) {
			pfmt(stderr, MM_ERROR, PERMDENIED, filep);
			return (-1);
		} 
		else if (errno == ENOENT) {
			pfmt(stderr, MM_ERROR, FILENOTFOUND, filep);
			return (-1);
		}
		pfmt(stderr, MM_ERROR, STATFAILED, filep, strerror(errno));
		return (-1);
	}
	while ((*nentriesp = acl(filep, ACL_GET, num_entries, aclp)) == -1) {
		if (errno == ENOSPC) {
			if (aclp != aclbuf)
				free(aclp);
			num_entries *= 2;
			if ((aclp = (struct acl *)malloc
				(num_entries * sizeof(struct acl))) == NULL) {
				pfmt(stderr, MM_ERROR, MALLOCFAIL);
				exit(32);
			}
		} else {
			if (errno == ENOPKG) {
				pfmt(stderr, MM_ERROR, NOPKG);
				exit(3);
			} 
			pfmt(stderr, MM_ERROR, ACLFAIL, filep, strerror(errno));
			return(-1);
		}
	} 	/* end while */  
	*aclpp = aclp;
	return (0);
}


/*
 * Procedure:     usage
 *
 * Restrictions:
			pfmt: none
*/
/* print Usage message */
void 
usage ()
{
	pfmt(stderr,MM_ACTION,":79:usage: getacl [-ad] file ...\n");
}

