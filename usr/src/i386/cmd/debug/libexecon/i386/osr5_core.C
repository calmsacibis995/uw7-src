#ident	"@(#)debugger:libexecon/i386/osr5_core.C	1.3"

#ifdef GEMINI_ON_OSR5

#include "Proccore.h"
#include "Proctypes.h"
#include "Interface.h"
#include "Reg1.h"
#include <elf.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "sys/regset.h"
#include "sys/paccess.h"
#include "utility.h"
#include "global.h"

struct CoreData {
	Elf_Phdr	*phdr;
	int		numphdr;
	fpregset_t	*fpregs;	
	gregset_t	gregs;
	char 		*psinfo;
	Iaddr		lo_stack;
	Iaddr		hi_stack;
			CoreData() { memset( (char *)this, 0, 
					sizeof(CoreData) ); }
			~CoreData();
};

CoreData::~CoreData()
{
	delete(phdr);
	delete(psinfo);
}


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

Proccore::Proccore()
{
	data = 0;
}

struct core_region {
	Iaddr		vaddr;
	unsigned int	off;
	unsigned int	size;
	short		flags;
	short		type;
	core_region	*next;
};

int
Proccore::open( int cfd )
{
	struct coreoffsets	coffs;
	struct coresecthead	chead;
	char			*ubytes = 0;
	bool			bReadCF = false;
	core_region		*regions = 0;
	core_region		*rtail = 0;
	core_region		*rptr;
	int			nregions = 0;
	Elf_Phdr		*pptr;
	unsigned int		reg_off;

	if ((fd = debug_dup(cfd)) == -1)
		return 0;
	data = new CoreData;
	if (!read_section(fd, 0, (char *)&chead, 
		sizeof(struct coresecthead)))
	{
		printe(ERR_core_access, E_ERROR, strerror(errno));
		goto err;
   	}
	if (chead.cs_stype != CORES_MAGIC ||
		chead.cs_x.csx_magic != COREMAGIC_NUMBER)
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	}

	// the coresecthead'ers form a sort of linked list
	while(chead.cs_hseek > 0)
	{
		if (!read_section(fd, chead.cs_hseek, (char *)&chead,
			sizeof(struct coresecthead)))
		{
			printe(ERR_core_access, E_ERROR, 
				strerror(errno));
			goto err;
		}
		switch(chead.cs_stype)
		{
		case CORES_PROC:
		case CORES_ITIMER:
		case CORES_SCOUTSNAME:
			break;
		case CORES_PREGION:
			rptr = new core_region;
			rptr->vaddr = chead.cs_vaddr;
			rptr->size = chead.cs_vsize;
			rptr->off = chead.cs_sseek;
			rptr->flags = chead.cs_x.csx_preg.csxp_rflg;
			rptr->type = chead.cs_x.csx_preg.csxp_rtyp;
			rptr->next = 0;
			if (!rtail)
				regions = rptr;
			else
				rtail->next = rptr;
			rtail = rptr;
			nregions++;
			break;
	        case CORES_OFFSETS:
			if (!read_section(fd, chead.cs_sseek, (char *)&coffs, 
				chead.cs_vsize))
			{
			     printe(ERR_core_access, E_ERROR, strerror(errno));
			     goto err;
                        }
			bReadCF = true;
                        break;
		}
	}
	rptr = regions;
	((CoreData *)data)->numphdr = nregions;
	pptr = ((CoreData *)data)->phdr = new Elf_Phdr[nregions];

        // Check that coreoffsets has been loaded, if not then assume
        // new file format and load it up
        if (!bReadCF)
        {
		ssize_t seekpos;
		     
		// Check for coreoffsets at EOF.
		if ((lseek(fd, -sizeof(long), SEEK_END) < 0) ||
			(::read(fd, (char *)&seekpos, sizeof(long)) < 0))
		{
			printe(ERR_core_format, E_ERROR);
			goto err;
		}

		// A hueristic. If the seek position is unreasonable
		// or the seek fails, we assume we have a version == 0
		// core file, i.e., non-SCO corefile.
		if ((seekpos < sizeof(struct coreoffsets)) ||
			(seekpos > 2 * sizeof(struct coreoffsets)) ||
			(lseek(fd, -seekpos, SEEK_END) < 0))
		{
			printe(ERR_core_format, E_ERROR);
			goto err;
		}

		// Read the coreoffsets
		if (::read (fd, (char *)&coffs,
			sizeof(struct coreoffsets)) < 0)
		{
		     printe(ERR_core_access, E_ERROR, strerror(errno));
		     goto err;
		}

		bReadCF = true;	
	}

        // Load up the ubytes from core offsets
        ubytes = new char[coffs.u_usize];
        if (!read_section(fd, coffs.u_user, ubytes, coffs.u_usize))
        {
             printe(ERR_core_access, E_ERROR, strerror(errno));
             goto err;
        }

	while(rptr)
	{
		core_region	*rnext = rptr->next;

		pptr->p_type = PT_LOAD;
		pptr->p_vaddr = rptr->vaddr;
		pptr->p_offset = rptr->off;
		pptr->p_filesz = rptr->size;
		pptr->p_memsz = rptr->size;
		pptr->p_flags = 0;
		if (rptr->flags & PF_READ)
			pptr->p_flags |= PF_R;
		if (rptr->flags & PF_WRITE)
			pptr->p_flags |= PF_W;
		if (rptr->flags & PF_EXEC)
			pptr->p_flags |= PF_X;
		DPRINT(DBG_SEG, ("Proccore::open - segment with flags 0x%x\n", pptr->p_flags));

		delete rptr;
		rptr = rnext;
		pptr++;
	}

	// copy relevant data from uarea: psargs, general and
	// floating-point regs
	((CoreData *)data)->psinfo = new char[PSARGSZ];
	strncpy(((CoreData *)data)->psinfo, (ubytes + PSARG_OFF),
		PSARGSZ);
	((CoreData *)data)->psinfo[PSARGSZ-1] = 0;

	reg_off = *(unsigned int *)(ubytes + coffs.u_ar0);
	reg_off -= coffs.u_uaddr;
	memcpy((char *)&((CoreData *)data)->gregs, 
		ubytes + reg_off, sizeof(gregset_t));

	if (*(ubytes + coffs.u_fpvalid) != 0)
	{
		((CoreData *)data)->fpregs = new fpregset_t;
		memcpy((char *)((CoreData *)data)->fpregs,
			(ubytes + coffs.u_fps), sizeof(fpregset_t));
	}

	((CoreData *)data)->lo_stack = *(Iaddr *)(ubytes + coffs.u_sub);
	((CoreData *)data)->hi_stack = ((CoreData *)data)->lo_stack + 
		(pagesize * *(unsigned int *)(ubytes + coffs.u_ssize));

	delete ubytes;

	return 1;
