#ident	"@(#)debugger:libsymbol/common/Dwarfbuild.C	1.18"
#include	"Dwarfbuild.h"
#include	"Evaluator.h"
#include	"Interface.h"
#include	"Object.h"
#include	"ELF.h"
#include	"Locdesc.h"
#include	"Tag.h"
#include	"Iaddr.h"
#include	"dwarf.h"
#include	"dwarf2.h"
#include	<string.h>


Dwarfbuild::Dwarfbuild(Evaluator *e)
{
	ptr = 0;
	entry_offset = 0;
	entry_end = 0;
	tag_internal = t_none;
	length = 0;
	curr_lang = UnSpec;
	evaluator = e;
}

// this is not inlined because of the destructors this function must call
Dwarfbuild::~Dwarfbuild()
{
}

unsigned char
Dwarfbuild::get_byte()
{
	byte *p;

	length -= 1;
	p = ptr; ++ptr;
	return *(unsigned char *)p;
}

// the following code will not work if the target byte order
// is different from the host byte order
unsigned short
Dwarfbuild::get_2byte()
{
	unsigned short	x;
	byte		*p = (byte *)&x;

	length -= 2;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr;
	return x;
}

unsigned long
Dwarfbuild::get_4byte()
{
	unsigned long	x;
	byte		*p = (byte *)&x;

	length -= 4;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr;
	return x;
}

#ifdef LONG_LONG
unsigned long long
Dwarfbuild::get_8byte()
{
	unsigned long long	x;
	byte			*p = (byte *)&x;

	length -= 8;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr; ++p;
	*p = *ptr; ++ptr;
	return x;
}
#endif

void
Dwarfbuild::get_low_pc()
{
	low_pc = get_address();
	protorec.add_attr(an_lopc, af_addr, low_pc);
	if (tag_internal == t_label)
	{
		// special case for labels: make hipc == lopc
		protorec.add_attr(an_hipc, af_addr, low_pc);
	}
}

void
Dwarfbuild::get_high_pc()
{
	protorec.add_attr(an_hipc, af_addr, get_address());
}

// dwarf1 are dwarf2 tag values are in most cases identical
Tag
Dwarfbuild::tagname(unsigned int index)
{
	switch (index)
	{
	case DW_TAG_array_type:			return t_arraytype;
	case DW_TAG_class_type:			return t_classtype;
	case DW_TAG_entry_point:		return t_entry;
	case DW_TAG_enumeration_type:		return t_enumtype;
	case DW_TAG_formal_parameter:		return t_argument;
	case TAG_global_subroutine:		return t_subroutine;
	case TAG_global_variable:		return t_variable;
	case DW_TAG_label:			return t_label;
	case DW_TAG_lexical_block:		return t_block;
	case TAG_local_variable:		return t_variable;
	case DW_TAG_member:			return t_structuremem;
	case DW_TAG_pointer_type:		return t_pointertype;
	case DW_TAG_reference_type:		return t_reftype;
	case DW_TAG_compile_unit:		return t_sourcefile;
	case DW_TAG_string_type:		return t_stringtype;
	case DW_TAG_structure_type:		return t_structuretype;
	case TAG_subroutine:			return t_subroutine;
	case DW_TAG_subroutine_type:		return t_functiontype;
	case DW_TAG_typedef:			return t_typedef;
	case DW_TAG_union_type:			return t_uniontype;
	case DW_TAG_unspecified_parameters:	return t_unspecargs;
	case DW_TAG_inheritance:		return t_baseclass_type;
	case DW_TAG_inlined_subroutine:		return t_inlined_sub;
	case DW_TAG_ptr_to_member_type:		return t_ptr_to_mem_type;
	case DW_TAG_base_type:			return t_basetype;
	case DW_TAG_catch_block:		return t_catch_block;
	case DW_TAG_const_type:			return t_consttype;
	case DW_TAG_constant:			return t_none;	
	case DW_TAG_enumerator:			return t_enumlittype;	
	case DW_TAG_subprogram:			return t_subroutine;	
	case DW_TAG_template_type_param:	return t_template_param;
	case DW_TAG_template_value_param:	return t_template_param;
	case DW_TAG_thrown_type:		return t_throwntype;
	case DW_TAG_try_block:			return t_try_block;
	case DW_TAG_variable:			return t_variable;
	case DW_TAG_volatile_type:		return t_volatiletype;	
	case DW_TAG_SCO_namespace:		return t_namespace;
	case DW_TAG_SCO_namespace_alias:	return t_namespace_alias;
	case DW_TAG_SCO_using_declaration:	return t_using_declaration;
	case DW_TAG_SCO_using_directive:	return t_using_directive;

	case TAG_padding:
	case DW_TAG_imported_declaration:
	case DW_TAG_variant:
	case DW_TAG_common_block:
	case DW_TAG_common_inclusion:
	case DW_TAG_module:
	case DW_TAG_set_type:
	case DW_TAG_subrange_type:
	case DW_TAG_with_stmt:
	case DW_TAG_access_declaration:
	case DW_TAG_file_type:
	case DW_TAG_friend:
	case DW_TAG_namelist:
	case DW_TAG_namelist_item:
	case DW_TAG_packed_type:
	case DW_TAG_variant_part:
	default:
		return t_none;
	}
}

