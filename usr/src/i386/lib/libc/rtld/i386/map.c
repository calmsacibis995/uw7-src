#ident	"@(#)rtld:i386/map.c	1.46"

#include "machdep.h"
#include "rtld.h"
#include "externs.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <elf.h>
#include <unistd.h>
#include <elfid.h>

/*
** Useful Macros
*/

#define set_prot(phdr_ptr, prot) \
	prot = 0; \
	if (phdr_ptr->p_flags & PF_R)\
		prot |= PROT_READ;\
	if (phdr_ptr->p_flags & PF_W)\
		prot |= PROT_WRITE;\
	if (phdr_ptr->p_flags & PF_X)\
		prot |= PROT_EXEC


/* load a single shared object or the a.out */
rt_map *
_rt_map_so(obj_id, entry, runpath, allow_foreign)
CONST object_id	*obj_id;
ulong_t		*entry;
char		**runpath;
int		allow_foreign;
{
	int	fd;
	int	hsize;		/* size of area mapped for headers */
	int	phsize;		/* size really needed for program headers */
	int	vmem_size;	/* amount of virtual space needed
		   	 	 * to load all loadable segments */
	int	firstprot;	
	int	map_criteria; 	/* map modes required for _mmap */

	int	vaddr_offset_diff; /* difference between fileoffset and 
			      	    * vaddr for first loadable segment */
	ulong_t	addr_offset;

	int	phentsz, index;
	ulong_t	file_offset, phoff;
	ulong_t	phdr_mapped;
	caddr_t	base_address;	/* start address where object is mapped*/
	rt_map 	*lm;	/* link map for the so */

	CONST char	*soname;
	Elf32_Ehdr	*ehdr;
	Elf32_Phdr	*text_phdr;	
	Elf32_Phdr	*first_loadable;
	Elf32_Phdr	*last_loadable;
	Elf32_Phdr	*phdr;
	Elf32_Phdr	*pptr;	
	Elf32_Phdr	*savephdr = 0;	/* seg containing phdr */
	Elf32_Dyn	*mld;
	ulong_t		phdr_end;
	int		phdr_sep = 0;
#ifndef GEMINI_ON_UW2
	Elf32_Phdr	*notep = 0;
#endif

	DPRINTF(LIST,(2,"rtld: _rt_map_so(0x%x)\n",obj_id));

	if (obj_id->n_name == (char *)0)
	{
		/* the object is the a.out */
		soname = _rt_proc_name;
	} 
	else
	{
		soname = obj_id->n_name;
	}

	fd = obj_id->n_fd;

	/* map in enough for elf header and 8 program headers
	 * (if they are contiguous)
	 */
	hsize = sizeof(Elf32_Ehdr) + (8 * sizeof(Elf32_Phdr));
	hsize = PROUND(hsize);
	ehdr = (Elf32_Ehdr *)_rtmmap(0, hsize, PROT_READ, MAP_SHARED, 
		fd, 0);
	if (ehdr == (Elf32_Ehdr *)(caddr_t)-1)
	{
		_rt_lasterr(
			"%s: %s: Cannot map elf header for file %s",
			_rt_name, _rt_proc_name, soname);
		return 0;
	}

	/*  Verify information in file header */

        /* check ELF identifier */
        if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
		ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
		ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
		ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		_rt_lasterr(
			"%s: %s: %s is not an ELF file",
			_rt_name, _rt_proc_name, soname);
		return 0;
	}

	/* check class and encoding */
	if (ehdr->e_ident[EI_CLASS] != M_CLASS ||
		ehdr->e_ident[EI_DATA] != M_DATA) 
	{
		_rt_lasterr(
			"%s: %s: %s has wrong class or data encoding",
			_rt_name, _rt_proc_name, soname);
		return 0;
	}
        /* check magic number */
        if (obj_id->n_name == 0)
	{
                if (ehdr->e_type != ET_EXEC) 
		{
			_rt_lasterr(
				"%s: %s: %s not an executable file",
				_rt_name, _rt_proc_name, soname);
			return 0;
		}
	}
	else 
	{ 
		/* shared object */
                if (ehdr->e_type != ET_DYN) 
		{
			_rt_lasterr( 
				"%s: %s: %s not a shared object",
				_rt_name, _rt_proc_name, soname);
                        return 0;
                }
	}

	/* check machine type */
	if (ehdr->e_machine != M_MACH) 
	{
		_rt_lasterr(
			"%s: %s: bad machine type for file: %s",
			_rt_name, _rt_proc_name, soname);
		return 0;
	}
	/* verify ELF version */
	/* ??? is this too restrictive ??? */
	if (ehdr->e_version > EV_CURRENT) 
	{
		_rt_lasterr(
			"%s: %s: bad file version for file: %s",
			_rt_name, _rt_proc_name, soname);
		return 0;
	}

	/* check the case where the phdr is not contiguous with
	 * the file header or where we did not get all of the phdr
	 */
	phsize = ehdr->e_phnum * ehdr->e_phentsize;
	phoff = ehdr->e_phoff;
	phdr_end = phoff + phsize;
	if (phdr_end > hsize) 
	{
		off_t	toff = PTRUNC(phoff);
		size_t	delta = phoff - toff;
		phsize += delta;
		phdr_sep = 1;  /* we will unmap the file header later */

		phdr_mapped = (ulong_t)_rtmmap(0, phsize, PROT_READ, 
			MAP_SHARED, fd, toff);
		if (phdr_mapped == (ulong_t)-1)
		{
			_rt_lasterr(
				"%s: %s: Cannot map program headers for file %s",
				_rt_name, _rt_proc_name, soname);
			return 0;
		}
		phdr = NEXT_PHDR(phdr_mapped, delta);
	}
	else
		phdr = NEXT_PHDR(ehdr, phoff);

	DPRINTF(MAP, (2, "phnum = %d phoff = %d phentsize = %d\n",
		ehdr->e_phnum, ehdr->e_phoff, ehdr->e_phentsize));

	/* traverse the program header information and determine
	* if any loadable segments exist and find the first and 
	* last loadable segment.
	*/
	pptr = phdr;
	first_loadable = 0;
	text_phdr = 0;
	phentsz = ehdr->e_phentsize;
	for (index = (int)ehdr->e_phnum; index > 0; index--,
		pptr = NEXT_PHDR(pptr, phentsz))
	{
                if (pptr->p_type == PT_LOAD) 
		{
                        if ((text_phdr == 0) && 
				(((pptr->p_flags & (PF_R|PF_W|PF_X))
				== (PF_R|PF_X))))
			{
				text_phdr = pptr;
                        }
                        if (first_loadable == 0) 
			{
                                first_loadable = pptr;
                        }
                        else if (pptr->p_vaddr <= last_loadable->p_vaddr) 
			{
                                _rt_lasterr(
					"%s: %s: invalid program header - segments out of order: %s",
                                        (char*) _rt_name,_rt_proc_name,soname);
                                return 0;
                        }
                        last_loadable = pptr;
                }
                else if (pptr->p_type == PT_DYNAMIC)
                        mld = (Elf32_Dyn *)(pptr->p_vaddr);

#ifndef GEMINI_ON_UW2
#ifdef GEMINI_ON_OSR5
		else if ((pptr->p_type == PT_NOTE) &&
			CONTROL(CTL_NOTELESS_SHUNT))
#else
		else if ((pptr->p_type == PT_NOTE) &&
			(pptr->p_filesz == NT_ELFID_SZ))
#endif
		{
			/* make sure we don't try to load an
			 * OpenServer 5 object
			 */
			 notep = pptr;
		}
#endif /* !GEMINI_ON_UW2 */
        } /* end of for */

