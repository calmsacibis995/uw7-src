#ident	"@(#)crash:i386/cmd/crash/u.c	1.2.2.2"

/*
 * This file contains code for the crash functions:  user, pcb, stack, trace.
 */

#include <sys/param.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <memory.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/immu.h>
#include <sys/tss.h>
#include <sys/seg.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/signal.h>
#include <sys/cred.h>
#include <sys/vmparam.h>
#include <sys/lwp.h>
#include <sys/resource.h>
#include <sys/engine.h>
#include "crash.h"

#define STAX MMU_PAGESIZE		/* stack extension size */

struct user *ubp;			/* uarea (user_t) pointer */
static char *ublock;			/* ublock (stack) pointer */
static int ublock_to_uarea;		/* offset of uarea from ublock */
static vaddr_t Ueng;
int onproc_engid;

extern struct panic_data panic_data;	/* panic data structure */
extern int active;			/* active system flag */
extern int debugmode;

vaddr_t trytraces(vaddr_t, vaddr_t, int);
vaddr_t pripcb(vaddr_t, vaddr_t, int);

char *rlimitstring[] = {
	"cpu time",
	"file size",
	"data size",
	"stack size",
	"coredump size",
	"file descriptors",
	"address space"
};


/* read ublock into buffer */

vaddr_t
getublock(int proc, int lwp, boolean_t multiple)
{
	static int prev_proc = -2;
	static int prev_lwp = -2;
	static vaddr_t kublock;
	proc_t *procp, procbuf;
	lwp_t *lwpp, lwpbuf;
	int stax;

	if (debugmode > 1)
		fprintf(stderr,"getublock: %x %x\n",proc,lwp);

	if (proc < -1 || proc >= vbuf.v_proc) {
		prerrmes("process %d is out of range 0-%d\n",
			proc, vbuf.v_proc - 1);
		return 0;
	}

	if (!active && proc == prev_proc && lwp == prev_lwp)
		return kublock;

	if (ubp == NULL) {
		/*
		 * Allocate ublock buf, with page in front for stack extension
		 * Use ueng to find uarea offset, instead of UBLOCK_TO_UAREA()
		 * macro: I don't know if a crash compiled without DEBUG gets
		 * this far on a kernel compiled with DEBUG, or vice versa,
		 * but UBLOCK_TO_UAREA() would certainly throw it off course.
		 */
		Ueng = symfindval("ueng");
		ublock_to_uarea = Ueng & (MMU_PAGESIZE-1);
		cr_unfree(ublock = cr_malloc((USIZE+1)*MMU_PAGESIZE, "ublock"));
		ublock += MMU_PAGESIZE;
		ubp = (struct user *)(ublock + ublock_to_uarea);
	}

	if (proc >= 0) {
		procp = slot_to_proc(proc);

		if (procp == NULL) {
			if (!multiple)
				prerrmes("process %d does not exist\n",proc);
			return 0;
		}

		readmem((vaddr_t)procp,1,-1,
			&procbuf, sizeof procbuf, "proc structure");

		if (procbuf.p_nlwp == 0) {
			prwarning(multiple,"process %d is a zombie\n",proc);
			return 0;
		}

		if(!(procbuf.p_flag & P_LOAD)) {
			prwarning(multiple,"process %d is swapped out\n",proc);
			return 0;
		}

		if ((lwpp = slot_to_lwp(lwp,&procbuf)) == NULL) {
			if (!multiple)
				prerrmes("lwp %d does not exist\n",lwp);
			return 0;
		}

		readlwp(lwpp, &lwpbuf);
	}
	else {
		/*
		 * Set some lwpbuf fields to suit below
		 */
		lwpbuf.l_up = (struct user *)Ueng;
		lwpbuf.l_stat = SONPROC;
		lwpbuf.l_ubinfo.ub_stack_mem = NULL;
		if ((lwpbuf.l_eng = id_to_eng(lwp)) == (engine_t *)(-1)) {
			if (!multiple)
				prerrmes("engine %d is out of range 0-%d\n",
					lwp, Nengine - 1);
			return 0;
		}
		Cur_eng = lwp;
	}

	/* read in the ublock pages */

	memset(ublock - STAX, 0, STAX);
	prev_proc = prev_lwp = -2;		/* lest read failure */
	kublock = (vaddr_t)lwpbuf.l_up - ublock_to_uarea;
	readmem(kublock, 1, -1, ublock, USIZE*MMU_PAGESIZE, "ublock");

	stax = kublock - ubp->u_kcontext.kctx_esp;
	if (stax > 0 && stax <= STAX) {
		if (lwpbuf.l_ubinfo.ub_stack_mem)
		    stax = kublock - (vaddr_t)lwpbuf.l_ubinfo.ub_stack;
		else
		    lwpbuf.l_ubinfo.ub_stack_mem = (void *)(kublock - stax);
		readmem((vaddr_t)lwpbuf.l_ubinfo.ub_stack_mem, 1, -1,
			ublock - stax, stax, "stack extension");
	}

	onproc_engid = (lwpbuf.l_stat == SONPROC)?
		eng_to_id(lwpbuf.l_eng): -1;
	Cur_eng = Sav_eng;
	prev_proc = proc;
	prev_lwp  = lwp;
	return kublock;
}

