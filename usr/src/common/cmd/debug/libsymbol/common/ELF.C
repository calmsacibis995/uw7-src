#ident	"@(#)debugger:libsymbol/common/ELF.C	1.14"

// provide acces to ELF object files

#include "Object.h"
#include "ELF.h"
#include "Interface.h"
#include "Symtable.h"
#include "Machine.h"
#include <sys/elf.h>
#include <sys/types.h>
#include <fcntl.h>
#include <elfid.h>
#include <unistd.h>
#include <string.h>

#ifdef GEMINI_ON_OSR5
static int foreign_file_has_no_note = 1;
#endif

ELF::ELF( int fd, dev_t dev, ino_t ino, time_t mtime) : OBJECT(fd, 
	dev, ino, mtime)
{
	// read or map all needed sections
	// on failure, set file_form = ff_none;
	Elf_Ehdr	ehdr;
	Elf_Shdr	*shdr, *sptr;
	char		*shnames;
	int		shdrnum;
	int		symindex = 0, strindex = 0, dindex = 0;
	int		lindex = 0, dynindex = 0;
	int		pltindex = 0, gotindex = 0;
	int		interpindex = 0, libindex = 0, shindex;
	int		dynsymindex = 0, dynstrindex = 0;
	int		d2infoindex = 0, d2abbrevindex = 0;
	int		d2lineindex = 0;
	
	if ((lseek(fdobj, 0, SEEK_SET) == -1) ||
		(::read(fdobj, &ehdr, sizeof(Elf_Ehdr)) != sizeof(Elf_Ehdr)))
	{
		printe(ERR_obj_file_read, E_ERROR, sizeof(Elf_Ehdr),
			(unsigned long)0);
		return;
	}
	if (ehdr.e_version != EV_CURRENT)
	{
		printe(ERR_obj_version, E_ERROR, ehdr.e_version);
		return;
	}
	if (ehdr.e_machine != MACHINE)
	{
		printe(ERR_wrong_mach, E_ERROR);
		return;
	}
	if (ehdr.e_type == ET_CORE)
		this->flags |= O_CORE;
	else if (ehdr.e_type == ET_DYN)
		this->flags |= O_SHARED;
	else if (ehdr.e_type != ET_EXEC)
	{
		printe(ERR_obj_file_type, E_ERROR, ehdr.e_type);
		return;
	}

	start = ehdr.e_entry;
	phdrnum = ehdr.e_phnum;
	shdrnum = ehdr.e_shnum;
	// read program header
	void	*p;
	if (phdrnum && (read_data(p, ehdr.e_phoff,
		(phdrnum * ehdr.e_phentsize), 1) !=
		(phdrnum * ehdr.e_phentsize)))
		return;
	phdr = (Elf_Phdr *)p;
	// read note sections for core file or executable
	if (!(this->flags & O_SHARED))
	{
		// Read note sections - might be multiple
		Elf_Phdr	*pptr = phdr;
		int		i;
		Sectinfo	*last = 0, *first = 0;

		for (i = 0; i < phdrnum; i++, pptr++)
		{
			if (pptr->p_type != PT_NOTE)
				continue;

			Sectinfo	*sect = new Sectinfo;
			
			sect->size = pptr->p_filesz;
			sect->vaddr = pptr->p_vaddr;
			sect->offset = pptr->p_offset;
			sect->next = 0;
			if (read_data(sect->data, sect->offset,
				sect->size, 0) != sect->size)
			{
				Sectinfo	*tmp;
				while(first)
				{
					tmp = first;
					last = first->next;
					delete tmp;
				}
				delete sect;
				return;
			}
			if (!first)
				first = last = sect;
			else
			{
				last->next = sect;
				last = sect;
			}
		}
		sections[s_notes] = first;
		if (this->flags & O_CORE)
		{
			file_form = ff_elf;
			return;
		}
	}
	if (!shdrnum)
	{
		file_form = ff_elf;
		return;
	}
	void* tempshdr; // C++ 2.0 workaround
	if (read_data(tempshdr, ehdr.e_shoff,
		(shdrnum * ehdr.e_shentsize), 1) !=
		(shdrnum * ehdr.e_shentsize))
		return;
	shdr = (Elf_Shdr*)tempshdr;
	
	shindex = ehdr.e_shstrndx;  // section header strings
	sptr = &shdr[shindex];
	void* tempshnames; // C++ 2.0 workaround
	if (read_data(tempshnames, sptr->sh_offset, sptr->sh_size, 1) !=
		sptr->sh_size)
	{
		delete shdr;
		return;
	}
	shnames = (char*)tempshnames;

	sptr = shdr + 1; // first header entry null
	for(int j = 1; j < shdrnum; j++, sptr++)
	{
		if (sptr->sh_type == SHT_DYNAMIC)
			dynindex = j;
		else if (sptr->sh_type == SHT_SYMTAB)
		{
			symindex = j;
			strindex = (int)sptr->sh_link;
		}
		else if (sptr->sh_type == SHT_DYNSYM)
		{
			dynsymindex = j;
			dynstrindex = (int)sptr->sh_link;
		}
		else if (sptr->sh_type == SHT_PROGBITS)
		{
			char	*name = shnames + sptr->sh_name;
			if (strcmp(".debug", name) == 0)
				dindex = j;
			else if (strcmp(".line", name) == 0)
				lindex = j;
			else if (strcmp(".interp", name) == 0)
				interpindex = j;
			else if (strcmp(".plt", name) == 0)
				pltindex = j;
			else if (strcmp(".got", name) == 0)
				gotindex = j;
			else if (strcmp(".debug_info", name) == 0)
				d2infoindex = j;
			else if (strcmp(".debug_abbrev", name) == 0)
				d2abbrevindex = j;
			else if (strcmp(".debug_line", name) == 0)
				d2lineindex = j;
		}
		else if (sptr->sh_type == SHT_SHLIB)
			libindex = j;
	}
	delete shnames;

	// read interp, lib and dynamic sections
	if (dynindex)
	{
		sptr = &shdr[dynindex];
		if (!read_section(s_dynamic, sptr, 0))
		{
			delete shdr;
			return;
		}
	}
	if (libindex)
	{
		sptr = &shdr[libindex];
		if (!read_section(s_lib, sptr, 0))
		{
			delete shdr;
			return;
		}
	}
	if (interpindex)
	{
		sptr = &shdr[interpindex];
		if (!read_section(s_interp, sptr, 0))
		{
			delete shdr;
			return;
		}
	}
	if (pltindex)
	{
		sptr = &shdr[pltindex];
		if (!read_section(s_plt, sptr, 1))
		{
			delete shdr;
			return;
		}
	}
	if (gotindex)
	{
		sptr = &shdr[gotindex];
		if (!read_section(s_got, sptr, 1))
		{
			delete shdr;
			return;
		}
	}

	// map symtab, strtab, debug, line
	// we try to get symtab/strab and debug/line
	// in one contiguous chunk
	if (symindex && strindex)
	{
		if (!map_sections(s_symtab, &shdr[symindex],
			s_strtab, &shdr[strindex]))
		{
			delete shdr;
			return;
		}
	}
	else if (dynsymindex && dynstrindex)
	{
		if (!map_sections(s_symtab, &shdr[dynsymindex],
			s_strtab, &shdr[dynstrindex]))
		{
			delete shdr;
			return;
		}
	}
	if (dindex && lindex)
	{
		if (!map_sections(s_debug, &shdr[dindex],
			s_line, &shdr[lindex]))
		{
			delete shdr;
			return;
		}
		this->flags |= O_DEBUG;
	}
	if (d2lineindex)
	{
		sptr = &shdr[d2lineindex];
		if (!read_section(s_dwarf2_line, sptr, 0))
		{
			delete shdr;
			return;
		}
	}

	if (d2infoindex && d2abbrevindex)
	{
		if (!map_sections(s_dwarf2_info, &shdr[d2infoindex],
			s_abbreviation, &shdr[d2abbrevindex]))
		{
			delete shdr;
			return;
		}
		this->flags |= O_DEBUG;
	}

	if (ehdr.e_type == ET_EXEC && ehdr.e_machine == EM_386)
	{
#ifdef GEMINI_ON_OSR5
		if ((ehdr.e_flags != OSR5_FIX_FLAG) &&
			((ehdr.e_flags == UDK_FIX_FLAG) ||
			(foreign_file_has_no_note &&
				!sections[s_notes])))
#else
		if ((ehdr.e_flags == OSR5_FIX_FLAG) ||
			((ehdr.e_flags != UDK_FIX_FLAG) &&
			(sections[s_notes] &&
			(sections[s_notes]->size == NT_ELFID_SZ))))
#endif
			foreign = 1;
	}
	file_form = ff_elf;
	symtable = new Symtable(fdobj, (Object *)this); 
	delete shdr;
}

