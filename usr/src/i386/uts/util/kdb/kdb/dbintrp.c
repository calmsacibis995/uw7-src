#ident	"@(#)kern-i386:util/kdb/kdb/dbintrp.c	1.36.3.2"
#ident	"$Header$"

#include <mem/vmparam.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <svc/cpu.h>
#include <svc/reg.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/kdb/db_as.h>
#include <util/kdb/db_slave.h>
#include <util/kdb/kdb/dbcmd.h>
#include <util/kdb/kdb/debugger.h>
#include <util/kdb/kdebugger.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

ushort dbtos;
struct item dbstack[DBSTKSIZ];
static struct variable variable[VARTBLSIZ];
static char *outformat = "%Lx";
static char *typename[] =
	{"NULL","NUMBER","STRING","NAME" };
char dbverbose;
int dbsingle;
int dbbranch;
int db_pass_calls;
static int exit_db;

extern struct brkinfo db_brk[MAX_BRKNUM+1];

extern int	db_hard_eol;
extern uint_t	st_max_args;
extern ulong_t	*db_ex_frame;

extern pl_t	db_entry_ipl[MAXNUMCPU];

as_addr_t db_cur_addr;
#define db_cur_as	db_cur_addr.a_as	/* Current address space */
#define db_as_number	db_cur_addr.a_mod	/* Numeric address space modifier */
#ifndef UNIPROC
#define db_cur_cpu	db_cur_addr.a_cpu	/* Current CPU number */
#else
#define db_cur_cpu	0
#endif

int	db_cur_size;	/* Current operand size */
int	db_cur_rset;	/* Current register set # */

static unsigned	cur_brk_type;

/*
 * The db_to_r0_reg[] array converts a DB_XXX register number to an
 * r0ptr trap frame offset.
 */
static int db_to_r0_reg[] = {
	T_EAX, T_EBX, T_ECX, T_EDX, T_EDI, T_ESI, T_EBP, 0, T_EIP, T_EFL,
	T_CS, T_DS, T_ES, T_TRAPNO
};
/*
 * The non_intr[] array indicates which registers are not available in an
 * interrupt (RS_INTR) register frame.
 */
static boolean_t non_intr[] = {
	B_FALSE, B_TRUE, B_FALSE, B_FALSE, B_TRUE, B_TRUE, B_TRUE, B_TRUE,
	B_FALSE, B_FALSE, B_TRUE, B_TRUE, B_TRUE
};

static void c_pbrk();

static int doname();


static void
push(n)
    unsigned n;
{
    if ((DBSTKSIZ - (dbtos + n)) <  1) {
	dberror("stack overflow on push");
	return;
    }
    dbtos += n;
}


static void
pop(n)
    unsigned n;
{
    if (n > dbtos) {
	dberror("not enough items on stack to pop");
	return;
    }
    while (n-- > 0) {
	dbtos--;
	if (dbstack[dbtos].type == STRING)
	    dbstrfree(dbstack[dbtos].value.string);
	dbstack[dbtos].type = NULL;
    }
}


static void
dbprintitem(struct item *ip, int raw)
{
    ushort_t t = ip->type;

    if (t > TYPEMAX) {
	dbprintf("*** logic error - illegal item type number = %x\n", t);
	return;
    }
    if (dbverbose && !raw)
	dbprintf("%s = ", typename[t]);

    switch (t) {
    case (int) NULL:
	break;
    case NUMBER:
	dbprintf(outformat, (ullong_t)ip->value.number);
	break;
    case STRING:
	if (!raw) {
	    dbprintf("\"%s\"", ip->value.string);
	    break;
	}
	/*FALLTHROUGH*/
    case NAME:
	dbprintf("%s", ip->value.string);
	break;
    }
    if (!raw)
	dbprintf("\n");
}


/* ARGSUSED */
static int
c_read(arg)
{
	union u_val {
		u_char	c_val;
		u_short	s_val;
		u_long	l_val;
		ullong_t ll_val;
	} val;

	db_cur_addr.a_addr = dbstack[dbtos-1].value.number;
	if (db_read(db_cur_addr, &val, db_cur_size) == -1) {
		dberror("invalid address");
		return -1;
	}
	switch (db_cur_size) {
	case 1:
		dbstack[dbtos-1].value.number = val.c_val;
		break;
	case 2:
		dbstack[dbtos-1].value.number = val.s_val;
		break;
	case 4:
		dbstack[dbtos-1].value.number = val.l_val;
		break;
	case 8:
		dbstack[dbtos-1].value.number = val.ll_val;
		break;
	}
	return 0;
}

/* ARGSUSED */
static void
c_write(arg)
{
	union u_val{
		u_char	c_val;
		u_short	s_val;
		u_long	l_val;
		ullong_t ll_val;
	}val;

	switch (db_cur_size) {
	case 1:
		val.c_val = (uchar_t)dbstack[dbtos-2].value.number;
		break;
	case 2:
		val.s_val = (ushort_t)dbstack[dbtos-2].value.number;
		break;
	case 4:
		val.l_val = (ulong_t)dbstack[dbtos-2].value.number;
		break;
	case 8:
		val.ll_val = dbstack[dbtos-2].value.number;
		break;
	}
	db_cur_addr.a_addr = dbstack[dbtos-1].value.number;
	if (db_write(db_cur_addr, &val, db_cur_size) == -1) {
		dberror("invalid address");
		return;
	}
	dbtos -= 2;
}

/* ARGSUSED */
static void
c_findsym(arg)
{
	findsymname(dbstack[--dbtos].value.number, dbtellsymname);
}

/* ARGSUSED */
static void
c_op_not(arg)
{
	dbstack[dbtos-1].value.number = !dbstack[dbtos-1].value.number;
}

/* ARGSUSED */
static void
c_op_incr(arg)
{
	dbstack[dbtos-1].value.number++;
}

/* ARGSUSED */
static void
c_op_decr(arg)
{
	dbstack[dbtos-1].value.number--;
}