/* get arguments for user function */
int
getuser()
{
	int proc, lwp;
	int all = 0;
	vaddr_t kublock;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'e' :	all = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(args[optind]) {
		if (all)
			longjmp(syn,0);
		proc = valproc(args[optind++]);
		if (args[optind])
			lwp = strcon(args[optind++],'d');
		else
			lwp = (proc == -1)? Cur_eng: 0;
		if (args[optind])
			longjmp(syn,0);
		if (kublock = getublock(proc, lwp, B_FALSE))
			pruser(proc, lwp, kublock);
	}
	else if (all) {
		if (active)
			makeslottab();
		for (proc = -1, lwp = 0; proc < vbuf.v_proc; ) {
			if (kublock = getublock(proc, lwp, B_TRUE))
				pruser(proc, lwp++, kublock);
			else
				lwp = 0, proc++;
		}
	}
	else if (kublock = getublock(Cur_proc, Cur_lwp, B_FALSE))
		pruser(Cur_proc, Cur_lwp, kublock);
}

/* print ublock */
int
pruser(proc,lwp,kublock)
int proc,lwp;
vaddr_t kublock;
{
	register int i;
	struct rlimits rlimits;

	if (proc == -1)
		fprintf(fp,"USER STRUCTURE FOR IDLE CONTEXT OF ENGINE %d:\n",
			lwp);
	else
		fprintf(fp,"USER STRUCTURE FOR PROCESS %d LWP %d:\n",proc,lwp);

	fprintf(fp, "up = %08x  u_procp = %08x  u_lwpp = %08x\n",
		kublock + ublock_to_uarea, ubp->u_procp, ubp->u_lwpp);

	fprintf(fp,"u_acflag: %s%s\n",
		ubp->u_acflag & AFORK ? "fork" : "exec",
		ubp->u_acflag & ASU ? " su-user" : "");

	fprintf(fp,"u_syscall = %d with arguments ( %x %x %x %x %x %x )\n",
		ubp->u_syscall, ubp->u_arg[0], ubp->u_arg[1], ubp->u_arg[2],
		ubp->u_arg[3], ubp->u_arg[4], ubp->u_arg[5] );

	fprintf(fp, "u_sigflag = %x\n", ubp->u_sigflag);
	fprintf(fp, "u_stkbase = %x\n", ubp->u_stkbase);
	fprintf(fp, "u_stksize = %x\n", ubp->u_stksize);

	fprintf(fp,"u_ior = %x %x\t", ubp->u_ior.dl_lop, ubp->u_ior.dl_hop);
	fprintf(fp,"u_iow = %x %x\t", ubp->u_iow.dl_lop, ubp->u_iow.dl_hop);
	fprintf(fp,"u_ioch = %x %x\n", ubp->u_ioch.dl_lop, ubp->u_ioch.dl_hop);

	fprintf(fp, "u_privatedatap = %x\n", ubp->u_privatedatap);

	fprintf(fp, "u_kse_ptep = %x\n", ubp->u_kse_ptep);

#ifdef DEBUG
	fprintf(fp, "u_debugflags = %x\n", ubp->u_debugflags);
#endif

	if (ubp->u_rlimits == NULL) {
		fprintf(fp,"\n");
		return;
	}
	fprintf(fp,"RESOURCE LIMITS:\n");
	readmem((vaddr_t)ubp->u_rlimits,1,-1,
		&rlimits, sizeof(struct rlimits),"rlimits structure");
	for (i = 0; i < RLIM_NLIMITS; i++) {
		if (rlimitstring[i] == 0)
			continue;
		fprintf(fp,"\t%s: ", rlimitstring[i]);
		if (rlimits.rl_limits[i].rlim_cur == RLIM_INFINITY)
			fprintf(fp,"unlimited/");
		else
			fprintf(fp,"%d/", rlimits.rl_limits[i].rlim_cur);
		if (rlimits.rl_limits[i].rlim_max == RLIM_INFINITY)
			fprintf(fp,"unlimited\n");
		else
			fprintf(fp,"%d\n", rlimits.rl_limits[i].rlim_max);
	}
	fprintf(fp,"\n");
}

