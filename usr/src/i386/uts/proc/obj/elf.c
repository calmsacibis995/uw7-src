#ident	"@(#)kern-i386:proc/obj/elf.c	1.32.5.1"
#ident	"$Header$"

#include <acc/priv/privilege.h>
#include <fs/pathname.h>
#include <fs/procfs/prdata.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prsystm.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/vmparam.h>
#include <proc/auxv.h>
#include <proc/core.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/obj/elf.h>
#include <proc/obj/elf_386.h>
#include <proc/obj/elftypes.h>
#include <proc/obj/x.out.h>
#include <proc/proc.h>
#include <proc/regset.h>
#include <proc/resource.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/fp.h>
#include <svc/secsys.h>
#include <svc/systm.h>
#include <svc/utsname.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/cmn_err.h>

#define OSR5_FIX_FLAG 0x3552534F

STATIC int getelfhead(exhda_t *, Elf32_Ehdr **, caddr_t *, Elf32_Phdr **,
		      Elf32_Phdr **, Elf32_Phdr **, long *, vaddr_t *,
		      Elf32_Phdr **, Elf32_Word *);
STATIC int mapelfexec(vnode_t *, Elf32_Ehdr *, caddr_t, vaddr_t *, vaddr_t *,
		      struct uarg *);
STATIC int elf_getcoffshlib(Elf32_Phdr *, struct exdata **, uint_t *, long *,
			    exhda_t *);
STATIC int elf_mapcoffshlib(struct exdata *, uint_t);
STATIC int getnsegs(proc_t *);

extern int getcoffshlibs(struct exdata *edp, struct exdata *dat_start,
			 long *execsz, exhda_t *ehdp);
extern void coffexec_err(struct exdata *shlb_data, long nlibs);

/*
 * Most of the code in this file is generic.  The parts that deal
 * with the stack placement (new virtual map support), and Xenix
 * support are i86 dependent.
 *
 * The stack placement code attempts to put the stack just below
 * the text segment.  The motivation is to cut down on process overhead
 * by using one page table for text+data+stack.  On the i86 architecture,
 * a single page table is 4Kb and can map 4Mb.
 *
 * Also, the setting of elfmagic to 0x457f in Master file is for little endian
 * architectures.  For big endian architectures, change elfmagic to 0x7f45.
 */


/*
 * int
 * elfexec(vnode_t *vp, struct uarg *args, int level, long *execsz,
 *	   exhda_t *ehdp)
 *	Exec an ELF object file.
 *
 * Calling/Exit State:
 *	Called from gexec via execsw[].  No spin locks can be held on
 *	entry, no spin locks are held on return.
 */
