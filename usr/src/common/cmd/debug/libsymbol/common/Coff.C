#ident	"@(#)debugger:libsymbol/common/Coff.C	1.7"

// provide access to Coff objects

#include "Coff.h"
#include "Object.h"
#include "Machine.h"
#include "Symtable.h"
#include "Interface.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <a.out.h>

#define LIBMAGIC	0443	// COFF optional header magic number

Coff::Coff( int fd, dev_t dev, ino_t ino, time_t mtime) : OBJECT(fd, 
	dev, ino, mtime)
{
	struct		filehdr	header;
	long		line_offset = 0;
	long		sym_offset = 0;
	long		str_offset = 0;
	Sectinfo	*line = 0, *sym, *str, *lib = 0;

	if ((lseek(fdobj, 0, SEEK_SET) == -1) ||
		(::read(fdobj, &header, FILHSZ) != FILHSZ))
	{
		printe(ERR_obj_file_read, E_ERROR, FILHSZ, (unsigned long)0);
		return;
	}
	if (header.f_magic != COFFMAGIC)
	{
		printe(ERR_obj_file_form, E_ERROR);
	}
	void* void_section_headers;
	// If the type of the actual is not correct, C++ 2.0 passes
	// a reference to a temporary instead.
	if (read_data(void_section_headers, FILHSZ+header.f_opthdr,
		(SCNHSZ * header.f_nscns), 1) 
		!= (SCNHSZ * header.f_nscns))
	{
		return;
	}
	section_headers = (SCNHDR*)void_section_headers;
	numscns = (int)header.f_nscns;

	if ( header.f_opthdr > 0 )
	{
		struct aouthdr ahdr;
		if ((lseek( fdobj, FILHSZ, SEEK_SET) == -1) ||
			( ::read( fdobj, (char *)&ahdr,
			header.f_opthdr) != header.f_opthdr ))
		{
			printe(ERR_obj_file_read, E_ERROR, 
				header.f_opthdr, FILHSZ);
			return;
		}
		start = ahdr.entry;
		if (ahdr.magic == LIBMAGIC)
			this->flags |= O_SHARED;
	}
	sym_offset = header.f_symptr;
	str_offset = header.f_symptr+(header.f_nsyms*SYMESZ);

	SCNHDR	*shdrp = section_headers;
	int	found = 0;
	for (int i = 0 ; i < numscns ; ++i, ++shdrp)
	{
		if ( !found && shdrp->s_lnnoptr != 0)
		{
			line_offset = shdrp->s_lnnoptr;
			found = 1;
		}
		if (shdrp->s_flags & STYP_LIB)
		{
			lib = new Sectinfo;
			lib->size = shdrp->s_size;
			lib->offset = shdrp->s_scnptr;
			lib->vaddr = shdrp->s_vaddr;
			lib->next = 0;
			if (found)
				break;
		}
	}
	if (sym_offset)
	{
		sym = new Sectinfo;
		str = new Sectinfo;
		sym->offset = sym_offset;
		sym->size = str_offset - sym_offset;
		sym->vaddr = 0;
		sym->next = 0;
		// find size of strings - first 4 bytes of table
		long	size;
		if (lseek( fdobj, str_offset, SEEK_SET) == -1
			|| ::read( fdobj, &size, sizeof(long)) 
			!= sizeof(long))
		{
			printe(ERR_obj_file_read, E_ERROR, 
				sizeof(long), str_offset);
			delete lib;
			return;
		}
		str->vaddr = 0;
		str->offset = str_offset;
		str->size = size;
	}
	else
	{
		file_form = ff_coff;
		return;
	}
	if (line_offset)
	{
		line = new Sectinfo;
		line->offset = line_offset;
		line->size = sym_offset - line_offset;
		line->vaddr = 0;
		line->next = 0;
		this->flags |= O_DEBUG;
	}

	// read .lib section, if present
	if (lib)
	{
		if (read_data(lib->data, lib->offset, lib->size, 0) !=
			lib->size)
		{
			delete lib;
			delete line;
			return;
		}
		sections[s_lib] = lib;
	}

	// map sections - we try to map all three in one
	// contiguous chunk
	if (line_offset)
	{
		if (!map_data(line->data, line->offset,
			(line->size + sym->size + str->size)))
		{
			delete line;
			delete str;
			delete sym;
			return;
		}
		sym->data = (char*)(line->data) + line->size;
		sections[s_line] = line;
	}
	else
	{
		if (!map_data(sym->data, sym->offset,
			(sym->size + str->size)))
		{
			delete str;
			delete sym;
			return;
		}
	}
	str->data = (char*)(sym->data) + sym->size;
	sections[s_symtab] = sym;
	sections[s_strtab] = str;

	file_form = ff_coff;
	symtable = new Symtable(fdobj, (Object *)this); 
}

