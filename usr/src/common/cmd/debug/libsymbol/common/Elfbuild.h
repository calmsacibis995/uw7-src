#ident	"@(#)debugger:libsymbol/common/Elfbuild.h	1.5"
#ifndef Elfbuild_h
#define Elfbuild_h

// Interface to ELF symbol tables - used when no debugging
// information is available for a given address range.
//
// Uses the ELF class for low-level access to ELF files.

#include	"Build.h"
#include	"Protorec.h"
#include	"Reflist.h"

struct Syminfo;
struct Attribute;
class  ELF;

class Elfbuild : public Build {
	Protorec	protorec;
	long		losym, hisym;
	long		histr;
	char *		symptr;
	char *		strptr;
	ELF*		object;
	Reflist		reflist;
	long		special;	// offset of first symbol
					// past _fake_hidden

	void		find_arcs(Syminfo &);
public:
			Elfbuild( ELF * );
			~Elfbuild() {};
	offset_t	globals_offset();
	offset_t	first_symbol();
	int		get_syminfo( offset_t offset, Syminfo &, int ignored = 0 );
	Attribute *	make_record( offset_t offset, int ignored = 0 );
	char *		get_name( offset_t offset );
	void		set_special(offset_t offset) { special = offset; }
	Lineinfo	*line_info(offset_t offset, const FileEntry *) { return 0; }
	int		out_of_range(offset_t);
};

#endif /* Elfbuild_h */
