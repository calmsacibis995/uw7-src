#ident	"@(#)acomp:common/cgstuff.c	55.4.13.22"
/* cgstuff.c */

/* This module contains routines that serve as mediators between
** the compiler front-end and the CG back-end.  p1allo.c contains
** CG-related routines that allocate storage offsets.
*/

#include "p1.h"
#include "lang.h"
#ifdef MERGED_CPP
#include "interface.h"		/* CPP interface */
#endif
#include "mfile2.h"
#include "arena.h"
#include "assert.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifdef	__STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

int r1debug;

Language_type language;

int function_has_eh_tables;

#define	t2alloc() ((ND2 *) talloc())
#define ND2NIL ((ND2 *) 0)

/* put this definition in mfile1.h eventually */
#define FF_ISVOL        020             /* flag to indicate volatile attribute */

#ifndef	IDENTSTR
#define	IDENTSTR ".ident"
#endif

#ifndef	MYLABFMT
#define	MYLABFMT	".X%d"
#endif

#define	MYLABTYPE	int
#ifndef LINT
static void cg_p2tree();

static int cg_lastlocctr = UNK;		/* last location counter used:
					** unknown initially
					*/
static int cg_savelc = UNK;		/* saved location counter (none) */
static char * cg_fstring = "";		/* current filename string */
static int cg_outhdr = 1;		/* need to output file header stuff
					** if non-0
					*/
#endif

#ifdef	FAT_ACOMP

static int cg_infunc = 0;		/* non-zero if within function */
static int cgq_tail_index = CGQ_NULL_INDEX;
#endif
static int cg_ldused = 0;		/* long double used somewhere */
extern int amigo_ran;

/* Force out a "touch" of a symbol that must be present in start-up
** code to support long double as double.
*/
#ifndef	CG_LDUSED
#define	CG_LDUSED() cg_copyasm("\t.globl\t__longdouble_used")
#endif

#ifdef	FAT_ACOMP
static void cg_q_gen();
static void cg_q_nd2();
static void cg_q_puts();
#endif
#ifndef LINT
static char * cg_statname();
static void cg_inflush();
static void cg_inaccum();
static void cg_indo();
static void cg_outbytes();
static void cg_copyasm();
static ND1 * cg_strout();
static void cg_pushlc();
static void cg_poplc();

/* void cg_p2compile(ND2 *p) */
#ifdef FAT_ACOMP

static int location = UNK;
#define CG_PUSHLC(lc) cg_q_int(cg_pushlc, lc)
#define CG_POPLC() cg_q_call(cg_poplc)
#define CG_SETLOCCTR(lc) if (cg_infunc) location = lc; else cg_setlocctr(lc);
#define	cg_p2compile(p) if (cg_outhdr) \
			    cg_begfile(); \
			cg_q_nd2(p);
#else	/* ! FAT_ACOMP */
#define CG_PUSHLC(lc) cg_pushlc(lc)
#define CG_POPLC() cg_poplc()
#define CG_SETLOCCTR(lc) cg_setlocctr(lc)
#define	cg_p2compile(p) if (cg_outhdr) \
			    cg_begfile(); \
			p2compile((NODE*) p);
#endif	/* def FAT_ACOMP */
#else
#define cg_p2compile(p) tfree((NODE*) p)
#define cg_p2tree(p)
#endif

/* Array used to maintain list of global symbols assigned to a register
   within a function.  Previously located in sym.c. */
#define MAX_REGS 32
static int sid_in_reg[MAX_REGS];
static int nsid_in_reg;

void
cg_bind_global_sym(sid)
/* Previously sy_bind_global() in sym.c. */
SX sid;
{
    if (nsid_in_reg == MAX_REGS)
	cerror("sy_bind_global: too many globals in registers");
    sid_in_reg[nsid_in_reg] = sid;
    nsid_in_reg++;
}

void
cg_unbind_globals()
{
    while (nsid_in_reg > 0) {
	SY(sid_in_reg[nsid_in_reg-1]).sy_regno = SY_NOREG;
	nsid_in_reg--;
    }
}



TWORD 
cg_machine_type(type1) 
T1WORD type1;
/* convert a pass1 type to a pass2 type, equivalent pass2 types are converted
   to the same type 
*/
{
	TWORD type = cg_tconv(type1,0);

#ifdef NOSHORT
	if ( type == TSHORT)
		type =  TINT;
	else if ( type == TUSHORT)
		type = TUNSIGNED;
#endif
			/*If this machine has no longs, change longs to ints*/
#ifdef NOLONG
	if ( type == TLONG)
		type =  TINT;
	else if ( type == TULONG)
		type = TUNSIGNED;
#endif
	return type;
}
TWORD
cg_tconv(t, ldflag)
T1WORD t;
int ldflag;
/* Convert a pass 1 type to a pass 2 type.  If ldflag is non-zero,
** complain about long doubles, as appropriate for the compiler
** version.  Otherwise, treat long double like double.
*/
{
    switch( TY_TYPE(t) ) {
    case TY_CHAR:
				if(chars_signed)
					return( T2_CHAR );
				else
					return( T2_UCHAR );
    case TY_SCHAR:			return( T2_CHAR );

    case TY_UCHAR:			return( T2_UCHAR );

    case TY_SHORT:	case TY_SSHORT:	return( T2_SHORT );
    case TY_USHORT:			return( T2_USHORT );

    case TY_INT:	case TY_SINT:	return( T2_INT );
    case TY_UINT:			return( T2_UNSIGNED );

    case TY_LONG:	case TY_SLONG:	return( T2_LONG );
    case TY_ULONG:			return( T2_ULONG );

    case TY_LLONG:	case TY_SLLONG:	return( T2_LLONG );
    case TY_ULLONG:			return( T2_ULLONG );

    case TY_FLOAT:			return( T2_FLOAT );
    case TY_DOUBLE:			return( T2_DOUBLE );
    case TY_LDOUBLE:
#ifndef NO_LDOUBLE_SUPPORT
					return( T2_LDOUBLE );
#else
		/* Do check for error message, then treat as double. */
					if (ldflag)
					    cg_ldcheck();
					return( T2_DOUBLE );
#endif

    case TY_STRUCT:	case TY_UNION:	return( T2_STRUCT );

    case TY_VOID:			return( T2_VOID );
    case TY_ENUM:
    {
#if	0
	BITOFF tsize = TY_SIZE(t);
	if      (tsize == TY_SIZE(TY_INT))	return( T2_INT );
	/* small enums not supported */
	else if (tsize == TY_SIZE(TY_SHORT))	return( T2_SHORT );
	else if (tsize == TY_SIZE(TY_CHAR))	return( T2_CHAR );
	return( T2_INT );			/* to keep us going */
	/*NOTREACHED*/
#else
	return( T2_INT );
	/* We used to check for TY_SIZE(TY_INT)), then if not:
	cerror(gettxt(":121","unknown size"));
	*/
#endif
    }

    case TY_PTR:
	return ( T2_ADDPTR(cg_tconv(TY_DECREF(t), ldflag)) );
    case TY_FUN:
	return ( T2_ADDFTN(cg_tconv(TY_DECREF(t), ldflag)) );
    case TY_ARY:
	return ( T2_ADDARY(cg_tconv(TY_DECREF(t), ldflag)) );

    default:
	cerror(gettxt(":122","confused type %d in cg_tconv()"), t);
    }
    /*NOTREACHED*/
}

#ifndef NODBG
int print_cgq_flag;
#endif

#ifndef LINT
/* lint doesn't need below routines - replaced with stubs */

static 
cg_align(t)
T1WORD t;
/* Produce CG alignment type for pass 1 type t. */
{
    TWORD altype;

    for (;;) {					/* for array case */
	switch( TY_TYPE(t) ) {
	    int align;
	case TY_STRUCT:
	case TY_UNION:
	    /* Choose some other type that has the alignment of the s/u,
	    ** because that information is missing from TSTRUCT.
	    */
	    align = TY_ALIGN(t);
	    if (align < ALSTRUCT)
		altype = ALSTRUCT;
	    else {				/* only bother if necessary */
		if (align == ALINT)
		    altype = T2_INT;
		else if (align == ALSHORT)
		    altype = T2_SHORT;
#ifndef NO_LDOUBLE_SUPPORT
		else if (align == ALLDOUBLE)
		    altype = T2_LDOUBLE;
#endif
		else if (align == ALDOUBLE)
		    altype = T2_DOUBLE;
		else if (align == ALFLOAT)
		    altype = T2_FLOAT;
		else if (align == ALPOINT)
		    altype = T2_ADDPTR(T2_VOID); /* arbitrary pointer type */
		else
		    altype = T2_CHAR;
	    }
	    break;
	case TY_ARY:
	    t = TY_DECREF(t);
	    continue;
	case TY_PTR:
	    altype = T2_ADDPTR(T2_VOID); break;
	/* align functions like ints */
#ifndef T2_FUNC
#define T2_FUNC T2_INT
#endif
	case TY_FUN:				altype = T2_FUNC; break;
	default:				altype = cg_tconv(t,0); break;
	} /* end switch */
	break;					/* out of loop */
    } /* end for */
    return( altype );
}

void
cg_setlocctr(lc)
int lc;
/* Generate code to set location counter lc.  If there's no
** change, just exit.
*/
{
    ND2 * new;

    if (lc == cg_lastlocctr || lc == UNK) return;

    cg_inaccum(&num_0, 0); /* flush any outstanding in current locctr */

    new = t2alloc();
    new->op = LOCCTR;
    new->c.label = lc;
    cg_p2compile(new);

    /* it is important that cg_lastlocctr is updated after the p2compile
       since p2compile may call cg_setlocctr before outputting the new
       section. 
    */
    cg_lastlocctr = lc;
}

#ifdef DBLINE
void
cg_mk_lineno() {
    ND2 *new = t2alloc();
    new->op = DBLINE;
    new->c.size = db_curline;
    cg_q_nd2(new);
    return;
}

void
cg_lineno(line)
int line;
{
	db_curline = line;
	db_lineno();
}
#endif


static void
cg_pushlc(lc)
int lc;
/* Push current location counter on (1-level) stack,
** set new location counter, lc.
*/
{
    if (cg_savelc != UNK)
	cerror(gettxt(":123","cg_pushlc:  lc already saved"));
    cg_savelc = cg_lastlocctr;
    cg_setlocctr(lc);
    return;
}

static void
cg_poplc()
/* Restore saved (on 1-level stack) location counter. */
{
    if (cg_savelc == UNK)
	cerror(gettxt(":124","cg_poplc:  no saved lc"));
    cg_setlocctr(cg_savelc);
    cg_savelc = UNK;
    return;
}


void
cg_defnam(sid)
SX sid;
/* Output symbol definition stuff for static/extern's first
** tentative definition, or for actual definition.
** The location counter must be set appropriately for the symbol.
** Only produce output for these cases:
**	1) Function definition.
**	2) New tentative definition.
**	3) Defining instance (has initializer).
*/
{
    ND2 * new;
    T1WORD t;
    BITOFF sz = 0;			/* size of COMMON */
    TWORD altype;			/* alignment type for CG */
    int class = SY_CLASS(sid);		/* symbol class */
    SY_FLAGS_t hasinit = (SY_FLAGS_t) (SY_FLAGS(sid) & SY_DEFINED);
    int isfunc = 0;
    
    /* Determine the proper alignment type for CG. */
    t = SY_TYPE(sid);
    if (TY_TYPE(t) == TY_FUN) {
	if (!hasinit) return;		/* function decl is unimportant */
	isfunc = 1;
    }
    altype = cg_align(t);

    if (!hasinit) sz = TY_SIZE(t);	/* size for common */

    new = t2alloc();

    new->op = DEFNAM;
    new->name = cg_extname(sid);
    new->type = altype;
    new->sid = 0;
    new->c.size = sz;
    if (hasinit) {
	new->sid |= DEFINE;
	if (class == SC_EXTERN)
	    new->sid |= EXTNAM;
    }
    else
	new->sid |= (class == SC_EXTERN ? COMMON : LCOMMON);
    if (isfunc)
	new->sid |= FUNCT;

    cg_p2compile(new);
    return;
}

#define CG_NAME_SIZE 100

