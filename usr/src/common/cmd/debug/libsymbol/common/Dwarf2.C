#ident	"@(#)debugger:libsymbol/common/Dwarf2.C	1.22"
#include	"Dwarf2.h"
#include	"Interface.h"
#include	"Vector.h"
#include	"Object.h"
#include	"ELF.h"
#include	"FileEntry.h"
#include	"Lineinfo.h"
#include	"Locdesc.h"
#include	"Protoline.h"
#include	"Protorec.h"
#include	"Syminfo.h"
#include	"Tag.h"
#include	"Iaddr.h"
#include	"Evaluator.h"
#include	<dwarf2.h>
#include	"Fund_type.h"
#include	"str.h"
#include	<libdwarf2.h>
#include	<string.h>

const int header_size = 11;	// 4 (length) + 2 (version) + 4(abbreviation offset)
				// 	+ 1 (sizeof address)

// An array of Abbrev_list structures is stored in the abbrev_tables vector,
// with one entry per compilation unit.  The offset is the offset of the
// compilation unit from the beginning of the .debug_info section, used
// to look up the appropriate abbreviation table.  Language gives the
// source language for that compilation unit.
struct Abbrev_list
{
	offset_t		offset;
	Dwarf2_Abbreviation	*table;
	Language		language;
};

Dwarf2build::Dwarf2build(ELF *obj, Evaluator *e) : Dwarfbuild(e)
{
	ptr = 0;

	curr_file_offset = (offset_t)-1;
	curr_file_end = (offset_t)-1;
	curr_file_index = 0;
	level = 0;
	curr_lang = UnSpec;

	Sectinfo	sect;
	if (obj->getsect(s_dwarf2_info, &sect))
	{
		entry_end = sect.size;
		info_ptr = (byte *)sect.data;
	}
	else
	{
		entry_end = 0;
		info_ptr = 0;
	}

	if (obj->getsect(s_abbreviation, &sect))
	{
		abbrev_size = sect.size;
		abbrev_ptr = (byte *)sect.data;
	}
	else
	{
		abbrev_size = 0;
		abbrev_ptr = 0;
	}

	if (obj->getsect(s_dwarf2_line, &sect))
	{
		line_size = sect.size;
		line_ptr = (byte *)sect.data;
	}
	else
	{
		line_size = 0;
		line_ptr = 0;
	}
}

// this is not inlined because of the destructors this function must call
Dwarf2build::~Dwarf2build()
{
}

Attribute *
Dwarf2build::make_record(offset_t offset, int new_file)
{
	if (offset == (offset_t)-1 || offset >= entry_end)
		return 0;

	Attribute		*attribute;
	Dwarf2_Abbreviation	*abb_table;

	DPRINT(DBG_DWARF, ("Dwarf2build: offset = %#x\n", offset))
	if (new_file)
	{
		if ((abb_table = process_file(offset)) == 0)
		{
			printe(ERR_internal, E_ERROR,
				"Dwarf2build::make_record", __LINE__);
			return 0;
		}
	}
	else if (reflist.lookup(offset, attribute))
	{
		return attribute;
	}
	else
	{
		abb_table = get_abbreviation_table(offset);
	}

	const Dwarf2_Attribute	*attr;
	Dwarf2_Abbreviation	*abbr;
	unsigned long		index;
	int			i;

	ptr = info_ptr + offset;
	ptr += dwarf2_decode_unsigned(&index, ptr);
	if (!index)	// end of sibling chain
		return 0;

	abbr = &abb_table[index];
	tag_internal = tagname(abbr->tag);
	protorec.add_attr(an_tag, af_tag, tag_internal);
	if (tag_internal != t_sourcefile)
	{
		// the value of this attribute is reset by
		// Evaluator::add_parent
		protorec.add_attr(an_parent, af_symbol, 0L);
	}

	sibling = 0;
	has_type = 0;
	name = 0;
	byte_size = 0;
	low_pc = (Iaddr)-1;
	has_specification = 0;
	level = 0;
	out_of_line_inline = false;
	for (i = abbr->nattr, attr = abbr->attributes; i; i--, attr++)
	{
		process_attribute(attr);
	}

	fixup_name();
	if (tag_internal == t_basetype)
		fixup_basetype();

	offset_t	next_off = ptr - info_ptr;
	if (!sibling)
	{
		if (tag_internal == t_sourcefile)
		{
			sibling = curr_file_end;
			protorec.add_attr(an_sibling, af_dwarf2file, sibling);
		}
		else
		{
			// MORE - this code assumes that each entry with children
			// has an explicit
			// sibling pointer.  If that isn't so, the debugger would
			// need to read through all the children until it finds an
			// end-of-sibling-chain entry.
			if (abbr->children)
				printe(ERR_no_sibling, E_WARNING);
			sibling = next_off;
			protorec.add_attr(an_sibling, af_dwarf2offs, sibling);
		}
	}

	if (abbr->children)
	{
		if (next_off < sibling)
		{
			// if the sibling is not the next record, we have
			// children and must save the scansize (size of
			// current/parent entry)
			protorec.add_attr(an_scansize, af_int, sibling - next_off);
			protorec.add_attr(an_child, af_dwarf2offs, next_off);
			DPRINT(DBG_DWARF, ("Dwarf2build::make_record - child = %#x\n",
				next_off))
		}
		// The sibling offset of scope entries will be needed for
		// Evaluator::build_parent_chain, which has to find
		// the appropriate scope for a random offset.
		// Although this is also saved in the an_sibling attribute,
		// Evaluator resets that attribute to be a symbol pointer.
		protorec.add_attr(an_sibling_offset, af_dwarf2offs, sibling);
	}

	if (tag_internal == t_subroutine || tag_internal == t_entry
		|| tag_internal == t_functiontype)
	{
		// for C and C++, functions with no return type are of type void -
		// needed to distinguish them from functions returning int compiled
		// without debugging information
		if (!has_type && !has_specification)
			protorec.add_attr(an_resulttype, af_fundamental_type, ft_void);
		if (out_of_line_inline && low_pc == (Iaddr)-1)
			// filled in later by lookup through NameList
			protorec.add_attr(an_definition, af_symbol, 0L);
	}

	attribute =  protorec.put_record();
#if DEBUG
	if (debugflag & DBG_DWARF)
		dumpAttr(attribute, 0);
#endif
	reflist.add(offset, attribute);
	return attribute;
}

