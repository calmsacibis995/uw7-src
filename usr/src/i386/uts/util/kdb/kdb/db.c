#ident	"@(#)kern-i386:util/kdb/kdb/db.c	1.37.3.2"
#ident	"$Header$"

/*	AT&T 80x86 Kernel Debugger	*/

#include <io/conssw.h>
#include <mem/kmem.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/msr.h>
#include <svc/reg.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/emask.h>
#include <util/engine.h>
#include <util/ghier.h>
#include <util/ipl.h>
#include <util/inline.h>
#include <util/kdb/kdebugger.h>
#include <util/kdb/xdebug.h>
#include <util/kdb/kdb/debugger.h>
#include <util/kdb/db_as.h>
#include <util/kdb/db_slave.h>
#include <util/ksynch.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

static int db_load(void), db_unload(void);

MOD_MISC_WRAPPER(db, db_load, db_unload, "kdb - kernel debugger");

label_t dbsave;
int db_msg_pending;
int db_show_exit;
int *db_ex_frame;	/* Pointer to exception stack frame */
enum regset_type db_ex_frame_type;

#ifndef UNIPROC

lock_t db_master_lock;
LKINFO_DECL(db_mlkinfo, "kernel debugger master", 0);

STATIC emask_t db_slaves;
STATIC volatile int db_slaves_signalled;

volatile int db_cp_active[MAXNUMCPU];
#define dbactive (db_cp_active[myengnum])

volatile int db_slave_flag[MAXNUMCPU];
struct lwp * volatile db_lwp[MAXNUMCPU];

#else /* UNIPROC */

int dbactive;

#endif /* UNIPROC */

int * volatile db_r0ptr[MAXNUMCPU];

pl_t db_entry_ipl[MAXNUMCPU];

extern int in_demon;

#ifdef DEBUG
STATIC int kdb_pdep;  /* check proper pairing of kdb_pdep_enter/kdb_pdep_exit */
#endif

static int	db_reason;
static unsigned db_d6;
static unsigned	enter;
static char	*do_cmds = NULL;

struct brkinfo db_brk[MAX_BRKNUM+1];
int db_hbrk[MAX_HBRKNUM+1];
int db_brknum;
unsigned long db_brkaddr;

extern int dbsingle,dbbranch;
extern int db_pass_calls;

extern uint_t db_nlines, db_lines_left;

static int dbsingle_noenter;

static as_addr_t bpt3_addr;
static const u_char opc_int3 = OPC_INT3;

static void load_pbrks(void), load_hbrks(void), unload_brks(void);
static void set_tmp_brk(vaddr_t);
static void db_show_frame(void);

void toggle_p6deb(ulong);
int db_master(int);

static int db_is_call(as_addr_t);

extern void st_frameregs(vaddr_t, void (*)(), boolean_t);

extern void db_clearbrks(void);

extern void db_xprintf(const char *fmt, VA_LIST ap);

#ifndef UNIPROC
void db_invoke_slave(void);
static void db_slave(void);
#endif /* not UNIPROC */

void _debugger(int, int *);

struct kdebugger db_kdb = {
        _debugger,
#ifndef UNIPROC
	db_slave_cmd,
#else
	0,
#endif /* not UNIPROC */
	db_xprintf
};

void
db_kdb_init(void)
{
	unsigned	hbrknum;

	for (hbrknum = 0; hbrknum <= MAX_HBRKNUM;)
		db_hbrk[hbrknum++] = -1;

	LOCK_INIT(&db_master_lock, KDB_HIER, PLHI, &db_mlkinfo, KM_NOSLEEP);

        kdb_register(&db_kdb);
}

static int
db_load(void)
{
	db_kdb_init();
	return 0;
}

static int
db_unload(void)
{
	_debugger(DR_UNLOAD, NO_FRAME);

	kdb_unregister(&db_kdb);

	LOCK_DEINIT(&db_master_lock);

	return 0;
}