/*ARGSUSED*/
int
elfexec(vnode_t *vp, struct uarg *args, int level, long *execsz,
	exhda_t *ehdp)
{
	Elf32_Ehdr	*ehdrp, *dy_ehdrp;
	caddr_t		phdrbase = NULL, dyphdrbase = NULL;
	vaddr_t 	base = NULL;
	char		*dlnp;
	int		dlnsize, fd, *aux, error;
	vaddr_t		voffset, startpc;
	vnode_t	*dyvp;
	Elf32_Phdr	*dyphdr = NULL;
	Elf32_Phdr	*stphdr = NULL;
	Elf32_Phdr	*uphdr = NULL;
	Elf32_Phdr	*junk = NULL;
	Elf32_Phdr	*notehdr = NULL;
	Elf32_Word	notesize = 0;
	Elf32_Word	wordjunk = 0;
	int		elfargs[24];
	exhda_t		dehdr;
	struct vattr	vattr;
	vaddr_t		lowest_addr = ~(vaddr_t)0;
	vaddr_t		stkbase, minstkbase;
	uint_t		stkgapsize;
	struct exdata	*coff_shlbdat;
	uint_t		coff_nshlb;
	rval_t		rval;
	cred_t		*credp;
	uint_t		maxstksize;

	if ((error = getelfhead(ehdp, &ehdrp, &phdrbase, &uphdr, &dyphdr,
				&stphdr, execsz, &lowest_addr, 
				&notehdr, &notesize)) != 0)
		return error;

	/* Do not allow a shared object to be directly exec'ed. */
	if (ehdrp->e_type == ET_DYN)
		return ELIBEXEC;

	if (stphdr != NULL) {
		/* Get info on COFF shared libs, if any. */
		if ((error = elf_getcoffshlib(stphdr, &coff_shlbdat,
					      &coff_nshlb, execsz, ehdp)) != 0)
			return error;
	}

	if (dyphdr != NULL) {			/* Have an interpreter */
		dlnsize = dyphdr->p_filesz;
		if (dlnsize > MAXPATHLEN || dlnsize <= 0)
			return ENOEXEC;

		/* Get pathname of interpreter. */
		error = exhd_read(ehdp, dyphdr->p_elfoffset, dyphdr->p_filesz,
				  (void **)&dlnp);
		if (error)
			return error;

		if (dlnp[dlnsize - 1] != '\0')
			return ENOEXEC;

		error = lookupname(dlnp, UIO_SYSSPACE, FOLLOW, NULLVPP, &dyvp);
		if (error)
			return error;

		if ((error = execpermissions(dyvp, &vattr, &dehdr, args)) != 0) {
			VN_RELE(dyvp);
			return error;
		}

		/* Get the ELF header of the interpreter. */
		if ((error = getelfhead(&dehdr, &dy_ehdrp, &dyphdrbase,
					&junk, &junk, &junk,
					execsz, NULL, &junk, &wordjunk)) != 0) {
			exhd_release(&dehdr);
			VN_RELE(dyvp);
			return error;
		}

		/*
		 * The interpreter itself must not require a
		 * second interpreter; nor may it use COFF shared libs.
		 */
		if (junk != NULL) {
			exhd_release(&dehdr);
			VN_RELE(dyvp);
			return ENOEXEC;
		}

		/*
		 * Determine aux size now so that stack can be built
		 * in one shot (except actual copyout of aux image)
		 * and still have this code be machine independent.
		 */
		args->auxsize = (uphdr != NULL ? (24 * NBPW) : (18 * NBPW));
	} else if (uphdr != NULL) {
		/* Can't have a program header and no interpreter. */
		return ENOEXEC;
	}

	/*
	 * New virtual map support. (i86 dependent code)
	 *
	 * Stack placement algorithm:
	 * 	stkbase: 	stack starts here and grows downwards
	 * 	stkgapsize: 	[stkbase, stkbase+stkgapsize] is available
	 * 	                for mapping shared libraries
	 *	lowest_addr: 	lowest address in the elf executable
	 *			or ~0 if undefined
	 */

#define	MINSTKEND	PAGESIZE
#define	STKOFF		0x48000

	/* Compute minstkbase as hard rlmit on the stack size + PAGESIZE */
	maxstksize = u.u_rlimits->rl_limits[RLIMIT_STACK].rlim_max;
	minstkbase = MINSTKEND + maxstksize;

	if (lowest_addr == ~(vaddr_t)0) {
		/*
		 * Case 1: lowest_addr undefined
		 * We start the stack at minstkbase, growing downwards and
		 * allow shared libraries to be mapped in the rest of the
		 * virtual mapped by the L2 page table mapping stkbase
		 */
		stkbase = minstkbase;
		stkgapsize = (stkbase + VPTSIZE - 1) & VPTMASK - stkbase;
	} else if (lowest_addr < minstkbase) {
		/*
		 * Case 2: lowest_addr is less than minstkbase.
		 * We're out of virtual for the stack and forced to map the
		 * stack at UVSTACK
		 */
		stkbase = UVSTACK;
		stkgapsize = 0;
	} else {
		/*
		 * Case 3: We have sufficient virtual to map the stack below
		 * the text. stkbase is selected such that it falls in the
		 * same L2 page table as the text. The space between stkbase 
		 * and the text is declared free for mapping shared libs.
		 * We also leave some room (STKOFF bytes)  in the virtual
		 * address space described by the L2 page table for
		 * the stack to grow, before needing another L2 page table.
		 */
		stkbase = (lowest_addr & VPTMASK) + STKOFF;
		if (stkbase > lowest_addr)
			stkbase = (lowest_addr & PAGEMASK);
		else if (stkbase < minstkbase)
			stkbase = minstkbase;
		stkgapsize = (lowest_addr & PAGEMASK) - stkbase;
	}

	if ((error = remove_proc(args, vp, stkbase, stkgapsize, execsz)) != 0) {
		if (dyphdr != NULL) {
			exhd_release(&dehdr);
			VN_RELE(dyvp);
		}
		return error;
	}

	/* Single threaded upon return from remove_proc(). */
	ASSERT(SINGLE_THREADED());

	if (dyphdr == NULL || uphdr != NULL) {
		if ((error = mapelfexec(vp, ehdrp, phdrbase, &base, &voffset,
					args)) != 0) {
			if (dyphdr != NULL) {
				exhd_release(&dehdr);
				VN_RELE(dyvp);
			}
			goto bad;
		}
	}

	if (stphdr != NULL) {
		/* Map in COFF shared libs, if any. */
		if ((error = elf_mapcoffshlib(coff_shlbdat, coff_nshlb)) != 0) {
			if (dyphdr != NULL) {
				exhd_release(&dehdr);
				VN_RELE(dyvp);
			}
			goto bad;
		}
	}

	if (dyphdr != NULL) {			/* Have an interpreter */
		startpc = dy_ehdrp->e_entry;

		/* Set up auxiliary information to pass to interpreter. */
		aux = elfargs;
		if (uphdr != NULL) {
			/*
			 * Have a program header; give the interpreter
			 * enough information to access the header.
			 */
			setexecenv(base);

			*aux++ = AT_PHDR;
			*aux++ = (int)uphdr->p_vaddr + voffset;
			*aux++ = AT_PHENT;
			*aux++ = ehdrp->e_phentsize;
			*aux++ = AT_PHNUM;
			*aux++ = ehdrp->e_phnum;
			*aux++ = AT_ENTRY;
			*aux++ = ehdrp->e_entry + voffset;
		} else {
			/*
			 * No program header; open a file descriptor
			 * to the file for the interpreter to use.
			 */
			if ((error = execopen(&vp, &fd)) != 0) {
				if (dyphdr != NULL) {
					exhd_release(&dehdr);
					VN_RELE(dyvp);
				}
				goto bad;
			}

			*aux++ = AT_EXECFD;
			*aux++ = fd;
		}

		error = mapelfexec(dyvp, dy_ehdrp, dyphdrbase, &base, &voffset,
				   args);
		/*
		 * Note: cannot dereference dy_ehdrp or dyphdrbase after
		 * this exhd_release() call.
		 */
		exhd_release(&dehdr);
		VN_RELE(dyvp);
		if (error != 0)
			goto bad;

		/*
		 * Fill in the rest of the aux info, which depends on the
		 * voffset value for the interpreter.
		 */
		*aux++ = AT_BASE;
		*aux++ = voffset & ~(ELF_386_MAXPGSZ - 1);
		*aux++ = AT_FLAGS;
		*aux++ = EF_I386_NONE;
		*aux++ = AT_PAGESZ;
		*aux++ = PAGESIZE;
		*aux++ = AT_INTP_DEVICE;
		*aux++ = (int)vattr.va_fsid;
		*aux++ = AT_INTP_INODE;
		*aux++ = (int)vattr.va_nodeid;
		*aux++ = AT_FPHW;
		*aux++ = fp_kind;
		*aux++ = AT_LIBPATH;
		/*
		 * We need to inform rtld if it is safe to use the
		 * LD_LIBRARY_PATH variable.  It is safe if the program
		 * is not setid, not setgid, and not privileged. 
		 * NOTE: if this is a SUM system, and we are the
		 * privileged id (privid), we have all privileges,
		 * and we will fail the "not privileged" test. 
		 * So, if we fail the privilege test, we still say
		 * it is safe if our real id is the privileged id,
		 */
		credp = u.u_procp->p_cred;
		if ((credp->cr_uid == credp->cr_ruid) &&
		    (credp->cr_gid == credp->cr_rgid) &&
		    ((pm_privileged(credp) == 0) ||
			((pm_secsys(ES_PRVID, &rval, 0) == 0) &&
			 (rval.r_val1 == credp->cr_ruid))))
				*aux++ = 1;
		else
				*aux++ = 0;
		*aux++ = AT_NULL;
		*aux++ = 0;

		/* Copy aux info into the stack image. */
		ASSERT((char *)aux - (char *)elfargs == args->auxsize);
		if ((error = execpoststack(args, elfargs, args->auxsize)) != 0)
			goto bad;
	} else {
		startpc = ehdrp->e_entry;
		ASSERT(args->auxsize == 0);
	}

	args->execinfop->ei_exdata.ex_entloc = startpc + voffset;

	/* XXX MACHINE/ENVIRONMENT DEPENDENT */
	/*
	 *	Unfortunate: Xenix support.
	 */
	args->execinfop->ei_exdata.ex_renv = XE_V5|XE_EXEC|RE_IS386|RE_ISELF;

	/* Enhanced Application Compatibility Support*/

	/* a note section of size 0x1c indicates a sco elf */
	if ((notehdr != NULL) && (notesize == 0x1c)) {
		args->execinfop->ei_exdata.ex_renv2 |= RE_ISSCO ;
		
	}
	else if(ehdrp->e_flags == OSR5_FIX_FLAG) {
	/*
	 * alternately, a header with a 'OSR5' string in it is a 'forced'
	 * sco elf. forced, becuase a cmd is used to mark it. this is
	 * used for a 'beta' version of OpenServer compiler that didn't
	 * mark the note section
	 */
		args->execinfop->ei_exdata.ex_renv2 |= RE_ISSCO ;
	}

	/* End Enhanced Application Compatibility Support*/
	


	if (!uphdr)
		setexecenv(base);

	return 0;

bad:
	if (fd != -1)		/* did we open the a.out yet */
		(void)execclose(fd);

	sigtoproc(u.u_procp, SIGKILL, (sigqueue_t *)NULL);

	return ((error != 0) ? error : ENOEXEC);
}