/* get arguments for pcb function */
int
getpcb()
{
	int proc = Cur_proc;
	int lwp = Cur_lwp;
	int all = 0;
	int first = 1;
	int types = 0;
	char type = 'k';
	vaddr_t kublock, addr;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"eiukw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'e' :	all = 1;
					break;
			case 'i' :	type = 'i';
					types++;
					break;
			case 'u' :	type = 'u';
					types++;
					break;
			case 'k' :	type = 'k';
					types++;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(types > 1)
		error("only one type may be specified: k u or i\n");
	if(type == 'i') {
		if(!args[optind])
			longjmp(syn,0);
		addr = strcon(args[optind++],'h');
		if (args[optind])
			longjmp(syn,0);
		/*
		 * The addr may well not be for the current proc/lwp,
		 * but if it is then we can get stack extension right.
		 */
		kublock = getublock(Cur_proc, Cur_lwp, B_TRUE);
		while ((addr = pripcb(addr, kublock, first)) && all) {
			fprintf(fp,"\n");
			first = 0;
		}
		return;
	}
	if(args[optind]) {
		if (all)
			longjmp(syn,0);
		proc = valproc(args[optind++]);
		if (args[optind])
			lwp = strcon(args[optind++],'d');
		else
			lwp = (proc == -1)? Cur_eng: 0;
		if (args[optind])
			longjmp(syn,0);
		if (kublock = getublock(proc, lwp, B_FALSE))
			prpcb(proc, lwp, kublock, type, B_FALSE);
	}
	else if (all) {
		if (active)
			makeslottab();
		for (proc = -1, lwp = 0; proc < vbuf.v_proc; ) {
			if ((kublock = getublock(proc, lwp, B_TRUE))
			&&  prpcb(proc, lwp++, kublock, type, B_TRUE))
				fprintf(fp,"\n");
			else
				lwp = 0, proc++;
		}
	}
	else if (kublock = getublock(Cur_proc, Cur_lwp, B_FALSE))
		prpcb(Cur_proc, Cur_lwp, kublock, type, B_FALSE);
}