void
_debugger(int reason, int *r0ptr)
{
	if (reason == DR_ONLINE) {
#ifndef UNIPROC
		pl_t oldpri;

		kdb_online();

		oldpri = LOCK(&db_master_lock, PLHI);
		while (db_master_cpu != -1 && !db_slaves_signalled)
			;
		if (db_master_cpu != -1)
			EMASK_SET1(&db_slaves, myengnum);
		if (db_master_cpu == -1) {
			load_hbrks();
			UNLOCK(&db_master_lock, oldpri);
			return;
		}
		reason = DR_SLAVE;
		UNLOCK(&db_master_lock, oldpri);
#else
		return;
#endif
	}

	if (!dbactive) {
#if !DEBUG
		if (kdb_security && (reason == DR_USER || reason == DR_OTHER))
#else
		if (kdb_security && reason == DR_USER)
#endif
			return;
		db_entry_ipl[myengnum] = splhi();
	} else
		splhi();

	db_flushtlb();

	db_r0ptr[myengnum] = r0ptr;
#ifndef UNIPROC
	db_lwp[myengnum] = u.u_lwpp;

	if (reason == DR_SLAVE) {
		if (dbactive)
			cmn_err(CE_PANIC, "KDB: slave already active");
		dbactive = 1;

		db_slave();
	} else
#endif /* not UNIPROC */
	if (db_master(reason)) {
		/* Bail out if we had to revert to slave mode */
#ifndef UNIPROC
		db_lwp[myengnum] = NULL;
#endif
		splx(db_entry_ipl[myengnum]);
		return;
	}

	load_hbrks();

#ifndef UNIPROC
	db_lwp[myengnum] = NULL;
#endif

	dbactive = 0;
	if (reason != DR_SLAVE)
		in_demon = 0;

	splx(db_entry_ipl[myengnum]);
}


