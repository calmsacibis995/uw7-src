#ifndef Source_h
#define Source_h
#ident	"@(#)debugger:inc/common/Source.h	1.5"

#include	"Iaddr.h"

struct	Lineinfo;
struct	FileEntry;
struct	FileTable;

class Source {
	Lineinfo	*lineinfo;
	FileTable	*file_table;
	Iaddr		ss_base;
	friend class	Symbol;
	const FileEntry	*get_values(long index, long &line, Iaddr *stmt_start);
	int		check_file(const FileEntry *, const char *name);
public:
			Source();
			Source(const Source& );
	int		isnull() { return (ss_base == 0 && lineinfo == 0); }
	void		null() { ss_base = 0; lineinfo = 0; }
	const FileEntry	*pc_to_stmt(Iaddr pc, long& line, int slide = -1,
				Iaddr *stmt_start = 0);
	const FileEntry	*find_header(const char *header);
	const FileEntry	*find_header(int);
	const FileEntry	*primary_file_entry();
	void		stmt_to_pc( long line, const char *file, Iaddr& pc, int slide = 0 );
	Source &	operator=( const Source & );
};

#endif

// end of Source.h