void
Dwarf2build::process_attribute(const Dwarf2_Attribute *attr)
{
	unsigned long value;

	switch (attr->name)
	{
	case DW_AT_sibling:	get_sibling(attr->form);	break;
	case DW_AT_low_pc:	get_low_pc();			break;
	case DW_AT_high_pc:	get_high_pc();			break;

	case DW_AT_language:
		get_language(attr->form);
		protorec.add_attr(an_language, af_language, curr_lang);
		break;

	case DW_AT_data_member_location:
	case DW_AT_location:	get_location(attr->form, an_location);	break;
	case DW_AT_vtable_elem_location:
				get_location(attr->form, an_vtbl_slot);	break;

	case DW_AT_external:
		if (get_byte())
			protorec.add_attr(an_external, af_int, 1);
		break;

	case DW_AT_name:
		// attribute for signature added later in fixup_name()
		name = (const char *)ptr;
		ptr += strlen((char *)ptr) + 1;
		break;

	case DW_AT_comp_dir:
		protorec.add_attr(an_comp_dir, af_stringndx, (char *)ptr);
		ptr += strlen((char *)ptr) + 1;
		break;

	case DW_AT_byte_size:
		byte_size = get_constant(attr->form);
		protorec.add_attr(an_bytesize, af_int, byte_size);
		break;

	case DW_AT_type:
		value = get_constant(attr->form);
		protorec.add_attr(type_name(), af_dwarf2offs,
					value ? (long)value : -1);
		has_type = 1;
		break;

	case DW_AT_encoding:
		// assumes encoding used only in base type record
		encoding = (unsigned short)get_constant(attr->form);
		break;

	case DW_AT_bit_offset:
		protorec.add_attr(an_bitoffs, af_int, get_constant(attr->form));
		break;

	case DW_AT_bit_size:
		protorec.add_attr(an_bitsize, af_int, get_constant(attr->form));
		break;

	case DW_AT_stmt_list:
		value = get_constant(attr->form);
		protorec.add_attr(an_lineinfo, af_dwarf2line, value);
		protorec.add_attr(an_file_table, af_dwarf2line, value);
		break;

	case DW_AT_const_value:
		protorec.add_attr((tag_internal == t_enumlittype) ? an_litvalue : an_const_value,
			af_int, get_constant(attr->form));
		break;

	case DW_AT_lower_bound:
		protorec.add_attr(an_lobound, af_int, get_constant(attr->form));
		break;

	case DW_AT_upper_bound:
		protorec.add_attr(an_hibound, af_int, get_constant(attr->form));
		break;

	case DW_AT_count:
		protorec.add_attr(an_subrange_count, af_int,
				get_constant(attr->form));
		break;

	case DW_AT_prototyped:
		if (get_byte())
			protorec.add_attr(an_prototyped, af_int, 1);
		break;

	case DW_AT_decl_file:
		protorec.add_attr(an_decl_file, af_int, get_constant(attr->form));
		break;

	case DW_AT_decl_line:
		protorec.add_attr(an_decl_line, af_int, get_constant(attr->form));
		break;

	case DW_AT_specification:
		value = get_constant(attr->form);
		protorec.add_attr(an_specification, af_dwarf2offs,
					value ? (long)(value) : -1);
		has_specification = 1;
		break;

	case DW_AT_virtuality:
		value = get_byte();
		if (value != DW_VIRTUALITY_none)
			protorec.add_attr(an_virtuality, af_int, value);
		break;

	case DW_AT_artificial:		// compiler-generated - Section 2.9
		if (get_byte())
			protorec.add_attr(an_artificial, af_int, 1);
		break;

	case DW_AT_containing_type:	// ptrs to members - Section 5.11
		value = get_constant(attr->form);
		protorec.add_attr(an_containing_type, af_dwarf2offs,
					value ? (long)(value) : -1);
		break;

	case DW_AT_declaration:		// Section 2.11
		if (get_byte())
			// filled in later by lookup through NameList
			protorec.add_attr(an_definition, af_symbol, 0L);
		break;

	case DW_AT_abstract_origin:
		value = get_constant(attr->form);
		protorec.add_attr(an_abstract, af_dwarf2offs,
					value ? (long)(value) : -1);
		has_specification = 1;
		break;

	case DW_AT_inline:
		value = get_constant(attr->form);
		if (value == DW_INL_declared_inlined || value == DW_INL_inlined)
			protorec.add_attr(an_inline, af_int, 1L);
		else if (value == DW_INL_declared_not_inlined)
		{
			out_of_line_inline = true;
			protorec.add_attr(an_inline, af_int, 0L);
		}
		break;

	case DW_AT_SCO_namespace:
		value = get_constant(attr->form);
		protorec.add_attr(an_namespace, af_dwarf2offs,
					value ? (long)(value) : -1);
		break;

	case DW_AT_SCO_using:
		value = get_constant(attr->form);
		protorec.add_attr(an_using, af_dwarf2offs,
					value ? (long)(value) : -1);
		break;

	// Attributes that apply only to C++
	case DW_AT_accessibility:
	case DW_AT_default_value:
	case DW_AT_friend:
	case DW_AT_start_scope:
	case DW_AT_use_location:	// ptrs to members

	// Attributes that may be needed for C/C++?
	case DW_AT_macro_info:
	case DW_AT_base_types:		// Section 3.1
	case DW_AT_calling_convention:	// Section 3.3.1
	case DW_AT_frame_base:		// Section 3.3.5
	case DW_AT_import:
	case DW_AT_is_optional:
	case DW_AT_return_addr:		// Section 3.3.5
	case DW_AT_variable_parameter:

	// These attributes are not handled
	// case DW_AT_address_class:	// architecture specific
	// case DW_AT_common_reference:	// FORTRAN
	// case DW_AT_discr:		// Pascal
	// case DW_AT_discr_list:	// Pascal
	// case DW_AT_discr_value:	// Pascal
	// case DW_AT_identifier_case:
	// case DW_AT_namelist_item:	// Fortran90
	// case DW_AT_ordering:		// row or column for arrays
	// case DW_AT_priority:		// modules
	// case DW_AT_producer:
	// case DW_AT_segment:		// architecture specific
	// case DW_AT_static_link:	// nested subroutines
	// case DW_AT_stride_size:	// not needed for C
	// case DW_AT_string_length:	// Fortran, ...
	// case DW_AT_visibility:	// Modula2
	// case DW_AT_lo_user:
	// case DW_AT_hi_user:
	// case DW_AT_decl_column:		// Section 2.12
	default:
		skip_attribute(attr->form);
		break;
	}
}

