#ifndef Lineinfo_h
#define Lineinfo_h
#ident	"@(#)debugger:inc/common/Lineinfo.h	1.3"

#include "Iaddr.h"

struct FileEntry;

struct LineEntry
{
	Iaddr		addr;
	long		linenum;
	const FileEntry	*file_entry;
};

struct Lineinfo
{
	int		entrycount;
	LineEntry	*addrpart;
	LineEntry	*linepart;
};

#define BIG_LINE	1000000000

#endif

// end of Lineinfo.h

