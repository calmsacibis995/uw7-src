#ident	"@(#)cg:common/manifest.h	1.34.6.28"

/*To allow multiple includes:*/
#ifndef MANIFEST_H
#define	MANIFEST_H

#ifndef EOF
# include <stdio.h>
#endif

#ifndef PUTCHAR
#define PUTCHAR(c) ((void)(putc((c),outfile)))
#endif
#ifndef	PUTS
#define	PUTS(s) ((void) fputs((s), outfile))
#endif

	/* macro definitions for common cases of type collapsing */
# ifdef NOSHORT
# define SZSHORT SZINT
# define ALSHORT ALINT
# endif

# ifdef NOLONG
# define SZLONG SZINT
# define ALLONG ALINT
# endif

# ifdef NOFLOAT
# define SZFLOAT SZLONG
# define SZDOUBLE SZLONG
# define ALFLOAT ALLONG
# define ALDOUBLE ALLONG
# endif

# ifdef ONEFLOAT
# define SZFLOAT SZDOUBLE
# define ALFLOAT ALDOUBLE
# endif

#ifdef NOLDOUBLE
#define SZLDOUBLE SZDOUBLE
#define ALLDOUBLE ALDOUBLE
#endif
/* define default assembly language comment starter */

# ifndef COMMENTSTR
# define COMMENTSTR	"#"
#endif

/*	manifest constant file for the lex/yacc interface */

# define ERROR 1
# define NAME 2
# define STRING 3
# define ICON 4
# define FCON 5
# define PLUS 6
	/* ASG PLUS 7 */
# define MINUS 8
	/* ASG MINUS 9 */
	/* UNARY MINUS 10 */
# define MUL 11
	/* ASG MUL 12 */
# define STAR (UNARY MUL)
# define AND 14
	/* ASG AND 15 */
	/* UNARY AND 16 */
# define OR 17
	/* ASG OR 18 */
# define ER 19
	/* ASG ER 20 */
# define QUEST 21
# define COLON 22
# define ANDAND 23
# define OROR 24

/*	special interfaces for yacc alone */
/*	These serve as abbreviations of 2 or more ops:
	ASOP	=, = ops
	RELOP	LE,LT,GE,GT (NLE,NLT,NGE,NGT,LG,NLG,LGE,NLGE)
	EQUOP	EQ,NE
	DIVOP	DIV,MOD
	SHIFTOP	LS,RS
	ICOP	INCR,DECR
	UNOP	NOT,COMPL
	STROP	DOT,STREF

	*/
# define ASOP 25
# define RELOP 26
# define EQUOP 27
# define DIVOP 28
# define SHIFTOP 29
# define INCOP 30
# define UNOP 31
# define STROP 32

/*	reserved words, etc */
# define TYPE 33
# define CLASS 34
# define STRUCT 35
# define RETURN 36
# define GOTO 37
# define IF 38
# define ELSE 39
# define SWITCH 40
# define BREAK 41
# define CONTINUE 42
# define WHILE 43
# define DO 44
# define FOR 45
# define DEFAULT 46
# define CASE 47
# define SIZEOF 48
# define ENUM 49

/*	little symbols, etc. */
/*	namely,

	LP	(
	RP	)

	LC	{
	RC	}

	LB	[
	RB	]

	CM	,
	SM	;

	*/

# define LP 50
# define RP 51
# define LC 52
# define RC 53
# define LB 54
# define RB 55
# define CM 56
# define SM 57
# define ASSIGN 58
	/* ASM returned only by yylex, and totally eaten by yyparse */
# define ASM 59

/*	END OF YACC */

/*	left over tree building operators */
# define COMOP 59
# define DIV 60
	/* ASG DIV 61 */
# define MOD 62
	/* ASG MOD 63 */
# define LS 64
	/* ASG LS 65 */
# define RS 66
	/* ASG RS 67 */
# define DOT 68
# define STREF 69
# define CALL 70
	/* UNARY CALL 72 */