/* print user or kernel pcb */
int
prpcb(proc,lwp,kublock,type,multiple)
int proc,lwp;
vaddr_t kublock;
char type;
boolean_t multiple;
{
	vaddr_t kaddr;
	struct as *junk;
	unsigned long *r0p;
	char *kernuser;

	kernuser = (type == 'k')? "KERNEL": "USER";

	if(!multiple)
		;	/* omit heading since it wasn't shown before */
	else if (proc == -1) {
		fprintf(fp,"%s CONTEXT SAVED FOR IDLE CONTEXT OF ENGINE %d:\n",
			kernuser, lwp);
		/* for completeness: but kernel's usually 0s and user's none */
	}
	else
		fprintf(fp,"%s CONTEXT SAVED FOR PROCESS %d LWP %d:\n",
			kernuser, proc, lwp);

	switch(type) {
	case 'k' :
		kaddr = kublock + (vaddr_t)&ubp->u_kcontext - (vaddr_t)ublock;
		printregs(kaddr, NULL, &ubp->u_kcontext);
		break;
	case 'u' :
		if ((kaddr = (vaddr_t)ubp->u_ar0) == 0) {
		/* we're only interested in the appropriate error message */
			if (readas(proc, &junk, multiple) == NULL)
				return 0;	/* jump to next proc */
			prwarning(multiple,"u_ar0 is not set\n");
			break;
		}
		r0p = (unsigned long *)(kaddr +
			(vaddr_t)ublock - (vaddr_t)kublock);
		if (&r0p[T_TRAPNO] < (unsigned long *)ublock
		||  &r0p[T_SS] > (unsigned long *)ubp - 1) {
			prwarning(multiple,"u_ar0 points outside the stack\n");
			break;
		}
		printregs(kaddr, r0p, &ubp->u_kcontext);
		break;
	}

	return 1;
}

#define T_ESP	(-4)			/* from esp in trap's push-all */
#define T_NREG	(1+T_SS-T_TRAPNO)

static void
getregs(vaddr_t kaddr, unsigned long *r0p, char *inublock)
{
	if (inublock)
		memcpy(r0p+T_TRAPNO, inublock+kaddr+T_TRAPNO*sizeof(long),
			T_NREG*sizeof(long));
	else
		readmem(kaddr+T_TRAPNO*sizeof(long), 1, -1,
			r0p+T_TRAPNO, T_NREG*sizeof(long), "register set");
}

static boolean_t
goodregs(vaddr_t kaddr, unsigned long *r0p, vaddr_t u_ar0)
{
	/* mask below may need updating for new processors */
	if ((r0p[T_EFL] & 0xffc0802a) != 0x2)
		return B_FALSE;
	if (r0p[T_CS] == KCSSEL
	&&  r0p[T_DS] == KDSSEL && r0p[T_ES] == KDSSEL) {
		r0p[T_SS] = KDSSEL;
		r0p[T_UESP] = kaddr + T_UESP*sizeof(long);
		return B_TRUE;
	}
	return (kaddr == u_ar0);
}

