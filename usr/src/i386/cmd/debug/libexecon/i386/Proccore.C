#ident	"@(#)debugger:libexecon/i386/Proccore.C	1.22"


#ifndef GEMINI_ON_OSR5

// provides access to core files,
// both old and new (ELF) format
//
// If old format, fake new format CoreData as best we can.

#include "Proccore.h"
#include "Proctypes.h"
#include "Interface.h"
#include "Machine.h"
#include "Reg1.h"
#include "Parser.h"
#include "ELF.h"
#include "utility.h"
#include "NewHandle.h"
#include <elf.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include <ucontext.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "sys/regset.h"

#ifndef OLD_PROC
#include <sys/core.h>
#endif

#ifdef DEBUG_THREADS
struct LwpCoreData {
	lwpstatus_t	*status;
	fpregset_t	*fpregs;
			LwpCoreData() { status = 0; fpregs = 0; }
			~LwpCoreData() {}
};

#endif

// core files may have multiple note sections for different LWPs;
// we keep a list of them

struct CoreData {
	Sectinfo	notes;		// list of note segments
	Elf_Phdr	*phdr;		// copy of program header array
	int		numphdr;	// how many phdrs?
	ELF		*elf;		// object access
	pstatus_t	*status;	// points into notes
	fpregset_t	*fpregs;	// ditto, or 0
	char 		*psinfo;	// ditto, or 0

			CoreData() { memset( (char *)this, 0, 
						sizeof(CoreData) ); }
			~CoreData();
};

CoreData::~CoreData()
{
	if (elf)
		delete elf;
	else
	{
		// if old style, space was malloc'd
		if (phdr)
			free(phdr);
		if (status)
			free(status);
		if (psinfo)
			free(psinfo);
		if (fpregs)
			free(fpregs);
	}
}

#ifdef __cplusplus
extern "C" {
#endif
extern int fake_ELF_core( int corefd, off_t sz, Elf_Phdr **, 
	int *phdrnm, pstatus_t **, fpregset_t **, char **psinfo );
#ifdef __cplusplus
}
#endif

Proccore::Proccore()
{
	data = 0;
#ifdef DEBUG_THREADS
	lwplist = 0;
#endif
}

