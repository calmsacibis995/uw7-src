#ident	"@(#)debugger:libsymbol/common/Dwarfbuild.h	1.7"
#ifndef Dwarfbuild_h
#define Dwarfbuild_h

// Interface to DWARF debugging information.  Uses
// the ELF class for low-level access to the file itself.
//

#include	"Attribute.h"
#include	"Build.h"
#include	"Protorec.h"
#include	"Reflist.h"
#include	"Protoline.h"
#include	"Iaddr.h"
#include	"Tag.h"
#include	"Language.h"
#include	<libdwarf2.h>

class Evaluator;

class Dwarfbuild : public Build
{
protected:
	Protorec	protorec;
	Protoline	protoline;
	Reflist		reflist;

	offset_t	entry_offset;
	offset_t	entry_end;
	size_t		length;

	Iaddr		low_pc;
	byte		*ptr;
	Tag		tag_internal;
	Language	curr_lang;
	Evaluator	*evaluator;
	const char	*name;	// for fixup_name()

	unsigned char	get_byte();
	unsigned short	get_2byte();
	unsigned long	get_4byte();
#if LONG_LONG
	unsigned long long	get_8byte();
#endif

			// should be machine specific?
	Iaddr		get_address()	{ return (Iaddr) get_4byte(); }

	void		get_low_pc();
	void		get_high_pc();

	Tag		tagname(unsigned int);
	Attr_name	type_name();
	Addrexp		make_chunk(void *p, int length);
	void		fixup_name();

public:
			Dwarfbuild(Evaluator *);
			~Dwarfbuild();

	offset_t	first_file() { return entry_offset; }
	offset_t	last_entry() { return entry_end; }
	int		out_of_range(offset_t);

	Language	language() { return curr_lang; }
	Attribute	*find_record(offset_t);
};

#endif /* Dwarfbuild_h */