char *
cg_escape_leading_dollar(char * name_in)
/*
** Check the first character of the external name.  If a '$', prefix
** the name with a '\'.  The new name is built in a static char array
** that will be reused on subsequent calls.  Any name produced/modified
** by this routine is meant for immediate output.
*/
{
    static char cg_name_buff[CG_NAME_SIZE];
    static char *cg_name_ptr = cg_name_buff;
    static int cg_buff_size = CG_NAME_SIZE;
    static int is_buff_malloced = 0;

    int len, growth;

    if (*name_in == '$') {
	if ((len = strlen(name_in)+2) > cg_buff_size) {
	    /* Need to increase the buffer size. */
	    if (is_buff_malloced) {
		free (cg_name_ptr);
	    }
	    growth = ((cg_buff_size >> 2) + cg_buff_size);
	    is_buff_malloced = 1;
	    cg_name_ptr = malloc((size_t) growth >= len ? growth : len);
	    if (cg_name_ptr == 0) {
		cerror("cg_escape_leading_dollar: Cannot increase name buffer !!!!!!");
	    }
	}
	*cg_name_ptr = '\\';
	strcpy(&cg_name_ptr[1], name_in);
	return st_nlookup(cg_name_ptr, len);
    } else {
	return name_in;
    }
}


char *
cg_extname(sid)
SX sid;
/* Return the character string that corresponds to the (presumed)
** external representation of a symbol's name.  Functions always
** use the symbol's name.  Take into account the alternate naming
** for block statics.
**
** Also allow for "unnamed" statics introduce at the file scope
** by the C++ frontend.
*/
{
    switch (SY_CLASS(sid)) {
    case SC_STATIC:
	if (   (SY_LEVEL(sid) != SL_EXTERN && !TY_ISFTN(SY_TYPE(sid)))
	    || ! SY_NAME(sid) )
	    return cg_statname((int)SY_OFFSET(sid));
	/*FALLTHRU*/
    case SC_EXTERN:
    case SC_ASM:
	return cg_escape_leading_dollar(SY_NAME(sid));
    }
    return (char *)0;		/* has no name */
}


void
cg_nameinfo(sid)
SX sid;
{
    SY_CLASS_t class = SY_CLASS(sid);

    /* Only produce information for function definitions and external
    ** or static objects.
    */
    switch (class) {
	ND2 *nameinfo;

    case SC_STATIC:
    case SC_EXTERN:
	nameinfo = t2alloc();

	nameinfo->op = NAMEINFO;
	nameinfo->type = T2_VOID;
	nameinfo->name = cg_extname(sid);
	if (TY_ISFTN(SY_TYPE(sid))) {
	    CG_SETLOCCTR(PROG);
	    nameinfo->c.size = 0;
	    nameinfo->sid = NI_FUNCT;
	}
	else {
	    nameinfo->c.size = TY_SIZE(SY_TYPE(sid));
	    /* Select appropriate symbol kind. */
	    if (class == SC_EXTERN)
		nameinfo->sid = NI_GLOBAL;
	    else if (SY_LEVEL(sid) == SL_EXTERN)
		nameinfo->sid = NI_FLSTAT;
	    else
		nameinfo->sid = NI_BKSTAT;
	    nameinfo->sid |= NI_OBJCT;
	}
	cg_p2compile(nameinfo);

	if (language == Cplusplus_language && function_has_eh_tables)
		/* put out symbol for function length, for C++ EH purposes */
		if (TY_ISFTN(SY_TYPE(sid)))
			fprintf(outfile, "\t.set\t..EH.length.%s,.-%s\n", SY_NAME(sid), SY_NAME(sid));
    }
    return;
}


/* These routines interface to CG's initialization handling.
** Because CG doesn't support bitfield initialization, we
** must accumulate field words ourselves.  All simple integer
** initializers are accumulated in an INTCON which is flushed
** when full or at the end.
*/

typedef struct t_cg_in cg_in_t;
struct t_cg_in {
	cg_in_t	*next;
	ND1	*node;
	BITOFF	len;
	char	strinit;
};
static cg_in_t	*cg_in_head;		/* front of the current init. seq. */
static cg_in_t	*cg_in_tail;		/* most recent item in list */
static int	cg_in_locctr;		/* location for init. data */
static int	cg_in_build;		/* set while adding to list */
static BITOFF	cg_in_offset;		/* distance so far into object */
static BITOFF	cg_in_avail = SZLLONG;	/* no. bits still available */
static TWORD	cg_in_align;		/* base alignment if no label */

static void
cg_in_add(ND1 *n, BITOFF l, int s)
/* Append item to current init. seq. list. */
{
	cg_in_t *init = malloc(sizeof(cg_in_t));

	if (!init)
		cerror("No space for initializer");
	init->next = 0;
	init->node = n;
	init->len = l;
	init->strinit = s;
	if (cg_in_head == 0) /* first item */
		cg_in_head = init;
	else 
		cg_in_tail->next = init;
	cg_in_tail = init;
}

static void
cg_in_walk(void)
/* Run through the list now that it's finished. */
{
	cg_in_t *p, *n;
	ND1 *node;

	for (p = cg_in_head; p != 0; p = n) {
		n = p->next;
		if ((node = p->node) == 0)
			cg_zecode(p->len);
		else if (p->strinit)
			(void)cg_strinit(node, p->len, 0, 0);
		else if (node->op != ICON || node->sid != ND_NOSYMBOL)
			cg_indo(node, p->len);
		else {
			cg_inaccum(&node->c.ival, p->len);
			nfree(node);
		}
		free((void *)p);
	}
	cg_in_head = 0;
}

static int
cg_inloc(T1WORD t, int mkro)
/* Return the proper location counter for the data type.
** The type dictates whether the data go in read-only or
** read/write storage.  Non-volatile const stuff goes into
** read-only storage.  If mkro == C_READONLY, force stuff
** into read-only section.
*/
{
	int ret = CDATA;

	if (mkro != C_READONLY) {
		while (TY_ISARY(t))
			t = TY_DECREF(t); /* get to non-array type */
		if (!TY_ISCONST(t) || TY_ISMBRVOLATILE(t))
			ret = DATA;
	}
	return ret;
}

void
cg_insetloc(T1WORD t, int mkro)
/* Like cg_inloc(), except here we actually set the location counter */
{
	CG_SETLOCCTR(cg_inloc(t, mkro));
}

void
cg_instart(T1WORD t, int mkro)
/* Start an init. seq. of an object with type t. */
{
	cg_in_t *next;

	if (cg_in_head != 0) { /* shouldn't happen */
		do {
			next = cg_in_head->next;
			free((void *)cg_in_head);
		} while ((cg_in_head = next) != 0);
	}
	cg_in_locctr = cg_inloc(t, mkro);
	cg_in_tail = 0;
	cg_in_build = 1;
	cg_in_offset = 0;
	cg_in_avail = SZLLONG;
	cg_in_align = cg_align(t);
}

void
cg_incode(ND1 *p, BITOFF len)
/* Add next nonzero value of an init. seq. */
{
	extern int picflag;

	if (p->op == ICON && p->sid != ND_NOSYMBOL || p->op == STRING) {
		/*
		* For address-ish ICONs and STRINGs, CG cannot initialize
		* targets smaller than the one-size-fits-all pointer.
		* Also, position independent code also restricts these
		* initializers from occurring in readonly space.
		*/
		if (len < SZPOINT)
			UERROR(gettxt(":125", "invalid initializer"));
		else if (picflag)
			cg_in_locctr = DATA;
	}
	cg_in_add(p, len, 0);
}

static void
cg_inaccum(INTCON *vp, BITOFF len)
/* Add another len<=SZLLONG bits to accumulated initializer value.
** If len==0, flush any remaining.  Note that *vp is modified, but
** in such a way that it has no affect on a zero (like num_0).
*/
{
	static INTCON accum;
	ND2 *init, *icon;
	INTCON spill;

	if (len == 0) { /* forced flush */
		if (cg_in_avail == SZLLONG) /* nothing's accumulated */
			return;
	} else {
		cg_in_offset += len;
		(void)num_unarrow(vp, len);
		if (cg_in_avail == SZLLONG) {
			accum = *vp;
			if ((cg_in_avail -= len) != 0)
				return;
			len = 0;
		} else if (len <= cg_in_avail) {
#ifdef RTOLBYTES
			(void)num_llshift(vp, SZLLONG - cg_in_avail);
#else
			(void)num_llshift(&accum, len);
#endif
			num_or(&accum, vp);
			if ((cg_in_avail -= len) != 0)
				return;
			len = 0;
		} else {
			len -= cg_in_avail;
			spill = *vp;
#ifdef RTOLBYTES
			(void)num_lrshift(&spill, cg_in_avail);
			(void)num_llshift(vp, SZLLONG - cg_in_avail);
#else
			(void)num_unarrow(&spill, len);
			(void)num_lrshift(vp, len);
			(void)num_llshift(&accum, cg_in_avail);
#endif
			num_or(&accum, vp);
			cg_in_avail = 0;
		}
	}
	cg_in_avail = SZLLONG - cg_in_avail;
	/*
	* Only here when need to flush cg_in_avail bits of accum.
	*/
	init = t2alloc();
	init->type = T2_CHAR;
	init->c.size = BITOOR(cg_in_avail);
	if (num_ucompare(&accum, &num_0) == 0) {
		init->op = UNINIT;
	} else {
		init->op = INIT;
		init->left = icon = t2alloc();
		icon->op = ICON;
		icon->type = init->type;
		icon->c.ival = accum;
		icon->name = (char *)0;
	}
	cg_p2compile(init);
	if ((cg_in_avail = SZLLONG - len) != SZLLONG)
		accum = spill;
}

void
cg_zecode(BITOFF len)
/* Add zero bits next in an init. seq. */
{
	BITOFF nb;
	ND2 *p;

	if (len == 0)
		return;
	if (cg_in_build) {
		cg_in_add(ND1NIL, len, 0);
		return;
	}
	/*
	* First fill in the remainder of any current INTCON value.
	*/
	if ((nb = cg_in_avail) != SZLLONG) {
		if (len <= nb) {
			cg_inaccum(&num_0, len);
			return;
		}
		cg_inaccum(&num_0, nb);
		len -= nb;
	}
	cg_in_offset += len;
	/*
	* Now at a fresh byte boundary with no current INTCON value.
	* Emit as many whole bytes of zero now.
	*/
	if ((nb = BITOOR(len)) != 0) {
		len -= ORTOBI(nb);
		p = t2alloc();
		p->op = UNINIT;
		p->type = T2_CHAR;
		p->c.size = nb;
		cg_p2compile(p);
	}
	if (len != 0) {
		cg_in_offset -= len;
		cg_inaccum(&num_0, len);
	}
}

static void
cg_indo(ND1 *p1, BITOFF len)
/* Add next value that isn't a simple unnamed ICON of an init. seq. */
{
	T1WORD t1 = p1->type;
	TWORD t2;
	ND2 *p2;

	/* Get an appropriate CG type. */
	if (TY_ISFPTYPE(t1))
		t2 = cg_tconv(t1, 1);
	else if (len == TY_SIZE(TY_INT))
		t2 = T2_INT;
	else if (len == TY_SIZE(TY_LLONG))
		t2 = T2_LLONG;
#ifndef NOLONG
	else if (len == TY_SIZE(TY_LONG))
		t2 = T2_LONG;
#endif
#ifndef NOSHORT
	else if (len == TY_SIZE(TY_SHORT))
		t2 = T2_SHORT;
#endif
	else if (len == TY_SIZE(TY_CHAR))
		t2 = T2_CHAR;
	else { /* don't know what CG type to use--just throw in some zeroes! */
		cg_inaccum(&num_0, len);
		nfree(p1);
		return;
	}
	cg_inaccum(&num_0, 0); /* flush any simple integer value */
#ifndef NO_AMIGO
	/* Must do before cg_p2tree() changes *p1 */
	if (p1->op == ICON && p1->sid > 0 && TY_ISFTN(SY_TYPE(p1->sid)))
		inline_address_function(p1->sid);
#endif
	cg_p2tree((ND2 *)p1);
	((ND2 *)p1)->type = t2;
	p2 = t2alloc();
	p2->op = INIT;
	p2->left = (ND2 *)p1;
	p2->type = t2;
	p2->c.size = BITOOR(len);
	cg_p2compile(p2);
	cg_in_offset += len;
#ifdef NO_LDOUBLE_SUPPORT
	if (TY_ISFPTYPE(t1)) {
		if (TY_TYPE(t1) == TY_LDOUBLE && t2 == T2_DOUBLE) {
			/*
			* Under -Xc, we permit long double, but really
			* generate double values, at least make sure
			* that enough space is reserved.
			*/
			cg_zecode(TY_SIZE(TY_LDOUBLE) - TY_SIZE(TY_DOUBLE));
		}
	}
#endif
}

