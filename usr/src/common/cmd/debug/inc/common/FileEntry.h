#ifndef FILEENTRY_H
#define FILEENTRY_H
#ident	"@(#)debugger:inc/common/FileEntry.h	1.4"

#include "Severity.h"
#include <time.h>

class	ProcObj;
class	Symtab;
class	Symbol;

// information needed to find a source file

struct FileEntry
{
	const char	*file_name;
	const char	*dir_name;
	time_t		time;	// time of last modification

			FileEntry()	{ file_name = dir_name = 0; time = 0; }
			FileEntry(const char *fname)
					{ file_name = fname; dir_name = 0; time = 0; }
	void		reset(const char *fname)
					{ file_name = fname; dir_name = 0; time = 0; }
	const char	*get_qualified_name(Symbol &) const;
	int		compare(Symbol &) const;
};

struct FileTable
{
	int		filecount;
	FileEntry	*files;

			~FileTable() { delete files; }
};

const FileEntry *find_file_entry(ProcObj *, Symtab *, const char *file_name,
	const char *header_name = 0, Severity = E_ERROR);
const FileEntry *find_file_entry(ProcObj *, Symbol &, const char *file_name,
	const char *header_name = 0, Severity = E_ERROR);

#endif // FILEENTRY_H