/*
 * STATIC int
 * getelfhead(exhda_t *ehdp, Elf32_Ehdr **ehdrp, caddr_t *phdrbase,
 *	      Elf32_Phdr **uphdr, Elf32_Phdr **dyphdr, Elf32_Phdr **stphdr,
 *	      long *execsz, vaddr_t *lowaddrp, Elf32_Phdr **notehdr,
 *	      Elf32_Word *notesize)
 *	Get the ELF header and the program header table.
 *
 * Calling/Exit State:
 *	No spin locks should be held by the caller on entry, none
 *	are held on return.
 *
 * Description:
 *	The ELF header is obtained from the file and checked for sanity;
 *	the program header table is then obtained from the file.
 *	The out parameters ehdrp and phdrbase are set to the address of
 *	the ELF header and the program header table, respectively.
 *	The out parameters uphdr, dyphdr and stphdr, are set to point to
 *	the program header, the interpreter header and the non-ABI (COFF)
 *	shared library header, respectively, if any; these parameters are
 *	left unchanged (and assumed NULL) if the corresponding header is
 *	not present.
 *	The note section pointer along with its size are returned for
 *	use in determining if the executable is a SCO elf.  (a note
 *	section of size 0x1c is the huristic for determing that an
 *	ELF is a SCO elf)
 *
 *	*execsz is incremented by the total memory size required for the
 *	ELF object.  If lowaddrp is non-NULL, *lowaddrp is set to the
 *	minimum of its existing value and the base addresses of any loadable
 *	segments.
 *
 *	On success 0 is returned; otherwise, a non-zero errno.
 */
