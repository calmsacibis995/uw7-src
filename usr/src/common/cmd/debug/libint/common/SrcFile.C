#ident	"@(#)debugger:libint/common/SrcFile.C	1.14"
#include "SrcFile.h"
#include "Interface.h"
#include "Vector.h"
#include "global.h"
#include "utility.h"
#include "ProcObj.h"
#include "Process.h"
#include "Program.h"
#include "Language.h"
#include "str.h"
#include "FileEntry.h"

#ifdef SDE_SUPPORT
#include "Path.h"
#endif

#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

int	pathage;
char	*global_path;


// The vector.adds in the contructor set up the 0 and 1 entries
// in the array of offsets.  0 is unused, but is there for
// correct indexing.  The offset of the first line (vector[1])
// is always zero.  That works even for an empty file
// program, global_age, and local_age are needed to differentiate
// between files with the same name in different directories

SrcFile::SrcFile(ProcObj *pobj, FILE *fd, const char *debugname,
	const char *path)
{
	long	number = 0;

	fptr = fd;
	dname = makestr(debugname);
#ifdef SDE_SUPPORT
	fullname = pathcanon(path);
#else
	fullname = makestr(path);
#endif
	hi = 1;
	last = 0;
	progptr = pobj->process()->program();
	g_age = pathage;
	l_age = (int)progptr->pathage();

	vector.add( &number, sizeof( long ) );
	vector.add( &number, sizeof( long ) );
}

// print a warning if the source file has a more recent
// modification date than the object file.
// keeping a linked list of all the source files for which a
// messages has already been printed allows it to avoid printing
// the message for any file more than once.

static void
check_newer(ProcObj *p, const char *fnpath, time_t time)
{
	typedef struct	fileid {
		ino_t		fino;
		dev_t		fent;
		struct fileid	*next;
	} fileid;
	static fileid	*fhead = (fileid *)0;

	struct stat 	stbuf;

	if (!fnpath || !(*fnpath))
		return;

	if (stat(fnpath,&stbuf) == -1) 
	{
		printe(ERR_no_access, E_ERROR, fnpath);
		return;
	}

	if (stbuf.st_mtime > p->program()->symfiltime() || (time && stbuf.st_mtime > time))
	{
		fileid *ptr = fhead;
		for (; ptr; ptr = ptr->next) 
		{
			if (ptr->fent == stbuf.st_dev
				&& ptr->fino == stbuf.st_ino)
				break;
		}
		if (!ptr)
		{
			fileid *nfile = new fileid;

			printe(ERR_newer_file, E_WARNING, fnpath, p->exec_name());
			nfile->fino = stbuf.st_ino;
			nfile->fent = stbuf.st_dev;
			nfile->next = fhead;
			fhead = nfile;
		}
	}
	return;
}

// walk the path, and for each subpath, try to open file name
// in that directory; :: is equivalent to :.:

static SrcFile *
search_path(ProcObj *pobj, const char *path, const char *fname, time_t time)
{
	size_t	len;
	char	buf[PATH_MAX];
	FILE	*fptr;
	SrcFile	*file;

	while (path && *path)
	{
		len = strcspn(path, ":");
		if (len)
		{
			strncpy(buf, path, len);
			buf[len] = '/';
			buf[len+1] = '\0';
		}
		else	// ::
			strcpy(buf, "./");
		strcat(buf, fname);

		if ((fptr = debug_fopen(buf, "r")) != NULL)
		{
			file = new SrcFile(pobj, fptr, fname, buf);
			return file;
		}
		else
			path += (len + 1);
	}
	return 0;
}

// open_srcfile tries to find the file named name in several steps:
// 1) if using cfront and the current language is C++ and the name ends in "..c",
//     replace the "..c" with ".C"
// 2) if the file name is absolute (starts with '/') try opening that file name as is
// 3) create the colon-separated list of directories from the
//	program-specific path and the global path, and search
//	for name in that list.  If that doesn't work, then
// 4) if name is actually a partial path name (has a / in it),
//	search the list of directories again with just the last
//	component of the name
// 5) if fullpath is given, try finding the file using that path
//	(fullpath is the combination of directory and file name from DwarfII)
// 6) if those fail, try just relative to the current directory,
// 7) if debugging cfront code and the current language is C++
//	and if the name ends in ".c",
//  	    	replace the suffix with '.C' and try again
//   	    else if current language is C++ and the name ends in "..c",
//   	   	try again with ".c"


