#ident	"@(#)debugger:gui.d/i386/CoreInfo.C	1.5"

#ifndef GEMINI_ON_OSR5

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include "Machine.h"
#include "CoreInfo.h"
#include "Proctypes.h"

#ifndef OLD_PROC
#include <sys/core.h>
#endif

static char *get_psargs(char *note, int size);

ELFcoreInfo::ELFcoreInfo(int fd, Elf_Ehdr *ehdrP)
{
	int phdrnum = ehdrP->e_phnum;
	if(!phdrnum)
		return;
        // read program header
	int phdrsz = phdrnum*ehdrP->e_phentsize;
	char *phdr = new char[phdrsz];
	if (lseek(fd, (long)ehdrP->e_phoff, SEEK_SET) == -1 ||
	    read(fd, phdr, phdrsz) != phdrsz)
	{
		delete phdr;
                return;
	}
        // look through all note sections
	// this is necessary since ES/MP has LWP specific note
	// sections, we're only interested in the one for the
	// the process as a whole, i.e. the one that contains
	// the CF_T_PRPSINFO entry.
        Elf_Phdr        *pptr = (Elf_Phdr *)phdr;
        int             i;
	note_data = 0;
	psargs = 0;
        for (i = 0; i < phdrnum; i++, pptr++)
        {
                if (pptr->p_type != PT_NOTE)
                        continue;
		// read note section
        	note_sz = (int)pptr->p_filesz;
		if (note_data)
			delete note_data;
		note_data = new char[note_sz];
		if (lseek(fd, (long)pptr->p_offset, SEEK_SET) == -1 ||
		    read(fd, note_data, note_sz) != note_sz)
		{
			delete note_data;
			note_data = NULL;
			note_sz = 0;
			break;
		}
		// find args used to invoke the process
		if (psargs = get_psargs(note_data, note_sz))
			break;
        }
	delete phdr;
	obj_name = NULL;
}

ELFcoreInfo::~ELFcoreInfo()
{
	if (note_data)
		delete note_data;
}

static char *
get_psargs(char *note, int size)
{
	int namesz, descsz, type;

	// find psargs in note section
	while(size > 0)
	{
		namesz =  *(int *)note; note += sizeof(int);
                descsz = *(int *)note; note += sizeof(int);
                type   = *(int *)note; note += sizeof(int);
                size -= 3 * sizeof(int) + namesz + descsz;
                note += namesz;
#ifdef OLD_PROC
                if (type == 3)
                        return ((prpsinfo_t *)note)->pr_psargs;
#else
                if (type == CF_T_PRPSINFO)
                        return ((psinfo_t *)note)->pr_psargs;
#endif
                note += descsz;
                int mod = (int)note % sizeof(int);
                if (mod)
                        note += sizeof(int) - mod;
        }
	return NULL;
}

#else	// GEMINI_ON_OSR5

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>

#include "Machine.h"
#include "Proctypes.h"
#include "CoreInfo.h"
#include "sys/paccess.h"

static int
read_section(int coref, uoff off, char *sect, size_t sz)
{
	if (lseek(coref, off, SEEK_SET) == -1)
	{
		return 0;
	}
	if (::read(coref, sect, sz) != sz)
	{
		return 0;
	}
	return 1;
}

ELFcoreInfo::ELFcoreInfo(int fd, Elf_Ehdr *)
{
	struct coreoffsets	coffs;
	struct coresecthead	chead;
	char			*ubytes = 0;

	psargs  = 0;

	if (!read_section(fd, 0, (char *)&chead, 
		sizeof(struct coresecthead)))
	{
		return;
   	}
	if (chead.cs_stype != CORES_MAGIC ||
		chead.cs_x.csx_magic != COREMAGIC_NUMBER)
	{
		return;
	}
	if (!read_section(fd, chead.cs_hseek, (char *)&chead, 
		sizeof(struct coresecthead)))
	{
		return;
   	}
	if (chead.cs_stype != CORES_OFFSETS)
	{
		return;
	}
	if (!read_section(fd, chead.cs_sseek, (char *)&coffs, 
		chead.cs_vsize))
	{
		return;
	}

	ubytes = new char[coffs.u_usize];
	if (!read_section(fd, coffs.u_user, ubytes, coffs.u_usize))
	{
		delete ubytes;
		return;
	}
	psargs = new char[PSARGSZ];
	strncpy(psargs, (ubytes + PSARG_OFF), PSARGSZ);
	psargs[PSARGSZ-1] = 0;
	delete ubytes;
}

ELFcoreInfo::~ELFcoreInfo()
{
	if (psargs)
		delete psargs;
}
#endif

char *
ELFcoreInfo::get_obj_name()
{
	if(obj_name)
		return obj_name;
#ifdef GEMINI_ON_OSR5
	if(!psargs)
#else
	if(!note_data || !psargs)
#endif
	{
		obj_name = "";
		return obj_name;
	}
	// return argv[0]
	char *p;
	for(obj_name = p = psargs; *p; ++p)
	{
		if(isspace(*p))
		{
			*p = '\0';
			break;
		}
	}
	return obj_name;
}
