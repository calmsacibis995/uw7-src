#ident	"@(#)nas:i386/stmt386.h	1.9"
/*
* i386/stmt386.h - i386 assembler statement/instruction header
*
*/

#include "chkgen.h"

enum	/* the registers */
{
	Reg_eax,	Reg_ecx,	Reg_edx,	Reg_ebx,
	Reg_esp,	Reg_ebp,	Reg_esi,	Reg_edi,

	Reg_ax,		Reg_cx,		Reg_dx,		Reg_bx,
	Reg_sp,		Reg_bp,		Reg_si,		Reg_di,

	Reg_al,		Reg_cl,		Reg_dl,		Reg_bl,
	Reg_ah,		Reg_ch,		Reg_dh,		Reg_bh,

	Reg_es,		Reg_cs,		Reg_ss,		Reg_ds,
	Reg_fs,		Reg_gs,

	Reg_cr0,	Reg_cr2,	Reg_cr3,	Reg_cr4,

	Reg_tr3,	Reg_tr4,	Reg_tr5,
	Reg_tr6,	Reg_tr7,

	Reg_dr0,	Reg_dr1,	Reg_dr2,	Reg_dr3,
	Reg_dr6,	Reg_dr7,

	Reg_st,				/* with no ()'s */
	Reg_st0,	Reg_st1,	Reg_st2,	Reg_st3,
	Reg_st4,	Reg_st5,	Reg_st6,	Reg_st7,

	Reg_TOTAL	/* not a register */
};

#ifdef __STDC__
void	initinst(void);				/* set up instrs */
void	stmt(const Uchar *, size_t, Oplist *);	/* arbitrary stmt */
void	nopsets(const Uchar *, size_t);		/* set nop-fill attributes */
void	operinst(const Inst *, Operand *);	/* check instr operand */
const Inst *findinst(const Uchar *, size_t);	/* look up instruction */
#else
void	initinst(), stmt(), nopsets(), operinst();
Inst * findinst();
#endif

enum	/* the known target processors */
{
	ProcType_386,
	ProcType_486,	/* default */
	ProcType_586
};

extern int	proc_type;	/* ProcType_* value (-t option) */

#ifdef __STDC__
typedef void Operchk(Code *);
#else
typedef void Operchk();
#endif

/* i386 instruction information */
typedef struct {
    Inst inst;			/* generic instruction:  must be first */
    Uchar minops;		/* minimum # of operands */
    Uchar maxops;		/* maximum # of operands */
    Uchar code[3];		/* byte-by-byte encoding */
    Uchar opersize;		/* size of operand (in bytes) */
    Operchk *operchk;		/* function to check operands */
    const chklist_t *chklist;	/* operand check list */
    const genlist_t *genlist;	/* code generation list */
} Inst386;

#define chkflags inst_impdep	/* use implementation defined field */
/* Flags for instruction checking: */
#define IF_STAR		0x1	/* allow '*' for indirection */
#define	IF_VARSIZE	0x2	/* instruction is variable-size */
#define	IF_RSIZE	0x4	/* check that register size matches operand size */
#define	IF_NOSEG	0x8	/* segment register not allowed (string inst.) */
#define	IF_BASE_DX	0x10	/* %dx can be base register (in/out inst.) */
#define IF_KRELOC	0x20	/* keep inst (call) relocatable */

#define IF_P5_0F_PREFIX	0x40	/* (fixed_size) instruction always has 0Fh
				   prefix - BUT NOT jumps */
#define IF_CALL		0x80	/* call instruction; next instruction is an
				   implied jump(ret) target */
#define IF_PREFIX_PSEUDO_OP	0x100	/* a prefix pseudo op such as addr16,
					   data16, lock, etc. */

#ifdef __STDC__
extern void gen_value(Eval *, int, Uchar *);
#else
extern void gen_value();
#endif

/* Operand checking routines */
Operchk chk_ar2;	/* check arithmetic/logical */
Operchk	chk_fxch;	/* check fxch */
Operchk chk_imul;	/* forms of imul */
Operchk	chk_inout;	/* check in/out */
Operchk chk_int;	/* check int */
Operchk chk_jmp;	/* plain jmp/call */
Operchk chk_ljmp;	/* check ljmp/lcall */
Operchk chk_mov;	/* check movl/movw */
Operchk chk_pcr;	/* check most PC-relative branches */
Operchk chk_pop;	/* check pop */
Operchk chk_push;	/* check push */
Operchk chk_ret;	/* check ret */
Operchk chk_shift;	/* check sal/sar/shl/shr/rcl/rcr/rol/ror */
Operchk	chk_shxd;	/* check double-size shifts */
Operchk chk_str;	/* check string instructions:  movs/smov */


/* Instruction generation routines */
InstGen gen_clr;	/* clr variants */
InstGen	gen_ent;	/* enter instruction */
InstGen	gen_fxch;	/* fxch */
InstGen gen_imul;	/* forms of imul */
InstGen gen_jmp;	/* plain jmp/call */
InstGen gen_ljmp;	/* ljmp/lcall */
InstGen gen_nop;	/* nullary instructions */
InstGen gen_optnop;	/* instructions w/ optional operands*/
InstGen gen_pcr;	/* most PC-relative branches */
InstGen gen_pc8;	/* loop PC-relative with 1-byte displacement */
InstGen gen_ret;	/* ret */
InstGen	gen_shxd;	/* generate double-size shifts */
InstGen gen_str;	/* generate string instructions:  movs/smov */
InstGen gen_cmov;	/* generate cmov instructions */

InstGen	gen_list;	/* generic, for anything with a genlist_t only */

#define olst_combo olst_impdep		/* remember operand combination */

#define ASFLAG_KRELOC	0x80000000	/* flag to keep calls relocatable */
