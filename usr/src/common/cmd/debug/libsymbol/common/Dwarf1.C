#ident	"@(#)debugger:libsymbol/common/Dwarf1.C	1.9"
#include	"Dwarf1.h"
#include	"Interface.h"
#include	"Object.h"
#include	"ELF.h"
#include	"Locdesc.h"
#include	"Syminfo.h"
#include	"Tag.h"
#include	"Iaddr.h"
#include	"dwarf.h"
#include	<string.h>

Dwarf1build::Dwarf1build( ELF *obj, Evaluator *e ) : Dwarfbuild(e)
{
	length = 0;
	nextoff = 0;

	Sectinfo	sinfo;
	if (obj->getsect(s_debug, &sinfo))
	{
		entry_data = (byte *)sinfo.data;
		entry_offset = sinfo.offset;
		entry_end = entry_offset + sinfo.size;
		entry_base = sinfo.vaddr;
	}
	if (!obj->getsect(s_line, &sinfo))
		stmt_offset = 0;
	else
	{
		stmt_data = (byte *)sinfo.data;
		stmt_offset = sinfo.offset;
		stmt_end = stmt_offset + sinfo.size;
		stmt_base = sinfo.vaddr;
	}
}

// this is not inlined because of the destructors this function must call
Dwarf1build::~Dwarf1build()
{
}

const char *
Dwarf1build::get_string()
{
	const char	*s = (const char *)ptr;
	register int	len = strlen(s)+1;

	ptr += len;
	length -= len;
	return s;
}

void
Dwarf1build::skip_attribute( short attrname )
{
	short	len2;
	long	word;

	switch( attrname & FORM_MASK )
	{
	case FORM_NONE:				
				break;
	case FORM_ADDR:
	case FORM_REF:		(void)get_4byte();		
				break;
	case FORM_BLOCK2:	len2 = get_2byte();
				length -= len2;
				ptr += len2;		
				break;
	case FORM_BLOCK4:	word = get_4byte();
				length -= word;
				ptr += word;	
				break;
	case FORM_DATA2:	(void)get_2byte();		
				break;
	case FORM_DATA8:	(void)get_4byte();
	case FORM_DATA4:	(void)get_4byte();		
				break;
	case FORM_STRING:	word = strlen((char *)ptr) + 1;
				length -= word;
				ptr += word;		
				break;
	default:
		printe(ERR_bad_debug_entry, E_ERROR, entry_offset +
			(ptr-2)-entry_data);
		length = 0;
	}
}

void
Dwarf1build::get_ft( Attr_form & form, Attr_value & value )
{
	short	ft;
	form = af_fundamental_type;
#if LONG_LONG
	if ((ft = get_2byte()) > (short)ft_ulong_long)
#else
	if ((ft = get_2byte()) > FT_label)
#endif
		value.fund_type  = ft_none;
	else
		value.fund_type = (Fund_type)ft;
}

void
Dwarf1build::fund_type()
{
	Attr_value	value;
	Attr_form	form;

	get_ft( form, value );
	protorec.add_attr( type_name(), form, value );
}

void
Dwarf1build::get_udt( Attr_form & form, Attr_value & value )
{
	form = af_dwarfoffs;
	value.word = entry_offset + get_4byte() - entry_base;
}

void
Dwarf1build::user_def_type()
{
	Attr_value	value;
	Attr_form	form;

	get_udt( form, value );
	protorec.add_attr( type_name(), form, value );
}

void
Dwarf1build::containing_type()
{
	Attr_value	value;
	value.word = entry_offset + get_4byte() - entry_base;
	protorec.add_attr(an_containing_type, af_dwarfoffs, value);
}

void
Dwarf1build::get_mft( Attr_form & form, Attr_value & value )
{
	short		len2;
	int		modcnt;
	byte		*p;
	int		i;
	
	// form is 2 byte length, followed by modcnt bytes of modifiers
	// followed by the 2 byte fundamental type
	len2 = get_2byte();
	modcnt = len2 - 2;
	ptr += modcnt;
	length -= modcnt;
	value.fund_type = (Fund_type)get_2byte();
	form = af_fundamental_type;
	p = ptr - 3;
	for ( i = 0 ; i < modcnt ; i++ )
	{
		switch (*(char *)p)
		{
		case MOD_pointer_to:
			prototype.add_attr(an_tag,af_tag,t_pointertype);
			break;
		case MOD_reference_to:
			prototype.add_attr(an_tag,af_tag,t_reftype);
			break;
		case MOD_const:
			prototype.add_attr(an_tag,af_tag,t_consttype);
			break;
		case MOD_volatile:
			prototype.add_attr(an_tag,af_tag,t_volatiletype);
			break;
		default:
			break;
		}
		prototype.add_attr(an_basetype,form,value);
		prototype.add_attr(an_bytesize, af_int, 4L );
		value.symbol = prototype.put_record();
		form = af_symbol;
		--p;
	}
}