int
db_master(int reason)
{
	int was_branch = 0;

#ifndef UNIPROC
	/*
	 * Stop the world (i.e. put other CPUs in slave mode),
	 * unless we were single-stepping, in which case it's already stopped.
	 */
	if (!dbactive && db_master_cpu != myengnum) {
		/* Negotiate for the right to be the master. */
		if (TRYLOCK(&db_master_lock, PLHI) == INVPL) {
			/*
			 * Someone else got to be master first.
			 * Return so we can reenter in slave mode.
			 */
			return 1;
		} else if (db_master_cpu != -1) {
			UNLOCK(&db_master_lock, PLHI);
			/*
			 * Someone else got to be master first.
			 * Return so we can reenter in slave mode.
			 */
			return 1;
		}
		db_master_cpu = myengnum;
		db_slaves_signalled = 0;
		UNLOCK(&db_master_lock, PLHI);

		/* Now signal the other CPUs to stop. */
		{
			int	cpu;

			for (cpu = 0; cpu < Nengine; cpu++) {
				if (cpu == myengnum)
					continue;
				db_lwp[cpu] = NULL;
				db_cp_active[cpu] = 0;
			}
			xcall_all(&db_slaves, B_TRUE, db_invoke_slave, NULL);
			db_slaves_signalled = 1;
		}

		/* Wait for them all to stop. */
		db_slave_cmd(-1, DBSCMD_SYNC);
	}
#endif /* not UNIPROC */

	db_d6 = db_dr6();		/* read debug status register */
	db_wdr6(0);		/* clear it for next time */
	db_wdr7(0);		/* disable breakpoints to prevent recursion */

	if ((db_reason = reason) == DR_SECURE_USER)
		db_reason = DR_USER;

	db_st_r0ptr = db_r0ptr[myengnum];
	db_stacktrace(NULL, NULL, 1, NULL);
	db_ex_frame = regset[0];
	db_ex_frame_type = regset_type[0];

	if (!dbactive) {
		db_brknum = -1;
		unload_brks();
		if (db_reason == DR_UNLOAD) {
			db_clearbrks();
			goto load_brks;
		}
		if (db_reason == DR_RELEASE_DBREGS)
			goto load_brks;
		kdb_start_io();
		ASSERT(kdb_pdep++ == 0);
		kdb_pdep_enter();
	}

	dbactive = 1;
	in_demon = 1;

	if (db_reason == DR_BPT1) {  /* called due to trap 1 brkpt */
		int	hbrknum;

		db_reason = DR_OTHER;
		for (hbrknum = 0; hbrknum <= MAX_HBRKNUM; hbrknum++) {
			if (!(db_d6 & (1 << hbrknum)))
				continue;
			switch (hbrknum) {
			case 0:	db_brkaddr = db_dr0();  break;
			case 1:	db_brkaddr = db_dr1();  break;
			case 2:	db_brkaddr = db_dr2();  break;
			case 3:	db_brkaddr = db_dr3();  break;
			}
			if ((db_brknum = db_hbrk[hbrknum]) == -1)
				continue;	/* shouldn't happen */
			break;
		}
	} else if (db_reason == DR_STEP) {	/* called due to single-step */
		if (db_ex_frame) {
			as_addr_t	addr;
			u_char		opc;
			flags_t		efl;

			SET_KVIRT_ADDR(addr, db_ex_frame[T_EIP] - 1);
			db_read(addr, &opc, 1);
			if (opc == OPC_PUSHFL) {
				/* The pushfl will have pushed the single-
				   step flag bit onto the stack, so we have
				   to turn it off. */
				addr.a_addr = (ulong_t)&db_ex_frame[T_EFL + 1];
				db_read(addr, (char *)&efl, sizeof(efl));
				efl.fl_tf = 0;
				db_write(addr, (char *)&efl, sizeof(efl));
			}
		}
	}

	/* Clear temporary breakpoint */
	db_brk[TMP_BRKNUM].state = BRK_CLEAR;

	if (db_brknum == TMP_BRKNUM) {	/* From bkpt set by single-stepping */
		db_reason = DR_STEP;
		db_brknum = -1;
	}

	if (db_reason == DR_STEP) {
		if (dbsingle)
			--dbsingle;
		else if ((was_branch = dbbranch) != 0)
			--dbbranch;
	} else {
		dbsingle = dbbranch = 0;
	}

	enter = !dbsingle && !dbbranch && !dbsingle_noenter;
	do_cmds = NULL;
	db_show_exit = 0;

	if (enter) {
		db_lines_left = db_nlines;	/* reset pagination */
		kdb_output_aborted = B_FALSE;
	}

	if (db_reason == DR_INIT) {
		if (*(do_cmds = debugger_init) == '\0')
			enter = 0;
	} else if (db_brknum != -1) {
		db_reason = DR_BPT1;
		db_msg_pending = 1;
		if (db_brk[db_brknum].state != BRK_ENABLED) {
			db_brk_msg(0);
			dbprintf(" - wasn't set!\n");
		} else {
			if (db_brk[db_brknum].tcount) {
				db_brk_msg(0);
				dbprintf(" (%d left)\n",
					 --db_brk[db_brknum].tcount);
				enter = 0;
			} else
				do_cmds = db_brk[db_brknum].cmds;
		}
	}

	if (db_reason == DR_BPT1 || db_reason == DR_INIT) {
		if (enter && !do_cmds)
			db_brk_msg(1);
	} else {
		if (db_reason != DR_STEP) {
			dbprintf("\nDEBUGGER:");
			if (db_reason == DR_BPT3)
				dbprintf(" Unexpected INT 3 Breakpoint!");
			dbprintf("\n");
		}
		if (db_reason != DR_USER && db_ex_frame != NULL &&
		    !dbsingle_noenter) {
			if (was_branch) {
				dbprintf("Branch");
				lbr_print_from_to(0);
				was_branch = 0;
			}
			db_show_frame();
			if (kdb_output_aborted && (dbsingle || dbbranch)) {
				dbsingle = dbbranch = 0;
				enter = 1;
			}
		}
	}
	if (enter) {
		db_show_exit = !do_cmds;
		dbinterp(do_cmds);
		if (dbsingle == 0 && dbbranch == 0 && db_show_exit) {
			kdb_output_aborted = B_FALSE;
			dbprintf("DEBUGGER exiting\n");
		}
	}

	dbsingle_noenter = 0;

	if (db_ex_frame != NULL) {
		if (dbsingle || dbbranch) {
			if (db_pass_calls) {
				uint		n;

				SET_KVIRT_ADDR(bpt3_addr, db_ex_frame[T_EIP]);
				if ((n = db_is_call(bpt3_addr)) > 0)
					bpt3_addr.a_addr += n;
				else
					bpt3_addr.a_addr = 0;
			} else
				bpt3_addr.a_addr = 0;
			if (bpt3_addr.a_addr)
				set_tmp_brk(bpt3_addr.a_addr);
			else
				((flags_t *)&db_ex_frame[T_EFL])->fl_tf = 1;
		}
		if (dbbranch)
			toggle_p6deb(BTF);
	}
	toggle_p6deb(LBR);

load_brks:
	load_pbrks();

	if (dbactive) {
		ASSERT(--kdb_pdep == 0);
		kdb_pdep_exit();
		kdb_end_io();
	}

#ifndef UNIPROC
	if (!dbsingle && !dbbranch && !dbsingle_noenter) {
		/* Release master ownership */
		db_master_cpu = -1;

		/* Tell all the slaves to exit */
		db_slave_cmd(-1, DBSCMD_EXIT);
	}
#endif /* not UNIPROC */

	return 0;
}