#ifndef GEMINI_ON_UW2
	if (!allow_foreign && (ehdr->e_flags != UDK_FIX_FLAG) &&
		(notep || (ehdr->e_flags == OSR5_FIX_FLAG)))
	{
		if (phdr_sep)
			_rtmunmap((caddr_t)phdr_mapped, phsize);
		_rtmunmap((caddr_t)ehdr, hsize);
		_rtclose(fd);
		_rt_lasterr("%s: %s: UDK executable cannot load OpenServer shared object: %s\n",
			(char *)_rt_name, _rt_proc_name, soname);
		return 0;
	}
#endif

	if (!first_loadable)
	{
		_rt_lasterr(
			"%s: %s: no loadable segments in %s",
			_rt_name, _rt_proc_name, soname);
		return 0;
	}

	/* Compute the amount of virtual address space that we need to map:
	* the difference between end of the last loadable segment
	* and the start of the first one (everything is rounded or
	* truncted as appropriate at page boundaries). We map in that 
	* much amount of space from 'fd' to make sure we have the required
	* amount of contiguous virtual space. We map it in with the
	* protections and fileoffset and vaddr offset constraints of the
	* first loadable segment, thus ensuring that the first segment is
	* loaded correctly. For all subsequent segments that do not have the 
	* same file offset and vaddr difference as the first segment
	* we remap appropriate portions from fd. For all subsequent segments
	* that have the same file offset and vaddr difference we just 
	* set the protections. Note: in some cases
	* the amount of virtual space needed may be larger than the filesize,
	* because the library does not have the bss section. In that case 
	* if the bss section extends beyond a page boundary we will 
	* map in /dev/zero to get zero filled pages mapped to bss.
	*/

	vmem_size = last_loadable->p_vaddr + last_loadable->p_memsz 
		   - STRUNC(first_loadable->p_vaddr);
	file_offset = STRUNC(first_loadable->p_offset);
	vaddr_offset_diff = first_loadable->p_vaddr - first_loadable->p_offset;

	if (ehdr->e_type == ET_DYN)
	{
		base_address = 0;
		map_criteria = MAP_PRIVATE;
	} 
	else 
	{
		/* The a.out is mapped at the exact address specified
		 * by the p_vaddr field
		 */
		base_address = (caddr_t)STRUNC(first_loadable->p_vaddr);
		map_criteria = MAP_PRIVATE|MAP_FIXED;
	}
	set_prot(first_loadable, firstprot);
	base_address = _rtmmap(base_address, vmem_size, firstprot,
			   map_criteria, fd, file_offset);
	if (base_address == (caddr_t)-1)
	{
		_rt_lasterr(
			"%s: %s: Cannot map from file %s",
			_rt_name, _rt_proc_name, soname);
		return 0;
	}
	if ((phoff >= file_offset) &&
		(phdr_end < (first_loadable->p_offset + first_loadable->p_filesz))) 
	{
		/* phdr is in first loadable segment */
		savephdr = NEXT_PHDR(base_address, (phoff - file_offset));
	}
	DPRINTF(MAP,(2,"Mapped %x bytes at addr = %x from offset = %x in file %s\n", 
		vmem_size, base_address, file_offset, soname));	

	addr_offset = (ehdr->e_type == ET_DYN) ? (int)base_address : 0;
	/* all mappings fixed from now on */
	map_criteria = MAP_PRIVATE|MAP_FIXED; 
	pptr = phdr;
	for(index = ehdr->e_phnum; index > 0; index--,
		pptr = NEXT_PHDR(pptr, phentsz))
	{
		ulong_t	prot;
		ulong_t	extra_read = 0;
		ulong_t	toff;
		ulong_t	baddr;
		ulong_t	taddr;
		int	retval;

		/* check if we need to map anything from the file*/
		if (pptr->p_type != PT_LOAD || pptr == first_loadable)
			continue;

		/* map from file only if difference between
		* vaddr and file offset are different from
		* the first loadable segment else just set
		* the protections
		*/
		baddr = pptr->p_vaddr + addr_offset;
		taddr = STRUNC(baddr);

		extra_read = baddr - taddr;
		toff = pptr->p_offset - extra_read;
		set_prot(pptr, prot);

		if (!savephdr && (phoff >= toff) &&
			(phdr_end < (pptr->p_offset + pptr->p_filesz))) 
		{
			savephdr = NEXT_PHDR(taddr, (phoff - toff));
		}
		if ((pptr->p_vaddr - pptr->p_offset) != vaddr_offset_diff)
		{
			retval = (int)_rtmmap((caddr_t)taddr,
				pptr->p_filesz + extra_read,
				prot, map_criteria, fd, toff);
			if (retval == -1)
			{
				_rt_lasterr(
					"%s: %s: Cannot map segment %d for file %s",
					_rt_name, _rt_proc_name,
					ehdr->e_phnum - index, soname);
				return 0;
			}
			DPRINTF(MAP,(2,
				"Remapped seg %d, size = %x offset = %x\n", 
				ehdr->e_phnum - index,
				pptr->p_filesz + extra_read, toff));

		} 
		else if (prot != firstprot)
		{
			retval = _rtmprotect((caddr_t)taddr,
				pptr->p_filesz + extra_read, prot);
			if (retval == -1)
			{
				_rt_lasterr(
				"%s: %s: cannot set protections file %s",
					_rt_name,_rt_proc_name,soname);
				   return 0;
			}
			DPRINTF(MAP, (2, 
			"Changing protection of seg# %d, size = %x\n",
				ehdr->e_phnum - index,
				pptr->p_filesz));
		}

		/* If memsize is greater than filesize then we
		* will have to make sure we have zero filled 
		* memory for the extra virtual space. For the
		* extra virtual space within the last page
		* boundary we just zero fill it by _rtclear.
		* For the virtual space beyond the last page
		* we map in '/dev/zero' because we may not
		* have valid mappings from 'fd' for these pages.
		*/
		if (pptr->p_memsz > pptr->p_filesz)
		{
			ulong_t	file_addr;
			ulong_t	rfile_addr;
			ulong_t	maddr;
			int	zcnt;

			file_addr = baddr + pptr->p_filesz;
			/* extent to which valid mapping exists */
			rfile_addr = PROUND(file_addr);
			maddr = baddr + pptr->p_memsz;

			/* See if we need any extra pages */
			if (rfile_addr < maddr)
			{
				DPRINTF(MAP, (2,
"mapping extra zero-filled pages for bss; from = 0x%x, size = 0x%x\n",
					rfile_addr, 
					(maddr - rfile_addr)));

				if (_rt_map_zero(rfile_addr,
					(maddr - rfile_addr),
					map_criteria, 0) == (char *)-1)
				{
					return 0;
				}
			}
			/* zero out last page which was mapped from fd */
			zcnt = rfile_addr - file_addr;
			if (zcnt > 0)
				_rtclear((caddr_t)file_addr, zcnt);

			DPRINTF(MAP, (2,
				"rtld: zero filled 0x%x bytes at 0x%x\n", 
				zcnt,file_addr));

			/* return any unused virtual space to the
			* rtld storage allocator. Note this space
			* is usually from the bss and has write permission
			* so can be used by the allocator. Strictly we
			* we should check for write permission and only
			* then let the allocator use the space.
			*/
			if (CONTROL(CTL_NO_DELETE) &&
				(obj_id->n_name != 0))
			{
				_rtmkspace((char *)maddr,
					PROUND(maddr) - maddr);
				DPRINTF(MAP, (2, 
			"rtld: returning 0x%x bytes from 0x%x to the allocator\n",
					PROUND(maddr) - maddr, maddr));
			}
		}
	} /* for */

	if (_rtclose(fd) < 0)
	{
		_rt_lasterr(
			"%s: %s: cannot close %s",
			_rt_name, _rt_proc_name, soname);
		return 0;
	}

	if (entry)
		*entry = (ulong_t)ehdr->e_entry + addr_offset;
	mld  = NEWPTR(Elf32_Dyn, mld, addr_offset);

	/* create and return new rt_map structure */
	if (savephdr)
		phdr = savephdr;

	lm = _rt_new_lm(obj_id, mld, (ulong_t)base_address, 
			vmem_size, phdr, ehdr->e_phnum, 
			ehdr->e_phentsize, text_phdr, runpath);
	if (!lm)
		return 0;

	if (savephdr) 
	{
		if (phdr_sep)
			_rtmunmap((caddr_t)phdr_mapped, phsize);
		_rtmunmap((caddr_t)ehdr, hsize);
	}
	else if (phdr_sep) 
	{
		_rtmunmap((caddr_t)ehdr, hsize);
	}

	if (TEST_FLAG(lm, RT_TEXTREL)) 
	{
		if (_rt_set_protect(lm, PROT_WRITE) == 0) 
		{
			_rt_cleanup(lm);
			return 0;
		}
	}
	return(lm);
}