/* print interrupt pcb */
vaddr_t
pripcb(vaddr_t kaddr, vaddr_t kublock, int first)
{
	register unsigned long *r0p;
	unsigned long Regs[T_NREG];
	kcontext_t kc;
	vaddr_t minaddr, maxaddr, u_ar0;
	char *inublock;

	kaddr &= ~3;
	if (kaddr >= kublock - STAX && kaddr <= kublock + ublock_to_uarea) {
		minaddr = kublock - STAX;
		maxaddr = kublock + ublock_to_uarea;
		u_ar0 = (vaddr_t)ubp->u_ar0;
		kc.kctx_fs = ubp->u_kcontext.kctx_fs;
		kc.kctx_gs = ubp->u_kcontext.kctx_gs;
		inublock = ublock - kublock;	/* flag and adjustment */
	}
	else {
		minaddr = kaddr & ~(MMU_PAGESIZE-1);
		maxaddr = minaddr + ublock_to_uarea;
		if (kaddr > maxaddr) {
			/*
			 * This only makes sense on a stack extension page,
			 * in which case we don't know where to get more info
			 */
			u_ar0 = 0;
			kc.kctx_fs = kc.kctx_gs = 0xffff;
			maxaddr = minaddr + MMU_PAGESIZE;
		}
		else {
			/*
			 * Unless we make assumptions about user cs,ds,es,ss,
			 * it's hard to recognize an interrupt frame from
			 * user mode; but such a frame should be at the same
			 * address as earlier traps, so look up that address.
			 */
			readmem(minaddr +
				(vaddr_t)&ubp->u_ar0 - (vaddr_t)ublock,
				1,-1,&u_ar0,sizeof(u_ar0),"registers pointer");
			/*
			 * Since fs and gs really belong to user context, but
			 * are saved only in kernel context, get them from last
			 * saved kernel context; but they may be out of date
			 * (like all this info), or we may have been given a
			 * stack extension addr and look in the wrong place.
			 */
			readmem(minaddr +
				(vaddr_t)&ubp->u_kcontext - (vaddr_t)ublock,
				1,-1,&kc,sizeof(kc),"kernel context");
		}
		inublock = NULL;
	}

	minaddr += (-T_TRAPNO)*sizeof(long);
	maxaddr -= (1+T_SS)*sizeof(long);
	if (u_ar0 >= minaddr && u_ar0 <= maxaddr)
		maxaddr = u_ar0;
	if (kaddr < minaddr)
		kaddr = minaddr;
	if (kaddr > maxaddr) {
		kaddr = maxaddr;
		first = 0;
	}
	else if (first)
		first = T_EFL;

	/*
	 * We want the user to give the r0ptr of the stackframe,
	 * but that isn't easy to recognize, so accept any address
	 * from r0ptr (EAX ptr) up to EFL ptr; and if those fail,
	 * then search up the stack.  (If the user is so familiar
	 * with stackframes that they know the exact address to
	 * give, then they don't really need "pcb -i" at all.)
	 */
	getregs(kaddr, r0p = &Regs[T_EAX-T_TRAPNO], inublock);
	while (first-- && kaddr > minaddr
	&&     !goodregs(kaddr, r0p, u_ar0))
		getregs(kaddr -= sizeof(long), r0p, inublock);
	while (!goodregs(kaddr, r0p, u_ar0)) {
		if (kaddr >= maxaddr)
			return 0;
		getregs(kaddr += sizeof(long), r0p, inublock);
	}

	printregs(kaddr, r0p, &kc);

	if (r0p[T_CS] != KCSSEL)
		return 0;
	if ((kaddr += (T_EFL-T_EBX)*sizeof(long)) > maxaddr)
		return 0;
	return kaddr;
}

