#ident	"@(#)rtld:i386/binder.c	1.22"


/* function binding routine - invoked on the first call
 * to a function through the procedure linkage table;
 * passes first through an assembly language interface;
 *
 *
 * Takes the addres of the rt_map structure for the object in
 * which the call originated and the offset into the PLTGOT
 * relocation table of the relocation entry associated with the call.
 *
 * Returns the address of the function referenced after
 * re-writing the GOT entry to invoke the function
 * directly.
 * 
 * On error, causes process to terminate with a SIGKILL
 *
 * We lock around code that needs to access or modify the
 * link_map to allow for simultaneous access by multiple threads
 * or for simultaneous calls to dlopen/dlclose.
 */

#include "machdep.h"
#include "rtld.h"
#include "externs.h"
#include <sys/types.h>
#include <elf.h>
#include "stdlock.h"

ulong_t 
_binder(ref_lm, reloc)
rt_map *ref_lm;
ulong_t reloc;
{
	rt_map		*def_lm, *first_lm;
	char		*symname;
	Elf32_Rel	*rptr;
	Elf32_Sym	*sym, *nsym;
	ulong_t		value = 0;
	ulong_t		*got_addr;

	DPRINTF((LIST|DRELOC),(2, "rtld: _binder(0x%x, 0x%x)\n", reloc, ref_lm));

	if (!ref_lm) 
	{
		_rt_lasterr( 
			"%s: %s: unidentifiable procedure reference\n",
			(char*) _rt_name,_rt_proc_name);
		_rt_fatal_error();
	}
	
	/* use relocation entry to get symbol table entry and 
	 * symbol name 
	 */
	first_lm = TEST_FLAG(ref_lm, RT_SYMBOLIC) ? ref_lm : 0;
	rptr = NEXT_REL(JMPREL(ref_lm), reloc);
	sym = NEXT_SYM(SYMTAB(ref_lm),
		(ELF32_R_SYM(rptr->r_info) * SYMENT(ref_lm)));
	symname = NEWPTR(char, STRTAB(ref_lm), sym->st_name);

	STDLOCK(&_rtld_lock); 

	/* find definition for symbol */
	nsym = _rt_lookup(symname, first_lm, _rt_map_head, ref_lm,
		&def_lm, LOOKUP_NORM);
	
	if (nsym == 0)
	{
		_rt_lasterr("%s: %s: symbol not found: %s\n",
			(char *)_rt_name, _rt_proc_name, symname);
		goto out;
	}

	/* get definition address and reset GOT entry */
	value = nsym->st_value;
	if (NAME(def_lm))
		value += ADDR(def_lm);

	DPRINTF(DRELOC,(2, "rtld: relocating function %s to 0x%x\n",
		symname, value));

	got_addr = (ulong_t *)rptr->r_offset;
	if (NAME(ref_lm))
		got_addr = (ulong_t *)((ulong_t)got_addr + ADDR(ref_lm));
	*got_addr = value;
	DPRINTF(DRELOC,(2, "got_addr = 0x%x, *got_addr = 0x%x\n", 
		got_addr, *got_addr));
	if (ref_lm != def_lm) 
	{
		/* add to list of referenced objects */
		if (!_rt_add_ref(ref_lm, def_lm)) 
		{
			_rt_lasterr("%s: %s: %s\n", _rt_name,
				_rt_proc_name, _rt_error);
			value = 0;
			goto out;
		}
	}
out:
	STDUNLOCK(&_rtld_lock); 
	if (!value)
		_rt_fatal_error();
	return(value);
}