void
cg_inend(ND1 *p, SX sid)
/* Finish an init. seq. */
{
	char *name;

	cg_in_build = 0;
	if (sid != SY_NOSYM
	    && (name = SY_NAME(sid)) != 0
	    && strncmp(name, ".eh.", 4) == 0)
	{
		/* Special C++ EH compiler-generated variables go in
		** their own section.  Since these names are NOT in
		** the string table, full strcmp()s must be used.
		*/
		if (strcmp(name, ".eh.region.table.var") == 0
			|| strcmp(name, ".eh.spec.array.var") == 0
			|| strcmp(name, ".eh.catch.array.var") == 0)
		{
			cg_in_locctr = EH_RELOC;
		}
		else if (strcmp(name, ".eh.array.table.var") == 0)
			cg_in_locctr = EH_OTHER;
	}
	CG_SETLOCCTR(cg_in_locctr);
	if (p) {
		cg_defstat(p);
	} else if (sid != SY_NOSYM) {
		cg_defnam(sid);
	} else {
		ND2 *align = t2alloc();

		align->op = ALIGN;
		align->type = cg_in_align;
		cg_p2compile(align);
	}
	cg_in_walk();
	cg_inaccum(&num_0, 0); /* flush any remaining data */
}

typedef struct t_cg_rostr cg_rostr_t;
struct t_cg_rostr {
	cg_rostr_t	*next;
	const char	*str;
	BITOFF		len;
	T1WORD		type;
	int		label;
};
static cg_rostr_t *cg_rostr_head; /* kept as self-organizing list */

static void
cg_rostr_keep(ND1 *def, ND1 *p, BITOFF len)
/* Remember readonly string literal info for possible reuse. */
{
	cg_rostr_t *sp;

	if ((sp = malloc(sizeof(cg_rostr_t))) == 0)
		return;
	sp->str = p->string;
	sp->len = len;
	sp->type = def->type;
	sp->label = def->sid;
	/* Insert new item at head of list. */
	sp->next = cg_rostr_head;
	cg_rostr_head = sp;
}

static ND1 *
cg_rostr_same(ND1 *p, BITOFF len)
/* Look for a match of (p->string,len) in the defined string literals. */
{
	cg_rostr_t *sp, **pp;
	ND1 *new;

	for (pp = &cg_rostr_head; (sp = *pp) != 0; pp = &sp->next) {
		if (p->string != sp->str)
			continue;
		if (len > sp->len) /* same string, but too many elements */
			continue;
		/* Move this entry to the front of the list. */
		*pp = sp->next;
		sp->next = cg_rostr_head;
		cg_rostr_head = sp;
		/* Create a fresh static name node for existing label. */
		new = tr_newnode(NAME);
		new->type = sp->type;
		new->c.off = 0;
		new->sid = sp->label; /* negative value was recorded */
		return new;
	}
	return ND1NIL;
}

int const_strings = 0;	/* Set to non-zero by -1c */

ND1 *
cg_strinit(ND1 *p, BITOFF len, int wantname, int mkro)
/* Build static string from STRING node with length "len".  If "len"
** is 0, use the whole string, including trailing null.  If wantname
** is non-0, put out a label first, and return a NAME node that
** corresponds to the initializer.  Generate the string in read-only
** space if mkro is C_READONLY or if const_strings is non-zero.
** If the sid field of the STRING node has the TR_IS_WIDE bit set,
** the string contains wide characters (still as multi-byte string).
** len is then the number of multibyte characters.
*/
{
	int iswide = p->sid & TR_ST_WIDE;
	ND1 *node = ND1NIL;
	BITOFF slen;

	if (cg_in_build) {
		if (wantname)
			cerror("cg_strinit: Want name in middle of initializer");
		cg_in_add(p, len, 1);
		return ND1NIL;
	}
	if ((slen = len) == 0)
		slen = p->c.size + 1; /* include terminator */
	if (wantname) {
		if (mkro != C_READONLY && !const_strings)
			mkro = ISTRNG;
		else if ((node = cg_rostr_same(p, slen)) != ND1NIL)
			return node;
		else
			mkro = CSTRNG;
		CG_PUSHLC(mkro);
		/* Put named strings on int boundary for speed. */
		node = cg_getstat((T1WORD)(iswide ? T_wchar_t : TY_INT), 1);
		cg_defstat(node);
		node->type = ty_mkaryof((T1WORD)(iswide ? T_wchar_t : TY_CHAR),
				len != 0 ? len : TY_NULLDIM);
		if (mkro == CSTRNG)
			cg_rostr_keep(node, p, slen);
	}
	if (iswide) {
		char *sp = p->string;
		BITOFF wlen = slen;
		INTCON wc;

		do {
			(void)num_fromulong(&wc, (unsigned long)lx_mbtowc(sp));
			cg_inaccum(&wc, TY_SIZE(T_wchar_t));
			sp += sizeof(wchar_t);
		} while (--wlen != 0);
	} else {
		ND2 *p2;

		cg_inaccum(&num_0, 0); /* flush remaining */
		p2 = t2alloc();
		p2->op = SINIT;
		p2->type = T2_CHAR;
		p2->c.size = slen;
		p2->name = p->string;
		cg_p2compile(p2);
		if (!wantname)
			cg_in_offset += slen * TY_SIZE(TY_CHAR);
	}
	nfree(p);
	if (wantname)
		CG_POPLC();
	return node;
}

static ND1 *
cg_strout(p)
ND1 * p;
{
    /* Turn STRING into an ICON or NAME node.
    ** If type is ptr to const, put in const data section.
    ** Type might not be pointer/array if cast got pushed
    ** onto STRING node.
    */
    T1WORD t = p->type;
    int newop = (p->sid & TR_ST_ICON) ? ICON : NAME;

    if ( (TY_ISPTR(t) || TY_ISARY(t)) && TY_ISCONST(TY_DECREF(t)) )
        p = cg_strinit(p, 0, 1, C_READONLY);
    else
        p = cg_strinit(p, 0, 1, C_READWRITE);
    p->op = newop;
    return (p);
}

void
cg_treeok()
/* Tell the tree allocator that we're not caught in an
** infinite loop, to forestall messages about "out of
** tree space".
*/
{
    extern int watchdog;
    watchdog = 0;			/* reset watchdog check */
    return;
}

static issue_access_warning;

static ND1 *
remove_star_of_void(p)
ND1 *p;
{
	switch(optype(p->op)) {
	case BITYPE:
		p->right = remove_star_of_void(p->right);
		/* FALLTHRU */
	case UTYPE:
		p->left = remove_star_of_void(p->left);
	}

	if (p->op == STAR && TY_TYPE(p->type) == TY_VOID) {
		issue_access_warning = 1;
		nfree(p);
		return p->left; /* just avoid the access */
	}
	else
		return p;
}


void
cg_ecode( p )
ND1 *p;
{
    extern int watchdog;

#ifndef NODBG
    if (b1debug > 1)
	tr_eprint(p);			/* print tree before conversion */
#endif

    /* standard version of writing the tree nodes */
    if( nerrors ) {
	t1free(p);
	return;
    }

    if (p) {
	/* Suppress code with an explicit access to a void value
	** via STAR.  CG has problem with access through volatile
	** void *.  Similarly suppress evaluation of struct ( normally
	** these disappear except for volatiles ).
	*/
	if (p->op == NAME && TY_ISSU(p->type)) {
		t1free(p);
		return;
	}
	issue_access_warning = 0;
	p = remove_star_of_void(p);
	if(issue_access_warning)
		WERROR(gettxt(":127","access through \"void\" pointer ignored"));
	CG_SETLOCCTR(PROG);
	cg_p2tree( (ND2 *) p );
	cg_p2compile( (ND2 *) p );

	al_e_expr();
    }
    watchdog = 0;			/* Persuade CG that node allocation is
					** still sane.
					*/
    return;
}


# ifndef RNODNAME
# define RNODNAME MYLABFMT
# endif