#ifndef UNIPROC

void
db_invoke_slave(void)
{
	(*cdebugger) (DR_SLAVE, NO_FRAME);
}

static void
db_slave(void)
{
	u_int	cmd;

	/* Process commands from the master */
	do {
		/* Wait for a command */
		while (db_slave_flag[myengnum] == 0)
			xcall_intr();
		/* Process the command */
		switch (cmd = db_slave_command.cmd) {
		case DBSCMD_SYNC:
		case DBSCMD_EXIT:
			break;
		case DBSCMD_GET_STACK:
			db_get_stack();
			break;
		case DBSCMD_AS_READ:
			db_slave_command.rval =
				db_read(db_slave_command.addr,
					db_slave_command.buf,
					db_slave_command.n);
			break;
		case DBSCMD_AS_WRITE:
			db_slave_command.rval =
				db_write(db_slave_command.addr,
					 db_slave_command.buf,
					 db_slave_command.n);
			break;
		case DBSCMD_CR0:
			db_slave_command.rval = db_cr0();
			break;
		case DBSCMD_CR2:
			db_slave_command.rval = db_cr2();
			break;
		case DBSCMD_CR3:
			db_slave_command.rval = db_cr3();
			break;
		case DBSCMD_CR4:
			db_slave_command.rval = db_safe_cr4();
			break;
		}
		/* Tell the master we're done */
		ASSERT(db_slave_flag[myengnum] != 0);
		db_slave_flag[myengnum] = 0;

	} while (cmd != DBSCMD_EXIT);
}


/*	int	slave;		Slave CPU #, or -1 for all */
void
db_slave_cmd(int slave, int cmd)
{
	int	cpu;

	db_slave_command.cmd = cmd;

	if (slave == -1) {
		for (cpu = 0; cpu < Nengine; ++cpu) {
			if (!EMASK_TEST1(&db_slaves, cpu))
				continue;
			ASSERT(db_slave_flag[cpu] == 0);
			db_slave_flag[cpu] = 1;
		}
		for (cpu = 0; cpu < Nengine; ++cpu) {
			while (db_slave_flag[cpu] != 0)
				;
		}
	} else {
		ASSERT(slave != myengnum);
		ASSERT(db_cp_active[slave]);
		ASSERT(db_slave_flag[slave] == 0);
		db_slave_flag[slave] = 1;
		while (db_slave_flag[slave] != 0)
			;
	}
}

#endif /* not UNIPROC */


