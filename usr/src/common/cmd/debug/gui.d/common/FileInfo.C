#ident	"@(#)debugger:gui.d/common/FileInfo.C	1.6"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <filehdr.h>
#include "Machine.h"
#include "FileInfo.h"
#include "CoreInfo.h"

static int is_strntext(const char *, int);
static FileType coff_type(const int fd);

FileInfo::FileInfo(char *name)
{
	Elf_Ehdr	elfhdr;
	int		nbytes;

	fname = name;
	core_info = NULL;
	ftype = FT_UNKNOWN;
	fd = open(name, O_RDONLY);
	if(fd < 0)
		// error
		return;
	nbytes = read(fd, &elfhdr, sizeof(elfhdr));
	if (nbytes > sizeof(elfhdr))
		// error
		return;
	if (nbytes < sizeof(elfhdr) || 
	    strncmp((char *)elfhdr.e_ident, ELFMAG, SELFMAG) != 0)
	{
		if (*(short *)elfhdr.e_ident == COFFMAGIC)
			ftype = coff_type(fd);
		else if (is_strntext((char *)elfhdr.e_ident, nbytes))
			ftype = FT_TEXT;
		// otherwise unknown type
		return;
	}
	// ELF file
	if (elfhdr.e_version != EV_CURRENT)
		return;
	switch(elfhdr.e_type)
	{
	case ET_EXEC:
		ftype = FT_EXEC;
		break;
	case ET_CORE:
		ftype = FT_CORE;
		// get core info
		core_info = new ELFcoreInfo(fd, &elfhdr);
		break;
	}
}

static FileType
coff_type(const int fd)
{
	struct filehdr fhdr;

	// seek to beginning of file and read file hdr
	if (lseek(fd, (long)0, SEEK_SET) == -1 ||
	    read(fd, &fhdr, sizeof(struct filehdr)) != sizeof(struct filehdr))
	{
		// error
		return FT_UNKNOWN;
	}
	// since COFF core files don't have a header (i.e. it's just
	// u-block followed by process image), there's no way to know
	// for sure that we have a COFF core file. just punt for now.
	return (fhdr.f_flags & F_EXEC) ? FT_EXEC : FT_UNKNOWN;
}

static int
is_strntext(const char *sp, int cnt) 
{
	int i;
	for(i = 0; i < cnt; ++i,++sp)
	{
		int c = *sp;
		if(!isgraph(c) && !isspace(c))
			return 0;
	}
	return 1;
}

FileInfo::~FileInfo()
{
	if (fd >= 0)
		close(fd);
	delete core_info;
}

char *
FileInfo::get_obj_name()
{
	switch(ftype)
	{
	case FT_EXEC:
		return fname;
	case FT_CORE:
		return core_info->get_obj_name();
	}
	return "???";
}

