#ident	"@(#)crash:i386/cmd/crash/proc.c	1.1.1.5"

/*
 * This file contains code for the crash functions:  proc, defproc, hrt.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/systm.h>
#ifdef notdef
#include <sys/hrtcntl.h>
#include <sys/hrtsys.h>
#endif
#include <sys/priocntl.h>
#include <sys/procset.h>
#include <sys/vnode.h>
#include <sys/session.h>
#include <priv.h>
#include <sys/secsys.h>
#include <sys/mac.h>
#include <audit.h>
#include <sys/stropts.h>
#include <sys/exec.h>
#include <sys/vmparam.h>
#include <sys/lwp.h>
#include "crash.h"

void pr_privs();
void pr_aproc();
void pr_alwp();

extern char *cnvemask();

/* get arguments for proc function */
int
getproc()
{
	int slot;
	int full = 0;
	int tokename = 0;
	long arg1;
	long arg2;
	int c;
	char *heading = "SLOT ST  PID  PPID   SID   UID  LWP  PRI ENG  EVENT       NAME\n";

	optind = 1;
	while((c = getopt(argcnt,args,"fnw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'n' :	tokename = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(tokename && !full)
		longjmp(syn,0);
	fprintf(fp,"PROC TABLE SIZE = %d\n",vbuf.v_proc);
	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) {
		do {
			if (getargs(PROCARGS,&arg1,&arg2))
			    for (slot = arg1; slot <= arg2; slot++)
				prproc(full,slot,tokename,-1,heading);
			else
				prproc(full,-1,tokename,arg1,heading);
		} while(args[++optind]);
	}
	else{
		/*
		 * Create a new slot table to
		 * reflect the current state
		 */
		if(active)
			makeslottab();
		for (slot = 0; slot < vbuf.v_proc; slot++)
			prproc(full,slot,tokename,-1,heading);
	}
}

/* print proc table */
int
prproc(full, slot, tokename, procaddr, heading)
int full, slot, tokename;
vaddr_t procaddr;
char *heading;
{
	char ch;
	struct proc procbuf;
	int i;
	lwp_t lwp;
	lwp_t *lwpp;
	struct cred cred;
	struct execinfo exec;

	if (procaddr == -1)
		procaddr = (vaddr_t)slot_to_proc(slot);
	else for (;;) {
		if (++slot == vbuf.v_proc)
			error("%x is not a valid proc address\n", procaddr);
		if (procaddr == (vaddr_t)slot_to_proc(slot))
			break;
	}
	if (!procaddr)
		return;

	/* get the proc structure */

	readmem(procaddr,1,-1,
		&procbuf,sizeof procbuf,"proc table");

	/* read in execinfo structure to get command name */
	if (procbuf.p_execinfo) {
		readmem((long)procbuf.p_execinfo, 1, -1,
			(char *)&exec,sizeof(struct execinfo ),
			"execinfo structure");
		exec.ei_psargs[sizeof(exec.ei_psargs)-1] = '\0';
		exec.ei_comm[sizeof(exec.ei_comm)-1] = '\0';
	}
	else
		memset(&exec, 0, sizeof(exec));

	/* lwp structure */

	if (procbuf.p_lwpp)
		readlwp(procbuf.p_lwpp, &lwp);
	else {
		memset(&lwp, 0, sizeof(lwp));
		lwp.l_stat = -1;
	}

	/* read in cred structure to get uid */

	if (lwp.l_cred)
		readmem((vaddr_t)lwp.l_cred, 1, -1,
			&cred, sizeof(cred), "cred structure");
	else
		memset(&cred, 0, sizeof(cred));

	if (full)
		fprintf(fp,"%s",heading);

	switch (lwp.l_stat) {
	case SSLEEP:	ch = 's'; break;
	case SRUN:	ch = 'r'; break;
	case SIDL:	ch = 'i'; break;
	case SSTOP:	ch = 't'; break;
	case SONPROC:	ch = 'p'; break;
	case -1:	ch = 'z'; break;
	default:	ch = '?'; break;
	}

	if (slot == -1)
		fprintf(fp,"  - ");
	else fprintf(fp,"%4d",slot);

	fprintf(fp," %c %5u %5u %5u %5u %4u  %3u",
			ch,
			slot_to_pid(slot),
			procbuf.p_ppid,
			readsid(procbuf.p_sessp),
			cred.cr_ruid,
			0,
			lwp.l_pri);
	if (lwp.l_eng)
		fprintf(fp," %3u",eng_to_id(lwp.l_eng));
	else
		fprintf(fp,"   -");

	switch(lwp.l_stype) {
		case ST_NONE:    ch = ' '; break;
		case ST_COND:    ch = 'c'; break;
		case ST_EVENT:   ch = 'e'; break;
		case ST_RDLOCK:  ch = 'r'; break;
		case ST_WRLOCK:  ch = 'w'; break;
		case ST_SLPLOCK: ch = 's'; break;
		case ST_USYNC:   ch = 'u'; break;
		default:	 ch = '?'; break;
	}
	if( lwp.l_stype != ST_NONE) {
		fprintf(fp,"  %c-%08x",ch,lwp.l_syncp);
	} else
		fprintf(fp,"            ");

	fprintf(fp,"  %s\n",full?exec.ei_psargs:exec.ei_comm);

	if (full) {
		int s;
		fprintf(fp,"\tProcess Credentials: ");
		fprintf(fp,"uid: %u, gid: %u, real uid: %u, real gid: %u lid: %x\n",
			cred.cr_uid, cred.cr_gid, cred.cr_ruid,
			cred.cr_rgid, cred.cr_lid);
		fprintf(fp, "\tProcess Privileges: ");
		pr_privs(cred, tokename);
		fprintf(fp,"\tSignal state:"
			" pend: %08x %08x ignore: %08x %08x",
			procbuf.p_sigs.ks_sigbits[0],
			procbuf.p_sigs.ks_sigbits[1],
			procbuf.p_sigignore.ks_sigbits[0],
			procbuf.p_sigignore.ks_sigbits[1]);
		for (s=0; s<MAXSIG; ++s)
		{
			if ((s%3) == 0)
				fprintf(fp, "\n\t");
			fprintf(fp,"  [%02d]: %02x %02x %08x",
				s,
				procbuf.p_sigstate[s].sst_cflags,
				procbuf.p_sigstate[s].sst_rflags,
				procbuf.p_sigstate[s].sst_handler);
		}
		putc('\n', fp);

		if (procbuf.p_fdtab.fdt_entrytab) {
			int i, p, n = procbuf.p_fdtab.fdt_size;
			fd_entry_t *buf;

			buf = readmem((long)procbuf.p_fdtab.fdt_entrytab, 1, -1,
				NULL, n * sizeof(fd_entry_t), "fdt_entrytab");

			fprintf(fp,"\tFile descriptors: entrytab=%x",
				procbuf.p_fdtab.fdt_entrytab);
			p = 0;
			for (i=0; i<n; ++i)
				if (buf[i].fd_status)
				{
					if ((p++%3) == 0)
						fprintf(fp, "\n\t");
					fprintf(fp,"  [%02d]: %08x %2u %1x %1x",
						i,
						buf[i].fd_file,
						buf[i].fd_lwpid,
						buf[i].fd_flag,
						buf[i].fd_status);
				}
			putc('\n', fp);
			cr_free(buf);
		}
	}

	if (full && procbuf.p_auditp) {
		fprintf(fp,"\tProcess Audit Structure:\n");
		pr_aproc(procbuf.p_auditp, tokename);
	}

	/* print info about each lwp */

	if (full) {
		fprintf(fp,"\tLWP signal state:"
			" pend: %08x %08x mask: %08x %08x\n",
			lwp.l_sigs.ks_sigbits[0],
			lwp.l_sigs.ks_sigbits[1],
			lwp.l_sigheld.ks_sigbits[0],
			lwp.l_sigheld.ks_sigbits[1]);

		if (lwp.l_auditp) {
			fprintf(fp,"\tLWP Audit Structure:\n");
			pr_alwp(lwp.l_auditp, tokename);
		}
	}

	for (i = 1; (lwpp = lwp.l_next) != NULL; i++) {
		readlwp(lwpp, &lwp);

		switch (lwp.l_stat) {
		case SSLEEP:	ch = 's'; break;
		case SRUN:	ch = 'r'; break;
		case SIDL:	ch = 'i'; break;
		case SSTOP:	ch = 't'; break;
		case SONPROC:	ch = 'p'; break;
		default:	ch = '?'; break;
		}

		fprintf(fp, "     %c                           %2u  %3u  %2u",
			ch, i, lwp.l_pri, eng_to_id(lwp.l_eng));

		switch(lwp.l_stype) {
			case ST_NONE:    ch = ' '; break;
			case ST_COND:    ch = 'c'; break;
			case ST_EVENT:   ch = 'e'; break;
			case ST_RDLOCK:  ch = 'r'; break;
			case ST_WRLOCK:  ch = 'w'; break;
			case ST_SLPLOCK: ch = 's'; break;
			case ST_USYNC:   ch = 'u'; break;
			default:	 ch = '?'; break;
		}
		if (lwp.l_stype != ST_NONE) {
			fprintf(fp,"  %c-%08x",ch,lwp.l_syncp);
		} else
			fprintf(fp,"            ");

		if (lwp.l_name != NULL) {
			char buf[20];
			readmem((long)lwp.l_name, 1, -1, (char *)buf,
				sizeof buf, "lwp name");
			fprintf(fp, "  %.20s\n", buf);
		} else
			fprintf(fp, "\n");

		if (full) {
			fprintf(fp,"\tLWP signal state:"
				" pend: %08x %08x mask: %08x %08x\n",
				lwp.l_sigs.ks_sigbits[0],
				lwp.l_sigs.ks_sigbits[1],
				lwp.l_sigheld.ks_sigbits[0],
				lwp.l_sigheld.ks_sigbits[1]);

			if (lwp.l_auditp) {
				fprintf(fp,"\tLWP Audit Structure:\n");
				pr_alwp(lwp.l_auditp, tokename);
			}
		}
	}
}

/* get arguments for defproc function */
int
getdefproc()
{
	int c;
	int proc = Cur_proc;
	int lwp = Cur_lwp;
	int reset = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"cw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		if (reset)
			longjmp(syn,0);
		proc = valproc(args[optind++]);
		if (args[optind])
			lwp = strcon(args[optind++],'d');
		else
			lwp = (proc == -1)? Cur_eng: 0;
		if (args[optind])
			longjmp(syn,0);
	}
	prdefproc(proc, lwp, reset);
}

int
getdeflwp()
{
	int c;
	int lwp = Cur_lwp;
	int reset = 0;

	optind = 1;
	while ((c = getopt(argcnt,args,"cw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	reset = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		if (reset)
			longjmp(syn,0);
		lwp = strcon(args[optind++],'d');
		if (args[optind])
			longjmp(syn,0);
	}
	prdefproc(Cur_proc, lwp, reset);
}

/* print results of defproc function */
int
prdefproc(proc,lwp,reset)
int proc,lwp,reset;
{
	extern int onproc_engid; /* set as side-effect of getublock() */

	if (reset)
		get_context(Cur_eng);
	else if (proc == -1) {			/* "idle" */
		check_engid(lwp);
		Cur_proc = -1;
		Sav_eng = Cur_eng = Cur_lwp = lwp;
	}
	else {
		if (lwp < 0)	/* no particular upper limit */
			error("lwp %d is out of range\n", lwp);
		Cur_proc = proc;
		Cur_lwp  = lwp;
		if (!active
		&&  getublock(proc, lwp, B_FALSE)
		&&  onproc_engid != -1)
			Sav_eng = Cur_eng = onproc_engid;
	}
	pr_context();
}

#ifdef notdef

/* print the high resolution timers */
int
gethrt()
{
	int c;
	static vaddr_t Hrt;
	timer_t hrtbuf;
	timer_t *hrp;
	extern timer_t hrt_active;
	char *prhralarm_hdg = "    PROCP       TIME     INTERVAL    CMD    EID     PREVIOUS     NEXT    \n\n";


	optind = 1;
	while((c=getopt(argcnt, args,"w:")) != EOF) {
		switch(c) {
			case 'w'  :	redirect();
					break;
			default   :	longjmp(syn,0);
		}
	}
	if(args[optind])
		longjmp(syn,0);

	Hrt = symfindval("hrt_active");
	readmem(Hrt, 1, -1, &hrtbuf, sizeof hrtbuf, "hrt_active");

	fprintf(fp, "%s", prhralarm_hdg);
	hrp=hrtbuf.hrt_next;
	for (; hrp != (timer_t *)Hrt; hrp=hrtbuf.hrt_next) {
		readmem((long)hrp, 1, -1, (char *)&hrtbuf,
			sizeof hrtbuf, "high resolution alarms");
		fprintf(fp, "%12x %7d%11d %7d %13x %10x\n",
			hrtbuf.hrt_proc,
			hrtbuf.hrt_time,
			hrtbuf.hrt_int,
			hrtbuf.hrt_cmd,
			hrtbuf.hrt_prev,
			hrtbuf.hrt_next);
	}
}

#endif

int
readsid(sessp)
	struct sess *sessp;
{
	struct sess s;

	if (!sessp)
		return 0;	/* don't stop proc or stream -f display */
	readmem((vaddr_t)sessp,1,-1,&s,sizeof(struct sess),"session structure");
	return readpid(s.s_sidp);
}

int
readpid(pidp)
	struct pid *pidp;
{
	struct pid p;

	if (!pidp)
		return 0;	/* don't stop proc or stream -f display */
	readmem((vaddr_t)pidp,1,-1,&p,sizeof(struct pid),"pid structure");
	return p.pid_id;
}

static	void
pr_privs(lst, tokename)
cred_t	lst;
int	tokename;
{
	extern	void	prt_symbprvs();

	if (tokename) {
		prt_symbprvs("\n\t\tworking: ", lst.cr_wkgpriv);
		prt_symbprvs("\t\tmaximum: ", lst.cr_maxpriv);
	}
	else {
		fprintf(fp, "working: %.8x", lst.cr_wkgpriv);
		fprintf(fp, "\tmaximum: %.8x", lst.cr_maxpriv);
		fprintf(fp, "\n");
	}
}

static	void
pr_aproc(ap, tokename)
aproc_t	*ap;
int tokename;
{
	aproc_t aproc;
	adtpath_t cdp;
	adtpath_t rdp;
	adtkemask_t emask;
	char *evtstr, *cwdp, *crdp;	

	readmem((long)ap, 1, -1,
    		(char *)&aproc, sizeof(aproc_t),"aproc structure");

	if (aproc.a_cdp) {
		readmem((long)aproc.a_cdp,1,-1,(char *)&cdp,sizeof cdp, 
			"audit current working directory structure");
		cwdp = readmem((vaddr_t)cdp.a_path, 1, -1, NULL, cdp.a_len + 1,
			"audit current working directory string");
		cwdp[cdp.a_len] = '\0';
		fprintf(fp,"\t\tCurrent Working Directory: %s\n",cwdp); 
		cr_free(cwdp);
	} else
		fprintf(fp,"\t\tCurrent Working Directory: ?\n"); 

	if (aproc.a_rdp) {
		readmem((long)aproc.a_rdp,1,-1,(char *)&rdp,sizeof rdp, 
			"audit current root directory structure");
		crdp = readmem((vaddr_t)rdp.a_path, 1, -1, NULL, rdp.a_len + 1,
			"audit current root directory string");
		crdp[rdp.a_len] = '\0';
		fprintf(fp,"\t\tCurrent Root Directory: %s\n",crdp); 
		cr_free(crdp);
	}

	readmem((long)aproc.a_emask, 1, -1,(char *)&emask, sizeof emask,
		"audit process event mask structure");

	if (tokename) {
		if (aproc.a_flags > 0) 
			fprintf(fp,"\t\tFlags: %s %s\n",
			aproc.a_flags & AOFF ? "AOFF," : "",
			aproc.a_flags & AEXEMPT ? "AEXEMPT," : "");
		else
			fprintf(fp,"\t\tFlags: NONE\n");
		if ((evtstr=(char *)cnvemask(&emask.ad_emask)) != NULL)
			fprintf(fp,"\t\tProcess Event Mask:\t%s\n",evtstr);
		else
			fprintf(fp,"\t\tProcess Event Mask:\tEVTSTR=NULL ERROR\n");
		if ((evtstr=(char *)cnvemask(&aproc.a_useremask)) != NULL)
			fprintf(fp,"\t\tUser    Event Mask:\t%s\n",evtstr);
		else
			fprintf(fp,"\t\tUser    Event Mask:\tEVTSTR=NULL ERROR\n");
	}else  {
		fprintf(fp,"\t\tFlags: %u\n",aproc.a_flags);
		fprintf(fp,"\t\tProcess Event Mask: %08lx %08lx %08lx %08lx\n",
			emask.ad_emask[0], emask.ad_emask[1],
			emask.ad_emask[2], emask.ad_emask[3]);
		fprintf(fp,"\t\t                    %08lx %08lx %08lx %08lx\n",
			emask.ad_emask[4], emask.ad_emask[5],
			emask.ad_emask[6], emask.ad_emask[7]);
		fprintf(fp,"\t\tUser    Event Mask: %08lx %08lx %08lx %08lx\n",
			aproc.a_useremask[0], aproc.a_useremask[1],
			aproc.a_useremask[2], aproc.a_useremask[3]);
		fprintf(fp,"\t\t                    %08lx %08lx %08lx %08lx\n",
			aproc.a_useremask[4], aproc.a_useremask[5],
			aproc.a_useremask[6], aproc.a_useremask[7]);
	}
}

static	void
pr_alwp(alwpp, tokename)
alwp_t	*alwpp;
int tokename;
{
	alwp_t alwp;
	adtpath_t cdp;
	adtpath_t rdp;
	adtkemask_t emask;
	register struct tm * td;
	char *evtstr, *cwdp, *crdp;	
	ulong_t	seqnum;

	readmem((long)alwpp, 1, -1,
    		(char *)&alwp, sizeof(alwp_t),"alwp structure");

	readmem((long)alwp.al_emask, 1, -1,(char *)&emask, sizeof emask,
		"lwp audit event mask structure");

	if (tokename) {
		if (alwp.al_flags > 0) 
			fprintf(fp,"\t\tFlags: %s %s\n",
			alwp.al_flags & AUDITME ? "AUDITME," : "",
			alwp.al_flags & ADT_NEEDPATH ? "ADT_NEEDPATH," : "",
			alwp.al_flags & ADT_OBJCREATE ? "ADT_OBJCREATE," : "");
		else
			fprintf(fp,"\t\tFlags: NONE\n");
		if ((evtstr=(char *)prevtnam(alwp.al_event))!=NULL)
			fprintf(fp,"\tEvent Number: %s\n", evtstr);
		else
			fprintf(fp,"\tEvent Number NULL: ERROR\n");
		seqnum=EXTRACTSEQ(alwp.al_seqnum);
		fprintf(fp,"\tRecord Sequence Number: %u\n",seqnum);
		if (alwp.al_time.tv_sec == 0)
			fprintf(fp,"\tStarting time of Event: 0\n");
		else {
			td = gmtime((const time_t *)&alwp.al_time);
			fprintf(fp,"\tStarting time of Event: %02d/%02d/%02d%  02d:%02d:%02d GMT\n",
				td->tm_mon + 1, td->tm_mday, td->tm_year,
				td->tm_hour, td->tm_min, td->tm_sec);
		}
	
		if (emask.ad_refcnt == 1) {
		    if ((evtstr=(char *)cnvemask(&emask.ad_emask)) != NULL)
			fprintf(fp,"\t\tLWP     Event Mask:\t%s\n",evtstr);
		    else
			fprintf(fp,"\t\tLWP     Event Mask:\tEVTSTR=NULL ERROR\n");
		}
	} else  {
		fprintf(fp,"\t\tFlags: %u\n",alwp.al_flags);
		fprintf(fp,"\t\tEvent Number: %u\n",alwp.al_event);
		seqnum=EXTRACTSEQ(alwp.al_seqnum);
		fprintf(fp,"\t\tRecord Sequence Number: %u\n",seqnum);
		fprintf(fp,"\t\tStarting time of Event: %ld\n",alwp.al_time);
		if (emask.ad_refcnt == 1) {
		  fprintf(fp,"\t\tLWP     Event Mask: %08lx %08lx %08lx %08lx\n",
			emask.ad_emask[0], emask.ad_emask[1],
			emask.ad_emask[2], emask.ad_emask[3]);
		  fprintf(fp,"\t\t                    %08lx %08lx %08lx %08lx\n",
			emask.ad_emask[4], emask.ad_emask[5],
			emask.ad_emask[6], emask.ad_emask[7]);
		}
	}

	if (alwp.al_cdp) {
		readmem((long)alwp.al_cdp,1,-1,(char *)&cdp,sizeof cdp, 
			"audit current working directory structure");
		if (cdp.a_ref == 1) {
			cwdp = readmem((vaddr_t)cdp.a_path,
				1, -1, NULL, cdp.a_len + 1,
				"audit current working directory string");
			cwdp[cdp.a_len] = '\0';
			fprintf(fp,"\t\tCurrent Working Directory: %s\n",cwdp); 
			cr_free(cwdp);
		} 
	}

	if (alwp.al_rdp) {
		readmem((long)alwp.al_rdp,1,-1,(char *)&rdp,sizeof rdp, 
			"audit current root directory structure");
		crdp = readmem((vaddr_t)rdp.a_path, 1, -1, NULL, rdp.a_len + 1,
			"audit current root directory string");
		crdp[rdp.a_len] = '\0';
		fprintf(fp,"\t\tCurrent Root Directory: %s\n",crdp); 
		cr_free(crdp);
	}

	fprintf(fp,"\t\tBufp:   0x%-8x\n",alwp.al_bufp);
	fprintf(fp,"\t\tObufp:  0x%-8x\n",alwp.al_obufp);
	fprintf(fp,"\t\tFrecp:  0x%-8x\n",alwp.al_frec1p);
	fprintf(fp,"\t\tCmpcnt:   %8d\n",alwp.al_cmpcnt);
}