int
Proccore::open( int cfd )
{
	char 		magic[SELFMAG];
	Elf_Phdr	*phdrp;
	int		phnum;
	struct stat	stbuf;
	off_t		size;
	char		*p;
	Sectinfo	*nsect;
#ifdef DEBUG_THREADS
	Lwpcore		*ltail = 0;
#endif

	if ((fd = debug_dup(cfd)) == -1)
		return 0;
	if (fstat(fd, &stbuf) == -1)
		return 0;
	size = stbuf.st_size;
	data = new CoreData;
	if ((lseek(fd, 0, SEEK_SET) == -1) ||
		(::read(fd, magic, sizeof magic) != sizeof magic))
	{
		printe(ERR_core_access, E_ERROR, strerror(errno));
		goto err;
	}


	// DEL E L F == ELF file
	if ( strncmp(magic, ELFMAG, SELFMAG) != 0 ) 
	{	
#ifdef USES_SVR3_CORE
		// old systems produce a different core format for coff
		// and ELF a.outs - new systems do not
		int		err;
		// old style
		if ((err = fake_ELF_core( fd, size, &((CoreData *)data)->phdr,
			&((CoreData *)data)->numphdr, &((CoreData *)data)->status, 
			&((CoreData *)data)->fpregs, &((CoreData *)data)->psinfo)) != 0)
			 
		{
			if (err == (int)ERR_internal)
				newhandler.invoke_handler();
			delete ((CoreData *)data);
			data = 0;
			printe((Msg_id)err, E_ERROR);
			return 0;
		} 
		return 1;
#else
		printe(ERR_core_format, E_ERROR);
		goto err;
#endif
	}
	((CoreData *)data)->elf = new ELF(fd, stbuf.st_dev, stbuf.st_ino, 
		stbuf.st_mtime);
	if ((((CoreData *)data)->elf->file_format() != ff_elf) || 
		!((CoreData *)data)->elf->is_core())
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	} 

	if (!((CoreData *)data)->elf->get_phdr(((CoreData *)data)->numphdr, ((CoreData *)data)->phdr))
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	}

	// check for truncated core file
	phdrp = ((CoreData *)data)->phdr;
	phnum = ((CoreData *)data)->numphdr;
	for(; phnum > 0; phnum--, phdrp++)
	{
		if ((phdrp->p_offset + phdrp->p_filesz) > size)
			break;
	}
	if (phnum > 0)
	{
		// core file is truncated, but
		// we may still get useful information
		// from it;  make sure we don't
		// try to access one of the truncated
		// segments
		Elf_Phdr	*nphdr, *ntmp;

		nphdr = ntmp = new Elf_Phdr[((CoreData *)data)->numphdr-1];
		// we know at least one segment is truncated

		phdrp = ((CoreData *)data)->phdr;
		phnum = ((CoreData *)data)->numphdr;
		for(; phnum > 0; phnum--, phdrp++)
		{
			if ((phdrp->p_offset + phdrp->p_filesz) > size)
			{
				((CoreData *)data)->numphdr--;
				continue;
			}
			memcpy((char *)nphdr, (char *)phdrp, 
				sizeof(Elf_Phdr));
			nphdr++;
		}
		((CoreData *)data)->elf->reset_phdr(ntmp, ((CoreData *)data)->numphdr);
		((CoreData *)data)->phdr = ntmp;
		printe(ERR_core_truncated, E_WARNING);
	}

	// Get core NOTE section for process.
	// The status and register information for ELF
	// core files is contained in the NOTE section.
	// For SVR4.2 ES/MP and beyond, core files may have
	// multiple note sections; here we want only the one
	// for the process as a whole.
	// The format of a NOTE section entry is:
	//
	// namesz	# int: number of bytes in name
	// descsz	# int: number of bytes in description
	// type		# int: type of entry
	// name		# namesz bytes (null-terminated)
	// description	# descsz bytes
	// padding	# to make next entry 4-byte aligned

	if (!((CoreData *)data)->elf->getsect(s_notes, &((CoreData *)data)->notes))
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	}

	nsect = &((CoreData *)data)->notes;
	int namesz, descsz, type;
	while(nsect)
	{
		size = nsect->size;
		p = (char *)nsect->data;
		while ( size > 0 ) 
		{
			namesz = *(int *)p; p += sizeof(int);
			descsz = *(int *)p; p += sizeof(int);
			type   = *(int *)p; p += sizeof(int);
			size -= 3 * sizeof(int) + namesz + descsz;
			p += namesz;
			switch( type ) 
			{
			default:
				break;
#ifdef OLD_PROC
			case 1:
				((CoreData *)data)->status = (pstatus_t *)p;
				break;
			case 2:
				((CoreData *)data)->fpregs = (fpregset_t *)p;
				break;
			case 3:			// psinfo
				((CoreData *)data)->psinfo = ((prpsinfo_t *)p)->pr_psargs;
				break;
#else	// new /proc
			case CF_T_PRSTATUS:
				((CoreData *)data)->status = (pstatus_t *)p;
				if (((CoreData *)data)->status->pr_lwp.pr_context.uc_flags & UC_FP)
					((CoreData *)data)->fpregs =
						&((CoreData *)data)->status->pr_lwp.pr_context.uc_mcontext.fpregs;
				break;
			case CF_T_PRPSINFO:		// psinfo
				((CoreData *)data)->psinfo = ((psinfo_t *)p)->pr_psargs;
				break;
#ifdef DEBUG_THREADS
			case CF_T_LWPSTATUS:
			{
				Lwpcore *lcore = new Lwpcore;

				lcore->open(p);
				if (ltail)
					ltail->set_next(lcore);
				else
					lwplist = lcore;
				ltail = lcore;
				break;
			}
#endif
#endif
			}
			p += descsz;
			int mod = (int)p % sizeof(int);
			if (mod)
				p += sizeof(int) - mod;
		}
		nsect = nsect->next;
	}
	if ( !((CoreData *)data)->status )
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	}
	return 1;
