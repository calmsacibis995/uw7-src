#ident	"@(#)crash:common/cmd/crash/stream.c	1.3.2.1"

/*
 * This file contains code for the crash functions:  stream, queue, 
 * strstat, linkblk, qrun.
 */

#include <sys/param.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/list.h>
#include <sys/plocal.h>
#include <sys/vfs.h>
#define _KERNEL
#include <sys/poll.h>
#undef _KERNEL
#include <sys/stream.h>
#define DEBUG
#include <sys/strsubr.h>
#undef DEBUG
#include <sys/strstat.h>
#include <sys/stropts.h>
#include "crash.h"

static vaddr_t Strinfop, Qsvc, Plocalmet;
vaddr_t prstream();
vaddr_t prlinkblk();


/* get arguments for stream function */
int
getstream()
{
	int full = 0;
	vaddr_t addr;
	int c;
	struct strinfo strinfo[NDYNAMIC];
 	char *heading = " ADDRESS      WRQ     IOCB    VNODE  PUSHCNT  RERR/WERR FLAG\n";

	optind = 1;
	while((c = getopt(argcnt,args,"fw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (!Strinfop)
		Strinfop = symfindval("Strinfo");
	readmem(Strinfop,1,-1,&strinfo,sizeof(strinfo),"Strinfo");
	fprintf(fp,"STREAM TABLE SIZE = %d\n",strinfo[DYN_STREAM].st_cnt);
	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) {
		do {
			addr = strcon(args[optind++], 'h');
			(void) prstream(full,addr,heading);
		}while(args[optind]);
	}
	else {
		addr = (vaddr_t)strinfo[DYN_STREAM].st_head;
		while(addr) {
			addr = prstream(full,addr,heading);
		}
	}
}

/* print streams table */
vaddr_t
prstream(full,addr,heading)
int full;
vaddr_t addr;
char *heading;
{
	struct shinfo strm;
	register struct stdata *stp;
	struct strevent evbuf;
	struct strevent *next;
	struct pollhead ph;
	struct polldat *pdp;
	struct polldat pdat;
	struct lwp lwp;
	struct sess sess;
	struct proc proc;
	struct queue *wrq;

	readmem(addr,1,-1,(char *)&strm,sizeof(strm),"streams table slot");
	stp = (struct stdata *) &strm;
	if(full)
		fprintf(fp,"%s",heading);
	fprintf(fp,"%8x %8x %8x %8x %6d    %d/%d",addr,stp->sd_wrq,stp->sd_strtab,
	    stp->sd_vnode,stp->sd_pushcnt,stp->sd_rerror,stp->sd_werror);
	fprintf(fp,"       %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		((stp->sd_flag & IOCWAIT) ? "iocw " : ""),
		((stp->sd_flag & RSLEEP) ? "rslp " : ""),
		((stp->sd_flag & WSLEEP) ? "wslp " : ""),
		((stp->sd_flag & STRPRI) ? "pri " : ""),
		((stp->sd_flag & STRHUP) ? "hup " : ""),
		((stp->sd_flag & STRTOHUP) ? "thup " : ""),
		((stp->sd_flag & STWOPEN) ? "stwo " : ""),
		((stp->sd_flag & STPLEX) ? "plex " : ""),
		((stp->sd_flag & STRISTTY) ? "istty " : ""),
		((stp->sd_flag & RMSGDIS) ? "mdis " : ""),
		((stp->sd_flag & RMSGNODIS) ? "mnds " : ""),
		((stp->sd_flag & STRDERR) ? "rerr " : ""),
		((stp->sd_flag & STWRERR) ? "werr " : ""),
		((stp->sd_flag & STRTIME) ? "sttm " : ""),
		((stp->sd_flag & UPF) ? "upf " : ""),
		((stp->sd_flag & STR3TIME) ? "s3tm " : ""),
		((stp->sd_flag & UPBLOCK) ? "upb " : ""),
		((stp->sd_flag & SNDMREAD) ? "mrd " : ""),
		((stp->sd_flag & OLDNDELAY) ? "ondel " : ""),
		((stp->sd_flag & STRSNDZERO) ? "sndz " : ""),
		((stp->sd_flag & STRTOSTOP) ? "tstp " : ""),
		((stp->sd_flag & RDPROTDAT) ? "pdat " : ""),
		((stp->sd_flag & RDPROTDIS) ? "pdis " : ""),
		((stp->sd_flag & STRMOUNT) ? "mnt " : ""),
		((stp->sd_flag & STRDELIM) ? "delim " : ""),
		((stp->sd_flag & STRSVSIG) ? "svsig " : ""),
		((stp->sd_flag & FLUSHWAIT) ? "flshw " : ""),
		((stp->sd_flag & STRLOOP) ? "loop " : ""),
		((stp->sd_flag & STRPOLL) ? "spoll " : ""),
		((stp->sd_flag & STRNOCTTY) ? "noctty " : ""),
		((stp->sd_flag & STRSIGPIPE) ? "spip " : ""));

	if(full) {
		if (stp->sd_sessp)
			readmem((vaddr_t)stp->sd_sessp,1,-1,
				&sess,sizeof(sess),"session structure");
		fprintf(fp,"\t     SID     PGID   IOCBLK    IOCID  IOCWAIT\n");
		fprintf(fp,"\t%8d %8d %8x %8d %8d\n",
			readpid(sess.s_sidp),
			readpid(stp->sd_pgidp),
			stp->sd_iocblk,stp->sd_iocid,stp->sd_iocwait);
		fprintf(fp,"\t  WOFF     MARK CLOSTIME\n");
		fprintf(fp,"\t%6d %8x %8d\n",stp->sd_wroff,stp->sd_mark,
		    stp->sd_closetime);
		fprintf(fp,"\tSIGFLAGS:  %s%s%s%s%s%s%s%s%s\n",
			((stp->sd_sigflags & S_INPUT) ? " input" : ""),
			((stp->sd_sigflags & S_HIPRI) ? " hipri" : ""),
			((stp->sd_sigflags & S_OUTPUT) ? " output" : ""),
			((stp->sd_sigflags & S_RDNORM) ? " rdnorm" : ""),
			((stp->sd_sigflags & S_RDBAND) ? " rdband" : ""),
			((stp->sd_sigflags & S_WRBAND) ? " wrband" : ""),
			((stp->sd_sigflags & S_ERROR) ? " err" : ""),
			((stp->sd_sigflags & S_HANGUP) ? " hup" : ""),
			((stp->sd_sigflags & S_MSG) ? " msg" : ""));
		fprintf(fp,"\tSIGLIST:\n");
		next = stp->sd_siglist;
		while(next) {
			readmem((long)next,1,-1,(char *)&evbuf,
				sizeof evbuf,"stream event buffer");
			readlwp(evbuf.se_lwpp, &lwp);
			readmem((long)lwp.l_procp,1,-1,(char *)&proc,
				sizeof proc,"proc");
			fprintf(fp,"\t\tPROC:  %3d  LWP:  %3d  %s%s%s%s%s%s%s%s%s\n",
				proc_to_slot(lwp.l_procp),
				lwp_to_slot(evbuf.se_lwpp, &proc),
				((evbuf.se_events & S_INPUT) ? " input" : ""),
				((evbuf.se_events & S_HIPRI) ? " hipri" : ""),
				((evbuf.se_events & S_OUTPUT) ? " output" : ""),
				((evbuf.se_events & S_RDNORM) ? " rdnorm" : ""),
				((evbuf.se_events & S_RDBAND) ? " rdband" : ""),
				((evbuf.se_events & S_WRBAND) ? " wrband" : ""),
				((evbuf.se_events & S_ERROR) ? " err" : ""),
				((evbuf.se_events & S_HANGUP) ? " hup" : ""),
				((evbuf.se_events & S_MSG) ? " msg" : ""));
			next = evbuf.se_next;	
		}
		readmem((long)stp->sd_pollist,1,-1,(char *)&ph,
			sizeof ph,"pollhead");
		fprintf(fp,"\tPOLLFLAGS:  %s%s%s%s%s%s\n",
			((ph.ph_events & POLLIN) ? " in" : ""),
			((ph.ph_events & POLLPRI) ? " pri" : ""),
			((ph.ph_events & POLLOUT) ? " out" : ""),
			((ph.ph_events & POLLRDNORM) ? " rdnorm" : ""),
			((ph.ph_events & POLLRDBAND) ? " rdband" : ""),
			((ph.ph_events & POLLWRBAND) ? " wrband" : ""));
		fprintf(fp,"\tPOLLIST:\n");
		pdp = ph.ph_list;
		while(pdp) {
			readmem((long)pdp,1,-1,(char *)&pdat,
				sizeof pdat,"poll data buffer");
			fprintf(fp,"\t\tFUNC:  %#.8x   ARG:  %#.8x   %s%s%s%s%s%s\n",
				pdat.pd_fn, pdat.pd_arg,
				((pdat.pd_events & POLLIN) ? " in" : ""),
				((pdat.pd_events & POLLPRI) ? " pri" : ""),
				((pdat.pd_events & POLLOUT) ? " out" : ""),
				((pdat.pd_events & POLLRDNORM) ? " rdnorm" : ""),
				((pdat.pd_events & POLLRDBAND) ? " rdband" : ""),
				((pdat.pd_events & POLLWRBAND) ? " wrband" : ""));
			pdp = pdat.pd_next;
		}
		fprintf(fp,"\n");
	}
	return (vaddr_t)strm.sh_next;
}

char *qheading = " QUEADDR     INFO              NEXT     LINK      PTR     RCNT FLAG\n";

/* get arguments for queue function */
int
getqueue()
{
	int c;
	vaddr_t addr;
	struct queinfo *qip;
	struct queinfo queinfo;
	struct strinfo strinfo[NDYNAMIC];
	int full = 0, style = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"fsw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'f' :	full = 1;
					break;
			case 's' :      ++style;
					/* -ss for read side, -sss for both */
					break;
			default  :	longjmp(syn,0);
		}
	}

	if ((style && full) || style > 3 || (style > 1 && args[optind]))
		longjmp(syn,0);
		
	if (!style || !args[optind]) {
		if (!Strinfop)
			Strinfop = symfindval("Strinfo");
		readmem(Strinfop,1,-1,&strinfo,sizeof(strinfo),"Strinfo");
		fprintf(fp,"QUEUE TABLE SIZE = %d\n",strinfo[DYN_QUEUE].st_cnt);
	}

	if(args[optind]) {
		if (!full && !style)
			fprintf(fp, qheading);
		do {
			addr = strcon(args[optind++], 'h');
			prqueue((queue_t *)addr,NULL,full,style);
		}while(args[optind]);
	}
	else {
		if (style)
			fprintf(fp,"STREAMS CONFIGURATION (%s)\n",
				(style == 1)? "write side": (
				(style == 2)? "read side": "both sides"));
		else if (!full)
			fprintf(fp, qheading);
		qip = (struct queinfo *) strinfo[DYN_QUEUE].st_head;
		while(qip) {
			readmem((vaddr_t)qip,1,-1,
				&queinfo,sizeof(queinfo),"queue");
			prqueue(&qip->qu_rqueue,&queinfo.qu_rqueue,full,style);
			prqueue(&qip->qu_wqueue,&queinfo.qu_wqueue,full,style);
			qip = queinfo.qu_next;
		}
	}
}

