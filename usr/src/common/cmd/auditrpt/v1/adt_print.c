#ident	"@(#)adt_print.c	1.6"
#ident  "$Header$"

/* This define is required for covert channel information to be included. */
#define CC_PARTIAL 1

/* LINTLIBRARY */
#include <stdio.h>
#include <errno.h>
#include <sys/vnode.h>
#include <acl.h>
#include <sys/param.h>
#include <time.h>
#include "audit.h"
#include <sys/privilege.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <mac.h>
#include <sys/lock.h>
#include <sys/mount.h>
#include <pfmt.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include "auditrec.h"
#include <sys/covert.h>
#include <sys/mkdev.h>
#include <sys/mod.h>
#include "auditrptv1.h"

/**
 ** external variables - defined in auditrptv1.c
 **/
extern 	int 	s_user,
		s_grp,
		s_class,
		s_type,
		s_priv,
		s_scall;

extern	ids_t 	*uidbegin, 
		*gidbegin,
		*typebegin, 
		*privbegin,
		*scallbegin;

extern	cls_t	*clsbegin;

extern 	save_t 	*fn_beginp, 
		*fn_lastp;

extern	int	mac_installed;

extern 	env_info_t	env;

extern int free_size;

/** 
 ** external variables - defined in adt_loadmap.c
 **/
extern	setdef_t	*setdef;	/* privilege mechanism info */
extern	nsets;				/* number of privilege sets */

/**
 ** external functions
 **/
extern	int	adt_lvlout();		/* from adt_lvlout.c */

/**
 ** local functions
 **/
static char	*id_conv(), *cc_getname();

static void 	pr_cmn(),
		pr_proc(),
		pr_priv(),
		pr_log(),
		pr_dev(),
		pr_lvl(),
		pr_acl(),
		pr_mode(),
		pr_otype(),
		pr_own(),
		pr_ipcobj(),
		pr_cron(),
		pr_amsk(),
		pr_passwd(),
		pr_aevt(),
		pr_alog(),
		pr_plock(),
		pr_recvfd(), 
		pr_cc(), 
		pr_mount(), 
		pr_fpriv(), 
		pr_setpgrp(),
		pr_setgrps(), 
		pr_ioctl(), 
		pr_fcntl(), 
		pr_fcntlk(), 
		pr_admin(),
		pr_parms(), 
		pr_admp(),
		pr_setid(),
		pr_rlim(),
		pr_mctl(),
		pr_modadm(),
		pr_modld(),
		pr_modpath(),
		pr_user(),
		pr_kill();

static int  	pr_obj(), 
		created_vn();

static struct tm *actual_time();

static void	adt_itoa();

static void	fn_free();


#ifdef CC_PARTIAL
static struct cc_names cc_names[] = { CC_NAMES };
#endif

#define SUCCESS	's'
#define FAIL   	'f'

#define adt_privbit(p)		(priv_t)(1 << (p))

/**
 ** Display the post-selected audit record pointed by rec.
 ** The output is written to the file pointed by fp.
 ** The output line per record looks like:
 **
 ** time,event,pid,outcome,user,group(s),session,"subj_lvl",(obj_id:obj_type:"obj_lvl":
 ** device:maj:min:inode:fsid)(...)[,pgm_prm]
 ** 
 ** The groups field is displayed as real:effective:suplementary[:...].
 ** There might be more than one object, in which case the additional object 
 ** information will follow the previous one enclosed in additional parentheses, i.e.
 **	(obj_id:obj_type:"obj_lvl":device:maj:min:inode:fsid)(obj_id:obj_type:...)(...)
 ** The last field (pgm_prm) is specific to each event type.
 ** All commas, except possibly the last one will be displayed even if the
 ** field is null (as place holders).
 **/