void
Dwarf1build::mod_fund_type()
{
	Attr_value	value;
	Attr_form	form;

	get_mft( form, value );
	protorec.add_attr( type_name(), form, value );
}

void
Dwarf1build::get_mudt( Attr_form & form, Attr_value & value )
{
	short		len2;
	int		modcnt;
	byte		*p;
	int		i;

	// form is 2 byte length, followed by modcnt bytes of modifiers
	// followed by the 4 byte reference to the user-defined type
	len2 = get_2byte();
	modcnt = len2 - 4;
	ptr += modcnt;
	length -= modcnt;
	value.word = entry_offset + get_4byte() - entry_base;
	form = af_dwarfoffs;
	p = ptr - 5;
	for ( i = 0 ; i < modcnt ; i++ )
	{
		switch (*(char *)p)
		{
		case MOD_pointer_to:
			prototype.add_attr(an_tag,af_tag,t_pointertype);
			break;
		case MOD_reference_to:
			prototype.add_attr(an_tag,af_tag,t_reftype);
			break;
		case MOD_const:
			prototype.add_attr(an_tag,af_tag,t_consttype);
			break;
		case MOD_volatile:
			prototype.add_attr(an_tag,af_tag,t_volatiletype);
			break;
		default:
			break;
		}
		prototype.add_attr(an_basetype,form,value);
		prototype.add_attr(an_bytesize, af_int, 4L );
		value.symbol = prototype.put_record();
		form = af_symbol;
		--p;
	}
}

void
Dwarf1build::mod_u_d_type()
{
	Attr_value	value;
	Attr_form	form;

	get_mudt( form, value );
	protorec.add_attr( type_name(), form, value );
}

void
Dwarf1build::sibling()
{
	long		word;
	offset_t	siboff;

	word = get_4byte();
	siboff = entry_offset + word - entry_base;
	protorec.add_attr(an_sibling,af_dwarfoffs,siboff);
	if ( (nextoff != siboff) && (tag_internal != t_enumtype) )
	{
		// if the sibling is not the next record, we have
		// children and must save the scansize;
		// enum children are handled elsewhere
		protorec.add_attr(an_scansize, af_int, siboff - nextoff);
		protorec.add_attr(an_child, af_dwarfoffs, nextoff);
		protorec.add_attr(an_sibling_offset, af_dwarfoffs, siboff);
	}
}

void
Dwarf1build::set_language()
{
	LANG		flang;
	Language	dlang;

	flang = (LANG) get_4byte();
	switch(flang)
	{
		case LANG_C89:	
		case LANG_C:
				dlang = C;	
				break;
		case LANG_C_PLUS_PLUS:	
				dlang = CPLUS;	
				break;
		case LANG_UNK:
		default:	
				dlang = C;	
				break;
	}
	protorec.add_attr(an_language, af_language, dlang);
	curr_lang = dlang;
}

void
Dwarf1build::get_location( Attr_form & form, Attr_value & value )
{
	Locdesc		locdesc;
	short		len2;
	unsigned char 	op;

	locdesc.clear();
	len2 = get_2byte();

	if (len2 == 0)
		return;

	while ( len2 > 0 )
	{
		op = (unsigned char)get_byte();
		// the constants subtracted in the following code
		// represent a 2 or 4 byte length plus 1 byte operator
		switch( op )
		{
		case OP_REG:
			locdesc.reg( (int)get_4byte() );
			len2 -= 5;
			break;
		case OP_BASEREG:
			locdesc.basereg( (int)get_4byte() );
			len2 -= 5;
			break;
		case OP_ADDR:
			locdesc.addr( get_4byte() );
			len2 -= 5;
			break;
		case OP_CONST:
			locdesc.offset( get_4byte() );
			len2 -= 5;
			break;
		case OP_DEREF4:
			locdesc.deref4();
			len2 -= 1;
			break;
		case OP_ADD:
			locdesc.add();
			len2 -= 1;
			break;
		case OP_SCO_REG_PAIR:
		{
			int reg1 = (int)get_4byte();
			locdesc.reg_pair(reg1, (int)get_4byte());
			len2 -= 9;
			break;
		}
		default:
			len2 -= 1;
			break;
		}
	}
	value.loc = make_chunk(locdesc.addrexp(),locdesc.size());
	form = af_locdesc;
}