# define NOT 76
# define COMPL 77
# define INCR 78
# define DECR 79
# define EQ 80
# define NE 81
# define LE 82
# define NLE 83
# define LT 84
# define NLT 85
# define GE 86
# define NGE 87
# define GT 88
# define NGT 89
# define LG 90
# define NLG 91
# define LGE 92
# define NLGE 93
# define ULE 94
# define UNLE 95
# define ULT 96
# define UNLT 97
# define UGE 98
# define UNGE 99
# define UGT 100
# define UNGT 101
# define ULG 102
# define UNLG 103
# define ULGE 104
# define UNLGE 105
# define ARS 106
	/* ASG ARS 107 */
# define REG 108
# define TEMP 109
# define CCODES 110
# define FREE 111
# define NOT_FREE -FREE
# define STASG 112
# define STARG 113
# define STCALL 114
	/* UNARY STCALL 116 */

/*	some conversion operators */
# define FLD 117
# define CONV 118
# define PMUL 119
# define PDIV 120

/*	special node operators, used for special contexts */
# define GENLAB 122
# define CBRANCH 123
# define GENBR 124
# define CMP 125
# define CMPE 126	/* if IEEE, used for exception raising fp cmps */
# define GENUBR 127
# define INIT 128
# define CAST 129
# define FUNARG 130
# define VAUTO 131
# define VPARAM 132
# define RNODE 133
# define SNODE 134
# define QNODE 135
# define REGARG  136
# define EH_OFFSET  137
# define EH_CATCH_VALS  138
# define EH_COMMA  139
# define EH_LABEL  140
# define EH_A_SAVE_REGS 141
# define EH_A_RESTORE_REGS 142
# define EH_B_SAVE_REGS  143
# define EH_B_RESTORE_REGS  144
# define DUMMYARG  145
#ifdef	IN_LINE
# define INCALL 146			/* beware of UNARY INCALL == 148! */
#endif
# define MANY  147
	/* UNARY INCALL 148 */

/* ops used for NAIL*/
# define SEMI	149
# define ENTRY	150
# define PROLOG	151
# define ENDF	152
# define LOCCTR	153
# define SWBEG	155
# define SWCASE	156
# define SWEND	157
# define DEFNAM	159
# define UNINIT	160
# define BMOVE	164
# define BMOVEO	165
# define JUMP	170
# define SINIT  171
# define LET	172
# define CSE	173
# define ALIGN	176
# define BCMP	181
# define COPYASM 183
# define NOP	184
# define COPY	186
# define BEGF	187
# define LABELOP 192
# define UPLUS	193	/* ANSI C unary + */
# define NAMEINFO 194

	/* DSIZE is the size of the dope array:  highest OP # + 1 */
#define DSIZE  NAMEINFO+1

/*	node types */
# define LTYPE 02
# define UTYPE 04
# define BITYPE 010


#if 0 /*no longer used*/
/*	type names, used in symbol table building */
# define TNULL 0
# define FARG 1
# define CHAR 2
# define SHORT 3
# define INT 4
# define LONG 5
# define FLOAT 6
# define DOUBLE 7
# define STRTY 8
# define UNIONTY 9
# define ENUMTY 10
# define MOETY 11
# define UCHAR 12
# define USHORT 13
# define UNSIGNED 14
# define ULONG 15
# define VOID 16
# define LDOUBLE 17
# define UNDEF 18
#endif /*0*/

# define ASG 1+
# define UNARY 2+
# define NOASG (-1)+
# define NOUNARY (-2)+

/*	various flags */
# define NOLAB (-1)

/* type modifiers */

# define PTR  040
# define FTN  0100
# define ARY  0140

/* type packing constants */

# define MTMASK 03
# define BTMASK 037
# define BTSHIFT 5 
# define TSHIFT 2
# define TMASK (MTMASK<<BTSHIFT)
# define TMASK1 (MTMASK<<(BTSHIFT+TSHIFT))
# define TMASK2  (TMASK||MTMASK)

/*	macros	*/

# ifndef BITMASK
	/* beware 1's complement */
# define BITMASK(n) (((n)==SZLONG)?-1L:((1L<<(n))-1))
# endif
# define ONEBIT(n) (1L<<(n))
# define MODTYPE(x,y) x = (x&(~BTMASK))|y  /* set basic type of x to y */
# define BTYPE(x)  (x&BTMASK)   /* basic type of x */

