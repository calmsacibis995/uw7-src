#ifndef SrcFile_h
#define SrcFile_h
#ident	"@(#)debugger:inc/common/SrcFile.h	1.6"

#include	"Vector.h"
#include	<stdio.h>

class ProcObj;
class Program;
struct FileEntry;

class SrcFile {
	Vector		vector;	// list of file offset for the beginning of
				// lines, indexed by line numbers
	FILE		*fptr;
	Program		*progptr;	// program the source file is
					// associated with
	int		hi;		// one beyond highest line number read so far
	int		last;		// indicates hi-1 is end-of-file
	int		g_age;		// age of global path at SrcFile creation
	int		l_age;		// age of local path at SrcFile creation
	char		*fullname;	// file path name
	char		*dname;		// name from debug info

public:
			SrcFile(ProcObj *, FILE *, const char *debugname,
					const char *fullpath);
			~SrcFile()	{ if (fptr) fclose(fptr); delete dname; delete fullname; }

	char		*filename()	{ return fullname; }
	char		*name()		{ return dname; }
	Program		*program()	{ return progptr; }
	int		global_age()	{ return g_age; }
	int		local_age()	{ return l_age; }
	FILE		*fileptr ()	{ return fptr; }

	char		*line(long linenum);
	long		num_lines(long start, long count, long &hi);
			// number of lines starting with start,
			// up to a maximum of count
			// if start is beyond end, returns total
			// number of lines in hi
};

SrcFile	*find_srcfile(ProcObj *, const FileEntry * = 0);

#endif