void
printrec(fp, rec, found)
FILE *fp;
char *rec;
int   found;	/* process information found? */
{
	dumprec_t	*dump_rec;
	struct proc_r	*proc;
	cmnrec_t	*cmn;
	char		*spec;
	char 		timeb[50];
	struct abuf_r	*abufp;
	struct acl_r	*aclp;
	struct actl_r	*actlp;
	struct chmod_r	*cmp;
	struct chown_r	*cop;
	struct fchdir_r	*fcdp;
	struct fchmod_r	*fcmp;
	struct fchown_r	*fcop;
	struct fdev_r	*fdevp;
	struct file_r	*filep;
	struct fmac_r	*fmacp;
	struct fpriv_r	*fprivp;
	struct fork_r	*forkp;
	struct fstat_r	*fstp;
	struct ipc_r	*ipcp;
	struct mac_r	*macp;
	struct modld_r	*modldp;
	struct time_r	*timep;
	struct setgroup_r *setgrpsp;
	struct pipe_r 	*pipep;
	struct sys_r 	*sysp;

	dump_rec = (dumprec_t *)rec;
	proc = &dump_rec->proc;
	spec = &dump_rec->d_spec;
	cmn =  &dump_rec->cmn;
	(void)fprintf(fp,"\n");
	/* display common part */
	pr_cmn(fp, cmn);
	/* display process information */
	if (found)
		pr_proc(fp, proc, dump_rec);
	else
		(void)fprintf(fp,",,,,");
	/* 
	 * display object information:  (obj_id:obj_type:obj_level:device:major:minor:inode:fsid)
	 * for all the objects related to this event
	 */
	if (has_path(cmn->c_rtype,cmn->c_event)){
		if (pr_obj(fp,cmn->c_seqnm,cmn->c_event,cmn->c_status) == -1)
				(void)pfmt(stdout,MM_WARNING,W_MISS_PATH,cmn->c_pid);
	}else {
		if (cmn->c_rtype == IPCR_R)
			pr_ipcobj(fp,(struct ipc_r *)spec,proc,cmn->c_event);
		else
                    	/*Display object information fields for the moduload event*/
                        /*to be consistant with IPC events and the modload event.*/
			if (cmn->c_event == ADT_MODULOAD) {
				modldp=(struct modld_r *)spec;
				(void)fprintf(fp,"(%d:%s::::::)",modldp->id,MODULE);
			}
	}

	/* display "pgm_prm" part of output - specific for each event */
	switch(cmn->c_rtype)
	{
	case PRIV_R :
		/* display privilege info for pm_denied events */
		(void)fprintf(fp,",");
		pr_priv(fp,(struct priv_r *)spec);
		break;
	case LOGIN_R :
		pr_log(fp,(alogrec_t *)spec,cmn->c_event);
		break;
	case PASSWD_R:
		pr_passwd(fp,(apasrec_t *)spec);
		break;
	case CC_R:
		pr_cc(fp,(struct cc_r *)spec);
		break;
	case CRON_R:
		pr_cron(fp,(acronrec_t *)spec);
		break;
	case TIME_R:
		/* display new date for date records */
		timep=(struct time_r *)spec;
		(void)cftime(timeb,"%a %b %d %T %Z %Y",&(timep->t_time));
		(void)fprintf(fp,",%s", timeb);
		break;
	case DEV_R:
		(void)fprintf(fp,",");
		pr_dev(fp,(struct dev_r *)spec,cmn->c_event);
		break;
	case MAC_R:
		macp=(struct mac_r *)spec;
		(void)fprintf(fp,",");
		pr_lvl(fp,macp->l_lid);
		break;
	case CHOWN_R:
		cop=(struct chown_r *)spec;
		pr_own(fp,cop->c_uid,cop->c_gid);
		break;
	case CHMOD_R:
		cmp=(struct chmod_r *)spec;
		pr_mode(fp,cmp->c_nmode);
		break;
	case FCHDIR_R:
		fcdp=(struct fchdir_r *)spec;
		(void)fprintf(fp,",%d",fcdp->f_fd);
		break;
	case FCHMOD_R:
		fcmp=(struct fchmod_r *)spec;
		pr_mode(fp,fcmp->f_chmod.c_nmode);
		(void)fprintf(fp,",%d",fcmp->f_fd);
		break;
	case FCHOWN_R:
		fcop=(struct fchown_r *)spec;
		pr_own(fp,fcop->f_chown.c_uid,fcop->f_chown.c_gid);
		(void)fprintf(fp,",%d",fcop->f_fd);
		break;
	case FDEV_R:
		(void)fprintf(fp,",");
		pr_dev(fp,(struct dev_r *)spec,cmn->c_event);
		fdevp=(struct fdev_r *)spec;
		(void)fprintf(fp,",%d",fdevp->l_fd);
		break;
	case FMAC_R:
		fmacp=(struct fmac_r *)spec;
		(void)fprintf(fp,",");
		pr_lvl(fp,fmacp->l_mac.l_lid);
		(void)fprintf(fp,",%d",fmacp->l_fd);
		break;
	case FSTAT_R:
		fstp=(struct fstat_r *)spec;
		(void)fprintf(fp,",%d",fstp->s_fd);
		break;
	case MOUNT_R:
		pr_mount(fp,(struct mount_r *)spec);
		break;
	case FPRIV_R:
		fprivp=(struct fpriv_r *)spec;
		if ((dump_rec->freeformat) != NULL)
	  		pr_fpriv(fp,(priv_t *)dump_rec->freeformat,fprivp->f_count);
		else
			(void)fprintf(fp,",?");
		break;
	case ACL_R:
		aclp=(struct acl_r *)spec;
		if (cmn->c_event == ADT_IPC_ACL)
			(void)fprintf(fp,",%d,%d", aclp->a_ityp,aclp->a_iid); 
	  	pr_acl(fp,dump_rec->freeformat,aclp->a_nentries);
		break;
 	case ABUF_R:
		/* display high water mark for auditbuf system call */
 		abufp=(struct abuf_r  *)spec;
 		(void)fprintf(fp,",%d",abufp->a_vhigh);
 		break;
 	case ACTL_R:
		/* display name of action requested in auditctl system call */
 		actlp=(struct actl_r *)spec;
		switch (actlp->a_cmd){
 		case AUDITON:
 			(void)fprintf(fp,",AUDITON");
			break;
 		case AUDITOFF:
 			(void)fprintf(fp,",AUDITOFF");
			break;
		default:	/* should never reach */
 			(void)fprintf(fp,"?,");
			break;
		}
 		break;
	case ADMIN_R:
		pr_admin(fp,(struct admin_r *)spec);
		break;
	case ADMP_R:
		pr_admp(fp,(struct admp_r *)spec);
		break;
 	case AEVT_R:
 		pr_aevt(fp,(struct aevt_r *)spec,dump_rec);
 		break;
 	case ALOG_R:
 		pr_alog(fp,(struct alog_r *)spec);
 		break;
	case PLOCK_R:
		pr_plock(fp,(struct plock_r *)spec);
		break;
	case RECVFD_R:
		pr_recvfd(fp,(struct recvfd_r *)spec);
		break;
	case FILE_R:
		filep=(struct file_r *)spec;
		if (filep->f_fd != -1)
			(void)fprintf(fp,",%d",filep->f_fd);/*file descriptor*/
		break;
	case FORK_R:
		forkp=(struct fork_r *)spec;
		(void)fprintf(fp,",P%ld",forkp->f_cpid);	/*child's pid*/
		break;
	case IOCTL_R:
		pr_ioctl(fp,(struct ioctl_r *)spec);
		break;
	case FCNTL_R:
		pr_fcntl(fp,(struct fcntl_r *)spec);
		break;
	case FCNTLK_R:
		pr_fcntlk(fp,(struct fcntlk_r *)spec);
		break;
	case IPCR_R:
		ipcp=(struct ipc_r *)spec;
		(void)fprintf(fp,",%d,%o,%d",ipcp->i_op,ipcp->i_flag,ipcp->i_cmd);
		break;
	case KILL_R:
		pr_kill(fp,(struct kill_r *)spec, (pid_t *)dump_rec->freeformat);
		break;
	case SETPGRP_R:
		pr_setpgrp(fp,(struct setpgrp_r *)spec);
		break;
	case SETUID_R:
		pr_setid(fp,(struct setid_r *)spec);
		break;	
	case SETGRPS_R:
		setgrpsp=(struct setgroup_r *)spec;
		pr_setgrps(fp,setgrpsp->s_ngroups,(gid_t *)dump_rec->freeformat);
		break;
	case PARMS_R:
		pr_parms(fp,(struct parms_r *)spec);
		break;
	case PIPE_R:
		pipep=(struct pipe_r *)spec;
		(void)fprintf(fp,",%d,%d",pipep->p_fd0,pipep->p_fd1);
		break;
	case SYS_R:
		sysp=(struct sys_r *)spec;
		/*the new limit*/
		(void)fprintf(fp,",%ld",sysp->s_arg);
		break;
	case RLIM_R:
		pr_rlim(fp,(struct rlim_r *)spec);
		break;
	case MCTL_R:
		pr_mctl(fp,(struct mctl_r *)spec);
		break;
	case MODADM_R:
		pr_modadm(fp,(struct modadm_r *)spec,dump_rec->freeformat);
		break;
	case MODPATH_R:
		pr_modpath(fp,cmn->c_status,dump_rec->freeformat);
		break;
	case MODLD_R:
		pr_modld(fp,(struct modld_r *)spec,cmn->c_event,cmn->c_status,dump_rec->freeformat);
		break;
	case USER_R:
		if (dump_rec->freeformat != NULL)
			pr_user(fp,dump_rec->freeformat);
		break;
	} /* end of switch */
	(void)fprintf(fp, "\n");
	(void)fflush(stdout);
} /* end of printrec */

/**
 ** Convert id to name using the audit map
 **/
char *
id_conv(id, s_tbl, tblp)
int id;
int s_tbl;
ids_t *tblp;
{
	char	 *namep;
	int 	 j;
   
	for (j=0; j < s_tbl; j++, tblp++) {
		if (id==tblp->id){
			namep=tblp->name;
			return(namep);
		}
	}
	return((char *)NULL);
}

/**
 ** Display time, event type, process id and outcome (common part of record). 
 **/
void
pr_cmn(fp, cmnp)
FILE	*fp;
cmnrec_t *cmnp;
{
	struct tm *tm;
	char 	*typename=NULL;

	/* display time */
	tm=actual_time(&(cmnp->c_time));
	(void)fprintf(fp, "%.2d:%.2d:%.2d:%.2d:%.2d:%.2d,",
		tm->tm_hour, tm->tm_min,tm->tm_sec,
		tm->tm_mday, tm->tm_mon+1, tm->tm_year);
	/* display event type */
	/* convert type number to type name if possible */
	typename=id_conv((int)cmnp->c_event,s_type,typebegin);
	if (typename != NULL)
		(void)fprintf(fp,"%s,",typename);
	else
		(void)fprintf(fp,"%d,",cmnp->c_event);
	/* display process id */
	(void)fprintf(fp,"P%ld,",cmnp->c_pid);
	/* display outcome */
	if (cmnp->c_status == 0)
		(void)fprintf(fp,"%c,",SUCCESS);
	else 
		(void)fprintf(fp,"%c(%ld),",FAIL,cmnp->c_status);
}

/**
 ** Display user, group(s), session, and subject level information from the
 ** process part of the dump record.
 **/
