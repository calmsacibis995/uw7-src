#ident	"@(#)rtld:i386/reloc.c	1.17"

/* i386 specific routines for performing relocations */

#include "machdep.h"
#include "rtld.h"
#include "externs.h"
#include <dlfcn.h>
#include <sys/types.h>
#include <elf.h>
#include <sys/elf_386.h>
#include <sgs.h>

/* read and process the relocations for one link object 
 * we assume all relocation sections for loadable segments are
 * stored contiguously in the file
 *
 * accept a pointer to the rt_map structure for a given object and
 * the binding mode: if RTLD_LAZY, then don't relocate procedure
 * linkage table entries; if RTLD_NOW, do.
 */

static int do_reloc ARGS((rt_map *, Elf32_Rel *, ulong_t, int));
static Elf32_Phdr *find_segment ARGS((rt_map *, ulong_t));

int 
_rt_relocate(lm, mode)
rt_map	*lm;
int	mode;
{

	DPRINTF((LIST|DRELOC),(2, "rtld: _rt_relocate(%s, %d), lm::0x%x\n",
		(CONST char *)((NAME(lm) ? NAME(lm) : "a.out")), mode, lm));
	
	/* if lazy binding, initialize first procedure linkage
	 * table entry to go to _rtbinder
	 */
	if (PLTGOT(lm)) 
	{
		register ulong_t	*got_addr;
		register ulong_t	base_addr;
		register Elf32_Rel	*rel;
		register Elf32_Rel	*rend;

		if (mode & RTLD_LAZY) 
		{
			/* fix up the first few GOT entries
			 *	GOT[GOT_XLINKMAP] = the address of the link map
			 *	GOT[GOT_XRTLD] = the address of rtbinder
			 */
			got_addr = PLTGOT(lm) + GOT_XLINKMAP;
			*got_addr = (ulong_t)lm;
	DPRINTF((LIST|DRELOC),(2, "got_addr = 0x%x *got_addr = 0x%x\n", got_addr, *got_addr));
			got_addr = PLTGOT(lm) + GOT_XRTLD;
			*got_addr = (ulong_t)_rtbinder;
	DPRINTF((LIST|DRELOC),(2, "got_addr = 0x%x *got_addr = 0x%x\n", got_addr, *got_addr));

			/* if this is a shared object, we have to step
			 * through the plt entries and add the base address
			 * to the corresponding got entry
			 */
			if (NAME(lm)) 
			{
				base_addr = ADDR(lm);
				rel = JMPREL(lm);
				rend = NEXT_REL(rel, PLTRELSZ(lm));
		DPRINTF((LIST|DRELOC),(2, "%s:%d: rel::0x%x\n", __FILE__, __LINE__, rel));

				for ( ; rel < rend; ++rel) 
				{
					got_addr = (ulong_t *)((char *)rel->r_offset + base_addr);	
					*got_addr += base_addr;
				}
			}
		}
		else
		{
			if (do_reloc(lm, JMPREL(lm), PLTRELSZ(lm), 
				LOOKUP_NORM) == 0)
				return 0;
			SET_FLAG(lm, RT_LAZY_PROCESSED);
		}
	}

	if (RELSZ(lm) && REL(lm))
		return do_reloc(lm, REL(lm), RELSZ(lm), LOOKUP_SPEC);
	return 1;
}

/* relocate delayed relocation sections on demand from C++ run-time;
 * kills process on relocation error
 */
void
_rt_process_delayed_relocations(lm)
rt_map	*lm;
{
	DPRINTF((LIST|DRELOC),(2, "rtld: _rt_process_delayed_relocations(0x%x)\n",
		lm));
	if (DELAY_REL(lm) && DELAY_RELSZ(lm) &&
		!TEST_FLAG(lm, RT_DELAY_REL_PROCESSED))
	{
		if (!do_reloc(lm, DELAY_REL(lm), 
			DELAY_RELSZ(lm), LOOKUP_SPEC))
		{
			_rt_fatal_error();
		}
		SET_FLAG(lm, RT_DELAY_REL_PROCESSED);
	}
}

/* process lazy binding relocations for a given object -
 * invoked by dlopen for an object origionally loaded
 * with lazy binding, and subsequently loaded with immediate
 * binding
 */
int
_rt_process_lazy_bindings(lm)
rt_map *lm;
{
	if (PLTGOT(lm) && !TEST_FLAG(lm, RT_LAZY_PROCESSED))
	{
		if (do_reloc(lm, JMPREL(lm), PLTRELSZ(lm), 
			LOOKUP_NORM) == 0)
			return 0;
		SET_FLAG(lm, RT_LAZY_PROCESSED);
	}
	return 1;
}