void
Dwarf1build::location()
{
	Attr_form	form;
	Attr_value	value;
	
	get_location( form, value );
	protorec.add_attr((tag_internal == t_subroutine) ? an_vtbl_slot : an_location,
			form, value );
}

void
Dwarf1build::byte_size()
{
	protorec.add_attr( an_bytesize, af_int, get_4byte() );
}

void
Dwarf1build::bit_offset()
{
	protorec.add_attr( an_bitoffs, af_int, get_2byte() );
}

void
Dwarf1build::bit_size()
{
	protorec.add_attr( an_bitsize, af_int, get_4byte() );
}

void
Dwarf1build::set_virtuality(short attrname)
{
	protorec.add_attr(an_virtuality, af_int, (attrname == AT_virtual) ? 1 : 2);
	get_byte();
}

void
Dwarf1build::stmt_list()
{
	long	word;

	word = stmt_offset + get_4byte() - stmt_base;
	protorec.add_attr(an_lineinfo,af_dwarfline, word);
	protorec.add_attr(an_file_table,af_dwarfline, word);
}

void
Dwarf1build::element_list( short attr, offset_t offset )
{
	Attribute	*prev;
	size_t		len;
	Attr_value	value;
	Attr_form	form;

	prev = 0;
	if ( (attr & FORM_MASK) == FORM_BLOCK2 )
		// old style enum list
		len = get_2byte();
	else if ( (attr & FORM_MASK) == FORM_BLOCK4 )
		len = get_4byte();
	else
	{
		printe(ERR_bad_debug_entry, E_ERROR, entry_offset+
			(ptr-2)-entry_data);
		len = 0;
	}
	while ( len > 0 )
	{
		prototype.add_attr( an_tag, af_tag, t_enumlittype );
		prototype.add_attr( an_litvalue, af_int, get_4byte() );
		len -= 4;
		len -= (strlen((char *)ptr)+1);
		buildNameAttrs(prototype, get_string());
		prototype.add_attr( an_sibling, af_symbol, prev );
		prototype.add_attr( an_parent, af_dwarfoffs, offset );
		prev = prototype.put_record();
	}
	form = af_symbol;
	value.symbol = prev;
	protorec.add_attr( an_child, form, value );
}


enum array_attrs 
{
	ar_elemtype = 0,
	ar_subtype,
	ar_lobound,
	ar_hibound,
	ar_tagtype
};

void
Dwarf1build::next_item( Attribute * a )
{
	Attribute	attr[5];

	if ( subscr_list( attr ) )
	{
		for(int i = 0; i < 5; i++)
			prototype.add_attr( (Attr_name)attr[i].name, 
				(Attr_form)attr[i].form, attr[i].value );
		a[ar_elemtype].value.symbol = prototype.put_record();
		a[ar_elemtype].form = af_symbol;
		a[ar_elemtype].name = an_elemtype;
	}
	else
	{
		a[ar_elemtype] = attr[ar_elemtype];
	}
}

int
Dwarf1build::subscr_list( Attribute * a )
{
	char		fmt;
	short		et_name;
	Attr_form	form;
	Attr_value	value;

	a[ar_elemtype].name = an_elemtype;
	a[ar_subtype].name = an_subscrtype;
	a[ar_lobound].name = an_lobound;
	a[ar_hibound].name = an_hibound;
	a[ar_tagtype].name = an_tag;
	a[ar_tagtype].form = af_tag;
	a[ar_tagtype].value.tag = t_arraytype;
	fmt = get_byte();
	if (fmt == FMT_ET)
	{
		et_name = get_2byte();
		switch( et_name )
		{
		case AT_fund_type:
			get_ft( form, value );
			break;
		case AT_user_def_type:
			get_udt( form, value );
			break;
		case AT_mod_fund_type:
			get_mft( form, value );
			break;
		case AT_mod_u_d_type:
			get_mudt( form, value );
			break;
		default:
			skip_attribute( et_name );
			return 0;
		}
		a[ar_elemtype].form = form;
		a[ar_elemtype].value = value;
		return 0;
	}
	else if (fmt > FMT_ET)
		return 0;
	else
	{
		if ((fmt >> 2) & FMT_UDT)
			get_udt( form, value );
		else
			get_ft( form, value );
		a[ar_subtype].form = form;
		a[ar_subtype].value = value;
		if ((fmt >> 1) & FMT_EXPR)
		{
			get_location( form, value );
			a[ar_lobound].form = form;
			a[ar_lobound].value = value;
		}
		else
		{
			a[ar_lobound].form = af_int;
			a[ar_lobound].value.word = get_4byte();
		}
		if (fmt & FMT_EXPR)
		{
			get_location( form, value );
			a[ar_hibound].form = form;
			a[ar_hibound].value = value;
		}
		else
		{
			a[ar_hibound].form = af_int;
			a[ar_hibound].value.word = get_4byte();
		}
		next_item( a );
		return 1;
	}
}

