#ident	"@(#)cg:common/mfile2.h	1.42"
 
#ifndef	MACDEFS_H			/* guard against double include */
# include "macdefs.h"
#define	MACDEFS_H
#endif
# include "manifest.h"

/*  code generation goals, used in table entries */

# define FOREFF 01 /* compute for effects only */
# define INREG 02 /* compute into a register */
# define FORCC 04 /* compute for condition codes only */

 /* types of operators and shapes; optab.tyop, optab.ltype optab.rtype */
 /* No longer can these types squeeze into 16 bits.
 ** These types must be distinct from SPTYPE (0x40000).
 */
#define TCHAR		0x00001
#define TUCHAR		0x00002
#define TSHORT		0x00004
#define TUSHORT		0x00008
#define urTINT		0x00010
#define urTUINT		0x00020
#define TLONG		0x00040
#define TULONG		0x00080
#define TLLONG		0x00100
#define TULLONG		0x00200
#define TFLOAT		0x00400
#define urTDOUBLE	0x00800
#define TLDOUBLE	0x01000
#define TVOID		0x02000
#define TSTRUCT		0x04000
#define TPOINT		0x08000
#define TPOINT2		0x10000
#define TFPTR		0x20000

#define TANY		0x3ffff	/* matches any of the above */

 /* Define TINT and TUNSIGNED given NOSHORT/NOLONG. */
#ifndef NOSHORT
#   ifndef NOLONG
#	define TINT		urTINT
#	define TUNSIGNED	urTUINT
#   else
#	define TINT		(urTINT|TLONG)
#	define TUNSIGNED	(urTUINT|TULONG)
#   endif
#else
#   ifndef NOLONG
#	define TINT		(urTINT|TSHORT)
#	define TUNSIGNED	(urTUINT|TUSHORT)
#   else
#	define TINT		(urTINT|TSHORT|TLONG)
#	define TUNSIGNED	(urTUINT|TUSHORT|TULONG)
#   endif
#endif

 /* Define TDOUBLE given ONEFLOAT and NOLDOUBLE. */
#ifndef ONEFLOAT
#   ifndef NOLDOUBLE
#	define TDOUBLE	urTDOUBLE
#   else
#	define TDOUBLE	(urTDOUBLE|TLDOUBLE)
#   endif
#else
#   ifndef NOLDOUBLE
#	define TDOUBLE	(urTDOUBLE|TFLOAT)
#   else
#	define TDOUBLE	(urTDOUBLE|TFLOAT|TLDOUBLE)
#   endif
#endif

	/* reclamation cookies */

# define RLEFT 01
# define RRIGHT 02
# define RESC1 04
# define RESC2 010
# define RESC3 020
# define RESCC 040
# define RNOP 0100   /* DANGER: can cause loops.. */
# define RNULL 0200    /* clobber result */
# define REITHER 0400	/* take result from L or R, whichever is "better" */

	/* needs */

# define NREG 01
# define NCOUNT 017
# define NMASK 0777
# define LSHARE 020 /* share left register */
# define RSHARE 040 /* share right register */
# define NPAIR 0100 /* allocate registers in pairs */
# define LPREF 0200 /* prefer left register, if any */
# define RPREF 0400 /* prefer right register, if any */

/* These bits are set by sty to force exact match of sub-trees.
** Note that they are not included under NMASK
*/

# define LMATCH 010000        /* left sub-tree must match exactly */
# define RMATCH 020000        /* right sub-tree must match exactly */
# define NO_IGNORE 01000	/*This template cannot ignore exceptions*/
# define NO_HONOR 02000		/*This template cannot honor exceptions*/
 
	/* register allocation */

extern int busy[];

# define INFINITY 10000

typedef struct shape SHAPE;

	/* special shapes (enforced by making costs high) */
#define SPTYPE	0x40000
#define SVMASK	0x00fff	/* value part of special shape */
#define	STMASK	0x07000	/* special shape kind: */
#define SVAL	0x01000		/* positive constant value */
#define SNVAL	0x02000		/* negative constant value */
#define SRANGE0	0x03000		/* positive range [0, (2**m)-1] */
#define SSRANGE	0x04000		/* signed range [-(2**m), (2**m)-1] */
#define SNRANGE	0x05000		/* negative range [-(2**m), -1] */
#define NACON	0x06000		/* nameless contant */
#define SUSER	0x07000		/* user's cost function */

#define SPECIAL	(SPTYPE|STMASK) /* something from the above */

struct shape {
	int	op;	/* operator */
	SHAPE *sl;	/* pointers to left- and right-hand shape */
	SHAPE *sr;
	long sh;	/* flags for special shape and type matches */
	RST sregset;	/* register set for this shape (REG shape only) */
};

typedef struct optab OPTAB;

typedef SHAPE *SH_PTR;
typedef SH_PTR *SH_PTR_PTR;