static void
db_show_frame(void)
{
	as_addr_t	addr;

	db_st_offset = 0;
	regset_next = 1;
	st_frameregs((ulong_t)db_ex_frame, (void (*)())dbprintf,
		     (db_ex_frame_type == RS_FULL));
	SET_KVIRT_ADDR(addr, db_ex_frame[T_EIP]);
	db_dis(addr, 1);
}

void
db_brk_msg(int eol)
{
	db_msg_pending = 0;
	dbprintf("\nDEBUGGER: breakpoint %d at 0x%lx", db_brknum, db_brkaddr);
	if (eol) {
		dbprintf("\n");
		if (db_ex_frame)
			db_show_frame();
	}
	db_show_exit = 1;
}

/* look for external names in kernel debugger */
int
dbextname(char *name)
{
    if ((dbstack[dbtos].value.number = findsymaddr(name)) != NULL) {
	if (dbstackcheck(0, 1))
	    return 1;
	dbstack[dbtos].type = NUMBER;
	dbtos++;
	return 1;
    }
    return 0;   /* not found */
}

void
dbtellsymname(char *name, vaddr_t addr, vaddr_t sym_addr)
{
	dbprintf("%s", name);
	if (addr != sym_addr)
		dbprintf("+0x%lx", addr - sym_addr);
	dbprintf("\n");
}


static void
load_pbrks(void)
{
	uint_t brknum, hbrknum;
	struct brkinfo *brkp;

	/* First pass: assign hardware bkpts to bkpts which require hardware */
	hbrknum = 0;
	brkp = &db_brk[brknum = MAX_BRKNUM];
	do {
		if (brkp->state != BRK_ENABLED)
			continue;
		if (brkp->type == BRK_INST)
			continue;
		if (user_debug_count != 0) {
			if (dbactive)
				dbprintf("Can't use data breakpoints while "
					 "user-level debugging is active;\n"
					 "disabling breakpoint #%d\n", brknum);
			else
				cmn_err(CE_WARN,
					"disabling kdb breakpoint #%d because "
					"user-level debugging is active.",
					brknum);
			brkp->state = BRK_DISABLED;
			continue;
		}
		if (hbrknum == MAX_HBRKNUM + 1) {
			if (dbactive)
				dbprintf("Too many data breakpoints "
					 "enabled; #%d ignored\n", brknum);
			else
				cmn_err(CE_WARN,
					"Too many kdb data breakpoints "
					"enabled; #%d ignored\n", brknum);
			continue;
		}
		db_hbrk[hbrknum++] = brknum;
	} while (--brkp, brknum-- != 0);

	/*
	 * Second pass: assign remaining hardware bkpts, if available;
	 * load remaining breakpoints using breakpoint opcodes (INT 3).
	 */
	brkp = db_brk;
	for (brknum = 0; brknum <= MAX_BRKNUM; ++brkp, ++brknum) {
		if (brkp->state != BRK_ENABLED)
			continue;
		if (brkp->type != BRK_INST)
			continue;
		if (user_debug_count == 0 && hbrknum <= MAX_HBRKNUM)
			db_hbrk[hbrknum++] = brknum;
		else if (db_ex_frame && db_ex_frame[T_EIP] == brkp->addr) {
			if (!((flags_t *)&db_ex_frame[T_EFL])->fl_tf) {
				((flags_t *)&db_ex_frame[T_EFL])->fl_tf = 1;
				dbsingle_noenter = 1;
			}
		} else {
			/*
			 * each CG may have a replica of kernel text
			 * we need to perform the breakpoint insertion
			 * on all replicas (this is done by specifying
			 * AS_ALL_CPU as the cpu below)
			 */
			SET_KVIRT_ADDR_CPU(bpt3_addr, brkp->addr,AS_ALL_CPU);
			db_read(bpt3_addr, &brkp->saved_opc, 1);
			db_write(bpt3_addr, &opc_int3, 1);
			brkp->state = BRK_LOADED;
		}
	}

	/* Clear any leftover hardware bkpts */
	while (hbrknum <= MAX_HBRKNUM)
		db_hbrk[hbrknum++] = -1;
}

