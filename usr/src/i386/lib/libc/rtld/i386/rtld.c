#ident	"@(#)rtld:i386/rtld.c	1.37"

/* main run-time linking routines 
 *
 * _rtld may be called either from _rt_setup, to get
 * the process going at startup, or from _dlopen, to
 * load a shared object into memory during execution.
 *
 * We read the interface structure: if we have been passed the path
 * name of some file, we load that file into memory and continue
 * further processing beginning with that file; else we begin processing
 * with the current end of the rt_map list.
 *
 * For each shared object listed in the first object's needed list,
 * we open that object and load it into memory.  We then run down
 * the list of loaded objects and perform any needed relocations on each.
 * Finally, we adjust all memory segment protections,
 * and return a pointer
 * to the map structure for the first object we loaded
 * The rt_ret argument is a pointer to an array of at least
 * DT_MAXNEGTAGS Elf32_Dyn entries.
 *
 * If the environment flag LD_TRACE_LOADED_OBJECTS is set, we load
 * all objects, as above, print out the path name of each, and then exit.
 * If LD_WARN is also set, we also perform relocations, printing out a
 * diagnostic for any unresolved symbol.
 */


#include "machdep.h"
#include "rtld.h"
#include "paths.h"
#include "externs.h"
#include <sys/types.h>
#include <elf.h>
#include <dlfcn.h>

struct load_info {
	const char	*name;
	rt_map		*needed_by;
	rt_map		*first_loaded;
	int		group;
	uint_t		is_global;
	short		loaded;
	short		first_for_this_needed;
};

static int		process_object ARGS((struct load_info *));