static char nomn[] = "-";

static char *
getmodname(queue_t *qp)
{
	static char mn[21];
	struct qinit qi;
	struct module_info mi;
	phaddr_t paddr;
	char *cp;
	/*
	 * These threads are so volatile, avoid collapse in readmem(virtual)
	 */
	if ((paddr = cr_vtop((vaddr_t)qp->q_qinfo, -1, NULL, B_FALSE)) == -1)
		return nomn;
	readmem64(paddr,0,-1,&qi,sizeof(qi),"q_qinfo");
	if ((paddr = cr_vtop((vaddr_t)qi.qi_minfo, -1, NULL, B_FALSE)) == -1)
		return nomn;
	readmem64(paddr,0,-1,&mi,sizeof(mi),"qi_minfo");
	if ((paddr = cr_vtop((vaddr_t)mi.mi_idname, -1, NULL, B_FALSE)) == -1)
		return nomn;
	readmem64(paddr,0,-1,mn,sizeof(mn)-1,"mi_idname");
	cp = mn;
	while (isgraph(*cp))
		++cp;
	if (*cp || cp == mn)
		return nomn;
	return mn;
}

/* print queue table */
int
prqueue(qaddr,qp,full,style)
queue_t *qaddr, *qp;
int full,style;
{
	queue_t *qnext;
	queue_t q[2];
	char *mn, *spacer;
	int qstyle, checkheadmn;

	if(qp == NULL) {
		checkheadmn = 0;
		readmem((vaddr_t)qaddr,1,-1,qp=q,sizeof(queue_t),"queue");
		if(style) {
			if (!(qp->q_flag & QUSE))
				return;
			if (qp->q_flag & QREADR)
				style = 2, qp->q_next = qaddr;
			else
				style = 1, qp->q_next = qaddr - 1;
			while (qp->q_next != NULL) {
				readmem((vaddr_t)(qaddr = qp->q_next),1,-1,
					q,sizeof(q),"queue");
				if ((qp->q_flag&(QUSE|QREADR)) != (QUSE|QREADR)
				||  ((qp+1)->q_flag&(QUSE|QREADR)) != QUSE)
					return;
			}
			if (style & 1)
				qaddr++, qp++;
		}
	}
	else
		checkheadmn = style;

	if(style) {
		qstyle = (qp->q_flag & QREADR)? 2: 1;
		if (!(style & qstyle)
		||  !(qp->q_flag & QUSE))
			return;
		/* heads have null read side next */
		if ((qp+qstyle-2)->q_next != NULL)
			return;
		if ((mn = getmodname(qp)) == nomn)
			return;
		if (qstyle & 1) {	/* write side */
			if (checkheadmn
			&&  strcmp(mn,"NWstrWriteHead") != 0
			&&  strcmp(mn,"strwhead") != 0)
				return;
			spacer = " >";
			qnext = qp->q_next;
		}
		else {			/* read side */
			if (checkheadmn
			&&  strcmp(mn,"NWstrReadHead") != 0
			&&  strcmp(mn,"strrhead") != 0)
				return;
			spacer = " <";
			qnext = (qp+1)->q_next;
		}
		fprintf(fp," ");
		while (1) {
			if (qnext == NULL)
				spacer = "\n";
			fprintf(fp,"%-8s(%8x)%s",mn,qaddr,spacer);
			if (qnext == NULL)
				break;
			readmem((vaddr_t)qnext,1,-1,
				qp = q+1,sizeof(queue_t),"queue");
			if (qstyle & 1)
				qaddr = qnext;
			else {	/* beware: "conn" seems to be twisted */
				if (q[1].q_flag & QREADR)
					qnext += 2;
				readmem((vaddr_t)(qnext-1),1,-1,
					qp = q,sizeof(queue_t),"queue");
				if (qp->q_next != qaddr) {
					fprintf(fp,"-\n");
					break;
				}
				qaddr = qnext-1;
			}
			qnext = q[1].q_next;
			if (qp->q_flag & QUSE)
				mn = getmodname(qp);
			else
				mn = nomn, qnext = NULL;
		}
	}

	if(!style){
		if (full)
			fprintf(fp,qheading);
		fprintf(fp,"%8x %8x %-8s ",
			qaddr,qp->q_qinfo,getmodname(qp));
		if (qp->q_next)
			fprintf(fp,"%8x ",qp->q_next);
		else fprintf(fp,"       - ");

		if (qp->q_link)
			fprintf(fp,"%8x ",qp->q_link);

		else fprintf(fp,"       - ");
		fprintf(fp,"%8x",qp->q_ptr);
		fprintf(fp," %8d ",qp->q_count);
		fprintf(fp,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
			((qp->q_svcflag & QENAB) ? "en " : ""),
			((qp->q_svcflag & QSVCBUSY) ? "bu " : ""),
			((qp->q_flag & QWANTR) ? "wr " : ""),
			((qp->q_flag & QWANTW) ? "ww " : ""),
			((qp->q_flag & QFULL) ? "fl " : ""),
			((qp->q_flag & QREADR) ? "rr " : ""),
			((qp->q_flag & QUSE) ? "us " : ""),
			((qp->q_flag & QUP) ? "up " : ""),
			((qp->q_flag & QBACK) ? "bk " : ""),
			((qp->q_flag & QPROCSON) ? "on " : ""),
			((qp->q_flag & QTOENAB) ? "te " : ""),
			((qp->q_flag & QFREEZE) ? "fr " : ""),
			((qp->q_flag & QBOUND) ? "bd " : ""),
			((qp->q_flag & QDEFCNT) ? "dc " : ""),
			((qp->q_flag & QNOENB) ? "ne " : ""));
		if (!full)
			return;
		fprintf(fp,"\t    HEAD     TAIL     MINP     MAXP     HIWT     LOWT BAND BANDADDR\n");
		if (qp->q_first)
			fprintf(fp,"\t%8x ",qp->q_first);
		else fprintf(fp,"\t       - ");
		if (qp->q_last)
			fprintf(fp,"%8x ",qp->q_last);
		else fprintf(fp,"       - ");
		fprintf(fp,"%8d %8d %8d %8d ",
			qp->q_minpsz,
			qp->q_maxpsz,
			qp->q_hiwat,
			qp->q_lowat);
		fprintf(fp," %3d %8x\n\n",
			qp->q_nband, qp->q_bandp);
	}
}