static void
load_hbrks(void)
{
	uint_t dr7, hbrknum;
	struct brkinfo *brkp;
	static vaddr_t hbrkaddr[MAX_HBRKNUM + 1];

	/* Can't use hardware breakpoint registers if user is using them */
	if (user_debug_count != 0)
		return;

	/* Load hardware breakpoint registers */
	dr7 = 0;
	for (hbrknum = MAX_HBRKNUM + 1; hbrknum-- > 0;) {
		if (db_hbrk[hbrknum] == -1)
			continue;
		brkp = &db_brk[db_hbrk[hbrknum]];
		dr7 |= brkp->type << (16 + 4 * hbrknum);
		dr7 |= 2 << (2 * hbrknum);
		if (brkp->type == BRK_IO && !(l.cpu_features[0] & CPUFEAT_DE)) {
			dbprintf("CPU %d does not support I/O breakpoints\n",
				 myengnum);
			continue;
		}
		if (brkp->type != BRK_INST)
			dr7 |= 0x200;
		hbrkaddr[hbrknum] = brkp->addr;
	}

	db_wdr0(hbrkaddr[0]);
	db_wdr1(hbrkaddr[1]);
	db_wdr2(hbrkaddr[2]);
	db_wdr3(hbrkaddr[3]);
	db_wdr7(dr7);
}

static void
unload_brks(void)
{
	struct brkinfo *brkp;

	brkp = &db_brk[MAX_BRKNUM + 1];
	while (brkp-- != db_brk) {
		if (brkp->state != BRK_LOADED)
			continue;
		SET_KVIRT_ADDR(bpt3_addr, brkp->addr);
		db_write(bpt3_addr, &brkp->saved_opc, 1);
		brkp->state = BRK_ENABLED;
		if (db_reason == DR_BPT3 && db_ex_frame &&
		    db_ex_frame[T_EIP] == brkp->addr + 1) {
			db_brknum = brkp - db_brk;
			db_ex_frame[T_EIP] = brkp->addr;
		}
	}
}

static void
set_tmp_brk(vaddr_t addr)
{
	db_brk[TMP_BRKNUM].addr = addr;
	db_brk[TMP_BRKNUM].type = BRK_INST;
	db_brk[TMP_BRKNUM].state = BRK_ENABLED;

	/* tcount and cmds are never set for TMP_BRKNUM */
}


static int
db_is_call(as_addr_t addr)
{
	u_char	opc;

	if (db_read(addr, &opc, sizeof(opc)) == -1)
		return 0;
	switch (opc) {
	case OPC_CALL_REL:
		return 5;
	case OPC_CALL_DIR:
		return 7;
	}
	return 0;
}


ulong_t
db_safe_cr4(void)
{
	if (l.cpu_features[0] == 0) {
		/* cr4 register not present */
		return 0;
	}
	return db_cr4();
}

/*
 * Calls to this function should be guarded with
 * calls to p6_debug()
 */
int
get_numlbr(void)
{
	ulong_t val[2];

	return(1);
}

/*
 * Calls to this function should be guarded with
 * calls to p6_debug().  Returns the from- and to-
 * EIP values for the branch record.
 */
void
read_brec(ulong_t *val, int indx)
{
	int base;
	ulong_t tval[2];
	ulong_t addr;

	base = LastBranchFromIP + (2 * indx);
	_rdmsr(base, tval);
	val[0] = (tval[1]  + tval[0]);
	_rdmsr(base+1,tval);
	val[1] = (tval[1]  + tval[0]);
}

int
p6_debug(void)
{
	if ((l.cpu_id == CPU_P6) && (l.cpu_model >= P6_MODEL))
		return(1);
	else
		return(0);
}

void
toggle_p6deb(ulong val)
{
	ulong_t rval[2];

	if (p6_debug()) {
		_rdmsr(DebugCtlMSR, rval);
		rval[0] |= val;
		_wrmsr(DebugCtlMSR, rval);
	}
}
