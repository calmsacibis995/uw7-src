#ident	"@(#)crash:i386/cmd/crash/page.c	1.2.3.3"

/*
 * This file contains code for the crash functions: page, as, and ptbl.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sysmacros.h>
#include <sys/tss.h>
#include <sys/immu.h>
#include <sys/vnode.h>
#include <sys/vmparam.h>
#include <vm/vm_hat.h>
#include <vm/as.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/page.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/proc.h>
#include "crash.h"

extern pte_t *kpd_start;
extern size_t sizeof_as;

static void prsegs(struct as *, struct as *);
static void prptbl(int, phaddr_t, int, vaddr_t);

#define MAX_PFN	0xFFFFFF

/*
 * On pagepool algorithms see "mem/vm_page.c"
 */
static uint_t max_pfn;
static page_slice_t *pslice_table;
static uint_t pslice_table_entries;        /* entries in the table */
static uint_t pslice_pages;                /* pages in a slice */
static uint_t pslice_page_shift;           /* log_base2(slice_pages) */
static uint_t pslice_page_mask;            /* The corresponding mask */

static void
load_page_slice_table()
{
	int i;
	vaddr_t addr, Pslice_table, Pslice_table_entries;
	
	Pslice_table_entries = symfindval("pslice_table_entries");
	readmem(Pslice_table_entries, 1, -1, &pslice_table_entries,
		sizeof(u_int), "pslice_table_entries");
	
	if (debugmode > 0)
		fprintf(stderr, "number of page slice table entries = %d\n",
			pslice_table_entries);

	Pslice_table = symfindval("pslice_table");

	cr_unfree(pslice_table = 
		cr_malloc(pslice_table_entries * sizeof(struct page_slice),
			  "pslice_table"));

	if (debugmode > 0)
		fprintf(stderr, "page slice table pointer = %x\n",
			Pslice_table);

	/* get the pointer value */
	readmem(Pslice_table, 1, -1, &addr, sizeof(vaddr_t),
		"page slice table pointer");

	readmem(addr, 1, -1, pslice_table, pslice_table_entries
		* sizeof(struct page_slice), "page slice table");

	/* Get the number of pages per slice */
	addr = symfindval("pslice_page_shift");
	readmem(addr, 1, -1, &pslice_page_shift, sizeof(u_int),
		"pslice_page_shift");

	pslice_pages = 1 << pslice_page_shift;
        pslice_page_mask = ~(pslice_pages - 1);

	max_pfn = pslice_pages * pslice_table_entries;
	
	if(debugmode > 0)
		dump_page_slice_table();
}

#define PS_TYPE(pst) ((pst->ps_type == PS_PAGES) ? \
		      "PS_PAGES" \
		      : (pst->ps_type == PS_EMPTY) ? \
		      "PS_EMPTY" \
		      : (pst->ps_type == PS_PURE_NONPAGED) ? \
		      "PS_PURE_NONPAGED" \
		      : "PS_NONPAGED")
		      
static int
dump_page_slice_table()
{
	int i;

	fprintf(stderr, "\tPFN\tpp\ttype\tCG\n");

	for (i = 0; i < pslice_table_entries; i++) {
		page_slice_t *pst;

		pst = &pslice_table[i];
		fprintf(stderr, "\t%x\t%x\t%s\t%d\n",
			i * pslice_pages,
			pst->ps_type == PS_PAGES ? pst->ps_pages : 0, 
			PS_TYPE(pst),
			pst->ps_cg);
	}
}

u_int
page_pptonum(pp)
	page_t	*pp;
{
	return pp->p_pfn;
}

page_t *
page_numtopp(pfn)
	u_int	pfn;
{
	page_t *pp;
        page_slice_t *pst;
	struct page pagebuf;

        pst = &pslice_table[PFN_TO_SLICE(pfn)];

        switch (pst->ps_type) {
        case PS_EMPTY:
        case PS_PURE_NONPAGED:
        case PS_NONPAGED:
                return NULL;
        case PS_PAGES:
                pp = pst->ps_pages + SLICE_PAGE_NUM(pfn);
		readmem((ulong_t) pp, 1, -1, &pagebuf, sizeof(struct page),
			"page structure");

                if (PAGE_IS_DUMMY(&pagebuf))
                        return NULL;
                else
                        return pp;
        default:
                /* should never happen */
		longjmp(syn,0);
        }

	
}

/* get arguments for page function */
int
getpage()
{
	u_int pfn;
	u_int all = 0;
	u_int arg1;
	u_int arg2;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"ew:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(pslice_table == NULL)
		load_page_slice_table();

	if(args[optind]) {
		do {
			if (getargs(MAX_PFN+1,(long *)&arg1,(long *)&arg2)) {
				if(arg2 > max_pfn)
					arg2 = max_pfn;
				for(pfn = arg1; pfn <= arg2; pfn++)
					prpage(all,pfn,-1);
			}
			else
				prpage(all,-1,arg1);
		} while(args[++optind]);
	} else {
		for(pfn = 0; pfn <= max_pfn; pfn++)
			prpage(all,pfn,-1);
	}
}

