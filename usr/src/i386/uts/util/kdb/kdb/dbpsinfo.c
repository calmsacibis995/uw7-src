#ident	"@(#)kern-i386:util/kdb/kdb/dbpsinfo.c	1.25.1.1"
#ident	"$Header$"

#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/kdb/db_as.h>
#include <util/kdb/db_slave.h>
#include <util/kdb/kdb/debugger.h>
#include <util/kdb/kdebugger.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/var.h>

extern as_addr_t	db_cur_addr;
#ifndef UNIPROC
#define db_cur_cpu	db_cur_addr.a_cpu
#else
#define db_cur_cpu	0
#endif

extern int * volatile db_r0ptr[MAXNUMCPU];

extern void _debugger(void);


static lwp_t *
db_ulwpp(cpu)
{
	as_addr_t	addr;
	user_t		*up;
	lwp_t		*ulwpp;

#ifndef UNIPROC
	if (!db_cp_active[cpu])
		return NULL;
#endif
	SET_KVIRT_ADDR_CPU(addr, (u_long)&upointer, cpu);
	if (db_read(addr, (char *)&up, sizeof(user_t *)) == -1)
		return NULL;
	SET_KVIRT_ADDR_CPU(addr, (u_long)&up->u_lwpp, cpu);
	if (db_read(addr, (char *)&ulwpp, sizeof(lwp_t *)) == -1)
		return NULL;
	return ulwpp;
}

static int
show_lwp(lwp_t *lwp, void (*prf)())
{
	as_addr_t addr;
	proc_t *procp;
	uint_t slot;
	k_lwpid_t lwpid;

	ASSERT(prf);

	SET_KVIRT_ADDR(addr, (u_long)&lwp->l_procp);
	if (db_read(addr, (char *)&procp, sizeof procp) == -1) {
invalid:
		(*prf) ("Invalid LWP: %x\n", lwp);
		return -1;
	}
	SET_KVIRT_ADDR(addr, (u_long)&procp->p_slot);
	if (db_read(addr, (char *)&slot, sizeof slot) == -1)
		goto invalid;
	SET_KVIRT_ADDR(addr, (u_long)&lwp->l_lwpid);
	if (db_read(addr, (char *)&lwpid, sizeof lwpid) == -1)
		goto invalid;

	(*prf) ("LWP %d,%d @ %x:", slot, lwpid, lwp);
	return 0;
}

void
db_lstack(lwp_t *lwp, void (*prf)())
{
	as_addr_t addr;
	user_t *uptr;
	int cpu;

	for (cpu = 0; cpu < Nengine; ++cpu) {
		if (lwp == (lwp_t *)-1) {
			if (cpu != db_cur_cpu)
				continue;
			lwp = db_ulwpp(cpu);
		} else if (lwp == NULL || lwp != db_ulwpp(cpu))
			continue;
		if (prf) {
#ifndef UNIPROC
			(*prf) ("(cpu %x current) ", cpu);
#else
			(*prf) ("(current) ");
#endif
			if (lwp == NULL)
				(*prf) ("idle stack:\n");
			else {
				if (show_lwp(lwp, prf) == -1)
					return;
				(*prf) ("\n");
			}
		}
		db_st_r0ptr = db_r0ptr[cpu];
#ifndef UNIPROC
		if (cpu != l.eng_num) {
			db_slave_cmd(cpu, DBSCMD_GET_STACK);
			db_st_offset = 0;
			db_st_cpu = cpu;
			db_stacktrace(prf, (ulong_t)_debugger, 0, lwp);
		} else
#endif
			db_stacktrace(prf, (ulong_t)_debugger, 1, lwp);
		return;
	}

	if (lwp == (lwp_t *)-1) {
		if (prf)
			(*prf) ("Couldn't find LWP for cpu %d\n", db_cur_cpu);
		return;
	}

	if (prf && show_lwp(lwp, prf) == -1)
		return;

	SET_KVIRT_ADDR(addr, (u_long)&lwp->l_up);
	if (db_read(addr, (char *)&uptr, sizeof uptr) == -1) {
		if (prf)
			(*prf) (" Invalid LWP\n");
		return;
	}

	/*
	 * The desired LWP is not currently running.
	 * We need to get its saved context from its uarea.
	 */
#ifndef lint
	ASSERT(sizeof(user_t) <= PAGESIZE);
#endif
	ASSERT(((vaddr_t)(uptr + 1) & PAGEOFFSET) == 0);
	if (DB_KVTOP((ulong_t)uptr, l.eng_num) == (paddr64_t)-1) {
		if (prf)
			(*prf) (" <swapped out>\n");
		return;
	}

	if (prf)
		(*prf) ("\n");

	db_st_startsp = uptr->u_kcontext.kctx_esp;
	db_st_startfp = uptr->u_kcontext.kctx_ebp;
	db_st_startpc = uptr->u_kcontext.kctx_eip;
	db_st_offset = 0;
	db_st_cpu = l.eng_num;

	db_stacktrace(prf, NULL, 0, lwp);
}

