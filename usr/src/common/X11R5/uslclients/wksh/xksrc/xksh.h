#ident	"@(#)wksh:xksrc/xksh.h	1.1"

/*	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T    */
/*	and UNIX System Laboratories, Inc.			*/
/*	The copyright notice above does not evidence any       */
/*	actual or intended publication of such source code.    */

#ifndef XKSH_H
#define XKSH_H
#include <sh_config.h>

#ifndef SYMS_ONLY

#define SH_FAIL 1
#define SH_SUCC 0

#define PRSYMBOLIC			1
#define PRMIXED				2
#define PRDECIMAL			4
#define PRHEX				8
#define PRMIXED_SYMBOLIC	16
#define PRNAMES				32

#define UPP(CH) (islower(CH) ? toupper(CH) : (CH))
#define C_PAIR(STR, CH1, CH2) (((STR)[0] == (CH1)) && ((STR)[1] == (CH2)))
#define XK_USAGE(X) return(xk_usage(X), SH_FAIL);

/* In the future, this will require following pointers, unless we
** can always trace back types to typedefs.  For example, ulong is
** a typedef, but it is simple because it is really just a long.
*/
#define IS_SIMPLE(TBL) ((TBL)->flags & F_SIMPLE)

#ifndef N_DEFAULT /* From name.h */
/* Stolen out of include/name.h, the problems of including things
** out of the ksh code is major.  Hence, the copy rather than the
** include.
*/

struct Bfunction {
	long	(*f_vp)();		/* value function */
	long	(*f_ap)();		/* assignment function */
};

#endif /* N_DEFAULT: From name.h */

#define ALLDATA		INT_MAX

#define IN_BAND		1
#define OUT_BAND	2
#define NEW_PRIM	4

struct fd {
	int vfd;
	int flags;
	char mode;
	struct strbuf *lastrcv;
	int rcvcount;
	int sndcount;
	int uflags;
};

struct vfd {
	int fd;
};

extern struct fd *Fds;
extern struct vfd *Vfds;

struct libdesc {
	char *name;
	VOID *handle;
};
struct libstruct {
	char *prefix;
	int nlibs;
	struct libdesc *libs;
};

extern char xk_ret_buffer[];
extern char *xk_ret_buf;

#ifndef OSI_LIB_CODE
#define PARPEEK(b, s) (((b)[0][0] == s[0]) ? 1 :  0 )
#define PAREXPECT(b, s) (((b)[0][0] == s[0]) ? 0 : -1 )
#define OFFSET(T, M) ((int)(&((T)NULL)->M))

typedef char *string_t;

/*
 * Structures for driving generic print/parse/copy/free routines
 */

typedef struct memtbl {
	char *name;	/* name of the member */
	char *tname;	/* name of the typedef */
	char  kind;	/* kind of member, see #defines below */
	char  flags;	/* flags for member, see #defines below */
	short  tbl;	/* -1 or index into ASL_allmems[] array */
	short  ptr;	/* number of "*" in front of member */
	short  subscr;	/* 0 if no subscript, else max number of elems */
	short  delim;	/* 0 if no length delim, +1 if next field, -1 if prev */
	short  id;	/* Id of the ASL in which this def is made */
	short  offset;	/* offset into the C structure */
	short  size;	/* size of this member, for easy malloc'ing */
	long  choice;	/* def of tag indicating field chosen for unions */
} memtbl_t;

struct envsymbols {
	char *name;
	int  id;
	int  (*parsefunc)();
	int  (*printfunc)();
	char *tname;
	int  intlike;
	int  string;
	int  topptr;
	int  valbits;
	struct {
		char *name;
		ulong val;
		int  cover;
	} vals[64];
}; 

char *malloc();
char *realloc();
char *strdup();
VOID *getaddr();

/*
 * Definitions for the kind field of the above structure 
 */

#define K_CHAR		(0)	/* char or unchar */
#define K_SHORT		(1)	/* short or ushort */
#define K_INT		(2)	/* int or uint */
#define K_LONG		(3)	/* long, ulong, PRIM, etc. */
#define K_STRING	(4)	/* char * or char [] */
#define K_OBJID		(5)	/* objid_t *, note the star is included */
#define K_ANY		(6)	/* any_t */
#define K_STRUCT	(7)	/* struct { } */
#define K_UNION		(8)	/* union { } */
#define K_TYPEDEF	(9)	/* typedef */
#define K_DSHORT	(10)	/* short delimiter */
#define K_DINT	(11)	/* int delimiter */
#define K_DLONG	(12)	/* long delimiter */

/*
 * Definitions for the flags field of the above structure, bitmask
 */

#define F_SIMPLE		(1)	/* simple, flat type */
#define F_FIELD			(2) /* memtbl is a field of a structure, not the
								name of a type */
#define F_TBL_IS_PTR	(4) /* tbl field is pointer, not number; */
#define F_TYPE_IS_PTR	(8) /* type is built-in, but is already a pointer, like K_STRING */

#define SUCCESS	0
#define FAIL	(-1)

#define TRUE	1
#define FALSE	0

/* The following macro, RIF, stands for Return If Fail.  Practically
 * every line of encode/decode functions need to do this, so it aids
 * in readability.
 */
#define RIF(X) do { if ((X) == FAIL) return(FAIL); } while(0)

#endif /* not OSI_LIB_CODE */

#if !defined(OSI_LIB_CODE) || defined(NEED_SYMLIST)
struct symlist {
	struct memtbl tbl;
	int isflag;
	int nsyms;
	struct symarray *syms;
};
#endif

#define DYNMEM_ID		(1)
#define BASE_ID			(2)

#define ALTPUTS(STR) do { p_setout(1); p_str((STR), '\n'); } while(0)

#ifndef NULL
#define NULL	(0)
#endif
#endif /* SYMS_ONLY */

#ifdef SPRINTF_RET_LEN
#define lsprintf sprintf
#endif

struct symarray {
	const char *str;
	unsigned long addr;
};

#endif /* XKSH_H */
