#ident	"@(#)rtld:i386/rtfcns.c	1.27"

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "machdep.h"
#include "rtld.h"
#include "externs.h"
#include "dllib.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <elf.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include "stdlock.h"

#define ERRSIZE 512	/* size of buffer for error messages */

typedef struct Space	Space;
struct	Space
{
	Space	*s_next;
	size_t	s_size;
	char	*s_ptr;
};

static Space	*space;


/* null function used as place where a debugger can set a breakpoint */
void 
_r_debug_state()
{
	DPRINTF(LIST,
		(2, "rtld: r_debug_state: the link map's state is %d; _rt_event is 0x%x\n", 
		_r_debug.r_state, _rt_event));

	if (_rt_event != 0 && _r_debug.r_state == RT_CONSISTENT)
	{
		(*_rt_event)((ulong_t)&_r_debug);
	}
}

/* call init routines
 * reverse list as we go for later use in fini routines
 */
void
_rt_call_init(ilist)
mlist	**ilist;
{
	mlist	*flist = 0;
	mlist	*mptr;

	DPRINTF(LIST,(2, "rtld: _rt_call_init(0x%x)\n", ilist));
	mptr = *ilist;
	while(mptr)
	{
		/* careful! - we are unlinking from list
		 * as we walk it;
		 */
		rt_map	*lm = mptr->l_map;
		mlist	*mnext = mptr->l_next;

		if (!TEST_FLAG(lm, RT_INIT_CALLED))
		{
			void	(*itmp)();

			SET_FLAG(lm, RT_INIT_CALLED);
			itmp = INIT(lm);
			if (itmp)
			{
				(*itmp)();
				DPRINTF(LIST,(2, "rtld: called init for %s\n",
					NAME(lm)));
			}

			/* if we have inits whose order is special
			 * and we have reached the end of the
			 * special part of the list, inform the
			 * debugger.
			 */
			if (TEST_FLAG(lm, RT_LAST_SPECIAL_INIT))
			{
				_r_debug.r_state = RT_SYSTEM_INIT; 
				_r_debug_state();
			}
		}
		mptr->l_next = flist;
		flist = mptr;
		mptr = mnext;
	}
	*ilist = flist;
}

/* Rebuild init list for certain special system libraries where
 * the initiallization order is important.  The libraries
 * known so far are:
 * 1. libc.so.1
 * 2. libthread.so.1
 * 3. libC.so.1
 */

/* this array is ordered in the reverse order of init execution
 * NOTE: these symbol names are not necessarily synonyms for the
 * corresponding DT_INIT entries; those entries must still be
 * used for invoking the init routines.
 */
static const char * const special_inits[] = {
	"__libC_init",
	"__libthread_init",
	"__libc_init",
	0
};

void
_rt_special_order_inits()
{
	int		found = 0;
	int		i;
	const char	* const *lname;

	for(lname = special_inits; *lname; lname++)
	{
		Elf32_Sym	*sym;
		rt_map		*lm;

		if ((sym = _rt_lookup(*lname,  0, _rt_map_head,
			0, &lm, LOOKUP_NORM)) != 0)
		{
			mlist	*mptr;
			mlist	**mpptr;

			for(mpptr = &_rt_fini_list; 
				(mptr = *mpptr) != 0;
				mpptr = &mptr->l_next)
			{
				if (mptr->l_map == lm)
				{
					/* unlink from rt_fini_list
					 * and prepend to beginning
					 * of list
					 */
					if (!found)
					{
						SET_FLAG(lm,
						RT_LAST_SPECIAL_INIT);
						found = 1;
					}
					*mpptr = mptr->l_next;
					mptr->l_next = _rt_fini_list;
					_rt_fini_list = mptr;
					DPRINTF(LIST,(2, "rtld: inserting %s at head of init list\n",
					*lname));
					break;
				}
			}
		}
	}
}

/* build initialization list
 * recursive function - follow siblings first, then children,
 * then current node
 */