void
Dwarf1build::subscr_data()
{
	Attribute	attr[5];

	(void)get_2byte();
	if ( subscr_list( attr ) )
	{
		for(int i = 0; i < 4; i++)
			protorec.add_attr( (Attr_name)attr[i].name, 
				(Attr_form)attr[i].form, attr[i].value );
		// make_record adds the tag for attr[4]
	}
}

// mask off form part of attribute name
#define NAME_MASK	0xfff0

Attribute *
Dwarf1build::make_record( offset_t offset, int )
{
	short		attrname;
	long		word;
	Attribute	*attribute;
	int		has_type = 0;
	short		tag;
	int		is_inline = 0;
	int		has_location = 0;

	if ( offset == 0 )
	{
		return 0;
	}
	if ( reflist.lookup(offset,attribute) )
	{
		return attribute;
	}

	if ((offset < entry_offset) || (offset >= entry_end))
		return 0;
	ptr = entry_data + offset - entry_offset;

	word = get_4byte();
	// length is not set here, but get_4byte() doesn't care
	if ( word <= 8 )
		return 0;
	length = word - 4;
	nextoff = offset + word;
	tag = get_2byte();
	if (tag > TAG_with_stmt)
		return 0;
	tag_internal = tagname(tag);
	protorec.add_attr( an_tag, af_tag, tag_internal );
	if ( tag != TAG_compile_unit )
		protorec.add_attr( an_parent, af_symbol, 0L );
		// the value of this attribute is reset by
		// Evaluator::add_parent
	if (tag == TAG_global_subroutine || tag == TAG_global_variable)
		protorec.add_attr(an_external, af_int, 1);
	name = 0;
	low_pc = (Iaddr)-1;
	while ( length > 0 )
	{
		attrname = get_2byte();
		switch( attrname )
		{
		case AT_padding:	break;
		case AT_sibling:	sibling(); break;
		case AT_location:	location(); has_location = 1; break;
		case AT_name:		name = get_string(); break;
		case AT_fund_type:	fund_type(); has_type = 1; break;
		case AT_mod_fund_type:	mod_fund_type(); has_type = 1; break;
		case AT_user_def_type:	user_def_type(); has_type = 1; break;
		case AT_mod_u_d_type:	mod_u_d_type(); has_type = 1; break;
		case AT_byte_size:	byte_size(); break;
		case AT_bit_offset:	bit_offset(); break;
		case AT_bit_size:	bit_size(); break;
		case AT_stmt_list:	stmt_list(); break;
		case AT_high_pc:	get_high_pc(); break;
		case AT_subscr_data:	subscr_data(); break;
		case AT_low_pc:		get_low_pc(); break;
		case AT_language: 	set_language(); break;
		case AT_inline:
			is_inline = 1;
			(void)get_byte();
			protorec.add_attr(an_inline, af_int, 1L);
			break;

		case AT_member:
		case AT_containing_type: containing_type(); break;

		case AT_pure_virtual:
		case AT_virtual:	set_virtuality(attrname); break;

		// not handled
		// case AT_ordering:
		// case AT_discr:
		// case AT_discr_value:
		// case AT_string_length:
		// case AT_common_reference:
		// case AT_comp_dir:
		// case AT_const_value:	multiple forms!
		// case AT_default_value:	multiple forms!
		// case AT_friends:
		// case AT_is_optional:
		// case AT_lower_bound:	multiple forms!
		// case AT_program:
		// case AT_private:
		// case AT_producer:
		// case AT_protected:
		// case AT_prototyped:
		// case AT_public:
		// case AT_return_addr:
		// case AT_specification:
		// case AT_start_scope:
		// case AT_stride_size:
		// case AT_upper_bound:	multiple forms!
		default:		
					// element list attributes
					// may have form block1 or
					// block2
					if ((attrname & NAME_MASK) == 
					(AT_element_list & NAME_MASK))
						element_list(attrname, offset);
					else
					{
						skip_attribute(attrname);
					}
					break;
		}
	}

	fixup_name();
	if (tag_internal == t_subroutine || tag_internal == t_entry
		|| tag_internal == t_functiontype)
	{
		// for C and C++, functions with no return type are of type void -
		// needed to distinguish them from functions returning int compiled
		// without debugging information
		if (!has_type)
			protorec.add_attr(an_resulttype, af_fundamental_type, ft_void);
	}
	if ((tag_internal == t_subroutine && low_pc == (Iaddr)-1 && !is_inline)
		|| (tag_internal == t_variable && !has_location))
	{
		// probably a member function or data member declaration
		// defined elsewhere - an_definition will be
		// filled in later by lookup through NameList
		protorec.add_attr(an_definition, af_symbol, 0L);
	}
	attribute =  protorec.put_record();
	reflist.add( offset, attribute );
#if DEBUG
	DPRINT(DBG_DWARF, ("Dwarf1build: offset = %#x\n", offset))
	if (debugflag & DBG_DWARF)
		dumpAttr(attribute, 0);
#endif
	return attribute;
}

