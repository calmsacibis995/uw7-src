#ident	"@(#)nas:common/as.h	1.8"
/*
* common/as.h - common assembler main header
*
* Includes:
*	<libelf.h> - if FORMAT == ELF
*/

	/*
	* Target object file format possibilities.
	*/
#define COFF	1	/* not implemented */
#define ELF	2

#if FORMAT == ELF
#  include <libelf.h>
#endif

#ifdef __STDC__
#  include <stddef.h>
#  include <limits.h>
#else
#  ifndef size_t
#     define size_t unsigned
#  endif
#  define CHAR_BIT	8	/* good guess; verified at startup */
#  define const
#endif

/*
** Intel Pentium Erratum # 41 - Incorrect decode of certain 0F instructions.
**
** The assembler will add padding where necessary to avoid a potential
** problem.  At present, a work-around for this erratum is unconditionally
** done.  In the future, it may be worthwhile adding as a user requested
** option.  Code being generated for chip specific applications may chose
** to bypass the generation of nops.
*/
#define P5_ERR_41 1 

#ifdef DEBUG
#  undef DEBUG
	/*
	* Debugging letter key.  By convention, the lower case letters
	* are used for common code:
	*	a - allocation
	*	c - section handling
	*	d - directives
	*	e - expression and operand handling
	*	h - hashing distributions
	*	n - number building and calculation
	*	o - object file handling
	*	t - symbol table handling
	*	v - variable-sized (span-dependent) instruction handling
	*
	* Target-specific debugging conventially uses upper case letters.
	* Certain common features may use these letters:
	*	D - directives
	*	H - hashing distributions
	*	I - instructions/statements
	*	R - relocation
#ifdef P5_ERR_41
	*	P - Pentium erratum #41 - incorrect decode of certain 0f
	*				  instructions - print stats
#endif
	*/
   extern int	dbflgs[];		/* debugging flag levels */
#  define DEBUG(n) (dbflgs[(n) < 'a' || (n) > 'z' ? (n) - 'A' + 26 : (n) - 'a'])
#endif

#define BIT(n)	(1 << (n))				/* n-th bit position */
#define MASK(n) ((n) >= sizeof(Ulong) * CHAR_BIT ? \
			(~(Ulong)0) : (~(~(Ulong)0 << (n))))

	/*
	* Shorthand names for unsigned integer types.
	*/
typedef unsigned char	Uchar;		/* 0 to at least 2**8 - 1 */
typedef unsigned short	Ushort;		/* 0 to at least 2**16 - 1 */
typedef unsigned int	Uint;		/* 0 to at least 2**16 - 1 */
typedef unsigned long	Ulong;		/* 0 to at least 2**32 - 1 */

	/*
	* Major data types.
	* Some contain implementation-dependent information.
	*/
typedef struct t_code_	Code;		/* defined in sect.h */
typedef struct t_eval_	Eval;		/* defined in eval.h */
typedef struct t_expr_	Expr;		/* defined in expr.h */
typedef struct t_inst_	Inst;		/* defined in stmt.h */
typedef struct t_oper_	Operand;	/* defined in expr.h */
typedef struct t_olst_	Oplist;		/* defined in expr.h */
typedef struct t_sect_	Section;	/* defined in sect.h */
typedef struct t_syms_	Symbol;		/* defined in syms.h */

typedef struct _fpemu_x_t	Float;		/* imported from fpemu.h */
typedef union _intnum_t	Integer;	/* imported from intemu.h */

extern Ulong	curlineno;		/* current input line number */
extern Ushort	curfileno;		/* current input file number */

extern Ulong	flags;			/* global flag bits: */
#define ASFLAG_TRANSITION	0x01	/* smooth COFF to ELF transition */
#ifdef P5_ERR_41
#define ASFLAG_P5_ERR		0x80	/* flag to accumulate P5 erratum #41
					   information */
#endif

	/* misc. functions */
#ifdef __STDC__
		/*PRINTFLIKE1*/
void	fatal(const char *, ...);	/* print message and exit */
		/*PRINTFLIKE1*/
void	error(const char *, ...);	/* print user error message */
		/*PRINTFLIKE3*/
void	backerror(Ulong, Ulong, const char *, ...); /* back ref'd error */
		/*PRINTFLIKE2*/
void	exprerror(const Expr *, const char *, ...); /* error w/expr's loc */
		/*PRINTFLIKE1*/
void	warn(const char *, ...);	/* print warning message */
		/*PRINTFLIKE3*/
void	backwarn(Ulong, Ulong, const char *, ...); /* back ref'd warning */
		/*PRINTFLIKE2*/
void	exprwarn(const Expr *, const char *, ...); /* warn w/expr's loc */
void	*alloc(void *, size_t);		/* realloc() front end */
char	*prtstr(const Uchar *, size_t);	/* return printable version of str */
#else
void	fatal();
void	error(), backerror(), exprerror();
void	warn(), backwarn(), exprwarn();
char	*alloc();			/* malloc()+realloc() front end */
char	*prtstr();
#endif
