#ident	"@(#)crash:i386/cmd/crash/util.c	1.1.4.2"

/*
 * This file contains code for utilities used by more than one crash function.
 */

#include <signal.h>
#include <stdio.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/vmparam.h>
#include <sys/var.h>
#include <sys/proc.h>
#include <sys/plocal.h>
#include <vm/page.h>
#include <sys/clock.h>
#include <sys/cglocal.h>
#include <sys/cg.h>
#include <sys/pid.h>
#include <unistd.h>
#include <memory.h>
#include <malloc.h>
#include <time.h>
#include "crash.h"

#ifdef UNIPROC
Error: crash is always built MP but understands UP
#endif

extern paddr_t KPD[];

jmp_buf jmp;
FILE *fp, *rp;

void (*intsig)(int);	/* 0 until initialized: then SIG_IGN or sigint */
void (*pipesig)(int);

struct panic_data panic_data;
static int vital_init_done = 0;

size_t sizeof_engine = sizeof(struct engine);
size_t sizeof_lwp    = sizeof(struct lwp);
size_t sizeof_plocal = sizeof(struct plocal);
size_t sizeof_cglocal = sizeof(struct cglocal);

int active = 1;
int opipe = 0;
int hexmode = 0;
int debugmode = 0;

global_cginfo_t	global_cginfo;
int Nengine = 1;
int Ncg = 1;
int Sav_eng = 0;
int Cur_eng = 0;
int Cur_cg = 0;
int Cur_proc = -1;
int Cur_lwp = 0;
int UniProc = 0;

char mallerr[] = "cannot malloc(%u) for %s\n";
static void **mallocs;
static int mallocslen;
static int nextmalloc;

/* malloc temporary space and note it to be freed before next command */
void *
cr_malloc(size_t size, char *name)
{
	register int i;
	size_t resize;
	int increment;
	void *malloced, **newmallocs;

	if (size == 0)
		return NULL;
	if ((malloced = malloc(size)) == NULL)
		error(mallerr, size, name);
	for (i = nextmalloc; i < mallocslen; i++)
		if (!mallocs[i])
			break;
	if (i == mallocslen) {
		increment = mallocslen? mallocslen: 4;
		resize = (mallocslen + increment) * sizeof(void *);
		newmallocs = realloc(mallocs, resize);
		if (newmallocs == NULL) {
			free(malloced);
			error("cannot malloc(%u) for mallocs[%s]\n",
				resize, name);
		}
		memset(newmallocs + mallocslen, 0, increment * sizeof(void *));
		mallocslen += increment;
		mallocs = newmallocs;
	}
	nextmalloc = i + 1;
	return (mallocs[i] = malloced);
}

/* free temporarily malloced space */
void
cr_free(void *malloced)
{
	register int i;

	if (malloced == NULL)
		return;
	for (i = mallocslen; --i >= 0; ) {
		if (mallocs[i]
		&& (mallocs[i] == malloced || malloced == ALLTMPBUFS)) {
			free(mallocs[i]);
			mallocs[i] = NULL;
			if (i < nextmalloc)
				nextmalloc = i;
			if (malloced != ALLTMPBUFS)
				return;
		}
	}
	if (malloced != ALLTMPBUFS)
		error("cr_free(%x) was invalid\n", malloced);
	if (mallocslen > 4) {
		mallocslen = 0;
		free(mallocs);
		mallocs = NULL;
	}
}

/* unregister temporarily malloced space so it can be retained */
void
cr_unfree(void *malloced)
{
	register int i;

	for (i = mallocslen; --i >= 0; ) {
		if (mallocs[i] == malloced) {
			mallocs[i] = NULL;
			if (i < nextmalloc)
				nextmalloc = i;
			return;
		}
	}
	error("cr_unfree(%x) was invalid\n", malloced);
}