static void
cg_p2tree(p)
ND2 *p;
{
    register ty;
    register o;
    T1WORD otype = p->type;		/* original Pass 1 type */
    T1WORD ftype;			/* type of left side of FLD node */
    int flags = ((ND1 *)p)->flags;	/* flags of pass 1 node */
#ifdef VA_ARG
    ND2 * va_arg_node;			/* ICON node: 2nd arg of special func */
    T1WORD va_arg_type;			/* ICON's Pass 1 type (pointed-at) */
#endif

# ifdef MYP2TREE
    MYP2TREE(p);  /* local action can be taken here; then return... */
# endif

    /* this routine sits painfully between pass1 and pass2 */
    ty = optype(o = p->op);
    p->goal = 0;		/* an illegal goal, just to clear it out */
    p->type = cg_tconv(otype, 1);	/* type gets second-pass (bits) */

    /* strategy field marks a volatile operand */
    if ((flags & FF_ISVOL) != 0  && p->op != ICON)
	p->strat |= VOLATILE;

    switch (o)
    {
    case EQ: case NE:
    case LE: case NLE: case ULE: case UNLE:
    case LT: case NLT: case ULT: case UNLT:
    case GE: case NGE: case UGE: case UNGE:
    case GT: case NGT: case UGT: case UNGT:
    case LG: case NLG: case ULG: case UNLG:
    case LGE: case NLGE: case ULGE: case UNLGE:
	if (TY_ISFPTYPE(((ND1 *)p)->left->type)) {
	    if (flags & FF_NOEXCEPT)
		p->strat |= EXIGNORE;
	    else
		p->strat |= EXHONOR;
	}
	goto process_default;
    case CSE:
	p->id = ((ND1 *)p)->c.size;
	break;
    case LET:
	p->id = ((ND1 *)p)->c.size;
	if (flags & FF_COPYOK)
	    p->strat |= COPYOK;
	break;
    case PLUS:
    {
	ND2 *temp;
	TWORD ntype = p->type;     /* save pass 2 type */
	OFFSET off;

	if (p->left->op != STRING || p->right->op != ICON)
	    goto process_default;
	if (num_toslong(&p->right->c.ival, &off) != 0)
	    goto process_default;
	/* Special processing for inits like &"string"[3].
	** Turn it into a NAME w/offset.
	*/
	nfree(p->right);
	temp = (ND2 *)cg_strout((ND1 *)p->left);
	nfree(p->left);
	*p = *temp;
	nfree(temp);
	p->type = ntype;
	p->c.off = off;
	ty = optype(p->op);
	goto process_icon;
    }
    case STRING:
    {
	ND2 *temp;
	TWORD ntype = p->type;     /* save pass 2 type */

	p->type = otype;            /* reassign old type (pass 1) */
	temp = (ND2 *)cg_strout((ND1 *)p);
	*p = *temp;
	nfree(temp);                /* discard temp now */
	p->type = ntype;            /* restore pass2 type */
    }
    /*FALLTHRU:  p is now NAME*/
    case ICON:
    case NAME:
    process_icon:
	if (p->sid == ND_NOSYMBOL)
	    p->name = (char *)0;
	else if (p->sid > 0)
	{
	    SX sid = p->sid;	/* embedded symbol */
	    int regno;

	    p->name = cg_extname(sid);
	    /* Convert op, offset, etc., based on storage class. */
	    switch (SY_CLASS(sid)) {
	    case SC_AUTO:
		p->op = VAUTO;
		if ((regno = SY_REGNO(sid)) != SY_NOREG) {
		    p->op = REG;
		    p->sid = regno;
		}
		else {
#ifdef ADDRTAKEN
		    if (SY_FLAGS(sid) & SY_ADDRTAKEN) {
			p->strat |= ADDRTAKEN;
			if (version == V_CI4_1)
			    p->strat |= VOLATILE;
		    }
#endif
		    p->sid = ND_NOSYMBOL;
		    p->c.off += SY_OFFSET(sid);
		}
		break;
	    case SC_PARAM:
		p->op = VPARAM;
		if ((regno = SY_REGNO(sid)) != SY_NOREG) {
		    p->op = REG;
		    p->sid = regno;
		}
		else {
#ifdef ADDRTAKEN
		    if (SY_FLAGS(sid) & SY_ADDRTAKEN ) {
			p->strat |= ADDRTAKEN;
			if (version == V_CI4_1)
			    p->strat |= VOLATILE;
		    }
#endif
		    p->sid = ND_NOSYMBOL;
		    p->c.off += SY_OFFSET(sid);
		}
		break;
	    case SC_STATIC:
		if ((regno = SY_REGNO(sid)) != SY_NOREG) {
		    p->op = REG;
		    p->sid = regno;
		}
		else if (SY_LEVEL(sid) == SL_EXTERN) {
		    p->sid = NI_FLSTAT;
		    if (version == V_CI4_1)
			p->strat |= VOLATILE;
		}
		else {
		    p->sid = NI_BKSTAT;
#ifdef ADDRTAKEN
		    if (SY_FLAGS(sid) & SY_ADDRTAKEN) {
			p->strat |= ADDRTAKEN;
			if (version == V_CI4_1)
			    p->strat |= VOLATILE;
		    }
#endif
		}
		break;
	    case SC_EXTERN:
		p->sid = NI_GLOBAL;
		if ((regno = SY_REGNO(sid)) != SY_NOREG) {
		    p->op = REG;
		    p->sid = regno;
		}
#ifdef ADDRTAKEN
		else if (version == V_CI4_1)
		    p->strat |= VOLATILE;
#endif
		break;
	    case SC_ASM:
	    case SC_MOS:
	    case SC_MOU:
		break;
	    default:
		cerror(gettxt(":128","strange storage class %d in cg_p2tree()"),SY_CLASS(sid));
		/*NOTREACHED*/
	    }
	}
	else {
	    /* p->sid is negative, is a label number already. */
	    p->name = cg_statname((int)-p->sid);
	    p->sid = NI_FLSTAT;
	}
	break;

    /* Find the structure size and alignment of structure ops. */
    case STARG:
    case STASG:
    case STCALL:
    case UNARY STCALL:
    {
	/* Front-end hides true s/u type in p->sttype.  Grab it
	** before it possibly gets clobbered by union-reference
	** of another field (name).
	*/
	T1WORD sttype = ((ND1 *)p)->sttype;

	if (!TY_ISPTR(otype) || !TY_ISSU(sttype))
	    cerror(gettxt(":129","confused cg_p2tree()"));
	/* CG can't figure arg. size unless type is T2_STRUCT. */
	if (o == STARG)
	    p->type = T2_STRUCT;
	else if (callop(o))
	    p->name = al_g_regs(p);
	p->stalign = (short)TY_ALIGN(sttype);
	p->stsize = TY_SIZE(sttype);
	break;
    }

    case CALL:
#ifdef VA_ARG
    {
	static char *va_arg_name;
	ND2 *argp;

	if (va_arg_name == 0)
	    va_arg_name = st_lookup(VA_ARG);
	
	va_arg_node = ND2NIL;

	/* Look for special name, remember type of second argument.
	** Later (below), doctor the second argument node.
	** Have to do this now, before child types have been changed.
	*/
	if (p->left->op == ICON
	    && p->left->sid != ND_NOSYMBOL
	    && SY_NAME(p->left->sid) == va_arg_name
	    && (argp = p->right)->op == CM
	    && ((argp = argp->right)->op == FUNARG ||
	    argp == REGARG)
	    && (argp = argp->left)->op == ICON
	    && TY_ISPTR(argp->type)
	) {
	    va_arg_node = argp;
	    va_arg_type = TY_DECREF(argp->type);
	}
    }
    /*FALLTHRU*/
#endif
    case UNARY CALL:
	/* Turn these into INCALL/UNARY INCALL if the class of the
	** left operand is SC_ASM.
	*/
#ifdef	IN_LINE
	if ((((ND1 *)(p->left))->flags & FF_BUILTIN) 
	    && as_intrinsic(SY_NAME(p->left->sid))) {
	    p->op = p->op == CALL ? INCALL : UNARY INCALL;
	    p->strat |= FULL_OPT;
	}
	else if (p->left->op == ICON && p->left->sid != ND_NOSYMBOL) {
	    SX sid = p->left->sid;

	    for ( ; SY_FLAGS(sid) & SY_TOMOVE; sid = SY_SAMEAS(sid)) {
		if (SY_SAMEAS(sid) == SY_NOSYM)
		    break;
	    }
	    if (SY_CLASS(sid) == SC_ASM) {
		p->op = p->op == CALL ? INCALL : UNARY INCALL;
		if (SY_FLAGS(sid) & SY_ASM_FULL_OPT)
		    p->strat |= FULL_OPT;
		else if (SY_FLAGS(sid) & SY_ASM_PARTIAL_OPT)
		    p->strat |= PART_OPT;
	    }
	}
#endif
	p->name = al_g_regs(p);
	break;

    case FLD:
	ftype = p->left->type;	/* remember type of left child */
	break;

    default:
    process_default:
	p->name = (char *)0;
    }

    if (ty != LTYPE)
	cg_p2tree(p->left);
    if (ty == BITYPE)
	cg_p2tree(p->right);

    /* Take care of random stuff or some special nodes. */
    switch (o) {
	ND2 *l;
	TWORD ntype;

#ifdef NO_LDOUBLE_SUPPORT
    case FCON:
	if (otype == TY_LDOUBLE)
	    p->c.fval = op_xtodp(p->c.fval);
	break;
#endif
    case UPLUS:
	/* Remove UPLUS node and mark child as parenthesized. */
	l = p->left;
	*p = *l;
	p->strat |= PAREN;
	nfree(l);
	break;
    case FLD:
	/* All fields must be explicitly signed or unsigned.  Which
	** way a "plain"-typed field behaves is implementation defined.
	** Select an explicit type for them.  The actual type of the
	** field (as opposed to the type it promotes to) is in the
	** node below FLD.  Now that its type has been converted, do
	** the appropriate thing for "natural" types.
	*/
	ntype = p->left->type;		/* Usually use current type. */

	switch (TY_TYPE(ftype)) {
#ifdef	SIGNEDFIELDS			/* signed by default */
	case TY_CHAR:	ntype = T2_CHAR; break;
	case TY_SHORT:	ntype = T2_SHORT; break;
	case TY_ENUM:			/* behaves like int */
	case TY_INT:	ntype = T2_INT; break;
	case TY_LONG:	ntype = T2_LONG; break;
	case TY_LLONG:	ntype = T2_LLONG; break;
#else					/* unsigned by default */
	case TY_CHAR:	ntype = T2_UCHAR; break;
	case TY_SHORT:	ntype = T2_USHORT; break;
	case TY_ENUM:			/* behaves like int */
	case TY_INT:	ntype = T2_UNSIGNED; break;
	case TY_LONG:	ntype = T2_ULONG; break;
	case TY_LLONG:	ntype = T2_ULLONG; break;
#endif
	}
	p->left->type = ntype;		/* Set type of addressing expr. to
					** reflect signed-ness correctly.
					*/
	break;
#ifdef VA_ARG
    case CALL:
	/* For varargs function, va_arg_node points to an ICON with
	** pointer type.  Change its type to the Pass 2 version of
	** the pointed-at type, save the size in lval and the
	** alignment in rval.
	*/
	if (va_arg_node) {
	    va_arg_node->type = cg_tconv(va_arg_type, 0);
	    va_arg_node->c.size = TY_SIZE(va_arg_type);
	    va_arg_node->sid = TY_ALIGN(va_arg_type);
	}
	break;
#endif
    }
    return;
}


static char *
cg_statname(o)
int o;
/* Create print name for static variable # o. */
{
    char buf[20];

    (void) sprintf(buf, MYLABFMT, (MYLABTYPE)o);
    return st_lookup(buf);	/* squirrel string away and return */
}


void
cg_begf(sid)
SX sid;
/* Do function prologue for function whose symbol table entry is sid. */
{
    ND2 * new = t2alloc();
    TWORD rettype = cg_tconv(TY_ERETTYPE(SY_TYPE(sid)), 1);
					/* converted CG type for function */

#ifdef	FAT_ACOMP
    cg_infunc = 1;
#else
    new->name = (char *) 0;		/* no register information */
#endif
#ifndef NO_AMIGO
/* forces cgq into malloc'ed storage */
/* freed in cg_endf */
   if (do_inline) {
	/* forces cgq into malloc'ed storage */
	/* freed in cg_endf */
	td_cgq.td_used = 0;
	td_cgq.td_flags = TD_MALLOC;
	td_cgq.td_allo = INI_CGQ_RESTART;	/* Reset starting allocation
						   size. */
	cgq_tail_index = CGQ_NULL_INDEX;	/* Reinitialize. */

	td_cgq.td_start = (myVOID *)malloc(td_cgq.td_allo*sizeof(cgq_t));
	if (!td_cgq.td_start) {
	    cerror(gettxt(":130","No space for cgq"));
	}  /* if */
	/* cgq is freed in cg_endf */
	inline_begf(sid);
   }
#endif
    DB_BEGF(sid);			/* generate debug code at func start */
    CG_SETLOCCTR(PROG);

    function_has_eh_tables = 0;

    new->op = BEGF;
    new->type = rettype;
    cg_p2compile(new);

#ifdef CGENDPROLOG
    cg_q_str(cgprolog_leng,cg_extname(sid));
#endif
    cg_defnam(sid);

    new = t2alloc();
    new->op = ENTRY;
    new->type = rettype;
    cg_p2compile(new);

    /* Function entry may establish new starting point for autos,
    ** and current upper bound.
    */
#ifdef FAT_ACOMP
    cg_q_call(al_s_fcode);
#else
    al_s_fcode();
#endif
    return;
}

#ifndef NO_AMIGO
Cgq_index
cg_inline_endf() {
    cg_infunc = 0;
    return cgq_tail_index;
}

void
cg_restore(index) 
Cgq_index index;
{
    cgq_tail_index = index;
    cg_infunc = 1;
}
#endif

static SX cg_curfunc;

void
cg_setcurfunc(sid)
SX sid;
{
	cg_curfunc = sid;
}

SX
cg_getcurfunc()
{
	return cg_curfunc;
}

ND1*
cg_tree_contains_op(ND1 *node, int op)
{
	for (;;) {
		ND1* nd;
		switch (optype(node->op)) {
		case BITYPE:
			if (nd = cg_tree_contains_op(node->right, op))
				return nd;
			/*FALLTHRU*/
		case UTYPE:
			node = node->left;
			continue;
		case LTYPE:
			return node->op == op ? node : 0;
		}
	}
}

static void rewrite_calls(ND1 *node);
static int call_count;
int max_call_count;

#define NOT_OK(op) ((op) != NAME && (op) != ICON && (op) != STRING \
|| max_call_count && call_count >= max_call_count)
	/* 
	** all args must be NAME CALL or ICON
	*/
typedef struct stack_elt {
	ND1 *node;
	struct stack_elt *next;
} *Arg_stack;

static Arg_stack
check_args(node,regset)
ND1 *node;
int regset; /* Set of arguments being passed in args */
{
	
	Arg_stack stack_item, argstack = 0;
	int retval = 0;

		/* Push args on a stack, so we can process in
		** left to right order.
		*/
	while(node->op == CM) {
		stack_item = (Arg_stack)malloc(sizeof(struct stack_elt));
		stack_item->node = node->right;
		stack_item->next = argstack;
		argstack = stack_item;
		node = node->left;
	}

	stack_item = (Arg_stack)malloc(sizeof(struct stack_elt));
	stack_item->node = node;
	stack_item->next = argstack;
	argstack = stack_item;
	
	while(stack_item) {
		if(stack_item->node->op != FUNARG) {
			retval = 0;
			break;
		}
		if(regset %2) {
			if(NOT_OK(stack_item->node->left->op)) {
				retval = 0;
				break;
			}
		}
		else {
			int temp;
			temp = TY_SIZE(stack_item->node->type)/TY_SIZE(TY_INT);
			while(temp > 1) {
				temp --;
				regset >>= 1;
				if(regset%2) {
					retval = 0;
					break;
				}
			}
		}

		stack_item = stack_item->next;
		regset >>= 1;
		if(regset == 0) {
			retval = 1; /* We think we saw all the reg args */
		}
	}

	if(retval)
		return argstack;
	else while(argstack) {
		stack_item = argstack;
		argstack = argstack->next;
		free((char *)stack_item);
	}

	return 0; 
}
#undef NOT_OK

static void
subst_funarg_op(Arg_stack argstack, int regset)
{
	while(argstack) {
		if(regset % 2) { /* This arg_no gets passed in reg */
			argstack->node->op = REGARG;
				/* rewrite_calls(argstack->node->left);*/
		}
		else {
			int temp;
			temp = TY_SIZE(argstack->node->type)/TY_SIZE(TY_INT);
			while(temp > 1) {
				temp --;
				regset >>= 1;
				if(regset%2) {
					break;
				}
			}
		}
		regset >>= 1;
		if(!regset)
			break;
		argstack = argstack->next;
	}
}

