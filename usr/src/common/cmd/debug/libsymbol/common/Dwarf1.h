#ident	"@(#)debugger:libsymbol/common/Dwarf1.h	1.5"
#ifndef Dwarf1_h
#define Dwarf1_h

// Interface to DWARF debugging information.  Uses
// the ELF class for low-level access to the file itself.
//
// This class handles DWARF Version 1.

// Dwarf records consist of tag-value pairs, with the tag
// specifying how to interpret the value.  Each record
// points to a sibling record.  Records between the current
// record and its sibling are children of the current record.
//
// Each dwarf entry consists of a 4-byte length, specifiying
// the length of the entire record; a 2-byte tag and an attribute
// list.  Each attribute may have 1 of 7 forms:
// 1) 4-byte address (relocated)
// 2) 4-byte reference to another dwarf entry
// 3) 2-byte datum
// 4) 4-byte datum
// 5) 2-byte length followed by length bytes
// 6) 4-byte length followed by length bytes
// 7) n bytes, 0-terminated
//
// Dwarf line number entries consist of tables of entries;
// each table refers to the entries for a given address range
// (usually a single dot-o).  Each table has a 4-byte length,
// followed by a 4-byte address, representing the lowest pc
// of the following entries.  This is followed by an array of
// entries, each with a 4-byte line number, a 2-byte intra-line
// position (currently unused) and a 4-byte virtual address.
// The last entry in each table has line number 0.
 

#include	"Attribute.h"
#include	"Build.h"
#include	"Protorec.h"
#include	"Reflist.h"
#include	"Protoline.h"
#include	"Iaddr.h"
#include	"Dwarfbuild.h"

struct	Syminfo;
class	ELF;
class	Evaluator;

class Dwarf1build : public Dwarfbuild {
	Protorec	prototype;
	Iaddr		entry_base;
	offset_t	stmt_offset, stmt_end;
	Iaddr		stmt_base;
	byte		*entry_data, *stmt_data;
	offset_t	nextoff;

	void		skip_attribute( short attrname );
	const char	*get_string();
	void		next_item( Attribute * );
	int		subscr_list( Attribute * );
	void		get_location( Attr_form &, Attr_value & );
	void		get_ft( Attr_form &, Attr_value & );
	void		get_udt( Attr_form &, Attr_value & );
	void		get_mft( Attr_form &, Attr_value & );
	void		get_mudt( Attr_form &, Attr_value & );
	void		sibling();
	void		location();
	void		fund_type();
	void		mod_fund_type();
	void		user_def_type();
	void		mod_u_d_type();
	void		byte_size();
	void		bit_offset();
	void		bit_size();
	void		stmt_list();
	void		high_pc();
	void		element_list(short attrname, offset_t offset);
	void		subscr_data();
	void		set_language();
	void		containing_type();
	void		set_virtuality(short);

public:
			Dwarf1build( ELF *, Evaluator * );
			~Dwarf1build();

	Attribute *	make_record( offset_t offset, int ignored = 0 );
	Lineinfo *	line_info(offset_t  offset, const FileEntry * = 0);
	int		get_syminfo( offset_t offset, Syminfo &, int ignored );
	offset_t	globals_offset();
};

#endif /* Dwarf1_h */