#if 0 /*no longer used*/
# define ISUNSIGNED(x) ((x)<=ULONG&&(x)>=UCHAR)
# define UNSIGNABLE(x) ((x)<=LONG&&(x)>=CHAR)
# define ENUNSIGN(x) ((x)+(UNSIGNED-INT))
# define DEUNSIGN(x) ((x)+(INT-UNSIGNED))
#endif /*0*/

# define ISPTR(x) ((x&TMASK)==PTR)
# define ISFTN(x)  ((x&TMASK)==FTN)  /* is x a function type */
# define ISARY(x)   ((x&TMASK)==ARY)   /* is x an array type */
# define INCREF(x) (((x&~BTMASK)<<TSHIFT)|PTR|(x&BTMASK))
# define DECREF(x) (((x>>TSHIFT)&~BTMASK)|(x&BTMASK))
# define SETOFF(x,y)   if( (x)%(y) != 0 ) x = ( ((x)/(y) + 1) * (y))
		/* advance x to a multiple of y */
# define NOFIT(x,y,z)   ( ((x)%(z) + (y)) > (z) )
	/* can y bits be added to x without overflowing z */
	/* pack and unpack field descriptors (size and offset) */
# define PKFIELD(s, o)	(((o) << 8) | (s))
# define UPKFSZ(v)	((v) & 0xff)
# define UPKFOFF(v)	((v) >> 8)

#ifdef FP_EMULATE

/* This include is ugly, but the list of search directories is getting
** too long.  Some pre-processors have hard limits on the number of -I's.
*/
#include "../../fpemu/common/fpemu.h"

#define FP_FLOAT        fpemu_f_t
#define FP_DOUBLE       fpemu_d_t
#define FP_LDOUBLE      fpemu_x_t
#define FP_CMPX(x1,x2)  fpemu_compare(x1,x2)

/* Round x to precision of float */
#define FP_XTOFP(x)     fpemu_xtofp(x)

/* Round x to precision of double */
#define FP_XTODP(x)     fpemu_xtodp(x)

#define FP_XTOF(x)      fpemu_xtof(x)
#define FP_XTOD(x)      fpemu_xtod(x)
#define FP_XTOL(x)      fpemu_xtol(x)
#define FP_XTOUL(x)     fpemu_xtoul(x)
#define FP_LTOX(l)      fpemu_ltox(l)
#define FP_ULTOX(u)     fpemu_ultox(u)
#define FP_NEG(x)       fpemu_neg(x)
#define FP_PLUS(x1,x2)  fpemu_add(x1,x2)
#define FP_MINUS(x1,x2) fpemu_add(x1,FP_NEG(x2))
#define FP_TIMES(x1,x2) fpemu_mul(x1,x2)
#define FP_DIVIDE(x1,x2) fpemu_div(x1,x2)
#define FP_ISZERO(x)    fpemu_iszero(x)
#define FP_ISPOW2(x)	fpemu_ispow2(x)
#define FP_ATOF(s)      fpemu_atox(s)
#define FP_XTOA(x)      fpemu_xtoa(x)
#define FP_XTOH(x)	fpemu_xtoh(x)

#endif /* FP_EMULATE */

/* Default definitions in the absence of emulation. */

extern int errno;
#ifndef FP_LDOUBLE
#define FP_LDOUBLE	long double	/* type containing long doubles */
#endif
#ifndef	FP_DOUBLE
#define	FP_DOUBLE	double	/* type containing doubles */
#endif
#ifndef	FP_FLOAT
#define	FP_FLOAT	float	/* type containing floats */
#endif

#if !defined(FP_XTOA) || !defined(FP_XTOH)
static fp_xto_buf[32]; /* assumed to be big enough */
#endif

