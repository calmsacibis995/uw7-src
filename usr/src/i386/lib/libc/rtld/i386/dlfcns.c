#ident	"@(#)rtld:i386/dlfcns.c	1.31"

#include "machdep.h"
#include "rtld.h"
#include "externs.h"
#include "stdlock.h"
#include "dllib.h"
#include <dlfcn.h>
#include <elf.h>

/* Programmatic access to the dynamic linker.
 * Access by multiple threads is safe.
 * We use a two-level locking scheme.  Calls to
 * dlopen, dlclose and dlsym are protected by a recursive
 * mutual exclusion lock (dlopen calls init which may
 * in turn call dlopen, dlsym, etc.).
 * Since lazy binding calls to rtld's binder will invoke
 * _rt_lookup and so need to walk the link map, we have a 
 * separate lock used by binder() and the dl* routines,
 * but that is released during init/fini calls (since init
 * and fini can do lazy binding).
 */

#pragma weak dlopen = _dlopen
#pragma weak dlclose = _dlclose
#pragma weak dlerror = _dlerror
#pragma weak dladdr = _dladdr

/* return pointer to string describing last occurring error
 * the notion of the last occurring error is cleared
 */

char *
_dlerror()
{
	char	*etmp;

	DPRINTF(LIST,(2,"rtld: dlerror()\n"));
	etmp = _rt_error;
	_rt_error = (char *)0;

	return(etmp);
}

/* open a shared object - uses rtld to map the object into
 * the process' address space - maintains list of
 * known objects; on success, returns a pointer to the structure
 * containing information about the newly added object;
 * on failure, returns a null pointer
 */

static DLLIB		*dl_head;

#ifdef _REENTRANT
static rtld_rlock	dl_lock;
#endif

static int dl_delete		ARGS((rt_map *lm));

