#ident	"@(#)debugger:libsymbol/common/Dwarf2.h	1.9"
#ifndef Dwarf2_h
#define Dwarf2_h

// Interface to DWARF debugging information.  Uses
// the ELF class for low-level access to the file itself.
//
// This class handles DWARF Version 2.

#include	"Attribute.h"
#include	"Build.h"
#include	"Dwarfbuild.h"
#include	"Vector.h"
#include	"Language.h"
#include	<stddef.h>
#include	<libdwarf2.h>

struct	Syminfo;
class	ELF;
struct	Dwarf2_Abbreviation;
struct	Dwarf2_Attribute;
struct	FileEntry;
struct	Lineinfo;
struct	FileTable;
class	Evaluator;

typedef unsigned long	form_t;


// Dwarf2 offsets are all zero based, from the beginning of the section
// (offset_t)-1 indicates an invalid offset

class Dwarf2build : public Dwarfbuild {
	Vector			abbrev_tables;

	byte			*info_ptr;
	byte			*abbrev_ptr;
	size_t			abbrev_size;
	byte			*line_ptr;
	offset_t		line_size;
	size_t			addr_size;
	
	offset_t		curr_file_offset;
	offset_t		curr_file_end;
	int			curr_file_index;

	offset_t		sibling;
	unsigned long		byte_size;	// save byte_size and encoding for
	unsigned short		encoding;	// fixup_basetype()
	bool			has_type;
	bool			has_specification;
	bool			out_of_line_inline;
	int			level;

	unsigned long		get_constant(form_t);

	void			get_language(form_t);
	void			get_sibling(form_t);
	void			get_location(form_t, Attr_name);

	void			process_attribute(const Dwarf2_Attribute *);
	void			skip_attribute(form_t form);

	Dwarf2_Abbreviation	*find_table(offset_t);
	Dwarf2_Abbreviation	*get_abbreviation_table(offset_t);
	Dwarf2_Abbreviation	*process_file(offset_t &offset);

	void			fill_in_syminfo(Syminfo &, Dwarf2_Abbreviation *,
						Dwarf2_Abbreviation *,
						int level, int &has_location);

	void			fixup_basetype();

public:
				Dwarf2build( ELF *, Evaluator * );
				~Dwarf2build();

	int		get_syminfo(offset_t offset, Syminfo &, int new_file);
	offset_t	globals_offset();
	Attribute	*make_record(offset_t offset, int new_file = 0);
	Lineinfo	*line_info(offset_t offset, const FileEntry *);
	FileTable	*process_includes(offset_t offset, const char *comp_dir);
	Language	language();
};

#endif /* Dwarf2_h */