#ifndef FP_XTOA
#define FP_XTOA(x) (sprintf(fp_xto_buf, "%Le", x), fp_xto_buf)
#endif
#ifndef FP_XTOH
#define FP_XTOH(x) (sprintf(fp_xto_buf, "%La", x), fp_xto_buf)
#endif
#ifndef FP_XTOFP                /* truncate long double to float precision */
#define FP_XTOFP(x) ((long double) FP_XTOF(x))
#endif
#ifndef FP_XTOD                 /* convert long double to double */
#define FP_XTOD(x) ((double) x)
#endif
#ifndef FP_XTODP                /* truncate long double to float precision */
#define FP_XTODP(x) ((long double) FP_XTODP(x))
#endif
#ifndef FP_XTOL                 /* convert long double to long */
#define FP_XTOL(x) ((long) (x))
#endif
#ifndef FP_XTOUL                /* convert long double to unsigned long */
#define FP_XTOUL(x) ((unsigned long) (x))
#endif
#ifndef FP_LTOX                 /* convert long to long double */
#define FP_LTOX(l) ((long double) (x))
#endif
#ifndef FP_ULTOX                /* convert unsigned long to long double */
#define FP_ULTOX(ul) ((long double) (ul))
#endif
#ifndef FP_NEG                  /* negate long double */
#define FP_NEG(x) (-(x))
#endif
#ifndef FP_PLUS                 /* add long double */
#define FP_PLUS(x1,x2) ((x1)+(x2))
#endif
#ifndef FP_MINUS                /* subtract long double */
#define FP_MINUS(x1,x2) ((x1)-(x2))
#endif
#ifndef FP_TIMES                /* multiply long double */
#define FP_TIMES(x1,x2) ((x1)*(x2))
#endif
#ifndef FP_DIVIDE               /* divide long double */
#define FP_DIVIDE(x1,x2) ((x1)/(x2))
#endif
#ifndef FP_ISZERO               /* is long double value zero? */
#define FP_ISZERO(x) (!(x))
#endif
#ifndef FP_ISPOW2
#define FP_ISPOW2(x)	(0)
#endif
#undef FPEMU_CMP_UNORDERED
#define FPEMU_CMP_UNORDERED	(-2)
#undef FPEMU_CMP_LESSTHAN
#define FPEMU_CMP_LESSTHAN	(-1)
#undef FPEMU_CMP_EQUALTO
#define FPEMU_CMP_EQUALTO	0
#undef FPEMU_CMP_GREATERTHAN
#define FPEMU_CMP_GREATERTHAN	1
#ifndef FP_CMPX                /* compare two long doubles */
#define FP_CMPX(x,y) ((x) == (y) ? FPEMU_CMP_EQUALTO \
			: (x) < (y) ? FPEMU_CMP_LESSTHAN \
			: (x) > (y) ? FPEMU_CMP_GREATERTHAN \
			: FPEMU_CMP_UNORDERED)
#endif
#ifndef FP_ATOF                         /* convert string to FP_LDOUBLE */
#ifndef FLOATCVT                        /* backward compatibility */

#define FP_ATOF(s) strtold(s, (char **)0)
#  ifdef c_plusplus
extern double strtold(const char *, char **);
#  else
extern double strtold();
#  endif
#else
#define FP_ATOF(s) FLOATCVT(s)
#endif
#endif

#ifndef NUMSIZE
#   define NUMSIZE SZLLONG
#endif
#include "../../intemu/common/intemu.h"

typedef IntNum INTCON;	/* full-sized integer constants */

extern INTCON	num_0;
extern INTCON	num_1;
extern INTCON	num_2;
extern INTCON	num_3;
extern INTCON	num_neg1;
extern INTCON	num_sc_min;
extern INTCON	num_sc_max;
extern INTCON	num_si_min;
extern INTCON	num_si_max;
extern INTCON	num_ui_max;
extern INTCON	num_sl_min;
extern INTCON	num_sl_max;
extern INTCON	num_ul_max;
extern INTCON	num_sll_min;
extern INTCON	num_sll_max;
extern INTCON	num_ull_max;

extern FP_LDOUBLE	flt_0;
extern FP_LDOUBLE	flt_1;
extern FP_LDOUBLE	flt_neg1;
extern FP_LDOUBLE	flt_sll_min_m1;
extern FP_LDOUBLE	flt_sll_max_p1;
extern FP_LDOUBLE	flt_ull_max_p1;

/*	operator information */

# define TYFLG 016
# define ASGFLG 01
# define LOGFLG 020

# define SIMPFLG 040
# define COMMFLG 0100
# define DIVFLG 0200
# define FLOFLG 0400
# define LTYFLG 01000
# define CALLFLG 02000
# define MULFLG 04000
# define SHFFLG 010000
# define ASGOPFLG 020000

