#ident	"@(#)kern-i386:proc/obj/coff.c	1.11.3.1"
#ident	"$Header$"

/*
 * This file contains COFF executable object specific routines.
 */

#include <acc/mac/mac.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/obj/x.out.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/mod/moddefs.h>
#include <util/param.h>
#include <util/types.h>

/* Enhanced Application Compatibility Support */
#include <io/termios.h>
#include <svc/sco.h>
/* End Enhanced Application Compatibility Support */

MOD_EXEC_WRAPPER(coff, NULL, NULL, "coff - loadable exec module");

int getcoffshlibs(struct exdata *edp, struct exdata *dat_start, long *execsz,
		  exhda_t *ehdp);
void coffexec_err(struct exdata *shlb_data, long nlibs);

STATIC int getcoffhead(vnode_t *vp, struct exdata *edp, long *execsz,
		       exhda_t *ehdp);

/*
 * int
 * coffexec(vnode_t *vp, struct uarg *args, int level, long *execsz,
 *	COFF specific exec routine, called through the execsw table.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.  This routine may block.
 *	The process is single-threaded at this point.
 */
/* ARGSUSED */
int
coffexec(vnode_t *vp, struct uarg *args, int level, long *execsz,
	 exhda_t *ehdp)
{
	execinfo_t *eip;
	struct exdata *edp;
	int error = 0;
	int i;
	struct exdata *shlb_dat, *datp;
	size_t shlb_datsz;
	const int dataprot = PROT_ALL;
	const int textprot = PROT_ALL & ~PROT_WRITE;
	proc_t *pp = u.u_procp;

	eip = args->execinfop;
	edp = &eip->ei_exdata;

	/*
	 * Get COFF header information.
	 */
	if ((error = getcoffhead(vp, edp, execsz, ehdp)) != 0)
		return error;

        /*
         * Look at what we got, edp->ex_mag = 410/411/413.
	 *
         * 410 is RO text.
	 * 411 is separated ID (treated like a 0410).
	 * 413 is RO text in an "aligned" a.out file.
	 */
	switch (edp->ex_mag) {
	case 0410:
	case 0411:
	case 0413:
                break;

        case 0443:
                return ELIBEXEC;

	default:
		return ENOEXEC;
	}


	if (edp->ex_nshlibs != 0) {
		/*
		 * Determine space needed for library headers.
		 */
		shlb_datsz = edp->ex_nshlibs * sizeof(struct exdata);

		shlb_dat = (struct exdata *)kmem_alloc(shlb_datsz, KM_SLEEP);
		datp = shlb_dat;

		/*
		 * Get header information for each needed library.
		 */
		if ((error = getcoffshlibs(edp, datp, execsz, ehdp)) != 0)
			goto done;
	}

	/*
	 * Remove current process image and allocate new address space.
	 */
	if ((error = remove_proc(args, vp, UVSTACK, 0, execsz)) != 0)
		goto done;

	/* Single threaded upon return from remove_proc(). */
	ASSERT(SINGLE_THREADED());

	/*
	 * Load any shared libraries that are needed.
	 */
	if (edp->ex_nshlibs) {
		for (i = 0; i < edp->ex_nshlibs; i++, datp++) {
			/*
			 * Load text.
			 */
			if ((error = execmap(datp->ex_vp,
					     (vaddr_t)datp->ex_txtorg,
					     datp->ex_tsize, (size_t)0,
					     datp->ex_toffset,
					     textprot)) != 0) {
				coffexec_err(++datp, edp->ex_nshlibs - i - 1);
				sigtoproc(pp, SIGKILL, (sigqueue_t *)NULL);
				goto done;
			}

			/*
			 * Load data.
			 */
			if ((error = execmap(datp->ex_vp,
					     (vaddr_t)datp->ex_datorg,
					     datp->ex_dsize,
					     datp->ex_bsize,
					     datp->ex_doffset,
					     dataprot)) != 0) {
				coffexec_err(++datp, edp->ex_nshlibs - i - 1);
				sigtoproc(pp, SIGKILL, (sigqueue_t *)NULL);
				goto done;
			}
			VN_RELE(datp->ex_vp);	/* done with this reference */
		}
	}

	/*
	 * Load the a.out's text and data.
	 */
	if ((error = execmap(edp->ex_vp, (vaddr_t)edp->ex_txtorg,
			     edp->ex_tsize, (size_t)0, edp->ex_toffset,
			     textprot)) != 0) {
		sigtoproc(pp, SIGKILL, (sigqueue_t *)NULL);
		goto done;
	}

	if ((error = execmap(edp->ex_vp, (vaddr_t)edp->ex_datorg,
			     edp->ex_dsize, edp->ex_bsize, edp->ex_doffset,
			     dataprot)) != 0) {
		sigtoproc(pp, SIGKILL, (sigqueue_t *)NULL);
		goto done;
	}

	/*
	 * Set runtime environment flags.
	 */
	edp->ex_renv = XE_V5|XE_EXEC|RE_IS386|RE_ISCOFF|RE_ISWSWAP;

	/* Enhanced Application Compatibility Support */
	
	/* all COFF is assumed to be SCO */
	edp->ex_renv2 |= RE_ISSCO;

	/* End Enhanced Application Compatibility Support */

        setexecenv((vaddr_t)edp->ex_datorg + edp->ex_dsize + edp->ex_bsize);

done:
	if (edp->ex_nshlibs != 0)
		kmem_free(shlb_dat, shlb_datsz);

	return error;
}