static int
build_init(lptr, ilist, tail)
mlist	*lptr, **ilist, **tail;
{
	rt_map	*lm = lptr->l_map;
	mlist	*mptr;

	DPRINTF(LIST,(2, "rtld: _rt_build_init(0x%x, 0x%x, 0x%x)\n",lm, ilist, tail));
	if (lptr->l_next)
	{
		if (!build_init(lptr->l_next, ilist, tail))
			return 0;
	}
	if (!TEST_FLAG(lm, RT_NEEDED_SEEN))
	{
		SET_FLAG(lm, RT_NEEDED_SEEN);
		if (NEEDED(lm))
		{
			if (!build_init(NEEDED(lm), ilist, tail))
				return 0;
		}
	}
	if ((mptr = (mlist *)_rtalloc(rt_t_ml)) == 0)
		return 0;
	mptr->l_map = lm;
	if (!*tail)
		*ilist = mptr;
	else
		(*tail)->l_next = mptr;
	*tail = mptr;
	return 1;
}

/* build list of init functions - we separate 
 * building the list from actually calling the init
 * routines so we can build the list within a protected
 * region of code - the calling of the routines is not
 * protected to allow access to lazy
 * binding from within init routines.
 *
 * upper level init - for a.out or dlopen'd lib;
 * calls recursive function;
 * returns 0 for fail, 1 for success;
 * fails only on memory allocation error 
 */
int
_rt_build_init_list(lm, ilist)
rt_map	*lm; 
mlist	**ilist;
{
	rt_map	*lm1;
	mlist	*tail = 0;
	mlist	*mptr;

	DPRINTF(LIST,(2, "rtld: _rt_build_init_list(0x%x, 0x%x)\n",lm, ilist));
	*ilist = 0;

	for(lm1 = _rt_map_head; lm1; lm1 = (rt_map *)NEXT(lm1))
		CLEAR_FLAG(lm1, RT_NEEDED_SEEN);

	SET_FLAG(lm, RT_NEEDED_SEEN); 		

	if (NEEDED(lm))
	{
		if (!build_init(NEEDED(lm), ilist, &tail))
			return 0;
	}
	if (NAME(lm))
	{
		/* not for a.out */
		if ((mptr = (mlist *)_rtalloc(rt_t_ml)) == 0)
			return 0;
		mptr->l_map = lm;
		if (!tail)
			*ilist = mptr;
		else
			tail->l_next = mptr;
	}
	return 1;
}

/* function called by atexit(3C) - goes through link_map
 * and invokes each shared object's _fini function (skips a.out)
 */
void 
_rt_do_exit()
{
	rt_map	*lm;

	/* call fini routines for objects loaded on startup */
	_rt_process_fini(_rt_fini_list, 1);

	/* call dlopen fini routines */
	_rt_dl_do_exit();

	/* make one more pass through map list to make sure
	 * we haven't missed any dlopen'd objects that
	 * were subsequently dlclose'd, but for some reason
	 * not unmapped
	 */
	 for(lm = _rt_map_head; lm; lm = (rt_map *)NEXT(lm))
	 {
		if (NAME(lm) && !TEST_FLAG(lm, RT_FINI_CALLED))
		{
			/* don't call fini routine if already called */
			void (*fptr)();

			fptr = FINI(lm);
			if (fptr)
			{
				(*fptr)();
				DPRINTF(LIST,(2, 
					"rtld: called fini for %s\n",
					NAME(lm)));
			}
		}
	 }
}

void
_rt_process_fini(list, about_to_exit)
mlist	*list;
int	about_to_exit;
{

	for (; list; list = list->l_next) 
	{
		rt_map	*lm = list->l_map;
		if ((about_to_exit || (COUNT(lm) == 0)) &&
			!TEST_FLAG(lm, RT_FINI_CALLED))
		{
			/* don't call fini routine if 
			 * we are not doing exit processing
			 * and the object's reference count hasn't
			 * reached 0 (for dlclose)
			 * or we have already called it
			 */
			void (*fptr)();

			SET_FLAG(lm, RT_FINI_CALLED);
			fptr = FINI(lm);
			if (fptr)
			{
				(*fptr)();
				DPRINTF(LIST,(2,
					"rtld: called fini for %s\n",
					NAME(lm)));
			}
		}
	}
}

