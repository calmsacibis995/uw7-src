/*		copyright	"%c%" 	*/

#ident	"@(#)auditfltrv4.c	1.3"
#ident  "$Header$"
/*
 * Command: auditfltrv4
 * Inheritable Privileges: None
 *       Fixed Privileges: None
 * Level:	USER_PUBLIC
 *
 * Usage:	cat filename | auditfltr [[[-iN] [-oX]] | [-iX -oN]]
 * 			-i type = specifies the type of input
 * 			-o type = specifies the type of output
 *
 * Notes:	Auditfltr reads from stdin and writes to stdout.
 *              Auditfltr translates an audit log from machine dependent
 *              format to external data representation (XDR) and vice-versa.
 *		Output of command is in data format therefore stdout must 
 *              be redirected to a file or into to a pipe.
 *              Auditfltr is to be run in maintenance mode.
 */

/*LINTLIBRARY*/
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/privilege.h>
#include <mac.h>
#include <fcntl.h>
#include <sys/vnode.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <sys/sysmacros.h>
#include <sys/resource.h>
#include <sys/ts.h>
#include <sys/rt.h>
#include <sys/fc.h>
#include <sys/keyctl.h>
#include <audit.h>
#include <sys/auditrec.h>
#include "auditrptv4.h"

#define MSG_USAGE	 ":80:usage: auditfltr [[-iN] [-oX]] | [-iX -oN]\n"
#define FDR1		 ":81:conversion type %s is not supported\n"
#define FDR2 		 ":82:invalid combination of conversion types\n"
#define FDR3 		 ":83:input file is in invalid format\n"
#define FDR4 		 ":84:XDR encryption of an audit record field failed\n"
#define FDR5 		 ":85:XDR decryption of an audit record field failed\n"
#define SHORT	1
#define USHORT	2
#define INT	3
#define UINT	4
#define ENUM	5
#define LONG	6
#define ULONG	7
#define STRING	8
#define BYTES	9
#define NUMLEN	25	/* Number of ASCII characters representing the size of an XDR record */

extern int	char_spec;	/* is file character special? */	
extern int 	free_size;	/* size of free-format data */

typedef struct xdrrec {
	char *bufp;
	int bytect;
	struct xdrrec *next;
}wxdr_t;

static int to_xdr(), encd_mem(), wr_xdr(), to_mdf(), dcd_mem(), wr_mdf(); 
static void write_mv();

extern int getopt();
extern void free();
extern int version_number();			/* from adt_getrec.c */
extern int optind;   				/* getopt() */
extern char *optarg; 				/* getopt() */

/*
 * Function:     main                  
 * Privileges:   none
 * Restrcitions: none  
 */
main(argc, argv)
int	argc;
char	**argv;
{
	int c, rc;
	char inopt, outopt;
	dumprec_t dump_rec;
	static int	first_time = 1;		/* denote 1st call to adt_getrec */

	/* Initialize locale information */
	(void)setlocale(LC_ALL, "");

	/* Initialize message label */
	(void)setlabel("UX:auditfltr");

	/* Initialize catalog */
	(void)setcat("uxaudit");

	inopt = outopt = NULL;

	/* Parse the command line */
	while ((c = getopt(argc, argv, "i:o:")) != EOF) {
		switch (c) {
			case 'i':
				if (((*optarg == 'X') ||
				     (*optarg == 'N')) &&
				     (strlen(optarg) == 1))
					inopt = *optarg;
				else {
	                       		(void)pfmt(stderr, MM_ERROR, FDR1, optarg);
	        			(void)pfmt(stderr, MM_ACTION, MSG_USAGE);
					exit(ADT_BADSYN);
				}
				break;
			case 'o':
				if (((*optarg == 'X') ||
				     (*optarg == 'N')) &&
				     (strlen(optarg) == 1))
					outopt = *optarg;
				else {
	                       		(void)pfmt(stderr, MM_ERROR, FDR1, optarg);
	        			(void)pfmt(stderr, MM_ACTION, MSG_USAGE);
					exit(ADT_BADSYN);
				}
				break;
			case '?':
	                       	(void)pfmt(stderr, MM_ACTION, MSG_USAGE);
				exit(ADT_BADSYN);
		} /* end switch */
	} /* end while */
	
	/* Invalid argument combinations: -iX -oX, -iN -oN, -oN, -iX */
	if ((outopt == 'N') && ((inopt == 'N') || (inopt == NULL))) {
        	(void)pfmt(stderr, MM_ERROR, FDR2);
                (void)pfmt(stderr, MM_ACTION, MSG_USAGE);
		exit(ADT_BADSYN);
	}

	if ((inopt == 'X') && ((outopt == 'X') || (outopt == NULL))) {
                (void)pfmt(stderr, MM_ERROR, FDR2);
                (void)pfmt(stderr, MM_ACTION, MSG_USAGE);
		exit(ADT_BADSYN);
	}

	/* The command line is invalid if it contains an argument */
	if (optind < argc) {
        	(void)pfmt(stderr, MM_ACTION, MSG_USAGE);
		exit(ADT_BADSYN);
	}

	/*
	 * Input for auditfltr is stdin, therefore the input could be a
	 * special character device. Value used in adt_getrec().
	 */
	char_spec = 1;

	if ((inopt == 'N') || (inopt == NULL)) {
		/* Translate audit log file to XDR */

		/*
		 * adt_getrec() reads the audit log file from stdin, one
		 * audit record at a time. Each audit record field is then
		 * translated to XDR format. When the entire audit record
		 * has been translated the results are written to stdout.
		 */
		while ((rc = adt_getrec(stdin, &dump_rec)) <= 0 ) {
			/*
			 * If auditfltr has called this version specific
			 * auditfltrv4 program, the magic number and version
			 * number will not be in the data stream, but they
			 * must be put at the start of the XDR output file.
			 * If this is the first time in this loop and the return
			 * code from adt_getrec was not GOTNEWMV, write out the magic
			 * number and version number.
			 */
			if (first_time){
				first_time = 0;
				if (rc != GOTNEWMV){
					/* set the magic number to XDR */
					(void)memset(log_byord, NULL, ADT_BYORDLEN);
					(void)strcpy(log_byord, ADT_XDR_BYORD);

					/* set the version number */
					(void)memset(adt_ver, NULL, ADT_VERLEN);
					(void)strcpy(adt_ver, V4);

					/* write the magic number and version number */
					write_mv();
				}
			}
			/* if a new magic number and version number have been read */
			if (rc == GOTNEWMV){
				/* set the magic number to XDR */
				(void)memset(log_byord, NULL, ADT_BYORDLEN);
				(void)strcpy(log_byord, ADT_XDR_BYORD);

				/* write the magic number and version number */
				write_mv();
				continue;
			} 
			if (rc == 0){
				/* translate to xdr */
				if ((rc = to_xdr(&dump_rec)) != 0)
					break;
			} else {
					/* An error or end of file was encountered. */
					break;
			}
		}
	} else {
		/* Translate audit log file to machine dependent format */
		while ((rc = to_mdf()) == 0 )
			;
	}

	/*
	 * An error occurred during reading stdin or translation
	 * to machine dependent format or writing to stdout.
	 */
	if (rc >0)
		exit(rc);

	exit(ADT_SUCCESS);
/*NOTREACHED*/
} /* end main */


/*
 * Procedure:     write_mv
 * Privileges:    none
 * Restrictions:  none
 * Notes:	  Write the magic number (byte order) and version number
 *		  to the log.
 */
void
write_mv()
{

char *typep;

	/*
	 * Write the audit magic number indicating XDR or
	 * machine dependent format.
	 * Ascii string does not require conversion.
	 */
	if ((typep = (char *)calloc(ADT_BYORDLEN, sizeof(char))) == NULL) {
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		exit(ADT_MALLOC);
	}
	(void)strcpy(typep, log_byord);
	if (write(1, typep, ADT_BYORDLEN) != ADT_BYORDLEN) {
	    	(void)pfmt(stderr, MM_ERROR, E_FMERR);
                       exit(ADT_FMERR);
	}

	/*
	 * Write the version number, ascii string does not
	 * require conversion.
	 */
	if ((typep = (char *)calloc(ADT_VERLEN, sizeof(char))) == NULL) {
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		exit(ADT_MALLOC);
	}
	(void)strcpy(typep, adt_ver);
	if (write(1, typep, ADT_VERLEN) != ADT_VERLEN) {
	    	(void)pfmt(stderr, MM_ERROR, E_FMERR);
                       exit(ADT_FMERR);
	}
}

/*
 * Procedure:     to_xdr
 * Privileges:    none
 * Restrictions:  none
 * Notes:	  Conversion to xdr is dependent on sys/types.h           
 *                On machines other than the 386 the members of the audit
 *                record structures may be different data types.
 *
 *                Each field of the audit record passed in is translated to XDR.
 *		  When all required fields have been translated to XDR the 
 *		  results are written to stdout.
 */