/* get arguments for qrun function */
int
getqrun()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind])
		longjmp(syn,0);
	prqrun();
}

/* print qrun information */
int
prqrun()
{
	struct qsvc qsvc;
	queue_t que, *q;
	struct plocal l;

	if(!Qsvc)
		Qsvc = symfindval("qsvc");
	readmem(Qsvc,1,-1,&qsvc,sizeof(qsvc),"qsvc");
	fprintf(fp,"Queue slots scheduled for service (global): ");
	q = qsvc.qs_head;
	while (q) {
		fprintf(fp,"%8x ",q);
		readmem((vaddr_t)q,1,-1,&que,sizeof(que),"que");
		q = que.q_link;
	}
	fprintf(fp,"\n");
	readplocal(Cur_eng, &l);
	fprintf(fp,"Queue slots scheduled for service (processor %d): ",
		l.eng_num);
	q = l.qsvc.qs_head;
	while (q) {
		fprintf(fp,"%8x ",q);
		readmem((vaddr_t)q,1,-1,&que,sizeof(que),"que");
		q = que.q_link;
	}
	fprintf(fp,"\n");
}

/* get arguments for strstat function */
int
getstrstat()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind])
		longjmp(syn,0);
	prstrstat();
}

#define	NTYPES	6
#define NMETS	(MET_TOTAL+1)