void
printregs(vaddr_t r0addr, unsigned long *r0p, struct kcontext *kcp)
{
	unsigned long Regs[T_NREG];
	char *unknownreg = "-";
	char *name;
	char type;

	if (r0p == NULL) {
		r0p = &Regs[T_EAX-T_TRAPNO];

		r0p[T_EAX] = kcp->kctx_eax;
		r0p[T_EBX] = kcp->kctx_ebx;
		r0p[T_ECX] = kcp->kctx_ecx;
		r0p[T_EDX] = kcp->kctx_edx;

		r0p[T_ESI] = kcp->kctx_esi;
		r0p[T_EDI] = kcp->kctx_edi;
		r0p[T_EBP] = kcp->kctx_ebp;
		r0p[T_UESP]= kcp->kctx_esp;

		r0p[T_CS] = KCSSEL;
		r0p[T_DS] = KDSSEL;
		r0p[T_ES] = KDSSEL;
		r0p[T_SS] = KDSSEL;

		r0p[T_EIP] = kcp->kctx_eip;
		type = 'k';
	}
	else if (r0p[T_ESP] == r0addr + T_ES*sizeof(long))
		type = 't';
	else
		type = 'i';

	if (r0p[T_CS] == KCSSEL) {
		name = db_sym_off(r0p[T_EIP]);
		if (*--name)		/* it found a symbol */
			*name = ' ';
	}
	else
		name = "";

    if (type == 'k')
	fprintf(fp,"  kcp:%8x trp:%8s efl:%8s eip:%8x%s\n",
		r0addr,unknownreg,unknownreg,r0p[T_EIP],name);
    else if (type == 'i')
	fprintf(fp,"  r0p:%8x trp:%8s efl:%8x eip:%8x%s\n",
		r0addr,unknownreg,r0p[T_EFL],r0p[T_EIP],name);
    else
	fprintf(fp,"  r0p:%8x trp:%8x efl:%8x eip:%8x%s\n",
		r0addr,r0p[T_TRAPNO],r0p[T_EFL],r0p[T_EIP],name);
	/* the trp shown is not necessarily a trapno, may be useful address */

    if (type == 'i')
	fprintf(fp,"  eax:%8x ebx:%8s ecx:%8x edx:%8x cs:%4x ds:%4x fs:%4x\n",
		r0p[T_EAX],unknownreg,r0p[T_ECX],r0p[T_EDX],
		r0p[T_CS]&0xffff,r0p[T_DS]&0xffff,kcp->kctx_fs&0xffff);
    else
	fprintf(fp,"  eax:%8x ebx:%8x ecx:%8x edx:%8x cs:%4x ds:%4x fs:%4x\n",
		r0p[T_EAX],r0p[T_EBX],r0p[T_ECX],r0p[T_EDX],
		r0p[T_CS]&0xffff,r0p[T_DS]&0xffff,kcp->kctx_fs&0xffff);

    if (type == 'i')
	fprintf(fp,"  esi:%8s edi:%8s ebp:%8s esp:%8x ss:%4x es:%4x gs:%4x\n",
		unknownreg,unknownreg,unknownreg,r0p[T_UESP],
		r0p[T_SS]&0xffff,r0p[T_ES]&0xffff,kcp->kctx_gs&0xffff);
    else
	fprintf(fp,"  esi:%8x edi:%8x ebp:%8x esp:%8x ss:%4x es:%4x gs:%4x\n",
		r0p[T_ESI],r0p[T_EDI],r0p[T_EBP],r0p[T_UESP],
		r0p[T_SS]&0xffff,r0p[T_ES]&0xffff,kcp->kctx_gs&0xffff);
}