void
pr_proc(fp, proc, dump_rec)
FILE *fp;
struct proc_r *proc;
dumprec_t	*dump_rec;
{
	char	*rlogname=NULL;
	char	*logname=NULL;
	char	*rgrpname=NULL;
	char	*grpname=NULL;
	char	*sgrpname=NULL;
	int 	i;

	/* display user and group */
	/* convert uid to logname, and gid to group name */
	/* if name is not found, print the id */
	if ((rlogname=id_conv((int)proc->pr_ruid,s_user,uidbegin)) != NULL)
		(void)fprintf(fp,"%s:",rlogname);
	else
		(void)fprintf(fp,"%ld:",proc->pr_ruid);

	if ((logname=id_conv((int)proc->pr_uid,s_user,uidbegin))!= NULL)
		(void)fprintf(fp,"%s,",logname);
	else
		(void)fprintf(fp,"%ld,",proc->pr_uid);

	if ((rgrpname=id_conv((int)proc->pr_rgid,s_grp,gidbegin)) != NULL)
		(void)fprintf(fp,"%s:",rgrpname);
	else
		(void)fprintf(fp,"%ld:",proc->pr_rgid);

	if ((grpname=id_conv((int)proc->pr_gid,s_grp,gidbegin)) != NULL)
		(void)fprintf(fp,"%s",grpname);
	else
		(void)fprintf(fp,"%ld",proc->pr_gid);
	/* display supplementary groups */
	for (i=0; i<dump_rec->ngroups; ++i){
		sgrpname=id_conv((int)dump_rec->groups[i],s_grp,gidbegin);
		if (sgrpname != NULL)
			(void)fprintf(fp,":%s",sgrpname);
		else
			(void)fprintf(fp,":%ld",dump_rec->groups[i]);
	}
	(void)fprintf(fp,",");
	/* display session id */
	(void)fprintf(fp,"S%ld,",proc->pr_sid);
	/* display subject level */
	if (proc->pr_lid != 0)
		pr_lvl(fp, proc->pr_lid);
	(void)fprintf(fp,",");
}

/**
 ** Display object information for all events that generate filename record(s):
 ** 	-the name of regular file, special file, directory, named pipe.
 ** 	-object type:  f, c, b, d, p, l.
 ** 	-object level.  
 ** 	-device.
 ** 	-devmajor, devminor pair (translated from device).
 ** 	-inode and file system id.
 ** (ipc objects are displayed in pr_ipcobj())
 ** This routine also frees the save_t structures (saved info. of filename 
 ** records) as the information in the structures is used.
 */
int
pr_obj(fp, longseq, event, status)
FILE	*fp;
unsigned long	longseq;
ushort	event;
long	status;
{
	struct save *scan, *scan2;
	struct save *before = (struct save *)NULL;
	struct save *before2 = (struct save *)NULL;
	struct save *next = (struct save *)NULL;
	int 	file_found = 0;
	int	link_first = 1;
	int	sym_first = 1;
	int	found2;
	int	seqnum;		/* sequence number of the event */
	int	recnum;		/* num. of filename records dumped for this event */
	dev_t	devmajor, devminor;

	/* EXTRACTSEQ gives the sequence number of the event; all records for the
	 * same event have the same sequence number.
	 */
	seqnum=EXTRACTSEQ(longseq);
	/* EXTRACTREC gives the number of records dumped for the event == 
	 * number of filename records + 1 event record
	 */
	recnum=EXTRACTREC(longseq) - 1;

	for (scan=fn_beginp; (scan != NULL) && (file_found<recnum); scan=next){
		if (scan->seqnum == seqnum){
			if (event == ADT_SYM_CREATE && sym_first) {
				/* ignore first filename record for sym_create,
				 * this is created due to a lookup before the 
				 * link is created.
				 */
				sym_first = 0;
			} 
			else  if (event == ADT_LINK && !link_first) {
				/* print only the pathaname for the second filename
				 * record of link event.
				 */
				(void)fprintf(fp,",%s",scan->name);   /*link name*/
			}
			else {
				(void)fprintf(fp,"(");
				(void)fprintf(fp,"%s:",scan->name);	/*object name*/
				if ((status == 0) && created_vn(event)){
				/* for the events that create vnodes (vn_create),
				 * the first filename record has the correct name,
				 * and the second record has the rest of the object
				 * information.
				 */
					found2=0;
					scan2=scan;
					while ((!found2) && (scan2->next != NULL)){
						before2=scan2;
						scan2=scan2->next;
						if (scan2->seqnum == seqnum){
							if (before2 == scan)
								before2 = before;
							/* free the first record found */
							fn_free(&before, &scan, &next);
							scan=scan2;
							found2++;
							before=before2;
							file_found++;
						}
					}
				}
				pr_otype(fp, scan->rec.f_type);/*object type*/
				pr_lvl(fp,scan->rec.f_lid);	/*object level*/
				(void)fprintf(fp,":");
				if (scan->rec.f_dev != 0){	/* device */
					(void)fprintf(fp,"0x%lx:",scan->rec.f_dev);
					devmajor = major(scan->rec.f_dev);
					devminor = minor(scan->rec.f_dev);
					(void)fprintf(fp,"%ld:%ld:",devmajor,devminor);
				}
				else
					(void)fprintf(fp,"?:?:?:");
				if (scan->rec.f_inum != 0)	/* inode */
					(void)fprintf(fp,"%ld:",scan->rec.f_inum);
				else
					(void)fprintf(fp,"?:");
				if (scan->rec.f_fsid != 0)	/* file system id */
					(void)fprintf(fp,"0x%lx",scan->rec.f_fsid);
				else
					(void)fprintf(fp,"?");
				(void)fprintf(fp,")");
				if (event == ADT_LINK){
					link_first = 0;
				}
			}
			file_found++;
			/* free the element found (scan) */
			fn_free(&before,&scan,&next);
		}
		else {
			next = scan->next;
		}
		before = scan;
	}
	if (file_found < recnum)
		return(-1);
	return(0);
}

/**
 ** Display privilege information for pm_denied records:
 ** 	requested_priv,system_call,maximum_set_of_privs
 **/
void
pr_priv(fp, privp)
FILE 	*fp;
struct priv_r *privp;
{
	char 	*priname=NULL;
	char 	*scallnm=NULL;
	int	i, n, first;
	char	tbuf[BUFSIZE],
		*tbp = &tbuf[0];
	char	num[MAXNUM];
	
	/* requested privilege */
	if ((priname=id_conv(privp->p_req,s_priv,privbegin)) != NULL)
		(void)fprintf(fp,"%s,",priname); 
	else	 
		(void)fprintf(fp,"%d,",privp->p_req);
	/* system call */
	if ((scallnm=id_conv(privp->p_scall,s_scall,scallbegin)) != NULL)
		(void)fprintf(fp,"%s",scallnm);
	else
		(void)fprintf(fp,"%d",privp->p_scall);

	/* maximum set of privileges */
	(void)fprintf(fp,",");
	(void)memset(tbp,0,BUFSIZE);
	if (privp->p_max > 0) {
		first = 1;
		for (i=0,n=0; i<NPRIVS; i++) {
			if (adt_privbit(i) & privp->p_max) {
				/* print this privilege */
				n++;
				if (first==1){
					first=0;
				}
				else
					(void)strcat(tbp,":");
				if ((priname=id_conv(i,s_priv,privbegin)) != NULL)
					(void)strcat(tbp,priname); 
				else{
					(void)memset(num,0,MAXNUM);
					adt_itoa(i,num);
					(void)strcat(tbp,num);
				}
			}
		}
		if (n == NPRIVS)
			(void)fprintf(fp,"allprivs");
		else
			(void)fprintf(fp,tbp);
	}
}

/** 
 ** Display the specific information for the login records:  tty for all
 ** events; logname, group and  level of user attempting to log on;
 ** bad message for the bad_auth event; default level for the 
 ** def_lvl event.
 **/