static int
contains_call(ND1 *node)
{
	int op = node->op;
	switch(optype(op)) {
	case BITYPE:
		if(op == CALL || op == INCALL || op == STCALL)
			return 1;
		else return (contains_call(node->left) || contains_call(node->right));
	case UTYPE:
		return contains_call(node->left);
	default:
		return 0;
	}
}

static void
rewrite_calls(ND1 *node)
{
	int op = node->op;	
	Arg_stack argstack;

	switch(optype(op)) {
	case BITYPE:
		if(op == CALL && node->left->op == ICON &&
			!contains_call(node->right)) {
			SX sx = node->left->sid;
			int rewrote = 0;
			if(max_call_count)
				call_count++;
			if(sx != ND_NOSYMBOL && SY_ARGS_IN_REGS(sx)) {
				if(argstack = check_args(node->right,SY_ARGS_IN_REGS(sx))) {
					subst_funarg_op(argstack, SY_ARGS_IN_REGS(sx));
					rewrote = 1;
					while(argstack) {
						Arg_stack stack_item;
						stack_item = argstack;
						argstack = argstack->next;
						free((char *)stack_item);
					}
				}
			}
			if(! rewrote) rewrite_calls(node->right);
		}
		else rewrite_calls(node->right);
		/* fallthrough */
	case UTYPE:
		rewrite_calls(node->left);
		break;
	case LTYPE:
		break;
	}
}

void
cg_endf(maxauto)
OFFSET maxauto;
/* Do function epilogue code.  For function-at-a-time compilation,
** generate code for the entire function, up to this point.
** maxauto is maximum extent of automatic variables.
*/
{
    ND2 * new = t2alloc();
    SX func_id = cg_getcurfunc();

#ifdef	FAT_ACOMP
#ifndef NODBG
    if(print_cgq_flag) {
	fprintf(stderr,"++++++++++\nFUNCTION: %s(sid: %d)\n",SY_NAME(func_id),func_id);
#ifdef FIXED_FRAME
	if(fixed_frame()) {
		fprintf(stderr,"\tfixed_frame\n");
	}
#endif
	if(SY_ARGS_IN_REGS(func_id))
		fprintf(stderr,"\tSY_ARGS_IN_REGS: x%x\n",SY_ARGS_IN_REGS(func_id));
    }
#endif
	/* tell cg if we need to generate an alternate entry */
#ifdef FIXED_FRAME
    if(SY_ARGS_IN_REGS(func_id)) 
	set_alt_entry(SY_NAME(func_id),SY_ARGS_IN_REGS(func_id));	
#endif
    set_next_temp(maxauto);		/* put expression TEMPs after autos */
    cg_infunc = 0;			/* avoid queuing secondary calls */
    CGQ_FOR_ALL(flow,index)
	ND2 *begf;
	ND1 *nd1;
	if (flow->cgq_op == CGQ_EXPR_ND2 && 
	    (begf=flow->cgq_arg.cgq_nd2)->op == BEGF) {
    	    begf->name = al_g_maxreg();	/* list of registers used in func. */
	}
#ifdef FIXED_FRAME
	else if(nd1 = HAS_ND1(flow)) {
		rewrite_calls(nd1);
	}
#endif

    CGQ_END_FOR_ALL
#ifndef NODBG
	if(print_cgq_flag > 1)
		cg_putq(3);
#endif

    cg_q_gen();				/* generate accumulated stuff */

#ifndef NODBG
    if (a1debug)
	al_debug();
#endif

    maxauto = al_g_maxtemp();		/* new high-water mark */
    new->name = 0;

#else	/* not FAT_ACOMP */
    new->name = al_g_maxreg();          /* list of registers used in func. */
#endif /*FAT_ACOMP */

    CG_SETLOCCTR(PROG);

    new->op = ENDF;
    new->c.off = maxauto;

    cg_p2compile(new);
    cg_nameinfo(func_id);
    DB_ENDF();
    cg_unbind_globals();

#ifndef NO_AMIGO
    if (do_inline)
	free(td_cgq.td_start);
    else
	sy_discard();
#endif
    tfreeall();
    return;
}


void
cg_copyprm(sid)
SX sid;
/* Do semantic work required for a parameter.  sid is the symbol
** ID for the parameter.  There are two things to do:
**	1) For float arguments to functions without a prototype,
**		the actual is a double.  Need to copy, either in
**		place or elsewhere, possibly to a register.
**	2) For other parameters declared to be in registers,
**		try to allocate a register and move the actual.
**	3) For parameters that are smaller than int, the conceptual
**		truncation is handled magically by the offset
**		assignment calculations in CG.
** For any argument that actually DOES end up in a register,
** change the symbol's register number.
*/
{
    int regno = -1;			/* No register allocated. */
    T1WORD t = SY_TYPE(sid);
    TWORD cgtype = cg_tconv(t, 0);	/* Pass 2 equivalent type */
    TWORD cgetype = cgtype;		/* Effective parameter type (Pass 2) */
    static char * name_dotdotdot = (char *) 0;
    static char * name_varargs = "";	/* different from any symbol string */

    if (name_dotdotdot == 0) {
	/* Look up name strings for "..." and alternate varargs name. */
	name_dotdotdot = st_lookup("...");
#ifdef	VA_ALIST
	name_varargs = st_lookup(VA_ALIST);
#endif
    }

    /* Prepare arguments for bindparam(). */
    if (SY_FLAGS(sid) & SY_ISDOUBLE)
	cgetype = T2_DOUBLE;

#ifndef FAT_ACOMP
    if (SY_FLAGS(sid) & SY_ISREG) {
	regno = al_reg(VPARAM, (OFFSET) SY_OFFSET(sid), t);
	if (regno >= 0)
	    SY_REGNO(sid) = regno;
    }
#endif

    if (SY_REGNO(sid) != SY_NOREG) {
	regno = SY_REGNO(sid);
    }

    bind_param(cgtype, cgetype, (long) SY_OFFSET(sid), &regno);
    
    /* If a register was allocated, update our register information. */
    if (regno >= 0) {
	al_regupdate(regno, szty(cgtype));
	SY_REGNO(sid) = regno;		/* remember parameter is in reg */
    }

    return;
}



ND1 *
cg_getstat(t)
T1WORD t;
/* Return a suitable NAME node corresponding to an object of type t. */
{
    ND1 * p;
    p = tr_newnode(NAME);
    p->type = t;
    p->c.off = 0;
    p->sid = -sm_genlab();
    return( p );
}

void
cg_defstat(p)
ND1 *p;
/* Generate a label for the static NAME node pointed to by p.
** Location counter must be set suitably already.
*/
{
    ND2 * new;
    new = t2alloc();
    new->op = DEFNAM;
    new->type = cg_align(p->type);		/* use type's alignment */
    new->sid = DEFINE;
    new->name = cg_statname((int)-p->sid);
    cg_p2compile(new);
}

void
cg_bmove(sid, p)
SX sid;
ND1 * p;
/* Generate block move code to copy data starting at NAME node p
** to the location designated by sid.  CG requires UNARY & on
** both sides.
*/
{
    T1WORD t = SY_TYPE(sid);
    T1WORD ptrtype = ty_mkptrto(t);
    ND1 * bmovenode;
    ND1 * cmnode;
    ND1 * iconnode;
    unsigned int align = TY_ALIGN(t);

    /* Start by building , node for source, destination. */
    cmnode = tr_newnode(CM);
    cmnode->type = t;
    /* Need address of both sides.  Left is destination, right
    ** is source.
    */
    cmnode->left = tr_generic(UNARY AND, tr_symbol(sid), ptrtype);
    cmnode->right = tr_generic(UNARY AND, p, ptrtype);

    iconnode = tr_smicon(BITOOR(TY_SIZE(t)));
    bmovenode = tr_newnode(BMOVE);
    if (TY_ISMBRVOLATILE(t))		/* if members are volatile, set flags */
	bmovenode->flags |= FF_ISVOL;
    bmovenode->left = iconnode;
    bmovenode->right = cmnode;
    /* Choose a type for the bmove node that has the same alignment
    ** as the type being moved.
    */
    if (align == ALINT)
	t = TY_INT;
    else if (align == ALSHORT)
	t = TY_SHORT;
    else if (align == ALLONG)
	t = TY_LONG;
#ifndef NO_LDOUBLE_SUPPORT
    else if (align == ALLDOUBLE)
	t = TY_LDOUBLE;
#endif
    else if (align == ALDOUBLE)
	t = TY_DOUBLE;
    else if (align == ALFLOAT)
	    t = TY_FLOAT;
    else
	t = TY_CHAR;

    bmovenode->type = t;

    sm_expr(bmovenode);
    return;
}


void
cg_deflab(l)
int l;
/* Generate code for text label l. */
{
    ND2 * lab;
    
    CG_SETLOCCTR(PROG);

    lab = t2alloc();
    lab->op = LABELOP;
    lab->c.label = l;

    cg_p2compile(lab);
    return;
}

void
cg_goto(l)
int l;
/* Generate an unconditional branch to text label l. */
{
    ND2 * jump;

    CG_SETLOCCTR(PROG);

    jump = t2alloc();
    jump->op = JUMP;
    jump->type = T2_INT;
    jump->c.label = l;

    cg_p2compile(jump);
    return;
}
#endif /* ifndef LINT */

/* Switch handling.
** Switch nodes are collected together on a doubly-linked list.
** The list is headed by a SWEND node and terminated by a SWBEG.
** Starting from swend->left is an in-order NOP-terminated list
** of SWCASEs.  Starting from swend->right is a reverse-order
** SWBEG-terminated list of the same SWCASEs.
**
** Nested lists are handled by "pushing" the current swlist value
** on the new swbeg->right.
**
** NOTE: This requires use of ->right members of leaf and unary nodes!
*/

static ND2 *swlist;	/* switch list header */

void
cg_swbeg(p)
ND1 *p;
{
    ND2 *swbeg = t2alloc();
    ND2 *swend = t2alloc();
    ND2 *swnop = t2alloc();

#ifndef LINT
    CG_SETLOCCTR(PROG);
#endif /* ifndef LINT */
    swbeg->op = SWBEG;
#ifndef FAT_ACOMP
    cg_p2tree((ND2 *)p);		/* tree will be a Pass 2 tree */
    swbeg->type = ((ND2 *)p)->type;
#else
    swbeg->type = cg_tconv(p->type, 0); /* Save the Pass 2 type */
#endif
    swbeg->left = (ND2 *)p;
    swbeg->right = swlist; /* "push" current list (if any) */
    swlist = swend;
    swend->op = SWEND;
    swend->sid = -1; /* no default label (yet) */
    swend->c.size = 0; /* no SWCASE nodes (yet) */
    swend->right = swbeg;
    swend->left = swnop;
    swnop->op = NOP;
    swnop->c.off = 0;
    swnop->name = (char *)0;
    swnop->type = T2_INT;
}

void
cg_swcase(vp, l)
INTCON *vp;
int l;
/* Add case to current switch.  Check for duplicates.  Keep cases sorted
** in reverse order (largest first) with the expectation that larger values
** are more likely to occur later.  Thus a search for duplicates hopefully
** can be cut short.
*/
{
    extern int elineno;
    ND2 *prev, *p, *swcase;
    INTCON ext;
    TWORD st;
    int cmp;

    prev = swlist;
    p = prev->right;
    if (p->op == SWBEG)
	st = p->type;
    else
	st = prev->left->right->type;
    if (st != T2_LLONG && st != T2_ULLONG) {
	ext = *vp;
	(void)num_snarrow(&ext, SZLONG);
	vp = &ext;
    }
    while (p->op != SWBEG) {
	if ((cmp = num_scompare(vp, &p->c.ival)) > 0)
	    break;
	if (cmp == 0) {
	    UERROR(gettxt(":1619", "duplicate case value %s; first on line %d"),
		num_tosdec(vp), p->lop);
	    return;
	}
	prev = p;
	p = prev->right;
    }
    /* Unique value: insert a new node between prev and p. */
    swcase = t2alloc();
    swcase->op = SWCASE;
    swcase->type = T2_LLONG;
    swcase->sid = l;
    swcase->c.ival = *vp;
    swcase->lop = elineno; /* solely for the above duplication diagnostic */
    prev->right = swcase;
    swcase->right = p;
    if (p->op == SWBEG) { /* new in-order front */
	if (prev->op == SWEND) /* first in SWCASE list */
	    swcase->left = swlist->left; /* the NOP */
	else
	    swcase->left = prev;
	swlist->left = swcase;
    } else if (prev->op == SWEND) { /* new reverse-order front */
	swcase->left = p->left; /* the NOP */
	p->left = swcase;
    } else {
	p->left = swcase;
	swcase->left = prev;
    }
    swlist->c.size++;
}