/*
 * Stack trace of LWP(s) in process, proc [proc may be a process pointer
 * or a slot number].  If lwpid == 0, all LWPs are shown; otherwise just
 * the LWP with ID lwpid.
 */
void
db_pstack(proc_t *proc, k_lwpid_t lwpid, void (*prf)())
{
	as_addr_t addr;
	lwp_t *lwp;
	k_lwpid_t lwp_id;

	if ((ulong_t)proc < v.v_proc)
		proc = PSLOT2PROC((ulong_t)proc);

	if (proc == NULL) {
		if (prf)
			(*prf) ("no such process: 0x%lx\n", (ulong_t)proc);
		return;
	}

	SET_KVIRT_ADDR(addr, (vaddr_t)&proc->p_lwpp);
	if (db_read(addr, &lwp, sizeof lwp) == -1)
		goto error;

	if (lwp == NULL) {
		if (prf)
			(*prf) ("zombie process: 0x%lx\n", (ulong_t)proc);
		return;
	}

	do {
		addr.a_addr = (vaddr_t)&lwp->l_lwpid;
		if (db_read(addr, &lwp_id, sizeof lwp_id) == -1)
			goto error;
		if (lwpid == 0 || lwpid == lwp_id) {
			db_lstack(lwp, prf);
			if (kdb_output_aborted)
				return;
		}
		addr.a_addr = (vaddr_t)&lwp->l_next;
		if (db_read(addr, &lwp, sizeof lwp) == -1)
			goto error;
	} while (lwp != NULL);

	return;

error:
	if (prf)
		(*prf) ("invalid process: 0x%lx\n", (ulong_t)proc);
}


void
db_tstack(ulong_t esp, void (*prf)())
{
	db_st_startsp = esp;
	db_st_startfp = UNKNOWN;
	db_st_startpc = UNKNOWN;
	db_st_offset = 0;
	db_st_cpu = l.eng_num;

	db_stacktrace(prf, NULL, 0, NULL);
}


static void
ps_command(const proc_t *pp)
{
	struct execinfo *ei = pp->p_execinfo;

	dbprintf("%-14.14s", ei ? ei->ei_comm : "");
}

void
db_ps(void)
{
	register lwp_t *lwpp;
	register proc_t	*pp;
	int	cpu;

	dbprintf(
"SLOT PP         PFLAGS   UID   PID  PPID    COMMAND\n"
"     LWPP       LFLAGS  LWP-NAME        CPU LWPID PRI S   SYNCP  STYPE\n");

	for (pp = practive; pp != NULL; pp = pp->p_next) {
		if (kdb_output_aborted)
			return;
		dbprintf("%4x %-8x", pp->p_slot, pp);
		dbprintf(" %8x", pp->p_flag);
		dbprintf(" %5d", pp->p_cred->cr_uid);
		if (pp->p_pidp)
			dbprintf(" %5d", pp->p_pidp->pid_id);
		else
			dbprintf("   ???");
		dbprintf(" %5d    ", pp->p_ppid);

		if ((lwpp = pp->p_lwpp) == NULL) {
			/* Must be a zombie */
			dbprintf("<zombie>\n");
			continue;
		}

		ps_command(pp);

		dbprintf("\n");

		do {
			dbprintf("%5s", "");
			dbprintf("%-8x", lwpp);
			dbprintf(" %8x", lwpp->l_flag);
			if (lwpp->l_name == NULL)
				dbprintf("%18s", "");
			else
				dbprintf("  %-15.15s ", lwpp->l_name);
			for (cpu = 0; cpu < Nengine; ++cpu) {
#ifndef UNIPROC
				if (lwpp == db_lwp[cpu])
#else
				if (lwpp == db_ulwpp(cpu))
#endif
					break;
			}
			if (cpu < Nengine)
				dbprintf("%3x", cpu);
			else
				dbprintf("   ");
			dbprintf(" %5d", lwpp->l_lwpid);
			dbprintf(" %3d ", lwpp->l_pri);
			switch (lwpp->l_stat) {
			case SONPROC:   dbprintf("O"); break;
			case SRUN:      dbprintf("R"); break;
			case SSLEEP:    dbprintf("S"); break;
			case SSTOP:     dbprintf("T"); break;
			case SIDL:      dbprintf("I"); break;
			default:        dbprintf("?"); break;
			}
			if (lwpp->l_stat == SSLEEP) {
				dbprintf(" %-8x", lwpp->l_syncp);
				switch (lwpp->l_stype) {
				case ST_COND:
					dbprintf(" COND"); break;
				case ST_EVENT:
					dbprintf(" EVNT"); break;
				case ST_RDLOCK:
					dbprintf(" RDLK"); break;
				case ST_WRLOCK:
					dbprintf(" WRLK"); break;
				case ST_SLPLOCK:
					dbprintf(" SLPLK"); break;
				case ST_USYNC:
					dbprintf(" USYNC"); break;
				default:
					dbprintf(" ????"); break;
				}
			}
			dbprintf("\n");
			if (kdb_output_aborted)
				return;
		} while ((lwpp = lwpp->l_next) != NULL);
	}
}