static char *msrtype[NTYPES] =
{
	"streams",
	"queues",
	"message blocks",
	"message triplets",
	"link blocks",
	"stream events",
};

/* print strstat table */
prstrstat()
{
	unsigned long totmsr[NTYPES][NMETS];
	unsigned long engmsr[NTYPES][NMETS];
	int i, j;

	if (!Plocalmet)
		Plocalmet = symfindval("lm");
	memset(totmsr, 0, sizeof(totmsr));

	for (Cur_eng = 0; Cur_eng < Nengine; Cur_eng++) {
		readmem((vaddr_t)
			(&((struct plocalmet *)Plocalmet)->metp_str_resrc),
			1, -1, &engmsr, sizeof(engmsr), "lm.metp_str_resrc");
		for (i = 0; i < NTYPES; i++)
			for (j = 0; j < NMETS; j++)
				totmsr[i][j] += engmsr[i][j];
	}
	Cur_eng = Sav_eng;

	fprintf(fp,"ITEM                  INUSE      TOTAL       FAIL\n");
	for (i = 0; i < NTYPES; i++)
		fprintf(fp,"%-16s %10u %10u %10u\n", msrtype[i],
		totmsr[i][MET_INUSE],totmsr[i][MET_TOTAL],totmsr[i][MET_FAIL]);
}