/*
 * Read the a.out headers.  There must be at least three sections,
 * and they must be .text, .data and .bss (although not necessarily
 * in that order).
 *
 * Possible magic numbers are 0410, 0411 (treated as 0410), and
 * 0413.  If there is no optional UNIX header then magic number
 * 0410 is assumed.
 */


/*
 *   Common object file header.
 */

/* f_magic (magic number)
 *
 *   NOTE:   For 3b-5, the old values of magic numbers
 *           will be in the optional header in the
 *           structure "aouthdr" (identical to old
 *           unix aouthdr).
 */

#define	IAPX386MAGIC	0514

/* f_flags
 *
 *	F_EXEC  	file is executable (i.e. no unresolved
 *	        	  externel references).
 *	F_AR16WR	this file created on AR16WR machine
 *	        	  (e.g. 11/70).
 *	F_AR32WR	this file created on AR32WR machine
 *	        	  (e.g. vax).
 *	F_AR32W		this file created on AR32W machine
 *	        	  (e.g. 3B, maxi).
 */
#define  F_EXEC		0000002
#define  F_AR16WR	0000200
#define  F_AR32WR	0000400
#define  F_AR32W	0001000

struct filehdr {
	u_short	f_magic;
	u_short	f_nscns;	/* number of sections */
	long	f_timdat;	/* time & date stamp */
	long	f_symptr;	/* file pointer to symtab */
	long	f_nsyms;	/* number of symtab entries */
	u_short	f_opthdr;	/* sizeof(optional hdr) */
	u_short	f_flags;
};

/*
 *  Common object file section header.
 */

/*
 *  s_name
 */
#define _TEXT ".text"
#define _DATA ".data"
#define _BSS  ".bss"
#define _LIB  ".lib"

/*
 * s_flags
 */
#define	STYP_TEXT	0x0020	/* section contains text only */
#define STYP_DATA	0x0040	/* section contains data only */
#define STYP_BSS	0x0080	/* section contains bss only  */
#define STYP_LIB	0x0800	/* section contains lib only  */

struct scnhdr {
	char	s_name[8];	/* section name */
	long	s_paddr;	/* physical address */
	long	s_vaddr;	/* virtual address */
	long	s_size;		/* section size */
	long	s_scnptr;	/* file ptr to raw	*/
				/* data for section	*/
	long	s_relptr;	/* file ptr to relocation */
	long	s_lnnoptr;	/* file ptr to line numbers */
	u_short	s_nreloc;	/* number of relocation	*/
				/* entries		*/
	u_short	s_nlnno;	/* number of line	*/
				/* number entries	*/
	long	s_flags;	/* flags */
};

/*
 * Common object file optional unix header.
 */

struct aouthdr {
	short	o_magic;	/* magic number */
	short	o_stamp;	/* stamp */
	long	o_tsize;	/* text size */
	long	o_dsize;	/* data size */
	long	o_bsize;	/* bss size */
	long	o_entloc;	/* entry location */
	long	o_tstart;
	long	o_dstart;
};