// map in data for 2 sections - try to make one contiguous mapping
int
ELF::map_sections(Sect_type stype1, Elf_Shdr *sptr1, Sect_type stype2,
	Elf_Shdr *sptr2)
{
	Sectinfo	*sect1;
	Sectinfo	*sect2;

	sect1 = new Sectinfo;
	sect2 = new Sectinfo;
	sect1->vaddr = sptr1->sh_addr;
	sect1->size = sptr1->sh_size;
	sect1->offset = sptr1->sh_offset;
	sect1->next = 0;
	sect2->vaddr = sptr2->sh_addr;
	sect2->size = sptr2->sh_size;
	sect2->offset = sptr2->sh_offset;
	sect2->next = 0;
	if ((sptr1->sh_offset + sptr1->sh_size) ==
		sptr2->sh_offset)
	{
		// one mapping
		if (!map_data(sect1->data, sect1->offset, 
			(sect1->size + sect2->size)))
		{
			delete sect1;
			delete sect2;
			return 0;
		}
		sect2->data = (char*)(sect1->data) + sect1->size;
	}
	else if ((sptr2->sh_offset + sptr2->sh_size) ==
		sptr1->sh_offset)
	{
		// one mapping
		if (!map_data(sect2->data, sect2->offset, 
			(sect1->size + sect2->size)))
		{
			delete sect1;
			delete sect2;
			return 0;
		}
		sect1->data = (char*)(sect2->data) + sect2->size;
	}
	else // not contiguous
	{
		if (!map_data(sect1->data, sect1->offset, 
			sect1->size) ||
			!map_data(sect2->data, sect2->offset, 
				sect2->size))
		{
			delete sect1;
			delete sect2;
			return 0;
		}
	}
	sections[stype1] = sect1;
	sections[stype2] = sect2;
	return 1;
}