struct optab {
	int	op;	/* operator */
	long	tyop;	/* type of the operator node itself */
	OPTAB	*nextop;
	SH_PTR_PTR lshape;	/* pointer to pointer to left shape */
	long	ltype;		/* type of left shape */
	SH_PTR_PTR rshape;	/* likewise for the right shape */
	long	rtype;
	int	needs;
	short	rneeds;		/* offset into rsbits[] */
	int	rewrite;
	char	*cstring;
	int	stinline;	/* line number in stin file */
};

extern OPTAB
	*match(),
	* const ophead[];

extern NODE resc[];

extern int tmpoff;
extern int maxboff;
extern int ftnno;
extern int sideff;

extern NODE
	*talloc(),
/* special hack for Nifty/C++, which uses tcopy():  check arg type */
#ifndef	NODEPTR
#define	NODEPTR
#endif
	*tcopy(NODEPTR),
	*getadr(),
	*getlr();

extern int
	argsize(),
	freetemp(),
	iseff(),
	lhsok(),
	rewass(),
	rewsto(),
	rs_rnum(),
	tnumbers(),
	special(),
	pre_ex(),
	p2nail(),
	semilp(),
	rewsemi(),
	ushare();

extern RST rs_reclaim();

extern void
	allo0(),
	allo(),
	allchk(),
	codgen(),
	expand(),
	fcons(),
	insprt(),
	mkdope(),
	qsort(),
	reallo(),
	reclaim(),
	rewcom(),
	reweop(),
	regrcl(),
	set_alt_entry(),
	tinit(),
	unorder(),
	commute(),
	typecheck(),
	uncomma(),
#ifdef  MYRET_TYPE
	myret_type(),
#endif
	exit()
#ifndef NODBG
	,e2print(),
	e22print(),
	e222print()
#endif
;

# define getlt(p,t) ((t)==LTYPE?p:p->in.left)
# define getlo(p,o) (getlt(p,optype(o)))
# define getl(p) (getlo(p,p->tn.op))
# define getrt(p,t) ((t)==BITYPE?p->in.right:p)
# define getro(p,o) (getrt(p,optype(o)))
# define getr(p) (getro(p,p->tn.op))

extern const char *const rnames[];

extern int lineno;
extern char ftitle[];
extern int fldshf, fldsz;
extern int fast;  /* try to make the compiler run faster */
extern int lflag, udebug, e2debug, odebug, rdebug, /*radebug,*/ sdebug;

#ifndef callchk
#define callchk(x) allchk()
#endif
#ifndef callreg
	/* try to number so results returned in 0 */
#define callreg(x) 0
#endif
#ifndef szty
	/* it would be nice if number of registers to hold type t were 1 */
	/* on most machines, this will be overridden in macdefs.h */
# define szty(t) 1
#endif


#ifndef PUTCHAR
# define PUTCHAR(x) putchar(x)
#endif

# define CEFF (NRGS+1)
# define CTEMP (NRGS+2)
# define CCC (NRGS+3)
	/* the assumption is that registers 0 through NRGS-1 are scratch */
	/* the others are real */
	/* some are used for args and temps, etc... */

# define istreg(r) ((r)<NRGS)

typedef struct st_inst INST;

struct st_inst {
	NODE	*p;
	OPTAB	*q;
	int	goal;
	/* set of registers where result is desired to be */
	RST	rs_want;
	/* Set of available registers:  cfix() may need this to
	** choose scratch registers.
	*/
	RST	rs_avail;
};

#undef NINS
extern struct td td_inst;
#define inst ((INST *)(td_inst.td_start))
#define nins (td_inst.td_used)
#define NINS (td_inst.td_allo)


	/* definitions of strategies legal for ops */

# define STORE 01
# define LTOR 02
# define RTOL 04
# define EITHER (LTOR|RTOL)

#define CONTAINS_CALL 01/* function contains call */
# define PAREN 010	/* do entire op at once*/
# define COPYOK 040	/* on LET node: OK to copy CSE's */
# define EXHONOR 0100	/* Must honor numeric exceptions*/
# define EXIGNORE 0200	/* Must ignore numeric exceptions*/
# define DOEXACT 0400	/* Must honor exact semantics (e.g. volatile)*/
# define WASCSE 01000	/* On TEMPS -- this node used to be a CSE */
# define OCOPY 02000	/* ordered copy: 1 = must make a copy of this value*/
# define VOLATILE 04000	/* treat node according to volatile conditions */
# define WAS_FCON 010000 /* This node was an FCON.  xval is still correct */

# define PIC_GOT 010000 /* for PIC "global offset table" flag */
# define PIC_PLT 020000 /* for PIC "procedure linkage table" flag */
# define PIC_PC  040000 /* for PIC "program counter" flag */
# define FULL_OPT 0100000 /* fully optimizable asm */
# define PART_OPT 0200000 /* partially optimizable asm */
# define RIGHT_QNODE 0400000 /* Right Qnode */
# define MOVEARGS 01000000 /* MOVE arg, not PUSH */