# define SPFLG 040000
# define STRFLG 0100000		/* for structure ops */
# define AMBFLG	0200000		/* ops that cause ambiguity */

#	define RS_BIT(n) (((RST) 1) << (n)) /* bit corresponding to reg n */
#	define RS_PAIR(n) ((RST)(n) << 1) /* reg n's pair companion */
/* Choose a register from a register set bit vector.  Used to pick a
** scratch register.  Always choose right-most bit.
*/
#	define RS_CHOOSE(vec) ((vec) & ~((vec)-1))
/* Choose left-most bit.  This one's a function. */

#	define RS_NONE	((RST) 0)	/* no register bits */
/* these definitions are slight perversions of the use of RS_BIT */
#	define RS_NRGS	(RS_BIT(NRGS)-1) /* bits for all scratch registers */
#	define RS_TOT	(RS_BIT(TOTREGS)-1) /* bits for all registers */

#define optype(o) (dope[o]&TYFLG)
#define asgop(o) (dope[o]&ASGFLG)
#define asgbinop(o) (dope[o]&ASGOPFLG)
#define logop(o) (dope[o]&LOGFLG)
#define mulop(o) (dope[o]&MULFLG)
#define shfop(o) (dope[o]&SHFFLG)
#define callop(o) (dope[o]&CALLFLG)
#define structop(o) (dope[o]&STRFLG)
#define ambop(o) (dope[o]&AMBFLG)
#define FUNARGOP(o) ((o) == FUNARG || (o) == REGARG)

/*	for CG: two basic exceptions*/
#define NUMERIC 1
#define STACKOV 2
/* 	numeric exception types*/
#define EXLSHIFT	01	/*dropped bit on ls*/
#define EXCAST		02	/*dropped bit on convert*/
#define EXINT		04	/*integer overflow*/
#define EXFLOAT		010	/*floating overflow*/
#define EXDBY0		020	/*div by zero*/

/*	table sizes	*/

/* The following undef-initions will help flag places where
** older machine-dependent code depends on fixed size tables.
*/
#undef BCSZ
#undef MAXNEST
#undef SYMTSZ
#undef DIMTABSZ
#undef PARAMSZ
#undef ARGSZ
#undef TREESZ
/* keep this one for non-users of MAKEHEAP */
# ifndef SWITSZ
# define SWITSZ 250 /* size of switch table */
# endif
# ifndef YYMAXDEPTH
# define YYMAXDEPTH 300 /* yacc parse stack */
# endif

/* The following define initial sizes for dynamic tables. */

#ifndef INI_NINS		/* inst[]:  table of generated instructions */
#define INI_NINS 50
#endif

#ifndef	INI_SWITSZ		/* swtab[]:  switch case table */
#define	INI_SWITSZ 100
#endif

#ifndef	INI_HSWITSZ		/* heapsw[]:  heap-sorted switch table */
#define	INI_HSWITSZ 25
#endif

#ifndef INI_BBFSZ		/* bb_flags[]:  .bb debug flag table */
#define INI_BBFSZ 10
#endif

#ifndef INI_FAKENM		/* mystrtab[]:  unnamed struct fake names */
#define INI_FAKENM 15
#endif

#ifndef	INI_N_MAC_ARGS		/* macarg_tab[]:  enhanced asm arg. names */
#define INI_N_MAC_ARGS 5
#endif

#ifndef	INI_SZINLARGS		/* inlargs[]:  buffer for enhanced asm. formals */
#define	INI_SZINLARGS 50
#endif

#ifndef INI_ASMBUF		/* asmbuf[]:  buffer for asm() lines */
#define INI_ASMBUF 20
#endif

#ifndef	INI_SYMTSZ		/* stab[]:  symbol table */
#define	INI_SYMTSZ 500
#endif

#ifndef INI_RNGSZ		/*case_ranges[]: ranges for big cases (CG) */
#define INI_RNGSZ INI_SWITSZ
#endif

/* dimtab must be at least 16:  see mainp1() */
#if !defined(INI_DIMTABSZ) || INI_DIMTABSZ < 20
#undef INI_DIMTABSZ		/* dimtab[]:  dimension (and misc.) table */
#define INI_DIMTABSZ 400
#endif