void
pr_log(fp,logp,event)
FILE *fp;
alogrec_t *logp;
ushort event;
{
	char *name=NULL;
	(void)fprintf(fp,",%s",logp->tty);
	/* display logname, group and level of user that is
  	 * attempting to log on.
	 */
	if (logp->uid == -1)
		(void)fprintf(fp,",?");
	else{
		if ((name=id_conv((int)logp->uid,s_user,uidbegin))!= NULL)
                       	(void)fprintf(fp,",%s",name);
               	else
                       	(void)fprintf(fp,",%ld",logp->uid);
	}
	if (logp->gid == -1)
		(void)fprintf(fp,",?");
	else{
               	if ((name=id_conv((int)logp->gid,s_grp,gidbegin))!= NULL)
                       	(void)fprintf(fp,",%s",name);
                else
                	(void)fprintf(fp,",%ld",logp->gid);
	}
	/* display level at which user is login on */
	(void)fprintf(fp,",");
	if (logp->hlid != 0)
		pr_lvl(fp,logp->hlid);
	else
		pr_lvl(fp,logp->ulid);
	/* display user's old and new default for a def_lvl event */
	if (event == ADT_DEF_LVL){
		(void)fprintf(fp,",");
		pr_lvl(fp,logp->ulid);
		(void)fprintf(fp,",");
		pr_lvl(fp,logp->vlid);
	} else
	/* display bad_auth error message for a bad_auth event */
	if (event == ADT_BAD_AUTH)
		(void)fprintf(fp,",%s",logp->bamsg);
}

/**
 ** For the record type PASSWD_R, generated for event passwd,
 ** the uid of the user whose passwd is being changed is displayed
 **/
void
pr_passwd(fp,passwdp)
FILE *fp;
apasrec_t *passwdp;
{
	char *name=NULL;
	if ((passwdp->nuid) == -1)	/*invalid*/
		(void)fprintf(fp,",?");
	else{
		if ((name=id_conv((int)passwdp->nuid,s_user,uidbegin))!= NULL)
			(void)fprintf(fp,",%s",name);
		else
			(void)fprintf(fp,",%ld",passwdp->nuid);
	}
}

/** 
 ** For a cron record, display the user's effective uid and gid, 
 ** the MAC level of the user that cron was running in behalf of;
 ** and display the name of the cron job.
 **/
void
pr_cron(fp,cronp)
FILE *fp;
acronrec_t *cronp;
{
	char *name=NULL;
	if ((name=id_conv((int)cronp->uid,s_user,uidbegin))!= NULL)
		(void)fprintf(fp,",%s",name);
	else
		(void)fprintf(fp,",%ld",cronp->uid);
	if ((name=id_conv((int)cronp->gid,s_grp,gidbegin))!= NULL)
		(void)fprintf(fp,",%s",name);
	else
		(void)fprintf(fp,",%ld",cronp->gid);
	(void)fprintf(fp,",");
	pr_lvl(fp,cronp->lid);
	(void)fprintf(fp,",%s",cronp->cronjob);
}

/**
 ** Display the specific information for a covert channel event.
 **/
void
pr_cc(fp,ccp)
FILE	*fp;
struct cc_r *ccp;
{
	char *namep;

	if ((namep=cc_getname(ccp->cc_event))!= NULL)
		(void)fprintf(fp,",%s,%dbps",namep, ccp->cc_bps);
	else
		(void)fprintf(fp,",%d,%dbps",ccp->cc_event, ccp->cc_bps);
}

/**
 ** Display ACL based on user options.
 **
 **	input-     1. pointer to the buffer of ACL entries
 **		   2. number of entries in the buffer
 **/
void
pr_acl(fp, aclbufp, nentries)
FILE *fp;
char  	*aclbufp;
int	nentries;
{
	tacl_t 		*aclp;
	long 		i; 
	long 		entry_type; 
	char		*name;

	if (aclbufp){
		aclp = (tacl_t *)aclbufp;
		(void)fprintf(fp, ",");
		for (i = 0;  i < nentries; i++, aclp++) {
			entry_type = aclp->ttype;
			switch (entry_type) {
			case USER_OBJ:
				(void)fprintf(fp, "user::");
				break;
			case USER:
				(void)fprintf(fp, "user:");
				if ((name=id_conv((int)aclp->tid,s_user,uidbegin))!=NULL)
					(void)fprintf(fp, "%s:", name);
				else
					(void)fprintf(fp, "%0ld:",aclp->tid);
	
				break;
			case GROUP_OBJ:
				(void)fprintf(fp, "group::");
				break;
			case GROUP:
				(void)fprintf(fp, "group:");
				if ((name=id_conv((int)aclp->tid,s_grp,gidbegin))!=NULL)
					(void)fprintf(fp, "%s:", name);
				else
					(void)fprintf(fp, "%0ld:",aclp->tid);
				break;
			case CLASS_OBJ:
				(void)fprintf(fp, "class:");
				break;
			case OTHER_OBJ:
				(void)fprintf(fp, "other:");
				break;
			default:
				break;
			}	/* end switch */
		
			/* Now print permissions field */
			(void)fprintf(fp, "%c%c%c ", 
				aclp->tperm & AREAD ? 'r' : '-',
				aclp->tperm & AWRITE ? 'w' : '-',
				aclp->tperm & AEXEC ? 'x' : '-');
	
		}	/* end for */
	}
	else 
		(void)fprintf(fp, ",?");
}

/**
 ** Display list of privileges set in the filepriv event in the form
 ** type:privilege:privilge(:...),type:privilege(:...)(,...)
 **/
void
pr_fpriv(fp, pvec, cnt)
FILE *fp;
priv_t  pvec[];
int	cnt;
{
	register int	i, j, k,
			legend = 0, printed = 0, n = 0;
	char		tbuf[BUFSIZE],
			*tbp = &tbuf[0];
	setdef_t	*sd;
	char		num[MAXNUM];
	ushort		match;
	priv_t		temp_pvec;
	char		c;
	char		*priname;

	tbuf[0] = '\0';

	/*
	 * Display names of all recognized privilege sets followed by privilege names.
	 * Format:  type:privname:privname(:...),type:privname:privname(:...)(,...)
	 * "Recognized" sets are based on valid privilege sets in the privilege
	 * mechanism running on the audited machine.  This information is carried
	 * on the audit map.
	 */
	for (i=0, sd=setdef; i < nsets; ++i, ++sd) {
		for (k = 0; k < cnt; ++k) {
			for (j = 0; j < sd->sd_setcnt; ++j) {
				if ((sd->sd_mask | (j)) == pvec[k]) {
					++n;
					if (!legend) {
						(void)fprintf(fp,",%s:", sd->sd_name);
						legend = 1;
					}
					if (printed) {
						printed = 0;
						(void)strcat(tbp, ":");
					}
					if ((priname=id_conv(j,s_priv,privbegin)) !=NULL)
						(void)strcat(tbp, priname);
					else{
						(void)memset(num,'0',MAXNUM);
						adt_itoa(j, num);
						(void)strcat(tbp,num);
					}
					printed = 1;
				}
			}
		}
		if (legend) {
			if (n == sd->sd_setcnt) { 
				(void)fprintf(fp,"allprivs");
			}
			else {
				(void)fprintf(fp,"%s", tbp);
			}
		}
		n = legend = 0;
		tbuf[0] = '\0';
	}

	/*
	 * Display "raw" data for the privilege sets not recognized 
	 * in the privilege mechanism of the audited system.
	 * Format:  character:privilege(,character:privilege,...)
	 * If privilege cannot be translated to name, display number.
	 */
	for (k=0; k < cnt; ++k){
		match=0;
		for(i=0,sd=setdef; i<nsets; ++i,++sd){
			temp_pvec=pvec[k];
			if (((sd->sd_mask)>>24) == (temp_pvec>>24)) {
				match++;
				break;
			}
		}
		if (!match){
			temp_pvec=pvec[k];
			c = temp_pvec >> 24;
			(void)fprintf(fp,",%c",c);
			for (j = 0; j < NPRIVS; j++) {
				if (((c<<24) | (j)) == pvec[k]){
					if((priname=id_conv(j,s_priv,privbegin)) !=NULL)
						(void)fprintf(fp,":%s",priname);
					else
						(void)fprintf(fp,":%d",j);
				}
			}
		}
	}
}

/** 
 ** Display object type.
 **/
