#ident	"@(#)trpt.c	1.2"
#ident	"$Header$"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

/*
 *	System V STREAMS TCP - Release 3.0
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#define PRUREQUESTS

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#define TCPSTATES
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#define	TCPTIMERS
#include <netinet/tcp_timer.h>
#define TLI_PRIMS
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#define	TANAMES
#include <netinet/tcp_debug.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <errno.h>
#include <nlist.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ksym.h>

n_time	ntime;
int	sflag;
int	tflag;
int	jflag;
int	aflag;
int	follow;
int	numeric();

#define GETINFO(index,buf,buflen,name)	ginfo(index,buf,buflen,name,MIOC_READKSYM)
#define GETIINFO(index,buf,buflen,name)	ginfo(index,buf,buflen,name,MIOC_IREADKSYM)

struct nlist nl[] = {
	{ "tcp_ndebug" },
	{ "tcp_debug" },
	{ "tcp_debx" },
	{ 0 }

};

struct	tcp_debug *tcp_debug;
#define	MAXPCBS	50
caddr_t	tcp_pcbs[MAXPCBS];
int	tcp_ndebug = 0;
int 	tcp_debx;
boolean_t memflg = B_FALSE;

#define	USAGE \
	"USAGE: %s [-a] [-f] [-j] [-s] [-t] [-p pcb addr] [system [core]]\n"

main(argc, argv)
	int argc;
	char **argv;
{
	int i, mask = 0, npcbs = 0, trbufsz;
	char *system = "/unix", *core = "/dev/kmem";
	extern char *optarg;
	extern int optind;

	while ( (i = getopt(argc, argv, "afjstp:")) != -1 ) {

		switch(i) {

		case 'a':	aflag++; break;
		case 'f':	follow++; break;
		case 'j':	jflag++; break;
		case 's':	sflag++; break;
		case 't':	tflag++; break;
		case 'p':
			if ( npcbs >= MAXPCBS ) {
				fprintf(stderr,
				"-p: too many pcb's specified\n");
				exit(1);
			}
			strcpy(&tcp_pcbs[npcbs++], optarg);
			break;
		default:
			fprintf(stderr, USAGE, argv[0]);
			exit(1);
		}
	}


	if (optind < argc) {
		system = argv[optind++];
		mask++;
	}
	if (optind < argc) {
		core = argv[optind++];
		memflg = B_TRUE;
		mask++;
	}
	if (optind != argc) {
		fprintf(stderr, USAGE, argv[0]);
		exit(1);
	}

	if(memflg) {
		if (!readata(system))
			(void) nlist(system, nl);
		if (nl[0].n_value == 0) {
			fprintf(stderr, "trpt: %s: no namelist\n", system);
			exit(1);
		}
		writedata();
	}

	(void) close(0);
	vaddrinit(core, "trpt", 0);
	if (memflg && mask) {
		nl[0].n_value &= 0x7fffffff;
		nl[1].n_value &= 0x7fffffff;
	}
	GETINFO(0, &tcp_ndebug, sizeof(tcp_ndebug), "tcp_ndebug");
	if (tcp_ndebug == 0) {
		fprintf(stderr, "trpt: kernel not configured for TCP trace\n");
		exit(3);
	}
	GETINFO(2, &tcp_debx, sizeof(tcp_debx), "tcp_debx");
	printf("tcp_debx=%d\n",tcp_debx);
	trbufsz = tcp_ndebug * sizeof(struct tcp_debug);
	if ((tcp_debug = (struct tcp_debug *) malloc(trbufsz)) == 0) {
		fprintf(stderr, "trpt: no memory\n");
		exit(3);
	}
#ifdef DEBUG_IS_PTR
	GETIINFO(1, tcp_debug, trbufsz, "tcp_debug - pointer");
#else
	GETINFO(1, tcp_debug, trbufsz, "tcp_debug");
#endif

	/*
	 * If no control blocks have been specified, figure
	 * out how many distinct one we have and summarize
	 * them in tcp_pcbs for sorting the trace records
	 * below.
	 */
	if (npcbs == 0) {
		for (i = 0; i < tcp_ndebug; i++) {
			register int j;
			register struct tcp_debug *td = &tcp_debug[i];

			if (td->td_tcb == 0)
				continue;
			for (j = 0; j < npcbs; j++)
				if (tcp_pcbs[j] == td->td_tcb)
					break;
			if (j >= npcbs)
				tcp_pcbs[npcbs++] = td->td_tcb;
		}
	}
	qsort(tcp_pcbs, npcbs, sizeof (caddr_t), numeric);
	if (jflag) {
		char *cp = "";

		for (i = 0; i < npcbs; i++) {
			printf("%s%x", cp, tcp_pcbs[i]);
			cp = ", ";
		}
		if (*cp)
			putchar('\n');
		exit(0);
	}
	for (i = 0; i < npcbs; i++) {
		printf("\n%x:\n", tcp_pcbs[i]);
		dotrace(tcp_pcbs[i]);
	}
	exit(0);
}

