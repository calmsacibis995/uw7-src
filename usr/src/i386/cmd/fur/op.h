#ident	"@(#)fur:i386/cmd/fur/op.h	1.4"
#ident	"$Header:"

#include <setjmp.h>
#include "dis.h"

EXTERN char obuf[1024];

struct prreg {
	char *name;
	int regnum;
};

EXTERN int Curstatement;
EXTERN int Targblock;
EXTERN int Retblock;
EXTERN int Curblock;
EXTERN int *Remap_ops;
EXTERN char *Genbuf;
EXTERN int Genspot;
EXTERN int Sgenbuf;
EXTERN int Test_optimization;
EXTERN int Test_generation;
EXTERN int Do_expr_subst;
EXTERN int Status;
EXTERN int Opchanged;
EXTERN int Nvisits;

EXTERN struct instable **Prefix;
EXTERN int Nprefix;

#define GIVE_UP 1
#define SMALL_IMMEDIATE(OPNO) (!(opsinfo[(OPNO)].flags & OPTINFO_REL) && (opsinfo[(OPNO)].u.imm.value < 127) && (opsinfo[(OPNO)].u.imm.value > -128))
#define SMALL_DISPLACEMENT(OPNO) ((opsinfo[(OPNO)].u.disp.value < 127) || (opsinfo[(OPNO)].u.disp.value > -128))
#define REMAP(OPNO) (Remap_ops[OPNO])

#define PSEUDO_REG_NO_REG		(NO_REG)
#define PSEUDO_REG_MEM			(NO_REG+1)
#define PSEUDO_REG_CONSTANT		(NO_REG+2)
#define PSEUDO_REG_SPECIAL		(NO_REG+3)
#define PSEUDO_REG_HOLD_EBX		(NO_REG+4)
#define PSEUDO_REG_HOLD_EBP		(NO_REG+5)
#define PSEUDO_REG_HOLD_ESI		(NO_REG+6)
#define PSEUDO_REG_HOLD_EDI		(NO_REG+7)
#define PSEUDO_REG_EARLY_STACK	(NO_REG+8)

#define IS_PSEUDO_REG(VAR) (((VAR) >= PSEUDO_REG_NO_REG) && ((VAR) <= PSEUDO_REG_EARLY_STACK))
#ifdef DECLARE_GLOBALS
const struct prreg Prreg[] = {
	{ "%eax", EAX },
	{ "%ecx", ECX },
	{ "%edx", EDX },
	{ "%ebx", EBX },
	{ "%esp", ESP },
	{ "%ebp", EBP },
	{ "%esi", ESI },
	{ "%edi", EDI },
	{ "NO_REG", PSEUDO_REG_NO_REG },
	{ "MEMORY", PSEUDO_REG_MEM },
	{ "CONSTANT", PSEUDO_REG_CONSTANT },
	{ "SPECIAL", PSEUDO_REG_SPECIAL },
	{ "EARLY STACK", PSEUDO_REG_EARLY_STACK },
};
#else
EXTERN const struct prreg Prreg[];
#endif

#define PR(VAR) (((VAR) < NBASE) ? Prreg[VAR].name : pr(VAR))
#define NREGS		(EDI+1)
#define NBASE		(PSEUDO_REG_EARLY_STACK+1)

EXTERN int Only_esr;
EXTERN int Before_stack, Last_arg;
EXTERN int Opinit;
EXTERN Elf32_Addr Addr, Next_addr; /* Beginning and end of current
										instruction */