void
cg_swend(l)
int l;
/* Generate end-of-switch code.  l is the default label.
** 0 means no default (fall through).
*/
{
    ND2 *swend = swlist;
    ND2 *swbeg;
#ifdef FAT_ACOMP
    cgq_arg_t arg;
#endif

    if (l != 0)
	swend->sid = l;
    swbeg = swend->left;
    if (swbeg->op != SWCASE) /* only when no SWCASEs */
	swbeg = swend->right;
    else
	swbeg = swbeg->right;
    swlist = swbeg->right; /* restore previous list (if any) */
#ifndef LINT
    CG_SETLOCCTR(PROG);
#endif
#ifdef FAT_ACOMP
    arg.cgq_nd2 = swbeg;
    cg_q(CGQ_FIX_SWBEG, (void (*)())NULL, arg);
#endif
    cg_p2compile(swend);
}

/* back to ignoring cgstuff for lint */
#ifndef LINT
void
cg_filename(fname)
char * fname;
/* Set new filename fname for CG. */
{
    /* Save new filename string if there's any likelihood we'll
    ** use it.  For our purposes, get the file's basename.
    */
    if (cg_outhdr) {
	char * start = strrchr(fname, '/');
	if (start++ == NULL)		/* Want portion after / */
	    start = fname;
	cg_fstring = st_lookup(start);
    }
    return;
}


void
cg_ident(i)
char * i;
/* Output #ident string.  Force out filename string, if any, first. */
{
    if (cg_outhdr)
	cg_begfile();
    fprintf(outfile, "	%s	%s\n", IDENTSTR, i);
    return;
}


void
cg_begfile()
/* Do start of file stuff, namely, produce .file pseudo-op.
** If a version string is defined (for the assembler) print
** it, too.
*/
{
    if (cg_outhdr) {
#ifdef	NO_DOT_FILE
	fprintf(outfile, "	%s	\"%s\"\n", NO_DOT_FILE, cg_fstring);
#else
	fprintf(outfile, "	.file	\"%s\"\n", cg_fstring);
#endif
#ifdef	COMPVERS
	fprintf(outfile, "	.version	\"%s\"\n", COMPVERS);
#endif
	cg_outhdr = 0;
    }
    return;
}


void
cg_asmold(p)
ND1 * p;
/* Write string node to output file as asm string.  Since CG doesn't
** supply trailing \n, we must.  Bracket output, if necessary, with
** implementation-specific bookends.
*/
{
    if (p->op != STRING)
	cerror(gettxt(":132","confused cg_asmold()"));
    
    /* Must be regular string literal. */
    if ((p->sid & TR_ST_WIDE) != 0)
	UERROR(gettxt(":133","asm() argument must be normal string literal"));

    /* CG can't handle string with embedded NUL. */
    else if (strlen(p->string) != p->c.size)
	UERROR(gettxt(":134","embedded NUL not permitted in asm()"));

#ifdef	ASM_COMMENT
    cg_copyasm(ASM_COMMENT);
#endif

    cg_copyasm(p->string);
    t1free(p);

#ifdef	ASM_END
    cg_copyasm(ASM_END);
#endif

    return;
}


static void
cg_copyasm(s)
char * s;
/* Generate COPYASM node for string s, follow with newline. */
{
    extern al_saw_asm; /* flag defined in p1allo.c */
    ND2 * new = t2alloc();
    al_saw_asm = 1;
    new->op = COPYASM;
    new->type = T2_VOID;
    new->name = s;
    cg_p2compile(new);

    new = t2alloc();
    new->op = COPYASM;
    new->type = T2_VOID;
    new->name = "\n";
    cg_p2compile(new);
    return;
}


void
cg_eof(flag)
int flag;
/* Do whatever needs to be done at end of file.  flag is
** C_TOKEN if there were any tokens, C_NOTOKEN otherwise.
** That means issuing a warning if no tokens were seen
** (an empty input file).  Also, we must be sure that all
** assembly files have the beginning of file stuff, so
** if we haven't done so already, do what needs to be
** done at the beginning of an output file.
*/
{
    record_end_of_src_file(PP_CURSEQNO());
#ifdef DEBUG
#if 0
    dmp_all_file_info();
#endif
#endif
#ifndef NO_AMIGO
    if (do_inline && (nerrors == 0))
	inline_eof();
#endif
    if (flag == C_NOTOKEN)
	WERROR(gettxt(":135","empty translation unit"));
    if (cg_outhdr)
	cg_begfile();
    if (cg_ldused) {
	/* Do something to force error at link time if
	** long double mismatch.
	*/
	CG_LDUSED();
    }
    return;
}

void
cg_profile()
/* Tell CG to generate functions with profiling information. */
{
    profile(1);
    return;
}
#endif
/* endif ifdef LINT */


void
cg_ldcheck()
/* Check for validity of long double type and issue error
** message as appropriate:  for strictly conforming version
** we can only issue a warning that things are treated as
** double.  For other versions, issue an error message to
** prevent further compilation.  In either case, just issue
** one message.
** Preserve errno, since this routine gets called from places
** that may expect errno to have been set (or clear).
*/
{
    if (!cg_ldused) {
	int olderrno = errno;

	cg_ldused = 1;		/* we've complained once */
	/* Hard error if CI4.1-compatible or ANSI compatible
	** modes; warning if "strictly conforming".
	*/
	if (version & V_STD_C)
	    WERROR(gettxt(":136","\"long double\" not yet supported; using \"double\""));
	else
	    UERROR(gettxt(":137","\"long double\" not yet supported"));
	errno = olderrno;
    }
    return;
}

OFFSET
cg_off_conv(space, off, from, to)
int space;
OFFSET off;
T1WORD from;
T1WORD to;
/* Mirror CG's off_conv(), but with pass 1 types.  Convert
** offset "off" for address space "space" from type "from"
** to type "to".
*/
{
    TWORD p2from = cg_tconv(from, 1);
    TWORD p2to = cg_tconv(to,1);
    off = off_conv(space, off, p2from, p2to);
    return( off );
}


BITOFF
al_struct(poffset, t, isfield, fsize)
BITOFF * poffset;
T1WORD t;
int isfield;
BITOFF fsize;
/* Allocate space for a structure member and return the bit
** number of the first bit for it.  t is the type of the
** member; isfield says whether to allocate a bitfield;
** fsize is the bitfield size if isfield is non-zero;
** poffset points to the starting bit offset, which gets
** updated to reflect the allocation of the new member.
*/
{
    int size = TY_SIZE( t );
    int align = TY_ALIGN( t );
    BITOFF noffset = *poffset;

#ifdef	PACK_PRAGMA
    /* Constrain alignment boundary of non-bitfield members. */
    if (!isfield && Pack_align != 0 && align > Pack_align)
	align = Pack_align;
#endif

    /* Update alignment for normal member or if zero-length
    ** field, or if not enough room for new field in current
    ** storage unit.
    */
    if (!isfield || fsize == 0 || (fsize + (noffset % align)) > size)
	noffset = AL_ALIGN(noffset, align);
    
    /* noffset contains the beginning of new member; we'll
    ** write back the update end bit number
    */
    *poffset = noffset + (isfield ? fsize : size);
    return( noffset );
}


#ifdef	OPTIM_SUPPORT

/* The following routines provide support for peephole
** optimizers.  More specifically the routines here pass information
** to CG, and CG generates information in the assembly code output.
**
** The interface contains two routines:
**	os_loop()	to generate information about loops
**	os_symbol()	to generate information about a symbol
**	os_uand()	to denote that the address of an object was taken
**
** --------	NOTE	--------
** This version assumes statement at a time compilation:  the calls
** here result in direct output (more or less).  For function at a
** time compilation we would have to store the string in temporary
** string space, create COPY nodes, and call cg_ecode().
**
** Another assumption (for speed right now) is that only parameters and
** top level automatics are of interest.
*/

void
os_loop(code)
int code;
/* Generate the appropriate optimizer information for the code.
** Typical codes are OI_LBODY, OI_LSTART, etc.
*/
{
#ifndef NO_GENERATE_OPTIM
#ifndef GENERATE_LOOP_INFO
    if (!do_amigo)
#endif
    	(void) fputs(oi_loop(code), outfile);
#endif
    return;
}

/* Do common setup for os_symbol, os_uand:  determine
** the correct interface storage class and set os_class.
** Discard symbols of no interest.  Build a node that's
** suitable to pass to one of CG's interface routines, or ND1NIL.
** If foralias is non-zero, enable some cases that don't apply
** for os_symbol():  defined struct/union parameters, "...".
*/

static int os_class;

static ND2 *
os_setup(sid, foralias)
SX sid;
int foralias;
{
    T1WORD t = SY_TYPE(sid);
    ND2 * name;

#ifdef NO_LDOUBLE_SUPPORT
    /*TEMPORARY:  avoid triggering long double error */
    if (TY_EQTYPE(t, TY_LDOUBLE))
	return( ND2NIL );
#endif

    /* Suppress checks here if foralias and name is parameter. */
    
    if (! (foralias && SY_CLASS(sid) == SC_PARAM)) {
	if (!TY_ISSCALAR(t) || TY_ISVOLATILE(t))
	    return( ND2NIL );
    }

    /* Avoid incomplete enum -- we don't know its size. */
    if (TY_TYPE(t) == TY_ENUM && ! TY_HASLIST(t))
	return( ND2NIL );

    /* Discard things already in registers. Hack: if not foralias
    ** and it's an argument, keep going ... we want an ALLOCDREGAL
    ** comment. */
    if ((SY_CLASS(sid) != SC_PARAM || foralias) && SY_REGNO(sid) != SY_NOREG)
	return( ND2NIL );

    /* Choose optimizer interface storage class for symbol. */
    switch( SY_CLASS(sid)) {
    case SC_PARAM:
	/* Parameters are special case.
	** If not for aliasing, checks for volatile, scalar have
	** already been done.  Generate symbol information.
	** For aliasing information, parameter need not be scalar,
	** but it must have been defined.  (Otherwise we'd generate
	** information for nested prototype declarations.)
	** "..." is a special case parameter:  it's not SY_DEFINED,
	** but we do generate aliasing information.
	*/
	if (   (SY_FLAGS(sid) & SY_DEFINED) != 0
	    || (foralias && SY_NAME(sid)[0] == '.')	/* "..." */
	) {
	    if(!foralias && SY_REGNO(sid) != SY_NOREG)
		os_class = OI_RPARAM;
	    else
		os_class = OI_PARAM;
	    break;
	}
	return( ND2NIL );
    case SC_AUTO:
	if (SY_LEVEL(sid) > SL_INFUNC && ! amigo_ran)
	    return( ND2NIL );		/* ignore non-top-level autos */
	os_class = OI_AUTO;
	break;
    case SC_EXTERN:
	os_class =
	    (SY_FLAGS(sid) & (SY_DEFINED|SY_TENTATIVE)) ? OI_EXTDEF : OI_EXTERN;
	break;
    case SC_STATIC:
	os_class = SY_LEVEL(sid) == SL_EXTERN ? OI_STATEXT : OI_STATLOC;
	break;
    default:
	return( ND2NIL );		/* other classes are of no interest */
    }

    name = (ND2 *) tr_symbol(sid);	/* Build a name node. */
    cg_p2tree(name);			/* convert type for CG */
    return( name );
}


void
os_symbol(sid)
SX sid;
/* Generate appropriate optimizer information for symbol whose
** index is sid.  Pass out a tree node and the symbol's storage
** class, size (mostly for struct/union) in bits, and level.
*/
{
#ifndef NO_GENERATE_OPTIM
    ND2 * name = os_setup(sid, 0);	/* not for aliasing information */
    int offset;

	/* Need to output the stack location of an allocated param */
    offset = (os_class == OI_RPARAM ? SY_OFFSET(sid) : 0);

    if (name) {
	char * s = oi_symbol((NODE *) name, os_class, offset);
	(void) fputs(s, outfile);
	nfree(name);
    }

    /* Take conservative approach on parameter struct/unions:
    ** mark them as having their address taken.  Only worry about
    ** parameters that we actually output information for.
    */
    if (SY_CLASS(sid) == SC_PARAM && TY_ISSU(SY_TYPE(sid))) {
	char * s;
	name = os_setup(sid, 1);	/* for aliasing information */
	if (name) {
	    s = oi_alias((NODE *) name);
	    (void) fputs(s, outfile);
	    nfree(name);
	}
    }
#endif
    return;
}