STATIC int
getelfhead(exhda_t *ehdp, Elf32_Ehdr **ehdrp, caddr_t *phdrbase,
	   Elf32_Phdr **uphdr, Elf32_Phdr **dyphdr, Elf32_Phdr **stphdr,
	   long *execsz, vaddr_t *lowaddrp, Elf32_Phdr **notehdr, 
	   Elf32_Word *notesize)
{
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	long phdrsize;
	boolean_t ptload = B_FALSE;
	int i;
	int error;

	/* Get the ELF header. */
	error = exhd_read(ehdp, 0, sizeof (Elf32_Ehdr), (void **)ehdrp);
	if (error != 0)
		return error;
	ehdr = *ehdrp;

	/*
	 * We got here by the first two bytes in ident.
	 * Check the remainder of the header for sanity.
	 */
	if (ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
	    ehdr->e_ident[EI_CLASS] != ELFCLASS32 ||
	    ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
	    ehdr->e_machine != EM_386 ||
	    (ehdr->e_type != ET_EXEC && ehdr->e_type != ET_DYN) ||
	    ehdr->e_phentsize == 0) {
			return ENOEXEC;
	}

	/* Get Program Header Table. */
	phdrsize = ehdr->e_phnum * ehdr->e_phentsize;
	error = exhd_read(ehdp, ehdr->e_phoff, phdrsize, (void **)phdrbase);
	if (error)
		return error;

	/* Update execsz and check for illegal combinations. */
	for (i = 0; i < (int)ehdr->e_phnum; i++) {
		/* Rely on i386 addressing misaligned data */
		/* LINTED pointer alignment */
		phdr = (Elf32_Phdr *)(*phdrbase + (ehdr->e_phentsize * i));

		switch (phdr->p_type) {
		case PT_LOAD:
			/*
			 * If we have an interpreter, and do not have
			 * a program header, return.  A program header
			 * must precede any loadable segment.
			 */
			if ((*dyphdr != NULL) && (*uphdr == NULL))
				return 0;

			ptload = B_TRUE;
			*execsz += btopr(phdr->p_memsz);
			if (lowaddrp != NULL && phdr->p_vaddr < *lowaddrp)
				*lowaddrp = phdr->p_vaddr;
			break;

		case PT_INTERP:
			/* Must precede any loadable segment. */
			if (ptload)
				return ENOEXEC;
			*dyphdr = phdr;
			break;

		case PT_SHLIB:
			/* Non ABI conforming (COFF shared library). */
			*stphdr = phdr;
			break;

		case PT_PHDR:			/* Program Header */
			/* Must precede any loadable segment. */
			if (ptload)
				return ENOEXEC;
			*uphdr = phdr;
			break;
		case PT_NOTE:			/*note section*/
			/* a note section with a size 0x1c indicates SCO ELF*/
			*notehdr = phdr;
			*notesize = phdr->p_filesz;
			break;

		default:
			break;
		}
	}

	return 0;
}


/*
 * STATIC int
 * mapelfexec(vnode_t *vp, Elf32_Ehdr *ehdr, caddr_t phdrbase,
 *	      vaddr_t *base, vaddr_t *voffset, struct uarg *args)
 *	Populate the calling processes address space from an ELF
 *	executable file.
 *
 * Calling/Exit State:
 *	No spinlocks are held by the caller upon entry, none are held on
 *	return.  The process is single threaded	at the time of the call.
 */