dotrace(tcpcb)
	register caddr_t tcpcb;
{
	register int i;
	register struct tcp_debug *td;
	int prev_debx = tcp_debx;

again:
	if (--tcp_debx < 0)
		tcp_debx = tcp_ndebug - 1;
	for (i = prev_debx % tcp_ndebug; i < tcp_ndebug; i++) {
		td = &tcp_debug[i];
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, td->td_tcb, &td->td_cb,
		    &td->td_ti, td->td_req);
		if (i == tcp_debx)
			goto done;
	}
	for (i = 0; i <= tcp_debx % tcp_ndebug; i++) {
		td = &tcp_debug[i];
		if (tcpcb && td->td_tcb != tcpcb)
			continue;
		ntime = ntohl(td->td_time);
		tcp_trace(td->td_act, td->td_ostate, td->td_tcb, &td->td_cb,
		    &td->td_ti, td->td_req);
	}
done:
	if (follow) {
	    prev_debx = tcp_debx + 1;
	    if (prev_debx >= tcp_ndebug)
		prev_debx = 0;
	    do {
		sleep(1);
		(void) lseek(0, nl[1].n_value, 0);
		if (read(0, &tcp_debx, sizeof(tcp_debx)) != sizeof(tcp_debx)) {
			fprintf(stderr, "trpt: "); perror("tcp_debx");
			exit(3);
		}
	    } while (tcp_debx == prev_debx);
	    (void) lseek(0, nl[0].n_value, 0);
	    if (read(0, tcp_debug, sizeof(tcp_debug)) != sizeof(tcp_debug)) {
		    fprintf(stderr, "trpt: "); perror("tcp_debug");
		    exit(3);
	    }
	    goto again;
	}
}

/*
 * Tcp debug routines
 */
tcp_trace(act, ostate, atp, tp, ti, req)
	short act, ostate;
	struct tcpcb *atp, *tp;
	struct tcpiphdr *ti;
	int req;
{
	tcp_seq seq, ack;
	int len, flags, win, timer;
	char *cp;

	ptime(ntime);
	printf("%s:%s ", tcpstates[ostate], tanames[act]);
	switch (act) {

	case TA_INPUT:
	case TA_OUTPUT:
	case TA_DROP:
		if (aflag) {
			printf("(src=%s,%d, ", inet_ntoa(ti->ti_src),
				ntohs(ti->ti_sport));
			printf("dst=%s,%d)", inet_ntoa(ti->ti_dst),
				ntohs(ti->ti_dport));
		}
		seq = ti->ti_seq;
		ack = ti->ti_ack;
		len = ti->ti_len;
		win = ti->ti_win;
		if (act == TA_OUTPUT) {
			seq = ntohl(seq);
			ack = ntohl(ack);
			len = ntohs(len);
			win = ntohs(win);
		}
		if (act == TA_OUTPUT)
			len -= sizeof (struct tcphdr);
		if (len)
			printf("[%x..%x)", seq, seq+len);
		else
			printf("%x", seq);
		printf("@%x", ack);
		if (win)
			printf("(win=%x)", win);
		flags = ti->ti_flags;
		if (flags) {
			char *cp = "<";
#define pf(flag,name) { if (ti->ti_flags & flag) { \
	printf("%s%s", cp, name); cp = ","; } }
			pf(TH_SYN,"TH_SYN"); pf(TH_ACK,"TH_ACK");
			pf(TH_FIN,"TH_FIN"); pf(TH_RST,"TH_RST");
			pf(TH_PUSH,"TH_PUSH"); pf(TH_URG,"TH_URG");
			printf(">");
		}
		break;

	case TA_USER:
		printf("%s",tli_primitives[req & 0xFF]);
		break;

	case TA_TIMER:
		printf("<%s>", tcptimers[req]);
		break;
	}
	printf(" -> %s", tcpstates[tp->t_state]);
	/* print out internal state of tp !?! */
	printf("\n");
	if (sflag) {
		printf("\trcv_nxt %x rcv_wnd %x snd_una %x snd_nxt %x snd_max %x\n",
		    tp->rcv_nxt, tp->rcv_wnd, tp->snd_una, tp->snd_nxt,
		    tp->snd_max);
		printf("\tsnd_wl1 %x snd_wl2 %x snd_wnd %x\n", tp->snd_wl1,
		    tp->snd_wl2, tp->snd_wnd);
		printf("\tidle%x rtt %x rtseq %x srtt %x\n",
		tp->t_idle, tp->t_rtt, tp->t_rtseq, tp->t_srtt);
	}
	/* print out timers? */
	if (tflag) {
		char *cp = "\t";
		register int i;

		for (i = 0; i < TCPT_NTIMERS; i++) {
			if (tp->t_timer[i] == 0)
				continue;
			printf("%s%s=%d", cp, tcptimers[i], tp->t_timer[i]);
			if (i == TCPT_REXMT)
				printf(" (t_rxtshft=%d)", tp->t_rxtshift);
			cp = ", ";
		}
		if (*cp != '\t')
			putchar('\n');
	}
}