void
os_uand(sid)
SX sid;
/* Report that the address has been taken of a symbol. */
{
#ifndef NO_GENERATE_OPTIM
    ND2 * name = os_setup(sid, 1);	/* this is for aliasing */

    if (name) {
	char * s = oi_alias((NODE *) name);
	(void) fputs(s, outfile);
	nfree(name);
    }
#endif
    return;
}

#endif	/* OPTIM_SUPPORT */

/* Support for function-at-a-time compilation. */

#ifdef	FAT_ACOMP

TABLE(/*extern*/, td_cgq, /* extern */, cgq_init,
		cgq_t, INI_CGQ, 0, "cgstuff queued operations");
static void cg_q_do();

#ifndef	INI_CGPRINTBUF
#define	INI_CGPRINTBUF 100
#endif

TABLE(Static, cg_printbuf, static, cg_printbuf_init,
		char, INI_CGPRINTBUF, 0, "cg printf buffer");

static void
cg_append(s,len)
const char * s;
int len;
/* Append len characters to cg_printbuf. */
{
    if (cg_printbuf.td_used + len >= cg_printbuf.td_allo)
	td_enlarge(&cg_printbuf, len);
    
    memcpy((myVOID *) &TD_ELEM(cg_printbuf, char, cg_printbuf.td_used),
		(myVOID *) s, len);
    TD_USED(cg_printbuf) += len;
    TD_CHKMAX(cg_printbuf);
    return;
}

/* This routine is a VERY partial simulation of printf,
** and it doesn't pretend to be very smart about what
** it does.  It can handle the following formats:
**	%s
**	%.ns
**	%.*s
**	%d
*/

