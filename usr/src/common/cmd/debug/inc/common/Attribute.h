#ifndef Attribute_h
#define Attribute_h
#ident	"@(#)debugger:inc/common/Attribute.h	1.15"

#include "Language.h"
#include "Itype.h"
#include "Fund_type.h"
#include "Locdesc.h"
#include "Tag.h"	// enum Tag;

// An Attribute is a 2-word struct, consisting of a name, a form, and a value.
// The name is a short, containing one of the members of enum Attr_name.
// The form is also a short, containing one of the members of enum Attr_form.
// The value is a union of several (word-sized) types.
//
// Each attribute name may have one or more forms.  Each form implies one
// member of the "value" union.
//
// An attribute list (the representation of a Symbol) is an array containing
// at least three Attributes:
//
//	the first is always "an_count", "af_int", value.word = number of
//		entries in this list, inclusive
//	the second is always "an_tag", "af_tag", value.tag = tag for this
//		record (see Tag.h and Tag1.h)
//	the last is always "an_nomore", "af_none", value.word = 0
//
// Both a count and an end-marker are used for efficiency in searching for
// a named attribute.  See find_attr() in builder.C.  Only one attribute with
// any given Attr_name is allowed in an attribute list.  This is not enforced,
// but only the first one will ever be found by find_attr().

typedef enum	{
	an_nomore,	// af_none
	an_tag,		// af_tag
	an_name,	// af_stringndx
	an_mangledname,	// af_stringndx
	an_child,	// af_symbol, af_coffrecord, af_dwarfoffs, af_dwarf2offs
	an_sibling,	// ditto, plus af_cofffile
	an_parent,	// af_symbol, af_coffrecord, af_dwarfoffs, af_dwarf2offs
	an_count,	// af_int
	an_type,	// af_fundamental_type, af_symbol, af_coffrecord, af_dwarfoffs
	an_elemtype,	// ditto
	an_elemspan,	// unused
	an_subscrtype,	// af_fundamental_type
	an_lobound,	// af_int
	an_hibound,	// af_int
	an_basetype,	// af_fundamental_type, af_symbol, af_dwarfoffs, af_dwarf2offs
	an_resulttype,	// af_fundamental_type, af_symbol, af_coffrecord, af_dwarfoffs
	an_argtype,	// unused
	an_bytesize,	// af_int
	an_bitsize,	// af_int
	an_bitoffs,	// af_int
	an_litvalue,	// af_int
	an_stringlen,	// unused
	an_lineinfo,	// af_lineinfo, af_coffline, af_dwarfline, af_dwarf2line
	an_location,	// af_locdesc
	an_lopc,	// af_addr
	an_hipc,	// af_addr
	an_visibility,	// unused
	an_scansize,	// af_int
	an_language,	// af_language
	an_assumed_type,  // af_int
	an_subrange_count,	// af_int
	an_prototyped,	// af_int (although existence is sufficient)
	an_comp_dir,	// af_stringndx
	an_file_table,	// af_file_table, af_coffline, af_dwarfline, af_dwarf2line
	an_decl_file,	// af_int
	an_decl_line,	// af_int
	an_specification,	// af_symbol, af_dwarf2offs
	an_vtbl_slot,	// af_locdesc
	an_virtuality,	// af_int
	an_artificial,	// af_int (although existence is sufficient)
	an_containing_type,	// af_symbol, af_dwarf2offs
	an_const_value,	// af_int
	an_definition,	// af_symbol
	an_external,	// af_int (although existence is sufficient)
	an_abstract,	// af_symbol, af_dwarf2offs
	an_sibling_offset,	// af_dwarfoffs, af_dwarf2offs
	an_inline,	// af_int
	an_namespace,	// af_symbol, af_dwarf2offs
	an_using,	// af_symbol, af_dwarf2offs
	an_fake,	// af_int (although existence is sufficient)
} Attr_name;

typedef enum	{
	af_none,
	af_tag,			// value.tag
	af_int,			// value.word
	af_locdesc,		// value.loc
	af_stringndx,		// value.name
	af_coffrecord,		// value.word
	af_coffline,		// value.word
	af_coffpc,		// unused
	af_fundamental_type,	// value.fund_type
	af_symndx,		// unused
	af_reg,			// unused
	af_addr,		// value.addr
	af_local,		// unused
	af_visibility,		// unused
	af_lineinfo,		// value.lineinfo
	af_attrlist,		// unused
	af_cofffile,		// value.word
	af_symbol,		// value.symbol
	af_dwarfoffs,		// value.word
	af_dwarfline,		// value.word
	af_dwarf2offs,		// value.word
	af_dwarf2file,		// value.word
	af_dwarf2line,		// value.word
	af_elfoffs,		// value.word
	af_language,		// value.language
	af_file_table,		// value.file_table
} Attr_form;

struct Attribute;
struct Lineinfo;
struct FileTable;

union Attr_value {
	Iaddr		addr;
	Fund_type	fund_type;
	Lineinfo       *lineinfo;
	Addrexp		loc;
	const char     *name;
	Attribute      *symbol;
	Tag		tag;
	Language	language;
	long		word;
	FileTable	*file_table;
};

struct Attribute {
	short		name;	// compact version of Attr_name
	short		form;	// compact version of Attr_form
	Attr_value	value;
}; 

#if DEBUG
void dumpAttr(Attribute *root, int indent=1);
#endif

#endif /* Attribute_h */
