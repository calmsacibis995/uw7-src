#ident	"@(#)debugger:libsymbol/common/Coffbuild.h	1.6"

#ifndef Coffbuild_h
#define Coffbuild_h

// Interface to the COFF symbol tables - converts COFF
// symbol table entries and line number entries into
// the internal format
//
// Uses a file descriptor for access to the COFF file,
// reads the line number entries, symbol table, and
// string table in each as one contigous chunk.
// Deals with symbol table entries one at a time
// through make_record and get_syminfo, handles line
// number info a file at a time through line_info

#include	"Build.h"
#include	"Coff.h"
#include	"Protorec.h"
#include	"Itype.h"
#include	"Reflist.h"
#include	"Protoline.h"

struct Syminfo;

class Coffbuild : public Build {
	Coff		*coff;		// low level routines read the file
	Protorec	protorec;
	Protorec	prototype;
	Protoline	protoline;
	Reflist		reflist;
	offset_t	nextofs;	// offset of symbol past current one
	offset_t	linedisp;	// file offset of line number entries
	offset_t	symtab_offset;	// beginning of the symbol table
	offset_t	end_offset;
	int		textsectno;
	struct syment	sym;
	union auxent	aux;

	void		find_arcs( offset_t & sibofs, offset_t & childofs );
	void		get_arcs();
	void		get_data();
	void		get_type_C();
	void		get_type();
	void		get_addr_C();
	void		get_addr();
	int		coff_find_record( offset_t offset, int want_file );
	void		get_lineinfo( offset_t loffset, size_t lncnt, offset_t foffset );
	const char *	get_fcn_lineinfo( const char * start, const char * end );
	int		has_line_info( offset_t offset ); // does this file
						// have line information?
public:
			Coffbuild( Coff * );
			~Coffbuild();

	Attribute *	make_record( offset_t offset, int want_file = 0 );
	int		get_syminfo( offset_t offset, Syminfo & syminfo, int ignored = 0 );
	offset_t	first_symbol();
	offset_t	globals_offset();
	Lineinfo *	line_info( offset_t offset, const FileEntry * = 0 );
	void		get_pc_info( offset_t offset, Iaddr & lopc, Iaddr & hipc );
	int		out_of_range( offset_t );
};

extern int	 coff_bit_loc(int, int, int, int &, int &);

#endif /* Coffbuild_h */