/* Add space to free list.
 * We add to end of list so subsequent rtmalloc requests
 * keep getting space from the same block they were using, 
 * to preserve locality of reference.
 */
void
_rtmkspace(p, sz)	
char	*p;
size_t	sz;
{
	register Space		*sp;
	register ulong_t	n = (ulong_t)p;
	register size_t		j;

	DPRINTF(LIST|ALLOC,(2, "rtld: _rtmkspace(0x%x, %d)\n",p, sz));

	if (sz < 2 * sizeof(Space) + 64)
		return;
	/* align on double boundary */
	j = sizeof(double) - n % sizeof(double);
	sp = (Space *)(p + j);
	sp->s_size = sz - (j + sizeof(Space));
	sp->s_ptr = (char *)(sp + 1);
	sp->s_next = 0;
	if (!space)
	{
		space = sp;
	}
	else
	{
		register Space	*sptr = space;
		while(sptr->s_next)
		{
			sptr = sptr->s_next;
		}
		sptr->s_next = sp;
	}
#ifdef DEBUG
	if (_rt_debugflag & ALLOC)
	{
		for(sp = space; sp; sp = sp->s_next)
		{
			_rtfprintf(2, "rtld: Space block 0x%x has %d bytes available\n",
				sp, sp->s_size);
		}
	}
#endif
}

	
/* Local heap allocator.  
 * Very simple, does not support storage freeing. */
VOID *
_rtmalloc(nb)
register uint_t	nb;
{
	register Space	*sp, **spp;
	register char	*tp;

	DPRINTF(LIST|ALLOC, (2, "rtld: _rtmalloc(%d)\n", nb));

	/* we always round to double-word boundary */
	nb = DROUND(nb);

	for(spp = &space; (sp = *spp) != 0; spp = &sp->s_next)
	{
		DPRINTF(ALLOC, (2, "space block 0x%x has %d bytes available\n",
			sp, sp->s_size));
		if (sp->s_size >= nb)
		{
			tp = sp->s_ptr;
			if ((sp->s_size -= nb) >= sizeof(double))
			{
				sp->s_ptr += nb;
			}
			else
			{
				DPRINTF(ALLOC, (2, "space block 0x%x discarded\n",
					sp));
				/* unlink */
				*spp = sp->s_next;
			}
			return tp;
		}
	}
	/* map in at least a page of anonymous memory */
	if ((tp = _rt_map_zero(0, PROUND(nb), MAP_PRIVATE, 1))
		== (char *)-1)
	{
		return 0;
	}
	DPRINTF(ALLOC, (2, "rtld: _rtmalloc mapped %d bytes at 0x%x\n",
		PROUND(nb), tp));

	sp = (Space *)tp;
	tp += sizeof(*sp);
	sp->s_ptr = tp + nb;
	sp->s_size = PROUND(nb) - nb - sizeof(*sp);

	/* add to end */
	sp->s_next = 0;
	if (!space)
	{
		space = sp;
	}
	else
	{
		register Space	*sptr = space;
		while(sptr->s_next)
		{
			sptr = sptr->s_next;
		}
		sptr->s_next = sp;
	}
	return tp;
}

/* internal getenv routine - only a few strings are relevant */
CONST char *
_rt_readenv(envp, bmode)
CONST char	**envp;
int		*bmode;
{
	register CONST char	*s1;
	CONST char		*envdirs = 0;

	if (envp == (CONST char **)0)
		return((char *)0);
	while (*envp != (CONST char *)0) 
	{
		s1 = *envp++;
		if (*s1++ != 'L' || *s1++ != 'D' || *s1++ != '_' )
			continue;

#ifdef DEBUG
		if (_rtstrncmp( s1, "DEBUG=", 6 ) == 0) 
		{
			int	dlev = 0;
			s1 += 6;
			while(*s1 >= '0' && *s1 <= '9')
			{
				dlev *= 10;
				dlev += (int)*s1 - '0';
				s1++;
			}
			if (dlev > 0 && dlev <= MAXLEVEL)
				_rt_debugflag = dlev;
			continue;
		}
#endif
		if (_rtstrncmp( s1, "TRACE_LOADED_OBJECTS=", 21) == 0) 
		{
			s1 += 21;
			SET_CONTROL(CTL_TRACING);
			continue;
		}
		if (_rtstrncmp( s1, "WARN=", 5) == 0) 
		{
			s1 += 5;
			SET_CONTROL(CTL_WARN);
			continue;
		}
		if (_rtstrncmp( s1, "LIBRARY_PATH=", 13 ) == 0) 
		{
			s1 += 13;
			envdirs = s1;
			continue;
		}
		if (_rtstrncmp( s1, "BIND_NOW=", 9 ) == 0) 
		{
			s1 += 9;
			if (*s1 != '\0' )
				*bmode = RTLD_NOW;
			continue;
		}
	}

	/* LD_WARN is meaningful only if tracing */
	if (!CONTROL(CTL_TRACING))
		CLEAR_CONTROL(CTL_WARN);

	return envdirs;
}