VOID *
_dlopen(pathname, mode)
CONST char	*pathname;
int		mode;
{
	rt_map		*lm, *first_map = 0;
	DLLIB		*dlptr = 0;
	DLLIB		*dltail;
	Elf32_Dyn	interface[4];
	Elf32_Dyn	lreturn[DT_MAXNEGTAGS+1];
	static ulong_t	dlgroup;
	int		fail_after_open = 0;
	int		bind_when;
	int		bind_scope;

	/* dlgroups specify NEEDED dependencies - group 0
	 * is reserved for everything loaded at startup
	 * plus any object loaded with RTLD_GLOBAL
	 */
	DPRINTF(LIST,(2,"rtld: dlopen(%s, %#x)\n",
		pathname?pathname:(CONST char *)"0",mode));

	bind_scope = mode & (RTLD_GLOBAL|RTLD_LOCAL);
	bind_when = mode & (RTLD_NOW|RTLD_LAZY);
	if (!bind_when || (bind_when == (RTLD_NOW|RTLD_LAZY)) ||
		(bind_scope == (RTLD_LOCAL|RTLD_GLOBAL)))
	{
		_rt_lasterr("%s: %s: illegal mode to dlopen: 0x%x",
			(char *) _rt_name, _rt_proc_name, mode);
		return 0;
	}

	if (!bind_scope)
		bind_scope = RTLD_LOCAL;

	RTLD_RLOCK(&dl_lock); 
	STDLOCK(&_rtld_lock);
	
	if (!dl_head)
	{
		/* first call: set up DLLIB structure for main */
		/* mark each object already on rt_map list as
		 * non-deletable so we do not remove those
		 * objects mapped in on startup
		 */
		CLEAR_CONTROL(CTL_NO_DELETE);
		for (lm = _rt_map_head; lm; lm = (rt_map *)NEXT(lm)) 
		{
			SET_FLAG(lm, RT_NODELETE);
		}
		SET_FLAG(_rtld_map, RT_NODELETE);
		if ((dl_head = (DLLIB *)_rtalloc(rt_t_dl)) == 0) 
			goto out;
		dl_head->dl_group = 0;
		dl_head->dl_object = _rt_map_head;
		dlgroup = 1;
	}

	if (pathname != 0) 
	{
		Elf32_Dyn	*retval;

		/* map in object if not already mapped */
		interface[0].d_tag = DT_FPATH;
		interface[0].d_un.d_ptr = (Elf32_Addr)pathname;
		interface[1].d_tag = DT_MODE;
		interface[1].d_un.d_val = mode;
		interface[2].d_tag = DT_GROUP;
		interface[2].d_un.d_val = dlgroup;
		interface[3].d_tag = DT_NULL;

		if (_rtld(interface, lreturn) != 0) 
			goto out;

		/* find pointer to rt_map of 
		 * shared object in Elf32_Dyn
		 * structure returned by rtld
		 */
		for(retval = lreturn; retval->d_tag != DT_NULL; 
			retval++)
		{
			if (retval->d_tag == DT_MAP)
				break;
		}
		if (retval->d_tag == DT_NULL) 
		{
			_rt_lasterr("%s: %s: internal interface error in dlopen",
				(char *)_rt_name, _rt_proc_name);
			goto out;
		}

		/* if we already have a dllib structure for this object,
		 * update it; otherwise create a new one
		 */
		lm = (rt_map *)(retval->d_un.d_ptr);
		dltail = 0;
		for (dlptr = dl_head; dlptr; 
			dltail = dlptr, dlptr = dlptr->dl_next) 
		{
			if (dlptr->dl_object == lm)
				break;
		}
	}
	else 
	{
		/* dlopen(0, mode) */
		dlptr = dl_head;
	}

	if (!dlptr) 
	{
		/* first open */
		if ((dlptr = (DLLIB *)_rtalloc(rt_t_dl)) == 0)
		{
			_rt_cleanup(lm);
			goto out;
		}
		/* save the first rt_map to be passed to
		 * _rt_call_init.
		 * The invocation of the _init routines for each object
	  	 * loaded will take place after the lock is released. 
		 * This is to prevent a deadlock that may occur when 
		 * an _init routine results in a call to _binder, 
		 * which also has a lock.
		 */
		first_map = lm;
		dlptr->dl_object = lm;
		dlptr->dl_group = dlgroup; /* set to new group */
		dlptr->dl_refcnt = 1;
		dltail->dl_next = dlptr;
		if (!_rt_hasgroup(dlgroup, lm))
		{
			/* rtld normally sets the group - if not
			 * set here, lm was loaded as the result of a
			 * different dlopen call, or on startup;
			 * group members need to be set with new group
			 */
			if (!_rt_setgroup(dlgroup, lm, 
				((mode & RTLD_GLOBAL) != 0)))
			{
				fail_after_open = 1;
				goto out;
			}
		}
		if (!_rt_build_init_list(lm, &dlptr->dl_fini))
		{
			dlptr->dl_fini = 0;
			fail_after_open = 1;
			goto out;
		}
		/* build ordering to be used in dlsym calls;
		 * there are 2 possible orders: strict dependency
		 * order (this is the X/Open standard)
		 * and load order - this is for compatibility
		 * with earlier UnixWare and OpenServer versions;
		 * also, increment reference count of 
		 * each object on list 
		 */
		if (_rt_dlsym_order_compat)
		{
			/* load order */
			mlist	**mpptr = &dlptr->dl_order;
			for (lm = _rt_map_head; lm; 
				lm = (rt_map *)NEXT(lm)) 
			{
				if (_rt_hasgroup(dlgroup, lm))
				{
					mlist	*ml;
					if ((ml = _rtalloc(rt_t_ml))
						== 0)
					{
						fail_after_open = 1;
						goto out;
					}
					COUNT(lm) += 1;
					ml->l_map = lm;
					*mpptr = ml;
					mpptr = &ml->l_next;
				}
			}
		}
		else
		{
			/* create dependency ordering;
			 * ordering is breadth-first on needed entries
			 */
			mlist	*tail;
			mlist	*ml;
			for (lm = _rt_map_head; lm; 
				lm = (rt_map *)NEXT(lm)) 
			{
				CLEAR_FLAG(lm, RT_NEEDED_SEEN);
			}
			if ((ml = _rtalloc(rt_t_ml)) == 0)
			{
				fail_after_open = 1;
				goto out;
			}
			COUNT(first_map) += 1;
			ml->l_map = first_map;
			dlptr->dl_order = ml;
			tail = ml;
			for(; ml; ml = ml->l_next)
			{
				/* add to list as we traverse it */
				mlist *needed;
				if (TEST_FLAG(ml->l_map, RT_NEEDED_SEEN))
					continue;
				SET_FLAG(ml->l_map, RT_NEEDED_SEEN);
				needed = NEEDED(ml->l_map);
				for(; needed; needed = needed->l_next)
				{
					mlist	*cur;
					if ((cur = _rtalloc(rt_t_ml))
						== 0)
					{
						fail_after_open = 1;
						goto out;
					}
					cur->l_map = needed->l_map;
					COUNT(cur->l_map) += 1;
					tail->l_next = cur;
					tail = cur;
				}
			}
		}
		dlgroup++;
	}
	else
	{
		/* already dlopen'd - update if modes have changed */
		if ((bind_when == RTLD_NOW) &&
			!TEST_FLAG(dlptr->dl_object, RT_LAZY_PROCESSED))
		{
			if (dlptr == dl_head)
			{
				/* dlopen(0, mode) - update all
				 * global objects
				 */
				for (lm = _rt_map_head; lm; 
					lm = (rt_map *)NEXT(lm)) 
				{
					if (_rt_isglobal(lm))
					{
						if (!_rt_process_lazy_bindings(lm))
						{
							dlptr = 0;
							goto out;
						}
					}
				}
			}
			else
			{
				mlist	*mptr = dlptr->dl_order;
				for(; mptr; mptr = mptr->l_next)
				{
					if (!_rt_process_lazy_bindings(mptr->l_map))
					{
						dlptr = 0;
						goto out;
					}
				}
			}
		}
		if ((bind_scope == RTLD_GLOBAL) && 
			!_rt_isglobal(dlptr->dl_object))
		{
			mlist	*mptr = dlptr->dl_order;
			for(; mptr; mptr = mptr->l_next)
			{
				if (!_rt_addset(0, mptr->l_map, 1))
				{
					dlptr = 0;
					goto out;
				}
			}
		}
		dlptr->dl_refcnt++;
	}
out:
	_rt_close_devzero();

	STDUNLOCK(&_rtld_lock);

	if (fail_after_open)
	{
		RTLD_RUNLOCK(&dl_lock);
		_dlclose(dlptr);
		return 0;
	}
	/* init routines must be called after unlock of the binder lock
	 * since they may result in calls to binder 
	 */
	if (first_map)
	{
		_rt_call_init(&dlptr->dl_fini);
	}
	RTLD_RUNLOCK(&dl_lock);
	return (VOID *)dlptr;
}