/*
 * STATIC int
 * getcoffhead(vnode_t *vp, struct exdata *edp, long *execsz, exhda_t *ehdp)
 *	Get the COFF file header information.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.  This routine may block.
 *	The process is single-threaded at this point.
 */
STATIC int
getcoffhead(vnode_t *vp, struct exdata *edp, long *execsz, exhda_t *ehdp)
{
	struct filehdr *filhdrp;
	struct aouthdr *aouthdrp;
	struct scnhdr  *scnhdrp;
	int opt_hdr = 0;
	int scns = 0;
	off64_t offset;
	int error;
	int nscns;
	size_t ssz;

	/*
	 * Read COFF file header.
	 */
	error = exhd_read(ehdp, 0, sizeof *filhdrp, (void **)&filhdrp);
	if (error != 0)
		return error;

	if (filhdrp->f_magic != IAPX386MAGIC || !(filhdrp->f_flags & F_EXEC))
		return ENOEXEC;

	/*
	 * Get what is needed from filhdrp now, since it is
	 * not kosher to modify the file page and little is needed.
	 */
	nscns = filhdrp->f_nscns;
	offset = sizeof(*filhdrp) + filhdrp->f_opthdr;

	/*
	 * Next, read the optional unix header if present; if not,
	 * then we will assume the file is a 410.
	 */
	if (filhdrp->f_opthdr >= sizeof *aouthdrp) {
		error = exhd_read(ehdp, sizeof *filhdrp,
				  sizeof *aouthdrp, (void **)&aouthdrp);
		if (error)
			return error;

		opt_hdr = 1;
		edp->ex_mag = aouthdrp->o_magic;
		edp->ex_entloc = aouthdrp->o_entloc;
	}

	/*
	 * Next, read the section headers.  There had better be at
	 * least three: .text, .data and .bss.  The shared library
	 * section is optional; initialize the number needed to 0.
	 */

	edp->ex_nshlibs = 0;

	ssz = nscns * sizeof *scnhdrp;
	error = exhd_read(ehdp, offset, ssz, (void **)&scnhdrp);
	if (error)
		return error;

	/*
	 * Determine which sections are present, and
	 * populate edp as required.
	 */
	for (; nscns > 0; scnhdrp++, nscns--) {

		switch ((int)scnhdrp->s_flags) {

		case STYP_TEXT:
			scns |= STYP_TEXT;

			if (!opt_hdr) {
				edp->ex_mag = 0410;
				edp->ex_entloc = scnhdrp->s_vaddr;
			}

			edp->ex_txtorg = scnhdrp->s_vaddr;
			edp->ex_toffset = scnhdrp->s_scnptr;
			*execsz += btopr(edp->ex_tsize = scnhdrp->s_size);
			break;

		case STYP_DATA:
			scns |= STYP_DATA;
			edp->ex_datorg = scnhdrp->s_vaddr;
			edp->ex_doffset = scnhdrp->s_scnptr;
			*execsz += btopr(edp->ex_dsize = scnhdrp->s_size);
			break;

		case STYP_BSS:
			scns |= STYP_BSS;
			*execsz += btopr(edp->ex_bsize = scnhdrp->s_size);
			break;

		case STYP_LIB:
			++shlbinfo.shlblnks;

			if ((edp->ex_nshlibs = scnhdrp->s_paddr) >
			     shlbinfo.shlbs) {
                                ++shlbinfo.shlbovf;
                                return ELIBMAX;
                        }

			edp->ex_lsize = scnhdrp->s_size;
			edp->ex_loffset = scnhdrp->s_scnptr;
			break;
		}
	}

	if (scns != (STYP_TEXT|STYP_DATA|STYP_BSS))
		return ENOEXEC;

	edp->ex_vp = vp;
	return 0;
}

/*
 * int
 * getcoffshlibs(struct exdata *edp, struct exdata *dat_start, long *execsz,
 *		 exhda_t *ehdp)
 *	Get header information for each needed library.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.  This routine may block.
 *	The process is single-threaded at this point.
 */