int
to_xdr(dumpp)
dumprec_t *dumpp;
{
	int new_rsize = 0;
	wxdr_t *firstp, *lastp;
	short x;
	tacl_t *acl_structp;
	struct tsdpent *tsp;
	struct rtdpent *rtp;
	struct fcdpent *fcp;
	lwpid_t *lwpidp;
        unsigned long *lvlp, *countp, *crnump;
	gid_t *sgrpp;
	pid_t *pidp;
	struct k_skey *skeyp;
	char final_rsize[50];

	struct	cmn_rec *cmnp;
	struct	abuf_r  *abufp;
	struct	acl_r	*aclp;
	struct	actl_r  *actlp;
	struct	admin_r *adminp;
	struct	admp_r	*admpp;
	struct	aevt_r  *aevtp;
	struct	alog_r  *alogp;
	struct	bind_r	*bindp;
#ifdef CC_PARTIAL
	struct	cc_r	*ccp;
#endif
	struct	chmod_r	*chmodp;
	struct	chown_r	*chownp;
	struct	cred_rec *credp;
	struct	credf_rec *credfp;
	struct	cron_r	*cronp;
	struct	dev_r	*devp;
	struct	fchmod_r *fchmodp;
	struct	fchown_r *fchownp;
	struct	fcntl_r *fcntlp;
	struct	fcntlk_r *fcntlkp;
	struct	fd_r	*fdp;
	struct	fdev_r	*fdevp;
	struct	fmac_r	*fmacp;
	struct	fname_r	*fnamep;
	struct	fork_r	*forkp;
	struct	fpriv_r	*fprivp;
	struct	id_r	*idp;
	struct	ioctl_r	*ioctlp;
	struct	ipc_r	*ipcp;
	struct	ipcacl_r *ipcaclp;
	struct	keyctl_r *keyp;
	struct	kill_r	*killp;
	struct	login_r	*loginp;
	struct	lwpcreat_r *lwpcrp;
	struct	mac_r	*macp;
	struct	mctl_r	*mctlp;
	struct	modadm_r *modadmp;
	struct	modload_r *modloadp;
	struct	mount_r *mountp;	
	struct	online_r *onlinep;
	struct	parms_r *parmsp;
	struct	passwd_r *passwdp;
	struct	pipe_r	*pipep;
	struct	plock_r	*plockp;
	struct	recvfd_r *recvfdp;
	struct	rlim_r	*rlimp;
	struct	setgroup_r *setgroupp;
	struct	setid_r	*setidp;
	struct	setpgrp_r *setpgrpp;
	struct	ulimit_r *ulimp;
	struct	time_r	*timep;

	char sernum[SNLEN+1];		/* serial number for keyctl record */
	char serkey[SKLEN+1];		/* encrypted key for keyctl record */

	firstp = lastp = NULL;

	/*
	 * Convert the common record, unless this is a record 
	 * without a common record (filename or credential_free).
	 */
	if ((dumpp->cmn.c_rtype != FNAME_R) && (dumpp->cmn.c_rtype != CRFREE_R)){
		cmnp = (struct cmn_rec *)&(dumpp->cmn);

		if ((encd_mem((char *)&(cmnp->c_rtype), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_event), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
			return(ADT_XDRERR);

		/*
	 	 * Skip the common record field c_size. Record
         	 * size will be recalculated in XDR bytes.
	 	 */
		if ((encd_mem((char *)&(cmnp->c_seqnum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_pid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_time.tv_sec), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_time.tv_nsec), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_status), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_sid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_lwpid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_crseqnum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_rprivs), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_rprvstat), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(cmnp->c_scall), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
			return(ADT_XDRERR);
	}


	/*
	 * Translate the specific fields belonging to the record type. Each
	 * translated field is placed in a wxdr_t structure. An entire 
	 * translated audit record will be a link list of wxdr_t structures
	 */
	switch (dumpp->r_rtype) {
		case FILEID_R :  	
			idp = (struct id_r *)&(dumpp->spec_data);
			if ((encd_mem(idp->i_mmp, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem(idp->i_ddp, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(idp->i_flags), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ABUF_R:
			abufp = (struct abuf_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(abufp->a_vhigh), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ACL_R :
			aclp = (struct acl_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(aclp->a_nentries), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ACTL_R:
			actlp = (struct actl_r *)&(dumpp->spec_data);
			 if ((encd_mem((char *)&(actlp->a_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ADMIN_R:
			adminp = (struct admin_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(adminp->nentries), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ADMP_R:
			admpp = (struct admp_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(admpp->a_rtype), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(admpp->a_status), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case AEVT_R:
			aevtp = (struct aevt_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(aevtp->a_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			for (x = 0; x < ADT_EMASKSIZE - 1; x++) {
				if ((encd_mem((char *)&(aevtp->a_emask[x]), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			if ((encd_mem((char *)&(aevtp->a_uid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(aevtp->a_flags), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(aevtp->a_nlvls), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(aevtp->a_lvlmin), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(aevtp->a_lvlmax), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ALOG_R:
			alogp = (struct alog_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(alogp->a_flags), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(alogp->a_onfull), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(alogp->a_onerr), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(alogp->a_maxsize), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(alogp->a_seqnum), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem(alogp->a_mmp, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem(alogp->a_ddp, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem(alogp->a_pnodep, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem(alogp->a_anodep, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem(alogp->a_ppathp, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem(alogp->a_apathp, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem(alogp->a_progp, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case BIND_R:
			bindp = (struct bind_r *)&(dumpp->spec_data);
			
			if ((encd_mem((char *)&(bindp->b_idtype), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(bindp->b_id), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(bindp->b_cpuid), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(bindp->b_obind), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
#ifdef CC_PARTIAL
		case CC_R:
			ccp = (struct cc_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(ccp->cc_event), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ccp->cc_bps), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
#endif
		case CHMOD_R:
			chmodp = (struct chmod_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(chmodp->c_nmode), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case CHOWN_R:
			chownp = (struct chown_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(chownp->c_uid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(chownp->c_gid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case CRFREE_R:
			credfp = (struct credf_rec *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(credfp->cr_rtype), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(credfp->cr_padd), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(credfp->cr_ncrseqnum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case CRON_R:
			cronp = (struct cron_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&((cronp->c_acronrec).uid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((cronp->c_acronrec).gid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((cronp->c_acronrec).lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((cronp->c_acronrec).cronjob, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case DEV_R:
			devp = (struct dev_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&((devp->devstat).dev_relflag), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((devp->devstat).dev_mode), &new_rsize, &firstp, &lastp, USHORT))== ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((devp->devstat).dev_hilevel), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((devp->devstat).dev_lolevel), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((devp->devstat).dev_state), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((devp->devstat).dev_usecount), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case FCHMOD_R :
			fchmodp = (struct fchmod_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fchmodp->f_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fchmodp->f_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fchmodp->f_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fchmodp->f_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fchmodp->f_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fchmodp->f_chmod).c_nmode), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FCHOWN_R :
			fchownp = (struct fchown_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fchownp->f_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fchownp->f_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fchownp->f_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fchownp->f_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fchownp->f_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fchownp->f_chown).c_uid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fchownp->f_chown).c_gid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FCNTL_R :
			fcntlp = (struct fcntl_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fcntlp->f_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlp->f_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlp->f_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlp->f_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlp->f_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlp->f_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlp->f_arg), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FCNTLK_R :
			fcntlkp = (struct fcntlk_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fcntlkp->f_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlkp->f_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlkp->f_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlkp->f_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlkp->f_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fcntlkp->f_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fcntlkp->f_flock).l_type), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fcntlkp->f_flock).l_whence), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fcntlkp->f_flock).l_start), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fcntlkp->f_flock).l_len), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fcntlkp->f_flock).l_sysid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fcntlkp->f_flock).l_pid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			for(x = 0; x < 4; x++) {
				if ((encd_mem((char *)&((fcntlkp->f_flock).l_pad[x]), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			break;
		case FD_R:
			fdp = (struct fd_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fdp->f_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fdp->f_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fdp->f_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fdp->f_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fdp->f_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FDEV_R:
			fdevp = (struct fdev_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fdevp->f_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fdevp->f_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fdevp->f_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fdevp->f_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fdevp->f_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fdevp->devstat).dev_relflag), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fdevp->devstat).dev_mode), &new_rsize, &firstp, &lastp, USHORT))== ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fdevp->devstat).dev_hilevel), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fdevp->devstat).dev_lolevel), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fdevp->devstat).dev_state), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fdevp->devstat).dev_usecount), &new_rsize, &firstp, &lastp, USHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FNAME_R : 	
			fnamep = (struct fname_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fnamep->f_rtype), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fnamep->f_event), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			/*
	 	 	 * Skip the record field f_size. Record
         	 	 * size will be recalculated in XDR bytes.
	 	 	 */
			if ((encd_mem((char *)&(fnamep->f_seqnum), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fnamep->f_cmpcnt), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);

			if ((encd_mem((char *)&(fnamep->f_vnode.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fnamep->f_vnode.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fnamep->f_vnode.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fnamep->f_vnode.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fnamep->f_vnode.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FMAC_R:
			fmacp = (struct fmac_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fmacp->f_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fmacp->f_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fmacp->f_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fmacp->f_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(fmacp->f_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((fmacp->f_lid).l_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case FORK_R :  	
			forkp = (struct fork_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(forkp->f_cpid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(forkp->f_nlwp), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FPRIV_R :  	
			fprivp = (struct fpriv_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(fprivp->f_count), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case IOCTL_R:
			ioctlp = (struct ioctl_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(ioctlp->i_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ioctlp->i_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ioctlp->i_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ioctlp->i_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ioctlp->i_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ioctlp->i_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ioctlp->i_flag), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case IPCS_R:
			ipcp = (struct ipc_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(ipcp->i_id), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ipcp->i_op), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ipcp->i_flag), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ipcp->i_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case IPCACL_R :
			ipcaclp = (struct ipcacl_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(ipcaclp->i_id), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ipcaclp->i_nentries), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ipcaclp->i_type), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case KEYCTL_R :
			keyp = (struct keyctl_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(keyp->k_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(keyp->k_nskeys), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case KILL_R :
			killp = (struct kill_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(killp->k_sig), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(killp->k_entries), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case LOGIN_R:
			loginp = (struct login_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&((loginp->l_alogrec).uid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((loginp->l_alogrec).gid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((loginp->l_alogrec).ulid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((loginp->l_alogrec).hlid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&((loginp->l_alogrec).vlid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((loginp->l_alogrec).bamsg, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((loginp->l_alogrec).tty, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case LWPCREATE_R:
			lwpcrp = (struct lwpcreat_r *)&(dumpp->spec_data);
			if ((encd_mem((lwpcrp->l_newid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);

			break;
		case MAC_R:
			macp = (struct mac_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(macp->l_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case MCTL_R:
			mctlp = (struct mctl_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(mctlp->m_attr), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case MODADM_R:
			modadmp = (struct modadm_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(modadmp->m_type), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(modadmp->m_cmd), &new_rsize, &firstp, &lastp, UINT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(modadmp->m_data), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case MODLOAD_R:
			modloadp = (struct modload_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(modloadp->m_id), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case MODPATH_R:
                        break;
		case MOUNT_R:
			mountp = (struct mount_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(mountp->m_flags), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case ONLINE_R:
			onlinep = (struct online_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(onlinep->p_procid), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(onlinep->p_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case PARMS_R:
			parmsp = (struct parms_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(parmsp->p_upri), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(parmsp->p_uprisecs), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(parmsp->nentries), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case PASSWD_R:
			passwdp = (struct passwd_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&((passwdp->p_apasrec).nuid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case PIPE_R:
			pipep = (struct pipe_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(pipep->p_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(pipep->p_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(pipep->p_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(pipep->p_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(pipep->p_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case PLOCK_R:
			plockp = (struct plock_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(plockp->p_op), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case RECVFD_R:
			recvfdp = (struct recvfd_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(recvfdp->r_fdinfo.v_type), &new_rsize, &firstp, &lastp, ENUM)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(recvfdp->r_fdinfo.v_fsid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(recvfdp->r_fdinfo.v_dev), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(recvfdp->r_fdinfo.v_inum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(recvfdp->r_fdinfo.v_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(recvfdp->r_spid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(recvfdp->r_slwpid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case RLIM_R:
			rlimp = (struct rlim_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(rlimp->r_rsrc), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(rlimp->r_soft), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(rlimp->r_hard), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case SETGRPS_R:
			setgroupp = (struct setgroup_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(setgroupp->s_ngroups), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case SETPGRP_R:
			setpgrpp = (struct setpgrp_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(setpgrpp->s_flag), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(setpgrpp->s_pid), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(setpgrpp->s_pgid), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
				
                        break;
		case SETID_R :
			setidp = (struct setid_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(setidp->s_nid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case TIME_R:
			timep = (struct time_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(timep->t_time.tv_sec), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(timep->t_time.tv_nsec), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case ULIMIT_R:
			ulimp = (struct ulimit_r *)&(dumpp->spec_data);
			if ((encd_mem((char *)&(ulimp->u_cmd), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
				return(ADT_XDRERR);
			if ((encd_mem((char *)&(ulimp->u_arg), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		default :
                        break;
	}

	/*
	 * If c_crseqnum is 0, and this is not a credfree or fname record,
	 * this record contains a cred record.
	 */
	if ((dumpp->cmn.c_rtype != FNAME_R) && (dumpp->cmn.c_rtype != CRFREE_R) && (dumpp->cmn.c_crseqnum == 0)){
		/* translate the cred record */
		credp = (struct cred_rec *)&(dumpp->cred);
		if ((encd_mem((char *)&(credp->cr_crseqnum), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(credp->cr_lid), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(credp->cr_uid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(credp->cr_gid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(credp->cr_ruid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(credp->cr_rgid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(credp->cr_maxpriv), &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
			return(ADT_XDRERR);
		if ((encd_mem((char *)&(credp->cr_ngroups), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
			return(ADT_XDRERR);

		/* If groups are present, convert each group */
		for (x = 0; x < dumpp->cred.cr_ngroups; x++){
			if ((encd_mem((char *)&(dumpp->groups[x]), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
				return(ADT_XDRERR);
		}
	}

	/*
	 * Convert the free format data. The record types: ACL_R, ADMIN_R, 
         * AEVT_R, CRFREE_R, FPRIV_R, IPCACL_R, KEYCTL_R, KILL_R, and PARMS_R,
	 * SETGRPS_R are processed specially.
	 */
	switch (dumpp->r_rtype) {
		case ACL_R :
			acl_structp = (tacl_t *)dumpp->freeformat;
			for (x = 0; x < aclp->a_nentries; x++, acl_structp++) {
				if ((encd_mem((char *)&(acl_structp->ttype), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
					return(ADT_XDRERR);
				if ((encd_mem((char *)&(acl_structp->tid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
					return(ADT_XDRERR);
				if ((encd_mem((char *)&(acl_structp->tperm), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			dumpp->freeformat = NULL;
			break;
		case ADMIN_R:
			/*
			 * Determine if the event generated an RT, FC, or
			 * TS record, and encode the appropriate type.
			 */
			switch (dumpp->cmn.c_event) {
			case ADT_SCHED_TS:
				tsp = (struct tsdpent *)dumpp->freeformat;
				for (x = 0; x < adminp->nentries; x++, tsp++) {
					if ((encd_mem((char *)&(tsp->ts_globpri), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
						return(ADT_XDRERR);
					if ((encd_mem((char *)&(tsp->ts_quantum), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
						return(ADT_XDRERR);
					if ((encd_mem((char *)&(tsp->ts_tqexp), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
						return(ADT_XDRERR);
					if ((encd_mem((char *)&(tsp->ts_slpret), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
						return(ADT_XDRERR);
					if ((encd_mem((char *)&(tsp->ts_maxwait), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
						return(ADT_XDRERR);
					if ((encd_mem((char *)&(tsp->ts_lwait), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
						return(ADT_XDRERR);
				}
				break;
			/* ADT_SCHED_RT and ADT_SCHED_FP are the same value */
			case ADT_SCHED_RT:
				rtp = (struct rtdpent *)dumpp->freeformat;
				for (x = 0; x < adminp->nentries; x++, rtp++) {
					if ((encd_mem((char *)&(rtp->rt_globpri), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
						return(ADT_XDRERR);
					if ((encd_mem((char *)&(rtp->rt_quantum), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
						return(ADT_XDRERR);
				}
				break;
			case ADT_SCHED_FC:
				fcp = (struct fcdpent *)dumpp->freeformat;
				for (x = 0; x < adminp->nentries; x++, fcp++) {
					if ((encd_mem((char *)&(fcp->fc_globpri), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
						return(ADT_XDRERR);
					if ((encd_mem((char *)&(fcp->fc_quantum), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
						return(ADT_XDRERR);
				}
				break;
			}
			dumpp->freeformat = NULL;
			break;
		case AEVT_R:
			lvlp = (unsigned long *)dumpp->freeformat;
			for (x = 0; x < aevtp->a_nlvls; x++, lvlp++) {
				if ((encd_mem((char *)lvlp, &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			dumpp->freeformat = NULL;
			break;
		case CRFREE_R :
			/* Process freeformat data uniquely */
                        crnump = (ulong_t *)dumpp->freeformat;
                        for (x = 0; x < credfp->cr_ncrseqnum; x++, crnump++) {
                         	if ((encd_mem((char *)crnump, &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
                  			return(ADT_XDRERR);
                        }
                        dumpp->freeformat = NULL;
			break;
		case FPRIV_R :  	
			countp = (unsigned long *)dumpp->freeformat;
			for (x = 0; x < fprivp->f_count; x++, countp++) {
				if ((encd_mem((char *)countp, &new_rsize, &firstp, &lastp, ULONG)) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			dumpp->freeformat = NULL;
			break;
		case IPCACL_R :
			/* Process freeformat data uniquely */
			acl_structp = (tacl_t *)dumpp->freeformat;
			for (x = 0; x < ipcaclp->i_nentries; x++, acl_structp++) {
				if ((encd_mem((char *)&(acl_structp->ttype), &new_rsize, &firstp, &lastp, INT)) == ADT_XDRERR)
					return(ADT_XDRERR);
				if ((encd_mem((char *)&(acl_structp->tid), &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
					return(ADT_XDRERR);
				if ((encd_mem((char *)&(acl_structp->tperm), &new_rsize, &firstp, &lastp, SHORT)) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			dumpp->freeformat = NULL;
			break;
		case KEYCTL_R :
			/* Process freeformat data uniquely */
                        skeyp = (struct k_skey *)dumpp->freeformat;
                        for (x = 0; x < keyp->k_nskeys; x++, skeyp++) {
				/* make sure serial number and key fields are NULL terminated */
                        	(void)memcpy(sernum, '\0', SNLEN+1);
                        	(void)memcpy(sernum, skeyp->sernum, SNLEN);
                        	(void)memcpy(serkey, '\0', SKLEN+1);
                        	(void)memcpy(serkey, skeyp->serkey, SKLEN);

				if ((encd_mem(sernum, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
					return(ADT_XDRERR);
				if ((encd_mem(serkey, &new_rsize, &firstp, &lastp, STRING)) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			
                        dumpp->freeformat = NULL;
			break;
		case KILL_R :
			/* Process freeformat data uniquely */
                        pidp = (pid_t *)dumpp->freeformat;
                        for (x = 0; x < killp->k_entries; x++, pidp++) {
                         	if ((encd_mem((char *)pidp, &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
                  			return(ADT_XDRERR);
                        }
                        dumpp->freeformat = NULL;
			break;
		case PARMS_R:
			/* Encode the lwpids in the freeformat data */
			lwpidp = (lwpid_t *)dumpp->freeformat;
			for (x = 0; x < parmsp->nentries; x++, lwpidp++) {
                         	if ((encd_mem((char *)lwpidp, &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
                  			return(ADT_XDRERR);
			}
			
                        dumpp->freeformat = NULL;
			break;
		case SETGRPS_R:
			/* Process freeformat data uniquely */
			sgrpp = (gid_t *)dumpp->freeformat;
			for (x = 0; x < setgroupp->s_ngroups; x++, sgrpp++) {
				if ((encd_mem((char *)sgrpp, &new_rsize, &firstp, &lastp, LONG)) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			dumpp->freeformat = NULL;
			break;
		default:
			if (dumpp->freeformat != NULL) {
				if (encd_mem(dumpp->freeformat, &new_rsize, &firstp, &lastp, BYTES) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
	}

	/*
	 * Final record size: number of encoded bytes.
	 * Excluding: rsize.
	 * Note:trailing blank needed for subsequent fscanf
	 *     ASCII string does not require conversion to XDR
	 */
	(void)sprintf(final_rsize, "%d%c", new_rsize, ' ');

	/* Write the translated audit record to stdout */
	if (wr_xdr(firstp, final_rsize) != 0)
		return(ADT_FMERR);

	return(0);
}

/*
 * Procedure:     encd_mem
 * Privileges:    none
 * Restrictions:  none
 * Notes:         Each audit record field and a constant defining its type is passed in.
 *                The audit record field is then translated to XDR, and stored in a 
 *                wxdr_t structure.
 */
int
encd_mem(memp, new_rsize, firstp, lastp, conv_type)
char *memp;
int *new_rsize;
wxdr_t **firstp;
wxdr_t **lastp;
short conv_type;
{
	enum_t v;		 /* XDR typedef */
	int arr_size, x;
	unsigned int u;
	u_int length;
	long y;
	unsigned long z;
	unsigned short w;
	short t;
	wxdr_t *tmpp;
	XDR *xdrp;
	XDR xdr;
	static int len = 100;

	xdrp = &xdr;

	if ((tmpp = (wxdr_t *)malloc (sizeof(wxdr_t))) == NULL) {
		(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
		exit(ADT_MALLOC);
        }

	(void)memset((char *)tmpp, '\0', sizeof(wxdr_t));

	if ((conv_type == STRING) || (conv_type == BYTES)) {
		/* Calculate space:string + XDR padding */
		if (conv_type == STRING)
			arr_size = (strlen(memp) + len);
		else
			/*
			 * The free-format data for the MODADM_R
			 * records may contain embedded null bytes.       
			 * Therefore strlen() can not be used to calculate
			 * length of data. The free-format data is treated
			 * as a sequence of bytes not a string due to the
			 * possibility of embedded null bytes.
			 */
			arr_size = free_size + len;

		/* Create a buffer to hold the XDR conversion */
		if ((tmpp->bufp = (char *)calloc(arr_size, sizeof(char))) == NULL) {
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			exit(ADT_MALLOC);
		}
		xdrmem_create(xdrp, tmpp->bufp, (arr_size * sizeof(char)), XDR_ENCODE);
	} else {
		/*
		 * FOR SHORT, INT, LONG, USHORT, UINT, ULONG, ENUM,
		 * create a buffer to hold the XDR conversion.
		 */
		if ((tmpp->bufp = (char *)calloc(len, sizeof(char))) == NULL) {
			(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
			exit(ADT_MALLOC);
		}

		xdrmem_create(xdrp, tmpp->bufp, (len * sizeof(char)), XDR_ENCODE);
	}

	switch(conv_type){
		case ENUM:
			v = *(enum_t *)memp;
			if (xdr_enum(xdrp, &v) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
		case SHORT:
			t = *(short *)memp;
			if (xdr_short(xdrp, &t) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
		case USHORT:
			w = *(unsigned short *)memp;
			if (xdr_u_short(xdrp, &w) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
		case INT:
			x = *(int *)memp;
			if (xdr_int(xdrp, &x) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
		case UINT:
			u = *(unsigned int *)memp;
			if (xdr_u_int(xdrp, &u) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
		case LONG:
			y = *(long *)memp;
			if (xdr_long(xdrp, &y) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
		case ULONG:
			z = *(unsigned long *)memp;
			if (xdr_u_long(xdrp, &z) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
		case STRING:
			if (xdr_string(xdrp, &memp, arr_size) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
		case BYTES:
			length = free_size;
			if (xdr_bytes(xdrp, &memp, &length, arr_size) != 1) {
				(void)pfmt(stderr, MM_ERROR, FDR4);
				return(ADT_XDRERR);
			}
			break;
	}

	/* The number of encoded bytes for this audit record field */
	tmpp->bytect = xdrp->x_private - xdrp->x_base;
	tmpp->next = NULL;

	/* Total number of encoded bytes so far */
	*new_rsize = *new_rsize + tmpp->bytect;
	
	/*
	 * Attach the wxdr_t structure for this audit record field 
	 * to the link list of wxdr_t structures for the audit record
	 */
	if (*firstp == NULL) {
		*firstp = tmpp;
		*lastp = tmpp;
	} else {
		(*lastp)->next = tmpp;
		*lastp = tmpp;
	}
	return(0);
		
}
/*
 * Procedure:     wr_xdr
 * Privileges:    none
 * Restrictions:  none
 * Notes:         For each audit record write a translated XDR record to stdout
 *                The layout of an XDR record:
 *                    -the number of encoded bytes (in ASCII)
 *                    -the common record (excluding rsize member)
 *                    -the unique members
 *                    -the free format data
 *                    -the cred record and groups
 */
int
wr_xdr(firstp, final_rsize)
wxdr_t *firstp;
char *final_rsize;
{
	wxdr_t *freep;

	/* Write the size (number of bytes) of the entire translated audit record */
	if (write(1, final_rsize, strlen(final_rsize)) == -1) {
		(void)pfmt(stderr, MM_ERROR, E_FMERR);
		return(ADT_FMERR);
	}

	/*
	 * Write the members of the linked list, common record, unique
	 * members, freeformat, cred record, and groups info.
	 * Free up the link list of wxdr_t structures.
	 */
	while (firstp) {
		if (write(1, firstp->bufp, firstp->bytect) == -1) {
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		freep = firstp;
		firstp = firstp->next;
		free(freep->bufp);
		free((char *)freep);
	}
	return(0);
}

/*
 * Procedure:	  to_mdf
 * Privileges:	  none
 * Restrictions:  none
 * Notes:	  Read the XDR audit record from stdin, translates the record
 *                to machine dependent format and write the results to stdout.
 */

int
to_mdf()
{
	int ff_toword = 0;
	int spec_size = 0;
	int len;
	int xdr_rsize = 0;
	short x;
	unsigned long *ulongp, *crnump;
	struct tsdpent *tsp;
	struct rtdpent *rtp;
	struct fcdpent *fcp;
	lwpid_t *lwpidp;
	level_t *lvlp;
	char *bufp, *origp;
	tacl_t *acl_structp;
	dumprec_t *dumpp, drec;
	gid_t *sgrpp;
	pid_t *pidp;
	struct k_skey *skeyp;

	struct	cmn_rec *cmnp;
	struct	abuf_r	*abufp;
	struct	acl_r	*aclp;
	struct	actl_r	*actlp;
	struct	admin_r *adminp;
	struct	admp_r  *admpp;
	struct	aevt_r	*aevtp;
	struct	alog_r	*alogp;
	struct	bind_r	*bindp;
#ifdef CC_PARTIAL
	struct	cc_r	*ccp;
#endif
	struct	chmod_r *chmodp;
	struct	chown_r *chownp;
	struct	cred_rec *credp;
	struct	credf_rec *credfp;
	struct	cron_r	*cronp;
	struct	dev_r	*devp;
	struct	fchmod_r *fchmodp;
	struct	fchown_r *fchownp;
	struct	fcntl_r *fcntlp;
	struct	fcntlk_r *fcntlkp;
	struct	fd_r	*fdp;
	struct	fdev_r	*fdevp;
	struct	fmac_r	*fmacp;
	struct	fname_r *fnamep;
	struct	fork_r	*forkp;
	struct	fpriv_r *fprivp;
	struct	id_r	*idp;
	struct	ioctl_r *ioctlp;
	struct	ipc_r	*ipcp;
	struct	ipcacl_r *ipcaclp;
	struct	keyctl_r *keyp;
	struct	kill_r	*killp;
	struct	login_r *loginp;
	struct	lwpcreat_r *lwpcrp;
	struct	mac_r	*macp;
	struct	mctl_r	*mctlp;
	struct	modadm_r *modadmp;
	struct	modload_r *modloadp;
	struct	mount_r *mountp;
	struct	online_r *onlinep;
	struct	parms_r *parmsp;
	struct	passwd_r *passwdp;
	struct	pipe_r	*pipep;
	struct	plock_r *plockp;
	struct	recvfd_r *recvfdp;
	struct	rlim_r  *rlimp;
	struct	setgroup_r *setgroupp;
	struct	setid_r *setidp;
	struct	setpgrp_r *setpgrpp;
	struct	ulimit_r *ulimp;
	struct	time_r	*timep;

	char sernum[SNLEN+1];		/* serial number for keyctl record */
	char serkey[SKLEN+1];		/* encrypted key for keyctl record */

	int i;
	int bufsiz = 0;
	char inbuf[512];	/* hold XDR record size or magic number */
	static int first_time = 1;		/* denote 1st call to to_mdf */


	dumpp = &drec;
	(void)memset((char *)dumpp, '\0', sizeof(dumprec_t));
	(void)memset((char *)inbuf, NULL, sizeof(inbuf));

	/*
	 * Read in a string which is the size of this record or the magic number.
	 * If it is the record size it will be terminated by a space.
	 * If it is a magic number it will be "XDR" followed by NULLs.
	 */
	if (fread(&inbuf[0], 1, sizeof(char), stdin) == 0)
		return(-1);
	/* a magic number or a record size should never exceed NUMLEN characters */
	for (i = 1; i < NUMLEN; i++ ){
		if (fread(&inbuf[i], 1, sizeof(char), stdin) == 0){
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		if (inbuf[i] == ' ')	/* this is probably a record size */  
			break;
		if (inbuf[i] == NULL)	/* this is probably a magic number */  
			break;
	}
	if (i == NUMLEN) {
		(void)pfmt(stderr, MM_ERROR, E_FMERR);
		return(ADT_FMERR);
	}

	/*
	 * The contents of inbuf will be an ascii string. This string may
	 * be a magic number (indicating the start of a new logfile) or it
	 * may be the size of an XDR record.
	 * If it is a numeric value (as determined by atoi) then it is
	 * the size of an XDR record, otherwise it will be checked as a
	 * magic number (note: all magic numbers currently defined start
	 * with an alpha char and in this case the only valid magic number
	 * is ADT_XDR_BYORD (XDR).
	 */
	if ( (xdr_rsize = atoi(inbuf)) == 0){
		/*
		 * Compare the characters that have been read, with
		 * the magic number.
		 */
		if (strcmp(inbuf, ADT_XDR_BYORD) == 0){
			/*
			 * If a magic number was read, it may not have read 
			 * ADT_BYORDLEN bytes. Read the remainder of the magic
			 * number string (one null has already been read).
			 */
			if ((bufsiz = (ADT_BYORDLEN - (strlen(inbuf) + 1) )) > 0){
				if (fread(&inbuf[bufsiz], 1, bufsiz, stdin) != bufsiz){
					(void)pfmt(stderr, MM_ERROR, E_FMERR);
					return(ADT_FMERR);
				}
			}

			/* read the version number */
			if (version_number(stdin) != 0){
				(void)pfmt(stderr, MM_ERROR, E_NOCOMPATVER);
				exit(ADT_FMERR);
			}
			/* set first_time to false */
			first_time = 0;

			/* set the magic number for this architecture */
			(void)memset(log_byord, NULL, ADT_BYORDLEN);
			(void)strcpy(log_byord, ADT_BYORD);
			write_mv();	/* write the magic and version numbers */

			return(0);
		} else {
			/*
			 * This is an invalid magic number
			 */
			(void)pfmt(stderr, MM_ERROR, FDR3);
			return(ADT_BADARCH);
		}
	}

	/* to reach this point, an XDR record size was read */
	if ((origp = (char *)calloc(xdr_rsize, sizeof(char))) == NULL) {
        	(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
               	return(ADT_MALLOC);
	}
	bufp = origp;

	/* Read in the XDR "audit record" */
	if (fread(bufp, 1, xdr_rsize, stdin) != xdr_rsize) {
		(void)pfmt(stderr, MM_ERROR, E_FMERR);
		return(ADT_FMERR);
	}

	/*
	 * If auditfltr has called this version specific
	 * auditfltrv4 program, the magic number and version
	 * number will not be in the data stream, but they
	 * must be put at the start of the MDF output file.
	 * If this is the first time to_mdf() is called
	 * write out the magic number and version number.
	 */
	if (first_time){
		/* set first_time to false */
		first_time = 0;

		/* set the magic number to machine dependent format */
		(void)memset(log_byord, NULL, ADT_BYORDLEN);
		(void)strcpy(log_byord, ADT_BYORD);

		/* set the version number */
		(void)memset(adt_ver, NULL, ADT_VERLEN);
		(void)strcpy(adt_ver, V4);

		/* write the magic number and version number */
		write_mv();
	}

	cmnp = (struct cmn_rec *)&(dumpp->cmn);
	/* Decode the record type */
	if (dcd_mem((char *)&(cmnp->c_rtype), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
		return(ADT_XDRERR);
	/*
	 * Decode the rest of the common record, unless this is a
	 * filename or credential_free record (a record without a common).
	 */
	if ((dumpp->cmn.c_rtype != FNAME_R) && (dumpp->cmn.c_rtype != CRFREE_R)){

		if (dcd_mem((char *)&(cmnp->c_event), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
			return(ADT_XDRERR);
		/*
	 	* Size will be recalculated in MDF bytes
	 	*/
		if (dcd_mem((char *)&(cmnp->c_seqnum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_pid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_time.tv_sec), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_time.tv_nsec), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_status), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_sid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_lwpid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_crseqnum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_rprivs), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_rprvstat), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(cmnp->c_scall), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
			return(ADT_XDRERR);
	}

	switch (cmnp->c_rtype){
		case FILEID_R :  	
			idp = (struct id_r *)&(dumpp->spec_data);
			if (dcd_mem(idp->i_mmp, sizeof(idp->i_mmp), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem(idp->i_ddp, sizeof(idp->i_ddp), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(idp->i_flags), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ABUF_R:
			abufp = (struct abuf_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(abufp->a_vhigh), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case ACL_R :
			aclp = (struct acl_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(aclp->a_nentries), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ACTL_R:
			actlp = (struct actl_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(actlp->a_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ADMIN_R:
			adminp = (struct admin_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(adminp->nentries), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ADMP_R:
			admpp = (struct admp_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(admpp->a_rtype), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(admpp->a_status), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case AEVT_R:
			aevtp = (struct aevt_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(aevtp->a_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			for (x = 0; x < ADT_EMASKSIZE - 1; x++) {
				if (dcd_mem((char *)&(aevtp->a_emask[x]), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			if (dcd_mem((char *)&(aevtp->a_uid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(aevtp->a_flags), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(aevtp->a_nlvls), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(aevtp->a_lvlmin), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(aevtp->a_lvlmax), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ALOG_R:
			alogp = (struct alog_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(alogp->a_flags), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(alogp->a_onfull), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(alogp->a_onerr), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(alogp->a_maxsize), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(alogp->a_seqnum), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem(alogp->a_mmp, sizeof(alogp->a_mmp), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem(alogp->a_ddp, sizeof(alogp->a_ddp), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem(alogp->a_pnodep, sizeof(alogp->a_pnodep), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem(alogp->a_anodep, sizeof(alogp->a_anodep), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem(alogp->a_ppathp, sizeof(alogp->a_ppathp), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem(alogp->a_apathp, sizeof(alogp->a_apathp), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem(alogp->a_progp, sizeof(alogp->a_progp), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case BIND_R:
			bindp = (struct bind_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(bindp->b_idtype), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(bindp->b_id), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(bindp->b_cpuid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(bindp->b_obind), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);

			break;
#ifdef CC_PARTIAL
		case CC_R:
			ccp = (struct cc_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(ccp->cc_event), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ccp->cc_bps), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
#endif
		case CHMOD_R:
			chmodp = (struct chmod_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(chmodp->c_nmode), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case CHOWN_R:
			chownp = (struct chown_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(chownp->c_uid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(chownp->c_gid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case CRFREE_R:
			credfp = (struct credf_rec *)&(dumpp->spec_data);
			/* The rtype has already been read, this record has no common */
			credfp->cr_rtype = cmnp->c_rtype;
			if (dcd_mem((char *)&(credfp->cr_padd), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(credfp->cr_ncrseqnum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case CRON_R:
			cronp = (struct cron_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&((cronp->c_acronrec).uid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((cronp->c_acronrec).gid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((cronp->c_acronrec).lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((cronp->c_acronrec).cronjob, sizeof((cronp->c_acronrec).cronjob), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case DEV_R:
			devp = (struct dev_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&((devp->devstat).dev_relflag), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((devp->devstat).dev_mode), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((devp->devstat).dev_hilevel), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((devp->devstat).dev_lolevel), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((devp->devstat).dev_state), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((devp->devstat).dev_usecount), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case FCHMOD_R :
			fchmodp = (struct fchmod_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(fchmodp->f_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fchmodp->f_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fchmodp->f_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fchmodp->f_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fchmodp->f_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fchmodp->f_chmod).c_nmode), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FCHOWN_R :
			fchownp = (struct fchown_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(fchownp->f_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fchownp->f_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fchownp->f_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fchownp->f_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fchownp->f_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fchownp->f_chown).c_uid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fchownp->f_chown).c_gid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FCNTL_R :
			fcntlp = (struct fcntl_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(fcntlp->f_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlp->f_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlp->f_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlp->f_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlp->f_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlp->f_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlp->f_arg), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FCNTLK_R :
			fcntlkp = (struct fcntlk_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(fcntlkp->f_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlkp->f_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlkp->f_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlkp->f_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlkp->f_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fcntlkp->f_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fcntlkp->f_flock).l_type), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fcntlkp->f_flock).l_whence), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fcntlkp->f_flock).l_start), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fcntlkp->f_flock).l_len), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fcntlkp->f_flock).l_sysid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fcntlkp->f_flock).l_pid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			for (x = 0; x < 4; x++) {
				if (dcd_mem((char *)&((fcntlkp->f_flock).l_pad[x]), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
					return(ADT_XDRERR);
			}
			break;
		case FD_R :
			fdp = (struct fd_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(fdp->f_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fdp->f_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fdp->f_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fdp->f_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fdp->f_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FDEV_R :
			fdevp = (struct fdev_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(fdevp->f_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fdevp->f_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fdevp->f_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fdevp->f_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fdevp->f_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fdevp->devstat).dev_relflag), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fdevp->devstat).dev_mode), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fdevp->devstat).dev_hilevel), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fdevp->devstat).dev_lolevel), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fdevp->devstat).dev_state), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fdevp->devstat).dev_usecount), 0, xdr_rsize, &bufp, USHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FNAME_R : 	
			fnamep = (struct fname_r *)&(dumpp->spec_data);
			/* The rtype has already been read, this record has no common */
			fnamep->f_rtype = cmnp->c_rtype;
			if (dcd_mem((char *)&(fnamep->f_event), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			/*
			 * Size will be recomputed.
			 */
			if (dcd_mem((char *)&(fnamep->f_seqnum), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fnamep->f_cmpcnt), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);

			if (dcd_mem((char *)&(fnamep->f_vnode.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fnamep->f_vnode.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fnamep->f_vnode.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fnamep->f_vnode.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fnamep->f_vnode.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FMAC_R:
			fmacp = (struct fmac_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(fmacp->f_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fmacp->f_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fmacp->f_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fmacp->f_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(fmacp->f_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((fmacp->f_lid).l_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FORK_R :  	
			forkp = (struct fork_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(forkp->f_cpid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(forkp->f_nlwp), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case FPRIV_R:
			fprivp = (struct fpriv_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(fprivp->f_count), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case IOCTL_R:
			ioctlp = (struct ioctl_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(ioctlp->i_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ioctlp->i_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ioctlp->i_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ioctlp->i_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ioctlp->i_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ioctlp->i_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ioctlp->i_flag), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case IPCS_R:
			ipcp = (struct ipc_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(ipcp->i_id), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ipcp->i_op), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ipcp->i_flag), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ipcp->i_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case IPCACL_R :
			ipcaclp = (struct ipcacl_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(ipcaclp->i_id), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ipcaclp->i_nentries), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ipcaclp->i_type), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case KEYCTL_R :
			keyp = (struct keyctl_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(keyp->k_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(keyp->k_nskeys), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case KILL_R :
			killp = (struct kill_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(killp->k_sig), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(killp->k_entries), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case LOGIN_R:
			loginp = (struct login_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&((loginp->l_alogrec).uid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((loginp->l_alogrec).gid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((loginp->l_alogrec).ulid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((loginp->l_alogrec).hlid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&((loginp->l_alogrec).vlid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((loginp->l_alogrec).bamsg, sizeof((loginp->l_alogrec).bamsg), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((loginp->l_alogrec).tty, sizeof((loginp->l_alogrec).tty), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case LWPCREATE_R:
			lwpcrp = (struct lwpcreat_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(lwpcrp->l_newid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case MAC_R:
			macp = (struct mac_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(macp->l_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case MCTL_R:
			mctlp = (struct mctl_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(mctlp->m_attr), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case MODADM_R:
			modadmp = (struct modadm_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(modadmp->m_type), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(modadmp->m_cmd), 0, xdr_rsize, &bufp, UINT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(modadmp->m_data), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case MODLOAD_R:
			modloadp = (struct modload_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(modloadp->m_id), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case MODPATH_R:
                        break;
		case MOUNT_R:
			mountp = (struct mount_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(mountp->m_flags), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case ONLINE_R:
			onlinep = (struct online_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(onlinep->p_procid), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(onlinep->p_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case PARMS_R:
			parmsp = (struct parms_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(parmsp->p_upri), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(parmsp->p_uprisecs), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(parmsp->nentries), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case PASSWD_R:
			passwdp = (struct passwd_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&((passwdp->p_apasrec).nuid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case PIPE_R:
			pipep = (struct pipe_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(pipep->p_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(pipep->p_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(pipep->p_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(pipep->p_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(pipep->p_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case PLOCK_R:
			plockp = (struct plock_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(plockp->p_op), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case RECVFD_R:
			recvfdp = (struct recvfd_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(recvfdp->r_fdinfo.v_type), 0, xdr_rsize, &bufp, ENUM) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(recvfdp->r_fdinfo.v_fsid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(recvfdp->r_fdinfo.v_dev), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(recvfdp->r_fdinfo.v_inum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(recvfdp->r_fdinfo.v_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(recvfdp->r_spid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(recvfdp->r_slwpid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case RLIM_R:
			rlimp = (struct rlim_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(rlimp->r_rsrc), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(rlimp->r_soft), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(rlimp->r_hard), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case SETGRPS_R:
			setgroupp = (struct setgroup_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(setgroupp->s_ngroups), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case SETPGRP_R:
			setpgrpp = (struct setpgrp_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(setpgrpp->s_flag), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(setpgrpp->s_pid), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(setpgrpp->s_pgid), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case SETID_R :
			setidp = (struct setid_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(setidp->s_nid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			break;
		case ULIMIT_R:
			ulimp = (struct ulimit_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(ulimp->u_cmd), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(ulimp->u_arg), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		case TIME_R:
			timep = (struct time_r *)&(dumpp->spec_data);
			if (dcd_mem((char *)&(timep->t_time.tv_sec), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
			if (dcd_mem((char *)&(timep->t_time.tv_sec), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
                        break;
		default :
                        break;
	}

	/*
	 * If this is not a credfree or fname record, and
	 *  c_crseqnum is 0, this record contains a cred record.
	 */
	if ((dumpp->cmn.c_rtype != FNAME_R) && (dumpp->cmn.c_rtype != CRFREE_R) && (dumpp->cmn.c_crseqnum == 0)){
		credp = (struct cred_rec *)&(dumpp->cred);
		if (dcd_mem((char *)&(credp->cr_crseqnum), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(credp->cr_lid), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(credp->cr_uid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(credp->cr_gid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(credp->cr_ruid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(credp->cr_rgid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(credp->cr_maxpriv), 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		if (dcd_mem((char *)&(credp->cr_ngroups), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
			return(ADT_XDRERR);
		/* If groups are present, convert each group */
		for (x = 0; x < dumpp->cred.cr_ngroups; x++){
			if (dcd_mem((char *)&(dumpp->groups[x]), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
				return(ADT_XDRERR);
		}
	}

	/*
	 * Convert the free format data. The record types: ACL_R, ADMIN_R, 
         * AEVT_R, CRFREE_R, FPRIV_R, IPCACL_R, KEYCTL_R, KILL_R, and PARMS_R,
	 * SETGRPS_R are processed specially.
	 */
	switch (dumpp->r_rtype) {
		case ACL_R :
			if (aclp->a_nentries > 0) {
				if ((dumpp->freeformat = (char *)malloc(aclp->a_nentries * sizeof(tacl_t))) == NULL) {
        				(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                			return(ADT_MALLOC);
				}
				acl_structp = (tacl_t *)dumpp->freeformat;
				for (x = 0; x < aclp->a_nentries; x++, acl_structp++) {
					if (dcd_mem((char *)&(acl_structp->ttype), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
						return(ADT_XDRERR);
					if (dcd_mem((char *)&(acl_structp->tid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
						return(ADT_XDRERR);
					if (dcd_mem((char *)&(acl_structp->tperm), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
						return(ADT_XDRERR);
				}
				/* Calculate size of free-format (sizeof(struct) round to word boundary) */
				ff_toword = aclp->a_nentries * sizeof(tacl_t);
			}
			break;
		case ADMIN_R:
			if (adminp->nentries > 0) {
				/*
				 * Determine if the event generated an RT, FC, or
				 * TS record, and decode the appropriate type.
				 */
				switch (dumpp->cmn.c_event) {
				case ADT_SCHED_TS:
					if ((dumpp->freeformat = (char *)malloc(adminp->nentries *
					    sizeof(struct tsdpent))) == NULL) {
        					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                				return(ADT_MALLOC);
					}
					/* Calculate size of free-format (sizeof(struct) round to word boundary) */
					ff_toword = adminp->nentries * sizeof(struct tsdpent);
					tsp = (struct tsdpent *)dumpp->freeformat;
					for (x = 0; x < adminp->nentries; x++, tsp++) {
						if (dcd_mem((char *)&(tsp->ts_globpri), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
							return(ADT_XDRERR);
						if (dcd_mem((char *)&(tsp->ts_quantum), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
							return(ADT_XDRERR);
						if (dcd_mem((char *)&(tsp->ts_tqexp), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
							return(ADT_XDRERR);
						if (dcd_mem((char *)&(tsp->ts_slpret), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
							return(ADT_XDRERR);
						if (dcd_mem((char *)&(tsp->ts_maxwait), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
							return(ADT_XDRERR);
						if (dcd_mem((char *)&(tsp->ts_lwait), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
							return(ADT_XDRERR);
					}
					break;
				/* ADT_SCHED_RT and ADT_SCHED_FP are the same value */
				case ADT_SCHED_RT:
					if ((dumpp->freeformat = (char *)malloc(adminp->nentries *
					    sizeof(struct rtdpent))) == NULL) {
        					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                				return(ADT_MALLOC);
					}
					/* Calculate size of free-format (sizeof(struct) round to word boundary) */
					ff_toword = adminp->nentries * sizeof(struct rtdpent);
					rtp = (struct rtdpent *)dumpp->freeformat;
					for (x = 0; x < adminp->nentries; x++, rtp++) {
						if (dcd_mem((char *)&(rtp->rt_globpri), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
							return(ADT_XDRERR);
						if (dcd_mem((char *)&(rtp->rt_quantum), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
							return(ADT_XDRERR);
					}
					break;
				case ADT_SCHED_FC:
					if ((dumpp->freeformat = (char *)malloc(adminp->nentries *
					    sizeof(struct fcdpent))) == NULL) {
        					(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                				return(ADT_MALLOC);
					}
					/* Calculate size of free-format (sizeof(struct) round to word boundary) */
					ff_toword = adminp->nentries * sizeof(struct fcdpent);
					fcp = (struct fcdpent *)dumpp->freeformat;
					for (x = 0; x < adminp->nentries; x++, fcp++) {
						if (dcd_mem((char *)&(fcp->fc_globpri), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
							return(ADT_XDRERR);
						if (dcd_mem((char *)&(fcp->fc_quantum), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
							return(ADT_XDRERR);
					}
					break;
				}
			}
			break;
		case AEVT_R:
			if (aevtp->a_nlvls > 0) {
				if ((dumpp->freeformat = (char *)calloc(aevtp->a_nlvls, sizeof(level_t))) == NULL) {
        				(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                			return(ADT_MALLOC);
				}
				lvlp = (level_t *)dumpp->freeformat;
				for (x = 0; x < aevtp->a_nlvls; x++, lvlp++) {
					if (dcd_mem((char *)lvlp, 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
						return(ADT_XDRERR);
				}
				/* Calculate size of free-format */
				ff_toword = aevtp->a_nlvls * sizeof(level_t);
			}
			break;
		case CRFREE_R :
                        if (credfp->cr_ncrseqnum > 0) {
                         	if ((dumpp->freeformat = (char *)calloc(credfp->cr_ncrseqnum,
				    sizeof(ulong_t))) == NULL) {
                                 	(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                                        return(ADT_MALLOC);
                                }
                                crnump = (ulong_t *)dumpp->freeformat;
                                for (x = 0; x < credfp->cr_ncrseqnum; x++, crnump++) {
                                 	if (dcd_mem((char *)crnump, 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
                                                return(ADT_XDRERR);
                                }
				/* Calculate size of free-format */
				ff_toword = credfp->cr_ncrseqnum * sizeof(ulong_t);
                        }
			break;
		case FPRIV_R:
			if (fprivp->f_count > 0) {
				if ((dumpp->freeformat = (char *)calloc(fprivp->f_count, sizeof(unsigned long))) == NULL) {
        				(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                			return(ADT_MALLOC);
				}
				ulongp = (unsigned long *)dumpp->freeformat;
				for (x = 0; x < fprivp->f_count; x++, ulongp++) {
					if (dcd_mem((char *)ulongp, 0, xdr_rsize, &bufp, ULONG) == ADT_XDRERR)
						return(ADT_XDRERR);
				}
				/* Calculate size of free-format data */
				ff_toword = fprivp->f_count * sizeof(unsigned long);
			}
			break;
		case IPCACL_R :
			if (ipcaclp->i_nentries > 0) {
				if ((dumpp->freeformat = (char *)malloc(ipcaclp->i_nentries * sizeof(tacl_t))) == NULL) {
        				(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                			return(ADT_MALLOC);
				}
				acl_structp = (tacl_t *)dumpp->freeformat;
				for (x = 0; x < ipcaclp->i_nentries; x++, acl_structp++) {
					if (dcd_mem((char *)&(acl_structp->ttype), 0, xdr_rsize, &bufp, INT) == ADT_XDRERR)
						return(ADT_XDRERR);
					if (dcd_mem((char *)&(acl_structp->tid), 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
						return(ADT_XDRERR);
					if (dcd_mem((char *)&(acl_structp->tperm), 0, xdr_rsize, &bufp, SHORT) == ADT_XDRERR)
						return(ADT_XDRERR);
				}
				/* Calculate size of free-format (sizeof(struct) round to word boundary) */
				ff_toword = ipcaclp->i_nentries * sizeof(tacl_t);
			}
			break;
		case KEYCTL_R :
                        if (keyp->k_nskeys > 0) {
                         	if ((dumpp->freeformat = (char *)malloc(keyp->k_nskeys * sizeof(struct k_skey))) == NULL) {
                                 	(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                                        return(ADT_MALLOC);
                                }
                        	skeyp = (struct k_skey *)dumpp->freeformat;
                                for (x = 0; x < keyp->k_nskeys; x++, skeyp++) {
					/*
					 * sernum and serkey may not be NULL terminated in
					 * the audit trail, but they are when stored in XDR
					 * format. Decode them into temp buffers then copy
					 * them to the skeyp structure, excluding the last
					 * byte (which is always a NULL).
					 */
                        		(void)memcpy(sernum, '\0', SNLEN+1);
                        		(void)memcpy(serkey, '\0', SKLEN+1);
					if (dcd_mem(sernum, sizeof(skeyp->sernum), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
						return(ADT_XDRERR);
					if (dcd_mem(serkey, sizeof(skeyp->serkey), xdr_rsize, &bufp, STRING) == ADT_XDRERR)
						return(ADT_XDRERR);
                        		(void)memcpy(skeyp->sernum, sernum, SNLEN);
                        		(void)memcpy(skeyp->serkey, serkey, SKLEN);
                                }
                                /* Calculate size of free-format */
                                ff_toword = keyp->k_nskeys * sizeof(struct k_skey);
			}
			break;
		case KILL_R :
                        if (killp->k_entries > 0) {
                         	if ((dumpp->freeformat = (char *)calloc(killp->k_entries, sizeof(pid_t))) == NULL) {
                                 	(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                                        return(ADT_MALLOC);
                                }
                                pidp = (pid_t *)dumpp->freeformat;
                                for (x = 0; x < killp->k_entries; x++, pidp++) {
                                 	if (dcd_mem((char *)pidp, 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
                                                return(ADT_XDRERR);
                                }
                                /* Calculate size of free-format */
                                ff_toword = killp->k_entries * sizeof(pid_t);
                        }
			break;
		case PARMS_R:
                        if (parmsp->nentries > 0) {
                         	if ((dumpp->freeformat = (char *)calloc(parmsp->nentries, sizeof(lwpid_t))) == NULL) {
                                 	(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                                        return(ADT_MALLOC);
                                }
                                lwpidp = (lwpid_t *)dumpp->freeformat;
                                for (x = 0; x < killp->k_entries; x++, lwpidp++)
                                {
                                 	if (dcd_mem((char *)lwpidp, 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
                                                return(ADT_XDRERR);
                                }
                                /* Calculate size of free-format */
                                ff_toword = parmsp->nentries * sizeof(lwpid_t);

			}
			break;
		case SETGRPS_R:
			if (setgroupp->s_ngroups > 0) {
				if ((dumpp->freeformat = (char *)calloc(setgroupp->s_ngroups, sizeof(gid_t))) == NULL) {
        				(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                			return(ADT_MALLOC);
				}
				sgrpp = (gid_t *)dumpp->freeformat;
				for (x = 0; x < setgroupp->s_ngroups; x++, sgrpp++) {
					if (dcd_mem((char *)sgrpp, 0, xdr_rsize, &bufp, LONG) == ADT_XDRERR)
						return(ADT_XDRERR);
				}
				/* Calculate size of free-format */
				ff_toword = setgroupp->s_ngroups * sizeof(gid_t);
			}
			break;
		default:
			if (bufp < (origp + xdr_rsize)) {
				/* Calculate size of freeformat data and XDR padding */
				len = (origp + xdr_rsize) - bufp; 

				/* Create area for converted freeformat data */
				if ((dumpp->freeformat = (char *)calloc(len, sizeof(char))) == NULL) {
        				(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
                			return(ADT_MALLOC);
				}
				if (dcd_mem(dumpp->freeformat, len * (int)sizeof(char), xdr_rsize, &bufp, BYTES) == ADT_XDRERR)
					return(ADT_XDRERR);

				ff_toword = ROUND2WORD(len);
			}
	}
	
	/* Final record size in MDF bytes. */
	spec_size = getspec(cmnp->c_rtype);
	if (dumpp->cmn.c_rtype == CRFREE_R){
		/*
	 	 * Size is not stored in the CRFREE_R record.
		 * This is a null statement.
	 	 */
		;
	} else {
		if (dumpp->cmn.c_rtype == FNAME_R){
			/*
	 	 	 * The fname record has no common.
	 	 	 * specific record + freeformat rounded to next word
	 	 	 */
			fnamep->f_size = spec_size + ff_toword;
		} else {
			/*
		 	 * common record + specific record + freeformat rounded to next word
		 	 * Note: the size of the cred record and the groups are not
		 	 * included in c_size.
		 	 */
			cmnp->c_size = sizeof(cmnrec_t) + spec_size + ff_toword;
		}
	}

	/* Write the translated audit record to stdout */
	if (wr_mdf(dumpp, spec_size, ff_toword) != 0)
		return(ADT_FMERR);

	free(origp);
	free(dumpp->freeformat);

	return(0);
}

/*
 * Procedure:	  dcd_mem
 * Privileges:	  none
 * Restrictions:  none
 * Notes:	  Each XDR "audit record" field and a constant defining its type 
 *                is passed in.
 *		  The audit record field is then translated to MDF and stored in a
 *		  temporary dumprec_t structure.
 */
int
dcd_mem(memp, len, xdr_rsize, bufp, conv_type)
char *memp;
int len;
int xdr_rsize;
char **bufp;
short conv_type;
{
	XDR xdr;
	register XDR *xdrp;
	int *intp;
	unsigned int *uintp;
	long *longp;
	unsigned long *ulongp;
	short *shortp;
	unsigned short *ushortp;
	enum_t *enump;
	u_int length;

	xdrp = &xdr;
	xdrmem_create(xdrp, *bufp, xdr_rsize, XDR_DECODE);

	switch (conv_type) {
		case SHORT:
			shortp = (short *)memp;
			if (xdr_short(xdrp, shortp) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
			*bufp = xdrp->x_private;
			break;
		case USHORT:
			ushortp = (unsigned short *)memp;
			if (xdr_u_short(xdrp, ushortp) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
			*bufp = xdrp->x_private;
			break;
		case INT:
			intp = (int *)memp;
			if (xdr_int(xdrp, intp) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
			*bufp = xdrp->x_private;
			break;
		case UINT:
			uintp = (unsigned int *)memp;
			if (xdr_u_int(xdrp, uintp) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
			*bufp = xdrp->x_private;
			break;
		case LONG:
			longp = (long *)memp;
			if (xdr_long(xdrp, longp) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
			*bufp = xdrp->x_private;
			break;
		case ULONG:
			ulongp = (unsigned long *)memp;
			if (xdr_u_long(xdrp, ulongp) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
			*bufp = xdrp->x_private;
			break;
		case ENUM:
			enump = (enum_t *)memp;
			if (xdr_enum(xdrp, enump) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
			*bufp = xdrp->x_private;
			break;
		case STRING:
			if (xdr_string(xdrp, &memp, len) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
			*bufp = xdrp->x_private;
			break;
		case BYTES:
			length = len;
			if (xdr_bytes(xdrp, &memp, &length, len) == -1) {
                                (void)pfmt(stderr, MM_ERROR, FDR5);
                                return(ADT_XDRERR);
			}
		default:
			break;
	}/* end switch */
	return(0);
}
/*
 * Procedure:	  wr_mdf
 * Privileges:	  none
 * Restrictions:  none
 * Notes:	  Write each audit record to stdout
 */
int
wr_mdf(dumpp, spec_size, ff_toword)
struct rec *dumpp;
int spec_size; /* size of specific data */
int ff_toword; /* size of free-format data */
{

	/*
	 * Write the common record, unless this is a record 
	 * without a common record (filename or credential_free).
	 */
	if ((dumpp->cmn.c_rtype != FNAME_R) && (dumpp->cmn.c_rtype != CRFREE_R)){
		if (write(1, &(dumpp->cmn), sizeof(cmnrec_t)) == -1) {
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
	}

	/* Write the specific record */
	if (spec_size != 0) {
		if (write(1, &(dumpp->d_spec), spec_size) == -1) {
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
	}

	/* If present, write the free format data */
	if (ff_toword != 0) {
		if (write(1, dumpp->freeformat, ff_toword) == -1) {
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}

	}

	/* If present, write the cred record */
	if ((dumpp->cmn.c_rtype != FNAME_R) && (dumpp->cmn.c_rtype != CRFREE_R) &&\
	    (dumpp->cmn.c_crseqnum == 0)){
		if (write(1, &(dumpp->cred), sizeof(credrec_t)) == -1) {
			(void)pfmt(stderr, MM_ERROR, E_FMERR);
			return(ADT_FMERR);
		}
		/* If present, write the groups */
		if (dumpp->cred.cr_ngroups > 0){
			if (write(1, dumpp->groups, (sizeof(gid_t) * dumpp->cred.cr_ngroups)) == -1) {
				(void)pfmt(stderr, MM_ERROR, E_FMERR);
				return(ADT_FMERR);
			}
		}
	}

	return(0);
}
