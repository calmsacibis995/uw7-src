/*		copyright	"%c%" 	*/

#ident	"@(#)auditrptv1.h	1.2"
#ident  "$Header$"

#include "../auditrpt.h" 

#define GOTNEWMV -2	/* a new magic number and version number have been read */

/*
 * one save structure is linked to a list for each filename record
 */
typedef struct save {
	char		*name;
	int		seqnum;
	int		pid;
	struct fname_r	rec;
	struct save	*next;
} save_t; 

/*
 * for each record to be displayed, a dumprec_t structure is filled before
 * calling the printing routines.
 */
typedef struct rec {
	cmnrec_t	cmn;			/* common data in all records */
	union {
		char		spec;
		struct	abuf_r  r_abuf;
		struct	acl_r	r_acl;
		struct	actl_r  r_actl;
		struct	admin_r	r_admin;
		struct	admp_r	r_admp;
		struct	aevt_r  r_aevt;
		struct	alog_r  r_log;
		struct	cc_r	r_cc;
		struct	chmod_r	r_chmod;
		struct	chown_r	r_chown;
		struct	cron_r	r_cron;
		struct	dev_r	r_dev;
		struct	fchdir_r r_fchdir;
		struct	fchmod_r r_fchmod;
		struct	fchown_r r_fchown;
		struct	fcntl_r r_fcntl;
		struct	fcntlk_r r_fcntlk;
		struct	fdev_r	r_fdev;
		struct	file_r	r_file;
		struct	fmac_r	r_fmac;
		struct	fname_r	r_fname;
		struct	fork_r	r_fork;
		struct	fpriv_r	r_fpriv;
		struct	fstat_r	r_fstat;
		struct	groups_r r_groups;
		struct	id_r	r_id;
		struct	ioctl_r	r_ioctl;
		struct	ipc_r	r_ipc;
		struct	kill_r	r_kill;
		struct	login_r	r_login;
		struct	mac_r	r_mac;
		struct	mctl_r	r_mctl;
		struct	modadm_r r_modadm;
		struct	modld_r r_modld;
		struct	mount_r	r_mount;
		struct	parms_r	r_parms;
		struct	passwd_r r_passwd;
		struct	pipe_r	r_pipe;
		struct	plock_r	r_plock;
		struct	priv_r	r_priv;
		struct	proc_r	r_proc;
		struct	recvfd_r r_recvfd;
		struct	rlim_r 	r_rlim;
		struct	setgroup_r r_setgroup;
		struct	setid_r	r_setid;
		struct	setpgrp_r r_setpgrp;
		struct	sys_r	r_sys;
		struct	time_r	r_time;
	}spec_data;				/* specific to record type */
	trailrec_t	trail;	
	char		*freeformat;		/* variable data */
	long		ngroups;		/* number of groups */
	gid_t 		groups[NGROUPS_MAX];	/* multiple grps */
	struct	proc_r	proc;			/* process information */
}dumprec_t;

#define r_seqnm 	cmn.c_seqnm