#ifndef	INI_PARAMSZ		/* paramstk[]:  struct parameter stack */
#define INI_PARAMSZ 100
#endif

#ifndef	INI_BCSZ		/* asavbc[]:  block information */
#define INI_BCSZ 50
#endif

/* scopestack should be at least 3 (indices 0-2) to handle simple fct. */
#if !defined(INI_MAXNEST) || INI_MAXNEST < 3
#undef INI_MAXNEST		/* scopestack[]:  sym. tab. scope stack */
#define INI_MAXNEST 20
#endif

#ifndef INI_ARGSZ		/* argstk[], argsoff[], argty[]:  incoming
				** arg information
				*/
#define INI_ARGSZ 15
#endif

#ifndef INI_INSTK		/* instack[]:  initialization stack */
#define INI_INSTK 10
#endif

#ifndef	INI_TREESZ		/* number of nodes per cluster */
#define	INI_TREESZ 100
#endif

#ifndef	INI_MAXHASH		/* number of segmented scanner hash tables */
#define	INI_MAXHASH 20
#endif

#ifndef	INI_NTSTRBUF		/* number of temp string buffers */
#define	INI_NTSTRBUF 20
#endif


/*	turn on NONEST if can't nest calls or allocs */
#  if defined(NOFNEST) || defined(NOANEST)
#    define NONEST
#  endif
	char		*tstr();
	extern FILE *	outfile;

#	define NCHNAM 8  /* number of characters in a truncated name */

/*	common defined variables */

extern int nerrors;  /* number of errors seen so far */

typedef unsigned long BITOFF;
typedef unsigned int TWORD;
typedef long OFFSET;	/* memory address offsets */

	/* default is byte addressing */
	/* BITOOR(x) converts bit width x into address offset */
# ifndef BITOOR
# define BITOOR(x) ((x)/SZCHAR)
# endif
# ifndef ORTOBI
# define ORTOBI(x) ((x)*SZCHAR)
# endif

#define OKTYPE  (TINT | TUNSIGNED | TLONG | TULONG | TLLONG | TULLONG \
		| TPOINT | TPOINT2 | TSHORT | TUSHORT | TCHAR | TUCHAR)

/* Many architectures can't handle literal double/float constants.
** For those that do, define LITDCON to return 1 in situations where
** a literal is permitted.  p is the FCON node.  Otherwise return 0,
** and a named constant will be defined.
*/

#ifndef	LITDCON
#define	LITDCON(p)	(0)
#endif

#if defined(TMPSRET) && !defined(AUXREG)
#define AUXREG (NRGS - 1)
#endif

# define NIL (NODE *)0

extern long dope[];  /* a vector containing operator information */
extern char *opst[];  /* a vector containing names for ops */

# define NCOSTS (NRGS+4)

/* The idea of all of the machinations below for nodes is to create
** compatible definitions of NODE, ND1, and ND2.  The existing code
** that deals with NODEs must continue to do so, and yet the code
** for ND1s and ND2s must be self-consistent as well.  Moreover,
** it's important that the proper corresponding fields be accessed
** with the appropriate names.
*/

/* This #define gives those items that must be in both first
** and second pass nodes.
*/

#define ND12(name) \
    int op;		/* opcode */				\
    TWORD type;		/* type encoding */			\
    name *left;		/* left operand */			\
    name *right;	/* right operand */			\
    int sid;		/* usually stab entry */		\
    unsigned long branch_stuff; /* mark predictable branches */	\
    union {		/* holds various constants */		\
      OFFSET off;	/* offsets and some signed constants */	\
      BITOFF size;	/* sizes and some unsigned constants */	\
      int label;	/* used by most "branch" nodes */	\
      INTCON ival;	/* full-sized integers */		\
      FP_LDOUBLE fval;	/* floating */				\
    } c

/* Pass 1 node */
#define DEF_P1NODE(sname,pname) struct sname {		\
    ND12(pname);	/* the above, and ... */	\
    int flags;		/* special pass 1 flags */	\
    int cdim;		/* dimoff */			\
    int csiz;		/* sizoff */			\
    TWORD sttype;	/* actual struct/union type for s/u ops */ \
    char *string;	/* ptr. to STRING string */	\
    struct Expr_info *opt;				\
};

