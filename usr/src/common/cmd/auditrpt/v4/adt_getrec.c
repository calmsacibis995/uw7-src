/*		copyright	"%c%" 	*/

#ident	"@(#)adt_getrec.c	1.3"
#ident  "$Header$"

/* LINTLIBRARY */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vnode.h>
#include <sys/param.h>
#include <sys/privilege.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <mac.h>
#include <pfmt.h>
#include <sys/fcntl.h>
#include <sys/resource.h>
#include <audit.h>
#include <sys/auditrec.h>
#include "auditrptv4.h"

#define SIZ_USHORT	sizeof(ushort)
#define SIZ_RESTCMN	sizeof(cmnrec_t) - sizeof(ushort)
#define BLOCK_SIZE	512

/*
 * Read the next record from the log file.  Fill the cmn and spec part of the
 * dump record.
 */

char adt_ver[ADT_VERLEN];		/* logfile version number */
char log_byord[ADT_BYORDLEN + 1];	/* logfile magic number (byte order) */
int	char_spec;		/* is file character special? */	
int 	free_size;		/* size of free-format data */

int version_number();
void get_ff();

int
adt_getrec(log_fp, dumpp)
FILE	*log_fp;
register dumprec_t *dumpp;
{

	int spec_size;
	int null, padnull; 
	int siz_grps;
	/* rtype may be a number (record type) or it may be the first two chars of a magic number */
	union {
		ushort 	rtype;
		char 	crtype[2];
	} u;

	u.rtype = null = free_size = spec_size = 0;
	(void)memset((char *)dumpp, '\0', sizeof(dumprec_t));
	if (fread((char *)&u.rtype, 1, SIZ_USHORT , log_fp) != SIZ_USHORT){
		if (feof(log_fp) != 0)
                        return(-1);
		else {
                 	(void)pfmt(stderr, MM_ERROR, E_FMERR);
                        return(ADT_FMERR);
		}

	}

	/* character special files might be padded with NULL until end of block */
	if (char_spec){	
		padnull = 2;
		while ((u.rtype == null) && (padnull < BLOCK_SIZE)) {
			padnull += SIZ_USHORT;
			if (fread((char *)&u.rtype, 1, SIZ_USHORT, log_fp) != SIZ_USHORT) {
				if (feof(log_fp) != 0)
                        		return(-1);
				else {
                 			(void)pfmt(stderr, MM_ERROR, E_FMERR);
                        		return(ADT_FMERR);
				}
			}

		}
	}

	/* Calculate the size of the specific record */
	if ((spec_size=getspec(u.rtype)) == -1){
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
		if (strncmp(u.crtype, ADT_BYORD, 2) == 0){
			strncpy(log_byord,u.crtype, 2);
			/* read the rest of the magic number */
        		if (fread(&log_byord[2], 1, (ADT_BYORDLEN - 2), log_fp) != (ADT_BYORDLEN - 2)) {
				(void)pfmt(stderr, MM_ERROR, E_FMERR);
				return(ADT_FMERR);
			}
			/* Is this really a valid magic number */
			if (strcmp(log_byord, ADT_BYORD) == 0){
				/* read the version number */
				if (version_number(log_fp) != 0){
					(void)pfmt(stderr, MM_ERROR, E_NOCOMPATVER);
					exit(ADT_FMERR);
				}
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
	dumpp->r_rtype = u.rtype;

	/*
	 * Process the common and specific records
	 */
	switch(u.rtype) {
	case FNAME_R:
		dumpp->spec_data.r_fname.f_rtype = u.rtype;
		/* Read the specific record, no common for this record type */
		if (fread(&(dumpp->spec_data.r_fname.f_event), 1,
		    spec_size - SIZ_USHORT, log_fp) != spec_size - SIZ_USHORT){
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		/*
	 	 * Calculate the size of the free format data:
		 * FNAME_R has no common or credential record.
		 */
		free_size = dumpp->spec_data.r_fname.f_size - spec_size;
		/* Put free format info into dumprec, if it is present */
		if (free_size > 0)
			get_ff(free_size, log_fp, dumpp);
		break;

	case CRFREE_R:
		dumpp->spec_data.r_credf.cr_rtype = u.rtype;
		/* Read the specific record, no common for this record type */
		if (fread(&(dumpp->spec_data.r_credf.cr_padd), 1, 
		    spec_size - SIZ_USHORT, log_fp) != (spec_size - SIZ_USHORT)){
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		/*
	 	 * Calculate the size of the free format data:
		 * CRFREE_R has no common or credential record.
		 */
		free_size = dumpp->spec_data.r_credf.cr_ncrseqnum * sizeof(ulong_t);
		/* Put freeformat info into dumprec, if it is present */
		if (free_size > 0)
			get_ff(free_size, log_fp, dumpp);
		break;

	default:
	 	/* Read the rest of the common record */
		if (fread((char *)&(dumpp->r_event), 1, SIZ_RESTCMN, 
		    log_fp) != SIZ_RESTCMN) {
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		if (spec_size > 0)
			/* Read the specific record */
			if (fread(&(dumpp->d_spec), 1, spec_size, log_fp) != spec_size){
                                (void)pfmt(stderr, MM_ERROR, E_FMERR);
                                return(ADT_FMERR);
                        }

		/*
		 * Calculate the size of the free format data:
		 *
		 *	For ACL_R, calculating the number of bytes in the acl
		 *	structures.  For AEVT_R calculating the number of
		 *	bytes in the lvltbl.  For FPRIV_R calculating the
		 *	number of bytes in list of priv_t.  For SETGRPS_R
		 *	calculating the number of bytes in the supplementary
		 *	group list. For ZMISC_R the application supplied data.
		 *	
		 *	The value in free_size is used to display the free-format
		 *	data for the USER_R and ZMISC_R records.
		 */
		free_size = dumpp->r_size - SIZ_CMNREC - spec_size;
		/* Put free format info in dumprec, if it is present */
		if (free_size > 0)
			get_ff(free_size, log_fp, dumpp);
		/* if cred sequence number is 0, read the credential information */
		if (dumpp->cmn.c_crseqnum == 0) {
			/* read the cred record */
			if (fread(&(dumpp->cred), 1, SIZ_CREDREC, log_fp) != SIZ_CREDREC){
				(void)pfmt(stderr, MM_ERROR, E_FMERR);
				return(ADT_FMERR);
			}
			/* compute the size of, and read the group information */
			if (dumpp->cred.cr_ngroups > 0){
				siz_grps = dumpp->cred.cr_ngroups *  sizeof(gid_t);
				if (fread((dumpp->groups), 1, siz_grps, log_fp) != siz_grps){
					(void)pfmt(stderr, MM_ERROR, E_FMERR);
					return(ADT_FMERR);
				}
			}
		}
	}
	return(0);
}

/* This routine puts the free format data into the dumprec structure. */
void
get_ff(free_size, log_fp, dumpp)
int free_size;
FILE	*log_fp;
register dumprec_t *dumpp;
{
	/*
	 * Malloc "free_size + 1" bytes to ensure that the string  
	 * in the dumprec_t structure will be null terminated. If  
	 * the free-formated data ended on a word boundary the data
	 * recorded in the audit log is NOT null-terminated.      
	 * If the free-formated data did NOT end on a word boundary
	 * the data recorded in the audit log was null-padded to a  
	 * word boundary.                                          
	 */
	if ((dumpp->freeformat = (char *)malloc(free_size + 1)) == NULL){
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		exit(ADT_MALLOC);
	}
	if (fread(dumpp->freeformat, 1, free_size, log_fp) != free_size){
		(void)pfmt(stderr, MM_ERROR, E_FMERR);
		exit(ADT_FMERR);
	}
	dumpp->freeformat[free_size] = '\0';
}

/*
 * This routine returns the size of the specific (other than cmn,
 * or freeformat) part of each of the record types in the log file.
 * If the rtype is not valid a -1 is returned.
 */
int
getspec(rtype)
ushort rtype;
{
	int s_size;

	switch (rtype) {
		case FILEID_R :
			s_size = SIZ_ID;
			break;
		case ABUF_R :
			s_size = SIZ_ABUF;
			break;
		case ACL_R :
			s_size = SIZ_ACL;
			break;
		case ACTL_R :
			s_size = SIZ_ACTL;
			break;
		case ADMIN_R :
			s_size = SIZ_ADMIN;
			break;
		case ADMP_R :
			s_size = SIZ_ADMP;
			break;
		case AEVT_R :
			s_size = SIZ_AEVT;
			break;
		case ALOG_R :
			s_size = SIZ_ALOG;
			break;
		case BIND_R :
			s_size = SIZ_BIND;
			break;
#ifdef CC_PARTIAL
		case CC_R :
			s_size = SIZ_CC;
			break;
#endif
		case CHMOD_R:
			s_size = SIZ_CHMOD;
                        break;
		case CHOWN_R:
			s_size = SIZ_CHOWN;
                        break;
		case CMN_R :
			s_size = 0;
			break;
		case CRFREE_R :
			s_size = SIZ_CREDFREC;
			break;
		case CRON_R:
			s_size = SIZ_CRON;
			break;
		case DEV_R:
			s_size = SIZ_DEV;
                        break;
		case FILE_R :
			s_size = 0;
			break;
		case FCHMOD_R :
			s_size = SIZ_FCHMOD;
			break;
		case FCHOWN_R :
			s_size = SIZ_FCHOWN;
			break;
		case FCNTL_R :
			s_size = SIZ_FCNTL;
			break;
		case FCNTLK_R :
			s_size = SIZ_FCNTLK;
			break;
		case FD_R :
			s_size = SIZ_FD;
			break;
		case FDEV_R :
			s_size = SIZ_FDEV;
			break;
		case FNAME_R :
			s_size = SIZ_FNAME;
			break;
		case FMAC_R :
			s_size = SIZ_FMAC;
			break;
		case FORK_R :  	
			s_size = SIZ_FORK;
			break;
		case FPRIV_R :
			s_size = SIZ_FPRIV;
			break;
		case IPCS_R:
			s_size = SIZ_IPC;
                        break;
		case IPCACL_R :
			s_size = SIZ_IPCACL;
			break;
		case IOCTL_R:
			s_size = SIZ_IOCTL;
                        break;
		case KEYCTL_R :
			s_size = SIZ_KEYCTL;
			break;
		case KILL_R :
			s_size = SIZ_KILL;
			break;
		case LOGIN_R:
			s_size = SIZ_LOGIN;
                        break;
		case LWPCREATE_R :
			s_size = SIZ_LWPCREATE;
			break;
		case MAC_R:
			s_size = SIZ_MAC;
                        break;
		case MCTL_R:
			s_size = SIZ_MCTL;
                        break;
		case MODADM_R:
                        s_size = SIZ_MODADM;
                        break;
		case MODLOAD_R:
                        s_size = SIZ_MODLOAD;
                        break;
		case MODPATH_R:
                        s_size = 0;
                        break;
		case MOUNT_R:
			s_size = SIZ_MOUNT;
                        break;
		case ONLINE_R :
			s_size = SIZ_ONLINE;
			break;
		case PARMS_R :
			s_size = SIZ_PARMS;
			break;
		case PASSWD_R:
			s_size = SIZ_PASSWD;
			break;
		case PIPE_R:
			s_size = SIZ_PIPE;
                        break;
		case PLOCK_R :
			s_size = SIZ_PLOCK;
			break;
		case RECVFD_R :
			s_size = SIZ_RECVFD;
			break;
		case RLIM_R:
			s_size = SIZ_RLIM;
			break;
		case SETGRPS_R :
			s_size = SIZ_SETGROUP;
			break;
		case SETPGRP_R:
			s_size = SIZ_SETPGRP;
                        break;
		case SETID_R :
			s_size = SIZ_SETID;
			break;
		case TIME_R:
			s_size = SIZ_TIME;
                        break;
		case ULIMIT_R :
			s_size = SIZ_ULIMIT;
			break;
		case ZMISC_R:
			s_size = 0;
			break;
		default :
			s_size = -1;
	}
	return(s_size);
}

/* This routine is called to read the second field of the audit log
 * file.  This field identifies the version number of the log file:
 * 1.0, 2.0, 3.0 or 4.0.
 * Note that for versions 1.0, 2.0, and 3.0 this field will be NULL.
 */
int
version_number(fp)
FILE *fp;
{
        if (fread(adt_ver, 1,  ADT_VERLEN, fp) != ADT_VERLEN) {
                (void)pfmt(stderr, MM_ERROR, E_NO_VER);
                exit(ADT_FMERR);
	}

	/*
	 * Check the version number of the log file. For ES/MP
	 * this command will only process logfiles for version 4.0. 
	 */
	if (strcmp(adt_ver, "4.0") == 0) {
		return(0);
	} else {
		return(-1);
	}
}