// do a binary search for the given offset
Dwarf2_Abbreviation *
Dwarf2build::find_table(offset_t offset)
{
	Abbrev_list	*list = (Abbrev_list *)abbrev_tables.ptr();
	int		nitems = abbrev_tables.size()/sizeof(Abbrev_list);
	int		first = 0;
	int		last = nitems - 1;
	int		middle = 0;

	while (first <= last)
	{
		middle = (first + last)/2;
		if (list[middle].offset > offset)
		{
			last = middle - 1;
		}
		else if ((list[middle].offset == offset)
			|| (list[middle+1].offset > offset))
		{
			break;
		}
		else
		{
			first = middle + 1;
		}
	}

	curr_file_offset = list[middle].offset;
	curr_file_index = middle;
	DPRINT(DBG_DWARF, ("Dwarf2build::find_table -  index = %d, offset = %#x\n",
		curr_file_index, curr_file_offset))
	curr_lang = list[middle].language;
	return list[middle].table;
}

// MORE - both process_file and get_abbreviation_table assume that compilation
// unit entries are processed in order to create the list in abbrev_tables.
// That assumption will not be valid if/when we use the accelerated access
// tables (.debug_pubnames or .debug_aranges sections)

Dwarf2_Abbreviation *
Dwarf2build::process_file(offset_t &offset)
{
	if (offset >= entry_end)
		return 0;
	ptr = info_ptr + offset;

	offset_t	abbrev_offset;
	int		version;

	curr_file_offset = offset;
	curr_file_end = offset + get_4byte() + 4; // length
	version = get_2byte();
	abbrev_offset = get_4byte();
	addr_size = get_byte();

	if (version != 2 || addr_size != sizeof(void *)
		|| abbrev_offset >= abbrev_size)
	{
		printe(ERR_bad_debug_entry, E_ERROR, offset);
		return 0;
	}

	Abbrev_list		*list = (Abbrev_list *)abbrev_tables.ptr();
	int			nitems = abbrev_tables.size()/sizeof(Abbrev_list);
	Dwarf2_Abbreviation	*table = 0;

	if (list && list[nitems-1].offset >= offset)
	{
		table = find_table(offset);
	}
	else
	{
		Abbrev_list item;
		table = dwarf2_get_abbreviation_table(abbrev_ptr + abbrev_offset,
						abbrev_size - abbrev_offset, 0);
		curr_file_index = nitems;
		item.offset = offset;
		item.table = table;
		item.language = curr_lang = UnSpec;
		abbrev_tables.add(&item, sizeof(Abbrev_list));
	}
	if (!table)
	{
		printe(ERR_bad_debug_entry, E_ERROR, offset);
		return 0;
	}
	offset += header_size;
#if DEBUG
	list = (Abbrev_list *)abbrev_tables.ptr();
	nitems = abbrev_tables.size()/sizeof(Abbrev_list);
	for (int i = 0; i < nitems; ++i)
	{
		DPRINT(DBG_DWARF, ("Dwarf2build::process_file - list[%d].offset = %#x\n", i, list[i].offset))
	}
#endif
	return table;
}

