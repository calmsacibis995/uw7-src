#ident	"@(#)debugger:libsymbol/common/Object.C	1.12"

#include "Object.h"
#include "Coff.h"
#include "ELF.h"
#include "ProcObj.h"
#include "Symtable.h"
#include "Interface.h"
#include "Machine.h"
#include "Iaddr.h"
#include "global.h"
#include "utility.h"
#include "List.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/elf.h>
#include <a.out.h>
#include <string.h>

#if defined(USES_SVR3_CORE) && defined(HAS_PACCESS)
#include "sys/paccess.h"
#endif

static List	objectlist;

Object::Object( int fd, dev_t dev, ino_t ino, time_t otime )
{
	fdobj = debug_dup(fd);
	obj_time = otime;
	device = dev;
	inumber = ino;
	seginfo = 0;
	start = 0;
	file_form = ff_none;
	flags = 0;
	symtable = 0;
	foreign = 0;
	objectlist.add(this);
	for(int i = s_interp; i < s_last; i++)
		sections[i] = 0;
}

Object::~Object()
{
	mem_map	*mm;

	delete seginfo;
	for(int i = s_interp; i < s_last; i++)
	{
		if (!sections[i])
			continue;

		Sectinfo	*tmp;
		while (sections[i]->next)
		{
			tmp = sections[i]->next;
			sections[i]->next = tmp->next;
			delete tmp;
		}
		delete(sections[i]);
	}
	mm = (mem_map *)memlist.first();
	while(mm)
	{
		if (mm->m_type == mt_read)
		{
			delete((char *)mm->m_start);
		}
		else if (mm->m_type == mt_map)
		{
			munmap((char *)mm->m_start,(size_t)mm->m_size);
		}
		mm = (mem_map *)memlist.next();
	}
	close(fdobj);
	objectlist.remove(this);
}


int
locate_obj( int fdobj, Object*&obj )
{
	Object		*s;
	struct stat	newstat;
	union {
		char		lmagic[SELFMAG];
		short		smagic;
#if defined(USES_SVR3_CORE) && defined(HAS_PACCESS)
		struct coresecthead	chead;
#endif
	} magic;

	if (( fdobj == -1 ) || fstat(fdobj, &newstat) )
	{
		obj = 0;
		return 0;
	}
	if (objectlist.first())
	{
		do
		{
			s = (Object *)objectlist.item();
			if (newstat.st_dev == s->device && 
				newstat.st_ino == s->inumber &&
				newstat.st_mtime == s->obj_time)
			{
				obj = s;
				return 1;
			}
		} while (objectlist.next());
	}

	if ((lseek( fdobj, 0L, SEEK_SET ) == -1) ||
		(read( fdobj, (char*)&magic, sizeof(magic)) != sizeof(magic)))
	{
		printe(ERR_obj_file_read, E_ERROR, sizeof(magic), 
			(unsigned long)0);
		return 0;
	}
	if ( strncmp(magic.lmagic,ELFMAG,SELFMAG) == 0 )
	{
		obj = (Object *)new ELF(fdobj, newstat.st_dev,
			newstat.st_ino, newstat.st_mtime);
	}
	else if ( magic.smagic == COFFMAGIC )
	{

		obj = (Object *)new Coff(fdobj,newstat.st_dev,
			newstat.st_ino, newstat.st_mtime);
	}
#if defined(USES_SVR3_CORE) && defined(HAS_PACCESS)
	else if ((magic.chead.cs_stype == CORES_MAGIC) &&
		(magic.chead.cs_x.csx_magic == COREMAGIC_NUMBER))
	{
		obj = new Object(fdobj,newstat.st_dev,
			newstat.st_ino, newstat.st_mtime);
		obj->file_form = ff_coff_core;
	}
#elif defined(USES_SVR3_CORE)
	else 
		obj->file_form = ff_coff_core;
#endif
#if !defined(USES_SVR3_CORE) || defined(HAS_PACCESS)
	else
	{
		printe(ERR_obj_file_form, E_ERROR);
		obj = 0;
		return 0;
	}
#endif
	if (obj->file_form == ff_none)
	{
		delete obj;
		obj = 0;
		return 0;
	}
	return -1;
}

Object *
find_object(int fdobj, const char *name )
{
	Object *obj;
	int	i;

	
	if ((i = locate_obj(fdobj, obj)) == 0)
	{
		return 0;
	}

	if ((i < 0) && name)
	{
		if (!obj->is_shared() && !obj->has_debug_info())
			printe(ERR_no_dashg, E_WARNING, name);
	}
	return obj;
}

