#ident	"@(#)debugger:libsymbol/common/Coff.h	1.2"
#ifndef	Coff_h
#define	Coff_h

//	Common Object File Format access routines.
//
//	The file header and section headers are read in and saved when
//	the file is first accessed.  The line number entries, symbol
//	table entries, and string table are each read in as one big
//	block and saved when any entry is first referenced.  The
//	individual entries are handed out one at a time through
//	get_lineno and get_symbol

#include "Iaddr.h"
#include "Object.h"
#include <a.out.h>
#include <sys/types.h>

class Coff : public Object {
	int		numscns;
	SCNHDR		*section_headers;
	char		*get_string( long offset );
public:
			Coff( int fdobj, dev_t, ino_t, time_t );
			~Coff() { delete section_headers; }
	int		sectno( const char * sname );
	long		get_symtab_offset();
	long		get_end_of_symtab();
	long		get_line_offset();
	long		get_symbol(long offset, struct syment &,
				union auxent &);
	char 		*get_lineno(long offset);
	Seginfo		*get_seginfo(int &count, int &isshared);
};

#endif /* Coff_h */