/* Pass 2 node */
#define DEF_P2NODE(sname,pname) struct sname {		\
    ND12(pname);	/* the basics, plus ... */	\
    int goal;		/* code generation goal */	\
    int lop;		/* GENBR branch condition */	\
    char *name;		/* pointer to name string */	\
    int stsize;		/* structure size */		\
    short stalign;	/* structure alignment */	\
    BITOFF argsize;	/* size of CALL argument list */ \
    int scratch;	/* index to scratch register vectors */ \
    int strat;		/* code generation strategy */ \
    int id;		/* name of CSE destination */	\
};

typedef struct nndu nNODE;
typedef union ndu NODE;		/* old-style node */
typedef struct nND1 ND1;	/* new first pass node */
typedef struct nND2 ND2;	/* new second pass node */




/* Define several different flavors of structures that
** depend on the type of the pointer contained within.
*/
DEF_P1NODE(nND1,ND1)
DEF_P2NODE(nND2,ND2)
DEF_P1NODE(oND1,NODE)
DEF_P2NODE(oND2,NODE)


union ndu {
    struct oND1 fn;	/* front end node */
    struct oND1 fpn;	/* floating point node */

    struct oND2 in;	/* interior (binary) node */
    struct oND2 tn;	/* terminal (leaf) node */
    struct oND2 bn;	/* branch node */
    struct oND2 stn;	/* structure node */
    struct oND2 csn;	/* CSE node */
};



struct nndu {
    NODE node;
#ifndef NODBG
    int _node_no;
#endif
    nNODE *free_list_next;
} /* type is nNODE */;			/* numbered node */

#define node_no(p) /*NODE *p*/ (p == 0 ? 0 : ((nNODE *) (p))->_node_no)


/* For CG, file pointers for output and debugging*/
extern FILE *textfile;		/* user-requested output file.
				** Normally same as outfile.
				*/
extern FILE *debugfile; 	/*output file for debugging*/
extern char 	costing;	/*1 if just costing*/
extern int strftn, proflag;
extern int str_spot;		/*place to hold struct return*/
NODE	*firstl(), *lastl();	/*CG functions*/

/* Location counters.  Formerly in mfile1.h, moved here for nail's
** benefit.
*/
# define PROG 0
# define ADATA 1
# define DATA 2
# define ISTRNG 3
# define STRNG 4
# define CDATA 5	/* constants */
# define CSTRNG 6	/* constant strings */
# define EH_RANGES 7
# define EH_RELOC 8
# define EH_OTHER 9
# define CTOR 10	/* section for static constructors */
# define CURRENT 11	/* Return current locctr*/
# define UNK 12		/* Unknown (inital) state */

/* Guarantee that const and myVOID are defined.  myVOID used for pointers. */
#ifndef	__STDC__
# ifndef const
# define const				/* this vanishes in old-style C */
# endif
typedef char myVOID;
#else
typedef void myVOID;
#endif

/* Define table descriptor structure for dynamically
** managed tables.
*/

struct td {
    int td_allo;		/* array elements allocated */
    int td_used;		/* array elements in use */
    int td_size;		/* sizeof(array element) */
    int td_flags;		/* flags word */
    myVOID * td_start;		/* (really void *) pointer to array start */
    myVOID * td_name;		/* table name for error */
#ifdef STATSOUT
    int td_max;			/* maximum value reached */
#endif
};

/* flags for td_flags */
#define TD_MALLOC 	1	/* array was malloc'ed */
#define TD_ZERO		2	/* zero expansion area */

int td_enlarge();		/* enlarges a table so described, returns
				** old size
				*/

#define TD_INIT(tab, allo, size, flags, start, name) \
struct td tab = { \
	allo,	/* allocation */ \
	0,	/* used always 0 */ \
	size,	/* entry size */ \
	flags,	/* flags */ \
	(myVOID *)start, /* pointer */ \
	name	/* table name */ \
}

#ifdef	OPTIM_SUPPORT

/* Definitions for HALO optimizer interface. */

/* "Storage classes" */
#define	OI_AUTO		1
#define	OI_PARAM	2
#define	OI_EXTERN	3
#define	OI_EXTDEF	4
#define	OI_STATEXT	5
#define	OI_STATLOC	6
#define	OI_RPARAM	7 /* PARM in REG */