Dwarf2_Abbreviation *
Dwarf2build::get_abbreviation_table(offset_t offset)
{
	Abbrev_list	*list = (Abbrev_list *)abbrev_tables.ptr();
	int		last = abbrev_tables.size()/sizeof(Abbrev_list) - 1;

	// optimization - most likely to be in the same file as last time
	if (list[curr_file_index].offset <= offset
		&& ((curr_file_index == last)
			|| list[curr_file_index+1].offset > offset))
	{
		curr_lang = list[curr_file_index].language;
		return list[curr_file_index].table;
	}

	// should not get into this function for a compilation unit entry,
	// or for random children of a compilation unit without having
	// built the record for the compilation unit, and compilation
	// units are covered sequentially, so can assume that any given
	// offset is covered by the table.
	if (offset >= list[last].offset)
	{
		curr_file_offset = list[last].offset;
		curr_file_index = last;
		curr_lang = list[last].language;
		DPRINT(DBG_DWARF, ("Dwarf2build::get_abbreviation_table -  index = %d, offset = %#x\n", curr_file_index, curr_file_offset))
		return list[last].table;
	}

	return find_table(offset);
}

// translate dwarf2 type information into debugger's internal format
// Do this when the type is read, to avoid having to do it each time
// the type is referenced
void
Dwarf2build::fixup_basetype()
{
	Fund_type ft = ft_none;

	if (!byte_size && strcmp(name, "void") == 0)
	{
		ft = ft_void;
		protorec.add_attr(an_type, af_fundamental_type, ft);
		return;
	}

	switch (encoding)
	{
	case DW_ATE_address:		ft = ft_pointer;	break;
	case DW_ATE_boolean:		ft = ft_boolean;	break;
	case DW_ATE_unsigned_char:	ft = ft_uchar;		break;

	case DW_ATE_signed_char:
		if (name && strstr(name, "signed"))
			ft = ft_schar;
		else
			ft = ft_char;
		break;

	case DW_ATE_float:
		switch (byte_size)
		{
		case SFLOAT_SIZE:	ft = ft_sfloat;		break;
		case LFLOAT_SIZE:	ft = ft_lfloat;		break;
		case XFLOAT_SIZE:	ft = ft_xfloat;		break;
		}
		break;

	case DW_ATE_signed:
		if (name && strstr(name, "signed"))
		{
			switch (byte_size)
			{
			case CHAR_SIZE:		ft = ft_schar;
						break;
			case SHORT_SIZE:	ft = ft_sshort;
						break;
#if INT_MAX < LONG_MAX
			case LONG_SIZE:		ft = ft_slong;
						break;
#endif
			case INT_SIZE:
				if (strstr(name, "long"))
					ft = ft_slong;
				else
					ft = ft_sint;
				break;
#if LONG_LONG
			case LONG_LONG_SIZE:	ft = ft_slong_long; 
						break;
#endif
			}
		}
		else
		{	// "plain" types
			switch (byte_size)
			{
			case CHAR_SIZE:		ft = ft_char;
						break;
			case SHORT_SIZE:	ft = ft_short;
						break;
#if INT_MAX < LONG_MAX
			case LONG_SIZE:		ft = ft_long;
						break;
#endif
			case INT_SIZE:
				if (strstr(name, "long"))
					ft = ft_long;
				else
					ft = ft_int;
				break;
#if LONG_LONG
			case LONG_LONG_SIZE:	ft = ft_long_long;
						break;
#endif
			}
		}
		break;

	case DW_ATE_unsigned:
		switch (byte_size)
		{
		case CHAR_SIZE:		ft = ft_uchar;
					break;
		case SHORT_SIZE:	ft = ft_ushort;
					break;
#if INT_MAX < LONG_MAX
		case LONG_SIZE:		ft = ft_ulong;
					break;
#endif
		case INT_SIZE:
				if (strstr(name, "long"))
					ft = ft_ulong;
				else
					ft = ft_uint;
				break;
#if LONG_LONG
		case LONG_LONG_SIZE:	ft = ft_ulong_long;
						break;
#endif
		}
		break;

	case DW_ATE_complex_float:
	default:
		printe(ERR_unimp_dwarf2_entry, E_ERROR, ".debug_info");
		break;
	}
	protorec.add_attr(an_type, af_fundamental_type, ft);
}

