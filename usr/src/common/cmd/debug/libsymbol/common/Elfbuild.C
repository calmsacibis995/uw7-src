#ident	"@(#)debugger:libsymbol/common/Elfbuild.C	1.13"
#include	"Interface.h"
#include	"Elfbuild.h"
#include	"Locdesc.h"
#include	"Syminfo.h"
#include	"Tag.h"
#include	"Machine.h"
#include	"Object.h"
#include	"ELF.h"
#include	<string.h>
#include	<stdio.h>
#include	<elf.h>
#include	<sys/types.h>
#include	<unistd.h>


Elfbuild::Elfbuild( ELF *obj )
{
	object = obj;
	symptr = 0;
	strptr = 0;
	losym = 0;
	special = 0;
	hisym = -1;
	histr = -1;
}

offset_t
Elfbuild::globals_offset()
{
	return first_symbol();
}

offset_t
Elfbuild::first_symbol()
{

	if ( symptr == 0 )
	{
		Sectinfo	sinfo;

		if (!object->getsect(s_symtab, &sinfo))
			return 0;
		symptr = (char *)sinfo.data;
		hisym = sinfo.size;

		if (!object->getsect(s_strtab, &sinfo))
			return 0;
		strptr = (char *)sinfo.data;
		histr = sinfo.size;
		losym = sizeof(Elf_Sym); // 1st symbol is null
		hisym = hisym - sizeof(Elf_Sym) + 1;
	}
	return losym;
}

int
Elfbuild::get_syminfo( offset_t offset, Syminfo & syminfo, int )
{
	Elf_Sym *	sym;
	size_t		size;

	if (( offset < losym ) || ( offset >= hisym ) || hisym == -1)
	{
		return 0;
	}
	sym = (Elf_Sym*)(symptr + offset );
	syminfo.name = sym->st_name;
	syminfo.lo = sym->st_value;
	size = sym->st_size;
	syminfo.hi = syminfo.lo + size;
	switch ( ELF32_ST_BIND(sym->st_info) )
	{
		case STB_LOCAL:		syminfo.bind = sb_local;	break;
		case STB_GLOBAL:	syminfo.bind = sb_global;	break;
		case STB_WEAK:		syminfo.bind = sb_weak;		break;
		default:		syminfo.bind = sb_none;		break;
	}
	syminfo.resolved = ( (sym->st_shndx != SHN_UNDEF) &&
		(sym->st_shndx != SHN_ABS) );
	switch ( ELF32_ST_TYPE(sym->st_info) )
	{
		case STT_NOTYPE:	syminfo.type = st_func;		break;
		case STT_OBJECT:	syminfo.type = st_object;	break;
		case STT_FUNC:		syminfo.type = st_func;		break;
		case STT_SECTION:	syminfo.type = st_section;	break;
		case STT_FILE:		syminfo.type = st_file;		break;
		default:		syminfo.type = st_none;		break;
	}
	if (special && (offset >= special) && (syminfo.bind == sb_local))
	{
		// special hidden global symbols in shared libraries
		syminfo.bind = sb_global;
	}
	if (!syminfo.resolved && syminfo.type == st_func && 
		syminfo.bind == sb_global && syminfo.lo != 0)
	{
		// plt entry - size is not really size of function
		syminfo.hi = syminfo.lo + sizeof(unsigned long);
	}
	syminfo.sibling = offset + sizeof(Elf_Sym);
	syminfo.child = 0;
	return 1;
}

void
Elfbuild::find_arcs(Syminfo &syminfo)
{
	Elf_Sym		*sym;
	Elf32_Addr	hi;
	Elf32_Addr	lo;
	int		count = 0;

	lo = (Elf32_Addr) -1;
	hi = 0;
	sym = (Elf_Sym *)(symptr + syminfo.sibling);
	while (ELF32_ST_TYPE(sym->st_info) != STT_FILE
		&& ELF32_ST_BIND(sym->st_info) == STB_LOCAL)
	{
		if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC)
		{
			if (sym->st_value > hi)
				hi = sym->st_value + sym->st_size;
			if (sym->st_value < lo)
				lo = sym->st_value;
		}
		++sym;
		++count;
	}

	if (count)
	{
		syminfo.lo = lo;
		syminfo.hi = hi;
		syminfo.child = syminfo.sibling;
		syminfo.sibling = (char *)sym - symptr;
	}
}