int
Dwarf1build::get_syminfo( offset_t offset, Syminfo & syminfo, int )
{
	long	word;
	int	len;
	short	attrname;
	int	loc = 0;
	short	tag;

	if ((offset < entry_offset) || (offset >= entry_end))
		return 0;
	ptr = entry_data + offset - entry_offset;
	word = get_4byte();
	// length is not set here, but get_4byte() doesn't care
	if ( word <= 8 )
	{
		syminfo.type = st_none;
		syminfo.bind = sb_weak;
		syminfo.name = 0;
		syminfo.sibling = offset + word;
		syminfo.child = 0;
		syminfo.lo = 0;
		syminfo.hi = 0;
		syminfo.resolved = 0;
		return 1;
	}
	nextoff = offset + word;
	length = word - 4;
	tag = get_2byte();
	switch ( tag )
	{
	case TAG_padding:
		return 0;
	case TAG_global_variable:
		syminfo.type = st_object;
		syminfo.bind = sb_global;
		break;
	case TAG_global_subroutine:
		syminfo.type = st_func;
		syminfo.bind = sb_global;
		break;
	case TAG_subroutine:
		syminfo.type = st_func;
		syminfo.bind = sb_local;
		break;
	case TAG_local_variable:
	case TAG_formal_parameter:
		syminfo.type = st_object;
		syminfo.bind = sb_local;
		break;
	case TAG_compile_unit:
		syminfo.type = st_file;
		syminfo.bind = sb_none;
		break;
	default:
		syminfo.type = st_none;
		syminfo.bind = sb_none;
		break;
	}
	syminfo.name = 0;
	syminfo.sibling = nextoff;
	syminfo.resolved = 1;
	syminfo.child = 0;
	syminfo.lo = 0;
	syminfo.hi = 0;
	while ( length > 0 )
	{
		attrname = get_2byte();
		switch ( attrname )
		{
		case AT_name:
			syminfo.name = (long)ptr;
			len = ::strlen((char *)ptr) + 1;
			ptr += len;
			length -= len;
			break;
		case AT_sibling:
			word = get_4byte();
			syminfo.sibling = entry_offset + word - entry_base;
			syminfo.child = nextoff;
			break;
		case AT_low_pc:
			syminfo.lo = get_4byte();
			loc = 1;
			break;
		case AT_high_pc:
			syminfo.hi = get_4byte();
			loc = 1;
			break;
		case AT_language:
			set_language();
			break;
		case AT_location:
			loc = 1;
			// FALLTHROUGH
		default:
			skip_attribute( attrname );
		}
	}
	// right now can't handle entries for global declarations
	// that are not definitions
	if (syminfo.bind == sb_global && !loc)
		syminfo.resolved = 0;
	return 1;
}

Lineinfo *
Dwarf1build::line_info( offset_t offset, const FileEntry * )
{
	Lineinfo *	lineinfo;
	long		line;
	Iaddr		pcval;
	Iaddr		base_address;

	lineinfo = 0;
	if ((offset < stmt_offset) || (offset >= stmt_end))
		return 0;
	ptr = stmt_data + offset - stmt_offset;
	{
		length = get_4byte();
		base_address = get_4byte();
		while ( length > 0 )
		{
			line = get_4byte();
			(void)get_2byte(); // line position not currently used
			pcval =  base_address + get_4byte();
			if ( line != 0 )
			{
				protoline.add_line( pcval, line );
			}
			else
			{
				lineinfo = protoline.put_line( pcval );
				break;
			}
		}
	}
	return lineinfo;
}

offset_t		
Dwarf1build::globals_offset() 
{ 
	return first_file();
}