/* print page structure table */
int
prpage(all,pfn,addr)
u_int all,pfn;
u_long addr;
{
	struct page pagebuf;

	if (addr == (u_long)-1) {
		if ((addr = (u_long)page_numtopp(pfn)) == 0)
			return;
	} else if (pfn == (u_int)-1)
		pfn = page_pptonum((page_t *)addr);

	readmem(addr,1,-1,&pagebuf,sizeof pagebuf,"page structure");

	/* check PAGE_IN_USE */
	if (!all && !pagebuf.p_activecnt && !pagebuf.p_mapcount)
		return;

	fprintf(fp,"   pfn = %6x    pagep  = %08x  vnode  = %08x  offset = %08x\n",
		pfn, addr, pagebuf.p_vnode, pagebuf.p_offset);
	fprintf(fp,"p_next = %08x  p_prev = %08x  vpnext = %08x  vpprev = %08x\n",
		pagebuf.p_next,pagebuf.p_prev,pagebuf.p_vpnext,pagebuf.p_vpprev);
	fprintf(fp,"p_hash = %08x  active = %08x  tstamp = %08x  mapcnt = %08x\n",
		pagebuf.p_hash,pagebuf.p_activecnt,pagebuf.p_timestamp,pagebuf.p_mapcount); 
	fprintf(fp,"%s%s%s%s%s\n",
		pagebuf.p_free    ? "FREE "    : "not-free ",
		pagebuf.p_pageout ? "PAGEOUT " : "no-pageout ",
		pagebuf.p_mod     ? "MOD "     : "not-dirty ",
		pagebuf.p_invalid ? "INVALID " : "valid ",
		pagebuf.p_physdma ? "DMAable " : "not-DMAable ");

	fprintf(fp, "nio = %d  type = %d\n",
		pagebuf.p_nio, pagebuf.p_type);
}

/* get arguments for ptbl function */
int
getptbl()
{
	int proc = Cur_proc;
	int all = 0;
	int phys = 0;
	phaddr_t addr;
	int c;
	int count = 1;
	int gotproc = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"epw:s:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 's' :	proc = setproc();
					gotproc++;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (gotproc > 1)
		longjmp(syn,0);
	if (args[optind]) {
		if (all || gotproc)
			longjmp(syn,0);
		addr = strcon64(args[optind++],phys);
		if (args[optind])
			count = strcon(args[optind++],'d');
		if (args[optind])
			longjmp(syn,0);
		prptbl(phys, addr, count, -1);
	} else if (all) {
		if (phys || gotproc)
			longjmp(syn,0);
		if (active)
			makeslottab();
		for (proc = 0; proc < vbuf.v_proc; proc++)
			pae? prptbls64(proc, B_TRUE): prptbls32(proc, B_TRUE);
	} else {
		if (phys)
			longjmp(syn,0);
		pae? prptbls64(proc, B_FALSE): prptbls32(proc, B_FALSE);
	}
}

/* print all of a proc's page tables: PAE not enabled */
static
prptbls32(proc,multiple)
int proc;
boolean_t multiple;
{
	struct as *asp, *kasp;
	struct hatpt *ptap, *hatptp;
	struct hatpt ptapbuf;
	vaddr_t vaddr;

	if ((asp = readas(proc, &kasp, multiple)) == NULL)
		return;

	fprintf(fp, "Page Tables for Process %d AS %x\n", proc, kasp);

	ptap = hatptp = asp->a_hat.hat_pts;

	do {
		/*
		 * Strict checking as in cr_vtop(), because we have
		 * potential for an infinite loop if something is wrong
		 * e.g. memory freed and reused on an active system
		 */

		if ((vaddr_t)ptap < kvbase) {
			fprintf(fp,"invalid hatpt pointer %x\n", ptap);
			break;
		}

		readmem((vaddr_t)ptap, 1, -1,
			&ptapbuf, sizeof(ptapbuf), "hatpt structure");

		if (ptapbuf.hatpt_as != kasp) {
			fprintf(fp,"hatpt_as %x does not match p_as %x\n",
				ptapbuf.hatpt_as, kasp);
			break;
		}

		/* ASSERT: HAT pointer must be in kpd0 range */
		if (ptapbuf.hatpt_pdtep <  kpd_start
		||  ptapbuf.hatpt_pdtep >= kpd_start
			+ MMU_PAGESIZE/sizeof(pte_t)) {
			fprintf(fp,"invalid hatpt page table pointer %x\n",
				ptapbuf.hatpt_pdtep);
			break;
		}

		vaddr = (ptapbuf.hatpt_pdtep - kpd_start) << PTNUMSHFT;
		fprintf(fp,"\nHATP %08x: vaddr %08x pde %08x\n\n",
			ptap, vaddr, ptapbuf.hatpt_pde.pg_pte);

		/* print page table */
		prptbl(1, ptapbuf.hatpt_pde.pg_pte & ~(MMU_PAGESIZE-1),
			NPGPT, vaddr);
	}
	while ((ptap = ptapbuf.hatpt_forw) != hatptp);

	if (multiple)
		fprintf(fp, "\n");
}