struct varinfo {
#define CONSTANT_TYPE			0
#define ALIAS_TYPE				1
#define NOT_STACK_TYPE			2
#define UNKNOWN_TYPE			3
#define REL_CONSTANT_TYPE		4
	unchar type;
#define READ_FLAG				(1<<0)
#define WRITE_FLAG				(1<<1)
#define WATCH_READ				(1<<2)
#define WATCH_WRITE				(1<<3)
#define BARRIER					(1<<4)
#define ANDBACK_FLAG			(1<<5)
#define SAW_READ				(1<<6)
#define SAW_WRITE				(1<<7)
#define ELIM_ME					(1<<8)
#define INDEXED_FLAG			(1<<9)
#define LOCATION_UNNEEDED		(1<<10)
#define VALUE_UNNEEDED			(1<<11)
#define VALUE_UNKNOWN_TO_CALLEE	(1<<12)
#define POPPED					(1<<13)
#define VAR_IS_BLOCKNO			(1<<14)
#define ON_IS_CONSERVATIVE		(READ_FLAG|WRITE_FLAG|POPPED)
#define OFF_IS_CONSERVATIVE		(WATCH_READ|WATCH_WRITE|BARRIER|LOCATION_UNNEEDED|VALUE_UNNEEDED|VALUE_UNKNOWN_TO_CALLEE|VAR_IS_BLOCKNO)
	ushort flags;
	union {
		Elf32_Addr val;
		Elf32_Rel *rel;
	} u;
	ulong alias;
	int prevcopy;
	int nextcopy;
};

void call_gencode(struct instable *dp);
struct instable *subst_dp();
struct instable *subst_xor_mov();
struct instable *subst_mov_xor();
#define STACK_RESET() do {\
		makealias(ESP, NBASE, 0);\
		newvar(NBASE);\
	} while(0)
#define GROWSTACK(SIZE) growstack(SIZE)
#define SHRINKSTACK(SIZE) shrinkstack(SIZE)

struct varinfo *Varinfo;
int Svarinfo;
extern int Silent_mode;

unchar regtab[26][26];

#define IS_PARTIAL(FLAGS) ((FLAGS) & (TWOPOS|FIRST8|SECOND8|SIXTEEN))
#define PARTIAL(FLAGS) ((FLAGS) & (TWOPOS|FIRST8|SECOND8|SIXTEEN))

#define NO_SUCH_REG		(INT_MAX)
#define NO_SUCH_REG2	(INT_MAX-1)
#define DANGEROUS_REF	(INT_MAX-2)
#define READREG(REG) DO_READ(REG, 0)
#define IS_REL_CONSTANT(VAR) (Varinfo[VAR].type == REL_CONSTANT_TYPE)
#define IS_CONSTANT(VAR) (Varinfo[VAR].type == CONSTANT_TYPE)
#define IS_ALIAS(VAR) (Varinfo[VAR].type == ALIAS_TYPE)
#define IS_NOT_STACK(VAR) (Varinfo[VAR].type == NOT_STACK_TYPE)
#define IS_UNKNOWN(VAR) (Varinfo[VAR].type == UNKNOWN_TYPE)
#define IS_INDEFINITE(VAR) ((Varinfo[VAR].type == UNKNOWN_TYPE) || IS_ANDBACK(VAR))
#define TYPE(VAR) (Varinfo[VAR].type)
#define CONSTANT(VAR) ((long) (Varinfo[VAR].u.val))
#define VAR_REL(VAR) (Varinfo[VAR].u.rel)
#define ALIAS(VAR) (Varinfo[VAR].alias)
#define NEXTCOPY(VAR) (Varinfo[VAR].nextcopy)
#define AND_ALIAS(VAR, VAL) ANYWHERE(VAR)
#define OR_CONSTANT(VAR, VAL) (Varinfo[VAR].u.val |= (VAL))
#define AND_CONSTANT(VAR, VAL) (Varinfo[VAR].u.val &= (VAL))
#define SUBTRACT_FROM_ALIAS(VAR, VAL) ((VAR == ESP) ? GROWSTACK((VAL)/sizeof(Elf32_Addr)) : (Varinfo[VAR].alias += ((VAL)/sizeof(Elf32_Addr))))
#define ADD_TO_ALIAS(VAR, VAL) ((VAR == ESP) ? SHRINKSTACK((VAL)/sizeof(Elf32_Addr)) : (Varinfo[VAR].alias -= ((VAL)/sizeof(Elf32_Addr))))
#define ADD_TO_CONSTANT(VAR, VAL) (Varinfo[VAR].u.val += (VAL))
#define SUBTRACT_FROM_CONSTANT(VAR, VAL) (Varinfo[VAR].u.val -= (VAL))
#define ANYWHERE(VAR) anywhere(VAR)
#define NOT_STACK(VAR) not_stack(VAR)

