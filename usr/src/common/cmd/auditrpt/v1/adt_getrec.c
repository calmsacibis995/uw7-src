/*		copyright	"%c%" 	*/

#ident	"@(#)adt_getrec.c	1.4"
#ident  "$Header$"

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vnode.h>
#include <sys/param.h>
#include <sys/privilege.h>
#include "audit.h"
#include <sys/proc.h>
#include <sys/systm.h>
#include <mac.h>
#include <pfmt.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include "auditrec.h"
#include "auditrptv1.h"

#define SIZ_USHORT	sizeof(ushort)
#define SIZ_RESTCMN	sizeof(cmnrec_t) - sizeof(ushort)

/**
 ** Read the next record from the log file.  Fill the cmn and spec part of the
 ** dump record, and if this is a groups record, fill the groups array of the
 ** dump record.
 **/

extern int	char_spec;	/* is file character special? */	
extern int 	free_size;	/* size of free-format data */

int
adt_getrec(log_fp,dumpp)
FILE	*log_fp;
register dumprec_t *dumpp;
{
	int spec_size;
	int null, padnull; 
	/* rtype may be a number (record type) or it may be the first two chars of a magic number */
	union {
		ushort 	rtype;
		char 	crtype[2];
	} u;

	u.rtype=null=free_size=spec_size=0;
	(void)memset((char *)dumpp,'\0',sizeof(dumprec_t));
	if (fread((char *)&u.rtype, 1, SIZ_USHORT , log_fp) != SIZ_USHORT){
		if (feof(log_fp) != 0)
                        return(-1);
		else {
                 	(void)pfmt(stderr,MM_ERROR,E_FMERR);
                        return(ADT_FMERR);
		}

	}

	/* character special files might be padded with NULL until end of block */
	if (char_spec){	
		padnull=2;
		while ((u.rtype == null) && (padnull < 512)) {
			padnull+=SIZ_USHORT;
			if (fread((char *)&u.rtype, 1, SIZ_USHORT, log_fp) != SIZ_USHORT) {
				if (feof(log_fp) != 0)
                        		return(-1);
				else {
                 			(void)pfmt(stderr,MM_ERROR,E_FMERR);
                        		return(ADT_FMERR);
				}
			}

		}
	}

	/* Calculate and read the size of the specific record */
	if((spec_size=getspec(u.rtype)) == -1){
		/*
		 * The record type is not valid if -1 was returned.
		 * However, this may be the start (magic number) of a new
		 * logfile. The magic number identifies the format or byte
		 * order of the log file: ADT_3B2_BYORD, ADT_386_BYORD, or
		 * ADT_XDR_BYORD.
		 *
	 	 * Auditrpt: The log file byte ordering must match the
	 	 * the residing machine's byte ordering.
	 	 * Auditfltr: The -i "type of input" must match the byte
	 	 * ordering of the log file.
		 *
		 * Compare the two characters that have been read, with
		 * the magic number.
		 */

		char log_byord[ADT_BYORDLEN + 1];	/* logfile magic number (byte order) */

		if (strncmp(u.crtype, ADT_BYORD, 2) == 0){
			strncpy(log_byord,u.crtype, 2);
			/* read the rest of the magic number */
        		if (fread(&log_byord[2], 1, (ADT_BYORDLEN - 2), log_fp) != (ADT_BYORDLEN - 2)) {
				(void)pfmt(stderr, MM_ERROR, E_FMERR);
				return(ADT_FMERR);
			}
			/* Is this really a valid magic number */
			if (strcmp(log_byord, ADT_BYORD) == 0){
				/*
				 * Return GOTNEWMV indicating that a magic number
				 * and version number have been read.
				 */
				return(GOTNEWMV);
			}
		}

		(void)pfmt(stderr, MM_ERROR, E_LOG_RTYPE, u.rtype);
		return(ADT_BADTYPE);
	}

	/* Assign the non-null u.rtype to the common record */
	dumpp->r_rtype=u.rtype;

	/* For all record types read the rest of the common record */
	if (fread((char *)&(dumpp->r_event), 1, SIZ_RESTCMN , log_fp) != SIZ_RESTCMN){
		(void)pfmt(stderr,MM_ERROR,E_FMERR);
		return(ADT_FMERR);
	}

	if (spec_size != 0)
		if (fread(&(dumpp->d_spec), 1, spec_size, log_fp) != spec_size){
			(void)pfmt(stderr,MM_ERROR,E_FMERR);
			return(ADT_FMERR);
		}

	/*
	 * Calculate the size of the free format data:
	 *
	 * 	For ACL_R, calculating the number of bytes in the acl
	 * 	structures.  For GROUPS_R calculating the number of
	 * 	bytes for the groups array.  For AEVT_R calculating
	 * 	the number of bytes in the lvltbl.  For FPRIV_R
	 * 	calculating the number of bytes in list of priv_t.
	 * 	For SETGRPS_R calculating the number of bytes in the
	 * 	supplementary group list. For ZMISC_R the application
         *      supplied data.
	 *	
	 *	The value in free_size is used to display the free-format
	 *	data for the USER_R and ZMISC_R records.
	 */
	free_size= dumpp->r_size - SIZ_CMNREC - spec_size - SIZ_TRAILREC;
	if (free_size != 0) {
		if (u.rtype != GROUPS_R) {
			/*Malloc "free_size + 1" bytes to ensure that the string  */
			/*in the dumprec_t structure will be null terminated. If  */
                        /*the free-formated data ended on a word boundary the data*/
                        /*recorded in the audit log is NOT null-terminated.       */
			/*If the free-formated data did NOT end on a word boundary*/
			/*the data recorded in the audit log was null-padded to a */ 
			/*word boundary.                                          */
			if((dumpp->freeformat = (char *)malloc(free_size + 1)) == NULL){
				(void)pfmt(stderr,MM_ERROR,E_BDMALLOC);
				return(ADT_MALLOC);
			}
			if (fread(dumpp->freeformat, 1, free_size, log_fp) != free_size){
				(void)pfmt(stderr,MM_ERROR,E_FMERR);
				return(ADT_FMERR);
			}
			dumpp->freeformat[free_size] = '\0';
		}
		else {
			dumpp->ngroups=((struct groups_r *)&(dumpp->spec_data))->gr_ngroups;
			if (fread((char *)dumpp->groups, 1, free_size, log_fp) != free_size){
				(void)pfmt(stderr,MM_ERROR,E_FMERR);
				return(ADT_FMERR);
			}
		}
	}

	/* Read in the trailer */
	if (fread((char *)&(dumpp->trail), 1, SIZ_TRAILREC, log_fp) != SIZ_TRAILREC){
		(void)pfmt(stderr,MM_ERROR,E_FMERR);
		return(ADT_FMERR);
	}

	return(0);
}