/* print all of a proc's page tables: PAE enabled */
static
prptbls64(proc,multiple)
int proc;
boolean_t multiple;
{
#define kpd_start64	((pte64_t *)kpd_start)
#define KPD_PAGES 4
	struct as *asp, *kasp;
	struct hatpt *ptap, *hatptp;
	struct hatpt ptapbuf;
	vaddr_t vaddr;

	if ((asp = readas(proc, &kasp, multiple)) == NULL)
		return;

	fprintf(fp, "Page Tables for Process %d AS %x\n", proc, kasp);

	ptap = hatptp = asp->a_hat.hat_pts;

	do {
		/*
		 * Strict checking as in cr_vtop(), because we have
		 * potential for an infinite loop if something is wrong
		 * e.g. memory freed and reused on an active system
		 */

		if ((vaddr_t)ptap < kvbase) {
			fprintf(fp,"invalid hatpt pointer %x\n", ptap);
			break;
		}

		readmem((vaddr_t)ptap, 1, -1,
			&ptapbuf, sizeof(ptapbuf), "hatpt structure");

		if (ptapbuf.hatpt_as != kasp) {
			fprintf(fp,"hatpt_as %x does not match p_as %x\n",
				ptapbuf.hatpt_as, kasp);
			break;
		}

		/* ASSERT: HAT pointer must be in kpd0 range */
		if (ptapbuf.hatpt_pdtep64 <  kpd_start64
		||  ptapbuf.hatpt_pdtep64 >= kpd_start64
			+ (KPD_PAGES*MMU_PAGESIZE)/sizeof(pte_t)) {
			fprintf(fp,"invalid hatpt page table pointer %x\n",
				ptapbuf.hatpt_pdtep64);
			break;
		}

		vaddr = (ptapbuf.hatpt_pdtep64 - kpd_start64) << PAE_PTNUMSHFT;
		fprintf(fp,"\nHATP %08x: vaddr %08x pde %09llx\n\n",
			ptap, vaddr, ptapbuf.hatpt_pde64.pg_pte);

		/* print page table */
		prptbl(1, ptapbuf.hatpt_pde64.pg_pte & ~(MMU_PAGESIZE-1),
			PAE_NPGPT, vaddr);
	}
	while ((ptap = ptapbuf.hatpt_forw) != hatptp);

	if (multiple)
		fprintf(fp, "\n");
}

/* print page table */
static void
prptbl(int phys, phaddr_t addr, int count, vaddr_t vaddr)
{
	phaddr_t eaddr;
	int	ptesize;
	char	ptebuf[MMU_PAGESIZE];
	char	*ptep;
	pte64_t	pte;
	int	i;

	if (count <= 0)
		return;

	fprintf(fp, "Page Table Entries\n");
	if (vaddr != (vaddr_t)(-1))
		fprintf(fp, "SLOT     VADDR    PFN   FLAGS\n");
	else
		fprintf(fp, "SLOT    PFN   FLAGS\n");

	ptesize = pae? sizeof(pte64_t): sizeof(pte_t);
	eaddr = addr + (count * ptesize);
	if (((eaddr-1) & ~(MMU_PAGESIZE-1)) != (addr & ~(MMU_PAGESIZE-1))) {
		eaddr = (addr + MMU_PAGESIZE) & ~(MMU_PAGESIZE-1);
		count = (int)(eaddr - addr) / ptesize;
	}
	readmem64(addr, !phys, -1, ptep=ptebuf, count*ptesize, "page table");

	for (i = 0; i < count; i++, ptep += ptesize) {
		pte.pg_pte = pae? *(ullong_t *)ptep:
			(ullong_t)(*(ulong_t *)ptep);
		if (pte.pg_pte == 0)
			continue;
		fprintf(fp, "%4u", i);
		if (vaddr != (vaddr_t)(-1))
			fprintf(fp, "  %08x", vaddr + (i * MMU_PAGESIZE));
		fprintf(fp, " %6llx   ", pte.pgm.pg_pfn);
		fprintf(fp, "%s%s%s%s%s%s%s%s%s%s%s\n",
			pte.pgm.pg_wasref?"was ": "",
			pte.pgm.pg_lock	? "lk "	: "",
			pte.pgm.pg_g	? "g "	: "",
			pte.pgm.pg_ps	? "ps "	: "",
			pte.pgm.pg_mod	? "mod ": "",	
			pte.pgm.pg_ref	? "ref ": "",	
			pte.pgm.pg_cd	? "cd "	: "",
			pte.pgm.pg_wt	? "wt "	: "",
			pte.pgm.pg_us	? "us "	: "",	
			pte.pgm.pg_rw	? "w "	: "",	
			pte.pgm.pg_v	? "v "	: "");	
	}
}