static int
do_reloc(lm, reladdr, relsz, flag)
rt_map		*lm;
ulong_t		relsz;
int		flag;
register Elf32_Rel	*reladdr;
{
	ulong_t			baseaddr, stndx;
	register ulong_t	off;
	register uint_t 	rtype;
	register Elf32_Rel	*rend;
	long			value;
	Elf32_Sym		*symref, *symdef;
	char			*name;
	int			is_rtld = 0;
	struct rel_copy		 *rcpy;
	rt_map			*def_lm, *first_lm, *list_lm;

	DPRINTF((LIST|DRELOC),(2, "rtld: do_reloc(%s, 0x%x, 0x%x)\n",
		(CONST char *)(NAME(lm) ? NAME(lm) : "a.out"), reladdr, relsz));
	
	if (lm == _rtld_map)
		is_rtld = 1;
	baseaddr = ADDR(lm);
	rend = NEXT_REL(reladdr, relsz);

	/* loop through relocations */
	for ( ; reladdr < rend; ++reladdr) 
	{
		rtype = ELF32_R_TYPE(reladdr->r_info);
		if (rtype == R_386_NONE)
			continue;

		off = (ulong_t)reladdr->r_offset;

		/* if not a.out, add base address to offset */
		if (NAME(lm))
			off += baseaddr;

		/* if R_386_RELATIVE, simply add base addr 
		 * to reloc location 
		 */
		if (rtype == R_386_RELATIVE)
		{
			/* if rtld, RELATIVE relocs were handled in startup */
			if (!is_rtld)
				*(ulong_t *)off += baseaddr;
			continue;
		}
		/* get symbol table entry - if symbol is local,
		 * value is base address of this object
		 */
		stndx = ELF32_R_SYM(reladdr->r_info);
		symref = NEXT_SYM(SYMTAB(lm), (stndx * SYMENT(lm)));
		/* if local symbol, just add base address 
		 * we should have no local relocations in the a.out
		 */
		if (ELF32_ST_BIND(symref->st_info) == STB_LOCAL)
		{
			value = baseaddr;
		}
		else
		{
			/* global or weak 
		 	 * lookup symbol definition - error 
			 * if name not found and reference was 
			 * not to a weak symbol - weak 
			 * references may be unresolved
			 */
		
			name = NEWPTR(char, STRTAB(lm), symref->st_name);
			DPRINTF(DRELOC,(2, "rtld: relocating %s\n",name));
			first_lm = 0;
			if (rtype == R_386_COPY) 
			{
				/* don't look in the a.out */
				list_lm = (rt_map *)NEXT(_rt_map_head);
			} 
			else 
			{
				list_lm = _rt_map_head;
				if (TEST_FLAG(lm, RT_SYMBOLIC))
				/* look in the current object first */
					first_lm = lm;
			}
					
			if ((symdef = _rt_lookup(name, first_lm, 
				list_lm, lm, &def_lm, flag))
				== (Elf32_Sym *)0)
			{
				if (ELF32_ST_BIND(symref->st_info)
					== STB_WEAK)
					/* undefined weak reference */
					continue;

				/* undefined */
				if (CONTROL(CTL_WARN)) 
				{
					_rtfprintf(2, "%s: %s: relocation error: symbol not found: %s\n",(char *)_rt_name, _rt_proc_name,name);
					continue;
				}
				else 
				{
					_rt_lasterr("%s: %s: relocation error: symbol not found: %s",(char *)_rt_name, _rt_proc_name, name);
					return(0);
				}
			}
			/* symbol found  - relocate */
			/* calculate location of definition 
			 * - symbol value plus base address of
			 * containing shared object
			 */
			value = symdef->st_value;
			if (NAME(def_lm) && 
				(symdef->st_shndx != SHN_ABS))
				value += ADDR(def_lm);

			/* for R_386_COPY, just make an entry 
			 * in the rt_copy_entries array
			*/
			if (rtype == R_386_COPY) 
			{
				if (_rtstrcmp(name, "__ctype") == 0)
				/* copy relocation exists
				 * for ctype - we do not
				 * need to do special copy
				 */
					CLEAR_CONTROL(CTL_COPY_CTYPE);
				if ((rcpy = (struct rel_copy *) 
					_rtmalloc(sizeof(struct rel_copy))) == 0) 
				{
					if (!CONTROL(CTL_WARN))
						return(0);
					else
						continue;
				}
				if (symdef->st_size != symref->st_size)
				{
					if (symdef->st_size > symref->st_size)
					{
					/* source bigger than 
					 * destination - this might
					 * be a candidate for special
					 * processing:
					 * if source definition comes 
					 * from a read only
					 * segment and target definition
					 * is in .bss,
				 	 * we will later redirect 
					 * references to this symbol
					 * to the shared library source
					 * rather than
				 	 * the a.out target
					 */
						Elf32_Phdr *sphdr, *dphdr;
						sphdr = find_segment(def_lm, value);
						dphdr = find_segment(_rt_map_head, off);
						if ((sphdr && 
						!(sphdr->p_flags & PF_W)) 
						&& (dphdr && 
						(dphdr->p_memsz != 
						dphdr->p_filesz)))
							rcpy->r_special = 1;
						rcpy->r_size = 
							symref->st_size;
						DPRINTF(DRELOC,(2, "rtld: special copy reloc %s\n",name));
					}
					else
						rcpy->r_size = 
							symdef->st_size;
					if (!rcpy->r_special)
						_rtfprintf(2, "%s: %s: warning: copy relocation size mismatch for symbol %s\n",
						_rt_name, _rt_proc_name, name);
				} 
				else 
				{
					rcpy->r_size = 
						symdef->st_size;
				}
				rcpy->r_to = (char *)off;
				rcpy->r_from = (char *) value;
				DPRINTF(DRELOC,(2, "rtld: rt_copy_entries: r_to is %x, r_from is %x\n",rcpy->r_to, rcpy->r_from));
				rcpy->r_next = _rt_copy_entries;
				_rt_copy_entries = rcpy;
				continue;
			} /* end R_386_COPY */
					
			if (lm != def_lm) 
			{
				SET_FLAG(def_lm, RT_REFERENCED);
				if ((ELF32_ST_TYPE(symdef->st_info)
					== STT_OBJECT) 
					&& !NAME(def_lm)) 
				{
					/* reference from
					 * shared obj to an object
					 * defined in a.out.
					 * If this object is on
					 * copy reloc list and
					 * is marked for special
					 * processing, direct
					 * ref to original shared
					 * object source of copy
					 * instead of a.out
					 */
					for (rcpy =_rt_copy_entries; 
						 rcpy; rcpy = rcpy->r_next)
					{
						if ((value == 
						  (long)rcpy->r_to) && 
						  rcpy->r_special) 
						{
							value = 
							(long)rcpy->r_from;
                                                        DPRINTF(DRELOC, (2,"rtld: found a special match in the _rt_copy_entries; setting value to 0x%x\n",value));
							break;
						}
					}
				}
			}
                        if ((ELF32_ST_TYPE(symdef->st_info)
                        	== STT_OBJECT) && NAME(def_lm)
				&& NAME(lm))
			{
				/* reference resolved to an object
				 * defined in a shlib; if this object
				 * is a synonym (defined at same
				 * address) for an object aleady
				 * copy-relocated, redirect reference
				 * to copy in a.out - make sure all
				 * synonyms reference the same object
				 */
                       		for (rcpy =_rt_copy_entries; 
					rcpy; rcpy = rcpy->r_next)
				{
                               		if (value == 
					  (long)rcpy->r_from)
					{
                                        	value = (long)rcpy->r_to;
                                                DPRINTF(DRELOC, (2,"rtld: found a match in the _rt_copy_entries; setting value to 0x%x\n",value));
						break;
					}
				}
			}

			/* calculate final value - 
			* if PC-relative, subtract ref addr
			*/
			if (PCRELATIVE(rtype))
				value -= off;
					
		} /* end global or weak */

		DPRINTF(DRELOC,(2,"rtld: sym value is 0x%x, offset is 0x%x\n",value, off));

		/* insert value calculated at reference point */
		/* two cases - we either take the current contents
		 * of the location referenced by off, add that
		 * to the value, and store it, or we just store
		 * the value
		 */
		switch(rtype) 
		{
		case R_386_GLOB_DAT:  /* word aligned */
		case R_386_32:	/* unaligned */
		case R_386_RELATIVE:	/* handled above */
		case R_386_PC32:
			value += *(ulong_t *)off;
			*(ulong_t *)off = value;
			break;
		case R_386_JMP_SLOT: 
			/* for plt-got do not add ref contents */
			*(ulong_t *)off = value;
			break;
		default:
			if (CONTROL(CTL_WARN))
				_rtfprintf(2, "%s: %s: invalid relocation type %d at 0x%x\n",(char *)_rt_name, _rt_proc_name, rtype, off);
			else 
			{
				_rt_lasterr("%s: %s: invalid relocation type %d at 0x%x",(char *)_rt_name, _rt_proc_name, rtype, off);
				return(0);
			}
			break;
		}
	}
	return(1);
}

/* find program header entry for a particular object/address
 * combination
 */
static Elf32_Phdr *
find_segment(lm, addr)
rt_map		*lm; 
ulong_t		addr;
{
	register Elf32_Phdr *phdr = (Elf32_Phdr *)PHDR(lm);
	size_t		phsize = PHSZ(lm);
	ulong_t		base = NAME(lm) ? ADDR(lm) : 0;
	int		i;

	for(i = PHNUM(lm); i > 0; i--,
		phdr = NEXT_PHDR(phdr, phsize))
	{
		register ulong_t	baddr;
		if (phdr->p_type != PT_LOAD)
			continue;
		baddr = phdr->p_vaddr;
		if (base)
			baddr += base;
		if ((addr >= baddr) && (addr < (baddr + phdr->p_memsz)))
			return phdr;
	}
	return 0;
}