Attribute *
Elfbuild::make_record( offset_t offset, int )
{
	Attribute	*attribute;
	Syminfo		syminfo;
	Locdesc		locdesc;
	Attr_value	value;
	Attr_value	value2;
	size_t		len;
	char		*name;

	if (( offset < losym ) || ( offset >= hisym ))
	{
		return 0;
	}
	else if ( reflist.lookup( offset, attribute) )
	{
		return attribute;
	}
	if ( get_syminfo( offset, syminfo ) == 0 )
	{
		return 0;
	}

	name = get_name(syminfo.name);
	if ( syminfo.type == st_object )
	{
		protorec.add_attr( an_tag, af_tag, t_variable );
		if (syminfo.bind == sb_global)
			protorec.add_attr(an_external, af_int, 1);
		else
		{
			protorec.add_attr( an_parent, af_symbol, 0L );
			protorec.add_attr( an_sibling, af_elfoffs,
				syminfo.sibling );
		}
		buildNameAttrs(protorec, name);
		locdesc.clear().addr(syminfo.lo);
		len = locdesc.size();
		value.loc = (Addrexp)new(char[len]);
		memcpy( (char*)value.loc, (char*)locdesc.addrexp(),
			(unsigned int)len );
		protorec.add_attr( an_location, af_locdesc, value );
		value.fund_type = ft_none;
		protorec.add_attr( an_type, af_fundamental_type, value);
		value2.word = 0;
		protorec.add_attr( an_assumed_type, af_int, value2);
	}
	else if ( syminfo.type == st_func )
	{
		int	plt = 0;
		if (syminfo.bind == sb_global || syminfo.bind == sb_weak)
		{
			if (!syminfo.resolved && syminfo.lo != 0)
				// plt entry, not real definition
				plt = 1;
			protorec.add_attr(an_tag, af_tag, t_subroutine);
			protorec.add_attr(an_external, af_int, 1);
		}
		else if (syminfo.bind == sb_local && !syminfo.resolved)
			return 0;
		else
		{
			protorec.add_attr( an_tag, af_tag, t_subroutine );
			protorec.add_attr( an_parent, af_symbol, 0L );
			protorec.add_attr( an_sibling, af_elfoffs,
				syminfo.sibling );
		}
		if (plt)
		{
			char	*newname = new char[strlen(name) + 5];
				// sizeof("@PLT" +1)
			sprintf(newname, "%s@PLT", name);
			protorec.add_attr( an_name, af_stringndx, newname);
		}
		else
			buildNameAttrs(protorec, name);
		protorec.add_attr( an_lopc, af_addr, syminfo.lo );
		protorec.add_attr( an_hipc, af_addr, syminfo.hi );
		value2.word = 0;
		protorec.add_attr( an_assumed_type, af_int, value2);
	}
	else if (syminfo.type == st_file)
	{
		if (strcmp(name, "_fake_hidden") == 0)
		{
			// special symbol marking hidden symbols
			// in shared libraries - not a real file
			// always last in chain of file entries
			special = syminfo.sibling;
			return 0;
		}
		find_arcs(syminfo);
		protorec.add_attr(an_tag, af_tag, t_sourcefile);
		protorec.add_attr(an_name, af_stringndx, name);
		protorec.add_attr(an_sibling, af_elfoffs, syminfo.sibling);
		if (syminfo.child)
		{
			protorec.add_attr(an_scansize, af_int,
				syminfo.sibling - syminfo.child);
			protorec.add_attr(an_child, af_elfoffs, syminfo.child);
			protorec.add_attr(an_lopc, af_addr, syminfo.lo);
			protorec.add_attr(an_hipc, af_addr, syminfo.hi);
		}
	}
	else
	{
		return 0;
	}
	attribute = protorec.put_record();
	reflist.add( offset, attribute );
	return attribute;
}

char *
Elfbuild::get_name( offset_t offset )
{
	if (( offset == 0 ) || (offset > histr))
		return 0;
	else
		return strptr + offset;
}

int
Elfbuild::out_of_range(offset_t offset)
{
	return (offset < losym || offset >= hisym);
}