int
ELF::read_section(Sect_type stype, Elf_Shdr *sptr, int noread)
{
	Sectinfo	*sect = new Sectinfo;

	sect->size = sptr->sh_size;
	sect->vaddr = sptr->sh_addr;
	sect->offset = sptr->sh_offset;
	sect->next = 0;
	if (noread)
	{
		sect->data = 0;
	}
	else if (read_data(sect->data, sect->offset, sect->size, 0)
		!= sect->size)
	{
		delete sect;
		return 0;
	}
	sections[stype] = sect;
	return 1;
}

int
ELF::find_symbol(const char *symbol_name, Iaddr & addr )
{
	int		i, count;
	long		symsize;
	Elf_Sym 	*symtab;
	char 		*strtab;
	Sectinfo	*sym = sections[s_symtab];
	Sectinfo	*str = sections[s_strtab];

	if (!sym || !str)
		return 0;
	symtab = (Elf_Sym *)sym->data;
	symsize = sym->size;
	strtab = (char *)str->data;
	symtab++; // first symbol is null
	count = (int)(symsize / sizeof(Elf_Sym));
	for ( i = 1 ; i < count ; ++i, symtab++ ) 
	{
		if ( !strcmp( symbol_name, strtab + symtab->st_name ) )
		{
			addr = symtab->st_value;
			return 1;
		}
	}
	return 0;
}

Seginfo *
ELF::get_seginfo( int & count, int & shared )
{
	Seginfo		*sptr;

	if (!phdr || phdrnum == 0)
		return 0;
	shared = this->flags & O_SHARED;
	count = phdrnum;
	if (seginfo)
		return seginfo;
	sptr = seginfo = (Seginfo*) new(Seginfo[phdrnum]);
	for (int i = 0 ; i < phdrnum ; ++i, phdr++, sptr++ )
	{
		sptr->offset = phdr->p_offset;
		sptr->vaddr = phdr->p_vaddr;
		sptr->mem_size = phdr->p_memsz;
		sptr->file_size = phdr->p_filesz;
		sptr->seg_flags = 0;
		if (phdr->p_type == PT_LOAD )
			sptr->seg_flags |= SEG_LOAD;
		if (phdr->p_flags & PF_X )
			sptr->seg_flags |= SEG_EXEC;
		if (phdr->p_flags & PF_W )
			sptr->seg_flags |= SEG_WRITE;
		if (phdr->p_flags & PF_R )
			sptr->seg_flags |= SEG_READ;
	}
	return seginfo;
}

int
ELF::get_phdr(int &count, Elf_Phdr *&phdrp)
{
	if (phdr && phdrnum > 0)
	{
		count = phdrnum;
		phdrp = phdr;
		return 1;
	}
	else
		return 0;
}

// used for truncated core files to get rid of phdr entries
// for truncated segments
void
ELF::reset_phdr(Elf_Phdr *phdrp, int num)
{
	phdrnum = num;
	delete phdr;
	phdr = phdrp;
}