void
Dwarf2build::fill_in_syminfo(Syminfo &syminfo, Dwarf2_Abbreviation *abb_table,
	Dwarf2_Abbreviation *abbr, int level, int &has_location)
{
	const Dwarf2_Attribute	*attr;
	int			i;

	for (i = abbr->nattr, attr = abbr->attributes; i; i--, attr++)
	{
		switch (attr->name)
		{
		case DW_AT_language:
			get_language(attr->form);
			break;
		case DW_AT_name:
			syminfo.name = (long)ptr;
			ptr += strlen((char *)ptr) + 1;
			break;

		case DW_AT_sibling:
		{
			offset_t sib = (offset_t)get_constant(attr->form);
			if (!level)
				syminfo.sibling = sib;
		}
			break;

		case DW_AT_low_pc:
			syminfo.lo = get_address();
			has_location = 1;
			break;

		case DW_AT_high_pc:
			syminfo.hi = get_address();
			has_location = 1;
			break;

		case DW_AT_external:
			if (get_byte())
				syminfo.bind = sb_global;
			break;

		case DW_AT_specification:
		{
			// fill in values from referenced symbol
			// probably DW_AT_name, DW_AT_external
			byte			*save_ptr;
			Dwarf2_Abbreviation	*new_abbr;
			unsigned long		index;
			offset_t	spec_offset = get_constant(attr->form);
			save_ptr = ptr;
			ptr = info_ptr + spec_offset;
			ptr += dwarf2_decode_unsigned(&index, ptr);
			if (index)
			{
				new_abbr = &abb_table[index];
				fill_in_syminfo(syminfo, abb_table, new_abbr,
						level+1, has_location);
			}
			ptr = save_ptr;
		}
			break;

		case DW_AT_location:
			has_location = 1;
			// FALLTHROUGH
		default:
			skip_attribute(attr->form);
			break;
		}
	}

	if (!level)
	{
		offset_t	next_off = ptr - info_ptr;
		if (tag_internal == t_sourcefile)
			syminfo.sibling = curr_file_end;
		else if (!syminfo.sibling)
			syminfo.sibling = next_off;
	
		if (abbr->children)
		{
			if (next_off < syminfo.sibling)
			{
				syminfo.child = next_off;
			}
		}
	}
}

int
Dwarf2build::get_syminfo(offset_t offset, Syminfo &syminfo, int new_file)
{
	if (offset >= entry_end)
		return 0;

	Dwarf2_Abbreviation	*abb_table;
	Dwarf2_Abbreviation	*abbr;
	unsigned long		index;
	int			has_location = 0;

	// assumption - if offset is not within range of current file,
	// must represent a new file -- could not have gotten an
	// offset within a file without going down from the top
	if (new_file || (abb_table = get_abbreviation_table(offset)) == 0)
	{
		if ((abb_table = process_file(offset)) == 0)
		{
			printe(ERR_internal, E_ERROR,
				"Dwarf2build::get_syminfo", __LINE__);
			return 0;
		}
	}

	syminfo.type = st_none;
	syminfo.bind = sb_none;
	syminfo.name = 0;
	syminfo.sibling = 0;
	syminfo.resolved = 1;
	syminfo.child = 0;
	syminfo.lo = 0;
	syminfo.hi = 0;

	ptr = info_ptr + offset;
	ptr += dwarf2_decode_unsigned(&index, ptr);
	if (!index)
	{
		// end of sibling chain)
		return 1;
	}

	abbr = &abb_table[index];
	tag_internal = tagname(abbr->tag);
	switch (tag_internal)
	{
	case t_subroutine:
		syminfo.type = st_func;
		syminfo.bind = sb_local;
		break;
	case t_argument:
	case t_variable:
		syminfo.type = st_object;
		syminfo.bind = sb_local;
		break;
	case t_sourcefile:
		syminfo.type = st_file;
		syminfo.bind = sb_none;
		break;
	default:
		break;
	}

	fill_in_syminfo(syminfo, abb_table, abbr, 0, has_location);

	// can't handle entries for global declarations
	// that are not definitions in the global name list
	if (syminfo.bind == sb_global && !has_location)
		syminfo.resolved = 0;

	return 1;
}

offset_t		
Dwarf2build::globals_offset() 
{
	level = 0;
	return first_file();
}