/* create a new rt_map structure and initialize all values. */

rt_map *
_rt_new_lm(obj_id, ld, addr, msize, phdr, phnum, phsize, 
	text_phdr, runpath)
CONST object_id	*obj_id;
Elf32_Dyn	*ld;
ulong_t		addr, msize, phnum, phsize;
Elf32_Phdr	*phdr;
Elf32_Phdr	*text_phdr;
char		**runpath;
{
	register rt_map		*lm;
	register ulong_t	offset;
	ulong_t			rpath = 0;

	DPRINTF(LIST,(2, "rtld: _rt_new_lm(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, %d, %d, 0x%x)\n",
		obj_id,(ulong_t)ld, addr,msize,
		(ulong_t)phdr,phnum,phsize,runpath));

	if (obj_id->n_lm)
		lm = obj_id->n_lm;
	else
	{
		/* allocate space */
		if ((lm = (rt_map *)_rtalloc(rt_t_pm)) == 0)
			return 0;
	}

	/* all fields not filled in were set to 0 by _rtmalloc */
	if ((NAME(lm) = obj_id->n_name) != 0)
	{
		INO(lm) = obj_id->n_ino;
		DEV(lm) = obj_id->n_dev;
		offset = addr;
	}
	else
		offset = 0;

	DYN(lm) = ld;
	ADDR(lm) = addr;
	MSIZE(lm) = msize;
	PHDR(lm) = (VOID *)phdr;
	PHNUM(lm) = (ushort_t)phnum;
	PHSZ(lm) = (ushort_t)phsize;


	/* fill in rest of rt_map entries with info from
	 * the file's dynamic structure
	 * if shared object, add base address to each address;
	 * if a.out, use address as is
	 */

	for (; ld->d_tag != DT_NULL; ++ld) 
	{
		switch (ld->d_tag) 
		{
		case DT_SYMTAB:
			SYMTAB(lm) = NEWPTR(char, ld->d_un.d_ptr, offset);
			break;

		case DT_STRTAB:
			STRTAB(lm) = NEWPTR(char, ld->d_un.d_ptr, offset);
			break;

		case DT_SYMENT:
			SYMENT(lm) = ld->d_un.d_val;
			break;

		case DT_TEXTREL:
			SET_FLAG(lm, RT_TEXTREL);
			break;

	/* at this time we can only handle 1 type of relocation per object */
		case DT_REL:
		case DT_RELA:
			REL(lm) = NEWPTR(char, ld->d_un.d_ptr, offset);
			break;

		case DT_RELSZ:
		case DT_RELASZ:
			RELSZ(lm) = ld->d_un.d_val;
			break;

		case DT_RELENT:
		case DT_RELAENT:
			RELENT(lm) = ld->d_un.d_val;
			break;

		case DT_HASH:
			HASH(lm) = NEWPTR(ulong_t, ld->d_un.d_ptr, offset);
			break;

		case DT_PLTGOT:
			PLTGOT(lm) = NEWPTR(ulong_t, ld->d_un.d_ptr, offset);
			break;

		case DT_PLTRELSZ:
			PLTRELSZ(lm) = ld->d_un.d_val;
			break;

		case DT_JMPREL:
			JMPREL(lm) = NEWPTR(char, ld->d_un.d_ptr, offset);
			break;

		case DT_INIT:
			INIT(lm) = (void (*)())((uchar_t *)ld->d_un.d_ptr + offset);
			break;

		case DT_FINI:
			FINI(lm) = (void (*)())((uchar_t *)ld->d_un.d_ptr + offset);
			break;

		case DT_SYMBOLIC:
			SET_FLAG(lm, RT_SYMBOLIC);
			break;

		case DT_BIND_NOW:
			SET_FLAG(lm, RT_BIND_NOW);
			break;

		case DT_RPATH:
			rpath = ld->d_un.d_val;
			break;

		case DT_DEBUG:
		/* set pointer to debugging information in a.out's
		 * dynamic structure
		 */
			ld->d_un.d_ptr = (Elf32_Addr)&_r_debug;
			break;
		case DT_RANGES:
			RANGES(lm) = NEWPTR(char, ld->d_un.d_ptr, offset);
			break;
		case DT_RANGES_SZ:
			RANGES_SZ(lm) = ld->d_un.d_val;
			break;
		case DT_DELAY_REL:
			DELAY_REL(lm) = NEWPTR(char, ld->d_un.d_ptr, offset);
			break;
		case DT_DELAY_RELSZ:
			DELAY_RELSZ(lm) = ld->d_un.d_val;
			break;
			
		}
	}
	if (text_phdr) 
	{
		TEXTSTART(lm) = text_phdr->p_vaddr + offset;
		TEXTSIZE(lm) = text_phdr->p_memsz;
	}
	if (rpath && runpath)
	{
		*runpath = NEWPTR(char, STRTAB(lm), rpath);
	}
	return(lm);
}