/* takes the address of the calling function, the name of a symbol 
 * and a pointer to a dllib structure;
 * searches for the symbol in the shared object specified
 * and in all objects in the specified object's needed list
 * returns the address of the symbol if found; else 0;
 * if handle is RTLD_NEXT, search begins with object after
 * caller in rt_map list.
 */

VOID *
_rt_real_dlsym(caller, handle, name)
ulong_t caller;
VOID	*handle;
CONST	char *name;
{
	rt_map		*def_lm;
	rt_map		*lm = 0;
	rt_map		*ref_lm = 0;
	Elf32_Sym	*sym = 0;
	ulong_t		addr = 0;

	DPRINTF(LIST,(2,"rtld: _rt_real_dlsym(0x%x, 0x%x, %s)\n",
		caller, handle, name ? name:(CONST char *)"0"));

	if (!name) 
	{
		_rt_lasterr("%s: %s: null symbol name to dlsym",
			(char *)_rt_name, _rt_proc_name);
		return(0);
	}

	RTLD_RLOCK(&dl_lock);

	if (handle == RTLD_NEXT)
	{
		/* start search with object loaded after object
		 * containing caller
		 */
		for (ref_lm = _rt_map_head; ref_lm; 
			ref_lm = (rt_map *)NEXT(ref_lm)) 
		{
			if ((caller >= TEXTSTART(ref_lm)) &&
				(caller < (TEXTSTART(ref_lm) +
					TEXTSIZE(ref_lm))))
				break;
		}
		if (!ref_lm)
		{
			_rt_lasterr("%s: %s: dlsym: (RTLD_NEXT) cannot find object containing caller at address 0x%x",
				(char *)_rt_name, _rt_proc_name, caller);
			goto out;
		}
		lm = (rt_map *)NEXT(ref_lm);
	}
	else if (((DLLIB *)handle)->dl_refcnt <= 0) 
	{
		_rt_lasterr("%s: %s: dlsym: attempt to find symbol %s in closed object",
			(char *)_rt_name, _rt_proc_name, name);
		goto out;
	}
	else if ((DLLIB *)handle == dl_head)
	{
		/* dlopen(0, mode) */
		/* search entire chain of global objects */
		lm = _rt_map_head;
	}
	if (lm != 0)
	{
		/* search all global objects or, if there was a
		 * ref_lm, all objects in the same dlopen groups;
		 * search order is load order
		 */
		for (; lm; lm = (rt_map *)NEXT(lm)) 
		{
			if (_rt_isglobal(lm) ||
				(ref_lm && _rt_ismember(lm, ref_lm)))
			{
				if ((sym = _rt_lookup(name, lm, 0, 
					0, &def_lm, LOOKUP_NORM))
					!= (Elf32_Sym *)0)
						break;
			}
		}
	}
	else if (!ref_lm)
	{
		/* just search this dlgroup */
		mlist	*ml;
		for (ml = ((DLLIB *)handle)->dl_order; ml; 
			ml = ml->l_next)
		{
			if ((sym = _rt_lookup(name, ml->l_map, 0, 0, 
				&def_lm,
				LOOKUP_NORM)) != (Elf32_Sym *)0)
				break;
		}
	}
	if (!sym) 
	{
		_rt_lasterr("%s: %s: dlsym: cannot find symbol: %s",
			(char *) _rt_name, _rt_proc_name, name);
		goto out;
	}
	addr = sym->st_value;
	if (NAME(def_lm))
		addr += ADDR(def_lm);

out:
	RTLD_RUNLOCK(&dl_lock);
	return((VOID *)addr);
}