Lineinfo *
Dwarf2build::line_info(offset_t offset, const FileEntry *file_list)
{
	byte		*endptr;
	byte		*prologue_end;
	unsigned int	version;
	unsigned int	minimum_instruction_length;
	int		line_base;
	unsigned int	line_range;
	unsigned int	opcode_base;
	const FileEntry	*file_entry = &file_list[1];

	// state machine variables
	Iaddr		address = 0;
	unsigned long	file = 1;
	unsigned long	line = 1;
	unsigned long	column = 0;
	Iaddr		prev_addr = (Iaddr)-1;
	unsigned long	prev_line = 0;
	int		is_stmt;	// MORE - this is set but not used yet
	
	if (offset == (offset_t)-1 || offset >= line_size)
		return 0;

	// read statement prologue
	ptr = line_ptr + offset;
	endptr = ptr + get_4byte();
	version = get_2byte();
	prologue_end = ptr + get_4byte();
	minimum_instruction_length = get_byte();
	is_stmt = get_byte();
	line_base = (signed char)get_byte();
	line_range = get_byte();
	opcode_base = get_byte();

	if (version != 2 || prologue_end > endptr)
	{
		printe(ERR_bad_debug_entry, E_ERROR, offset);
		return 0;
	}

	// read standard opcodes lengths table
	int	*opcode_lengths = new int[opcode_base];
	opcode_lengths[0] = 0;
	for (int i = 1; i < opcode_base; i++)
	{
		opcode_lengths[i] = get_byte();
	}

	ptr = prologue_end;	// skip headers, already read in process_includes
	const unsigned int op255_advance = (255 - opcode_base)/line_range;
	// read statement program
	while (ptr < endptr)
	{
		unsigned long	advance;
		long		sadvance;
		byte opcode = get_byte();
		switch (opcode)
		{
		case 0:
		{
			// extended opcodes
			unsigned long	size;
			byte		subop;
			ptr += dwarf2_decode_unsigned(&size, ptr);
			subop = get_byte();
			switch (subop)
			{
			case DW_LNE_end_sequence:	goto end_loop;
			case DW_LNE_set_address:
				address = (Iaddr)get_4byte();
				break;
			case DW_LNE_define_file:	// MORE
			default:	// skip if not recognized
				ptr += size - 1;
				break;
			}
		}
			break;

		case DW_LNS_copy:
			if ((address == prev_addr) &&
				(line != prev_line))
			{
				// adding new line at same address
				// get rid of former line entry
				protoline.drop_line();
			}
			prev_addr = address;
			prev_line = line;
			protoline.add_line(address, line, file_entry);
			break;

		case DW_LNS_advance_pc:
			ptr += dwarf2_decode_unsigned(&advance, ptr);
			address += advance * minimum_instruction_length;
			break;

		case DW_LNS_advance_line:
			ptr += dwarf2_decode_signed(&sadvance, ptr);
			line += sadvance;
			break;

		case DW_LNS_set_file:
			ptr += dwarf2_decode_unsigned(&file, ptr);
			file_entry = &file_list[file];
			break;

		case DW_LNS_set_column:
			ptr += dwarf2_decode_unsigned(&column, ptr);
			break;

		case DW_LNS_negate_stmt:	is_stmt = !is_stmt;	break;
		case DW_LNS_fixed_advance_pc:	address += get_2byte();	break;
		case DW_LNS_const_add_pc:	address += op255_advance; break;

		case DW_LNS_set_basic_block:	// unused
			break;

		default:
			if (opcode < opcode_base)
			{
				// skip over unknown standard opcodes
				printe(ERR_unimp_dwarf2_entry, E_WARNING, ".debug_line");
				for (int i = 1; i < opcode_lengths[opcode]; i++)
					ptr += dwarf2_decode_unsigned(&advance, ptr);
				continue;
			}

			// decode special opcodes
			opcode -= opcode_base;
			address += opcode/line_range;
			line += line_base + (opcode % line_range);
			if ((address == prev_addr) &&
				(line != prev_line))
			{
				// adding new line at same address
				// get rid of former line entry
				protoline.drop_line();
			}
			prev_addr = address;
			prev_line = line;
			protoline.add_line(address, line, file_entry);
			break;
		}
	}
end_loop:
	delete opcode_lengths;
	return protoline.put_line(address);
}

FileTable *
Dwarf2build::process_includes(offset_t offset, const char *comp_dir)
{
	byte		*endptr;
	byte		*prologue_end;
	unsigned int	version;
	unsigned int	opcode_base;

	if (offset == (offset_t)-1 || offset >= line_size)
		return 0;

	// read statement prologue
	ptr = line_ptr + offset;
	endptr = ptr + get_4byte();
	version = get_2byte();
	prologue_end = ptr + get_4byte();
	(void) get_byte();	// minimum instruction length
	(void) get_byte();	// is_stmt
	(void) get_byte();	// line_base
	(void) get_byte();	// line_range
	opcode_base = get_byte();

	if (version != 2 || prologue_end > endptr)
	{
		printe(ERR_bad_debug_entry, E_ERROR, offset);
		return 0;
	}

	// skip standard opcodes lengths table
	for (int i = 1; i < opcode_base; i++)
	{
		(void) get_byte();
	}

	Vector		*vec = vec_pool.get();
	FileEntry	fentry;
	int		cnt = 1;
	const char	*first_dir;
	const char	**includes;

	// initialize the first slot with the compilation directory
	// if not available, use the current directory
	if (comp_dir)
	{
		// skip over "host_name:"
		if ((first_dir = strchr(comp_dir, ':')) != 0)
			first_dir++;
		else
			first_dir = comp_dir;
	}
	else
		first_dir = ".";
	vec->clear();
	vec->add(&first_dir, sizeof(char *));

	// read list of include_directories
	while (*ptr != '\0')
	{
		vec->add(&ptr, sizeof(char *));
		ptr += strlen((char *)ptr) + 1;
		cnt++;
		if (ptr > prologue_end)
		{
			printe(ERR_bad_debug_entry, E_ERROR, offset);
			vec_pool.put(vec);
			return 0;
		}
	}
	if (cnt)
	{
		includes = new const char *[cnt];
		memcpy(includes, vec->ptr(), sizeof(char *) * cnt);
	}
	else
		includes = 0;

	// read list of file names
	ptr++;
	cnt = 1;
	vec->clear();
	memset(&fentry, 0, sizeof(FileEntry));
	vec->add(&fentry, sizeof(FileEntry));
	while (*ptr != '\0')
	{
		unsigned long	time = 0;
		unsigned long	size = 0;
		unsigned long	dindex = 0;

		fentry.file_name = (const char *)ptr;
		ptr += strlen((char *)ptr) + 1;
		ptr += dwarf2_decode_unsigned(&dindex, ptr);
		fentry.dir_name = includes[dindex];
		ptr += dwarf2_decode_unsigned(&time, ptr);
		fentry.time = (time_t)time;
		ptr += dwarf2_decode_unsigned(&size, ptr);	// unused for now
		vec->add(&fentry, sizeof(FileEntry));
		cnt++;
		if (ptr > prologue_end)
		{
			printe(ERR_bad_debug_entry, E_ERROR, offset);
			vec_pool.put(vec);
			delete includes;
			return 0;
		}
	}

	FileTable	*table = new FileTable;
	table->filecount = cnt;
	table->files = new FileEntry[cnt];
	memcpy(table->files, vec->ptr(), sizeof(FileEntry) * cnt);
	vec_pool.put(vec);

	ptr++;
	return table;
}

