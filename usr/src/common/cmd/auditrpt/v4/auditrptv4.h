/*		copyright	"%c%" 	*/

#ident	"@(#)auditrptv4.h	1.2"
#ident  "$Header$"

#include "../auditrpt.h" 

#define GOTNEWMV -2	/* a new magic number and version number have been read */
#define CRED_FOUND 1	/* credential information for for the record has been found */

/* To differentiate IPC types */
#define	IPC_SHM		0x01		/* shared memory */
#define	IPC_SEM		0x02		/* semaphores */
#define	IPC_MSG		0x04		/* message queues */

/*
 * one save structure is linked to a list for each filename record
 */
typedef struct save {
	int			seqnum;
	struct fname_r 		rec;
	char 			*name;
	struct save		*next;
} save_t; 

/*
 * for each record to be displayed, a dumprec_t structure is filled before
 * calling the printing routines.
 */
typedef struct rec {
	cmnrec_t	cmn;			/* common data in all records */
	union {
		char		spec;
		struct	abuf_r	r_abuf;
		struct	acl_r	r_acl;
		struct	actl_r  r_actl;
		struct	admin_r r_admin;
		struct	admp_r	r_admp;
		struct	aevt_r  r_aevt;
		struct	alog_r  r_log;
		struct	bind_r  r_bind;
#ifdef CC_PARTIAL
		struct	cc_r	r_cc;
#endif
		struct	chmod_r	r_chmod;
		struct	chown_r	r_chown;
		struct	credf_rec r_credf;
		struct	cron_r	r_cron;
		struct	dev_r	r_dev;
		struct	fchmod_r r_fchmod;
		struct	fchown_r r_fchown;
		struct	fcntl_r r_fcntl;
		struct	fcntlk_r r_fcntlk;
		struct	fd_r 	r_fd;
		struct	fdev_r 	r_fdev;
		struct	fmac_r	r_fmac;
		struct	fname_r r_fname;
		struct	fork_r	r_fork;
		struct	fpriv_r	r_fpriv;
		struct	id_r	r_id;
		struct	ioctl_r	r_ioctl;
		struct	ipc_r	r_ipc;
		struct	ipcacl_r r_ipcacl;
		struct	kill_r	r_kill;
		struct	login_r	r_login;
		struct	lwpcreat_r r_lwpcreat;
		struct	mac_r	r_mac;
		struct	mctl_r	r_mctl;
		struct	modadm_r r_modadm;
		struct	modload_r r_modload;
		struct	mount_r	r_mount;
		struct	online_r r_online;
		struct	parms_r	r_parms;
		struct	passwd_r r_passwd;
		struct	pipe_r	r_pipe;
		struct	plock_r	r_plock;
		struct	recvfd_r r_recvfd;
		struct	rlim_r	r_rlim;
		struct	setgroup_r r_setgroup;
		struct	setid_r	r_setid;
		struct	setpgrp_r r_setpgrp;
		struct	time_r	r_time;
		struct	ulimit_r	r_ulimit;
	}spec_data;				/* specific to record type */
	char		*freeformat;		/* variable data */
	struct	cred_rec	cred;			/* credential information */
	gid_t 		groups[NGROUPS_MAX];	/* multiple grps */
}dumprec_t;

/* list entry for records that do not have cred information */
typedef struct need_cred {
	struct need_cred *next;		/* points to next entry */
	dumprec_t *dumpp;		/* pointer to a dumprec structure */
} needcred_t;

/* list entry for records that do not have all fname records */
typedef struct need_fname {
	struct need_fname *next;	/* points to next entry */
	int seqnum;			/* seq num to match fname records */
	dumprec_t *dumpp;		/* pointer to a dumprec structure */
}needfname_t;

extern char adt_ver[ADT_VERLEN];		/* logfile version number */
extern char log_byord[ADT_BYORDLEN + 1];	/* logfile magic number (byte order) */