#define VOL_OPND1 01	/* operands which are volatile objects */
#define VOL_OPND2 02
#define VOL_OPND3 04
#define VOL_OPND4 010

#define RS_FAIL	(RS_BIT(TOTREGS)) /* special flag for match fail */
#ifndef RS_FORTYPE
#define RS_FORTYPE(type) (RS_NRGS) /* any scratch reg for any type */
#endif
/* flags for insout/bprt */
#define REWROTE (RS_FAIL + 1)	/* tree rewritten */
#define OUTOFREG (RS_FAIL + 2)	/* ran out of regs, couldn't gen. code */
#define REWRITE_THRESH (1<<10)	/* Maximum number of strategy shifts
				** before we give up.  This prevents
				** expontential compile time.
				*/
extern RST insout();
#define INSOUT(p,g) insout(p,g,NRGS,RS_NRGS,g==NRGS?RS_NRGS:RS_NONE)
					/* add reg.set. as avail. */
/* definitions for templates */
extern RST rsbits[];		/* array of register set bits */
#define RSB(n)	(&rsbits[(n)])	/* address of bit vector n */
#ifndef RS_MOVE			/* default is cerror() */
#define RS_MOVE(p,q) cerror("RS_MOVE called!!")
#endif

#ifndef	NODBG
extern OPTAB const table[];
extern SH_PTR const pshape[];
extern SHAPE const shapes[];
#endif

/* definitions of INSOUT() for use at "top-level", when all registers
** are available.
*/

        /*flags for CG DEFNAM node (in "sid")*/
# define EXTNAM 01
# define FUNCT  02
# define COMMON 04
# define DEFINE 010
# define DEFNOI 020
# define ICOMMON 040
# define LCOMMON 0100


/*Structure for common subexpressions*/
#define MAXCSE 100	/*max # of cse's*/
struct cse {
	int id;	/*number for this CSE*/
	int reg;/*reg that holds the cse*/
};

extern struct cse cse_list[MAXCSE];
extern struct cse *cse_ptr;
struct cse *getcse();

#ifdef	IN_LINE
/* Declaration for expanding asm's */
extern void as_gencode();
#endif

/* Types - T2_types make available to the front
 * end for using either RCC or PCC back ends.
*/
#define T2_CHAR		TCHAR
#define T2_SHORT	TSHORT
#define T2_INT		TINT
#define T2_LONG		TLONG
#define T2_LLONG	TLLONG
#define T2_FLOAT	TFLOAT
#define T2_DOUBLE	TDOUBLE
#define T2_LDOUBLE	TLDOUBLE
#define T2_UCHAR	TUCHAR
#define T2_USHORT	TUSHORT
#define T2_UNSIGNED	TUNSIGNED
#define T2_ULONG	TULONG
#define T2_ULLONG	TULLONG
#define T2_STRUCT	TSTRUCT
#define T2_VOID		TVOID

#define T2_ADDPTR(t)	TPOINT
#define T2_ADDFTN(t)	(t)
#define T2_ADDARY(t)	(t)

/* parameter flags */
#define REGPARAM	01
#define VARPARAM	02
#define VOLPARAM	04

/* name node information */
#define NI_FUNCT	01
#define NI_OBJCT	02
#define NI_GLOBAL	04
#define NI_FLSTAT	010
#define NI_BKSTAT	020

#ifndef FORCE_LC
#define	FORCE_LC(lc)	(lc)
#endif


/* debugging remarks for branch heuristics*/
/* make them bits (and not enum) to enable represenatation of a number of
** them simultaneously
*/

#define BR_UNK			0x0
#define ABOVE			0x80000000 /*above a logical connector*/
#define BELOW			0x00000001 /*below, lowermost*/
#define CMP_DEREF               0x00000002
#define GUARD_CALL              0x00000004
#define GUARD_LOOP              0x00000008
#define GUARD_SHORT_BLOCK       0x00000010
#define CMP_GLOB_CONST          0x00000020
#define CMP_PARAM_CONST         0x00000040
#define CMP_PTR_ERR             0x00000080
#define CMP_GLOB_PARAM          0x00000100
#define CMP_2_AUTOS             0x00000200
#define	CMP_LOGICAL		0x00000400
#define	CMP_USE_SHIFT		0x00000800
#define SHORT_IF_THEN_ELSE	0x00001000
#define CMP_FUNC_CONST		0x00002000
#define GUARD_RET               0x00004000
#define COMPOUND		0x20000000
#define LOOP			0x40000000
#define PREDICTABLE_BRANCH (GUARD_RET|GUARD_CALL|GUARD_LOOP|CMP_PTR_ERR \
							|CMP_FUNC_CONST)
