#ifndef _rtld_h_
#define _rtld_h_
#ident	"@(#)rtld:i386/rtld.h	1.22"

/* common header for run-time linker */

/* global macros - allow us to use ANSI or non-ANSI compilation */

/*
 * CONST is for stuff we know is constant.
 * VOID is for use in void* things.
*/
#ifdef	__STDC__
#define	CONST	const
#define	VOID	void
#else	
#define	CONST	/* empty */
#define	VOID	char
#endif

/*
 * ARGS(t) allows us to use function prototypes for
 * ANSI C and not use them for old C.
 * Example:
 *   void fido ARGS((int bone, uint_t fetch));
 * which expands to
 *   void fido (int bone, uint_t fetch);   in ANSI C
 * and
 *   void fido ();  in old C.
 * NOTE the use of two sets of parentheses.
*/
#ifdef	__STDC__
#define	ARGS(t)	t
#else	
#define	ARGS(t) ()
#endif	

#include "machdep.h"
#include <sys/types.h>
#include <elf.h>
#include <link.h>
#include "stdlock.h"

/* structure used to determine whether a particular shared library
 * is part of a given group, opened in a single call to rtld;
 * objects may reference symbols defined in any of the other objects 
 * belonging to the same groups to which they belong, or in any
 * object with the refpermit bit set
 */
struct rt_set {
	ulong_t		members;
	struct rt_set	*next;
};

/* struct of fd descriptors and path names used in
 * opening objects;
 */
typedef struct {
	char	*n_name;
	int	n_fd;
	dev_t	n_dev;
	ino_t	n_ino;
	struct rt_private_map	*n_lm;
} object_id;

/* linked list of rt_private maps; used to keep track of
 * dependencies
 */
typedef struct maplist {
	struct rt_private_map	*l_map;
	struct maplist		*l_next;
} mlist;

#ifdef _REENTRANT

/* recursive mutual exclusion lock */
typedef struct {
	StdLock	lock;		/* mutex for lock itself */
	StdLock	user;		/* mutex for this user */
	int	usercnt;	/* nesting depth */
	id_t	userown;	/* owner of lock 0 => nobody */
} rtld_rlock;

#define RTLD_RLOCK(P)	(__multithreaded && (_rt_rlock(P), 0))
#define RTLD_RUNLOCK(P)	(__multithreaded && (_rt_runlock(P), 0))

#else

#define RTLD_RLOCK(P)
#define RTLD_RUNLOCK(P)

#endif

/* run-time linker private data maintained for each shared object -
 * connected to link_map structure for that object
 */

typedef struct rt_private_map {
	struct link_map	r_public;	/* public data */
	VOID		*r_symtab;	/* symbol table */
	ulong_t		*r_hash;	/* hash table */
	char		*r_strtab;	/* string table */
	VOID		*r_reloc;	/* relocation table */
	ulong_t		*r_pltgot;	/* addresses for procedure linkage table */
	VOID		*r_jmprel;	/* plt relocations */
	ulong_t		r_pltrelsize;	/* size of PLT relocation entries */
	VOID		*r_delay_rel;	/* delayed relocations */
	ulong_t		r_delay_relsz; /* size of delayed relocations */
	void		(*r_init)();	/* address of _init */
	void		(*r_fini)();	/* address of _fini */
	ulong_t		r_relsz;	/* size of relocs */
	ulong_t		r_msize;	/* total memory mapped */
	VOID		*r_phdr;	/* program header of object */
	ushort_t	r_phnum;	/* number of segments */
	ushort_t	r_phentsize;	/* size of phdr entry */
	ushort_t	r_relent;	/* size of base reloc entry */
	ushort_t	r_syment;	/* size of symtab entry */
	ushort_t	r_count;	/* reference count */
	ushort_t	r_flags;	/* set of flags */
	dev_t		r_dev;		/* device for file */
	ino_t		r_ino;		/* inode for file */
	mlist		*r_needed;	/* needed list for this object */
	mlist		*r_reflist;	/* list of references */
	struct rt_set	r_grpset;	/* which groups does this 
					 * object belong to? */
} rt_map;

/* definitions for use with flags field */
#define RT_SYMBOLIC	0x1
#define RT_TEXTREL	0x2
#define RT_NODELETE	0x4
#define RT_REFERENCED	0x8
#define RT_BIND_NOW	0x10
#define RT_INIT_CALLED	0x20
#define RT_FINI_CALLED	0x40
#define RT_NEEDED_SEEN	0x80
#define RT_LAZY_PROCESSED	0x100
#define RT_LAST_SPECIAL_INIT	0x200
#define RT_DELAY_REL_PROCESSED	0x400

/* macros for getting to link_map data */
#define ADDR(X) ((X)->r_public.l_addr)
#define NAME(X) ((X)->r_public.l_name)
#define DYN(X) ((X)->r_public.l_ld)
#define NEXT(X) ((X)->r_public.l_next)
#define PREV(X) ((X)->r_public.l_prev)
#define TEXTSTART(X)	((X)->r_public.l_tstart)
#define TEXTSIZE(X)	((X)->r_public.l_tsize)
#define RANGES(X)	((X)->r_public.l_eh_ranges)
#define RANGES_SZ(X)	((X)->r_public.l_eh_ranges_sz)