void
pr_otype(fp, ftype)
FILE *fp;
vtype_t  ftype;
{
	switch(ftype) {
		case 0:
		case 8:
			(void)fprintf(fp,"?:");
			break;
		case 1:
			(void)fprintf(fp,"f:"); 	/*reg file*/
			break;
		case 2:
			(void)fprintf(fp,"d:");	/*directory*/
			break;
		case 3:
			(void)fprintf(fp,"b:");	/*block special*/
			break;
		case 4:
			(void)fprintf(fp,"c:");	/*character special*/
			break;
		case 5:
			(void)fprintf(fp,"l:"); /*link*/
			break;
		case 6:
			(void)fprintf(fp,"p:");	/*pipe*/
			break;
		default:
			(void)fprintf(fp,"%d:",ftype);
			break;
	}
}		

/**
 ** Display file permissions.
 **/
void
pr_mode(fp, mode)
FILE *fp;
mode_t  mode;
{
	(void)fprintf(fp, ",%c%c%c%c%c%c%c%c%c", 
			mode & OREAD ? 'r' : '-',
			mode & OWRITE ? 'w' : '-',
			mode & OEXEC ? 'x' : '-',
			mode & GREAD ? 'r' : '-',
			mode & GWRITE ? 'w' : '-',
			mode & GEXEC ? 'x' : '-',
			mode & AREAD ? 'r' : '-',
			mode & AWRITE ? 'w' : '-',
			mode & AEXEC ? 'x' : '-');
}

/**
 ** Display change owner or change group specific information.
 **/
void
pr_own(fp,uid,gid)
FILE 	*fp;
id_t  	uid;
id_t  	gid;
{
	char *newname;

	if (uid != -1){	/* new user owner */
		(void)fprintf(fp,",user:");
   		if ((newname=id_conv((int)uid,s_user,uidbegin))!= NULL)
	    		(void)fprintf(fp,"%s",newname);
	   	else
			(void)fprintf(fp,"%ld",uid);
	}
	if (gid != -1){	/* new group owner */
		(void)fprintf(fp,",group:");
   		if ((newname=id_conv((int)gid,s_grp,gidbegin))!= NULL)
	    		(void)fprintf(fp,"%s",newname);
	   	else
			(void)fprintf(fp,"%ld",gid);
	}
}

/**
 ** Display ipc events object information.
 **/
void
pr_ipcobj(fp,ipcp,procp,event)
FILE 	*fp;
struct ipc_r *ipcp;
struct proc_r *procp;
ushort event;
{
	(void)fprintf(fp,"(%d:",ipcp->i_id);	/* object id */
	switch (event){				/* type */
   	case ADT_MSG_CTL: case ADT_MSG_GET: case ADT_MSG_OP:
		(void)fprintf(fp,"%c:",MSG);		/* message */
		break;
   	case ADT_SEM_CTL: case ADT_SEM_GET: case ADT_SEM_OP:
		(void)fprintf(fp,"%c:",SEMA);		/* semaphore */
		break;
   	case ADT_SHM_CTL: case ADT_SHM_GET: case ADT_SHM_OP:
		(void)fprintf(fp,"%c:",SHMEM);		/* shared memory */
		break;
	default:	/* should never reach */
		(void)fprintf(fp,"?:");
		break;
	}
	pr_lvl(fp, procp->pr_lid);		/* object level */
	(void)fprintf(fp,":::::)");		/* device,devmajor,devminor,inode,fsid are null */
}
	
/**
 ** Display fully qualified level name (or lid, if it can't be translated).
 **/
