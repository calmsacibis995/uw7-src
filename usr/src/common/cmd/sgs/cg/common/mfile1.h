#ident	"@(#)cg:common/mfile1.h	1.16"


#ifndef	MACDEFS_H			/* guard against double include */
# include "macdefs.h"
#define	MACDEFS_H
#endif
# include "manifest.h"

/*	storage classes  */
# define SNULL 0
# define AUTO 1
# define EXTERN 2
# define STATIC 3
# define REGISTER 4
# define EXTDEF 5
# define LABEL 6
# define ULABEL 7
# define MOS 8
# define PARAM 9
# define STNAME 10
# define MOU 11
# define UNAME 12
# define TYPEDEF 13
# define FORTRAN 14
# define ENAME 15
# define MOE 16
# define UFORTRAN 17
# define USTATIC 18

#ifdef IN_LINE
#define INLINE 19
#endif

	/* field size is ORed in */
# define FIELD 0100
# define FLDSIZ 077
# ifndef NODBG
extern char *scnames();
# endif

/* symbol table flags */
# define SMOS 01
/* SHIDDEN 02 and SHIDES 04 were removed.
** Their codes have been reused.
*/
# define SLABEL 02			/* symbol is label */
# define SISREG 04			/* declared as REG, but not in REG */
# define SSET 010
# define SREF 020
# define SNONUNIQ 040
# define STAG 0100

/* flags for fn.flags field */
#define	FF_ISREG	01		/* user wanted this (leaf) node to be REG */
#define	FF_ISFLD	02		/* node started as bitfield */
/* three flags available to implementations */
#define	FF_MDP1		01000
#define	FF_MDP2		02000

typedef long OFFSZ;

struct symtab {
	char *sname;
	TWORD stype;		/* type word */

	char sclass;		/* storage class */
	char slevel;		/* scope level */
	char sflags;		/* flags for set, use, hidden, mos, etc. */
	int offset;		/* offset or value */
	short dimoff;		/* offset into the dimension table */
	short sizoff;		/* offset into the size table */
	short suse;		/* line number of last use of the variable */
	unsigned short st_scopelink;
				/* index of next symbol at current scope level */
	unsigned short st_next;	/* index of next symbol on hash chain */
	unsigned short * st_own;/* pointer to owning hash chain */
};

extern int ftnno;
extern char ftitle[];
extern int strftn;
extern int curloc;
extern RST regvar;			/* bit vector of current reg. vars. */
extern int nextrvar;
extern int strflg;

extern OFFSZ inoff;

#ifdef IMPSWREG
	extern int swregno;
#endif
extern int retlab;

/*	flags used in structures/unions */

# define SEENAME 01
# define INSTRUCT 02
# define INUNION 04
# define FUNNYNAME 010
# define TAGNAME 020

/*	flags used in the (elementary) flow analysis ... */

# define FBRK 02
# define FCONT 04
# define FDEF 010
# define FLOOP 020

/*
* These defines control "while" and "for" loop code generation.
* wloop_level and floop_level each must be set to one of these values.
*/
#define LL_TOP	0	/* test at loop top */
#define LL_BOT	1	/* test at loop bottom */
#define LL_DUP	2	/* duplicate loop test at top and bottom */

/*	flags used for return status */

# define RETVAL 1
# define NRETVAL 2

/*	used to mark a constant with no name field */

# define NONAME 040000

	/* mark an offset which is undefined */

# define NOOFFSET (-10201)

/*	declarations of various functions */

extern void 
	defnam(),
	protect(),
	unprot(),
	jmplab(),
	zecode(),
	sincode(),
	fincode(),
	defalign(),
	deflab(),
	lineid();

/* The following may be be declared as macros in macdefs.h */
#ifndef exname
char *exname(); 
#endif
#ifndef setswreg
extern NODE *setswreg();
#endif
#ifndef sw_cmp_jeq
extern void sw_cmp_jeq();
#endif
#ifndef sw_cmp_jeqlt
extern void sw_cmp_jeqlt();
#endif
#ifndef sw_cmp_jeqgt
extern void sw_cmp_jeqgt();
#endif

extern void rbusy();
extern NODE * clocal();

# define checkst(x) 		/* turn off symbol table checking */

/* type that is equivalent to pointers in size */
# ifndef PTRTYPE
# define PTRTYPE INT
# endif

/* size of hash bucket table */
#ifndef HASHTSZ
#define HASHTSZ 511
#endif