static SrcFile *
open_srcfile(ProcObj *pobj, const FileEntry *fentry, const char *fullpath)
{
	SrcFile		*file = 0;
	char		*npath = 0;
	char		*p, *q;
	char		*name2 = 0;
	int		cplus = 0;
	int		count = 0;
	const char	*name = fentry->file_name;
	int		len = strlen(name);
	FILE		*fptr;

	npath = set_path(pobj->process()->program()->src_path(),
		global_path);

	if (current_language(pobj) == CPLUS_ASSUMED)
		cplus = 1;
	if ((strcmp(name+(len-3), "..c") == 0) && cplus)
	{
		name2 = new char[len+1];
		strcpy(name2, name);
		name2[len-2] = 'C';
		name2[len-1] = 0;
		p = name2;
	}
	else
		p = (char *)name;

	while(1)
	{
		if (*p == '/' && (fptr = debug_fopen(p, "r")) != NULL)
		{
			file = new SrcFile(pobj, fptr, p, p);
		}
		else if ((file = search_path(pobj, npath, p, fentry->time)) == 0)
		{
			if ((q = strrchr(p, '/')) != 0
				 && (file = search_path(pobj, npath, q, fentry->time)) != 0)
				;
			else if (fullpath && (fptr = debug_fopen(fullpath, "r")) != NULL)
				file = new SrcFile(pobj, fptr, p, fullpath);
			// try just relative to current dir
			else if ((fptr = debug_fopen(p, "r")) != NULL)
				file = new SrcFile(pobj, fptr, p, p);
		}
		if (!count && !file && 
			(strcmp(name+(len-2), ".c") == 0) && cplus)
		{
			if (name2)
				delete name2;
			name2 = new char[len+1];

			strcpy(name2, name);
			if (name2[len-3] == '.')
			{
				// "..c"
				name2[len-2] = 'c';
				name2[len-1] = 0;
			}
			else
			{
				name2[len-1] = 'C';
			}
			p = name2;
			count++;
		}
		else
			break;
	}

	if (file)
		check_newer(pobj, file->filename(), fentry->time);
	delete name2;
	delete npath;
	return file;
}

// Search through the list of saved source files for fname.
// If there is no match, or if the either the global path
// or the pre-program path has changed since the file was
// opened, call open_srcfile to do a path search

#define NUMSAVED	10

SrcFile *
find_srcfile(ProcObj *pobj, const FileEntry *fentry)
{
	static SrcFile	*ftab[NUMSAVED];
	static int	nextslot = 0;
	SrcFile		*file;
	char		buf[PATH_MAX];
	const char	*full_path = 0;
	bool		bError = false;
	struct stat	fstat_info, stat_info;
	char		*fname;

	if (!fentry && !fentry->file_name)
		return 0;

	if (fentry->file_name[0] == '/')
		full_path = fentry->file_name;
	else if (fentry->dir_name)
	{
		if (sprintf(buf, "%s/%s", fentry->dir_name, fentry->file_name)
			> PATH_MAX)
		{
			printe(ERR_internal, E_ERROR, "find_srcfile", __LINE__);
			return 0;
		}
		full_path = buf;
	}

	for ( register int i = 0 ; i < NUMSAVED ; i++ ) 
	{
		if ((file = ftab[i]) == 0
			|| file->program() != pobj->process()->program())
			continue;

		// Check whether its the same file, by checking the
		// inode numbers of the current open file and the 
		// path. This check is necessary if the open file
		// is actually moved.
			
		bError = false;

		if (full_path)
		{
			fname = file->filename ();	
		}
		else
		{
			fname = file->name ();
		}
	
		// Retrieve the status information. Errors are handled so
		// that open_srcfile is used to handle them.
		if (!fstat (fileno (file->fileptr ()), &fstat_info)
			|| stat (fname, &stat_info))
		{
			bError = true;
		}

		if (bError || fstat_info.st_ino != stat_info.st_ino)
		{
			delete file;
			ftab[i] = file = open_srcfile(pobj, fentry, 0);
			// Further checks are still neseccary
		}

		// Further checks
		if (full_path && strcmp(full_path, file->filename()) == 0)
			return file;
		else if (strcmp(fentry->file_name, file->name()) == 0)
		{
			if (file->global_age() != pathage
				|| file->local_age()
				!= pobj->process()->program()->pathage())
			{
				delete file;
				ftab[i] = file = open_srcfile(pobj, fentry, 0);
			}
			return file;
		}
	}

	if (ftab[nextslot]) 
		delete ftab[nextslot];
	file = open_srcfile(pobj, fentry, full_path);
	ftab[nextslot] = file;
	if (file)
		if (++nextslot >= NUMSAVED)
			nextslot = 0;

	return file;
}