#define OPT_OPS (OPT_PUSH|OPT_POP|OPT_CALL|OPT_RET|OPT_LEA|OPT_MOV|OPT_ADD|OPT_AND|OPT_SUB)
#define MARK_READ(VAR) mark_read(VAR)
#define MARK_WRITE(VAR) (Varinfo[VAR].flags |= WRITE_FLAG)
#define UNMARK_WRITE(VAR) (Varinfo[VAR].flags &= ~WRITE_FLAG)
#define DO_WRITE(VAR, FLAGS) do_write(VAR, FLAGS)
#define DO_READ(VAR, FLAGS) do_read(VAR, FLAGS)
#define FLUSH_OUT() (fputs(obuf, stdout), obuf[0] = '\0', fflush(stdout))
#define GET_ANDBACK(VAR) (Varinfo[VAR].flags & ANDBACK_FLAG)
#define IS_ANDBACK(VAR) (Varinfo[VAR].flags & ANDBACK_FLAG)
#define MAKE_ANDBACK(VAR) (Varinfo[VAR].flags |= ANDBACK_FLAG)
#define MAKE_NOT_ANDBACK(VAR) (Varinfo[VAR].flags &= ~ANDBACK_FLAG)

#define GET_INDEXED(VAR) (Varinfo[VAR].flags & INDEXED_FLAG)
#define IS_INDEXED(VAR) (Varinfo[VAR].flags & INDEXED_FLAG)
#define MAKE_INDEXED(VAR) (Varinfo[VAR].flags |= INDEXED_FLAG)
#define MAKE_NOT_ANDBACK_OR_INDEXED(VAR) (Varinfo[VAR].flags &= ~(ANDBACK_FLAG|INDEXED_FLAG))

/* Substitution types */
#define CONSTSRC	0
#define CONSTDEST	1
#define REGSRC		2
#define REGDEST		3

#define SUBST_CONSTANT	(NO_SUCH_REG)

#define SET_STATEMENT_START(BLOCK, START) (Pstatements[(BLOCK)].statements_start = (START))
#define SET_NSTATEMENTS(BLOCK, N) (Pstatements[(BLOCK)].nstatements = (N))
#define NSTATEMENTS(BLOCK) ((unsigned long) Pstatements[(BLOCK)].nstatements)
#define STATEMENT_SUB_I(BLOCK, I) (Statements[Pstatements[(BLOCK)].statements_start + (I)])
#define DECODED(BLOCK) (Pstatements[(BLOCK)].statements_start)

#define SET_NVISITS(BLOCK, N) (Pstatements[(BLOCK)].nvisits = (N))
#define NVISITS(BLOCK) (Pstatements[(BLOCK)].nvisits)

#define SET_NENTRYVARS(BLOCK, N) ((Pstatements[(BLOCK)].entry_vars < Nholdvars) ? (Holdvars[Pstatements[(BLOCK)].entry_vars].nholdvars = (N)) : 0)
#define NENTRYVARS(BLOCK) ((Pstatements[(BLOCK)].entry_vars < Nholdvars) ? Holdvars[Pstatements[(BLOCK)].entry_vars].nholdvars : 0)
#define SET_SENTRYVARS(BLOCK, S) (Holdvars[Pstatements[(BLOCK)].entry_vars].sholdvars = (S))
#define SENTRYVARS(BLOCK) (Holdvars[Pstatements[(BLOCK)].entry_vars].sholdvars)
#define SET_ENTRYVARS(BLOCK, ENTRYVARS) (Holdvars[Pstatements[(BLOCK)].entry_vars].holdvars = (ENTRYVARS))
#define ENTRYVARS(BLOCK) (Holdvars[Pstatements[(BLOCK)].entry_vars].holdvars)
#define IS_INIT_ENTRYVARS(BLOCK) (Pstatements[(BLOCK)].entry_vars > 0)