void
_rt_fatal_error()
{
	_rtfprintf(2, "%s\n",_dlerror());
	(void)_rtkill(_rtgetpid(), SIGKILL);
}

/*  Local "fprintf"  facilities.  */

static char *printn ARGS((int, ulong_t, int, char *, register char *, char *));
static void doprf ARGS((int , CONST char *, va_list , char *));
static void rtstatwrite ARGS((int fd, char *buf, int len));


/*VARARGS2*/
#ifdef __STDC__
void
_rtfprintf(int fd, CONST char *fmt, ...)
#else
void 
_rtfprintf(fd, fmt, va_alist)
int	fd;
char	*fmt;
va_dcl
#endif
{
	va_list adx;
	char linebuf[ERRSIZE];
#ifdef __STDC__
	va_start(adx, fmt);
#else
	va_start(adx);
#endif
	doprf(fd, fmt, adx, linebuf);
	va_end(adx);
}

/* error recording function - we write the error string to
 * a static buffer and set a global pointer to point to the string
 */

/*VARARGS1*/
#ifdef __STDC__
void
_rt_lasterr(CONST char *fmt, ...)
#else
void
_rt_lasterr(fmt, va_alist)
char	*fmt;
va_dcl
#endif
{
	va_list adx;
	static char errptr[ERRSIZE];
#ifdef __STDC__
	va_start(adx, fmt);
#else
	va_start(adx);
#endif

	doprf(-1, fmt, adx, errptr);
	va_end(adx);
	_rt_error = errptr;
}

static void 
doprf(fd, fmt, adx, linebuf)
int		fd;
CONST char	*fmt;
char		*linebuf;
va_list		adx;
{
	register char	c;		
	register char	*lbp, *s;
	int		i, b, num;	

#define	PUTCHAR(c)	{\
			if (lbp >= &linebuf[ERRSIZE]) {\
				rtstatwrite(fd, linebuf, lbp - linebuf);\
				lbp = linebuf;\
			}\
			*lbp++ = (c);\
			}

	lbp = linebuf;
	while ((c = *fmt++) != '\0') 
	{
		if (c != '%') 
		{
			PUTCHAR(c);
		}
		else 
		{
			c = *fmt++;
			num = 0;
			switch (c) 
			{
			case 'x': 
			case 'X':
				b = 16;
				num = 1;
				break;
			case 'd': 
			case 'D':
			case 'u':
				b = 10;
				num = 1;
				break;
			case 'o': 
			case 'O':
				b = 8;
				num = 1;
				break;
			case 'c':
				b = va_arg(adx, int);
				for (i = 24; i >= 0; i -= 8)
				{
					if ((c = ((b >> i) & 0x7f)) != 0) 
					{
						PUTCHAR(c);
					}
				}
				break;
			case 's':
				s = va_arg(adx, char*);
				while ((c = *s++) != 0) 
				{
					PUTCHAR(c);
				}
				break;
			case '%':
				PUTCHAR('%');
				break;
			}
			if (num) 
			{
				lbp = printn(fd, va_arg(adx, ulong_t), b,
					linebuf, lbp, &linebuf[ERRSIZE]);
			}
			
		}
	}
	rtstatwrite(fd, linebuf, lbp - linebuf);
}