int 
_rtld(interface, rt_ret)
Elf32_Dyn	*interface;
Elf32_Dyn	*rt_ret;
{
	Elf32_Dyn	*lneed;
	register rt_map	*lm;
	uint_t		mode = RTLD_LAZY;
	struct load_info info;

	DPRINTF(LIST,(2, "rtld: _rtld(0x%x, 0x%x)\n", (ulong_t)interface, 
		(ulong_t)rt_ret));

	info.name = 0;
	info.group = 0;
	for(; interface->d_tag != DT_NULL; interface++)
	{
		switch(interface->d_tag)
		{
		case DT_GROUP:
			/* dlopen group - 0 for startup files */
			info.group = interface->d_un.d_val;
			break;
		case DT_MODE:
			/* binding mode  - default is lazy binding */
			mode = interface->d_un.d_val;
			break;
		case DT_FPATH:
			info.name = (CONST char *)(interface->d_un.d_ptr);
			if (info.name == (char *)0) 
			{
				_rt_lasterr("%s: %s: internal interface error: null pathname specified",
					(char*)_rt_name, _rt_proc_name);
				return 1;
			}
			break;
		default:
			if (interface->d_tag > 0) 
			{
				_rt_lasterr("%s: %s: internal interface error",
					(char*) _rt_name,_rt_proc_name);
				return 1;
			}
			break;
		}
	}

	/* inform debuggers that we are adding to the rt_map */
	_r_debug.r_state = RT_ADD;
	_r_debug_state();

	info.is_global = ((mode & RTLD_GLOBAL) != 0);

	/* if interface contains pathname, map in that file */
	/* used by dlopen */
	if (info.name)
	{
		info.first_loaded = 0;
		info.needed_by = 0;
		if (!process_object(&info))
			return 1;
		if (info.loaded == 0) 
		{
			/* file already loaded;
			 * set up return value:
		    	 * rt_map of already loaded object
		    	 */
			rt_ret[0].d_tag = DT_MAP;
			rt_ret[0].d_un.d_ptr = (Elf32_Addr)info.first_loaded;
			rt_ret[1].d_tag = DT_NULL;
			return 0;
		}
	} 
	else /* no pathname specified - startup */
	{
		info.first_loaded = _rt_map_tail;
		if (!_rt_addset(info.group, _rt_map_tail, info.is_global))
			return 1;
	}

	/* map in all shared objects needed; start with needed
	 * section of first_loaded and map all those in;
	 * then go through needed sections of all objects just mapped,
	 * etc. result is a breadth first ordering of all needed objects
	 */

	for (lm = info.first_loaded; lm; lm = (rt_map *)NEXT(lm)) 
	{
		/* process each shared object on needed list */
		info.needed_by = lm;
		info.first_for_this_needed = 1;
		for (lneed = (Elf32_Dyn *)DYN(lm); 
			lneed->d_tag!=DT_NULL; lneed++) 
		{

			if (lneed->d_tag != DT_NEEDED) 
			{
				continue;
		   	}
			info.name = (CONST char *)STRTAB(lm) +
				lneed->d_un.d_val;
			if (!process_object(&info))
				return 1;
		}
	} 
	if (CONTROL(CTL_TRACING)) 
	{
		/* if LD_WARN not set, exit */
		if (!CONTROL(CTL_WARN))
			_rtexit(0);
	}

	/* for each object just added, call relocate 
	 * to relocate symbols
	 */

	for (lm = info.first_loaded; lm; lm = (rt_map *)NEXT(lm)) 
	{
	 /*
	 * If the binding mode was set at ld time with -Bbind_now
	 * (if the .dynamic section has DT_BIND_NOW set), then RTLD_NOW 
	 * should be the mode and it should take precedence over all others
	 */
				
		rt_map	*lm2;
		if (!_rt_relocate(lm, 
			(TEST_FLAG(lm, RT_BIND_NOW))? RTLD_NOW: mode))
		{
			_rt_cleanup(info.first_loaded);
			return 1;
		}
		/* go through entire list of objects; for
		 * those whose referenced bit has been
		 * set by relocate(), add it to lm's reference
		 * list; then clear bit
		 */
		for (lm2 = _rt_map_head; lm2; lm2 = (rt_map *)NEXT(lm2)) 
		{
			if (TEST_FLAG(lm2, RT_REFERENCED)) 
			{
				if (!_rt_add_ref(lm, lm2)) 
				{
					_rt_cleanup(info.first_loaded);
					return 1;
				}
				CLEAR_FLAG(lm2, RT_REFERENCED);
			}
		}
	}

	/* if tracing, exit */
	if (CONTROL(CTL_TRACING))
		_rtexit(0);

	/* perform special copy type relocations */
	for (;_rt_copy_entries; _rt_copy_entries = 
		_rt_copy_entries->r_next)
		_rt_memcpy(_rt_copy_entries->r_to, 
			_rt_copy_entries->r_from,
			_rt_copy_entries->r_size);
		

	/* set correct protections for each segment mapped */
	for (lm = info.first_loaded; lm; lm = (rt_map *)NEXT(lm)) 
	{
		if (!TEST_FLAG(lm, RT_TEXTREL) || lm == _rtld_map)
			continue;
		if (!_rt_set_protect(lm, 0)) 
		{
			_rt_cleanup(info.first_loaded);
			return 1;
		}
	}

	/* tell debuggers that the rt_map is now in a consistent state */
	_r_debug.r_state = RT_CONSISTENT;
	_r_debug_state();

	/* set up return value  - pointer to first object loaded */
	rt_ret[0].d_tag = DT_MAP;
	rt_ret[0].d_un.d_ptr = (Elf32_Addr)info.first_loaded;
	rt_ret[1].d_tag = DT_NULL;
	return 0;
}

/* add an entry to end of link_map;
 * first entry is always setup by rtsetup: the a.out
 */
static void
add_link_map(lm)
rt_map	*lm;
{
	NEXT(_rt_map_tail) = (struct link_map *)lm;
	PREV(lm) = (struct link_map *)_rt_map_tail;
	_rt_map_tail = lm;
}

/* process an object just loaded - add to link
 * map and needed lists; set new groups 
 */