/* macros for getting to linker private data */
#define SET_FLAG(X, F) ((X)->r_flags |= (F))
#define CLEAR_FLAG(X, F) ((X)->r_flags &= ~(F))
#define TEST_FLAG(X, F) ((X)->r_flags & (F))
#define COUNT(X) ((X)->r_count)
#define SYMTAB(X) ((X)->r_symtab)
#define HASH(X) ((X)->r_hash)
#define STRTAB(X) ((X)->r_strtab)
#define PLTGOT(X) ((X)->r_pltgot)
#define JMPREL(X) ((X)->r_jmprel)
#define PLTRELSZ(X) ((X)->r_pltrelsize)
#define INIT(X) ((X)->r_init)
#define FINI(X) ((X)->r_fini)
#define RELSZ(X) ((X)->r_relsz)
#define REL(X) ((X)->r_reloc)
#define RELENT(X) ((X)->r_relent)
#define SYMENT(X) ((X)->r_syment)
#define MSIZE(X) ((X)->r_msize)
#define PHDR(X) ((X)->r_phdr)
#define PHNUM(X) ((X)->r_phnum)
#define PHSZ(X) ((X)->r_phentsize)
#define REFLIST(X) ((X)->r_reflist)
#define GRPSET(X)  (&((X)->r_grpset))
#define NEEDED(X)  ((X)->r_needed)
#define DEV(X)  ((X)->r_dev)
#define INO(X)  ((X)->r_ino)
#define DELAY_REL(X) ((X)->r_delay_rel)
#define DELAY_RELSZ(X) ((X)->r_delay_relsz)


/* data structure used to keep track of COPY relocations */
struct rel_copy	{
	VOID		*r_to;		/* copy to address */
	VOID		*r_from;	/* copy from address */
	ulong_t		r_size;		/* copy size bytes */
	struct rel_copy	*r_next;	/* next on list */
	uchar_t 	r_special;	/* special process copy reloc */
};

/* data types recognized by free-list manipulator */
enum rttype {
	rt_t_pm,	/* rt_map */
	rt_t_dl,	/* DLLIB */
	rt_t_ml,	/* mlist */
	rt_t_set	/* struct rt_set */
};

/* Elf32_Dyn tags used in dlopen/rtld interface */
#define	DT_FPATH	(-1)	/* pathname of file to be opened */
#define	DT_MODE		(-2)	/* function binding mode */
#define	DT_MAP		(-3)	/* pointer to link_map */
#define	DT_GROUP	(-4)	/* dlopen group id */

#define DT_MAXNEGTAGS	4	/* number of negative tags */

/* debugger information version*/
#define LD_DEBUG_VERSION 1

/* flags for lookup routine */
#define	LOOKUP_NORM	0	/* normal action for a.out undefines */
#define	LOOKUP_SPEC	1	/* special action for a.out undefines */

/* rtld control flags */
#define	CTL_NO_DELETE	0x1
#define	CTL_TRACING	0x2
#define	CTL_WARN	0x4
#define	CTL_COPY_CTYPE	0x8
#ifdef GEMINI_ON_OSR5
#define	CTL_NOTELESS_SHUNT	0x10
#endif

#define CONTROL(C)	(_rt_control & (C))
#define SET_CONTROL(C)	(_rt_control |= (C))
#define CLEAR_CONTROL(C)	(_rt_control &= ~(C))

/* convenience macros for adding offsets to pointers */
#define NEWPTR(T, P, N)	((T *)((N) + (uchar_t *)(P)))
#define NEXT_PHDR(P, N)	(NEWPTR(Elf32_Phdr, (P), (N)))
#define NEXT_REL(P, N)	(NEWPTR(Elf32_Rel, (P), (N)))
#define NEXT_SYM(P, N)	(NEWPTR(Elf32_Sym, (P), (N)))

#ifdef GEMINI_ON_OSR5

/* OSR5 definitions for stat/fstat calls */
#define _ST_FSTYPSZ	16
#define _SCO_STAT_VER	51

struct osr5_stat32 {
	short		st_dev;
	long		st_pad1[3];
	ulong_t 	st_ino;
	ushort_t	st_mode;
	short		st_nlink;
	ushort_t 	st_uid;
	ushort_t 	st_gid;
	short		st_rdev;
	long		st_pad2[2];
	off_t		st_size;
	long		st_pad3;
	time_t		st_atime;
	time_t		st_mtime;
	time_t		st_ctime;
	long		st_blksize;
	long		st_blocks;
	char		st_fstype[_ST_FSTYPSZ];
	long		st_pad4[7];
	long		st_sco_flags;
};

#endif

#ifdef	DEBUG
/*
 * Print a debugging message
 * Usage: DPRINTF(DBG_MAIN, (MSG_DEBUG, "I am here"));
 */

#define MAXLEVEL	0x1f	/* maximum debugging level */
#define LIST		0x01	/* debugging levels - or'able flags */
#define DRELOC		0x02
#define MAP		0x04
#define ALLOC		0x08
#define PATH		0x10

#define	DPRINTF(D,M)	if (_rt_debugflag & (D)) _rtfprintf M

#else	/* DEBUG */
#define	DPRINTF(D,M)
#endif	/* DEBUG */

#endif