// types indexed by Tag value
static Attr_name	typename_table[] = 
{
	an_nomore,		// t_none
	an_nomore,		// pt_startvars
	an_type,		// t_argument
	an_type,		// t_variable
	an_type,		// t_unionmem
	an_type,		// t_structuremem
	an_nomore,		// pt_endvars
	an_resulttype,		// t_entry
	an_resulttype,		// t_subroutine
	an_nomore,		// t_extlabel
	an_resulttype,		// t_inlined_sub
	an_nomore,		// pt_starttypes
	an_type,		// t_basetype
	an_elemtype,		// t_arraytype
	an_basetype,		// t_pointertype
	an_basetype,		// t_reftype
	an_resulttype,		// t_functiontype
	an_type,		// t_baseclass_type
	an_nomore,		// t_classtype
	an_nomore,		// t_structuretype
	an_nomore,		// t_uniontype
	an_nomore,		// t_enumtype
	an_nomore,		// t_enumlittype
	an_nomore,		// t_functargtype
	an_nomore,		// t_discsubrtype
	an_nomore,		// t_stringtype
	an_type,		// t_typedef
	an_nomore,		// t_unspecargs
	an_basetype,		// t_consttype
	an_basetype,		// t_volatiletype
	an_type,		// t_ptr_to_mem_type
	an_type,		// t_throwntype
	an_nomore,		// pt_endtypes
	an_nomore,		// t_sourcefile
	an_nomore,		// t_block
	an_nomore,		// t_label
	an_nomore,		// t_try_block
	an_nomore,		// t_catch_block
	an_nomore,		// t_namespace
	an_nomore,		// t_namespace_alias
	an_nomore,		// t_using_directive
	an_nomore,		// t_using_declaration
	an_type,		// t_template_param
};

Attr_name
Dwarfbuild::type_name()
{
	return typename_table[tag_internal];
}

Addrexp
Dwarfbuild::make_chunk(void *p, int len)
{
	char *s;

	s = new char[len];
	memcpy(s, (char *)p, len);
	return (Addrexp)s;
}

int
Dwarfbuild::out_of_range(offset_t offset)
{
	return (offset < entry_offset || offset >= entry_end);
}

void
Dwarfbuild::fixup_name()
{
	if (curr_lang == CPLUS && tag_internal == t_subroutine && low_pc != (Iaddr)-1)
	{
		// get full function signature from addrlist, which is constructed
		// from the ELF symbol table
		Attribute	*attr;
		Attribute	*attr_name;
		if ((attr = evaluator->lookup_addr(low_pc)) != 0
			&& (attr_name = evaluator->attribute(attr, an_name)) != 0
			&& attr_name->form == af_stringndx)
		{
			name = attr_name->value.name;
		}
		if (name)
			protorec.add_attr(an_name, af_stringndx, (char *)name);
	} else if (name)
		buildNameAttrs(protorec, name);
}

Attribute *
Dwarfbuild::find_record(offset_t offset)
{
	Attribute *attr;
	if (reflist.lookup(offset, attr))
	{
		return attr;
	}
	return 0;
}