/* Printn prints a number n in base b. */
static char *
printn(fd, n, b, linebufp, lbp, linebufend)
int	fd, b;
ulong_t	n;
char		*linebufp, *linebufend;
register char	*lbp;
{
	char		prbuf[11]; /* Local result accumulator */
	CONST char	nstring[] = "0123456789abcdef";
	register char	*cp;

#undef PUTCHAR
#define	PUTCHAR(c)	{\
			if (lbp >= linebufend) {\
				rtstatwrite(fd, linebufp, lbp - linebufp);\
				lbp = linebufp;\
			}\
			*lbp++ = (char)(c);\
			}

	if (b == 10 && (int)n < 0) 
	{
		PUTCHAR('-');
		n = (uint_t)(-(int)n);
	}
	cp = prbuf;
	do 
	{
		*cp++ = nstring[n % b];
		n /= b;
	} while (n);
	do 
	{
		PUTCHAR(*--cp);
	} while (cp > prbuf);
	return (lbp);
}

static void
rtstatwrite(fd, buf, len)
int	fd, len;
char	*buf;
{
	if (fd == -1) 
	{
		*(buf + len) = '\0';
		return;
	}
	(void)_rtwrite(fd, buf, len);
}

static int		dz_fd = -1;

void
_rt_close_devzero()
{
	DPRINTF(LIST,(2,"rtld: _rt_close_devzero()\n"));
	if (dz_fd >= 0)
	{
		(void)_rtclose(dz_fd);
		dz_fd = -1;
	}
}

/* map from /dev/zero - open if needed; if one_time is set,
 * then close the file if we opened it, else leave it open
 */
char *
_rt_map_zero(addr, sz, flags, one_time)
ulong_t	addr;
size_t	sz;
int	flags;
int	one_time;
{
	static const char	dzname[] = "/dev/zero";
	int			dz_opened = 0;

	char	*tp;

	DPRINTF(LIST,(2,"rtld: _rt_map_zero(0x%x, %d, %d, %d)\n", addr, sz, flags, one_time));

	if (dz_fd == -1)
	{
		if ((dz_fd = _rtopen(dzname, O_RDONLY)) == -1) 
		{
			_rt_lasterr("%s: %s: can't open %s",
				(char*) _rt_name,_rt_proc_name, dzname);
				return (char *)-1;
		}
		dz_opened = 1;
	}
	if ((tp = _rtmmap((char *)addr, sz, (PROT_READ|PROT_WRITE),
		flags, dz_fd, 0)) == (caddr_t)-1)
	{
		_rt_lasterr(
			"%s: %s: cannot map zero filled pages",
			 _rt_name,_rt_proc_name);
	}
	if (dz_opened && one_time)
	{
		(void)_rtclose(dz_fd);
		dz_fd = -1;
	}
	return tp;
}

/* returns non-zero if set1 and set2 contain a common member, i.e.
 * the objects they belong to belong to a common group; else returns 0
 */
int 
_rt_ismember(lm1, lm2)
rt_map	*lm1, *lm2;
{
	struct rt_set	*set1, *set2;
	DPRINTF(LIST,(2,"rtld: _rt_ismember(0x%x, 0x%x)\n", lm1, lm2));

	set1 = GRPSET(lm1);
	set2 = GRPSET(lm2);
	
	while(set1 && set2)
	{
		if ((set1->members & set2->members) != 0)
			return 1;
		set1 = set1->next;
		set2 = set2->next;
	}
	return 0;
}

/* returns 1 if lm is a member of group grp, else 0 */
int
_rt_hasgroup(grp, lm)
ulong_t	grp;
rt_map	*lm;
{

	struct	rt_set	*set;
	ulong_t		chunk;
	ulong_t		bit;

	DPRINTF(LIST,(2,"rtld: _rt_hasgroup(%d, 0x%x)\n", grp, lm));

	set = GRPSET(lm);
	chunk = grp/LONG_BIT;
	bit = grp - chunk*LONG_BIT;

	for(; chunk != 0; chunk--)
	{
		set = set->next;
		if (!set)
			return 0;
	}
	return(set->members & ((ulong_t)1 << bit));
}