/* Close the shared object associated with handle;
 * reference counts are decremented - we check reference
 * counts of all objects in dlopen group; 
 * When an object's reference count goes to 0, it is unmapped.
 * An object's reference count is incremented for:
 *	dlopen group
 *	each time it is a member of an object's needed list
 *	each object implicitly referencing this object.
 * So, an object cannot be unmapped until:
 *	All dlopen groups of which it is a member are closed;
 *	All objects whose needed lists it belongs to have 0 ref counts
 *	All objects implicitly referencing it have 0 ref counts
 * Returns 0 on success, 1 on failure.
 */

int 
_dlclose(handle)
VOID	*handle;
{
	rt_map	*lm;
	DLLIB	*dlptr, **dlpptr;
	DLLIB	*object = (DLLIB *)handle;
	int	ret = 0;
	int	group;
	mlist	*ml;

	DPRINTF(LIST,(2,"rtld: dlclose(0x%x)\n",object));

	RTLD_RLOCK(&dl_lock);
	STDLOCK(&_rtld_lock); 

	if (object->dl_refcnt <= 0) 
	{
		_rt_lasterr("%s: %s: dlclose: attempt to close already closed object",
			(char *) _rt_name,_rt_proc_name);
		ret = 1;
		goto out;
	}

	if ((--(object->dl_refcnt) != 0) || object == dl_head)
		goto out;

	/* last close */
	group = object->dl_group;

	/* decrement reference counts of all objects associated
	 * with this object
	 */

	for (ml = object->dl_order; ml; ml = ml->l_next)
	{
		lm = ml->l_map;
		COUNT(lm) -= 1;
		_rt_delset(group, lm);
		if (COUNT(lm) <= 0) 
		{
			mlist	*mptr;
			for(mptr = REFLIST(lm); mptr; 
				mptr = mptr->l_next)
				COUNT(mptr->l_map) -= 1;
			for(mptr = NEEDED(lm); mptr; 
				mptr = mptr->l_next)
				COUNT(mptr->l_map) -= 1;
		}
	}

	/* unlink from dllist */
	/* can never unlink first item - global group */
	dlpptr = &dl_head->dl_next;
	for(; (dlptr = *dlpptr) != 0; dlpptr = &dlptr->dl_next)
	{
		if (dlptr == object)
			break;
	}
	*dlpptr = dlptr->dl_next;

	/* call fini routines of dlgroup 
	 * we release the binder lock here to allow fini routines
	 * to do lazy binding
	 */

	STDUNLOCK(&_rtld_lock);
	_rt_process_fini(dlptr->dl_fini, 0);

	/* now re-acquire lock and check for non-referenced
	 * objects
	 */

	STDLOCK(&_rtld_lock);

	/* go through map list - for each entry
	 * whose refcount has gone to 0
	 * delete those members;
	 * careful - we are unlinking from this list as we walk it!
	 */
	lm = _rt_map_head;
	while(lm)
	{
		rt_map	*nlm = (rt_map *)NEXT(lm);
		if ((COUNT(lm) <= 0) && !TEST_FLAG(lm, RT_NODELETE))
		{
			if (!dl_delete(lm))
			{
				ret = 1;
			}
		}
		lm = nlm;
	}
	ml = dlptr->dl_order;
	while(ml)
	{
		mlist	*mnext = ml->l_next;
		_rtfree(ml, rt_t_ml);
		ml = mnext;
	}
	ml = dlptr->dl_fini;
	while(ml)
	{
		mlist	*mnext = ml->l_next;
		_rtfree(ml, rt_t_ml);
		ml = mnext;
	}
	_rtfree(dlptr, rt_t_dl);
out:
	STDUNLOCK(&_rtld_lock);
	RTLD_RUNLOCK(&dl_lock);
	return(ret);
}