#define SET_STATEMENT_SUB_I(BLOCK, DEST, STATEMENT) (Statements[Pstatements[(BLOCK)].statements_start + (DEST)] = (STATEMENT))
#define COPY_SUB_I(BLOCK, DEST, SRC) SET_STATEMENT_SUB_I(BLOCK, DEST, STATEMENT_SUB_I(BLOCK, SRC))
#define PREFIX_SUB_I(BLOCK, I) (Statements[Pstatements[(BLOCK)].statements_start + (I)].prefixes)
#define OPSINFO_SUB_I(BLOCK, I) (Statements[Pstatements[(BLOCK)].statements_start + (I)].opsinfo)
#define SET_DP_SUB_I(BLOCK, I, DP) (DP_SUB_I(BLOCK, I) = (DP))
#define DP_SUB_I(BLOCK, I) (Statements[Pstatements[(BLOCK)].statements_start + (I)].dp)
#define SET_ADDR_SUB_I(BLOCK, I, ADDR) (ADDR_SUB_I(BLOCK, I) = (ADDR))
#define ADDR_SUB_I(BLOCK, I) (Statements[Pstatements[(BLOCK)].statements_start + (I)].addr)
#define SET_FLAGS_SUB_I(BLOCK, I, FLAGS) (FLAGS_SUB_I(BLOCK, I) = (FLAGS))
#define ADD_FLAGS_SUB_I(BLOCK, I, FLAGS) (FLAGS_SUB_I(BLOCK, I) |= (FLAGS))
#define FLAGS_SUB_I(BLOCK, I) (Statements[Pstatements[(BLOCK)].statements_start + (I)].flags)

struct statement {
#define STATEMENT_SUNK 1
#define STATEMENT_FLOATED 2
	int flags;
	struct opsinfo opsinfo[NUMOPS];
	struct instable *dp;
	struct instable *prefixes[3];
	Elf32_Addr addr;
};

struct holdvars {
	ushort nholdvars;
	ushort sholdvars;
	struct varinfo *holdvars;
};
struct pstatement {
	ushort nvisits;
	ushort entry_vars;
	ushort statements_start;
	ushort nstatements;
};
EXTERN struct pstatement		*Pstatements;
EXTERN int						Spstatements;
EXTERN struct statement			*Statements;
EXTERN int						Nstatements;
EXTERN int						Sstatements;
EXTERN struct holdvars			*Holdvars;
EXTERN int						Nholdvars;
EXTERN int						Sholdvars;

#define IS_DEST(OPNO) (opsinfo[OPNO].flags & OPTINFO_DEST)
#define IS_SRC(OPNO) (opsinfo[OPNO].flags & OPTINFO_SRC)
#define SET_REFVAR(OPNO, VAR) (opsinfo[OPNO].refto = (VAR))
#define REFVAR(OPNO) ((OPNO == NO_SUCH_REG) ? 0 : opsinfo[OPNO].refto)
#define SET_REFFLAGS(OPNO, VAR) (opsinfo[OPNO].refflags = (VAR))
#define REFFLAGS(OPNO) ((OPNO == NO_SUCH_REG) ? 0 : opsinfo[OPNO].refflags)

/*
** These are the flags associated with references
*/
#define ANDBACK				(1<<0)
#define INDEXED				(1<<1)
#define TWOPOS				(1<<2)
#define FIRST8				(1<<3)
#define SECOND8				(1<<4)
#define SIXTEEN				(1<<5)
#define REL_CONSTANT		(1<<6)
#define EARLY_STACK			(1<<7)
#define POSSIBLY_MEMORY		(1<<8)
#define CANT_SUBST			(1<<9)

void makealias(int to, int from, int flags);
void anywhere(int index);
void clear_eax_ecx_edx();
void clearvar(int i);
void clearvar_unset(int i);

EXTERN struct varinfo *Watchinfo;

EXTERN int Nwatchinfo;
EXTERN int Swatchinfo;

#define VERBOSE1 (Verbose_mode)
#define VERBOSE2 (Verbose_mode > 1)
#define VERBOSE3 (Verbose_mode > 2)
#define VERBOSE4 (Verbose_mode > 3)
EXTERN int Verbose_mode;