/* is lm a member of the global group (group 0)? */
int
_rt_isglobal(lm)
rt_map	*lm;
{
	struct	rt_set	*set;

	DPRINTF(LIST,(2,"rtld: _rt_isglobal(0x%x)\n", lm));
	set = GRPSET(lm);  /* always at least 1 */
	return(set->members & 1);
}

/* add group with id grp to set;
 * if is_global is non-zero, add set 0;
 * fails only on allocation error
 */

int 
_rt_addset(grp, lm, is_global)
ulong_t	grp;
rt_map	*lm;
uint_t is_global;
{
	struct	rt_set	*set;
	ulong_t		chunk;
	ulong_t		bit;
	
	DPRINTF(LIST,(2,"rtld: _rt_addset(%d, 0x%x, %d)\n", grp, lm, is_global));

	set = GRPSET(lm);
	if (is_global || grp == 0) 
	{
		/* global group is bit 1 of first set on list */
                set->members |= 1;
		if (grp == 0)
			return 1;
	}
	chunk = grp/LONG_BIT;
	bit = grp - chunk*LONG_BIT;
	for(; chunk != 0; chunk--)
	{
		/* depends on rtmalloc zeroing memory */
		if (!set->next)
		{
			set->next = (struct rt_set *)_rtalloc(rt_t_set);
			if (!set->next)
				return 0;
		}
		set = set->next;
	}
	set->members |= ((ulong_t)1 << bit);
	return 1;
}

/* delete group with id grp from set */
void
_rt_delset(grp, lm)
ulong_t	grp;
rt_map	*lm;
{
	struct rt_set	*set;
	ulong_t		chunk;
	ulong_t		bit;

	DPRINTF(LIST,(2,"rtld: _rt_delset(%d, 0x%x)\n", grp, lm));

	set = GRPSET(lm);
	chunk = grp/LONG_BIT;
	bit = grp - chunk*LONG_BIT;
	
	for(; chunk != 0; chunk--)
	{
		set = set->next;
		if (!set)
			return;
	}
	set->members &= ~((ulong_t)1 << bit);
}

/* add group to entire dependency graph of lm0 */
/* fails only on allocation error */
int
_rt_setgroup(group, lm0, is_global)
ulong_t	group;
rt_map	*lm0;
uint_t is_global;	
{
	mlist	*lptr;

	DPRINTF(LIST,(2,"rtld: _rt_setgroup(%d, 0x%x)\n", group, lm0));

	if (_rt_hasgroup(group, lm0))
		/* have already been here */
		return 1;
	if (!_rt_addset(group, lm0, is_global))
		return 0;
	for(lptr = NEEDED(lm0); lptr; lptr = lptr->l_next) 
	{
		if (!_rt_setgroup(group, lptr->l_map, is_global))
			return 0;
	}
	return 1;
}

/* add an entry to lm's reference list and bump refcount for
 * ref; can fail only if malloc fails
 * do not add or bump count if lm already references ref
 */
int
_rt_add_ref(lm, ref)
rt_map	*lm, *ref;
{
	mlist	*lptr, **lpptr;

	DPRINTF(LIST,(2,"rtld: _rt_add_ref(0x%x, 0x%x)\n", lm, ref));
	for(lpptr = &lm->r_reflist; (lptr = *lpptr) != 0;
		lpptr = &lptr->l_next)
	{
		if (ref == lptr->l_map) 
			return 1;
	}
	if ((lptr = (mlist *)_rtalloc(rt_t_ml)) == 0)
		return 0;

	lptr->l_map = ref;
	COUNT(ref) += 1;

	/* append the new list item */
	*lpptr = lptr;
	return 1;
}

/* add an entry to lm's needed list
 * can fail only if malloc fails;
 * this also results in incrementing the reference
 * count of the needed object
 */
