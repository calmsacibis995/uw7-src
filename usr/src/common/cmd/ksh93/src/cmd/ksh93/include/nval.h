#ident	"@(#)ksh93:src/cmd/ksh93/include/nval.h	1.2"
#pragma prototyped
#ifndef NV_DEFAULT
/*
 * David Korn
 * AT&T Bell Laboratories
 *
 * Interface definitions of structures for name-value pairs
 * These structures are used for named variables, functions and aliases
 *
 */


#include	<ast.h>
#include	<hash.h>

typedef struct Namval Namval_t;
typedef struct Namfun Namfun_t;
typedef struct Namdisc Namdisc_t;
typedef struct Namarray Namarr_t;
/*
 * This defines the template for nodes that have their own assignment
 * and or lookup functions
 */
struct Namdisc
{
	size_t	dsize;
	void	(*putval)(Namval_t*, const char*, int, Namfun_t*);
	char	*(*getval)(Namval_t*, Namfun_t*);
	double	(*getnum)(Namval_t*, Namfun_t*);
	char	*(*setdisc)(Namval_t*, const char*, Namval_t*, Namfun_t*);
	Namval_t *(*create)(Namval_t*, const char*, Namfun_t*);
	char	*(*name)(Namval_t*, Namfun_t*);
	int	(*scope)(Namval_t*, Namval_t*, int, Namfun_t*);
};

struct Namfun
{
	const Namdisc_t *disc;
	Namfun_t *next;
};

/* This is an array template header */
struct Namarray
{
	long	nelem;					/* number of elements */
	void	*(*fun)(Namval_t*,const char*,int);	/* associative arrays */
};

/* attributes of name-value node attribute flags */

#define NV_DEFAULT 0
/* This defines the attributes for an attributed name-value pair node */
struct Namval
{
	HASH_HEADER;			/* space for hash library */
	unsigned short	nvflag; 	/* attributes */
	short 		nvsize;		/* size of item */
#ifdef _NV_PRIVATE
	_NV_PRIVATE
#else	/* hopefully these will become private soon */
	char		*nvalue;
	char		*nvenv;         /* pointer to environment name */
	Namfun_t	*nvfun;         /* pointer to trap disciplines */
#endif /* _NV_PRIVATE */
};

/* The following attributes are for internal use */
#define NV_NOALLOC	0x100	/* don't allocate space for the value */
#define NV_NOFREE	0x200	/* don't free the space when releasing value */
#define NV_ARRAY	0x400	/* node is an array */
#define NV_IMPORT	0x1000	/* value imported from environment */

#define NV_INTEGER	0x2	/* integer attribute */
/* The following attributes are valid only when NV_INTEGER is off */
#define NV_LTOU		0x4	/* convert to uppercase */
#define NV_UTOL		0x8	/* convert to lowercase */
#define NV_ZFILL	0x10	/* right justify and fill with leading zeros */
#define NV_RJUST	0x20	/* right justify and blank fill */
#define NV_LJUST	0x40	/* left justify and blank fill */
#define NV_HOST		(NV_RJUST|NV_LJUST)	/* map to host filename */

/* The following attributes do not effect the value */
#define NV_RDONLY	0x1	/* readonly bit */
#define NV_EXPORT	0x2000	/* export bit */
#define NV_REF		0x4000	/* reference bit */
#define NV_TAGGED	0x8000	/* user define tag bit */

/* The following are used with NV_INTEGER */
#define NV_SHORT	(NV_LTOU)	/* when integers are not long */
#define NV_UNSIGN	(NV_UTOL)	/* for unsigned quantities */
#define NV_DOUBLE	(NV_ZFILL)	/* for floating point */
#define NV_EXPNOTE	(NV_LTOU)	/* for scientific notation */
#define NV_CPOINTER	(NV_RJUST)	/* for pointer */


/*  options for nv_open */

#ifdef _NV_PRIVATE
#   define NV_ADD		(HASH_CREATE|HASH_SIZE((long)sizeof(Namval_t)))
#else
#   define NV_ADD		(HASH_CREATE|HASH_SIZE((long)sizeof(Namval_t))+2*sizeof(char*))
#endif /* _NV_PRIVATE */
						/* add node if not found */
#define NV_NOSCOPE	NV_NOALLOC		/* look only in current scope */
#define NV_ASSIGN	NV_NOFREE		/* assignment is possible */
#define NV_NOASSIGN	0			/* backward compatibility */
#define NV_NOARRAY	NV_ARRAY		/* array name not possible */
#define NV_NOREF	NV_REF			/* don't follow reference */
#define NV_IDENT	0x80			/* name must be identifier */
#define NV_VARNAME	0x800			/* name must be ?(.)id*(.id) */
#define NV_NOADD	NV_IMPORT		/* do not add node */ 	
#define NV_NODISC	NV_IDENT		/* ignore disciplines */