STATIC int
mapelfexec(vnode_t *vp, Elf32_Ehdr *ehdr, caddr_t phdrbase,
	   vaddr_t *base, vaddr_t *voffset, struct uarg *args)
{
	proc_t *p = u.u_procp;
	Elf32_Phdr *phdr;
	int i, prot, error = 0;
	vaddr_t addr;
	size_t zfodsz;
	vaddr_t data_base = 0;

	*voffset = 0;
	if (ehdr->e_type == ET_DYN) {
		/*
		 * Compute the amount of space required to map in
		 * the shared object.
		 */
		ulong_t	size, nsize;

		size = 0;
		for (i = 0; i < (int)ehdr->e_phnum; i++) {
			/* Rely on i386 addressing misaligned data */
			/* LINTED pointer alignment */
			phdr = (Elf32_Phdr *)(phdrbase + (ehdr->e_phentsize*i));
			if (phdr->p_type == PT_LOAD && phdr->p_memsz != 0) {
				nsize = phdr->p_vaddr + phdr->p_memsz;
				if (nsize > size)
					size = nsize;
			}
		}

		/* Find a home for the shared object. */
		as_wrlock(p->p_as);
		map_addr(voffset, size, 0, 1);
		as_unlock(p->p_as);
		if (*voffset == 0)
			return ENOMEM;			/* no vacancy */
	}

	for (i = 0; i < (int)ehdr->e_phnum; i++) {
		/* Rely on i386 addressing misaligned data */
		/* LINTED pointer alignment */
		phdr = (Elf32_Phdr *)(phdrbase + (ehdr->e_phentsize * i));

		switch (phdr->p_type) {
		case PT_LOAD:
			prot = PROT_USER;
			if (phdr->p_flags & PF_R)
				prot |= PROT_READ;
			if (phdr->p_flags & PF_W)
				prot |= PROT_WRITE;
			if (phdr->p_flags & PF_X)
				prot |= PROT_EXEC;

			addr = phdr->p_vaddr + *voffset;
			zfodsz = phdr->p_memsz - phdr->p_filesz;

			if ((error = execmap(vp, addr, phdr->p_filesz, zfodsz,
					     phdr->p_elfoffset, prot)) != 0)
				return error;

			if (phdr->p_flags & PF_W) {
				if (addr > *base)
					*base = addr + phdr->p_memsz;

				/* new virtual map support */
				if (data_base == 0 || addr < data_base)
					args->execinfop->ei_exdata.ex_datorg =
						data_base =
							addr;
			}
			break;

		default:
			break;
		}
	}
	return 0;
}


/*
 * STATIC int
 * elf_getcoffshlib(Elf32_Phdr *stphdr, struct exdata **shlb_datp,
 *		    uint_t *nshlibsp, long *execsz, exhda_t *ehdp)
 *	Get header info for COFF shared libraries referenced by an ELF
 *	executable file.
 *
 * Calling/Exit State:
 *	No spin locks are held by the caller on entry, none are held on
 *	return.
 *
 * Remarks:
 *	A strange beast indeed, but the tools can build one.
 */
/*ARGSUSED*/
STATIC int
elf_getcoffshlib(Elf32_Phdr *stphdr, struct exdata **shlb_datp,
		 uint_t *nshlibsp, long *execsz, exhda_t *ehdp)
{
	struct exdata edp;
	size_t shlb_datsz;
	int error;

	edp.ex_lsize = stphdr->p_filesz;
	edp.ex_loffset = stphdr->p_elfoffset;

	shlb_datsz = shlbinfo.shlbs * sizeof (struct exdata);

	*shlb_datp = kmem_alloc(shlb_datsz, KM_SLEEP);

	if ((error = getcoffshlibs(&edp, *shlb_datp, execsz, ehdp)) != 0) {
		kmem_free(*shlb_datp, shlb_datsz);
		return error;
	}

	*nshlibsp = edp.ex_nshlibs;

	return 0;
}


/*
 * STATIC int
 * elf_mapcoffshlib(struct exdata *shlb_dat, uint_t nshlibs)
 *	Get COFF shared libraries referenced by an ELF executable file.
 *
 * Calling/Exit State:
 *	No spin locks are held by the caller on entry, none are held on
 *	return.
 */
/*ARGSUSED*/
STATIC int
elf_mapcoffshlib(struct exdata *shlb_dat, uint_t nshlibs)
{
	const int dataprot = PROT_ALL;
	const int textprot = PROT_ALL & ~PROT_WRITE;
	struct exdata *datp;
	size_t shlb_datsz;
	uint_t i;
	int error;

	datp = shlb_dat;

	for (i = 0; i < nshlibs; i++, datp++){
		if ((error = execmap(datp->ex_vp, datp->ex_txtorg,
				     datp->ex_tsize, 0,
				     datp->ex_toffset, textprot)) != 0) {
			coffexec_err(++datp, nshlibs - i - 1);
			goto done;
		}

		if ((error = execmap(datp->ex_vp, datp->ex_datorg,
				     datp->ex_dsize, datp->ex_bsize,
				     datp->ex_doffset, dataprot)) != 0) {
			coffexec_err(++datp, nshlibs - i - 1);
			goto done;
		}
		VN_RELE(datp->ex_vp);	/* done with this reference */
	}

done:
	shlb_datsz = shlbinfo.shlbs * sizeof (struct exdata);
	kmem_free(shlb_dat, shlb_datsz);
	return error;
}


/*
 * int
 * elftextinfo(exhda_t *ehdp, extext_t *extxp)
 *	Get info on the text section of a text-only ELF file.
 *
 * Calling/Exit State:
 *	No spin locks should be held by the caller on entry, none
 *	are held on return.
 */
