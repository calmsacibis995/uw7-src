#ident	"@(#)alint:common/lnstuff.h	1.3.1.4"

#define LDI 01		/* defined and initialized: storage set aside   */
#define LIB 02		/* defined on a library				*/
#define LDC 04		/* defined as a common region on UNIX		*/
#define LDX 010		/* defined by an extern: if ! pflag, same as LDI*/
#define LRV 020		/* function returns a value			*/
#define LUV 040		/* function used in a value context		*/
#define LUE 0100	/* function used in effects context		*/
#define LUM 0200	/* mentioned somewhere other than at the declaration*/
#define LDS 0400	/* defined static object (like LDI)		*/
#define LFN 01000	/* filename record 				*/
#define LSU 02000	/* struct/union def				*/
#define LPR 04000	/* prototype declaration			*/
#define LND 010000	/* end module marker				*/
#define LPF 020000	/* printf like					*/
#define LSF 040000	/* scanf like					*/
#define LWF 0100000	/* wide ... like				*/

#define LNQUAL		00037		/* type w/o qualifiers		*/
#define LNUNQUAL	0174000		/* remove type, keep other info */
#define LCON		(1<<15)		/* type qualified by const	*/
#define LVOL		(1<<14)		/* type qualified by volatile	*/
#define LNOAL		(1<<13)		/* not used */
#define LCONV		(1<<12)		/* type is an integer constant	*/
#define LPTR		(1<<11)		/* last modifier is a pointer	*/
#define LINTVER		4

typedef unsigned short ushort;
typedef long FILEPOS;
typedef short TY;

typedef struct flens {
	long f1,f2,f3,f4;
	ushort ver, mno;
} FLENS;

typedef struct {
	TY aty;			/* base type 			*/
	unsigned long dcl_mod;	/* ptr/ftn/ary modifiers 	*/
	ushort dcl_con;		/* const qualifiers		*/
	ushort dcl_vol;		/* volatile qualifiers		*/
	union {
		T1WORD ty;
		FILEPOS pos;
	} extra;
} ATYPE;

typedef struct {
	short decflag;		/* what type of record is this	*/
	short nargs;		/* # of args (or members)	*/
	int fline;		/* line defined/used in		*/
	ATYPE type;		/* type information		*/
} LINE;

union rec {
	LINE l;
	struct {
		short decflag;
		char *fn;
	} f;
};


/* type modifiers */
# define LN_PTR 1
# define LN_FTN 2
# define LN_ARY 3

# define LN_TMASK 3

# define LN_ISPTR(x)	(((x)&LN_TMASK) == LN_PTR)
# define LN_ISFTN(x)	(((x)&LN_TMASK) == LN_FTN)  /* is x a function type */
# define LN_ISARY(x)	(((x)&LN_TMASK) == LN_ARY)  /* is x an array type */



/* type numbers for pass2 */

enum {
	LN_CHAR,	LN_UCHAR,	LN_SCHAR,
	LN_SHORT,	LN_USHORT,	LN_SSHORT,
	LN_INT,		LN_UINT,	LN_SINT,
	LN_LONG,	LN_ULONG,	LN_SLONG,
	LN_LLONG,	LN_ULLONG,	LN_SLLONG,
	LN_FLOAT,	LN_DOUBLE,	LN_LDOUBLE,
	LN_VOID,
	LN_STRUCT,	LN_UNION,	LN_ENUM,

	/* special types only for printf/scanf format checking */
	LN_GCHAR,
	LN_GSHORT,
	LN_GINT,
	LN_GLONG,
	LN_GLLONG,
	LN_PTR_VOID
};