/* ARGSUSED */
static void
c_op_mul(arg)
{
	dbstack[dbtos-2].value.number *= dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_div(arg)
{
	dbstack[dbtos-2].value.number /= dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_mod(arg)
{
	dbstack[dbtos-2].value.number %= dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_plus(arg)
{
	dbstack[dbtos-2].value.number += dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_minus(arg)
{
	dbstack[dbtos-2].value.number -= dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_rshift(arg)
{
	unsigned	n = dbstack[dbtos-1].value.number & 0x1f;

	dbstack[dbtos-2].value.number >>= n;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_lshift(arg)
{
	unsigned	n = dbstack[dbtos-1].value.number & 0x1f;

	dbstack[dbtos-2].value.number <<= n;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_lt(arg)
{
	dbstack[dbtos-2].value.number =
		dbstack[dbtos-2].value.number < dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_gt(arg)
{
	dbstack[dbtos-2].value.number =
		dbstack[dbtos-2].value.number > dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_eq(arg)
{
	dbstack[dbtos-2].value.number =
		dbstack[dbtos-2].value.number == dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_ne(arg)
{
	dbstack[dbtos-2].value.number =
		dbstack[dbtos-2].value.number != dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_and(arg)
{
	dbstack[dbtos-2].value.number &= dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_xor(arg)
{
	dbstack[dbtos-2].value.number ^= dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_or(arg)
{
	dbstack[dbtos-2].value.number |= dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_log_and(arg)
{
	dbstack[dbtos-2].value.number =
		dbstack[dbtos-2].value.number && dbstack[dbtos-1].value.number;
	--dbtos;
}

/* ARGSUSED */
static void
c_op_log_or(arg)
{
	dbstack[dbtos-2].value.number =
		dbstack[dbtos-2].value.number || dbstack[dbtos-1].value.number;
	--dbtos;
}

static void
c_assign(arg)	/* assignment to a variable (or macro) */
{
	int	i, n;
	char	*s;

	i = dbgetitem(&dbstack[dbtos]); /* get variable name*/
	if (dbverbose) {
	    dbprintf("%x: ", dbtos);
	    dbprintitem(&dbstack[dbtos], 0);
	}
	if (i == EOF) {
	    exit_db = 1;
	    return;
	}
	if (dbtypecheck(dbtos, NAME))
	    return;
	s = dbstack[dbtos].value.string;
	if (doname(s, 1)) {
	    dberror("name already used");
	    return;
	}
	if (arg == VAR_MACRO && dbtypecheck(dbtos - 1, STRING))
	    return;
	for (i = 0, n = VARTBLSIZ; i < VARTBLSIZ; i++) {
	    if (variable[i].name == NULL) {
		if (n == VARTBLSIZ)
		    n = i;          /* save free table slot */
	    }
	    else if (strcmp(variable[i].name, s) == 0)    /* name found */
		break;
	}
	if (i == VARTBLSIZ) {       /* name not found */
	    if (n == VARTBLSIZ) {
		dberror("variable table overflow");
		return;
	    }
	    i = n;                  /* new slot */
	    variable[i].name = dbstrdup(s);
	}
	else if (variable[i].item.type == STRING)   /* existing string */
	    dbstrfree(variable[i].item.value.string); /* free string */
	variable[i].item = dbstack[dbtos-1];
	variable[i].type = arg;
	dbtos--;
}

static void
c_print(arg)
{
	if (dbverbose)
		dbprintf("%x: ", dbtos-1);
	dbprintitem(&dbstack[dbtos-1], arg);
	if (arg)
		pop(1);
}

/* ARGSUSED */
static void
c_print_n(arg)
{
	int	i, n_vals;

	n_vals = dbstack[dbtos-1].value.number;
	if (n_vals < 0) {
		dberror("negative number of arguments");
		return;
	}
	if (dbstackcheck(n_vals + 1, 0))
		return;
	for (i = n_vals; i-- > 0;) {
		dbprintitem(&dbstack[dbtos - i - 2], 1);
		if (kdb_output_aborted)
			break;
	}
	dbtos -= n_vals + 1;
}

static void
c_pop(arg)
{
	pop(arg ? arg : dbtos);
}

/* ARGSUSED */
static void
c_dup(arg)
{
	dbstack[dbtos] = dbstack[dbtos-1];
	if (dbstack[dbtos].type == STRING) {
		char	*s = dbstrdup(dbstack[dbtos].value.string);

		dbstack[dbtos].value.string = s;
	}
	dbtos++;
}

static void
c_verbose(arg)
{
	dbverbose = (char)arg;
}

static void
c_inbase(arg)
{
	if (arg == 0) {
		arg = dbstack[--dbtos].value.number;
		if (arg < 2 || arg > 16) {
			dberror("illegal input base value - "
					"2 thru 16 accepted");
			return;
		}
	}
	dbibase = (ushort_t)arg;
}

static void
c_outbase(arg)
{
	if (arg == 0)
		arg = dbstack[--dbtos].value.number;
	switch (arg) {
	case 8:
		outformat = "%Lo";  break;
	case 10:
		outformat = "%Ld";  break;
	case 16:
		outformat = "%Lx";  break;
	default:
		dberror("illegal output base value - 8, 10, and 16 accepted");
		return;
	}
}

/* ARGSUSED */
static void
c_dumpstack(arg)
{
	register u_int	i;

	for (i = 0; i < dbtos; i++) {
		if (dbverbose)
			dbprintf("%x: ", i);
		dbprintitem(&dbstack[i], 0);
		if (kdb_output_aborted)
			break;
	}
}

/* ARGSUSED */
static void
c_vars(arg)
{
	register u_int	i;

	for (i = 0; i < VARTBLSIZ; i++) {
		if (variable[i].name == NULL)
			continue;
		if (dbverbose)
			dbprintf("%x: ", i);
		dbprintf("%s %s ", variable[i].name,
			 variable[i].type == VAR_MACRO ? "::" : "=");
		dbprintitem(&variable[i].item, 0);
		if (kdb_output_aborted)
			break;
	}
}

/* ARGSUSED */
static void
c_dump(arg)
{
	db_cur_addr.a_addr = dbstack[dbtos-2].value.number;
	db_dump(db_cur_addr, dbstack[dbtos-1].value.number);
	dbtos -= 2;
}

/* ARGSUSED */
static void
c_fdump(arg)
{
	db_cur_addr.a_addr = dbstack[dbtos-3].value.number;
	db_fdump(&db_cur_addr, dbstack[dbtos-2].value.number,
		 dbstack[dbtos-1].value.string);
	dbstrfree(dbstack[dbtos-1].value.string);
	dbtos -= 3;
}

static void
c_stacktrace(arg)
{
	switch (arg) {
	case 0:
		db_lstack((lwp_t *)-1, (void (*)())dbprintf);
		break;
	case 1:
		{ lwp_t *lwp = (lwp_t *)dbstack[--dbtos].value.number;

			if ((vaddr_t)lwp < kvbase && dbstackcheck(1, 0) == 0 &&
			    dbtypecheck(dbtos - 1, NUMBER) == 0) {
				db_pstack(
				    (proc_t *)dbstack[--dbtos].value.number,
				    (k_lwpid_t)lwp,
				    (void (*)())dbprintf);
			} else
				db_lstack(lwp, (void (*)())dbprintf);
		}
		break;
	case 2:
		db_pstack((proc_t *)dbstack[--dbtos].value.number, 0,
			  (void (*)())dbprintf);
		break;
	case 3:
		db_tstack(dbstack[--dbtos].value.number, (void (*)())dbprintf);
		break;
	}
}

/* ARGSUSED */
static void
c_stackargs(arg)
{
	if (dbstack[--dbtos].value.number >= 0)
		st_max_args = dbstack[dbtos].value.number;
	else
		dberror("value for stackargs must not be negative");
}


/* ARGSUSED */
static void
c_cr0(arg)
{
#ifndef UNIPROC
	if (db_cur_cpu != l.eng_num) {
		db_slave_cmd(db_cur_cpu, DBSCMD_CR0);
		dbstack[dbtos].value.number = db_slave_command.rval;
	} else
#endif /* not UNIPROC */
		dbstack[dbtos].value.number = db_cr0();
	dbstack[dbtos++].type = T_NUMBER;
}

/* ARGSUSED */
static void
c_cr2(arg)
{
#ifndef UNIPROC
	if (db_cur_cpu != l.eng_num) {
		db_slave_cmd(db_cur_cpu, DBSCMD_CR2);
		dbstack[dbtos].value.number = db_slave_command.rval;
	} else
#endif /* not UNIPROC */
		dbstack[dbtos].value.number = db_cr2();
	dbstack[dbtos++].type = T_NUMBER;
}

/* ARGSUSED */
static void
c_cr3(arg)
{
#ifndef UNIPROC
	if (db_cur_cpu != l.eng_num) {
		db_slave_cmd(db_cur_cpu, DBSCMD_CR3);
		dbstack[dbtos].value.number = db_slave_command.rval;
	} else
#endif /* not UNIPROC */
		dbstack[dbtos].value.number = db_cr3();
	dbstack[dbtos++].type = T_NUMBER;
}

/* ARGSUSED */
static void
c_cr4(arg)
{
#ifndef UNIPROC
	if (db_cur_cpu != l.eng_num) {
		db_slave_cmd(db_cur_cpu, DBSCMD_CR4);
		dbstack[dbtos].value.number = db_slave_command.rval;
	} else
#endif /* not UNIPROC */
		dbstack[dbtos].value.number = db_safe_cr4();
	dbstack[dbtos++].type = T_NUMBER;
}

static int
_load_regset()
{
	if (db_cur_as != AS_UVIRT)
		db_as_number = -1;
	db_lstack((lwp_t *)db_as_number, (void (*)())NULL);

	if (regset[db_cur_rset] == NULL) {
		dberror("no such register set");
		return -1;
	}
	return 0;
}

static void
c_getreg(arg)
{
	int reg_idx = arg & ~REG_TYPE;

	if (_load_regset() == -1)
		return;

	if (reg_idx == DB_ESP) {
		/* Special case, since not explicitly saved */
		int ktrap = (!(regset[db_cur_rset][T_EFL] & PS_VM) &&
			     (regset[db_cur_rset][T_CS] & RPL_MASK) ==
			      KERNEL_RPL);

		dbstack[dbtos].value.number =
			(int)&regset[db_cur_rset][(ktrap ? T_EFL : T_SS) + 1];
	} else {
		if (regset_type[db_cur_rset] == RS_INTR && non_intr[reg_idx]) {
			dberror("no such register in interrupt frame");
			return;
		}
		reg_idx = db_to_r0_reg[reg_idx];
		dbstack[dbtos].value.number = regset[db_cur_rset][reg_idx];
	}
	switch (arg & REG_TYPE) {
	case REG_X:
		dbstack[dbtos].value.number &= 0xFFFF;
		break;
	case REG_H:
		dbstack[dbtos].value.number &= 0xFF00;
		dbstack[dbtos].value.number >>= 8;
		break;
	case REG_L:
		dbstack[dbtos].value.number &= 0xFF;
		break;
	}
	dbstack[dbtos++].type = T_NUMBER;
}

static void
c_setreg(arg)
{
	unsigned	val = dbstack[--dbtos].value.number;
	int		reg_idx = (arg & ~REG_TYPE);

	if (_load_regset() == -1)
		return;
	if (regset_type[db_cur_rset] == RS_INTR && non_intr[reg_idx]) {
		dberror("no such register in interrupt frame");
		return;
	}

	reg_idx = db_to_r0_reg[reg_idx];
	switch (arg & REG_TYPE) {
	case REG_E:
		regset[db_cur_rset][reg_idx] = val;
		break;
	case REG_X:
		*(short *)&regset[db_cur_rset][reg_idx] = (short)val;
		break;
	case REG_H:
		*((char *)&regset[db_cur_rset][reg_idx] + 1) = (char)val;
		break;
	case REG_L:
		*(char *)&regset[db_cur_rset][reg_idx] = (char)val;
		break;
	}
}

/* ARGSUSED */
static void
c_getfs(arg)
{
	dbstack[dbtos].value.number = db_fs();
	dbstack[dbtos++].type = T_NUMBER;
}

/* ARGSUSED */
static void
c_getgs(arg)
{
	dbstack[dbtos].value.number = db_gs();
	dbstack[dbtos++].type = T_NUMBER;
}

/* ARGSUSED */
static void
c_setfs(arg)
{
	db_wfs(dbstack[--dbtos].value.number);
}

/* ARGSUSED */
static void
c_setgs(arg)
{
	db_wgs(dbstack[--dbtos].value.number);
}

/* ARGSUSED */
static void
c_getipl(arg)
{
	dbstack[dbtos].value.number = db_entry_ipl[db_cur_cpu];
	dbstack[dbtos++].type = T_NUMBER;
}


static void
c_brk(arg)
{
	unsigned	brknum;
	struct brkinfo	*brkp;
	char		*cmds;

	if (arg == BRK_MYNUM) {
		brknum = dbstack[--dbtos].value.number;
		if (brknum > MAX_UBRKNUM) {
			dberror("breakpoint number too big");
			return;
		}
	} else {
		/* Find first unused breakpoint */
		for (brknum = 0; brknum <= MAX_UBRKNUM; ++brknum) {
			if (db_brk[brknum].state == BRK_CLEAR)
				break;
		}
		if (brknum > MAX_UBRKNUM) {
			dberror("all breakpoints in use");
			return;
		}
	}

	brkp = &db_brk[brknum];

	if (dbstack[dbtos-1].type == STRING) {
		if (dbstackcheck(2, 0))
			return;
		cmds = dbstack[--dbtos].value.string;
	} else
		cmds = NULL;

	if (dbtypecheck(dbtos - 1, NUMBER)) {
		if (cmds)
			++dbtos;
		return;
	}
	brkp->addr = dbstack[--dbtos].value.number;

	if (brkp->cmds != NULL)
		dbstrfree(brkp->cmds);
	brkp->cmds = cmds;

	if (cur_brk_type == BRK_IO && !(l.cpu_features[0] & CPUFEAT_DE)) {
		dbprintf("This CPU does not support I/O breakpoints\n");
		brkp->state = BRK_CLEAR;
		if (cmds) {
			brkp->cmds = NULL;
			++dbtos;
		}
		return;
	}
	if ((brkp->type = (char)cur_brk_type) != BRK_INST)
		brkp->type |= (db_cur_size - 1) << 2;

	brkp->state = BRK_ENABLED;
	brkp->tcount = 0;

	c_pbrk(brknum);

	if (arg == BRK_RETNUM) {
		dbstack[dbtos].value.number = brknum;
		dbstack[dbtos++].type = NUMBER;
	}
}

/* ARGSUSED */
static void
c_trace(arg)
{
	unsigned	brknum;

	brknum = dbstack[--dbtos].value.number;
	if (brknum > MAX_UBRKNUM) {
		dberror("breakpoint number too big");
		return;
	}
	db_brk[brknum].tcount = dbstack[--dbtos].value.number;
	c_pbrk(brknum);
}

static void
_brkstate(brknum, state)
	unsigned	brknum, state;
{
	static struct brkinfo	null_brk;
	struct brkinfo	*brkp = &db_brk[brknum];

	if (state == BRK_CLEAR) {
		if (brkp->cmds != NULL)
			dbstrfree(brkp->cmds);
		*brkp = null_brk;
	} else if (brkp->state != BRK_CLEAR)
		brkp->state = (char)state;
}

static void
c_brkstate(arg)
{
	unsigned	brknum;

	brknum = dbstack[--dbtos].value.number;
	if (brknum > MAX_UBRKNUM) {
		dberror("breakpoint number too big");
		return;
	}
	_brkstate(brknum, arg);
	c_pbrk(brknum);
}

static void
c_allbrkstate(arg)
{
	unsigned	brknum;

	for (brknum = 0; brknum <= MAX_UBRKNUM; brknum++)
		_brkstate(brknum, arg);
}

void
db_clearbrks(void)
{
	c_allbrkstate(BRK_CLEAR);
}

static void
c_addrbrkstate(arg)
{
	unsigned	brknum;
	u_long		addr = dbstack[--dbtos].value.number;

	for (brknum = 0; brknum <= MAX_UBRKNUM; brknum++) {
		if (db_brk[brknum].state != BRK_CLEAR &&
		    db_brk[brknum].addr == addr)
			_brkstate(brknum, arg);
	}
}

static void
c_pbrk(arg)
{
	unsigned	brknum, endnum;
	struct brkinfo	*brkp;
	u_long		addr, sym_addr;
	char		*name;
	unsigned	any_set = 0;

	if (arg == -1) {
		brknum = 0;
		endnum = MAX_UBRKNUM;
	} else
		brknum = endnum = arg;

	do {
		brkp = &db_brk[brknum];

		if (arg == -1 && brkp->state == BRK_CLEAR)
			continue;

		++any_set;

		addr = brkp->addr;
		dbprintf("%d: 0x%lx", brknum, addr);
		if ((name = findsymname(addr, NULL)) != NULL) {
			dbprintf("(%s", name);
			if ((sym_addr = findsymaddr(name)) != addr)
				dbprintf("+0x%lx", addr - sym_addr);
			dbprintf(")");
		}

		switch (brkp->state) {
		case BRK_CLEAR:
			dbprintf(" OFF ");
			break;
		case BRK_ENABLED:
			dbprintf(" ON ");
			break;
		case BRK_DISABLED:
			dbprintf(" DISABLED ");
			break;
		}
		if (brkp->tcount)
			dbprintf(" 0x%x ", brkp->tcount);

		switch (brkp->type & 3) {
		case BRK_IO:
			dbprintf("/io");
			break;
		case BRK_INST:
			dbprintf("/i");
			break;
		case BRK_MODIFY:
			dbprintf("/m");
			break;
		case BRK_ACCESS:
			dbprintf("/a");
			break;
		}

		if (brkp->type != BRK_INST) {
			switch (brkp->type & 0xC) {
			case 0:
				dbprintf("b");
				break;
			case 4:
				dbprintf("w");
				break;
			case 0xC:
				dbprintf("l");
				break;
			}
		}

		if (brkp->cmds)
			dbprintf(" \"%s\" ", brkp->cmds);

		dbprintf("\n");

	} while (brknum++ != endnum);

	if (!any_set)
		dbprintf("No breakpoints set\n");
}

/* ARGSUSED */
static void
c_curbrk(arg)
{
	extern int	db_brknum;

	dbstack[dbtos].value.number = db_brknum;
	dbstack[dbtos++].type = T_NUMBER;
}

static void
c_vtop(arg)
{
	ulong_t	vaddr;
	paddr64_t	paddr;

	if ((db_cur_as = (db_as_t)arg) == AS_UVIRT) {
		const proc_t	*proc;
		int	n = dbstack[--dbtos].value.number;

		if ((proc = PSLOT2PROC(n)) == NULL) {
			dberror("no such process");
			return;
		}
		vaddr = dbstack[--dbtos].value.number;
		paddr = db_uvtop(vaddr, proc);
		dbprintf("User Process %x", n);
	} else { /* db_cur_as == AS_KVIRT */
		vaddr = dbstack[--dbtos].value.number;
		paddr = DB_KVTOP(vaddr, db_cur_cpu);
		dbprintf("Kernel");
	}
	dbprintf(" Virtual %x ", vaddr);
	if (paddr == (paddr64_t)-1)
		dbprintf("not mapped\n");
	else {
		if(PAE_ENABLED()) {
			dbprintf("= Physical %Lx\n", paddr);
		} else {
			dbprintf("= Physical %x\n", (ulong_t)paddr);
		}
	}
}

/* ARGSUSED */
static void
c_ps(arg)
{
	db_ps();
}

/* ARGSUSED */
static void
c_newterm(arg)
{
	char	*devname;
	minor_t	minor;

	devname = dbstack[dbtos-2].value.string;
	minor = dbstack[dbtos-1].value.number;
	dbtos -= 2;

	if (!kdb_select_io(devname, minor))
		dberror("no such console device");
}

/* ARGSUSED */
static void
c_newdebug(arg)
{
	kdb_next_debugger();
}

/* ARGSUSED */
static void
c_dis(arg)
{
	db_cur_addr.a_addr = dbstack[dbtos-2].value.number;
	db_dis(db_cur_addr, dbstack[dbtos-1].value.number);
	dbtos -= 2;
}

static void
c_single(arg)
{
	if (arg == 0)
		arg = dbstack[--dbtos].value.number;
	if (arg) {
		if (db_ex_frame == NULL) {
			dberror("Can't single-step without a register frame");
			return;
		}
	}
	if ((dbsingle = arg) != 0)
		exit_db = 1;
	db_pass_calls = 0;
}

static void
c_bs(arg)
{
	if (!p6_debug()) {
			dbprintf("Branch single step not supported by this CPU\n");
			return;
			}
	if (arg == 0)
		arg = dbstack[--dbtos].value.number;
	if (arg) {
		if (db_ex_frame == NULL) {
			dberror("Can't branch-step without a register frame");
			return;
		}
	}
	if ((dbbranch = arg) != 0)
		exit_db = 1;
	db_pass_calls = 0;
}

static void
lbr_print_sym(u_long addr)
{
	char *name;
	u_long sym_addr;

	if ((name = findsymname(addr, NULL)) != NULL) {
		dbprintf("%s", name);
		if ((sym_addr = findsymaddr(name)) != addr)
			dbprintf("+0x%lx", addr - sym_addr);
	} else {
		dbprintf("0x%08lx", addr);
	}
}

/*
 *  Print the from- and to- EIP values for the
 *  branch record, symbolically if possible.
 *
 *  Assumes that arg has already been sanity checked.
 */

void
lbr_print_from_to(arg)
{
	ulong_t tmp[2];

	read_brec(tmp, arg);

	dbprintf(" from ");
	lbr_print_sym(tmp[0]);
	dbprintf(" to ");
	lbr_print_sym(tmp[1]);
	dbprintf("\n");
}

static void
c_lbr(arg)
{
	int num_lbr;

	if (!p6_debug()) {
		dbprintf("%s records not supported by this CPU\n",
			 (arg ? "Branch" : "Interrupt"));
		return;
	}
	num_lbr = get_numlbr();
	if (arg) {
		while (num_lbr-- != 0) {
			dbprintf("Branch");
			lbr_print_from_to(num_lbr);
		}
	}  else {
		dbprintf("Interrupt");
		lbr_print_from_to(num_lbr);
	}
}

static void
c_pass_calls(arg)
{
	c_single(arg);
	if (dbsingle)
		db_pass_calls = 1;
}

/* ARGSUSED */
static void
c_call(arg)
{
	int	(*func)();
	int	n_args, ret;

	func = (int (*)())dbstack[dbtos-2].value.number;
	n_args = dbstack[dbtos-1].value.number;
	if (n_args < 0) {
		dberror("negative number of arguments");
		return;
	}
	if (n_args > 8) {
		dberror("more than 8 arguments not supported");
		return;
	}
	if (dbstackcheck(n_args + 2, 0))
		return;
	dbtos -= n_args + 2;
	ret = (*func)((long)dbstack[dbtos].value.number,
		      (long)dbstack[dbtos+1].value.number,
		      (long)dbstack[dbtos+2].value.number,
		      (long)dbstack[dbtos+3].value.number,
		      (long)dbstack[dbtos+4].value.number,
		      (long)dbstack[dbtos+5].value.number,
		      (long)dbstack[dbtos+6].value.number,
		      (long)dbstack[dbtos+7].value.number);
	/* don't trust the function to not have changed the ipl */
	(void) splhi();
	/* free strings */
	while (n_args-- != 0) {
		if (dbstack[dbtos + n_args].type == STRING)
			dbstrfree(dbstack[dbtos + n_args].value.string);
	}
	/* push function return value */
	dbstack[dbtos].value.number = ret;
	dbstack[dbtos++].type = NUMBER;
}

/* ARGSUSED */
static void
c_lcall(arg)
{
	int	(*func)();
	int	n_args, ret;

	func = (int (*)())dbstack[dbtos-2].value.number;
	n_args = dbstack[dbtos-1].value.number;
	if (n_args < 0) {
		dberror("negative number of arguments");
		return;
	}
	if (n_args > 8) {
		dberror("more than 8 arguments not supported");
		return;
	}
	if (dbstackcheck(n_args + 2, 0))
		return;
	dbtos -= n_args + 2;
	ret = (*func)((llong_t)dbstack[dbtos].value.number,
		      (llong_t)dbstack[dbtos+1].value.number,
		      (llong_t)dbstack[dbtos+2].value.number,
		      (llong_t)dbstack[dbtos+3].value.number,
		      (llong_t)dbstack[dbtos+4].value.number,
		      (llong_t)dbstack[dbtos+5].value.number,
		      (llong_t)dbstack[dbtos+6].value.number,
		      (llong_t)dbstack[dbtos+7].value.number);
	/* don't trust the function to not have changed the ipl */
	(void) splhi();
	/* free strings */
	while (n_args-- != 0) {
		if (dbstack[dbtos + n_args].type == STRING)
			dbstrfree(dbstack[dbtos + n_args].value.string);
	}
	/* push function return value */
	dbstack[dbtos].value.number = ret;
	dbstack[dbtos++].type = NUMBER;
}

/* ARGSUSED */
static void
c_then(arg)
{
	int	t;

	if (dbstack[--dbtos].value.number) {
		/* "True" case - just return and execute next cmd(s) */
		return;
	}

	/* "False" case - ignore all input up to an "endif" or end of "line" */
	db_hard_eol = 1;
	for (;;) {
		t = dbgetitem(&dbstack[dbtos]);
		if (dbverbose) {
			dbprintf("%x: ", dbtos);
			dbprintitem(&dbstack[dbtos], 0);
			if (kdb_output_aborted)
				break;
		}
		if (t == NAME) {
			if (strcmp(dbstack[dbtos].value.string, "endif") == 0)
				break;
		} else if (t == STRING)
			dbstrfree(dbstack[dbtos].value.string);
		else if (t == EOL)
			break;
		else if (t == EOF) {
			exit_db = 1;
			break;
		}
	}
	db_hard_eol = 0;
}

/* ARGSUSED */
static void
c_endif(arg)
{
	/* No-op - place marker for end of "then" scope */
}

/* ARGSUSED */
static void
c_quit(arg)
{
	db_flush_input();
	exit_db = 1;
}

static char *cmds_help_text[] = {
"Value stack:   clrstk dup P p PP pop stk",
"Arithmetic:    + - * / % >> << > < == != & | ^ && || ! ++ --",
"Read/Write:    dump fdump r w",
"Get Registers: %<reg> cr[0|2|3|4]		Set Registers: w%<reg>",
"   <reg> is one of: eax ax ah al ebx bx bh bl ecx cx ch cl edx dx dh dl efl fl",
"                    esi si edi di ebp bp esp sp eip ip cs ds es fs gs trap ipl",
"Variables:     = :: vars",
"Breakpoints:   B b bn brkoff brkon brksoff brkson clraddrbrks clrbrk clrbrks",
"               curbrk trace ?brk",
"Single Step:   S s SS ss",
"Branch Step:   bs bss",
"Last Branch:   lbr",
"Last Interrupt:lint",
"Process Info:  lstack ps pstack stack stackargs tstack",
"Numeric Base:  ibase ibinary idecimal ihex ioctal obase odecimal ohex ooctal",
"Address Space: kvtop uvtop",
"Help:          cmds help",
"Flow Control:  endif then q",
"Miscellaneous: call lcall vcall dis findsym nonverbose newdebug newterm verbose",
"",
"Suffixes:      /l = 4-byte long (default), /w = 2-byte word, /b = byte, ",
"               /L = 8-byte long long",
"               / or /k = kernel virtual (default), /p = physical, /io = I/O",
#ifdef UNIPROC
"               /u# = user process # virtual, /rs# = register-set #",
#else
"               /u# = user process # virtual, /rs# = register-set #, /c# = cpu #",
#endif
"               breakpoint: /a = data access, /m = data modify, /i = instruction",
NULL
};
static char *main_help_text[] = {
"cmds			display a list of all commands",
"{ARGS} ADDR COUNT call	call the function at ADDR with COUNT arguments",
"{ARGS} ADDR COUNT lcall	as call, but with long long arguments",
"ADDR COUNT dump		show COUNT bytes starting at ADDR",
"ADDR COUNT FMT fdump	show COUNT formatted items at ADDR w/format string FMT",
"ADDR r			read 1, 2 or 4 bytes from virtual address ADDR",
"VAL ADDR w		write VAL to virtual address ADDR (1, 2 or 4 bytes)",
"ADDR [STRING] NUM B	set breakpoint #NUM at ADDR w/optional command string",
"			use suffixes /a for access or /m for modify breakpoints",
"ADDR [STRING] b		same as \"B\", but use first free breakpoint #",
"NUM brkoff		temporarily disable breakpoint #NUM",
"NUM brkon		re-enable breakpoint #NUM",
"COUNT NUM trace		skip over COUNT instances of breakpoint #NUM",
"s			single step one instruction (\"S\" passes over calls)",
"COUNT ss		single step COUNT instructions (\"SS\" passes over calls)",
"bs			branch step: execute until a branch instruction",
"COUNT bss		branch step COUNT branch instructions",
"lint			show FROM and TO address for last interrupt",
"lbr			show FROM and TO address for last branch(es)",
"			where the number of branches is processor dependent",
"stack			show stack trace of current LWP (same as \"-1 lstack\")",
"PROC pstack		show stack trace of process PROC (slot # or procp)",
"LWP lstack		show stack trace of lwp LWP (-1 for current LWP)",
"PROC LWPID lstack	show stack trace of lwp #LWPID in proc PROC",
"ADDR findsym		print name of symbol nearest ADDR",
"ADDR COUNT dis		disassemble COUNT instructions starting at ADDR",
"VAL = VAR		store VAL in a variable named VAR",
"STRING :: VAR		define variable VAR as a macro of commands from STRING",
"STRING NUM newterm	switch to device STRING minor NUM for terminal I/O",
"q -or- ^D		exit from debugger",
NULL
};
static char **help_text[] = {
	main_help_text,
	cmds_help_text
};

static void
c_help(arg)
{
	register char	**msgp = help_text[arg];

	dbprintf("\n");
	while (*msgp != NULL) {
		dbprintf("%s\n", *msgp++);
		if (kdb_output_aborted)
			return;
	}
	dbprintf("\n");
}


struct cmdentry cmdtable[] = {

{ "r",		(void (*)())c_read,	0,	S_1_0,	1,	T_NUMBER },
{ "w",		c_write,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "!",		c_op_not,	0,	S_1_0,	1,	T_NUMBER },
{ "++",		c_op_incr,	0,	S_1_0,	1,	T_NUMBER },
{ "--",		c_op_decr,	0,	S_1_0,	1,	T_NUMBER },
{ "*",		c_op_mul,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "/",		c_op_div,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "%",		c_op_mod,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "+",		c_op_plus,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "-",		c_op_minus,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ ">>",		c_op_rshift,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "<<",		c_op_lshift,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "<",		c_op_lt,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ ">",		c_op_gt,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "==",		c_op_eq,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "!=",		c_op_ne,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "&",		c_op_and,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "^",		c_op_xor,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "|",		c_op_or,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "&&",		c_op_log_and,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "||",		c_op_log_or,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "=",		c_assign,	VAR_VAR,	S_1_0,	0 },
{ "::",		c_assign,	VAR_MACRO,	S_1_0, 	0 },
{ "p",		c_print,	0,	S_1_0,	0 },
{ "P",		c_print,	1,	S_1_0,	0 },
{ "PP",		c_print_n,	0,	S_1_0,	1,	T_NUMBER },
{ "pop",	c_pop,		1,	0,	0 },
{ "clrstk",	c_pop,		0,	0,	0 },
{ "dup",	c_dup,		0,	S_0_1,	0 },
{ "nonverbose",	c_verbose,	0,	0,	0 },
{ "verbose",	c_verbose,	1,	0,	0 },
{ "ibase",	c_inbase,	0,	S_1_0,	1,	T_NUMBER },
{ "ibinary",	c_inbase,	2,	0,	0 },
{ "ioctal",	c_inbase,	8,	0,	0 },
{ "idecimal",	c_inbase,	10,	0,	0 },
{ "ihex",	c_inbase,	16,	0,	0 },
{ "obase",	c_outbase,	0,	S_1_0,	1,	T_NUMBER },
{ "ooctal",	c_outbase,	8,	0,	0 },
{ "odecimal",	c_outbase,	10,	0,	0 },
{ "ohex",	c_outbase,	16,	0,	0 },
{ "stk",	c_dumpstack,	0,	0,	0 },
{ "vars",	c_vars,		0,	0,	0 },
{ "findsym",	c_findsym,	0,	S_1_0,	1,	T_NUMBER },
{ "dump",	c_dump,		0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "fdump",	c_fdump,   0, STACKCHK(3,0), 3, T_STRING, T_NUMBER, T_NUMBER },
{ "stack",	c_stacktrace,	0,	0,	0 },
{ "lstack",	c_stacktrace,	1,	S_1_0,	1,	T_NUMBER },
{ "pstack",	c_stacktrace,	2,	S_1_0,	1,	T_NUMBER },
{ "tstack",	c_stacktrace,	3,	S_1_0,	1,	T_NUMBER },
{ "stackargs",	c_stackargs,	1,	S_1_0,	1,	T_NUMBER },
{ "B",		c_brk,		BRK_MYNUM,	S_2_0,	1,	T_NUMBER },
{ "b",		c_brk,		BRK_ANYNUM,	S_1_0,	0 },
{ "bn",		c_brk,		BRK_RETNUM,	S_1_1,	0 },
{ "brkoff",	c_brkstate,	BRK_DISABLED,	S_1_0,	1,	T_NUMBER },
{ "brkon",	c_brkstate,	BRK_ENABLED,	S_1_0,	1,	T_NUMBER },
{ "brksoff",	c_allbrkstate,	BRK_DISABLED,	0,	0 },
{ "brkson",	c_allbrkstate,	BRK_ENABLED,	0,	0 },
{ "clraddrbrks",c_addrbrkstate,	BRK_CLEAR,	S_1_0,	1,	T_NUMBER },
{ "clrbrk",	c_brkstate,	BRK_CLEAR,	S_1_0,	1,	T_NUMBER },
{ "clrbrks",	c_allbrkstate,	BRK_CLEAR,	0,	0 },
{ "trace",	c_trace,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "?brk",	c_pbrk,		-1,	0,	0 },
{ "curbrk",	c_curbrk,	0,	S_0_1,	0 },
{ "help",	c_help,		0,	0,	0 },
{ "?",		NULL,		(int)"help" },
{ "cmds",	c_help,		1,	0,	0 },
{ "ps",		c_ps,		0,	0,	0 },
{ "newterm",	c_newterm,	0,	S_2_0,	2,	T_NUMBER, T_STRING },
{ "newdebug",	c_newdebug,	0,	0,	0 },
{ "dis",	c_dis,		0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "s",		c_single,	1,	0,	0 },
{ "ss",		c_single,	0,	S_1_0,	1,	T_NUMBER },
{ "S",		c_pass_calls,	1,	0,	0 },
{ "SS",		c_pass_calls,	0,	S_1_0,	1,	T_NUMBER },
{ "%eax",	c_getreg,	DB_EAX+REG_E,	S_0_1,	0 },
{ "%ax",	c_getreg,	DB_EAX+REG_X,	S_0_1,	0 },
{ "%ah",	c_getreg,	DB_EAX+REG_H,	S_0_1,	0 },
{ "%al",	c_getreg,	DB_EAX+REG_L,	S_0_1,	0 },
{ "%ebx",	c_getreg,	DB_EBX+REG_E,	S_0_1,	0 },
{ "%bx",	c_getreg,	DB_EBX+REG_X,	S_0_1,	0 },
{ "%bh",	c_getreg,	DB_EBX+REG_H,	S_0_1,	0 },
{ "%bl",	c_getreg,	DB_EBX+REG_L,	S_0_1,	0 },
{ "%ecx",	c_getreg,	DB_ECX+REG_E,	S_0_1,	0 },
{ "%cx",	c_getreg,	DB_ECX+REG_X,	S_0_1,	0 },
{ "%ch",	c_getreg,	DB_ECX+REG_H,	S_0_1,	0 },
{ "%cl",	c_getreg,	DB_ECX+REG_L,	S_0_1,	0 },
{ "%edx",	c_getreg,	DB_EDX+REG_E,	S_0_1,	0 },
{ "%dx",	c_getreg,	DB_EDX+REG_X,	S_0_1,	0 },
{ "%dh",	c_getreg,	DB_EDX+REG_H,	S_0_1,	0 },
{ "%dl",	c_getreg,	DB_EDX+REG_L,	S_0_1,	0 },
{ "%edi",	c_getreg,	DB_EDI+REG_E,	S_0_1,	0 },
{ "%di",	c_getreg,	DB_EDI+REG_X,	S_0_1,	0 },
{ "%esi",	c_getreg,	DB_ESI+REG_E,	S_0_1,	0 },
{ "%si",	c_getreg,	DB_ESI+REG_X,	S_0_1,	0 },
{ "%ebp",	c_getreg,	DB_EBP+REG_E,	S_0_1,	0 },
{ "%bp",	c_getreg,	DB_EBP+REG_X,	S_0_1,	0 },
{ "%esp",	c_getreg,	DB_ESP+REG_E,	S_0_1,	0 },
{ "%sp",	c_getreg,	DB_ESP+REG_X,	S_0_1,	0 },
{ "%eip",	c_getreg,	DB_EIP+REG_E,	S_0_1,	0 },
{ "%ip",	c_getreg,	DB_EIP+REG_X,	S_0_1,	0 },
{ "%efl",	c_getreg,	DB_EFL+REG_E,	S_0_1,	0 },
{ "%fl",	c_getreg,	DB_EFL+REG_X,	S_0_1,	0 },
{ "%cs",	c_getreg,	DB_CS+REG_X,	S_0_1,	0 },
{ "%ds",	c_getreg,	DB_DS+REG_X,	S_0_1,	0 },
{ "%es",	c_getreg,	DB_ES+REG_X,	S_0_1,	0 },
{ "%fs",	c_getfs,	0,	S_0_1,	0 },
{ "%gs",	c_getgs,	0,	S_0_1,	0 },
{ "%trap",	c_getreg,	DB_TRAPNO+REG_E,	S_0_1,	0 },
{ "%ipl",	c_getipl,	0,	S_0_1,	0 },
{ "w%eax",	c_setreg,	DB_EAX+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%ax",	c_setreg,	DB_EAX+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%ah",	c_setreg,	DB_EAX+REG_H,	S_1_0,	1,	T_NUMBER },
{ "w%al",	c_setreg,	DB_EAX+REG_L,	S_1_0,	1,	T_NUMBER },
{ "w%ebx",	c_setreg,	DB_EBX+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%bx",	c_setreg,	DB_EBX+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%bh",	c_setreg,	DB_EBX+REG_H,	S_1_0,	1,	T_NUMBER },
{ "w%bl",	c_setreg,	DB_EBX+REG_L,	S_1_0,	1,	T_NUMBER },
{ "w%ecx",	c_setreg,	DB_ECX+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%cx",	c_setreg,	DB_ECX+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%ch",	c_setreg,	DB_ECX+REG_H,	S_1_0,	1,	T_NUMBER },
{ "w%cl",	c_setreg,	DB_ECX+REG_L,	S_1_0,	1,	T_NUMBER },
{ "w%edx",	c_setreg,	DB_EDX+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%dx",	c_setreg,	DB_EDX+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%dh",	c_setreg,	DB_EDX+REG_H,	S_1_0,	1,	T_NUMBER },
{ "w%dl",	c_setreg,	DB_EDX+REG_L,	S_1_0,	1,	T_NUMBER },
{ "w%edi",	c_setreg,	DB_EDI+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%di",	c_setreg,	DB_EDI+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%esi",	c_setreg,	DB_ESI+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%si",	c_setreg,	DB_ESI+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%ebp",	c_setreg,	DB_EBP+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%bp",	c_setreg,	DB_EBP+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%eip",	c_setreg,	DB_EIP+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%ip",	c_setreg,	DB_EIP+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%efl",	c_setreg,	DB_EFL+REG_E,	S_1_0,	1,	T_NUMBER },
{ "w%fl",	c_setreg,	DB_EFL+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%cs",	c_setreg,	DB_CS+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%ds",	c_setreg,	DB_DS+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%es",	c_setreg,	DB_ES+REG_X,	S_1_0,	1,	T_NUMBER },
{ "w%fs",	c_setfs,	0,	S_1_0,	1,	T_NUMBER },
{ "w%gs",	c_setgs,	0,	S_1_0,	1,	T_NUMBER },
{ "w%trap",	c_setreg,	DB_TRAPNO+REG_E,	S_1_0,	1,	T_NUMBER },
{ "cr0",	c_cr0,		0,	S_0_1,	0 },
{ "cr2",	c_cr2,		0,	S_0_1,	0 },
{ "cr3",	c_cr3,		0,	S_0_1,	0 },
{ "cr4",	c_cr4,		0,	S_0_1,	0 },
{ "lbr",	c_lbr,		1,	0,	0 },
{ "lint",	c_lbr,		0,	0,	0 },
{ "bs",		c_bs,		1,	0,	0 },
{ "bss",	c_bs,		0,	S_1_0,	1,	T_NUMBER },
{ "call",	c_call,		0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "lcall",	c_lcall,	0,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "vcall",	NULL,		(int)"call pop" },
{ "kvtop",	c_vtop,	(int)AS_KVIRT,	S_1_0,	1,	T_NUMBER },
{ "uvtop",	c_vtop,	(int)AS_UVIRT,	S_2_0,	2,	T_NUMBER, T_NUMBER },
{ "q",		c_quit,		0,	0,	0 },
{ "then",	c_then,		0,	0,	0 },
{ "endif",	c_endif,	0,	0,	0 },
{ NULL }
};

	/* NOTE: suffix names must not be > 3 characters */
struct suffix_entry suffix_table[] = {
{ "k",		AS_KVIRT,	0,	0,		0 },
{ "p",		AS_PHYS,	0,	0,		0 },
{ "io", 	AS_IO,		0,	BRK_IO,		0 },
{ "u",		AS_UVIRT,	0,	0,		SFX_NUM },
{ "b",		0,		1,	0,		0 },
{ "w",		0,		2,	0,		0 },
{ "l",		0,		4,	0,		0 },
{ "L",		0,		8,	0,		0 },
{ "i",		0,		0,	BRK_INST,	0 },
{ "a",		0,		0,	BRK_ACCESS,	0 },
{ "m",		0,		0,	BRK_MODIFY,	0 },
{ "rs",		0,		0,	0,		SFX_NUM|SFX_RSET },
#ifndef UNIPROC
{ "cpu",	0,		0,	0,		SFX_NUM|SFX_CPU },
{ "c",		0,		0,	0,		SFX_NUM|SFX_CPU },
#endif /* not UNIPROC */
{ "" }
};


dbstackcheck(down, up)
    unsigned	down, up;
{
    if (down > dbtos) {
	dberror("not enough items on stack");
	return 1;
    }
    if ((DBSTKSIZ - (dbtos + up)) <  1) {
	dberror("stack overflow");
	return 1;
    }
    return 0;
}

dbtypecheck(x, t)
    unsigned int x, t;
{
    if (dbstack[x].type == t)
	return 0;
    dberror("bad operand type");
    if (t > TYPEMAX || dbstack[x].type > TYPEMAX)
	dbprintf("*** logic error - illegal item type number in typecheck()\n");
    else
	dbprintf("(operand at stack location %x is a %s and should be a %s)\n",
		 x, typename[dbstack[x].type], typename[t]);
    return 1;
}

dbmasktypecheck(x, t)
    unsigned int x, t;
{
    if (t & T_NUMBER && dbstack[x].type == NUMBER) return 0;
    if (t & T_NAME   && dbstack[x].type == NAME)   return 0;
    if (t & T_STRING && dbstack[x].type == STRING) return 0;
    dberror("bad operand type");
    if (!(t & (T_NUMBER|T_NAME|T_STRING)) || dbstack[x].type > TYPEMAX)
	dbprintf("*** logic error - illegal item type number in typecheck()\n");
    else
	dbprintf(
	    "(operand at stack location %x is a %s and should be a %s %s %s)\n",
		 x, typename[dbstack[x].type],
		 t & T_NUMBER ? typename[NUMBER] : "",
		 t & T_NAME   ? typename[NAME]   : "",
		 t & T_STRING ? typename[STRING] : "");
    return 1;
}

static int
dosuffix(s)
    char *s;
{
    register struct suffix_entry *p;
    register char	*s_c, *p_c;

    if (*s == '\0') {
	/* Special case; null suffix */
	db_cur_as = AS_KVIRT;
	return 1;
    }

    do {
	    for (p = suffix_table - 1;;) {
		if ((++p)->name[0] == '\0') {
		    dberror("unknown suffix");
		    return 0;
		}
		for (s_c = s, p_c = p->name; *p_c != '\0'; ++s_c, ++p_c) {
		    if (*s_c != *p_c)
			break;
		}
		if (*p_c == '\0') {
			s = s_c;
			break;
		}
	    }

	    if (p->as)
		db_cur_as = p->as;
	    if (p->size)
		db_cur_size = p->size;
	    if (p->brk)
		cur_brk_type = p->brk;
	    if (p->flags & SFX_NUM) {
		register unsigned	n = 0;
		register char		*dp;
		register char		c;
		extern char		db_xdigit[];

		for (; (c = (*s | LCASEBIT)) != '\0'; ++s) {
		    for (dp = db_xdigit; *dp != '\0' && *dp != c; ++dp)
			;
		    if (*dp == '\0')
			break;
		    n = (n << 4) + (dp - db_xdigit);
		}
#ifndef UNIPROC
		if (p->flags & SFX_CPU) {
		    if (n >= Nengine || !db_cp_active[n]) {
			dberror("non-existent cpu number");
			return 0;
		    }
		    db_cur_cpu = n;
		} else
#endif /* not UNIPROC */
		if (p->flags & SFX_RSET) {
		    if (n >= NREGSET) {
			dberror("illegal register-set number");
			return 0;
		    }
		    db_cur_rset = n;
		} else
		    db_as_number = n;
	    }
    } while (*s != '\0');

    return 1;
}

static int
doname(name, check)
    char *name;     /* name to process */
    int check;      /* if set, just check for existence of name */
{
    register struct cmdentry *p;
    char *s;
    u_int i;

    for (p = cmdtable; p->name != NULL; p++) {
	if (strcmp(name, p->name) == 0) {
		/* Found a command */
	    if (!check) {   /* check -> no action, testing */
		if (p->func == NULL) { /* alias */
		    dbcmdline((char *)p->arg);
		} else
		    /* Check for stack growth limits */
		if (!dbstackcheck(S_DOWN(p->stackcheck),S_UP(p->stackcheck))) {
			/* Check for correct parameter types */
		    for (i = 0; i < p->parmcnt; i++) {
			if (dbmasktypecheck(dbtos-1-i, p->parmtypes[i]))
			    return 1;
		    }
		    (*p->func) (p->arg);
		}
	    }
	    return 1;
	}
    }

    if (check)          /* check only applies to builtin operation names */
	return 0;

    for (i = 0; i < VARTBLSIZ; i++) {
	if ((s = variable[i].name) != NULL && strcmp(name, s) == 0) {
	    if (variable[i].type == VAR_MACRO) {
		dbcmdline(variable[i].item.value.string);
		return 0;
	    }
	    if (dbstackcheck(0, 1))
		return 1;
	    dbstack[dbtos] = variable[i].item;
	    if (dbstack[dbtos].type == STRING &&
		  (dbstack[dbtos].value.string = dbstrdup(dbstack[dbtos].value.string))
		     == NULL)
		return 1;
	    dbtos++;
	    return 0;
	}
    }

    if (dbextname(name))        /* try external names */
	return 0;

    dberror("name not found");
    return 1;
}

void
dbinterp(char *cmds)
{
    short t, do_suffix;
    struct item	suffix;
    char c, *name;

    if (cmds)
	dbcmdline(cmds);

    do {
#ifndef UNIPROC
	db_cur_cpu = AS_CUR_CPU;
#endif
	db_cur_as = 0;
	db_cur_size = 0;
	db_cur_rset = 0;
	do_suffix = 0;
	cur_brk_type = BRK_INST;

	if ((t = dbgetitem(&dbstack[dbtos])) == NAME)
	    name = dbstrdup(dbstack[dbtos].value.string);
	if (dbverbose) {
	    dbprintf("%x: ", dbtos);
	    dbprintitem(&dbstack[dbtos], 0);
	}

	if (t == NAME && name[0] == '/' && name[1] != '\0') {
	    suffix = dbstack[dbtos];
	    t = 0;
	    goto got_suffix;
	}

	if (t == NAME || t == NUMBER) {
	    while (*dbpeek() == '/') {
		/* handle suffix */
		(void) dbgetitem(&suffix);
got_suffix:
		if (suffix.value.string[1] == '\0') {
		    c = *dbpeek();
		    if (c != '\0' && !isspace(c)) {
			dberror("unknown suffix");
			t = do_suffix = 0;
			break;
		    }
		}
		if (!(do_suffix = dosuffix(suffix.value.string + 1))) {
		    t = 0;
		    break;
		}
	    }
	}

	if (db_cur_as == 0)
	    db_cur_as = AS_KVIRT;
	if (db_cur_size == 0)
	    db_cur_size = 4;
#ifndef UNIPROC
	if (db_cur_cpu == AS_CUR_CPU)
	    db_cur_cpu = l.eng_num;
#endif

	switch (t) {
	case EOF:
	    exit_db = 1;
	    break;
	case NUMBER:
	case STRING:
	    push(1);
	    break;
	case NAME:
	    if (doname(name, 0))
		do_suffix = 0;
 	    dbstrfree(name);
	    break;
	}

	if (do_suffix && !dbstackcheck(0, 1)) {
	    if (c_read(0) != -1)
		c_print(0);
	}
    } while (!exit_db);

    exit_db = 0;
}