int
elftextinfo(exhda_t *ehdp, extext_t *extxp)
{
	Elf32_Ehdr *ehdrp;
	Elf32_Phdr *phdr;
	caddr_t phdrbase;
	boolean_t found_text = B_FALSE;
	Elf32_Phdr *junk;
	Elf32_Word wordjunk;
	long junksz;
	int i;
	int error;

	/* Get the Elf header. */
	if ((error = getelfhead(ehdp, &ehdrp, &phdrbase, &junk, &junk, &junk,
				&junksz, NULL, &junk, &wordjunk)) != 0)
		return error;

	/* Do not allow a shared object to be directly exec'ed. */
	if (ehdrp->e_type == ET_DYN)
		return ELIBEXEC;

	/*
	 * Scan all valid program headers to find the file offset and size
	 * of the "text" segment.  Make sure there is one and only one such
	 * segment and that there are no other loadable segments.
	 */
	for (i = 0; i < (int)ehdrp->e_phnum; i++) {
		/* Rely on i386 addressing misaligned data */
		/* LINTED pointer alignment */
		phdr = (Elf32_Phdr *)(phdrbase + (ehdrp->e_phentsize * i));
		if (phdr->p_type == PT_LOAD) {
			/* "text" is executable but not writeable. */
			if ((phdr->p_flags & (PF_X|PF_W)) == PF_X) {
				if (found_text) {
					/* Multiple text segments */
					return EINVAL;
				}
				extxp->extx_offset = phdr->p_elfoffset;
				extxp->extx_size = phdr->p_filesz;
				found_text = B_TRUE;
			} else if (phdr->p_memsz != 0) {
				/* Non-text loadable segment */
				return EINVAL;
			}
		}
	}

	if (!found_text)
		return EINVAL;

	extxp->extx_entloc = ehdrp->e_entry;

	return 0;
}


#define WR(vp, base, count, offset, rlimit, credp) \
	vn_rdwr(UIO_WRITE, vp, (caddr_t)base, count, offset, UIO_SYSSPACE, \
	0, rlimit, credp, (int *)NULL)

typedef struct {
	Elf32_Word namesz;
	Elf32_Word descsz;
	Elf32_Word type;
	char name[8];
} Elf32_Note;

/*
 * STATIC int
 * elfnote(vnode_t *vp, off64_t *offsetp, int type, int descsz, void *desc,
 *	   rlim_t rlimit, cred_t *credp)
 *	Dumps out the elf "note" section to the file pointed to by vp at
 *	specified offset.
 *
 * Calling/Exit State:
 *	No locks should be held on entry and none will be held on return.
 *	The offsetp parameter will be updated to reflect the new offset
 *	into file, after the note section has been written out.
 */
STATIC int
elfnote(vnode_t *vp, off64_t *offsetp, int type, int descsz, void *desc,
	rlim_t rlimit, cred_t *credp)
{
	Elf32_Note note;
	int error;

	bzero(&note, sizeof note);
	bcopy("CORE", note.name, 4);
	note.type = type;
	note.namesz = 8;
	note.descsz = roundup(descsz, sizeof (Elf32_Word));
	if (error = WR(vp, &note, sizeof note, *offsetp, rlimit, credp))
		return error;
	*offsetp += sizeof note;
	if (error = WR(vp, desc, note.descsz, *offsetp, rlimit, credp))
		return error;
	*offsetp += note.descsz;
	return 0;
}


#define prgetcred(x,y)		bzero((y), sizeof *(y))
#define prgetutsname(x,y)	bcopy(&utsname, y, sizeof *(y))

/*
 * int
 * elfcore(vnode_t *vp, proc_t *p, cred_t *credp, rlim_t rlimit, int sig)
 *	Dump core for an ELF process image.  The core file is denoted
 *	by vp.
 *
 * Calling/Exit State:
 *	No spin locks are held on entry and none will be held on return.
 *	The calling process is single threaded when invoked. That is, all
 *	LWPs in the process other than the calling context are expected
 *	to have rendezvoused.
 *
 * Remarks:
 *	Following is the format of the core file that will be dumped:
 *
 *
 *	*********************************************************
 *	*							*
 *	*		Elf header				*
 *	*********************************************************
 *	*							*
 *	*							*
 *	*		Program header:				*
 *	*							*
 *	*			One entry per note section.	*
 *	*							*
 *	*			One entry for each region of	*
 *	*			memory in the addrress space 	*
 *	*			with different permissions.	*
 *	*							*
 *	*********************************************************
 *	*							*
 *	*		Note sections:				*
 *	*							*
 *	*			For a process with N LWPs	*
 *	*			there will be N+1 note 		*
 *	*			sections - (a note section per	*
 *	*			LWP and a process-wide note	*
 *	*			section.			*
 *	*							*
 *	*********************************************************
 *	*							*
 *	*		Dump of the address space.		*
 *	*							*
 *	*********************************************************
 */