void
Dwarf2build::get_language(form_t form)
{
	unsigned long	flang;
	Language	dlang;
	Abbrev_list	*list = (Abbrev_list *)abbrev_tables.ptr();

	flang = get_constant(form);
	switch(flang)
	{
		case DW_LANG_C89:	
		case DW_LANG_C:
				dlang = C;	
				break;
		case DW_LANG_C_plus_plus:	
				dlang = CPLUS;	
				break;
		default:
				printe(ERR_lang_not_supported, E_WARNING,
					dwarf2_language_name(flang));
				dlang = C;	
				break;
	}
	list[curr_file_index].language = curr_lang = dlang;
}

void
Dwarf2build::get_sibling(form_t form)
{
	sibling = (offset_t)get_constant(form);
	protorec.add_attr(an_sibling, af_dwarf2offs,
			sibling ? sibling : (offset_t)-1);
}

void
Dwarf2build::get_location(form_t form, Attr_name attrname)
{
	unsigned long	size = 0;

	// MORE - location could be a location list, represented by a
	// constant, which is an offset into the .debug_loc section
	switch (form)
	{
	case DW_FORM_block1:	size = (unsigned long) get_byte();	break;
	case DW_FORM_block2:	size = (unsigned long) get_2byte();	break;
	case DW_FORM_block4:	size = get_4byte();	break;
	case DW_FORM_block:
		ptr += dwarf2_decode_unsigned(&size, ptr);
		break;
	default:
		printe(ERR_unimp_dwarf2_entry, E_ERROR, ".debug_info");
		return;
	}

	if (size == 0)
		return;

	Locdesc		locdesc;
	int 		op;
	byte		*endptr = ptr + size;
	unsigned long	uval;
	long		val;

	locdesc.clear();
	while (ptr < endptr)
	{
		op = get_byte();
		if (op >= DW_OP_breg0 && op <= DW_OP_breg31)
		{
			locdesc.basereg((int)(op - DW_OP_breg0));
			ptr += dwarf2_decode_signed(&val, ptr);
			locdesc.offset(val);
			locdesc.add();
		}
		else if (op >= DW_OP_reg0 && op <= DW_OP_reg31)
		{
			locdesc.reg((int)(op - DW_OP_reg0));
		}
		else if (op >= DW_OP_lit0 && op <= DW_OP_lit31)
		{
			locdesc.offset((int)(op - DW_OP_lit0));
		}
		else
		{
			switch (op)
			{
			case DW_OP_addr:
				locdesc.addr(get_4byte());
				break;

			case DW_OP_bregx:
				ptr += dwarf2_decode_unsigned(&uval, ptr);
				locdesc.basereg(uval);
				ptr += dwarf2_decode_signed(&val, ptr);
				locdesc.offset(val);
				locdesc.add();
				break;

			case DW_OP_const1u:
				locdesc.offset(get_byte());
				break;

			case DW_OP_const1s:	// sign extension
				locdesc.offset((signed char)get_byte());
				break;

			case DW_OP_const2u:
				locdesc.offset(get_2byte());
				break;

			case DW_OP_const2s:
				locdesc.offset((signed short)get_2byte());
				break;

			case DW_OP_const4u:
				locdesc.offset(get_4byte());
				break;

			case DW_OP_const4s:
				locdesc.offset(get_4byte());
				break;

#if LONG_LONG
			case DW_OP_const8u:
				locdesc.offset(get_8byte());
				break;

			case DW_OP_const8s:
				locdesc.offset(get_8byte());
				break;
#endif

			case DW_OP_constu:
				ptr += dwarf2_decode_unsigned(&uval, ptr);
				locdesc.offset(uval);
				break;

			case DW_OP_consts:
				ptr += dwarf2_decode_signed(&val, ptr);
				locdesc.offset(val);
				break;

			case DW_OP_nop:
				break;

			case DW_OP_plus_uconst:
				ptr += dwarf2_decode_unsigned(&uval, ptr);
				locdesc.offset(uval);
				// fall-through

			case DW_OP_plus:
				locdesc.add();
				break;

			case DW_OP_regx:
				ptr += dwarf2_decode_unsigned(&uval, ptr);
				locdesc.reg((int)uval);
				break;

			case DW_OP_deref:
				locdesc.deref4();
				break;

			case DW_OP_SCO_reg_pair:
			{
				unsigned long uval2;
				ptr += dwarf2_decode_unsigned(&uval, ptr);
				ptr += dwarf2_decode_unsigned(&uval2, ptr);
				locdesc.reg_pair((int)uval, (int)uval2);
			}
				break;

			// MORE - some of these operators may be needed for
			// vtbl location expressions, when we have more C++ support
			case DW_OP_dup:
			case DW_OP_drop:
			case DW_OP_over:
			case DW_OP_pick:
			case DW_OP_swap:
			case DW_OP_rot:
			case DW_OP_xderef:
			case DW_OP_abs:
			case DW_OP_and:
			case DW_OP_div:
			case DW_OP_minus:
			case DW_OP_mod:
			case DW_OP_mul:
			case DW_OP_neg:
			case DW_OP_not:
			case DW_OP_or:
			case DW_OP_shl:
			case DW_OP_shr:
			case DW_OP_shra:
			case DW_OP_xor:
			case DW_OP_skip:
			case DW_OP_bra:
			case DW_OP_eq:
			case DW_OP_ge:
			case DW_OP_gt:
			case DW_OP_le:
			case DW_OP_lt:
			case DW_OP_ne:
			case DW_OP_fbreg:
			case DW_OP_piece:
			case DW_OP_deref_size:
			case DW_OP_xderef_size:
			default:
				printe(ERR_unimplemented_dwarf2_loc_op, E_ERROR,
					ptr - info_ptr,
					dwarf2_location_op_name(op));
				break;
			}
		}
	}
	protorec.add_attr(attrname, af_locdesc,
			make_chunk(locdesc.addrexp(),locdesc.size()));
}