/* function to correct protection settings 
 * segments are all mapped initially with permissions as given in
 * the segment header, but we need to turn on write permissions
 * on a text segment if there are any relocations against that segment,
 * and them turn write permission back off again before returning control
 * to the program.  This function turns the permission on or off depending
 * on the value of the argument
 */

int 
_rt_set_protect(lm, permission)
rt_map	*lm;
int	permission;
{
	register int		i, prot;
	register Elf32_Phdr	*phdr;
	ulong_t			msize, addr;

	DPRINTF(LIST,(2, "rtld: _rt_set_protect(%s, %d)\n",(NAME(lm) ? NAME(lm)
		:"a.out"), permission));

	phdr = (Elf32_Phdr *)PHDR(lm);
	/* process all loadable segments */
	for (i = (int)PHNUM(lm); i > 0; i--,
		phdr = NEXT_PHDR(phdr, PHSZ(lm)))
	{
		if ((phdr->p_type == PT_LOAD) && 
			((phdr->p_flags & PF_W) == 0)) 
		{
			ulong_t	taddr;
			prot = PROT_READ | permission;
			if (phdr->p_flags & PF_X)
				prot |=  PROT_EXEC;
			addr = (ulong_t)phdr->p_vaddr;
			if (NAME(lm))
				addr += ADDR(lm);
			taddr = PTRUNC(addr);
			msize = phdr->p_memsz + (addr - taddr);
			if (_rtmprotect((caddr_t)taddr, msize,
				prot) == -1)
			{
				_rt_lasterr("%s: %s: can't set protections on segment of length 0x%x at 0x%x",(char*) _rt_name, _rt_proc_name,msize, taddr);
				return(0);
			}
		}
	}
	return(1);
}