int
_rt_add_needed(lm, needed)
rt_map	*lm, *needed;
{
	mlist	*lptr, **lpptr;

	DPRINTF(LIST,(2,"rtld: _rt_add_needed(0x%x, 0x%x)\n", lm, needed));
	for(lpptr = &lm->r_needed; (lptr = *lpptr) != 0;
		lpptr = &lptr->l_next)
	{
		if (needed == lptr->l_map) 
			return 1;
	}
	if ((lptr = (mlist *)_rtalloc(rt_t_ml)) == 0) 
		return 0;

	lptr->l_map = needed;
	COUNT(needed) += 1;

	/* append the new list item */
	*lpptr = lptr;
	return 1;
}

/* maintain free lists of most commonly used data structures
 * to prevent unnnecesary re-allocation
 */

static rt_map		*pm_free;
static DLLIB		*dl_free;
static mlist		*ml_free;
static struct rt_set	*set_free;

VOID *
_rtalloc(rt)
enum rttype	rt;
{
	size_t	sz;
	VOID	*ret = 0;

	DPRINTF(LIST, (2, "rtld: _rtalloc(%d)\n", rt));

	switch(rt)
	{
	case rt_t_pm:
		sz = sizeof(rt_map);
		if (!pm_free)
			break;
		ret = pm_free;
		pm_free = (rt_map *)NEXT(pm_free);
		break;
	case rt_t_dl:
		sz = sizeof(DLLIB);
		if (!dl_free)
			break;
		ret = dl_free;
		dl_free = dl_free->dl_next;
		break;
	case rt_t_ml:
		sz = sizeof(mlist);
		if (!ml_free)
			break;
		ret = ml_free;
		ml_free = ml_free->l_next;
		break;
	case rt_t_set:
		sz = sizeof(struct rt_set);
		if (!set_free)
			break;
		ret = set_free;
		set_free = set_free->next;
		break;
	}
	if (!ret)
		return(_rtmalloc(sz));
	_rtclear(ret, sz);
	return ret;
}

void
_rtfree(ptr, rt)
VOID		*ptr;
enum rttype	rt;
{
	DPRINTF(LIST, (2, "rtld: _rfree(0x%x, %d)\n", ptr, rt));
	switch(rt)
	{
	case rt_t_pm:
		NEXT((rt_map *)ptr) = (struct link_map *)pm_free;
		pm_free = (rt_map *)ptr;
		break;
	case rt_t_dl:
		((DLLIB *)ptr)->dl_next = dl_free;
		dl_free = (DLLIB *)ptr;
		break;
	case rt_t_ml:
		((mlist *)ptr)->l_next = ml_free;
		ml_free = (mlist *)ptr;
		break;
	case rt_t_set:
		((struct rt_set *)ptr)->next = set_free;
		set_free = (struct rt_set *)ptr;
		break;
	}
}

#ifdef _REENTRANT
/* routines to lock/unlock a recursive mutex 
 * we implement the recursive mutex with 2 regular
 * mutexes: one protects the lock data themselves, the other
 * protects the client data;  the recursive mutext allows
 * access by the same thread that currently has the mutex;
 * all other threads block;
 */
void
_rt_runlock(rlock)
rtld_rlock	*rlock;
{
	STDLOCK(&rlock->lock);
	if ((rlock->userown == (*_libc_self)()) && 
		(--rlock->usercnt <= 0))
	{
		rlock->userown = 0;
		STDUNLOCK(&rlock->user);
	}
	STDUNLOCK(&rlock->lock);
}

void
_rt_rlock(rlock)
rtld_rlock	*rlock;
{
	id_t	id = (*_libc_self)();

	for(;;)
	{
		int	locked = 0;
		STDLOCK(&rlock->lock);
		if (STDTRYLOCK(&rlock->user) == 0)
		{
			/* was not owned */
			rlock->userown = id;
			rlock->usercnt = 1;
		}
		else if (rlock->userown == id)
		{
			/* we already own */
			rlock->usercnt++;
		}
		else
		{
			/* someone else owns */
			locked = 1;
		}
		STDUNLOCK(&rlock->lock);
		if (locked)
		{
			/* block until user lock is released */
			STDLOCK(&rlock->user);
			STDUNLOCK(&rlock->user);
		}
		else
			break;
	}
}
#endif /* _REENTRANT */