void
#ifdef __STDC__
cg_printf(const char *fmt, ...) {
    va_list args;
    va_start(args,fmt);
#else	/* non-ANSI case */
cg_printf(fmt, va_alist)
char *fmt;
va_dcl
{
    va_list args;
    va_start(args);
#endif

    TD_USED(cg_printbuf) = 0;

    for (;;) {
	char *fmtpos = strchr(fmt, '%');
	int len = (fmtpos ? fmtpos-fmt : strlen(fmt));
	int hadprec;
	int prec;

	cg_append(fmt,len);
	if (fmtpos == 0)
	    break;
	
	/* Had a format character. */
	fmt = fmtpos+1;
	hadprec = 0;
	if (*fmt == '.') {
	    hadprec = 1;
	    ++fmt;
	    if (*fmt == '*') {
		++fmt;
		prec = va_arg(args, int);
	    }
	    else {
		prec = 0;
		while (isdigit(*fmt)) {
		    prec = prec*10 + (*fmt - '0');
		    ++fmt;
		}
	    }
	}
	switch (*fmt) {
	    char * s;
	    char intbuf[20];
	    int intval;
	case 's':
	    s = va_arg(args, char *);
	    len = strlen(s);
	    if (hadprec && len > prec)
		len = prec;
	    cg_append(s,len);
	    break;
	case 'd':
	    intval = va_arg(args, int);
	    len = sprintf(intbuf, "%d", intval);
	    cg_append(intbuf,len);
	    break;
	default:
	    cerror(gettxt(":138","cg_printf():  bad fmt char %c"), *fmt);
	}
	++fmt;
    }

    cg_append("", 1);
    cg_q_puts(
	st_nlookup(&TD_ELEM(cg_printbuf, char, 0),
		  (unsigned int) TD_USED(cg_printbuf)
		  )
	  );
    return;
}


void
cg_q(op, func, arg)
int op;
void (*func)();
cgq_arg_t arg;
/* Queue operation for later if within a function. */
{
    cgq_t *qp;
    extern int elineno;

    if (cg_infunc) {
	Cgq_index new_index;
	TD_NEED1(td_cgq);
	qp = &TD_ELEM(td_cgq, cgq_t, TD_USED(td_cgq));
	qp->cgq_op = (short) op;
	qp->cgq_func = func;
	qp->cgq_arg = arg;
	qp->cgq_ln = elineno;
	qp->cgq_dbln = db_curline;
	qp->cgq_next = CGQ_NULL_INDEX;
	qp->cgq_prev = cgq_tail_index;
	qp->cgq_scope_marker = 0;
	qp->cgq_flags = 0;
	qp->cgq_location = location;
	location = UNK;
	new_index = (TD_USED(td_cgq))*sizeof(cgq_t);
	if (cgq_tail_index != CGQ_NULL_INDEX) {
		CGQ_ELEM_OF_INDEX(cgq_tail_index)->cgq_next = new_index;
	}
	cgq_tail_index = new_index;
	++TD_USED(td_cgq);
	TD_CHKMAX(td_cgq);
    }
    else
	cg_q_do(op, func, arg);
    return;
}

#ifndef NO_AMIGO
cgq_t *
cg_q_insert(place)
Cgq_index place;
/* Add item to queue after place. */
{
	cgq_t *qp;
	Cgq_index index_of_qp;
	TD_NEED1(td_cgq);
	qp = &TD_ELEM(td_cgq, cgq_t, TD_USED(td_cgq));
	if (place != CGQ_NULL_INDEX) {
		index_of_qp = (TD_USED(td_cgq))*sizeof(cgq_t);
		qp->cgq_next=CGQ_ELEM_OF_INDEX(place)->cgq_next;
		CGQ_ELEM_OF_INDEX(place)->cgq_next = index_of_qp;
		qp->cgq_prev = place;
		if(place != cgq_tail_index)
			CGQ_ELEM_OF_INDEX(qp->cgq_next)->cgq_prev = index_of_qp;
		else
			cgq_tail_index = index_of_qp;
	}
	qp->cgq_func = 0;
	qp->cgq_scope_marker = 0;
	qp->cgq_flags = 0;
	qp->cgq_location = UNK;
	++TD_USED(td_cgq);
	TD_CHKMAX(td_cgq);
	return qp;
}
		/*
		** Remove from the queue, but don't free.
		*/
void
cg_q_remove(place)
Cgq_index place;
{
	cgq_t *elem = CGQ_ELEM_OF_INDEX(place);
	CGQ_ELEM_OF_INDEX(elem->cgq_prev)->cgq_next = elem->cgq_next;
	CGQ_ELEM_OF_INDEX(elem->cgq_next)->cgq_prev = elem->cgq_prev;
}

	/*
	** Remove a segment from the queue and null the "next"
	** field of the last element.
	*/
void
cg_q_cut(from, to)
Cgq_index from, to;
{
	cgq_t *elem_from = CGQ_ELEM_OF_INDEX(from);
	cgq_t *elem_to = CGQ_ELEM_OF_INDEX(to);
	CGQ_ELEM_OF_INDEX(elem_from->cgq_prev)->cgq_next = elem_to->cgq_next;
	CGQ_ELEM_OF_INDEX(elem_to->cgq_next)->cgq_prev = elem_from->cgq_prev;
	elem_to->cgq_next = CGQ_NULL_INDEX;
}

		/*
		** Remove from the queue, free the item.
		*/
void
cg_q_delete(place)
Cgq_index place;
{

	cgq_t *elem = CGQ_ELEM_OF_INDEX(place);
	if (elem->cgq_op == CGQ_EXPR_ND1 || elem->cgq_op == CGQ_EXPR_ND2)
		tfree((NODE *)elem->cgq_arg.cgq_nd1);
	CGQ_ELEM_OF_INDEX(elem->cgq_prev)->cgq_next = elem->cgq_next;
	CGQ_ELEM_OF_INDEX(elem->cgq_next)->cgq_prev = elem->cgq_prev;
}
#endif

void cgq_print_tree();
static void
cg_q_gen()
/* Flush the CG queue of pending operations. */
{
    extern int elineno;
    int save_ln = elineno;
    int save_dbln = db_curline;

    CGQ_FOR_ALL(qp,index)
	elineno = qp->cgq_ln;
	db_curline = qp->cgq_dbln;
	cg_setlocctr(qp->cgq_location);
#if 0
fprintf(stderr,"FLUSHING CG_Q_ITEM:\n");
cgq_print_tree(qp,index,3);
#endif
	if ( ! (qp->cgq_flags & CGQ_NO_DEBUG)) {
	    cg_q_do(qp->cgq_op, qp->cgq_func, qp->cgq_arg);
	}  /* if */
    CGQ_END_FOR_ALL
    TD_USED(td_cgq) = 0;
    cgq_tail_index = CGQ_NULL_INDEX;
    elineno = save_ln;
    db_curline = save_dbln;
    return;
}

/* JUMP_FIX is a solution to following problem:  When a CBRANCH
** is optimized to a JUMP, sometimes CG_SETLOCCTR(PROG) is not
** called in time.  This is especially true if we remove all the
** code guarded by the expression.
*/

#define JUMP_FIX(n)	if (n->op == JUMP) 	CG_SETLOCCTR(PROG);

static void
cg_q_do(op, func, arg)
int op;
void (*func)();
cgq_arg_t arg;
/* Do a queued CG operation. */
{
    switch( op ){
    case CGQ_CALL:	(*func)(); break;
    case CGQ_CALL_SID:	(*func)(arg.cgq_sid); break;
    case CGQ_CALL_INT:	(*func)(arg.cgq_int); break;
    case CGQ_CTCH_HNDLR: /* Generate catch handler prolog */
			(*func)(arg.cgq_nd1);
			break;
    case CGQ_EH_A_SAVE_REGS: /* Generate saves/restores of local registers */
    case CGQ_EH_A_RESTORE_REGS:
    case CGQ_EH_B_SAVE_REGS:
    case CGQ_EH_B_RESTORE_REGS:
			(*func)(arg.cgq_nd1);
			break;
    case CGQ_TRY_BLOCK_START_LABEL:	/* Generate EH-related labels */
    case CGQ_TRY_BLOCK_END_LABEL:
    case CGQ_CATCH_HANDLERS_LABEL:
			(*func)(arg.cgq_int);
			break;
    case CGQ_FORBID_EXACT_FRAME:	/* Generate restriction for optim */
			(*func)();
			break;
    case CGQ_SUPPRESS_OPTIM:		/* Generate restriction for optim */
			(*func)(arg.cgq_str);
			break;
    case CGQ_EMIT_EH_TABLES:	/* Generate EH_related tables */
			function_has_eh_tables = 1;
			(*func)(arg.cgq_ptr);
			break;
    case CGQ_CALL_STR:	(*func)(arg.cgq_str); break;
    case CGQ_CALL_TYPE:	(*func)(arg.cgq_type); break;
    case CGQ_PUTS:	fputs(arg.cgq_str, stdout); break;
    case CGQ_EXPR_ND1:	cg_ecode(arg.cgq_nd1); break;
    case CGQ_FIX_SWBEG: cg_p2tree(arg.cgq_nd2->left);
			p2compile((NODE *) arg.cgq_nd2); 
			al_e_expr();
			break;
    case CGQ_EXPR_ND2:	JUMP_FIX(arg.cgq_nd2);
                        if (arg.cgq_nd2->op == INIT &&
			    arg.cgq_nd2->left->op == EH_OFFSET) {
			    arg.cgq_nd2->left->op = ICON;
			    (void)num_fromslong(&arg.cgq_nd2->left->c.ival,
				SY_OFFSET(arg.cgq_nd2->left->sid)
				+ arg.cgq_nd2->left->c.off);
			    arg.cgq_nd2->left->sid = ND_NOSYMBOL;
                        }
			p2compile((NODE *)arg.cgq_nd2); 
			break;
    case CGQ_START_SCOPE:
	break;
    case CGQ_END_SCOPE:
#ifdef OPTIM_SUPPORT
			if (amigo_ran)
				os_symbol(arg.cgq_sid);
#endif  /* OPTIM_SUPPORT */
	break;
    case CGQ_START_SPILL:
	break;
    case CGQ_END_SPILL:
	break;
    case CGQ_DELETED:
	break;
    default:
	cerror("cg_q_do():  bad op %d\n", op);
    }
    return;
}


void
cg_q_int(f, i)
void (*f)();
int i;
/* Queue function call with int argument. */
{
    cgq_arg_t u;
    u.cgq_int = i;
    cg_q(CGQ_CALL_INT, f, u);
    return;
}


void
cg_q_sid(f, sid)
void (*f)();
SX sid;
/* Queue function call with symbol ID argument. */
{
    cgq_arg_t u;
    u.cgq_sid = sid;
    cg_q(CGQ_CALL_SID, f, u);
    return;
}


void
cg_q_str(f, s)
void (*f)();
char *s;
/* Queue function call with string argument. */
{
    cgq_arg_t u;
    u.cgq_str = s;
    cg_q(CGQ_CALL_STR, f, u);
    return;
}


void
cg_q_type(f, t)
void (*f)();
T1WORD t;
/* Queue function call with type word argument. */
{
    cgq_arg_t u;
    u.cgq_type = t;
    cg_q(CGQ_CALL_TYPE, f, u);
    return;
}


void
cg_q_call(f)
void (*f)();
/* Queue function call with no argument. */
{
    cgq_arg_t u;			/* dummy argument */
    /* LINTED */
    cg_q(CGQ_CALL, f, u);
    return;
}


static void
cg_q_puts(s)
char * s;
/* Queue request to output string. */
{
    cgq_arg_t u;
    u.cgq_str = s;
    cg_q(CGQ_PUTS, CGQ_NOFUNC, u);
    return;
}

void
cg_q_nd1(p)
ND1 * p;
/* Queue Pass 1 node for later cg_ecode(). */
{
    cgq_arg_t u;
    u.cgq_nd1 = p;
    cg_q(CGQ_EXPR_ND1, CGQ_NOFUNC, u);
    return;
}


static void
cg_q_nd2(p)
ND2 * p;
/* Queue Pass 2 node for later p2compile(). */
{
    cgq_arg_t u;
    u.cgq_nd2 = p;
    cg_q(CGQ_EXPR_ND2, CGQ_NOFUNC, u);
    return;
}

void
cg_eh_a_save_regs(ND1 *node)
{
	fputs("/SAFE_ASM\n", outfile);
	cg_p2tree(node->left);
	cg_p2tree(node->right);
	node->op = EH_A_SAVE_REGS;
	node->type = TINT;
	p2compile(node);
}

void
cg_eh_a_restore_regs(ND1 *node)
{
	fputs("/SAFE_ASM\n", outfile);
	cg_p2tree(node->left);
	cg_p2tree(node->right);
	node->op = EH_A_RESTORE_REGS;
	node->type = TINT;
	p2compile(node);
}

void
cg_eh_b_save_regs(ND1 *node)
{
	cg_p2tree(node->left);
	cg_p2tree(node->right);
	node->op = EH_B_SAVE_REGS;
	node->type = TINT;
	p2compile(node);
	fputs("/ASMEND\n", outfile);
}

void
cg_eh_b_restore_regs(ND1 *node)
{
	cg_p2tree(node->left);
	cg_p2tree(node->right);
	node->op = EH_B_RESTORE_REGS;
	node->type = TINT;
	p2compile(node);
	fputs("/ASMEND\n", outfile);
}

void
cg_catch_handlers_beg(ND1 *node)
{
	cg_p2tree(node->left);
	cg_p2tree(node->right);
	node->op = EH_CATCH_VALS;
	node->type = TINT;
	p2compile(node);
}

void
cg_forbid_exact_frame()
{
	fputs("/NO_EXACT_FRAME\n", outfile);
}

void
cg_suppress_optim(char* s)
{
	 fprintf(outfile, "/SUPPRESS_%s\n", s);
}

#ifndef NODBG

char *
cg_db_loop(flow,buffer)
cgq_t * flow;
char *buffer;
{
	if (flow->cgq_func != os_loop) {
		sprintf(buffer,"%d", flow->cgq_arg.cgq_int);
		return buffer;
	}
	switch (flow->cgq_arg.cgq_int) {
	case OI_LSTART: return "start";
	case OI_LBODY: return "body";
	case OI_LCOND: return "cond";
	case OI_LEND: return "end";
	default: return "BAD LOOP";
	}

}
char *
cg_funcname(func,detail)
int detail;
void (*func)();
{
	if (func == os_uand) return ("os_uand");
	else if (func == os_loop) return ("os_loop");
	else if (func == cg_copyprm) return ("cg_copyprm");
	else if (func == cg_pushlc) return (detail > 1) ? ("cg_pushlc") : 0;
	else if (func == cg_poplc) return (detail > 1) ? ("cg_poplc") : 0;
	else if (func == ret_type) return  (detail > 1) ?  "ret_type" : 0;
	else if (func == al_s_fcode) return (detail > 1) ? ("al_s_fcode") : 0;
	else if (func == cg_p2tree) return (detail > 1) ? "cg_p2tree" : 0;
	else if (func == os_symbol) return (detail > 1) ? "os_symbol" : 0;
	else if (func == db_s_file) return(detail > 1) ?  ("db_s_file") : 0;
	else if (func == db_e_file) return (detail > 1) ? ("db_e_file") : 0;
	else if (func == db_begf) return (detail > 1) ? ("db_begf") : 0;
	else if (func == db_s_fcode) return (detail > 1)  ? ("db_s_fcode") : 0;
	else if (func == db_e_fcode) return (detail > 1) ? ("db_e_fcode") : 0;
	else if (func == db_s_block) return (detail > 1) ? ("db_s_block") : 0;
	else if (func == db_e_block) return (detail > 1) ? ("db_e_block") : 0;
	else if (func == db_symbol) return (detail > 1) ? ("db_symbol") : 0;
	else if (func == db_endf) return (detail > 1) ? ("db_endf") : 0;
	else if (func == db_lineno) return (detail > 1) ? "db_lineno" : 0;
	else if (func == db_sue) return (detail > 1) ? ("db_sue") : 0;
	else if (func == db_sy_clear) return (detail > 1) ? ("db_sy_clear") : 0;
	else if (func == cg_setlocctr) cerror("cg_setlocctr in queue");
	else if (func == db_s_inline) return (detail > 1) ? ("db_s_inline") : 0;
	else if (func == db_s_inline_expr) return (detail > 1) ? ("db_s_inline_expr") : 0;
	else if (func == db_e_inline) return (detail > 1) ? ("db_e_inline") : 0;
	else return ("NO FUNC");
}

void dprint1();
void dprint2();

void
cgq_print_tree(flow, index, detail)
cgq_t *flow;
Cgq_index index;
int detail;
{
	char *name;
	char buffer[12];
	if (flow->cgq_func == CGQ_NOFUNC)
		name = "";
	else
		name = cg_funcname(flow->cgq_func,detail);
	if (!name)
		return;
	fprintf(stderr,"flow=%d line=%d dbline=%d scope_marker=%d flags=%d ",
		CGQ_INDEX_NUM(index),
		flow->cgq_ln,
		flow->cgq_dbln,
		flow->cgq_scope_marker,
		flow->cgq_flags
	);
	switch (flow->cgq_op) {
	case CGQ_CALL: fprintf(stderr, "CALL %s\n",name); 
		break;
	case CGQ_CALL_SID: fprintf(stderr, "CALL_SID %s(%d)\n",
		name,flow->cgq_arg.cgq_sid); break;
	case CGQ_CALL_INT: 
		fprintf(stderr, "CALL_INT %s(%s)\n",
			name,cg_db_loop(flow,buffer));
		break;
	case CGQ_CTCH_HNDLR: /* Used only by glue */
		fprintf(stderr, "CATCH HANDLER temps = \n" );
		dprint1(flow->cgq_arg.cgq_nd1); 
		break;
	case CGQ_EH_A_SAVE_REGS: /* Used only by glue */
		fprintf(stderr, "EH A SAVE REGS temps = \n" );
		dprint1(flow->cgq_arg.cgq_nd1); 
		break;
	case CGQ_EH_A_RESTORE_REGS: /* Used only by glue */
		fprintf(stderr, "EH A RESTORE REGS temps = \n" );
		dprint1(flow->cgq_arg.cgq_nd1); 
		break;
	case CGQ_EH_B_SAVE_REGS: /* Used only by glue */
		fprintf(stderr, "EH B SAVE REGS temps = \n" );
		dprint1(flow->cgq_arg.cgq_nd1); 
		break;
	case CGQ_EH_B_RESTORE_REGS: /* Used only by glue */
		fprintf(stderr, "EH B RESTORE REGS temps = \n" );
		dprint1(flow->cgq_arg.cgq_nd1); 
		break;
	case CGQ_TRY_BLOCK_START_LABEL: /* Used only by glue */
		fprintf(stderr, "TRY BLOCK START LABEL(%d)\n", flow->cgq_arg.cgq_int);
		break;
	case CGQ_TRY_BLOCK_END_LABEL: /* Used only by glue */
		fprintf(stderr, "TRY BLOCK END LABEL(%d)\n", flow->cgq_arg.cgq_int);
		break;
	case CGQ_CATCH_HANDLERS_LABEL: /* Used only by glue */
		fprintf(stderr, "CATCH HANDLERS LABEL(%d)\n", flow->cgq_arg.cgq_int);
		break;
	case CGQ_FORBID_EXACT_FRAME: /* Used only by glue */
		fprintf(stderr, "FORBID EXACT FRAME\n");
		break;
	case CGQ_SUPPRESS_OPTIM: /* Used only by glue */
		fprintf(stderr, "SUPPRESS OPTIM %s\n", flow->cgq_arg.cgq_str);
		break;
	case CGQ_EMIT_EH_TABLES: /* Used only by glue */
		fprintf(stderr, "EMIT EH TABLES func(%x)\n", flow->cgq_arg.cgq_ptr);
		break;
	case CGQ_CALL_STR: fprintf(stderr, "CALL_STR %s(%s)\n",
		name,flow->cgq_arg.cgq_str);
		break;
	case CGQ_CALL_TYPE: fprintf(stderr, "CALL_TYPE %s(",
		name);
		ty_print(flow->cgq_arg.cgq_type); 
		fprintf(stderr, ")\n"); break;
	case CGQ_PUTS: fprintf(stderr, "PUTS %s\n",
		flow->cgq_arg.cgq_str); break;
	case CGQ_EXPR_ND1: fprintf(stderr, "ND1\n");
		dprint1(flow->cgq_arg.cgq_nd1); break;
	case CGQ_START_SCOPE:
		fprintf(stderr, "START_SCOPE(%d)\n",flow->cgq_arg.cgq_sid );
		break;
	case CGQ_END_SCOPE:
		fprintf(stderr, "END_SCOPE(%d)\n",flow->cgq_arg.cgq_sid );
		break;
	case CGQ_START_SPILL:
		fprintf(stderr, "START_SPILL(%d)\n",flow->cgq_arg.cgq_int );
		break;
	case CGQ_END_SPILL:
		fprintf(stderr, "END_SPILL(%d)\n",flow->cgq_arg.cgq_int );
		break;
	case CGQ_FIX_SWBEG: fprintf(stderr, "CGQ_FIX_SWBEG\n");
		dprint1(flow->cgq_arg.cgq_nd2->left); break;
		break;
	case CGQ_EXPR_ND2: fprintf(stderr, "ND2\n");
		dprint2(flow->cgq_arg.cgq_nd2);
		break;
	case CGQ_DELETED:
		fprintf(stderr, "DELETED\n");
		break;
	default:
		cerror("cg_print_tree():  bad op %d\n", flow->cgq_op);
	}
	fputc('\n',stderr);
}

void
cg_putq(detail) int detail;
{
	extern void cg_putq_between();
	cg_putq_between(detail,CGQ_FIRST_INDEX,cgq_tail_index);
}

void
cg_putq_between(detail,first,last)
Cgq_index first,last;
int detail;
{
	CGQ_FOR_ALL_BETWEEN(flow,index,first,last)
		extern block_boundary();
		int block_num;
		block_num = block_boundary(index);
		if( block_num )
			fprintf(stderr,
				"vvvvvvvvvvvvvvvvvvvvv Block %d vvvvvvvvvvvvvvvvvvvvv\n\n",
				block_num);
		cgq_print_tree(flow,index,detail);
	CGQ_END_FOR_ALL_BETWEEN
}

	/*
	** Only use for debugging when block data is suspect.
	*/
void
old_cg_putq(detail)
int detail;
{
	CGQ_FOR_ALL_BETWEEN(flow,index,CGQ_FIRST_INDEX,cgq_tail_index)
		cgq_print_tree(flow,index,detail);
	CGQ_END_FOR_ALL_BETWEEN
}

void
dprint1(node)
ND1 *node;
{
	extern int simple_print;
	simple_print = 1;
	tr_e1print(node,"T");
	fflush(stderr);
	simple_print = 0;
}
void
dprint2(node) 
ND2 *node;
{
	FILE save_outfile;
	save_outfile = *outfile;
	*outfile = *stderr;
	e22print(node,"T");
	*outfile = save_outfile;
	fflush(stderr);
}

#endif	/* ifndef NODBG */

#endif  /* def FAT_ACOMP */