#define NV_PUBLIC	(~(NV_NOSCOPE|NV_ASSIGN|NV_IDENT|NV_VARNAME|NV_NOADD))

/* name-value pair macros */
#define nv_isattr(np,f)		((np)->nvflag & (f))
#define nv_isarray(np)		(nv_isattr((np),NV_ARRAY))

#ifdef _NV_PRIVATE
#   define nv_isnull(np)	(!(np)->nvalue.cp && !(np)->nvfun)  /* strings only */
#else
#   define nv_isnull(np)	(!(np)->nvalue && !(np)->nvfun)  /* strings only */
#endif /* _NV_PRIVATE */
#define nv_name(np)		hashname((Hashbin_t*)(np))
#define nv_size(np)		((np)->nvsize&0xfff)
#define nv_unscope()		(sh.var_tree = hashfree(sh.var_tree))
#define nv_search(name,root,mode)	((Namval_t*)hashlook((Hashtab_t*)(root),name,(mode)^HASH_SCOPE,(char*)0))
#ifdef SHOPT_OO
#   define nv_class(np)		(nv_isattr(np,NV_IMPORT)?0:(Namval_t*)((np)->nvenv))
#endif /* SHOPT_OO */

/* The following are operations for associative arrays */
#define NV_AINIT	1	/* initialize */
#define NV_AFREE	2	/* free array */
#define NV_ANEXT	3	/* advance to next subscript */
#define NV_ANAME	4	/* return subscript name */
#define NV_ADELETE	5	/* delete current subscript */
#define NV_AADD		6	/* add subscript if not found */
#define NV_ACURRENT	7	/* return current subscript Namval_t* */


/* The following are operations for nv_putsub() */
#define ARRAY_BITS	16
#define ARRAY_ADD	(1L<<ARRAY_BITS)	/* add subscript if not found */
#define	ARRAY_SCAN	(2L<<ARRAY_BITS)	/* For ${array[@]} */
#define ARRAY_UNDEF	(4L<<ARRAY_BITS)	/* For ${array} */

/* prototype for array interface*/
extern Namarr_t	*nv_setarray(Namval_t*,void*(*)(Namval_t*,const char*,int));
extern void	*nv_associative(Namval_t*,const char*,int);
extern int	nv_aindex(Namval_t*);
extern int	nv_nextsub(Namval_t*);
extern char	*nv_getsub(Namval_t*);
extern Namval_t	*nv_putsub(Namval_t*, char*, long);
extern Namval_t	*nv_opensub(Namval_t*);

/* name-value pair function prototypes */
extern void 	nv_close(Namval_t*);
extern Namval_t *nv_create(Namval_t*,const char*,Namfun_t*);
extern double 	nv_getn(Namval_t*, Namfun_t*);
extern double 	nv_getnum(Namval_t*);
extern char 	*nv_getv(Namval_t*, Namfun_t*);
extern char 	*nv_getval(Namval_t*);
extern void 	nv_newattr(Namval_t*,unsigned,int);
extern Namval_t	*nv_open(const char*,Hashtab_t*,int);
extern void 	nv_putval(Namval_t*,const char*,int);
extern void 	nv_putv(Namval_t*,const char*,int,Namfun_t*);
extern int	nv_scan(Hashtab_t*,void(*)(Namval_t*),int,int);
extern Namval_t	*nv_scoped(Namval_t*);
extern char 	*nv_setdisc(Namval_t*,const char*,Namval_t*,Namfun_t*);
extern void	nv_setref(Namval_t*);
extern void 	nv_setvec(Namval_t*,int,char*[]);
extern void	nv_setvtree(Namval_t*);
extern Namfun_t *nv_stack(Namval_t*,Namfun_t*);
extern void 	nv_unset(Namval_t*);

#if 0
/*
 * The names of many functions were changed in early '95
 * Here is a mapping to the old names
 */
#   define nv_istype(np)	nv_isattr(np)
#   define nv_newtype(np)	nv_newattr(np)
#   define nv_namset(np,a,b)	nv_open(np,a,b)
#   define nv_free(np)		nv_unset(np)
#   define nv_settype(np,a,b,c)	nv_setdisc(np,a,b,c)
#   define nv_search(np,a,b)	nv_open(np,a,((b)?0:NV_NOADD))
#   define settype	setdisc
#endif

#endif /* NV_DEFAULT */