static int
process_new_object(info, new_lm)
struct load_info	*info;
rt_map			*new_lm;
{
	add_link_map(new_lm);
	info->loaded = 1;
	if (info->needed_by)
	{
		if (!_rt_add_needed(info->needed_by, new_lm)) 
		{
			_rt_cleanup(info->first_loaded);
			return 0;
		}
	}
	else if (!info->first_loaded)
	{
		info->first_loaded = new_lm;
	}
	/* add new group to object */
	if (!_rt_addset(info->group, new_lm, info->is_global))
	{
		_rt_cleanup(info->first_loaded);
		return 0;
	}
	if (CONTROL(CTL_TRACING)) 
	{
		/* ldd processing */
		if (info->first_for_this_needed)
		{
			info->first_for_this_needed = 0;
			_rtfprintf(1, "%s needs:\n",
				NAME(info->needed_by) ? 
				NAME(info->needed_by) : _rt_proc_name);
		}
		if (_rtstrcmp(info->name, NAME(new_lm)) != 0)
		{
			_rtfprintf(1, "\t%s => %s\n",
				info->name, NAME(new_lm));
		}
		else
		{
			_rtfprintf(1, "\t%s\n", NAME(new_lm));
		}
	}
	return 1;
}

/* process an object previously loaded - just
 * add to needed lists and adjust group settings
 */
static int
process_loaded_object(info, loaded_lm)
struct load_info	*info;
rt_map			*loaded_lm;
{
	if (info->needed_by)
	{
		if (!_rt_add_needed(info->needed_by, loaded_lm) ||
			!_rt_setgroup(info->group, loaded_lm, 
				info->is_global))
		{
			_rt_cleanup(info->first_loaded);
			return 0;
		}
	}
	else if (!info->first_loaded)
	{
		info->first_loaded = loaded_lm;
	}
	return 1;
}

/* process a single shared object - if not loaded, map it in;
 * else just adjust needed lists and group memberships accordingly.
 */
static int
process_object(info)
struct load_info	*info;
{
	object_id	obj_id;
	rt_map		*lm;
	static int	mapped_rtld;

	info->loaded = 0;
#ifdef GEMINI_ON_OSR5
	if (_rtstrcmp(info->name, NAME(_rtld_map)+ALT_PREFIX_LEN) == 0) 
#else
	if (_rtstrcmp(info->name, NAME(_rtld_map)) == 0) 
#endif
	{
		if (!mapped_rtld)
		{
			if (!process_new_object(info, _rtld_map))
				return 0;
			mapped_rtld = 1;
			return 1;
		}
		else
			return(process_loaded_object(info, _rtld_map));
	}

	if (_rt_so_find(info->name, &obj_id) == 0)
	{
		if (info->first_loaded)
			_rt_cleanup(info->first_loaded);
		return 0;
	}

	if ((lm = obj_id.n_lm) == _rtld_map)
	{
		if (!mapped_rtld)
		{
			if (!process_new_object(info, _rtld_map))
				return 0;
			mapped_rtld = 1;
		}
		else
			if (!process_loaded_object(info, _rtld_map))
				return 0;
	} 
	else if (lm == 0)
	{
		/* map the object in and put it in the link map
		 */

		if ((lm = _rt_map_so(&obj_id, 0, 0, 0)) == 0)
		{
			if (info->first_loaded)
				_rt_cleanup(info->first_loaded);
			return 0;
		}
		if (!process_new_object(info, lm))
			return 0;
	}
	else
	{
		if (!process_loaded_object(info, lm))
			return 0;
	}
	return 1;
}

/* clean up all attached shared objects beginning with
 * lm in case of error
 */
void
_rt_cleanup(lm)
rt_map	*lm;
{
	if (CONTROL(CTL_NO_DELETE))
		return;

	if (PREV(lm))
	{
		/* break list at previous entry */
		NEXT((rt_map *)PREV(lm)) = 0;
		_rt_map_tail = (rt_map *)PREV(lm);
	}

	/* careful - we are deleting from list as we go */
	while(lm)
	{
		rt_map	*nlm = (rt_map *)NEXT(lm);
		if (TEST_FLAG(lm, RT_NODELETE))
		{
			/* add back to list */
			/* can this happen? */
			NEXT(lm) = 0;
			add_link_map(lm);
		}
		else
		{
			(void)_rt_unmap_so(lm);
		}
		lm = nlm;
	}
}