/* get arguments for linkblk function */
int
getlinkblk()
{
	vaddr_t addr;
	int c;
	struct strinfo strinfo[NDYNAMIC];

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (!Strinfop)
		Strinfop = symfindval("Strinfo");
	readmem(Strinfop,1,-1,&strinfo,sizeof(strinfo),"Strinfo");
	fprintf(fp,"LINKBLK TABLE SIZE = %d\n",strinfo[DYN_LINKBLK].st_cnt);
	fprintf(fp,"LBLKADDR     QTOP     QBOT FILEADDR    MUXID\n");
	if(args[optind]) {
		do {
			addr = strcon(args[optind++], 'h');
			(void) prlinkblk(addr);
		}while(args[optind]);
	}
	else {
		addr = (vaddr_t)strinfo[DYN_LINKBLK].st_head;
		while(addr) {
			addr = prlinkblk(addr);
		}
	}
}

/* print linkblk table */
vaddr_t
prlinkblk(addr)
vaddr_t addr;
{
	struct linkinfo linkbuf;
	struct linkinfo *lp;

	readmem(addr,1,-1,(char *)&linkbuf,sizeof(linkbuf),"linkblk table");
	lp = &linkbuf;
	fprintf(fp,"%8x", addr);
	fprintf(fp," %8x",lp->li_lblk.l_qtop);
	fprintf(fp," %8x",lp->li_lblk.l_qbot);
	fprintf(fp," %8x",lp->li_fpdown);
	if (lp->li_lblk.l_qbot)
		fprintf(fp," %8d\n",lp->li_lblk.l_index);
	else
		fprintf(fp,"        -\n");
	return (vaddr_t)lp->li_next;
}