/* get arguments for stack function */
int
getstack()
{
	int proc, lwp;
	int all = 0;
	int c;
	vaddr_t kublock;
	vaddr_t trysp = UNKNOWN;

	optind = 1;
	while((c = getopt(argcnt,args,"ew:t:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 't' :	trysp = strcon(optarg,'h');
					break;
			case 'e' :	all = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(args[optind]) {
		if (all)
			longjmp(syn,0);
		proc = valproc(args[optind++]);
		if (args[optind])
			lwp = strcon(args[optind++],'d');
		else
			lwp = (proc == -1)? Cur_eng: 0;
		if (args[optind])
			longjmp(syn,0);
		if (kublock = getublock(proc, lwp, B_FALSE))
			prstack(proc, lwp, kublock, trysp);
	}
	else if (all) {
		if (trysp != UNKNOWN)
			longjmp(syn,0);
		if (active)
			makeslottab();
		for (proc = -1, lwp = 0; proc < vbuf.v_proc; ) {
			if (kublock = getublock(proc, lwp, B_TRUE)) {
				prstack(proc, lwp++, kublock, UNKNOWN);
				fprintf(fp, "\n");
			}
			else
				lwp = 0, proc++;
		}
	}
	else if (kublock = getublock(Cur_proc, Cur_lwp, B_FALSE))
		prstack(Cur_proc, Cur_lwp, kublock, trysp);
}

/* print kernel stack */
int
prstack(proc, lwp, kublock, trysp)
int proc, lwp;
vaddr_t kublock, trysp;
{
	vaddr_t stklo,stkhi,savsp,cursp;
	unsigned long *stkptr;
	int pad, len;
	char *s;

	if (proc == -1)
		fprintf(fp,"KERNEL STACK FOR IDLE CONTEXT OF ENGINE %d:\n",lwp);
	else
		fprintf(fp,"KERNEL STACK FOR PROCESS %d LWP %d:\n",proc,lwp);

	if (trysp != UNKNOWN)
		savsp = trysp & ~3;
	else if (!active || onproc_engid == -1)
		savsp = ubp->u_kcontext.kctx_esp;
	else
		savsp = UNKNOWN;

	stklo = kublock - STAX;
	stkhi = kublock + ublock_to_uarea - sizeof(long);

	if (savsp > stkhi || savsp < stklo) {
		if (trysp != UNKNOWN)
			error("trysp %x is not in range %x-%x\n",
				trysp, stklo, stkhi);
		st_init(ublock - kublock);
		savsp = trytraces(stklo, stkhi, (proc == -1)? lwp: -1);
	}

	cursp = savsp & ~15;
	stkptr = (unsigned long *)(ublock + (cursp - kublock));

	for (; cursp < stkhi; stkptr++, cursp += 4) {
		if ((cursp & 15) == 0) {
			fprintf(fp,"\n%08x:", cursp);
			pad = 3;
		}
		if (cursp < savsp)
			s = "        ";
		else
			s = db_sym_off(*stkptr);
		len = strlen(s);
		if (len > 10) {
			len = 10;
			if (pad > 2)
				pad = 2;
		}
		while (pad > 0) {
			putc(' ',fp);
			--pad;
		}
		fputs(s,fp);
		pad = 12 - len;
	}

	fprintf(fp,"\n");
}

/* get arguments for trace function */
int
gettrace()
{
	int proc, lwp;
	int all = 0;
	int c;
	vaddr_t kublock;
	vaddr_t trysp = UNKNOWN;

	optind = 1;
	while((c = getopt(argcnt,args,"ew:t:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 't' :	trysp = strcon(optarg,'h');
					break;
			case 'e' :	all = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(args[optind]) {
		if (all)
			longjmp(syn,0);
		proc = valproc(args[optind++]);
		if (args[optind])
			lwp = strcon(args[optind++],'d');
		else
			lwp = (proc == -1)? Cur_eng: 0;
		if (args[optind])
			longjmp(syn,0);
		if (kublock = getublock(proc, lwp, B_FALSE))
			prtrace(proc, lwp, kublock, trysp);
	}
	else if (all) {
		if (trysp != UNKNOWN)
			longjmp(syn,0);
		if (active)
			makeslottab();
		for (proc = -1, lwp = 0; proc < vbuf.v_proc; ) {
			if (kublock = getublock(proc, lwp, B_TRUE)) {
				prtrace(proc, lwp++, kublock, UNKNOWN);
				fprintf(fp,"\n");
			}
			else
				lwp = 0, proc++;
		}
	}
	else if (kublock = getublock(Cur_proc, Cur_lwp, B_FALSE))
		prtrace(Cur_proc, Cur_lwp, kublock, trysp);
}

/* print kernel trace */
int
prtrace(proc, lwp, kublock, trysp)
int proc, lwp;
vaddr_t kublock, trysp;
{
	vaddr_t stklo,stkhi,savsp,savip,savbp;
	void crash_prf();

	if (proc == -1)
		fprintf(fp,"STACK TRACE FOR IDLE CONTEXT OF ENGINE %d:\n",lwp);
	else
		fprintf(fp,"STACK TRACE FOR PROCESS %d LWP %d:\n",proc,lwp);

	if (trysp != UNKNOWN) {
		savsp = trysp & ~3;
		savip = UNKNOWN;
		savbp = UNKNOWN;
	}
	else if (!active || onproc_engid == -1) {
		savsp = ubp->u_kcontext.kctx_esp;
		savip = ubp->u_kcontext.kctx_eip;
		savbp = ubp->u_kcontext.kctx_ebp;
	}
	else
		savsp = UNKNOWN;

	stklo = kublock - STAX;
	stkhi = kublock + ublock_to_uarea - sizeof(long);
	st_init(ublock - kublock);

	if (savsp > stkhi || savsp < stklo) {
		if (trysp != UNKNOWN)
			error("trysp %x is not in range %x-%x\n",
				trysp, stklo, stkhi);
		savsp = trytraces(stklo, stkhi, (proc == -1)? lwp: -1);
		savip = UNKNOWN;
		savbp = UNKNOWN;
	}

	stacktrace(crash_prf, stkhi, savsp, savbp, savip, 0, 0, 0, NULL);
}

static void
crash_prf(fmt,a,b,c,d,e,f)
char *fmt;
{
	fprintf(fp,fmt,a,b,c,d,e,f);
}

static size_t prevlen, currlen;
static boolean_t prevgood, currgood, stoploop;
static char *previous, *current;

static void
buf_prf(fmt,a,b,c,d,e,f)
char *fmt;
{
	char buf[80], *bp;
	char *recurrent;
	size_t len;

	/* don't waste time on alignments */
	if (fmt[0] == '.' && fmt[1] == '\0')
		return;
	/* guard against buffer overflow from long symbols */
	if (fmt[0] == '%' && fmt[1] == 's' && fmt[2] == '\0')
		len = strlen(bp = (char *)a);
	else
		len = sprintf(bp = buf, fmt,a,b,c,d,e,f);
	if (len == 0)
		return;
	if (current == NULL)
		currgood = B_TRUE;
	if (bp[0] == '<' && bp[1] == '<')
		currgood = B_FALSE;
	if (prevgood && !currgood) {
		stoploop = B_TRUE;
		return;
	}
	recurrent = cr_malloc(currlen + len, "trace buffer");
	memcpy(recurrent, current, currlen);
	memcpy(recurrent + currlen, bp, len);
	cr_free(current);
	current = recurrent;
	currlen += len;
}

vaddr_t
trytraces(vaddr_t stklo, vaddr_t stkhi, int idleng)
{
	unsigned long *stkptr;
	vaddr_t trysp;

	if (idleng != -1) {
		engine_t engine;
		readengine(idleng, &engine);
		if (!(engine.e_flags & E_OFFLINE))
			idleng = -1;
	}
	if (idleng == -1)
		fprintf(fp, "<<stack pointer is volatile>>\n");

	trysp = stkhi;
	stkptr = (unsigned long *)(ublock + (stkhi - stklo - STAX));
	current = previous = NULL;
	stoploop = prevgood = B_FALSE;
	prevlen = 0;

	while ((trysp -= sizeof(long)) >= stklo) {
		if (!istextval(*--stkptr))
			continue;
		currlen = 0;
		currgood = B_FALSE;
		stacktrace(buf_prf, stkhi, trysp,
			UNKNOWN, UNKNOWN, 0, 0, 0, NULL);
		if (stoploop)
			break;
		if ((currgood && !prevgood)
		||  (currlen >= prevlen &&
		memcmp(current+(currlen-prevlen), previous, prevlen) == 0)) {
			cr_free(previous);
			previous = current;
			prevlen = currlen;
			prevgood = currgood;
			current = NULL;
		}
		else
			break;
	}

	if (previous)		/* go from just above the diverging sp */
		trysp += sizeof(long);
	else			/* force "<<no stack frames found>>" */
		trysp = stkhi;
	cr_free(current);
	cr_free(previous);
	return trysp;
}