Seginfo *
Coff::get_seginfo( int & count, int & shared )
{
	SCNHDR		*shdrp;
	Seginfo		*sptr;
	int		i;

	shared = (this->flags & O_SHARED);
	count = numscns;
	if (seginfo)
		return seginfo;
	shdrp = section_headers;
	sptr = seginfo = (Seginfo*) new(Seginfo[count]);
	for ( i = 0 ; i < count ; i++, shdrp++, sptr++ )
	{
		sptr->offset = shdrp->s_scnptr;
		sptr->vaddr = shdrp->s_vaddr;
		sptr->mem_size = shdrp->s_size;
		sptr->file_size = shdrp->s_size;
		sptr->seg_flags = SEG_READ;
		if ((strcmp(shdrp->s_name,".text") == 0) ||
			(strcmp(shdrp->s_name,".data") == 0))
			sptr->seg_flags |= SEG_LOAD;
		if ((shdrp->s_flags & 0xf) == STYP_REG)
			sptr->seg_flags |= SEG_EXEC;
		if (!(shdrp->s_flags & STYP_TEXT))
			sptr->seg_flags |= SEG_WRITE;
	}
	return seginfo;
}

// return the file offset of the beginning of the symbol table

long
Coff::get_symtab_offset()
{
	if (sections[s_symtab])
		return sections[s_symtab]->offset;
	else
		return 0;
}

long
Coff::get_end_of_symtab()
{
	Sectinfo	*sect = sections[s_symtab];
	if (sect)
		return sect->offset + sect->size;
	else
		return 0;
}

// return the section number of the named section

int
Coff::sectno( const char * sname )
{
	SCNHDR *shdrp;
	int	i;

	shdrp = section_headers;
	for (i = 0 ; i < numscns ; ++i, ++shdrp)
	{
		if ( strncmp( sname, shdrp->s_name, 8 ) == 0)
			return i + 1;
	}
	return 0;
}

// return the file offset of the first line number entry

long
Coff::get_line_offset()
{
	if (sections[s_line])
		return sections[s_line]->offset;
	else
		return 0;
}

// return the symbol at a given offset The symbol table entries have to be
// copied since they are not guarenteed to be aligned properly (on most
// machines the file size is 18 bytes and the internal structure must be
// aligned on a four-byte boundary)
//
// Note that this fills in only the first aux. entry, but will skip over
// the others if there is more than one.
long
Coff::get_symbol( long offset, struct syment & sym, union auxent & aux )
{
	char	*p;

	Sectinfo	*sect = sections[s_symtab];
	if (!sect || offset < sect->offset || offset >= (sect->offset
		+ sect->size))
	{
		return 0;
	}

	p = (char *)sect->data + (offset - sect->offset);
	(void) memcpy( &sym, p, SYMESZ );
	if ( sym.n_numaux != 0 )
	{
		(void) memcpy( &aux, p+SYMESZ, SYMESZ );
	}

	// replace the name field with a pointer to where the string is saved
	// in memory - this has to be done here, where we have the original
	// address, otherwise short strings would have to be copied to be saved
	// as it is, we only have to copy strings that are exactly eight bytes -
	// there are 3 cases:
	//	names shorter than 8 bytes - live in the symbol table and are
	//		null-terminated
	//	names longer than 8 byte - live in the string table, which must
	//		be read in and save.  these are also null-terminated
	//	names exactly 8 bytes long - live in the symbol table but are
	//		not guarenteed to be null terminated.  these must be
	//		copied with a null byte appended

	if (sym.n_zeroes == 0)
	{
		// n_nptr overlays n_offset field
		sym.n_nptr = get_string( sym.n_offset );
	}
	else if (sym.n_name[SYMNMLEN-1] == '\0')
	{
		// short names are at the beginning of the symbol table entry
		sym.n_nptr = (char *)sect->data + (offset - sect->offset);
	}
	else
	{
		// make a null-terminated copy of the eight-byte-long name
		char *p = new char[SYMNMLEN+1];
		(void) memcpy( p, sym.n_name, SYMNMLEN );
		p[SYMNMLEN] = '\0';
		sym.n_nptr = p;
	}

	return offset + SYMESZ + (sym.n_numaux * SYMESZ);
}

char *
Coff::get_string( long offset )
{
	if (sections[s_strtab] == 0)
	{
		return 0;
	}
	else
		return (char *)sections[s_strtab]->data + offset;
}

// return a pointer to the line number entry at offset
// read in and save the line number entries if they are not
// already in memory

char *
Coff::get_lineno( long offset )
{
	Sectinfo	*line = sections[s_line];
	if ( !line || (offset < line->offset ) || 
		(offset >= (line->offset + line->size)) )
	{
		return 0;
	}
	return (char *)line->data + (offset - line->offset);
}