/**
 ** This routine calculates the size of the specific (other than cmn,
 ** freeformat or trailer) part of each of the record types in the log file.
 **/
int
getspec(rtype)
ushort rtype;
{
	int s_size;

	switch (rtype) {
		case A_FILEID :
			s_size=SIZ_ID;
			break;
		case ABUF_R :
			s_size=SIZ_ABUF;
			break;
		case ACL_R :
			s_size=SIZ_ACL;
			break;
		case ACTL_R :
			s_size=SIZ_ACTL;
			break;
		case ADMIN_R :
			s_size=SIZ_ADMIN;
			break;
		case ADMP_R :
			s_size=SIZ_ADMP;
			break;
		case AEVT_R :
			s_size=SIZ_AEVT;
			break;
		case ALOG_R :
			s_size=SIZ_ALOG;
			break;
		case CC_R :
			s_size=SIZ_CC;
			break;
		case CHMOD_R:
			s_size=SIZ_CHMOD;
                        break;
		case CHOWN_R:
			s_size=SIZ_CHOWN;
                        break;
		case CRON_R:
			s_size=SIZ_CRON;
			break;
		case DEV_R:
			s_size=SIZ_DEV;
                        break;
		case EXEC_R :
			s_size=0;
			break;
		case FILE_R :
			s_size=SIZ_FILE;
			break;
		case FCHDIR_R :
			s_size=SIZ_FCHDIR;
			break;
		case FCHMOD_R :
			s_size=SIZ_FCHMOD;
			break;
		case FCHOWN_R :
			s_size=SIZ_FCHOWN;
			break;
		case FCNTL_R :
			s_size=SIZ_FCNTL;
			break;
		case FCNTLK_R :
			s_size=SIZ_FCNTLK;
			break;
		case FDEV_R :
			s_size=SIZ_FDEV;
			break;
		case FILENAME_R : 	
			s_size=SIZ_FNAME;
			break;
		case FMAC_R :
			s_size=SIZ_FMAC;
			break;
		case FORK_R :  	
			s_size=SIZ_FORK;
			break;
		case FPRIV_R :
			s_size=SIZ_FPRIV;
			break;
		case FSTAT_R :
			s_size=SIZ_FSTAT;
			break;
		case GROUPS_R:
			s_size=SIZ_GROUPS;
                        break;
		case INIT_R:
			s_size=0;
                        break;
		case IPCR_R:
			s_size=SIZ_IPC;
                        break;
		case IOCTL_R:
			s_size=SIZ_IOCTL;
                        break;
		case KILL_R :
			s_size=SIZ_KILL;
			break;
		case LOGIN_R:
			s_size=SIZ_LOGIN;
                        break;
		case MAC_R:
			s_size=SIZ_MAC;
                        break;
		case MCTL_R:
			s_size=SIZ_MCTL;
                        break;
		case MODADM_R:
                        s_size=SIZ_MODADM;
                        break;
		case MODLD_R:
                        s_size=SIZ_MODLD;
                        break;
		case MODPATH_R:
                        s_size=0;
                        break;
		case MOUNT_R:
			s_size=SIZ_MOUNT;
                        break;
		case PARMS_R :
			s_size=SIZ_PARMS;
			break;
		case PASSWD_R:
			s_size=SIZ_PASSWD;
			break;
		case PIPE_R:
			s_size=SIZ_PIPE;
                        break;
		case PLOCK_R :
			s_size=SIZ_PLOCK;
			break;
		case RECVFD_R :
			s_size=SIZ_RECVFD;
			break;
		case RLIM_R:
			s_size=SIZ_RLIM;
			break;
		case SETGRPS_R :
			s_size=SIZ_SETGROUP;
			break;
		case PRIV_R :
			s_size=SIZ_PRIV;
			break;
		case PROC_R:
			s_size=SIZ_PROC;
                        break;
		case SETPGRP_R:
			s_size=SIZ_SETPGRP;
                        break;
		case SETUID_R :
			s_size=SIZ_SETID;
			break;
		case SYS_R:
			s_size=SIZ_SYS;
                        break;
		case TIME_R:
			s_size=SIZ_TIME;
                        break;
		case USER_R :
			s_size=0;
			break;
		case ZMISC_R:
			s_size=0;
			break;
		default :
			s_size = -1;
	}
	return(s_size);
}