/* Unmap all segments for a given shared object - to do unmapping
 * we need to read program headers, so be careful to unmap
 * segment containing program headers last.
 * Returns 0 for any unmapping error, else 1.
 */
int
_rt_unmap_so(lm)
rt_map	*lm;
{
	Elf32_Phdr	*phdr;
	int		unmap_later = 0;
	ulong_t		phdr_addr, phdr_msize;
	int		j;
	int		ret = 1;
	mlist		*mptr;
	struct rt_set	*rset;

	phdr = (Elf32_Phdr *)PHDR(lm);
	for (j = PHNUM(lm); j > 0; j--,
		phdr = NEXT_PHDR(phdr, PHSZ(lm)))
	{
		ulong_t	addr, msize, taddr;

		if (phdr->p_type == PT_LOAD) 
		{
			addr = (ulong_t)phdr->p_vaddr + ADDR(lm);
			taddr = PTRUNC(addr);
			msize = phdr->p_memsz + (addr - taddr);
			if (((ulong_t)phdr >= taddr)&& ((ulong_t)phdr <
				(taddr + msize)))
			{
				phdr_addr = taddr;
				phdr_msize = msize;
				unmap_later = 1;
			} 
			else
			{
				if (_rtmunmap((caddr_t)taddr, msize)
					== -1)
					ret = 0;
			}
		}
	}

	/* Now it's safe to unmap the segment containing the phdr */
	if (unmap_later != 0) 
	{
		if (_rtmunmap((caddr_t)phdr_addr, phdr_msize) == -1)
			ret = 0;
	}
	else
	{
		/* phdr was mapped separately - unmap it;
		 * we may not get everything if mapping included
		 * more before the phdr
		 */
		ulong_t tpaddr = PTRUNC((ulong_t)phdr);
		size_t	sz = (PHNUM(lm) * PHSZ(lm)) + 
			((ulong_t)phdr - tpaddr);
		if (_rtmunmap((caddr_t)tpaddr, sz) == -1)
			ret = 0;
	}

	/* free link map and structures it points to */
	mptr = REFLIST(lm);
	while(mptr)
	{
		mlist	*mnext = mptr->l_next;
		_rtfree(mptr, rt_t_ml);
		mptr = mnext;
	}
	mptr = NEEDED(lm);
	while(mptr)
	{
		mlist	*ml = mptr->l_next;
		_rtfree(mptr, rt_t_ml);
		mptr = ml;
	}
	rset = lm->r_grpset.next;
	while(rset)
	{
		struct rt_set	*rn = rset->next;
		_rtfree(rset, rt_t_set);
		rset = rn;
	}
	_rtfree(lm, rt_t_pm);

	return ret;
}