EXTERN int *Funcs_beenhere;
EXTERN int Nfuncs_beenhere;
EXTERN int Sfuncs_beenhere;

EXTERN int *Changed_blocks;
EXTERN int Nchanged_blocks;
EXTERN int Schanged_blocks;

EXTERN int Before_retblock_stack;
EXTERN int Fixed_retblock;
EXTERN int Ndelete;
EXTERN int Enclosing_func;
EXTERN jmp_buf Giveup;

#define IS_ELIM(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags & ELIM_ME))
#define MARK_ELIM(VAR) (((VAR >= Nwatchinfo) ? grow_watch(VAR) : 0), (Watchinfo[(VAR)].flags |= ELIM_ME))
#define UNMARK_ELIM(VAR) (((VAR >= Nwatchinfo) ? grow_watch(VAR) : 0), (Watchinfo[(VAR)].flags &= ~ELIM_ME))

#define IS_VALUE_UNNEEDED(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags & VALUE_UNNEEDED))
#define MARK_VALUE_UNNEEDED(VAR) (((VAR >= Nwatchinfo) ? grow_watch(VAR) : 0), (Watchinfo[(VAR)].flags |= VALUE_UNNEEDED))
#define UNMARK_VALUE_UNNEEDED(VAR) (((VAR >= Nwatchinfo) ? grow_watch(VAR) : 0), (Watchinfo[(VAR)].flags &= ~VALUE_UNNEEDED))

#define IS_LOCATION_UNNEEDED(VAR) (IS_LOCATION_UNNEEDED_GLOBAL(VAR) || IS_LOCATION_UNNEEDED_ON_PATH(VAR))

#define IS_LOCATION_UNNEEDED_GLOBAL(VAR) (((VAR) >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags & LOCATION_UNNEEDED))
#define MARK_LOCATION_UNNEEDED(VAR) ((((VAR) >= Nwatchinfo) ? grow_watch(VAR) : 0), (Watchinfo[(VAR)].flags |= LOCATION_UNNEEDED))
#define UNMARK_LOCATION_UNNEEDED(VAR) ((((VAR) >= Nwatchinfo) ? grow_watch(VAR) : 0), (Watchinfo[(VAR)].flags &= ~LOCATION_UNNEEDED))

#define IS_LOCATION_UNNEEDED_ON_PATH(VAR) (((VAR) > Varinfo[ESP].alias) ? 0 : (Varinfo[(VAR)].flags & LOCATION_UNNEEDED))
#define MARK_LOCATION_UNNEEDED_ON_PATH(VAR) (Varinfo[(VAR)].flags |= LOCATION_UNNEEDED)
#define UNMARK_LOCATION_UNNEEDED_ON_PATH(VAR) (Varinfo[(VAR)].flags &= ~LOCATION_UNNEEDED)

#define IS_UNKNOWN_TO_CALLEE(VAR) (Varinfo[VAR].flags | VALUE_UNKNOWN_TO_CALLEE)
#define MARK_UNKNOWN_TO_CALLEE(VAR) (Varinfo[VAR].flags |= VALUE_UNKNOWN_TO_CALLEE)
#define UNMARK_UNKNOWN_TO_CALLEE(VAR) (Varinfo[VAR].flags &= ~VALUE_UNKNOWN_TO_CALLEE)
#define MAKE_WATCHED_FOR_READ_ON_PATH(VAR) (Varinfo[(VAR)].flags |= WATCH_READ)
#define MAKE_WATCHED_FOR_WRITE_ON_PATH(VAR) (Varinfo[(VAR)].flags |= WATCH_WRITE)
#define MAKE_NOT_WATCHED_ON_PATH(VAR) (Varinfo[(VAR)].flags &= ~(WATCH_WRITE|WATCH_READ))
#define IS_WATCHED_FOR_READ_ON_PATH(VAR) (Varinfo[(VAR)].flags & WATCH_READ)
#define IS_WATCHED_FOR_WRITE_ON_PATH(VAR) (Varinfo[(VAR)].flags & WATCH_WRITE)
#define MARK_POPPED(VAR) (Varinfo[VAR].flags |= POPPED)