#define SBSIZE	513

// read a line, up to SBSIZE bytes, from the source file
// if the line is longer than SBSIZE bytes, throw the rest away
// (nobody will want to see all that on their screen anyway)
// the newline is NOT included in the string returned

static char *
readline(FILE *fptr, long &len)
{
	static char	buf[SBSIZE];

	if (fgets(buf, SBSIZE, fptr) == 0)
		return 0;

	len = strlen(buf);
	if (len == (SBSIZE-1) && buf[SBSIZE-2] != '\n')
	{
		int c;
		while ((c = getc(fptr)) != EOF)
		{
			len++;
			if (c == '\n')
				break;
		}
	}
	else
	{
		buf[len - 1] = 0;
	}
	return buf;
}

// read line num from the source file. 
// if we have not read up to this line before from this file,
// we have to walk through
// each line in the file, but line and num_lines build a list of line
// offsets, so the next time line is called, it can seek there directly
// the newline is NOT included in the string returned

char *
SrcFile::line(long num)
{
	long 	*array;
	long	offset;
	long	len;
	char	*ptr;

	if (num <= 0)
		return 0;

	array = (long*) vector.ptr();
	if (num < hi)
	{
		// already know the offset, just go there and read
		if (fseek(fptr, array[num], 0) != 0)
			return 0;
		return readline(fptr, len);
	}

	if (last)
		return 0;

	if (fseek(fptr, array[hi], 0) != 0)
	{
		last = 1;
		return 0;
	}

	offset = array[hi];
	while (hi <= num)
	{
		if ((ptr = readline(fptr, len)) == 0)
		{
			last = 1;
			return 0;
		}
		else
		{
			offset += len;
			vector.add( &offset, sizeof( long ) );
			++hi;
		}
	}
	return ptr;
}

// determine the number of lines available in the file
// starting at start; if start is 0, we start at beginning.
// we only search a maximum of count lines; if count is
// 0 we search to the end

long
SrcFile::num_lines(long start, long count, long &hiline)
{
	long	*array;
	long	offset;
	long	hiwant;
	long	len;

	if (start <= 0)
		start = 1;
	if (count > 0) 
	{
		hiwant = (start + count) - 1;
		if (hiwant < hi)
			return count;
	}
	else 
		hiwant = LONG_MAX;	// big enough

	if (last)
	{
		goto not_enough;
	}

	array = (long*) vector.ptr();
	if (fseek(fptr, array[hi], 0) != 0)
	{
		last = 1;
		goto not_enough;
	}


	offset = array[hi];
	while (hi <= hiwant)
	{
		if (!readline(fptr, len))
		{
			last = 1;
			goto not_enough;
		}
		else
		{
			offset += len;
			vector.add( &offset, sizeof( long ) );
			++hi;
		}
	}
	return count;
not_enough:
	if (start >= hi)
	{
		hiline = hi - 1;
		return 0;
	}
	return hi - start;
}