static int 
dl_delete(lm)
rt_map	*lm;
{
	int ret = 1;

	DPRINTF(LIST,(2,"rtld: dl_delete(0x%x)\n",lm));

	/* alert debuggers that link_map list is shrinking */
	_r_debug.r_state = RT_DELETE;
	_r_debug_state();

	if (!TEST_FLAG(lm, RT_FINI_CALLED))
	{
		/* might not have been called if there
		 * were still references to this object
		 * when its dlgroup was closed
		 */
		void (*fptr)();
		fptr = FINI(lm);
		if (fptr)
			(*fptr)();
	}
	/* unlink lm from chain; we never unlink the 1st item on 
	 * the chain (the a.out)
	 */
	NEXT((rt_map *)PREV(lm)) = NEXT(lm);
	if (!NEXT(lm))
		_rt_map_tail = (rt_map *)PREV(lm);
	else
		PREV((rt_map *)NEXT(lm)) = PREV(lm);

	/* alert debuggers and profilers 
	 * that link_map is consistent again;
	 * this must happen before library is unmapped, so
	 * profilers can look in library's data section
	 */

	_r_debug.r_state = RT_CONSISTENT;
	_r_debug_state();

	if (!_rt_unmap_so(lm))
	{
		_rt_lasterr("%s: %s: dlclose: failure unmapping %s", 
			(char *) _rt_name, _rt_proc_name, NAME(lm));
		ret = 0;
	}

	return ret;
}

