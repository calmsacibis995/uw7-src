#ident	"@(#)nas:i386/chkgen.0.h	1.4"
/* chkgen.0.h:  template file for chkgen.h */

/* Definitions for chk_list/gen_list routines. */

#define GL_OVERRIDE	0x1	/* generate operand size override */
#define GL_PREFIX	0x2	/* generate 0x0F prefix byte */
#define GL_PLUS_R	0x4	/* generate +r form */
#define	GL_SLASH_R	0x8	/* generate /r form */
#define	GL_SLASH_N	0x10	/* generate /n form */
#define GL_IMM		0x20	/* generate inst.-sized literal */
#define GL_IMM8		0x40	/* generate 8-bit literal (only) */
#define	GL_MEMRIGHT	0x80	/* right operand is memory (left is deflt.) */
#define	GL_MEMLEFT	0x00	/* nop (left assumed) */
#define	GL_MOFFSET	0x100	/* memory offset form (for mov's) */
#define	GL_PCREL	0x200	/* PC-relative (size in gl_slashn) */
#define	GL_FWAIT	0x400	/* emit "fwait" prefix */

typedef struct {
    Ushort gl_flags;	/* combinations of the above */
    Uchar gl_opcode;	/* opcode for this combination */
    Uchar gl_slashn;	/* /n number to use, if any, or opcode2, or PC-relative
			** size
			*/
} genlist_t;

/* macro to initialize a genlist_t */
#define GL(code, flags, sln) { flags, code, sln }

typedef struct {
    Uchar cl_combo;	/* CASE1/CASE2 operand combination */
    /* auxiliary numbers are register number + 1 or operand size */
    Uchar cl_l_auxno;
    Uchar cl_r_auxno;
} chklist_t;

/* macro to initialize a chklist_t */
#define CLEND	{ 0, 0, 0 }		/* logical end of main list */

/* Operand class values:  used to distinguish different kinds
** of operands.  After an operand is checked, one of these
** codes is put in op->oper_info.
*/
enum {
    OC_error,	/* invalid operand */
    OC_R8,	/* 8-bit register */
    OC_R16,	/* 16-bit register */
    OC_R32,	/* 32-bit register */
    OC_ST,	/* %st */
    OC_STn,	/* %st(n) */
    OC_SEG,	/* segment register */
    OC_SPEC,	/* special registers */
    OC_MEM,	/* memory operand */
    OC_LIT,	/* $literal */
    OC_last	/* gives number */
};

/* For switches on groups of operands: */
#define CASEVAL(l,r) (r*OC_last+l)