err:
	::close(fd);
	fd = -1;
	delete ubytes;
	delete ((CoreData *)data);
	data = 0;
	return 0;
}

void
Proccore::close()
{
	::close(fd);
	fd = -1;
}

Proccore::~Proccore()
{
	delete ((CoreData *)data);
	data = 0;
}

Elf_Phdr *
Proccore::segment( int which )
{
	if ( ((CoreData *)data) && ((CoreData *)data)->phdr && which >= 0 && which < ((CoreData *)data)->numphdr )
		return ((CoreData *)data)->phdr + which;
	else
		return 0;
}

Procstat
Proccore::status(int &what, int &why)
{
	if (((CoreData *)data))
	{
		why = STOP_SIGNALLED;
		what = 0;
		return p_core;
	}
	return p_unknown;
}

// print name of signal that caused core file to be generated
// and faulting address, if applicable
void
Proccore::core_state()
{
	return;
}

gregset_t *
Proccore::read_greg()
{
	if (data)
	{
		return &((CoreData *)data)->gregs;
	}
	return 0;
}

fpregset_t *
Proccore::read_fpreg()
{
	if (data)
	{
		return ((CoreData *)data)->fpregs;
	}
	return 0;
}

const char *
Proccore::psargs()
{
	if (((CoreData *)data))
		return ((CoreData *)data)->psinfo;
	return 0;
}

int
Proccore::update_stack(Iaddr &lo, Iaddr &hi)
{
	if (((CoreData *)data))
	{
		lo =  ((CoreData *)data)->lo_stack;
		hi =  ((CoreData *)data)->hi_stack;
		return 1;
	}
	return 0;
}
#endif