/* get arguments for as function */
int
getas()
{
	int proc;
	int full = 0;
	int all = 0;
	long arg1;
	long arg2;
	int c;
	char *heading = "PROC      SEGS   SEGLAST      SIZE     RSS LOCKED       WSS  WHENAGED   HAT_PTS\n";

	optind = 1;
	while((c = getopt(argcnt,args,"efw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if (all && args[optind])
		longjmp(syn,0);
	if (!full)
		fprintf(fp,"%s",heading);

	if (args[optind]) {
		do {
			if (getargs(PROCARGS,&arg1,&arg2))
			    for (proc = arg1; proc <= arg2; proc++)
				pras(full,proc,-1,heading);
			else
				pras(full,-1,arg1,heading);
		} while(args[++optind]);
	} else if (all) {
		if (active)
			makeslottab();
		for (proc = 0; proc < vbuf.v_proc; proc++) {
			if (slot_to_proc(proc) == NULL)
				continue;
			pras(full,proc,-1,heading);
		}
	} else
		pras(full,Cur_proc,-1,heading);
}


/* print address space structure */
int
pras(full,slot,addr,heading)
int full,slot,addr;
char *heading;
{
	struct proc prbuf, *procaddr;
	struct as asbuf;

	if(addr == -1)
		procaddr = slot_to_proc(slot);
	else
		procaddr = (struct proc *) addr;

	if (procaddr) {
		readmem((long)procaddr,1, -1,(char *)&prbuf,sizeof prbuf,
		    "proc table");
	}
	else
		prbuf.p_as = NULL;

	if (full)
		fprintf(fp,"\n%s",heading);

	if (addr != -1)
		fprintf(fp, "  -   ");
	else if (slot == -1)
		fprintf(fp, "idle  ");
	else
		fprintf(fp, "%4d  ", slot);

	if (procaddr == NULL && slot != -1) {
		fprintf(fp, "does not exist\n");
		return;
	}

	if (prbuf.p_as == NULL) {
		fprintf(fp, "has no user address space\n");
		return;
	}

	/*
	 * Don't use readas() here because that reports errors
	 * for users of the virtual address space e.g. swapped out,
	 * but here we're merely displaying the structure contents.
	 */
	readmem((vaddr_t)prbuf.p_as,1,-1,&asbuf,sizeof_as,"as structure");

	fprintf(fp,"%08x  %08x  %08x %7d %6d  %08x  %08x  %08x\n",
		asbuf.a_segs,
		asbuf.a_seglast,
		asbuf.a_size,
		asbuf.a_rss,
		asbuf.a_lockedrss,
		asbuf.a_wss,
		asbuf.a_whenaged,
		asbuf.a_hat.hat_pts);

	if (full) { 
		prsegs(prbuf.p_as, &asbuf);
	}
}


/* print list of seg structures */
static void
prsegs(struct as *as, struct as *asbuf)
{
	struct seg *seg, *sseg;
	struct seg  segbuf;
	struct syment *sp;
	extern struct syment *findsym();

	sseg = seg = asbuf->a_segs;

	if (seg == NULL)
		return;

	fprintf(fp, "    BASE     SIZE      NEXT     PREV          OPS      DATA\n");

	do {
		readmem((vaddr_t)seg, 1, -1,
			&segbuf, sizeof segbuf, "seg structure");
		fprintf(fp, "%08x %08x  %08x %08x ",
			segbuf.s_base,
			segbuf.s_size,
			segbuf.s_next,
			segbuf.s_prev);

		/* Try to find a symbolic name for the sops vector.
		 * If it can't find one print the hex address.
		 */
		sp = findsym((unsigned long)segbuf.s_ops);
		if ((!sp) || ((unsigned long)segbuf.s_ops != sp->n_value))
			fprintf(fp,"    %08x  ", segbuf.s_ops);
		else
			fprintf(fp, "%12.12s  ", (char *)sp->n_offset);

		fprintf(fp,"%08x\n", segbuf.s_data);

		if (segbuf.s_as != as) {
			fprintf(fp,"seg s_as %08x does not match p_as %08x\n",
				segbuf.s_as, as);
			break;
		}
	} while((seg = segbuf.s_next) != sseg);
}