int
getcoffshlibs(struct exdata *edp, struct exdata *dat_start, long *execsz,
	      exhda_t *ehdp)
{
	vnode_t *nvp;
	uint_t *bp;
	struct exdata *dat = dat_start;
	uint_t n = 0;
	uint_t *libend;
	char *shlibname;
	struct vattr vattr;
	int error;
	exhda_t lhda;

	/*
	 * Read library section.
	 */
	error = exhd_read(ehdp, edp->ex_loffset, edp->ex_lsize, (void **)&bp);
	if (error)
		return error;

	edp->ex_nshlibs = 0;   /* ELF may call this code */
	libend = bp + (edp->ex_lsize / sizeof(*bp));

	while (bp < libend) {

		/* Check the validity of the shared lib entry. */

		if (bp[0] * NBPW > edp->ex_lsize
		  || bp[1] > edp->ex_lsize || bp[0] < 3) {
			error = ELIBSCN;
			goto bad;
		}

		/* Locate the shared lib and get its header info.  */

		shlibname = (caddr_t)(bp + bp[1]);
		bp += bp[0];

		if (lookupname(shlibname, UIO_SYSSPACE, FOLLOW, NULLVPP, &nvp)
						!= 0) {
			error = ELIBACC;
			goto bad;
		}

		/* check for MAC exec access */
		if (MAC_VACCESS(nvp, VEXEC, CRED()))  {
			error = ELIBACC;
			VN_RELE(nvp);
			goto bad;
		}

		/* Enhanced Application Compatibility Support */

		/*
		 * We need to remember if the executable uses the static
		 * shared NSL library so that we can do the right thing in
		 * "os/streamio.c:strioctl()".
		 */
		if (strcmp(shlibname, SHNSLPATH) == 0 ||
		    strcmp(shlibname, SCO_SHNSLPATH) == 0)
			edp->ex_renv2 |= SCO_SHNSL;

		/* End Enhanced Application Compatibility Support */

		/*
		 * nvp has a VN_HOLD on it,
		 * so a VN_RELE must happen for all error cases.
		 */

		vattr.va_mask = AT_MODE;
		if ((error = VOP_GETATTR(nvp, &vattr, ATTR_EXEC, CRED()))
						!= 0) {
			VN_RELE(nvp);
			goto bad;
		}

		if ((error = VOP_ACCESS(nvp, VEXEC, 0, CRED())) != 0
		      || nvp->v_type != VREG
		      || (vattr.va_mode & (VEXEC|(VEXEC>>3)|(VEXEC>>6))) == 0) {
			error = ELIBACC;
			VN_RELE(nvp);
			goto bad;
		}

		struct_zero(&lhda, sizeof(lhda));
		lhda.exhda_vp = nvp;
		lhda.exhda_vnsize = vattr.va_size;

		/*
		 * Get file header information for library.
		 */
		error = getcoffhead(nvp, dat, execsz, &lhda);
		exhd_release(&lhda);

		if (error || dat->ex_mag != 0443) {
			error = ELIBBAD;
			VN_RELE(nvp);
			goto bad;
		}

		++dat;
		++edp->ex_nshlibs;
		++n;
	}

	return 0;

bad:
	coffexec_err(dat_start, (long)n);
	return error;
}


/*
 * void
 * coffexec_err(struct exdata *shlb_data, long nlibs)
 *	Release holds on library vnodes upon error.
 *
 * Calling/Exit State:
 *	No locks are required on entry and none are held on exit.
 */
void
coffexec_err(struct exdata *shlb_data, long nlibs)
{
	for (; nlibs > 0; --nlibs, ++shlb_data)
		VN_RELE(shlb_data->ex_vp);
}


/*
 * int
 * coffcore(vnode_t *vp, proc_t *pp, cred_t *credp, rlim_t rlimit, int sig)
 *	COFF specific core routine, called through the execsw table.
 *
 * Calling/Exit State:
 *	Same as elfcore().
 *
 * Description:
 *	Just use elfcore, and dump it in ELF format.
 */
int
coffcore(vnode_t *vp, proc_t *pp, cred_t *credp, rlim_t rlimit, int sig)
{
	extern int elfcore(vnode_t *, proc_t *, cred_t *, rlim_t, int);

	return elfcore(vp, pp, credp, rlimit, sig);
}