int		
Object::getsect( Sect_type sect, Sectinfo *sinfo)
{
	if (sect < s_interp || sect >= s_last || !sections[sect])
		return 0;
	
	memcpy((char *)sinfo, (char *)sections[sect], sizeof(Sectinfo));
	return 1;
}

#define SHLBMAX 10

static char *stsl_names[SHLBMAX];

//
// read .lib section, populate stsl_names with static shared libs pathnames.
// layout of each .lib entry is:
//	total	(4bytes;total * sizeof(long) is bytes in entry
//	topath  (4bytes;topath * sizeof(long) is bytes from beginning
//			of entry to path
//	extra	(n bytes)
//	path	(null terminated; padded to long boundary)
//

char **
Object::get_stsl_names()
{
	char		*slptr, *slbuf;
	int		i, total, topath, len;
	long		size;
	Sectinfo	*s = sections[s_lib];

	if (!s)
		return 0;
	slbuf = (char *)s->data;
	size = s->size;
	slptr = slbuf;
	i = 0;
	while ((slptr < slbuf + size) && (i < SHLBMAX))
	{
		total = *(int *)slptr;	
		topath = *(int *)(slptr + sizeof(long));
		len = (total - topath) * sizeof(long);
		stsl_names[i] = new(char[len]);
		(void) memcpy(stsl_names[i++], slptr + (topath * sizeof(long)),
			len);
		slptr += total * sizeof(long);	/* path for next lib */
	}
	return stsl_names;
}

// allocate space for and read data
int
Object::read_data(void *&dest, long offset, long size, int nodelete)
{
	int	i;
	dest = new(char[size]);
	if ((i = read(offset, dest, size)) <= 0)
	{
		printe(ERR_obj_file_read, E_ERROR, size, offset);
		delete dest;
		return 0;
	}
	if (!nodelete)
	{
		mem_map	*mm = new mem_map;
		mm->m_start = dest;
		mm->m_size = size;
		mm->m_type = mt_read;
		memlist.add(mm);
	}
	return i;
}

// map data; data is mapped READ-ONLY
// add to memlist
// if mmap fails, try malloc/read

int
Object::map_data(void *&dest, long offset, long size)
{
	long	mapoff;
	long	mapsize;
	void	*mapdest;
	mem_map	*mm;

	// make sure offset is aligned to pagesize
	mapoff = PTRUNC(offset);
	mapsize = size + (offset - mapoff);
	mapdest = mmap(0, (size_t)mapsize, PROT_READ, MAP_SHARED,
		fdobj, mapoff);
	if (mapdest == (char *)-1)
	{
		return(read_data(dest, offset, size, 0));
	}
	dest = (void *)((Iaddr)mapdest + (mapsize - size));
	mm = new mem_map;
	mm->m_start = mapdest;
	mm->m_size = mapsize;
	mm->m_type = mt_map;
	memlist.add(mm);
	return 1;
}

// null base class versions of virtual functions

Seginfo *
Object::get_seginfo( int &, int &)
{
	return 0;
}

// check whether an object given to create is actually
// a #! shell script - if so, return the #! command line
#define	INTPSZ	256 // max size of #! line allowed by OS
char *
check_shell_script(int fd)
{
	char	*ptr, *cmd, *newcmd;
	char	buf[INTPSZ+1];
	int	sz;
	if ((lseek( fd, 0L, SEEK_SET ) == -1) ||
		((sz = read( fd, buf, INTPSZ)) <= 0))
	{
		printe(ERR_obj_file_read, E_ERROR, 4,
			(unsigned long)0);
		return 0;
	}
	if (buf[0] != '#' || buf[1] != '!')
		return 0;

	buf[sz] = 0;
	if ((ptr = strchr(&buf[2], '\n')) == 0)
		return 0;
	*ptr = 0;
	cmd = &buf[2];
	// skip leading whitespace
	while(*cmd && (*cmd == ' ' || *cmd == '\t'))
		cmd++;
	if (!*cmd)
		return 0;
	newcmd = new char[(ptr - cmd) + 1];
	strcpy(newcmd, cmd);
	return newcmd;
}

int
Object::read(Iaddr from, void *to, int len)
{
	int	result;
	do {
		errno = 0;
		lseek(fdobj, from, SEEK_SET);
	} while(errno == EINTR);
	if (errno)
		return -1;
	do {
		errno = 0;
		result = ::read(fdobj, to, len);
	} while(errno == EINTR);
	return result;
}