/* Loop codes */
#define	OI_LSTART	10
#define	OI_LBODY	11
#define	OI_LCOND	12
#define	OI_LEND		13

/* Routines: */
extern char * oi_loop();
extern char * oi_symbol();
extern char * oi_alias();

#endif	/* def OPTIM_SUPPORT */

#ifdef FIXED_FRAME
	/* Access to string used in fixed frame style addresses.
	** Eventually it is .set to the frame size.
	** The "PLUS" version has a "+" tacked on the front.
	*/
#define FRAME_OFFSET_STRING_LEN 20 /* Storage for following */
#define FRAME_OFFSET_STRING (frame_offset_string(0))
#define PLUS_FRAME_OFFSET_STRING (frame_offset_string(0)-1)
extern char *frame_offset_string();
#endif

extern void	special_instr_end();

extern NODE * talloc();
extern void load_globals(), restore_globals();


/* C++ compatible declarations */
#ifdef c_plusplus

extern OFFSET
	next_temp(TWORD, BITOFF, int),
	next_arg(TWORD, BITOFF, int),
	off_conv(int, OFFSET, TWORD, TWORD),
	off_bigger(int, OFFSET, OFFSET),
	off_incr(int, OFFSET, long),
	max_temp(void), max_arg(void),
	get_temp_offset(void); 

extern int    
	p2done(void),
	cisreg(TWORD, char*);
	rgsave(char*),
	tyreg(TWORD),
	gtalgin(TWORD),
	gtsize(TWORD),
	tchech(void),
	off_is_err(int, OFFSET);

extern void   
	cg_map_section(int, char *),
	p2init(void),
	p2abort(void),
	p2compile(NODE *),
	p2flags(char *),
        bind_param(TWORD, TWORD, OFFSET, int *),
	bind_global(char *,TWORD, int, int, int, int),
	tfree(NODE *),
	nfree(NODE *),
	ret_type(TWORD),
	ofile(FILE *),
	profile(int),
	tshow(void),
	set_next_temp(OFFSET),
	set_next_arg(OFFSET),
	off_init(int),
	restore_temp_offset(OFFSET);
#else

extern OFFSET
	next_temp(),
	next_arg(),
	off_conv(),
	off_bigger(),
	off_incr(),
	max_temp(),
	max_arg(),
	get_temp_offset(); 
	
extern int
	cisreg(),
	tcheck(),
	p2done(),
	rgsave(),
	tyreg(),
#ifdef FIXED_FRAME
	fixed_frame(),	/* returns true if we are using fixed stack frame */
#endif
	gtalign(),
	gtsize(),
	ieee_fp(),
	inline_alloca(),	/* returns true if alloca should be inlined */
	inline_intrinsics(),	/* returns true if any intrinsics should be inlined */
	hosted(),
	off_is_err();

extern void
	p2compile(),
	p2init(),
	p2abort(),
	p2flags(),
	bind_param(),
	bind_global(),
	ret_type(),
	ofile(),
	profile(),
	set_next_temp(),
	set_next_arg(),
	off_init(),
	tshow(),
	tfree(),
	cg_map_section(),
	nfree(),
	restore_temp_offset();
#endif

#ifdef	IN_LINE
/* Declarations for enhanced asm support. */
#ifdef __STDC__
extern void as_gencode(ND2 *, FILE *);
extern void as_start(char *);
extern void as_param(char *);
extern void as_e_param(void);
extern void as_putc(int);
extern void as_end(void);
extern int as_intrinsic(char *);
#else	/* no prototypes */
extern void as_gencode();
extern void as_start();
extern void as_param();
extern void as_e_param();
extern void as_putc();
extern void as_end();
extern int as_intrinsic();
#endif
#endif	/* def IN_LINE */

/* Error reporting routines. */
#ifdef __STDC__
extern void uerror(const char *, ...);
extern void werror(const char *, ...);
extern void cerror(const char *, ...);
#else
extern void uerror();
extern void werror();
extern void cerror();
#endif	/* def __STDC__ */

#endif	/* def MANIFEST_H:  from top of file */