void
pr_lvl(fp,level)
FILE 	*fp;
level_t level;
{
	static char	*bufp = NULL;
	int		ret;

	if (mac_installed){
		if (bufp == NULL) {
			if((bufp=(char *)calloc((LVL_MAXNAMELEN+1),sizeof(char)))==NULL){
				(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
				adt_exit(ADT_MALLOC);
			}
		}
		(void)fprintf(fp,"\"");
		if (level != 0){
			if (((ret=adt_lvlout(&level, bufp, LVL_MAXNAMELEN, LVL_FULL)))==-1){
				if (errno == ENOSPC){
					ret=adt_lvlout(&level, bufp, 0, LVL_FULL);
					if((bufp=(char *)realloc(bufp, ret+1))==NULL){
						(void)pfmt(stderr, MM_ERROR, E_BDMALLOC);
						adt_exit(ADT_MALLOC);
					}
					if((adt_lvlout(&level, bufp, ret, LVL_FULL))!=0){
						bufp[0]='\0';
					}
				}
			}
			if (bufp[0] != '\0')
				(void)fprintf(fp,"%s",bufp);
			else
				(void)fprintf(fp,"%ld",level);
		}
		else
			(void)fprintf(fp,"?");
		(void)fprintf(fp,"\"");
	}
}

/**
 ** Display disp_attr and set_attr events specific information.
 **/
void
pr_dev(fp,dp,event)
FILE 	*fp;
struct dev_r *dp;
ushort 	event;
{

	if (dp->devstat.dev_relflag==1)		/* release flag */
		(void)fprintf(fp,"persistent");
	else if (dp->devstat.dev_relflag==2)
		(void)fprintf(fp,"lastclose");
	else if (dp->devstat.dev_relflag==3)
		(void)fprintf(fp,"system");
	else
		(void)fprintf(fp,"?");
	if (dp->devstat.dev_mode==1)		/* device mode */
		(void)fprintf(fp,",static,");
	else if (dp->devstat.dev_mode==2)
		(void)fprintf(fp,",dynamic,");
	else
		(void)fprintf(fp,",?,");
	pr_lvl(fp,dp->devstat.dev_lolevel);	/* low level */
	(void)fprintf(fp,",");
	pr_lvl(fp,dp->devstat.dev_hilevel);	/* high level */
	if (dp->devstat.dev_state==1)		/* device state */
		(void)fprintf(fp,",private");
	else if (dp->devstat.dev_state==2)
		(void)fprintf(fp,",public");
	else
		(void)fprintf(fp,",?");
	if (event == ADT_DISP_ATTR){	/*if event disp_attr, print inuse flag*/
		if (dp->devstat.dev_usecount==1)
			(void)fprintf(fp,",inuse");
		else
			(void)fprintf(fp,",unused");
	}
}

/**
 ** Display the information corresponding to a sched_lk event
 ** generated by the system call plock().
 **/
void pr_plock(fp,plockp)
FILE	*fp;
struct plock_r	*plockp;
{
	switch (plockp->p_op){
	case PROCLOCK:
		(void)fprintf(fp,",PROCLOCK");
		break;
	case TXTLOCK:
		(void)fprintf(fp,",TXTLOCK");
		break;
	case DATLOCK:
		(void)fprintf(fp,",DATLOCK");
		break;
	default:
		(void)fprintf(fp,",?");
		break;
	}
}
/**
 ** Display flags in mount record that describe the bits passed to
 ** the system call.
 **/
void
pr_mount(fp,mountp)
FILE	*fp;
struct mount_r *mountp;
{
	if (mountp->m_flags & MS_RDONLY)
		(void)fprintf(fp,",RDONLY");	/* Read-only */ 
	if (mountp->m_flags & MS_FSS)
		(void)fprintf(fp,",FSS");	/* Old (4-argument) mount */
	if (mountp->m_flags & MS_DATA)
		(void)fprintf(fp,",DATA");	/* 6-argument mount */
	if (mountp->m_flags & MS_NOSUID)
		(void)fprintf(fp,",NOSUID");	/* Setuid disallowed */
	if (mountp->m_flags & MS_REMOUNT)
		(void)fprintf(fp,",REMOUNT");	/* Remount */
	if (mountp->m_flags & MS_NOTRUNC)
		(void)fprintf(fp,",NOTRUNC");	/* Return ENAMETOOLONG for long  filenames */
}
/**
 ** Display the specific information for a recvfd event.
 **/
void
pr_recvfd(fp,rcfdp)
FILE	*fp;
struct recvfd_r *rcfdp;
{
	(void)fprintf(fp,",P%ld,%d,P%ld,%d",
		rcfdp->r_spid,rcfdp->r_sfd, /*sender's pid and file descriptor*/
		rcfdp->r_rpid,rcfdp->r_rfd); /*receiver's pid and fd */
}
/**
 ** Display specific information for the iocntl event.
 **/
void
pr_ioctl(fp,ioctlp)
FILE	*fp;
struct ioctl_r *ioctlp;
{
	(void)fprintf(fp,",%d,%d,", ioctlp->i_fdes, ioctlp->i_cmd);
	if (ioctlp->i_flag & FOPEN)
		(void)fprintf(fp,"FOPEN:");
	if (ioctlp->i_flag & FREAD)
		(void)fprintf(fp,"FREAD:");
	if (ioctlp->i_flag & FWRITE)
		(void)fprintf(fp,"FWRITE:");
	if (ioctlp->i_flag & FNDELAY)
		(void)fprintf(fp,"FNDELAY:");
	if (ioctlp->i_flag & FAPPEND)
		(void)fprintf(fp,"FAPPEND:");
	if (ioctlp->i_flag & FSYNC)
		(void)fprintf(fp,"FSYNC:");
	if (ioctlp->i_flag & FNONBLOCK)
		(void)fprintf(fp,"FNONBLOC:");
	if (ioctlp->i_flag & FMASK)
		(void)fprintf(fp,"FMASK:");
	if (ioctlp->i_flag & FCREAT)
		(void)fprintf(fp,"FCREAT:");
	if (ioctlp->i_flag & FTRUNC)
		(void)fprintf(fp,"FTRUNC:");
	if (ioctlp->i_flag & FEXCL)
		(void)fprintf(fp,"FEXCL:");
	if (ioctlp->i_flag & FNOCTTY)
		(void)fprintf(fp,"FNOCTTY:");
	if (ioctlp->i_flag & FASYNC)
		(void)fprintf(fp,"FASYNC:");
	if (ioctlp->i_flag & FCLONE)
		(void)fprintf(fp,"FCLONE:");
}

/**
 ** Display specific information for the fcntl event when using the 
 ** fcntl_r record.
 **/
void
pr_fcntl(fp,fcntlp)
FILE	*fp;
struct fcntl_r *fcntlp;
{
	(void)fprintf(fp,",%d",fcntlp->f_fdes);
	switch (fcntlp->f_cmd){
	case F_DUPFD:
		/* display cmd and dup file descriptor */
		(void)fprintf(fp,",F_DUPFD,%d",fcntlp->f_arg);
		break;
	case F_SETFD:
		/* display cmd and close-on-exec flag (0 or 1) */
		(void)fprintf(fp,",F_SETFD,%d",fcntlp->f_arg);
		break;
	case F_SETFL:
		/* display cmd and fildes status flag -- see fcntl(5) */
		(void)fprintf(fp,",F_SETFL,");
		if (fcntlp->f_arg & O_APPEND)
			(void)fprintf(fp,"O_APPEND:");
		if (fcntlp->f_arg & O_NDELAY)
			(void)fprintf(fp,"O_NDELAY:");
		if (fcntlp->f_arg & O_NONBLOCK)
			(void)fprintf(fp,"O_NONBLOCK:");
		if (fcntlp->f_arg & O_SYNC)
			(void)fprintf(fp,"O_SYNC:");
		break;
	default:
		(void)fprintf(fp,",%d",fcntlp->f_cmd);
	}
}

/**
 ** Display specific information for the fcntl event when using the 
 ** fcntlk_r record.
 **/
void
pr_fcntlk(fp,fcntlkp)
FILE	*fp;
struct fcntlk_r *fcntlkp;
{
	/* display file descriptor */
	(void)fprintf(fp,",%d",fcntlkp->f_fdes);
	/* display cmd paased to system call */
	switch (fcntlkp->f_cmd){
	case F_ALLOCSP:
		(void)fprintf(fp,",F_ALLCOSP");
		break;
	case F_FREESP:
		(void)fprintf(fp,",F_FREESP");
		break;
	case F_SETLK:
		(void)fprintf(fp,",F_SETLK");
		break;
	case F_SETLKW:
		(void)fprintf(fp,",F_SETLKW");
		break;
	case F_RSETLK:
		(void)fprintf(fp,",F_RSETLK");
		break;
	case F_RSETLKW:
		(void)fprintf(fp,",F_RSETLKW");
		break;
	default:
		(void)fprintf(fp,",%d",fcntlkp->f_cmd);
		break;
	}
	/* display the members of the struct flock passed to the system call */ 
	(void)fprintf(fp,",%d,%d,%ld,%ld",
		fcntlkp->f_flock.l_type, fcntlkp->f_flock.l_whence,
		fcntlkp->f_flock.l_start, fcntlkp->f_flock.l_len);
}

/**
 ** Display specific information for set_pgrp and set_sid events.
 **/
void
pr_setpgrp(fp,pgrpp)
FILE 	*fp;
struct setpgrp_r *pgrpp;
{
	switch (pgrpp->s_flag){
	case 1:	/* event triggered by setpgrp() */
		(void)fprintf(fp,",setpgrp");
		break;
	case 3:	/* event triggered by setsid(), no specific info */
		break;
	case 5:	/* event triggered by setpgid() */
		(void)fprintf(fp,",setpgid,P%d,%d",pgrpp->s_pid,pgrpp->s_pgid);
		break;
	default:
		(void)fprintf(fp,",?");
		break;
	}
}
/**
 ** Display list of groups for the set_groups event
 **/
void
pr_setgrps(fp,ngroups,groups)
FILE	*fp;
int	ngroups;
gid_t	*groups;
{
	int 	i;
	char	*groupname;
	if (groups){
		for (i=0; i<ngroups; i++,groups++){
			if((groupname=id_conv((int)*groups,s_grp,gidbegin))!=NULL)
				(void)fprintf(fp,",%s",groupname);
			else
				(void)fprintf(fp,",%ld",*groups);
		}
	}
	else
		(void)fprintf(fp,",?");
}

/**
 ** Display specific information for the sched_rt and sched_ts events when
 ** the priocntl(2) system call was invoked with the PC_ADMIN command.
 **/
void
pr_admin(fp, adminp)
FILE 	*fp;
struct admin_r *adminp;
{
	(void)fprintf(fp,",");
	if (adminp->a_func==ADT_RT_SETDPTBL)
		(void)fprintf(fp,"%s","RT_SETDPTBL");
	else if (adminp->a_func==ADT_TS_SETDPTBL)
		(void)fprintf(fp,"%s","TS_SETDPTBL");
	else
		(void)fprintf(fp,"%s","?");
	(void)fprintf(fp,",%d",adminp->a_globpri);
	(void)fprintf(fp,",%ld",adminp->a_quantum);

	if (adminp->a_func==ADT_TS_SETDPTBL) {
		(void)fprintf(fp,",%d",adminp->a_tqexp);
		(void)fprintf(fp,",%d",adminp->a_slpret);
		(void)fprintf(fp,",%d",adminp->a_maxwait);
		(void)fprintf(fp,",%d",adminp->a_lwait);
	}
}

/**
 ** Display specific information for the sched_rt and sched_ts events when
 ** the priocntl(2) system call was invoked with the PC_SETPARMS command.
 **/
void
pr_parms(fp, parmsp)
FILE 	*fp;
struct parms_r *parmsp;
{
	(void)fprintf(fp,",");
	if (parmsp->p_func==ADT_RT_NEW)
		(void)fprintf(fp,"%s","RT_NEW");
	else if (parmsp->p_func==ADT_TS_NEW)
		(void)fprintf(fp,"%s","TS_NEW");
	else if (parmsp->p_func==ADT_RT_PARMSSET)
		(void)fprintf(fp,"%s","RT_PARMSSET");
	else if (parmsp->p_func==ADT_TS_PARMSSET)
		(void)fprintf(fp,"%s","TS_PARMSSET");
	else
		(void)fprintf(fp,"%s","?");
	(void)fprintf(fp,",P%ld",parmsp->p_procid);
	(void)fprintf(fp,",%d",parmsp->p_upri);
	(void)fprintf(fp,",%ld",parmsp->p_uprisecs);

	if (parmsp->p_func==ADT_RT_NEW || parmsp->p_func==ADT_RT_PARMSSET)
		(void)fprintf(fp,",%ld",parmsp->p_tqnsecs);
}

/**
 ** Display the information corresponding to an auditevt system call event.
 **/
void
pr_aevt(fp,aevtp,dumpp)
FILE 	*fp;
struct aevt_r	*aevtp;
dumprec_t	*dumpp;
{
	level_t	*lvlp;
	int	i;
	char *name=NULL;

 	switch (aevtp->cmd) {
		case ASETME:	/* if process event mask set */
 				(void)fprintf(fp,",ASETME");
 				pr_amsk(fp,aevtp->emask);
				break;
		case ASETSYS:	/* if system mask set */
 				(void)fprintf(fp,",ASETSYS");
 				pr_amsk(fp,aevtp->emask);
				break;
		case ASETUSR:	/* if user mask set */
 				(void)fprintf(fp,",ASETUSR");
        			if (aevtp->uid < 0)
					(void)fprintf(fp,",?");
				else {
					/*display login name of the user whose mask has been changed*/
					if ((name=id_conv((int)aevtp->uid,s_user,uidbegin))!= NULL)
                        			(void)fprintf(fp,",%s",name);
					else
                    				(void)fprintf(fp,",%ld",aevtp->uid);
				}
 				pr_amsk(fp,aevtp->emask);
				break;
		case ASETLVL:	/* if level set, display level range or list */
 				(void)fprintf(fp,",ASETLVL,");
 				if ( aevtp->flags & ADT_RMASK ) {
					pr_lvl(fp,aevtp->lvlmin);
 					(void)fprintf(fp,"-");
					pr_lvl(fp,aevtp->lvlmax);
				}
 				if (aevtp->flags & ADT_LMASK){
				    lvlp=(level_t *)dumpp->freeformat;
				    for (i=1;i<=aevtp->nlvls;i++) {
					/* freeformat is null when lvl_tblp
					   passed by user invalid */
					if (lvlp) { 
					    pr_lvl(fp,*lvlp);
                        		    lvlp++;
                		        }
					else
						break;
				    }
				}
				/* if object level mask set, display mask */
 				if (aevtp->flags & ADT_OMASK)
 					pr_amsk(fp,aevtp->emask);
				break;
		case ANAUDIT:
 				(void)fprintf(fp,",ANAUDIT");
				break;
		case AYAUDIT:
 				(void)fprintf(fp,",AYAUDIT");
				break;
		default:
				/* should never reach */
 				(void)fprintf(fp,",?");
				break;
	}
}

/**
 ** Display event type and status (of event that failed to be recorded)
 ** for an audit_dmp event.
 **/
void
pr_admp(fp,admpp)
FILE	*fp;
struct	admp_r	*admpp;
{
	char *typename = NULL;

	typename=id_conv(admpp->a_event,s_type,typebegin);
	if (typename != NULL)
		(void)fprintf(fp,",%s",typename);
	else
		(void)fprintf(fp,",%d",admpp->a_event);
	if (admpp->a_status == 0)
		(void)fprintf(fp,",SUCCESS");
	else 
		(void)fprintf(fp,",FAILURE(%d)",admpp->a_status);
}

/**
 ** Display information corresponding to an auditlog system call event.
 **/
void
pr_alog(fp,alogp)
FILE 	*fp;
struct alog_r *alogp;
{
	
	(void)fprintf(fp,",%s%s%s%s%s%s%s",
		alogp->flags & PPATH ? "PPATH:" : "",
		alogp->flags & PNODE ? "PNODE:" : "",
		alogp->flags & APATH ? "APATH:" : "",
		alogp->flags & ANODE ? "ANODE:" : "",
		alogp->flags & PSIZE ? "PSIZE:" : "",
		alogp->flags & ASPECIAL ? "ASPECIAL:" : "",
		alogp->flags & PSPECIAL ? "PSPECIAL:" : "");
	if (alogp->onfull==ASHUT)
		(void)fprintf(fp,",ASHUT");
	else if (alogp->onfull==ADISA)
		(void)fprintf(fp,",ADISA");
	else if (alogp->onfull&AALOG) {
		(void)fprintf(fp,",AALOG");
		if (alogp->onfull&APROG)
			(void)fprintf(fp,",APROG");
	}
	if (alogp->onerr==ASHUT)
		(void)fprintf(fp,",ASHUT");
	else if (alogp->onerr==ADISA)
		(void)fprintf(fp,",ADISA");
	(void)fprintf(fp,",%d",alogp->maxsize);
	(void)fprintf(fp,",%s,%s,%s,%s,%s",
		alogp->pnodep,
		alogp->anodep,
		alogp->ppathp,
		alogp->apathp,
		alogp->progp);
}


/**
 ** Display the names of the events set based on the audit event mask emask.
 **/
void
pr_amsk(fp,emask)
FILE 	*fp;
adtemask_t	emask;
{
	int i;
	int allcnt = 0;
	char 	*typename=NULL;

	/* don't print event names if they are all set, print "all" instead; 
	 * print "none" if no event is set.
	 */
	for (i=1;i<=ADT_NUMOFEVTS;i++)
		if (EVENTCHK(i,emask)) {
                        allcnt++;
			if (i != allcnt)
				break;
		}
	if (allcnt == 0)
		(void)fprintf(fp,",none");
	else if (allcnt == ADT_NUMOFEVTS)
		(void)fprintf(fp,",all");
	else
             for (i=1;i<=ADT_NUMOFEVTS;i++)	
		if (EVENTCHK(i,emask))
			if((typename=id_conv(i,s_type,typebegin))!=NULL)
				(void)fprintf(fp,",%s",typename);
			else
				(void)fprintf(fp,",%d",i);
}

/** If the SETUID record was generated by the set_uid event print the new uid
 ** If the SETUID record was generated by the set_gid event print the new gid
 **/
void
pr_setid(fp,setidp)
FILE *fp;
struct setid_r *setidp;
{
	char *name=NULL;

	if (setidp->s_nid < 0){
		(void)fprintf(fp,",?");
		return;
	}

	if (setidp->s_type == ADT_UID) {
		/*display user login name*/
		if ((name=id_conv((int)setidp->s_nid,s_user,uidbegin))!= NULL)
               	       	(void)fprintf(fp,",%s",name);
               	else
               	       	(void)fprintf(fp,",%ld",setidp->s_nid);
	}
	else {
		if (setidp->s_type == ADT_GID) {
               		if ((name=id_conv((int)setidp->s_nid,s_grp,gidbegin))!= NULL)
                       		(void)fprintf(fp,",%s",name);
                	else
                		(void)fprintf(fp,",%ld",setidp->s_nid);
		}
		else
			/*s_type is invalid, display the uid without conversion*/
                	(void)fprintf(fp,",%ld",setidp->s_nid);
	}
}


/**
 ** Determine if an event has a second filename record generated by vn_create().
 ** All events that create objects and are successful generate a second
 ** filename record.
 **/
int
created_vn(event) 
ushort event;
{
	switch (event) {
	case ADT_CREATE : case ADT_MK_DIR : 
	case ADT_MK_MLD : case ADT_MK_NODE : 
		return (1);
	default:
		return (0);
	}
}

/**
 ** Free an element (this_p) of the linked list of pathname records.  
 ** At the end of the routine, "next_p" points to the next element of 
 ** the list that will be examined in pr_obj(), "this_p" points to the
 ** the element preceding "next_p."
 **/
void
fn_free(before_p,this_p,next_p)
struct save	**before_p; 
struct save	**this_p;
struct save	**next_p;
{
	if (*this_p == fn_beginp) {
		fn_beginp = (*this_p)->next;
	}
	if (*this_p == fn_lastp) {
		fn_lastp = *before_p;
	}
	if (*before_p != (struct save *)NULL){
		(*before_p)->next = (*this_p)->next;
		*next_p = (*before_p)->next;
	}
	else {
		*next_p = fn_beginp;
	}
	free((*this_p)->name);
	free(*this_p);
	*this_p = *before_p;
}

/**
 ** Convert the time according to the timezone of the audited machine
 **/
struct tm *actual_time(clock)
const time_t *clock;
{
	time_t	conv_time;

	(void *)ctime(clock);
	conv_time = *clock - env.gmtsecoff;
	return (gmtime(&conv_time));
}

/**
 ** Convert integer to character string (ASCII representation of number).
 **/
void
adt_itoa(n, s)
int n;
char s[];
{
	int i, sign, j, p;
	char c;

	if ((sign = n) < 0)	/* record sign */
		n = -n;		/* make n positive */
	i = 0;
	do {	/* generate digits in reverse order */
		s[i++] = n % 10 + '0';	/* get next digit */
	} while ((n /= 10) > 0); 	/* delete it */
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';

	for (p=0, j=strlen(s)-1; p<j; p++, j--) 
		c=s[p], s[p]=s[j], s[j]=c;
}


/**
 ** Get covert channel event name from event number.
 **/
static char *
cc_getname(num)
	int num;
{
#ifdef CC_PARTIAL
 	register int i;
	char *name = (char *)NULL;

        for (i = 0; cc_names[i].cc_number >= 0; i++) {
		if (cc_names[i].cc_number == num) {
                        name = cc_names[i].cc_name;
                        break;
		}
	}

        return(name);
#endif
}
/**
 ** Display the resource, sof limit and hard limit for the setrlimit event
 **/
void pr_rlim(fp,rlimp)
FILE	*fp;
struct rlim_r	*rlimp;
{
	switch (rlimp->r_rsrc){
		case RLIMIT_CORE:
			(void)fprintf(fp,",RLIMIT_CORE");
			break;
		case RLIMIT_CPU:
			(void)fprintf(fp,",RLIMIT_CPU");
			break;
		case RLIMIT_DATA:
			(void)fprintf(fp,",RLIMIT_DATA");
			break;
		case RLIMIT_FSIZE:
			(void)fprintf(fp,",RLIMIT_FSIZE");
			break;
		case RLIMIT_NOFILE:
			(void)fprintf(fp,",RLIMIT_NOFILE");
			break;
		case RLIMIT_STACK:
			(void)fprintf(fp,",RLIMIT_STACK");
			break;
		case RLIMIT_VMEM:
			(void)fprintf(fp,",RLIMIT_VMEM");
			break;
		default:
			(void)fprintf(fp,",?");
			break;
	}
	(void)fprintf(fp,",%ld,%ld",rlimp->r_soft,rlimp->r_hard);
	return;
 }
/**
 ** Display the information corresponding to a sched_lk event
 ** generated by the system call memctl().
 **/
void pr_mctl(fp,mctlp)
FILE	*fp;
struct mctl_r	*mctlp;
{
	if (mctlp->m_attr == 0)
	{
		(void)fprintf(fp,",0");
		return;
	}
		
	/*Attributes for page mapping*/
	if (mctlp->m_attr & SHARED)
		(void)fprintf(fp,",SHARED");
	if (mctlp->m_attr & PRIVATE)
		(void)fprintf(fp,",PRIVATE");

	/*Attributes for page protection*/
	if (mctlp->m_attr & PROT_READ)
		(void)fprintf(fp,",PROT_READ");
	if (mctlp->m_attr & PROT_WRITE)
		(void)fprintf(fp,",PROT_WRITE");
	if (mctlp->m_attr & PROT_EXEC)
		(void)fprintf(fp,",PROT_EXEC");
		
	return;
}
/**
 ** Display the free-format data for a USER_R record.
 ** The free-format data is parsed to allow for embedded null bytes.
 **/
void 
pr_user(fp,datap)
FILE *fp;
char *datap;
{
	int char_ct,count;
	char *tmp;

	tmp=datap;
	char_ct=count=0;

	(void)fprintf(fp,",");

	/*Display the entire free-format data*/
	while (char_ct < free_size)
	{
		/*Display up to the null byte*/
		count=fprintf(fp,"%s",datap);
		(void)fprintf(fp,"%c",' ');
		datap = datap + count +1;
		char_ct = char_ct + count + 1;
	}
}
/**
 ** Display the information corresponding to a modadm event
 ** generated by the system call modadm.
 **/
void
pr_modadm(fp,modadmp,freep)
FILE	*fp;
struct modadm_r *modadmp;
char *freep;
{

	switch (modadmp->type) {
	case MOD_TY_STR:
			(void)fprintf(fp,"%s",",streams,");
			break;
	case MOD_TY_FS:
			(void)fprintf(fp,"%s",",filesystem,");
			break;
	case MOD_TY_CDEV:
			(void)fprintf(fp,"%s",",character device,");
			break;
	case MOD_TY_BDEV:
			(void)fprintf(fp,"%s",",block device,");
			break;
        case MOD_TY_NONE:
			(void)fprintf(fp,"%s",",none,");
			break;
        case MOD_TY_MISC:
			(void)fprintf(fp,"%s",",misc,");
			break;
	default:
			(void)fprintf(fp,"%s",",");
			break;
	}

	switch (modadmp->cmd) {
	case MOD_C_MREG:
			(void)fprintf(fp,"%s","register");
			break;
	default:
			(void)fprintf(fp,"%s",",");
			break;
	}

	if (freep != NULL) {
                (void)fprintf(fp,",%s",freep);
		switch (modadmp->type) {
		case MOD_TY_FS:
                        	/*The "module type specific data" is contained*/
				/*in the free-format field.                   */
                        	(void)fprintf(fp,",%s",freep+MODMAXNAMELEN);
				break;
		case MOD_TY_CDEV:
		case MOD_TY_BDEV:
                        	(void)fprintf(fp,",%d",modadmp->typedata);
				break;
		case MOD_TY_STR:
        	case MOD_TY_NONE:
        	case MOD_TY_MISC:
		default:
				break;
		}
	}
}

/**
 ** Display the information corresponding to the modload and moduload events
 **/
void
pr_modld(fp,modldp,event,status,freep)
FILE	*fp;
struct modld_r *modldp;
ushort event;
long status;
char *freep;
{
	/*For a modload failure filename records were not generated.*/
        /*Display object information fields for consistency.        */
	if ((status != 0) && (event == ADT_MODLOAD)) {
        	if (freep != NULL)
        		(void)fprintf(fp,"(%s:module::::::)",freep);
		else
        		(void)fprintf(fp,"(:module::::::)");
	}

	switch (modldp->action) {
	case 1:
		(void)fprintf(fp,",auto");
		break;
	case 2:
		(void)fprintf(fp,",demand");
		break;
	default:
		(void)fprintf(fp,",?");
		break;
	}

	/*For the moduload event, the module id has been displayed in the*/ 
	/*object_id field.                                               */
	if ((event == ADT_MODLOAD) && (status == 0))
		(void)fprintf(fp,",%d",modldp->id);
}

/**
 ** Display the information corresponding to the modpath event
 **/
void
pr_modpath(fp,status,freep)
FILE	*fp;
long status;
char *freep;
{
	if (freep)
		(void)fprintf(fp,",%s",freep);
	else {
		if (status == 0 )
			/*case modadmin -D or modpath(NULL)*/
			(void)fprintf(fp,",%s","<NULL>");
	}
}

/**
 ** Display the information corresponding to the kill event
 **/
void
pr_kill(fp,killp,freep)
FILE *fp;
struct kill_r	*killp;
pid_t *freep;
{
	int i;

	/* Display the signal */
	(void)fprintf(fp,",%d",killp->k_sig);

	if (killp->k_entries <= 0)
		return;

	/* Display the pid of each process to which the signal was posted */
	if (freep) {
		for (i=0; i<killp->k_entries; i++,freep++)
                	(void)fprintf(fp,",%ld",*freep);
	}
	return;
}