/* symbol lookup routine - takes symbol name, pointer to a
 * rt_map to search first, a list of rt_maps to search if that fails.
 * If successful, returns pointer to symbol table entry
 * and to rt_map of enclosing object. Else returns a null
 * pointer.
 * 
 * If flag argument is 1, we treat undefined symbols with type
 * function specially in the a.out - if they have a value, even though
 * undefined, we use that value.  This allows us to associate all references
 * to a function's address to a single place in the process: the plt entry
 * for that function in the a.out.  Calls to lookup from plt binding routines
 * do NOT pass a flag value of 1.
 * An object may reference symbols in a second object's symbol table
 * if both objects belong to the same dlopen group or the referenced
 * object belongs to the global group
 */

Elf32_Sym *
_rt_lookup(name, first_lm, lm_list, ref_lm, def_lm, flag)
CONST char	*name;
rt_map	*first_lm;
rt_map	*lm_list;
rt_map	*ref_lm;
rt_map	**def_lm;
int	flag;
{
	register ulong_t	hval;
	register CONST char	*p;
	register ulong_t	g;
	rt_map			*nlm;
	int			first;

	DPRINTF(LIST,(2, "rtld: _rt_lookup(%s, 0x%x, 0x%x 0x%x 0x%x)\n",name, 
		(ulong_t)first_lm, (ulong_t)lm_list, 
		(ulong_t)ref_lm, (ulong_t)def_lm));

	/* hash symbol name - use same hash function used by ELF access
	 * library 
	 */
	p = name;
	hval = 0;
	while (*p) 
	{
		hval = (hval << 4) + *p++;
		if ((g = (hval & 0xf0000000)) != 0)
			hval ^= g >> 24;
		hval &= ~g;
	}

	/* go through each rt_map and look for symbol;
	 * this would be cleaner if lookup work itself were
	 * separate function, but speed is of the essence here 
	 */
	if (first_lm)
	{
		first = 1;
		nlm = first_lm;
	}
	else
	{
		first = 0;
		nlm = lm_list;
	}
	while(nlm)
	{
		if (first || (nlm != first_lm && 
			(_rt_isglobal(nlm) || 
			_rt_ismember(nlm, ref_lm))))
		{
			ulong_t			hash;
			ulong_t			buckets;
			register ulong_t	ndx;
			Elf32_Sym		*sym;
			Elf32_Sym		*symtabptr;
			char			*strtabptr;
			ulong_t			*hashtabptr;

			/*
			 * the form of the hash table is
			 * |--------------|
			 * | # of buckets |
			 * |--------------|
			 * | # of chains  |
			 * |--------------|
			 * | bucket[]	  |
			 * |   ...	  |
			 * |--------------|
			 * | chain[]	  |
			 * |   ...	  |
			 * |--------------|
			 */

			buckets = HASH(nlm)[0];
			hash = hval % buckets;
			/* get first symbol on hash chain */
			ndx = HASH(nlm)[hash + 2];

			hashtabptr = HASH(nlm) + 2 + buckets;
			strtabptr = STRTAB(nlm);
			symtabptr = SYMTAB(nlm);

			while (ndx) 
			{
				sym = symtabptr + ndx;
				if (_rtstrcmp(strtabptr + sym->st_name, 
					name) != 0)
				{
					/* names do not match */
					ndx = hashtabptr[ndx];
				}
				else if ((sym->st_shndx != SHN_UNDEF) ||
					(flag == LOOKUP_SPEC && 
					!NAME(nlm) &&
					sym->st_value != 0 &&
					(ELF32_ST_TYPE(sym->st_info)
					== STT_FUNC)))
				{
					*def_lm = nlm;
					return(sym);
				}
				else 
				{
					/* undefined; try next object */
					break;
				}
			}
		}
		if (first)
		{
			/* now go through list */
			first = 0;
			nlm = lm_list;
		}
		else
		{
			nlm = (rt_map *)NEXT(nlm);
		}
	}
	/* if here, no definition found */
	return 0;
}