/* close pipe and reset file pointers */
void
resetfp(void)
{
	if (opipe == 1) {
		pclose(fp);
		signal(SIGPIPE,pipesig);
		opipe = 0;
		fp = stdout;
	}
	else if ((fp != stdout) && (fp != rp) && (fp != NULL)) {
		fclose(fp);
		fp = stdout;
	}
}

/* signal handling */
void
sigint(int signo)
{
	signal(signo, sigint);
	if (signo != SIGPIPE)
		fprintf(fp,"\n");
	if (signo == SIGSEGV) {
		static char memflt[] = "memory fault\n";
		if (!vital_init_done)
			fatal(memflt);
		prerrmes(memflt);
	}
	if (signo == SIGSYS) {
		static char enosys[] = "unsupported system call\n";
		if (!vital_init_done)
			fatal(enosys);
		prerrmes(enosys);
	}
	if (signo == SIGPIPE && intsig)
		signal(SIGINT,intsig);	/* lest rdmodsyms() did SIG_IGN */
	else
		fflush(fp);
	resetfp();
	Cur_eng = Sav_eng;
	longjmp(jmp, 0);
}

/* report error and exit */
/*VARARGS1*/
int
fatal(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{
	fprintf(stderr,"crash: ");
	fprintf(stderr,string,arg1,arg2,arg3);
	exit(1);
}

/* report error and terminate command */
/*VARARGS1*/
int
error(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{
	if (!vital_init_done)
		fatal(string,arg1,arg2,arg3);
	if (fp != stdout && !opipe)
		fprintf(stdout,string,arg1,arg2,arg3);
	fprintf(fp,string,arg1,arg2,arg3);
	fflush(fp);
	resetfp();
	if (intsig)
		signal(SIGINT,intsig);	/* lest rdmodsyms() did SIG_IGN */
	Cur_eng = Sav_eng;
	longjmp(jmp,0);
}

/* report error and continue */
/*VARARGS1*/
int
prerrmes(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{
	if (fp != stdout && !opipe) 
		fprintf(stdout,string,arg1,arg2,arg3);
	fprintf(fp,string,arg1,arg2,arg3);
	fflush(fp);
}

/* report warning (as error unless multiple) and continue */
/*VARARGS1*/
int
prwarning(multiple,string,arg1,arg2,arg3)
boolean_t multiple;
char *string;
int arg1,arg2,arg3;
{
	if (!multiple && (fp != stdout && !opipe)) 
		fprintf(stdout,string,arg1,arg2,arg3);
	fprintf(fp,string,arg1,arg2,arg3);
	if (multiple)
		fprintf(fp,"\n");
	else
		fflush(fp);
}

/* traditional 32-bit addr lseek and read: suitable for virtual addrs,
 * but physical addrs supported with a warning, lest used by some .so
 */
void *
readmem(vaddr_t addr, int virt, int proc,
	void *buffer, size_t size, char *name)
{
	static int warned;
	if (!virt && !warned) {
		prerrmes("readmem64 should be used on a physical address\n");
		warned = 1;
	}
	return readmem64((phaddr_t)addr, virt, proc, buffer, size, name);
}

/* 64-bit (well, 36-bit) addr lseek and read: added for physical addrs,
 * but virtual addrs also supported for the convenience of its callers
 */
void *
readmem64(phaddr_t addr, int virt, int proc,
	void *buffer, size_t size, char *name)
{
	size_t cnt, subcnt;
	phaddr_t paddr, faddr;
	unsigned char *bufp;

	if (buffer == NULL)
		buffer = cr_malloc(size, name);

	if (debugmode > 5) {
		/* message split so it's usually ok even if %llx unsupported */
		fprintf(stderr, "readmem:   %caddr=%llx", virt?'v':'p', addr);
		fprintf(stderr, " proc=%d size=%d %s\n", proc, size, name);
	}

	bufp = buffer;
	while (size != 0) {
		cnt   = size;
		paddr = virt? (highlong(addr)? -1:
			cr_vtop((vaddr_t)addr, proc, &cnt, B_TRUE)): addr;
		if (paddr == -1)
			error("readmem: invalid address %llx for %s\n",
				addr, name);
		addr += cnt;
		size -= cnt;

		while ((subcnt = cnt) != 0) {
			if (active)
			    faddr = paddr;
			else if ((faddr = cr_ptof(paddr, &subcnt)) == -1)
			    error("readmem: dumpfile omits paddr %llx for %s\n",
				paddr, name);		/* MREG_HOLE */

			if (faddr == 0 && !active)
			    memset(bufp, 0, subcnt);	/* MREG_ZERO/USER */
			else if (pread64(mem, bufp, subcnt, faddr) != subcnt)
			    error("readmem: read error at offset %llx for %s\n",
				faddr, name);

			bufp += subcnt;
			paddr += subcnt;
			cnt -= subcnt;
		}
        }

	if (debugmode > 5) {
		int i;
		unsigned long *lbufp = buffer;
		/*
		 * Display buffer in longs: but if odd bytes were
		 * requested, the last long shown will include garbage
		 * (or, on occasions too rare to worry about since
		 * this is only debug mode, could cause a SIGSEGV).
		 */
		addr -= (vaddr_t)bufp - (vaddr_t)lbufp;
		fprintf(stderr,"%08llx:",addr);
		for (i = 0; lbufp < (unsigned long *)bufp; lbufp++, i++) {
			if (i == 32
			&&  lbufp + 32 < (unsigned long *)bufp) {
				fprintf(stderr,"  ........");
				i = ((unsigned long *)bufp - lbufp) - 32;
				addr  += i * sizeof(unsigned long);
				lbufp += i;
				i = 32;
			}
			if (i && !(i&3))
				fprintf(stderr,"\n%08llx:",addr);
			fprintf(stderr,"  %08x",*lbufp);
			addr += sizeof(unsigned long);
		}
		fprintf(stderr,"  %s\n", name);
	}

	return buffer;
}

/* read string of unknown length without accessing invalid page unnecessarily */
char *
readstr(char *kvstr, char *name)
{
	size_t cnt;
	char *str, *cp;

	if (kvstr == NULL) {
		str = cr_malloc(1, name);
		*str = '\0';
		return str;
	}
	cnt = 16 - ((vaddr_t)kvstr & 15);
	cp = str = cr_malloc(cnt, name);
	readmem((vaddr_t)kvstr, 1, -1, cp, cnt, name);
	for (;;) {
		while (cp < str + cnt)
			if (*cp++ == '\0')
				return str;	/* cr_unfree it if permanent */
		cp = cr_malloc(cnt + 16, name);
		memcpy(cp, str, cnt);
		cr_free(str);
		str = cp;
		cp = str + cnt;
		readmem((vaddr_t)(kvstr + cnt), 1, -1, cp, 16, name);
		cnt += 16;
	}
}

static engine_t *engine;

/* get Nengine, address of engine array, and panic_data; return panic engine */
int
enginit(void)
{
	vaddr_t addr;
	engine_t eng;
	int engid;
	char *sym;

	if (!UniProc) {
		addr = symfindval(sym = "Nengine");
		readmem(addr, 1, -1, &Nengine, sizeof(Nengine), sym);
		if (Nengine < 1 || Nengine > 32) {
			prerrmes("Nengine claims to be %d: assuming 1\n",
				Nengine);
			Nengine = 1;
		}
	}

	addr = symfindval(sym = "engine");
	readmem(addr, 1, -1, &engine, sizeof(engine), sym);

	for (engid = 1; engid < Nengine; engid++) {
		readengine(engid, &eng);
		if (eng.e_local == NULL) {
			Nengine = engid;
			break;
		}
		KPD[engid] = (paddr_t)cr_vtop((vaddr_t)
			&eng.e_local->pp_kl1pt[0][0], -1, NULL, B_TRUE);
	}

	addr = symfindval(sym = "panic_data");
	readmem(addr, 1, -1, &panic_data, sizeof(panic_data), sym);
	vital_init_done = 1;

	engid = panic_data.pd_engine - engine;
	if (engid >= 0 && engid < Nengine
	&&  panic_data.pd_engine == engine + engid)
		return engid;
	else
		return 0;
}

/* convert engine kernel address to engine number */
int
eng_to_id(engine_t *kengp)
{
	int id = kengp - engine;
	if (id >= 0 && id < Nengine && kengp == engine + id)
		return id;
	return -1;
}

/* convert engine number to engine kernel address */
engine_t *
id_to_eng(int engid)
{
	if (engid < 0 || engid >= Nengine)
		return (engine_t *)(-1);
	return engine + engid;
}

/* error if engine number out of range */
void
check_engid(int engid)
{
	if (engid < 0 || engid >= Nengine)
		error("engine %d is out of range 0-%d\n", engid, Nengine-1);
}

/* error if cg number out of range */
void
check_cgnum(int cgnum)
{
static	vaddr_t		Cg_global = 0;
	
	if(!Cg_global) {
		Cg_global = symfindval("global_cginfo");
		readmem(Cg_global, 1, -1, &global_cginfo, sizeof(global_cginfo), 
			"global_cginfo structure");
		Ncg = global_cginfo.cg_num;
	}
	
	if (cgnum < 0 || cgnum >= Ncg)
		error("cg %d is out of range 0-%d\n", cgnum, Ncg-1);
}

/* read engine structure, adjusting if necessary */
void
readengine(int engid, engine_t *uengp)
{
	check_engid(engid);
	readmem((vaddr_t)(engine + engid), 1, -1, uengp,
		sizeof_engine, "engine structure");
	if (UniProc) {
		uengp->e_smodtime = *(timestruc_t *)(uengp->e_rqlist);
		uengp->e_rqlist[2] = uengp->e_rqlist[1] = NULL;
		uengp->e_rqlist[0] = (struct runque *)(uengp->e_lastpick);
		uengp->e_lastpick = &engine[engid].e_rqlist[0];
	}
	if (uengp->e_local == NULL)	/* copy e_local_pae to e_local */
		uengp->e_local = (struct ppriv_pages *)uengp->e_local_pae;
}

/* read plocal structure, adjusting if necessary */
void
readplocal(int engid, struct plocal *ulp)
{
	static vaddr_t Plocal;
	engine_t eng;

	check_engid(engid);
	if (!Plocal)
		Plocal = symfindval("l");
	Cur_eng = engid;
	readmem(Plocal, 1, -1, ulp, sizeof_plocal, "plocal structure");
	Cur_eng = Sav_eng;
	if (ulp->kl1ptp == NULL) {
		readengine(engid, &eng);
		ulp->kl1ptp = &eng.e_local->pp_kl1pt[0][0];
	}
}

/* read cglocal structure, adjusting if necessary */
void
readcglocal(int cgnum, struct cglocal *cglocalp)
{
	static vaddr_t CGlocal;
	static vaddr_t CG_array_base;
	vaddr_t cgarrayp;

	check_cgnum(cgnum);
	if (!CG_array_base)
		CG_array_base = symfindval("cg_array_base");
	readmem(CG_array_base, 1, -1, &cgarrayp, sizeof(cgarrayp) , "cg array base");

	CGlocal = cgarrayp + mmu_ptob(CGL_PAGES)*cgnum;
	Cur_cg = cgnum;
	readmem(CGlocal, 1, -1, cglocalp, sizeof (struct cglocal), "cglocal structure");
}


/* read lwp structure, adjusting if necessary */
void
readlwp(lwp_t *klwpp, lwp_t *ulwpp)
{
	readmem((vaddr_t)klwpp, 1, -1, ulwpp,
		sizeof_lwp, "lwp structure");
}

static struct procslot {
	proc_t *p;
	pid_t   pid;
} *slottab;
static int maketab = 1;

/* make process address/slot/pid conversion table */
void
makeslottab(void)
{
	proc_t *prp, pr;
	struct pid pid;
	static vaddr_t Practive;
	register int i;
	char *sym;

	maketab = 1;	/* lest interrupted */

	if (slottab != NULL) {
		free(slottab);
		slottab = NULL;
	}
	cr_unfree(slottab =
		cr_malloc(vbuf.v_proc*sizeof(struct procslot), "slot table"));
	memset(slottab, 0,vbuf.v_proc*sizeof(struct procslot));

	if (!Practive)
		Practive = symfindval(sym = "practive");
	readmem(Practive, 1, -1, &prp, sizeof(proc_t *), sym);

	for (; prp != NULL; prp = pr.p_next) {
                readmem((vaddr_t)prp, 1, -1, &pr,
			sizeof(proc_t), "proc table");

                readmem((vaddr_t)pr.p_pidp, 1, -1, &pid,
			sizeof(struct pid), "pid table");

                i = pr.p_slot;
                if (i < 0 || i >= vbuf.v_proc)
                        error("practive process slot is out of range\n");

                slottab[i].p = prp;
                slottab[i].pid = pid.pid_id;
	}

	maketab = 0;
}

/* convert process slot number to pid */
pid_t
slot_to_pid(int slot)
{
	if (slot < 0 || slot >= vbuf.v_proc)
		return 0;
	if (maketab)
		makeslottab();
	return slottab[slot].pid;
}

/* convert pid to process slot number */
int
pid_to_slot(pid_t pid)
{
	register int i;

	if (maketab)
		makeslottab();
	for (i = 0; i < vbuf.v_proc; i++) {
		if (slottab[i].pid == pid
		&&  slottab[i].p != NULL)
			return i;
	}
	return -1;
}

/* convert process slot number to kernel address */
proc_t *
slot_to_proc(int slot)
{
	if (slot < 0 || slot >= vbuf.v_proc)
		return NULL;
	if (maketab)
		makeslottab();
	return slottab[slot].p;
}

/* convert process kernel address to slot number */
int
proc_to_slot(proc_t *kprocp)
{
	int i;

	if (kprocp == NULL)
		return -1;
	if (maketab)
		makeslottab();
	for (i = 0; i < vbuf.v_proc; i++)
		if (slottab[i].p == kprocp)
			return i;
	return -1;
}

/* convert lwp ordinal to kernel address (given its proc structure) */
lwp_t * 
slot_to_lwp(int slot, proc_t *uprocp)
{
	lwp_t *nlwpp;

	if (slot < 0)
		return NULL;
	for (nlwpp = uprocp->p_lwpp; --slot >= 0 && nlwpp != NULL; )
		readmem((vaddr_t)&nlwpp->l_next,1,-1,
			&nlwpp,sizeof(nlwpp),"lwpp->l_next");
	return nlwpp;
}

/* convert lwp kernel address to ordinal (given its proc structure) */
int
lwp_to_slot(lwp_t *lwpp, proc_t *uprocp)
{
	lwp_t *nlwpp;
	int slot;

	for (nlwpp = uprocp->p_lwpp, slot = 0; nlwpp != lwpp; ++slot) {
		readmem((vaddr_t)&nlwpp->l_next,1,-1,
			&nlwpp,sizeof(nlwpp),"lwpp->l_next");
		if (nlwpp == NULL)
			error("cannot find lwp for process\n");
	}
	return slot;
}

/* set current context: engine, process and lwp */
void
get_context(int engid)
{
	static vaddr_t Upointer;
	struct user *upointer;
	struct {
		proc_t *u_procp;
		lwp_t  *u_lwpp;
	} upl;
	proc_t proc;

	Cur_eng = engid;	/* which sets the KPD to be used */

	if (active)
		makeslottab();

	if (!Upointer)
		Upointer = symfindval("upointer");
	readmem(Upointer, 1, -1, &upointer, sizeof(upointer), "upointer");

	if (!CR_KADDR(upointer)) 
		error("get_context: engine %d upointer %x is invalid\n",
			engid, upointer);
	if (debugmode > 1)
		fprintf(stderr,"get_context: engine is %d and upointer is %x\n",
			engid, upointer);

	readmem((vaddr_t)&(upointer->u_procp), 1, -1,
		&upl, sizeof(upl), "current u_procp and u_lwpp");

	if (upl.u_procp) {
		if (debugmode > 5)
			fprintf(stderr,"get_context: u_procp %x u_lwpp %x\n",
				upl.u_procp, upl.u_lwpp);

		readmem((vaddr_t)upl.u_procp, 1, -1,
			&proc, sizeof(proc), "proc structure");

		Cur_proc = proc_to_slot(upl.u_procp);
		Cur_lwp = lwp_to_slot(upl.u_lwpp, &proc);
	}
	else {
		Cur_proc = -1;
		Cur_lwp = Cur_eng;
	}

	Sav_eng = Cur_eng;
}

/* report current context: engine, process and lwp */
void
pr_context(void)
{
	if (Cur_proc == -1) {
		fprintf(fp,
			"Engine: %u of %u  CG: %u of %u Idle Context\n",
			Cur_eng, Nengine, Cur_cg, Ncg);
	} else {
		fprintf(fp,
			"Engine: %u of %u  CG: %u of %u Procslot: %u  Lwpslot: %u\n",
			Cur_eng, Nengine, Cur_cg, Ncg, Cur_proc, Cur_lwp);
	}
}
		
/* convert kernel address to table slot number */
int
getslot(vaddr_t addr, vaddr_t base, size_t size, int phys_archaic, int limit)
{
	int slot = (addr - base) / size;
	if ((slot >= 0) && (slot < limit))
		return(slot);
	return(-1);
}

/* print char as control or two hex digits if unprintable */
void
putch(unsigned int c)
{
	c &= 0xff;
	if (isprint(c))
		fprintf(fp," %c ",c);
	else if (c < 0x20)
		fprintf(fp,"^%c ",c+'@');
	else if (c == 0x7f)
		fprintf(fp,"^? ");
	else
		fprintf(fp,"%02x ",c);
}

/* expand time long into readable string */
char *
date_time(long toc)
{
	static char time_buf[50];
	static char date_fmt[] = "%a %b %e %H:%M:%S %Y";
/*
 * 	%a	abbreviated weekday name
 *	%b	abbreviated month name
 *	%e	day of month
 *	%H	hour
 *	%M	minute
 *	%S	second
 *	%Y	year
 */
	cftime(time_buf, date_fmt, (long *)&toc);
	return time_buf;
}

/* fprintf() with decimal and octal replaced by hex if hexmode on */
/*PRINTFLIKE2*/
int
cr_fprintf(FILE *fp, char *format, ...)
{
	char hexformat[LINESIZE];
	register char *hp;

	if (hexmode && strlen(format) < sizeof(hexformat)) {
		strcpy(hexformat, format);
		for (hp = hexformat; *hp; hp++) {
			if (*hp == '%') {
				if (*++hp == '%')
					continue;
				while (*hp && (!isalpha(*hp) || *hp == 'l'))
					hp++;
				if (!*hp)
					break;
				if (*hp == 'd' || *hp == 'u'
				||  *hp == 'i' || *hp == 'o')
					*hp = 'x';
			}
		}
		format = hexformat;
	}
	return vfprintf(fp, format, &format + 1);
}

/* but a few commands need to override that */
/*PRINTFLIKE2*/
int
raw_fprintf(FILE *fp, char *format, ...)
{
	return vfprintf(fp, format, &format + 1);
}