err:
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
#ifdef DEBUG_THREADS
	while(lwplist)
	{
		Lwpcore	*lcore;
		lcore = lwplist;
		lwplist = lwplist->next();
		delete lcore;
	}
#endif
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
#ifdef OLD_PROC
		what = ((CoreData *)data)->status->pr_what;
		why = ((CoreData *)data)->status->pr_why;
#else
		what = ((CoreData *)data)->status->pr_lwp.pr_what;
		why = ((CoreData *)data)->status->pr_lwp.pr_why;
#endif
		return p_core;
	}
	return p_unknown;
}

// print name of signal that caused core file to be generated
// and faulting address, if applicable
void
Proccore::core_state()
{
	char		*sname;
	int		sig;
	Iaddr		fltaddr;
	char		signal_name[14]; // big enough for any name

	if (!((CoreData *)data) || !((CoreData *)data)->status)
		return;
#ifdef OLD_PROC
	sig = ((CoreData *)data)->status->pr_cursig;
#else
	sig = ((CoreData *)data)->status->pr_lwp.pr_cursig;
#endif
	if (sig < 1 || sig > NUMBER_OF_SIGS)
		return;
	sname = (char *)signame(sig);
	sprintf(signal_name, "sig%s", sname);
	switch(sig)
	{
		case SIGILL:
		case SIGTRAP:
		case SIGFPE:
		case SIGEMT:
		case SIGBUS:
		case SIGSEGV:
#ifdef OLD_PROC
			fltaddr = (Iaddr)((CoreData *)data)->status->pr_info._data._fault._addr;
#else
			fltaddr = (Iaddr)((CoreData *)data)->status->pr_lwp.pr_info._data._fault._addr;
#endif
			printm(MSG_core_state_addr, signal_name, fltaddr);
			break;
		default:
			printm(MSG_core_state, signal_name);
			break;
	}
}
gregset_t *
Proccore::read_greg()
{
	if (data)
	{
#ifdef OLD_PROC
		return &((CoreData *)data)->status->pr_reg;
#else
		return &((CoreData *)data)->status->pr_lwp.pr_context.uc_mcontext.gregs;
#endif
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
#ifndef OLD_PROC
	if (((CoreData *)data))
	{
		hi = ((CoreData *)data)->status->pr_stkbase;
		lo  = hi - ((CoreData *)data)->status->pr_stksize;
		return 1;
	}
	else
		return 0;
#else
	return 0;
#endif
}

#ifdef DEBUG_THREADS

// return the id of the lwp that dumped core
lwpid_t
Proccore::lwp_id()
{
	if (((CoreData *)data))
	{
		return ((CoreData *)data)->status->pr_lwp.pr_lwpid;
	}
	return (lwpid_t)-1;
}

Lwpcore::Lwpcore() : Proccore()
{
	data = new LwpCoreData;
	_next = 0;
}

Lwpcore::~Lwpcore()
{
	delete ((LwpCoreData *)data);
	data = 0; // shared by base class; prevent double deletion
}

int
Lwpcore::open(void *status)
{
	if (!status)
		return 0;
	((LwpCoreData *)data)->status = (lwpstatus_t *)status;
	if (((LwpCoreData *)data)->status->pr_context.uc_flags & UC_FP)
		
		((LwpCoreData *)data)->fpregs = 
			&((LwpCoreData *)data)->status->pr_context.uc_mcontext.fpregs;
	return 1;
}

void
Lwpcore::close()
{
	return;
}

gregset_t *
Lwpcore::read_greg()
{
	if (data)
	{
		return &((LwpCoreData *)data)->status->pr_context.uc_mcontext.gregs;
	}
	return 0;
}

fpregset_t *
Lwpcore::read_fpreg()
{
	if (data)
	{
		return ((LwpCoreData *)data)->fpregs;
	}
	return 0;
}

lwpid_t
Lwpcore::lwp_id()
{
	if (((LwpCoreData *)data))
	{
		return ((LwpCoreData *)data)->status->pr_lwpid;
	}
	return (lwpid_t)-1;
}

Procstat
Lwpcore::status(int &what, int &why)
{
	if (((LwpCoreData *)data))
	{
		what = ((LwpCoreData *)data)->status->pr_what;
		why = ((LwpCoreData *)data)->status->pr_why;
		return p_core;
	}
	return p_unknown;
}

#if DEBUG

#include <signal.h>
#include <ucontext.h>
#include <thread.h>

void
dump_thread_desc(thread_map *desc)
{
printf("thr_tid == %d\n", desc->thr_tid);
printf("thr_lwpp == %#x\n", desc->thr_lwpp);
printf("thr_state == %d\n", desc->thr_state);
printf("thr_psig == 0x%x_%x_%x_%x\n", desc->thr_psig.sa_sigbits[0],
	desc->thr_psig.sa_sigbits[1], desc->thr_psig.sa_sigbits[2],
		desc->thr_psig.sa_sigbits[3]);
printf("thr_dbg_set == 0x%x_%x_%x_%x\n", desc->thr_dbg_set.sa_sigbits[0],
	desc->thr_dbg_set.sa_sigbits[1], desc->thr_dbg_set.sa_sigbits[2],
		desc->thr_dbg_set.sa_sigbits[3]);
printf("thr_dbg_cancel == %d\n", desc->thr_dbg_cancel);
printf("thr_dbg_busy == %d\n", desc->thr_dbg_busy);
printf("thr_next == %#x\n", desc->thr_next);
printf("thr_ucontext:\n");
printf("\tuc_flags == %#x\n", desc->thr_ucontext.uc_flags);
printf("\tuc_link == %#x\n", desc->thr_ucontext.uc_link);
printf("\tuc_sigmask == 0x%x_%x_%x_%x\n", 
	desc->thr_ucontext.uc_sigmask.sa_sigbits[0],
	desc->thr_ucontext.uc_sigmask.sa_sigbits[1], 
	desc->thr_ucontext.uc_sigmask.sa_sigbits[2], 
	desc->thr_ucontext.uc_sigmask.sa_sigbits[3] );
printf("\tuc_stack:\n");
printf("\t\tss_sp == %#x\n", desc->thr_ucontext.uc_stack.ss_sp);
printf("\t\tss_size == %#x\n", desc->thr_ucontext.uc_stack.ss_size);
printf("\t\tss_flags == %#x\n", desc->thr_ucontext.uc_stack.ss_flags);
printf("\tgeneral regs:\n");
printf("\t\tR_GS == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_GS]);
printf("\t\tR_FS == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_FS]);
printf("\t\tR_ES == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_ES]);
printf("\t\tR_DS == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_DS]);
printf("\t\tR_EDI == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_EDI]);
printf("\t\tR_ESI == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_ESI]);
printf("\t\tR_EBP == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_EBP]);
printf("\t\tR_EBX == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_EBX]);
printf("\t\tR_EDX == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_EDX]);
printf("\t\tR_ECX == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_ECX]);
printf("\t\tR_EAX == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_EAX]);
printf("\t\tR_EIP == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_EIP]);
printf("\t\tR_CS == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_CS]);
printf("\t\tR_EFL == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_EFL]);
printf("\t\tR_ESP == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_ESP]);
printf("\t\tR_SS == %#x\n", desc->thr_ucontext.uc_mcontext.gregs.greg[R_SS]);
}
#endif

#endif
#endif