/*ARGSUSED*/
int
elfcore(vnode_t *vp, proc_t *p, cred_t *credp, rlim_t rlimit, int sig)
{
	Elf32_Ehdr 	ehdr;
	Elf32_Phdr 	*phdrp;
	u_long 		phdrsz;
	off64_t		offset;
	Elf32_Off	poffset;
	int 		error;
	int		i;
	int		index = 0;
	int		nhdrs;
	struct seg 	*seg;
	union info_item {
		pstatus_t 	prstat;
		psinfo_t 	psinfo;
		prcred_t   	prcred;
		lwpstatus_t	lwpstatus;
		lwpsinfo_t	lwpsinfo;
		struct utsname	uname;
	} *infop;
	lwp_t		*walkp;
	struct as	*asp = p->p_as;

	infop = kmem_alloc(sizeof *infop, KM_SLEEP);

	ASSERT(getpl() == PLBASE);

	/*
	 * The total number of note sections we will need will be one more than
	 * the number of LWPs in the process (one note section for the
	 * process-wide info and one note section per LWP). And so, the
	 * number of entries in the program header is given by: Total number
	 * regions of memory that have different protections in the address
	 * space + Total number of note sections.
	 */

	nhdrs = getnsegs(p) + p->p_nlwp + 1;
	phdrsz = nhdrs * sizeof (Elf32_Phdr);

	phdrp = kmem_zalloc(phdrsz, KM_SLEEP);

	bzero(&ehdr, sizeof ehdr);
	ehdr.e_ident[EI_MAG0] = ELFMAG0;
	ehdr.e_ident[EI_MAG1] = ELFMAG1;
	ehdr.e_ident[EI_MAG2] = ELFMAG2;
	ehdr.e_ident[EI_MAG3] = ELFMAG3;
	ehdr.e_ident[EI_CLASS] = ELFCLASS32;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_type = ET_CORE;
	ehdr.e_machine = EM_386;
	ehdr.e_version = EV_CURRENT;
	ehdr.e_phoff = sizeof (Elf32_Ehdr);
	ehdr.e_ehsize = sizeof (Elf32_Ehdr);
	ehdr.e_phentsize = sizeof (Elf32_Phdr);
	ehdr.e_phnum = (unsigned short)nhdrs;

	/*
	 * Write the elf header out.
	 */
	if (error = WR(vp, &ehdr, sizeof (Elf32_Ehdr), 0, rlimit, credp))
		goto done;

	offset = sizeof (Elf32_Ehdr);
	poffset = sizeof (Elf32_Ehdr) + phdrsz;

	/*
	 * Initialize the entry for process-wide note section.
	 */

	phdrp[index].p_type = PT_NOTE;
	phdrp[index].p_flags = PF_R;
	phdrp[index].p_elfoffset = poffset;
	phdrp[index].p_filesz =
		(sizeof (Elf32_Note) * 4 +
		 roundup(sizeof (pstatus_t), sizeof (Elf32_Word)) +
		 roundup(sizeof (psinfo_t), sizeof (Elf32_Word)) +
		 roundup(sizeof (prcred_t), sizeof (Elf32_Word)) +
		 roundup(sizeof (struct utsname), sizeof (Elf32_Word)));
	poffset += phdrp[0].p_filesz;
	index++;

	/*
	 * Initialize the program header entries for the per-lwp
	 * note sections.
	 */

	for (walkp = p->p_lwpp; walkp != NULL; walkp = walkp->l_next) {
		phdrp[index].p_type = PT_NOTE;
		phdrp[index].p_flags = PF_R;
		phdrp[index].p_elfoffset = poffset;
		phdrp[index].p_filesz =
			(sizeof (Elf32_Note) * 2 +
			 roundup(sizeof (lwpstatus_t), sizeof (Elf32_Word)) +
			 roundup(sizeof (lwpsinfo_t), sizeof (Elf32_Word)));

		poffset += phdrp[index].p_filesz;
		index++;
	}

	for (i = index, seg = asp->a_segs; i < nhdrs; seg = seg->s_next) {
		vaddr_t naddr;
		vaddr_t saddr = seg->s_base;
		vaddr_t eaddr = seg->s_base + seg->s_size;
		do {
			uint_t prot, size;
			/*
			 * Note that we call as_getprot() here without
			 * holding the AS lock, since the process
			 * MUST be single threaded when we are executing this
			 * code.
			 */
			prot = as_getprot(asp, saddr, &naddr);
			size = naddr - saddr;
			phdrp[i].p_type = PT_LOAD;
			phdrp[i].p_vaddr = (Elf32_Word)saddr;
			phdrp[i].p_memsz = size;
			if (prot & PROT_WRITE)
				phdrp[i].p_flags |= PF_W;
			if (prot & PROT_READ)
				phdrp[i].p_flags |= PF_R;
			if (prot & PROT_EXEC)
				phdrp[i].p_flags |= PF_X;

			if (prot & PROT_WRITE) {
				/*
				 * Pages in the range [saddr, naddr) are
				 * writable; these will be dumped if they
				 * are backed by real memory.
				 */
				if (as_memory(asp, &saddr, &size) == 0) {
					/*
					 * The range [saddr, saddr+size) is
					 * writable and backed by real memory.
					 * This will be dumped.  Note that the
					 * mapped size, p_memsz, may need to be
					 * adjusted but may still extend beyond
					 * the pages actually dumped.
					 */
					phdrp[i].p_memsz -=
					  (Elf32_Word)saddr - phdrp[i].p_vaddr;
					phdrp[i].p_vaddr = (Elf32_Word)saddr;
					phdrp[i].p_filesz = size;
					phdrp[i].p_elfoffset = poffset;
					poffset += size;
				}
			}
			saddr = naddr;
			i++;
		} while (naddr < eaddr);
	}

	/*
	 * Write the program header to the core file.
	 */

	error = WR(vp, phdrp, phdrsz, offset, rlimit, credp);
	if (error)
		goto done;
	offset += phdrsz;

	/*
	 * Write the note sections for the process-wide data
	 * (pstatus, psinfo, credentials, utsname).
	 */
	error = prgetpstatus(p, &infop->prstat);
	if (error)
		goto done;

	infop->prstat.pr_lwp.pr_cursig = sig;

	error = elfnote(vp, &offset, CF_T_PRSTATUS,
			sizeof (pstatus_t), &infop->prstat, rlimit, credp);
	if (error)
		goto done;

	bzero(&infop->psinfo, sizeof (psinfo_t));
	(void)LOCK(&p->p_mutex, PLHI);
	prgetpsinfo(p, &infop->psinfo);
	ASSERT(KS_HOLD0LOCKS());
	error = elfnote(vp, &offset, CF_T_PRPSINFO,
			sizeof (psinfo_t), &infop->psinfo, rlimit, credp);
	if (error)
		goto done;

	prgetcred(p, &infop->prcred);
	error = elfnote(vp, &offset, CF_T_PRCRED,
			sizeof (prcred_t), &infop->prcred, rlimit, credp);
	if (error)
		goto done;

	prgetutsname(p, &infop->uname);
	error = elfnote(vp, &offset, CF_T_UTSNAME,
			sizeof (struct utsname), &infop->uname, rlimit, credp);
	if (error)
		goto done;

	/*
	 * Dump the note sections for the per-lwp data.
	 */

	for (walkp = p->p_lwpp; walkp != NULL; walkp = walkp->l_next) {
		error = prgetlwpstatus(walkp, &infop->lwpstatus);
		if (error)
			goto done;
		if (walkp == u.u_lwpp)
			infop->lwpstatus.pr_cursig = sig;
		error = elfnote(vp, &offset, CF_T_LWPSTATUS,
				sizeof (lwpstatus_t), &infop->lwpstatus,
				rlimit, credp);
		if (error)
			goto done;

		bzero(&infop->lwpsinfo, sizeof (lwpsinfo_t));
		(void)LOCK(&p->p_mutex, PLHI);
		prgetlwpsinfo(walkp, &infop->lwpsinfo);
		ASSERT(KS_HOLD0LOCKS());
		error = elfnote(vp, &offset, CF_T_LWPSINFO,
				sizeof (lwpsinfo_t), &infop->lwpsinfo,
				rlimit, credp);
		if (error)
			goto done;
	}

	/*
	 * Dump out the address space. For regions of memory which should not
	 * be dumped, (regions which were not writable will not be dumped)
	 * the p_filesz in the corresponding program header entry will have
	 * been set to 0.
	 */

	for (i = index; !error && i < nhdrs; i++) {
		if (phdrp[i].p_filesz == 0)
			continue;
		error = core_seg(p, vp, phdrp[i].p_elfoffset,
				 phdrp[i].p_vaddr,
		  		 phdrp[i].p_filesz, rlimit, credp);
	}

done:
	kmem_free(phdrp, phdrsz);
	kmem_free(infop, sizeof *infop);
	return error;
}

/*
 * STATIC int
 * getnsegs(proc_t *p)
 *	Count the number of segments in this process's address space.
 *	We always return 0 for a system process.
 *
 * Calling/Exit State:
 *	The caller has ensured that the structure of the address space
 *	is stable.
 */
STATIC int
getnsegs(proc_t *p)
{
	int 		n;
	vaddr_t 	saddr;
	vaddr_t		addr;
	vaddr_t		eaddr;
	struct seg 	*seg;
	struct seg	*sseg;
	struct as 	*as;

	n = 0;
	if (!(p->p_flag & P_SYS) &&
	    (as = p->p_as) != NULL &&
	    (seg = sseg = as->a_segs) != NULL) {
		do {
			saddr = seg->s_base;
			eaddr = seg->s_base + seg->s_size;
			do {
				(void)as_getprot(as, saddr, &addr);
				n++;
			} while ((saddr = addr) != eaddr);
		} while ((seg = seg->s_next) != sseg);
	}

	return n;
}