ptime(ms)
	int ms;
{

	printf("%05d ", (ms/10) % 1000);
}

numeric(c1, c2)
	caddr_t *c1, *c2;
{
	
	return (*c1 - *c2);
}

int             memfd;
#define vtop(x,y)	x

vaddrinit(mem, name, flag)
	char           *mem;
	char           *name;
{
	if ((memfd = open(mem, 0)) < 0)
		error("%s: can't open %s\n", name, mem);
}

seekmem(addr, mode, proc)
	long            addr;
	int             mode, proc;
{
	long            paddr;

	if (mode)
		paddr = vtop(addr, proc);
	else
		paddr = addr;
	if (paddr == -1)
		error("%x is an invalid address\n", addr);
	if (lseek(memfd, paddr, 0) == -1)
		error("seek error on address %x\n", addr);
}

/* lseek and read */
int
readmem(addr, mode, proc, buffer, size, name)
	long            addr;
	int             mode, proc;
	char           *buffer;
	unsigned        size;
	char           *name;
{
	seekmem(addr, mode, proc);
	if (read(memfd, buffer, size) != size)
		error("read error on %s\n", name);
}

int
ginfo(index, buf, buflen, name, cmd)
int index;
char *name;
size_t buflen;
void * buf;
int cmd;
{
	struct mioc_rksym rks;
	if(memflg) {
		if(nl[index].n_value == 0)
			return(-1);
		(void) readmem(nl[index].n_value,1,0,buf,buflen,name);
		return(0);
	}
	else {
		rks.mirk_symname = nl[index].n_name;
		rks.mirk_buf = buf;
		rks.mirk_buflen = buflen;
		return(ioctl(memfd,cmd,&rks));
	}
}

error(string, arg1, arg2, arg3)
	char           *string;
	int             arg1, arg2, arg3;
{
	fprintf(stderr, string, arg1, arg2, arg3);
	exit(1);
}
/*
** hack to avoid nlist'ing all the time
** save data to a file, a la PS...
*/
 
#define TRPTDATA "/etc/trpt_data"
  
readata(kfile)
	char    *kfile;
{
	int f;
	int cnt;
	struct stat sbuf1, sbuf2;

	if (stat(TRPTDATA, &sbuf1) < 0
	    || stat(kfile, &sbuf2) < 0
	    || sbuf1.st_mtime <= sbuf2.st_mtime
	    || sbuf1.st_mtime <= sbuf2.st_ctime)
		return(0);

	if ((f = open(TRPTDATA, O_RDONLY)) < 0)
		return 0;

	cnt = read(f, nl, sizeof(nl));

	if (cnt != sizeof(nl)) {
                (void) close(f);
                (void) unlink(TRPTDATA);
                return 0;
        }

        (void) close(f);
        return 1;
}

writedata()
{

        int f;
        int cnt;

        umask(02);
        unlink(TRPTDATA);
        if ((f = open(TRPTDATA, O_WRONLY | O_CREAT | O_EXCL , 0664)) < 0)
                return 0;

        cnt = write(f, nl, sizeof(nl));

        if (cnt != sizeof(nl)) {
                (void) close(f);
                (void) unlink(TRPTDATA);
                return 0;
        }

        (void) close(f);
        return 1;
}