#define MARK_AS_BLOCKNO(VAR) (Varinfo[VAR].flags |= VAR_IS_BLOCKNO)
#define IS_VAR_BLOCKNO(VAR) (Varinfo[VAR].flags & VAR_IS_BLOCKNO)

#define MAKE_WATCHED_FOR_READ(VAR) (((VAR >= Nwatchinfo) ? grow_watch(VAR) : 0), (Watchinfo[(VAR)].flags |= WATCH_READ))
#define MAKE_WATCHED_FOR_WRITE(VAR) (((VAR >= Nwatchinfo) ? grow_watch(VAR) : 0), (Watchinfo[(VAR)].flags |= WATCH_WRITE))

#define MAKE_NOT_WATCHED_FOR_READ(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags &= ~WATCH_READ))
#define MAKE_NOT_WATCHED_FOR_WRITE(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags &= ~WATCH_WRITE))
#define MAKE_NOT_WATCHED(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags &= ~(WATCH_WRITE|WATCH_READ)))

#define IS_WATCHED_FOR_READ(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags & WATCH_READ))
#define IS_WATCHED_FOR_WRITE(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags & WATCH_WRITE))
#define IS_WATCHED(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags & (WATCH_WRITE|WATCH_READ)))

#define READ_VAR(VAR) (Varinfo[(VAR)].flags & READ_FLAG)
#define WROTE_VAR(VAR) (Varinfo[(VAR)].flags & WRITE_FLAG)
#define READ_OR_WROTE_VAR(VAR) (Varinfo[(VAR)].flags & (READ_FLAG|WRITE_FLAG))
#define UNMARK_READ(VAR) (Varinfo[(VAR)].flags &= ~READ_FLAG)
#define UNMARK_READ_WROTE(VAR) (Varinfo[(VAR)].flags &= ~(WRITE_FLAG|READ_FLAG))

#define MARK_WATCHED_READ(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags |= SAW_READ))
#define MARK_WATCHED_WRITTEN(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags |= SAW_WRITE))

#define WAS_WATCHED_READ(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags & SAW_READ))
#define WAS_WATCHED_WRITTEN(VAR) ((VAR >= Nwatchinfo) ? 0 : (Watchinfo[(VAR)].flags & SAW_WRITE))

Elf32_Rel *checkrel(Elf32_Addr start, Elf32_Addr end);
int proc_block_analyze_ea(int block);
int proc_block_analyze_esr(int block);
int proc_block_substitution(int block);
void proc_block_gentext(int block);
char *pr(int var);
int delete_or_modify_instruction(struct instable *dp, int prevstack);
int arg_delete_or_modify_instruction(struct instable *dp, int prevstack);
int esr_delete_instruction(struct instable *dp, int prevstack);

void set_traversal_params(int flags, int (*proc_block)(int block), int (*each_statement)(struct instable *dp, int arg), int (*each_block)(), int (*call_at_end)());
#define FOLLOW_CALLS					(1<<0)
#define SHOW_SUBST						(1<<1)
#define VISIT_UNTIL_DONE				(1<<2)
#define INTERPROCEDURAL_JUMPS			(1<<3)
#define FOLLOW_RET						(1<<4)
#define VISIT_ON_PATHS					(1<<5)
#define DO_SUBST						(1<<6)
#define DO_NOT_GIVE_UP_JUST_END			(1<<7)
#define JUST_WATCHING_REGS				(1<<8)
#define MARK_READ_ONLY_AFTER_WRITTEN	(1<<9)
#define SEE_ARGS						(1<<10)
#define NEVER_FOLLOW_RET				(1<<11)

EXTERN int Traversal_ctl;

struct esr_watch {
#define ESR_ELIM	1
#define ESR_POPPED	2
	int flags;
	int pushvar;
	int nelim;
	Elf32_Addr elim[10];
};

EXTERN struct esr_watch Esr_watch[NREGS];
EXTERN int Freeregs[NREGS];
EXTERN int Nfreeregs;
