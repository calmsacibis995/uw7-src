#ident	"@(#)ksh93:src/cmd/ksh93/include/name.h	1.2"
#pragma prototyped
#ifndef _NV_PRIVATE
/*
 * This is the implementation header file for name-value pairs
 */

#define _NV_PRIVATE	\
	union Value	nvalue; 	/* value field */ \
	char		*nvenv;		/* pointer to environment name */ \
	Namfun_t	*nvfun;		/* pointer to trap functions */

#include	<ast.h>
#include	"shtable.h"

/* Nodes can have all kinds of values */
union Value
{
	const char	*cp;
	int		*ip;
	char		c;
	int		i;
	unsigned	u;
	long		*lp;
	short		s;
	double		*dp;	/* for floating point arithmetic */
	struct Namarray	*array;	/* for array node */
	struct Namval	*np;	/* for Namval_t node */
	union Value	*up;	/* for indirect node */
	struct Ufunction *rp;	/* shell user defined functions */
	int (*bfp)(int,char*[],void*);/* builtin entry point function pointer */
};

#include	"nval.h"

/* used for arrays */
#define ARRAY_MASK	((1L<<ARRAY_BITS)-1)	/* For index values */

#define ARRAY_MAX 	4096	/* maximum number of elements in an array
				   This must be less than ARRAY_MASK */
#define ARRAY_INCR	16	/* number of elements to grow when array 
				   bound exceeded.  Must be a power of 2 */

/* These flags are used as options to array_get() */
#define ARRAY_ASSIGN	0
#define ARRAY_LOOKUP	1
#define ARRAY_DELETE	2


/* This describes a user shell function node */
struct Ufunction
{
	int	*ptree;			/* address of parse tree */
	int	lineno;			/* line number of function start */
	off_t	hoffset;		/* offset into history file */
};

/* attributes of Namval_t items */

/* The following attributes are for internal use */
#define NV_FUNCT	NV_IDENT	/* node value points to function */
#define NV_NOCHANGE	(NV_EXPORT|NV_IMPORT|NV_RDONLY|NV_TAGGED|NV_NOFREE)
#define NV_ATTRIBUTES	(~(NV_NOSCOPE|NV_ARRAY|NV_IDENT|NV_ASSIGN|NV_REF|NV_VARNAME))
#define NV_PARAM	(1<<12)		/* expansion use positional params */

/* This following are for use with nodes which are not name-values */
#define NV_FUNCTION	(NV_RJUST|NV_FUNCT)	/* value is shell function */
#define NV_FPOSIX	NV_LJUST		/* posix function semantics */

#define NV_NOPRINT	(NV_LTOU|NV_UTOL)	/* do not print */
#define NV_NOALIAS	(NV_NOPRINT|NV_IMPORT)
#define NV_NOEXPAND	NV_RJUST		/* do not expand alias */
#define NV_BLTIN	(NV_NOPRINT|NV_EXPORT)
#define BLT_ENV		(NV_RDONLY)		/* non-stoppable,
						 * can modify enviornment */
#define BLT_SPC		(NV_LJUST)		/* special built-ins */
#define BLT_EXIT	(NV_RJUST)		/* exit value can be > 255 */
#define BLT_DCL		(NV_TAGGED)		/* declaration command */
#define is_abuiltin(n)	(nv_isattr(n,NV_BLTIN)==NV_BLTIN)
#define is_afunction(n)	(nv_isattr(n,NV_FUNCTION)==NV_FUNCTION)
#define	nv_funtree(n)	((n)->nvalue.rp->ptree)
#define	funptr(n)	((n)->nvalue.bfp)


/* NAMNOD MACROS */
#define nv_link(root,n)		hashlook((Hashtab_t*)(root),(char*)(n),\
					HASH_INSTALL,(char*)sizeof(Namval_t))
/* ... for attributes */

#define nv_onattr(n,f)	((n)->nvflag |= (f))
#define nv_setattr(n,f)	((n)->nvflag = (f))
#define nv_offattr(n,f)	((n)->nvflag &= ~(f))
/* ... etc */

#define nv_setsize(n,s)	((n)->nvsize = ((s)&0xfff))

/* ...	for arrays */

#define nv_arrayptr(np)	(nv_isattr(np,NV_ARRAY)?(np)->nvalue.array:(Namarr_t*)0)
#define array_elem(ap)	((ap)->nelem&ARRAY_MASK)
#define array_assoc(ap)	((ap)->fun)

extern void		array_check(Namval_t*, int);
extern union Value	*array_find(Namval_t*, int);
extern char 		*nv_endsubscript(Namval_t*, char*, int);
extern Namfun_t 	*nv_cover(Namval_t*);
struct argnod;		/* struct not declared yet */
extern void		nv_setlist(struct argnod*, int);
extern void 		nv_scope(struct argnod*);

extern const char	e_subscript[];
extern const char	e_subscript_id[];
extern const char	e_nullset[];
extern const char	e_nullset_id[];
extern const char	e_notset[];
extern const char	e_notset_id[];
extern const char	e_notset2[];
extern const char	e_notset2_id[];
extern const char	e_noparent[];
extern const char	e_noparent_id[];
extern const char	e_readonly[];
extern const char	e_readonly_id[];
extern const char	e_badfield[];
extern const char	e_badfield_id[];
extern const char	e_restricted[];
extern const char	e_restricted_id[];
extern const char	e_ident[];
extern const char	e_ident_id[];
extern const char	e_varname[];
extern const char	e_varname_id[];
extern const char	e_funname[];
extern const char	e_funname_id[];
extern const char	e_intbase[];
extern const char	e_noalias[];
extern const char	e_noalias_id[];
extern const char	e_aliname[];
extern const char	e_aliname_id[];
extern const char	e_badexport[];
extern const char	e_badexport_id[];
extern const char	e_badref[];
extern const char	e_badref_id[];
extern const char	e_noref[];
extern const char	e_noref_id[];
extern const char	e_selfref[];
extern const char	e_selfref_id[];
extern const char	e_envmarker[];
extern const char	e_badlocale[];
extern const char	e_badlocale_id[];
extern const char	e_loop[];
extern const char	e_loop_id[];
#endif /* _NV_PRIVATE */