// MORE - doesn't handle DW_FORM_data8, DW_FORM_ref8
unsigned long
Dwarf2build::get_constant(form_t form)
{
	switch (form)
	{
	case DW_FORM_ref1:	return (unsigned long) get_byte() + curr_file_offset;
	case DW_FORM_data1:	return (unsigned long) get_byte();
	case DW_FORM_ref2:	return (unsigned long) get_2byte() + curr_file_offset;
	case DW_FORM_data2:	return (unsigned long) get_2byte();
	case DW_FORM_ref4:
	case DW_FORM_ref_addr:	return (unsigned long) get_4byte() + curr_file_offset;
	case DW_FORM_data4:	return (unsigned long) get_4byte();
	case DW_FORM_ref_udata:
	{
		unsigned long ul;
		ptr += dwarf2_decode_unsigned(&ul, ptr);
		return ul + curr_file_offset;
	}
	case DW_FORM_udata:
	{
		unsigned long ul;
		ptr += dwarf2_decode_unsigned(&ul, ptr);
		return ul;
	}
	case DW_FORM_sdata:
	{
		long l;
		ptr += dwarf2_decode_signed(&l, ptr);
		return (unsigned long)l;
	}
	case DW_FORM_indirect:
		ptr += dwarf2_decode_unsigned(&form, ptr);
		return get_constant(form);
	default:
		printe(ERR_unimp_dwarf2_entry, E_ERROR, ".debug_info");
	}
	return 0;	
}

void
Dwarf2build::skip_attribute(form_t form)
{
	form_t		indirect_form;
	size_t		bytes_skipped = 0;
	long		l;
	unsigned long	ul;

	switch (form)
	{
	case DW_FORM_ref_addr:
	case DW_FORM_addr:	bytes_skipped = addr_size;	break;
	case DW_FORM_ref1:
	case DW_FORM_flag:
	case DW_FORM_data1:	bytes_skipped = 1;		break;
	case DW_FORM_ref2:
	case DW_FORM_data2:	bytes_skipped = 2;		break;
	case DW_FORM_strp:
	case DW_FORM_ref4:
	case DW_FORM_data4:	bytes_skipped = 4;		break;
	case DW_FORM_ref8:
	case DW_FORM_data8:	bytes_skipped = 8;		break;
	case DW_FORM_block1:	bytes_skipped = get_byte();	break;
	case DW_FORM_block2:	bytes_skipped = get_2byte();	break;
	case DW_FORM_block4:	bytes_skipped = get_4byte();	break;
	case DW_FORM_string:	bytes_skipped = strlen((char *)ptr) + 1; break;

	case DW_FORM_sdata:
		bytes_skipped = dwarf2_decode_signed(&l, ptr);
		break;

	case DW_FORM_ref_udata:
	case DW_FORM_udata:
		bytes_skipped = dwarf2_decode_unsigned(&ul, ptr);
		break;

	case DW_FORM_block:
		ptr += dwarf2_decode_unsigned(&ul, ptr);
		bytes_skipped = (size_t)ul;
		break;

	case DW_FORM_indirect:
		ptr += dwarf2_decode_unsigned(&indirect_form, ptr);
		skip_attribute(indirect_form);
		break;
	}

	ptr += bytes_skipped;
}

Language
Dwarf2build::language()
{
	return curr_lang;
}