/* call fini functions for all dlgroups still around at exit 
 */

void
_rt_dl_do_exit()
{

	RTLD_RLOCK(&dl_lock);

	if (dl_head)
	{
		DLLIB	*dllist;

		dllist = dl_head->dl_next;
		dl_head->dl_next = 0;

		for(; dllist; dllist = dllist->dl_next) 
		{
			_rt_process_fini(dllist->dl_fini, 1);
		}
	}

	RTLD_RUNLOCK(&dl_lock);
}

/* dladdr fills in a Dl_info structure with info on the symbol
 * closest to address addr (closest means equal to or
 * the nearest symbol with a lower address).  If addr does not fall
 * within one of the loaded objects, returns 0; otherwise
 * returns 1.
 */

int
_dladdr(addr, dlip)
VOID	*addr;
Dl_info	*dlip;
{

	rt_map		*lm;
	Elf32_Sym	*sym, *closest;
	ulong_t		sym_size;
	ulong_t		nsyms;
	ulong_t		base;
	ulong_t		diff;
	int		ndx;

	DPRINTF(LIST,(2,"rtld: dladdr(0x%x, 0x%x)\n", addr, dlip));

	if (!dlip)
	{
		_rt_lasterr("%s: %s: dladdr: null Dl_info pointer",
			(char *)_rt_name, _rt_proc_name);
		return 0;
	}

	for(lm = _rt_map_head; lm; lm = (rt_map *)NEXT(lm))
	{
		if (((ulong_t)addr >= ADDR(lm)) &&
			((ulong_t)addr < (ADDR(lm) + MSIZE(lm))))
			break;
	}
	if (!lm)
	{
		_rt_lasterr("%s: %s: dladdr: address 0x%x does not match any loaded object",
			(char *)_rt_name, _rt_proc_name, addr);
		return 0;
	}

	if ((dlip->dli_fname = NAME(lm)) == 0)
		dlip->dli_fname = _rt_proc_name;
	dlip->dli_fbase = (VOID *)ADDR(lm);
	
	sym_size = SYMENT(lm);
	nsyms = HASH(lm)[1];
	/* first symbol entry is null */
	sym = NEXT_SYM(SYMTAB(lm), sym_size);
	diff = ~0UL;
	base = NAME(lm) ? ADDR(lm) : 0;
	closest = 0;


	for (ndx = 1; ndx < nsyms; ndx++, 
		sym = NEXT_SYM(sym, sym_size))
	{
		ulong_t	vaddr;

		if ((sym->st_shndx == SHN_UNDEF) ||
			(sym->st_shndx == SHN_ABS) ||
			(ELF32_ST_TYPE(sym->st_info) == STT_FILE) ||
			(ELF32_ST_TYPE(sym->st_info) == STT_SECTION))
			continue;
		vaddr = sym->st_value + base;
		if (vaddr > (ulong_t)addr)
			continue;
		if (vaddr == (ulong_t)addr)
		{
			closest = sym;
			break;
		}
		if (((ulong_t)addr - vaddr) < diff)
		{
			closest = sym;
			diff = (ulong_t)addr - vaddr;
		}
	}
	if (closest)
	{
		dlip->dli_saddr = (VOID *)(closest->st_value + base);
		dlip->dli_sname = STRTAB(lm) + closest->st_name;
		dlip->dli_size = closest->st_size;
		dlip->dli_bind = ELF32_ST_BIND(closest->st_info);
		dlip->dli_type = ELF32_ST_TYPE(closest->st_info);
	}
	else
	{
		dlip->dli_saddr = 0;
		dlip->dli_sname = 0;
		dlip->dli_size = 0;
		dlip->dli_bind = STB_LOCAL;
		dlip->dli_type = STT_NOTYPE;
	}
	return 1;
}
