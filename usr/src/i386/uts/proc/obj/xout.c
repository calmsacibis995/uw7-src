#ident	"@(#)kern-i386:proc/obj/xout.c	1.8.2.1"
#ident	"$Header$"

/* XENIX Support */
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <proc/mman.h>
#include <mem/kmem.h>
#include <fs/fstyp.h>
#include <util/var.h>
#include <proc/proc.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <io/conf.h>
#include <fs/pathname.h>
#include <proc/obj/x.out.h>
#include <proc/exec.h>
#include <proc/seg.h>
#include <mem/immu.h>
#include <mem/seg.h>
#include <mem/as.h>
#include <mem/vmparam.h>
#include <mem/seg_vn.h>

#define	 X286EMUL	"/usr/bin/x286emul"


static int getxouthead(vnode_t *, struct exdata *,
		       long *, exhda_t *, struct uarg *);
static int readxouthdr(vnode_t *, struct exdata *,
		       struct xexec *, exhda_t *, long *);


/*
 * int
 * xoutexec(vnode_t *vp, struct uarg *args, int level, long *execsz,
 *	    exhda_t *ehdp)
 *	Exec a Xenix x.out executable file.
 *
 * Calling/Exit State:
 *	Called from gexec via execsw[].
 *
/* ARGSUSED */
int
xoutexec(struct vnode	*vp,
	 struct uarg	*args,
	 int		level,
	 long		*execsz,
	 exhda_t	*ehdp)
{
	struct exdata	*edp = &args->execinfop->ei_exdata;
	int		error;
	struct segment_desc dscr;
	vaddr_t		base;
	size_t		size;
	off64_t		offset;
	u_long		adjust = 0;
	vaddr_t		regva;
	struct proc	*p = u.u_procp;

	error = getxouthead(vp, edp, execsz, ehdp, args);
	if (error == -1)
		return 0;		/* Emulator was loaded */
	if (error != 0)
		return error;

	/*
	 * Remove current process image and allocate new address space.
	 */
	if ((error = remove_proc(args, vp, UVSTACK, 0, execsz)) != 0)
		return error;

	/* Single threaded upon return from remove_proc(). */
	ASSERT(SINGLE_THREADED());

	/*
	 * Load the text section.
	 */

	/* allow for text origins other than 0 */
	size = edp->ex_tsize + edp->ex_txtorg;
	base = edp->ex_txtorg;
	offset = edp->ex_toffset;

	/* x.out file offsets are confined to 32 bits */
	ASSERT((off64_t)(vaddr_t)offset == offset);

	/*
	 * First make sure that text & data don't overlap for
	 * separate I & D space processes. (note that 413 x.out binaries
	 * are normally split also.)
	 *
	 * NOTE: for now we just force text to top of address space,
	 * we may be able to fix this up for more efficient use of page
	 * tables later.  With the current x.out layout, we could
	 * usually fit it below the data, but that would required sharing
	 * regions, and re-mapping the start of data.
	 */
	regva = (0xBFFFF000 - (size + (vaddr_t)offset)) & ~SOFFMASK;

	/*
	 * set up segment descriptor.  The 'adjust' is so we
	 * can handle the non-aligned part in the segment table,
	 * to allow for non-filesystem-block aligned binaries,
	 * without a lot of extra overhead (i.e., no extra read
	 * in S5READMAP()).  Olson, 3/87
	 */
	if (base <= (vaddr_t)offset) {
		if (PAGOFF(base) != PAGOFF(offset)) {
			adjust = PAGOFF((vaddr_t)offset - base);
		}
	}

	/* Need to use a non-standard code segment */
	BUILD_MEM_DESC(&dscr, regva + adjust, mmu_btopr(size),
		       UTEXT_ACC1, TEXT_ACC2);
	(void)set_dt_entry(USER_CS, &dscr);

	if ((error = execmap(edp->ex_vp, regva + adjust, size,
			     0, offset, (PROT_ALL & ~PROT_WRITE))) != 0) {
		sigtoproc(p, SIGKILL, (sigqueue_t *)NULL);
		return error;
	}

	/*
	 * Load the data section.
	 */

	offset = edp->ex_doffset;
	base = edp->ex_datorg;

	if ((error = execmap(edp->ex_vp, base, edp->ex_dsize,
			     edp->ex_bsize, offset, PROT_ALL)) != 0) {
		sigtoproc(p, SIGKILL, (sigqueue_t *)NULL);
		return error;
	}

	/*
	 * For XENIX binaries the data page at virtual address 0
	 * must be accessible to allow NULL pointer dereferences.
	 * If not already attached to the process address space,
	 * map in the first page.
	 */
	as_wrlock(p->p_as);
	if (as_segat(p->p_as, 0) == NULL) {
		if ((error = as_map(p->p_as, 0, PAGESIZE,
				    segvn_create, zfod_argsp)) != 0) {
			as_unlock(p->p_as);
			return error;
		}
	}
	as_unlock(p->p_as);

	setexecenv(base + edp->ex_dsize + edp->ex_bsize);

	args->rvp->r_val1 = (long) base + edp->ex_dsize + edp->ex_bsize;

	args->rvp->r_val2 = USER_DS;

	return 0;
}


/*
 * static int
 * getxouthead(vnode_t *vp, struct exdata *edp, long *execsz, exhda_t *ehdp)
 *	Get the X.OUT file header information.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.  This routine may block.
 *	The process is single-threaded at this point.
 */
static int
getxouthead(vnode_t *vp,
	    struct exdata *edp,
	    long *execsz,
	    exhda_t *ehdp,
	    struct uarg *args)
{
	struct xexec *filhdrp;
	int error;

	error = exhd_read(ehdp, 0, sizeof *filhdrp, (void **)&filhdrp);
	if (error != 0)
		return error;

	if (!(filhdrp->x_renv & XE_EXEC))
		return ENOEXEC;

	switch (filhdrp->x_magic) {
	case X_MAGIC:
		if ((filhdrp->x_cpu & XC_386) != XC_386) {

			/*
			 * the emulator must determine if this is
			 * a valid 8086 or 80286 binary.
			 */
			if (error = setxemulate(X286EMUL, args, execsz))
				return error;

			/* support for execute only binaries */
			edp->ex_renv |= RE_EMUL;
			/* let xoutexec know - don't want to go any further */
			return -1;
		}
		break;
	default:
		return ENOEXEC;
	}

	if (!readxouthdr(vp, edp, filhdrp, ehdp, execsz))
		return ENOEXEC;

	edp->ex_vp = vp;
	return 0;
}


/*
 * static int
 * readxouthdr(vnode_t *vp, struct exdata *edp,
 *	       struct xexec *xouthdr, exhda_t *ehdp, long *execsz)
 *	Get the X.OUT file header information.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.  This routine may block.
 *	The process is single-threaded at this point.
 *
 * Remarks:
 *	XENIX 386 x.out binary COMPATIBILITY
 */
static int
readxouthdr(vnode_t *vp,
	    struct exdata *up,
	    struct xexec *xouthdr,
	    exhda_t *ehdp,
	    long *execsz)
{
	unsigned segsize;
	off64_t	 segoffset;
	struct xext  *exext;	/* extension to header */
	struct xseg  *xseg;
	int    error;

	/* Fill in what we can now */

	up->ex_tsize = xouthdr->x_text;
	up->ex_dsize = xouthdr->x_data;
	up->ex_bsize = xouthdr->x_bss;

	*execsz += btopr(up->ex_tsize + up->ex_dsize + up->ex_bsize);

	/* No shared library support yet */
	up->ex_nshlibs = 0;
	up->ex_lsize = 0;
	up->ex_loffset = 0;

/* Enhanced Application Compatibility Support */
	/*
	 * don't allow 386 impure or multiple segment binaries.
	 * To support pure-segmented 386 executables, we no longer require
	 * the XE_SEP flag to be set.  Instead, we check the relative
	 * positions of the text and data at the end of this function.
	 */
	if (((xouthdr->x_cpu & XC_386) == XC_386) &&
	    (xouthdr->x_renv & (XE_LTEXT|XE_LDATA))) {
		return 0;
	}
/* End Enhanced Application Compatibility Support */

	up->ex_entloc = xouthdr->x_entry;

	/*
	 * this may be the wrong magic number to use
	 * since we do paging only if the file alignment
	 * meets paging requirement
	 */
	up->ex_mag = 0413;
	up->ex_renv = xouthdr->x_renv | (xouthdr->x_cpu << 16);

	/*
	 * get header extension
	 * xe_eseg is the last structure element in xext
	 */
	if (xouthdr->x_ext < STRUCTOFF(xext,xe_eseg) + sizeof(exext->xe_eseg))
		return 0;

	error = exhd_read(ehdp, sizeof (struct xexec),
			  min(sizeof exext, xouthdr->x_ext), (void **)&exext);
	if (error != 0)
		return 0;

	/* Get the segment table */
	for (segsize=0, segoffset=exext->xe_segpos;
	     segsize < exext->xe_segsize;
	     segsize += sizeof xseg, segoffset += sizeof xseg) {
		error = exhd_read(ehdp, segoffset, sizeof *xseg,
				  (void **)&xseg);
		if (error != 0)
			return 0;

		if (xseg->xs_type == XS_TTEXT) {
			if ((xseg->xs_seg&(SEL_LDT|SEL_RPL)) !=
			    (SEL_LDT|SEL_RPL))
				return 0; /* must be in LDT at ring 3 */
			up->ex_toffset = xseg->xs_filpos;
			up->ex_txtorg  = xseg->xs_rbase;
		} else if (xseg->xs_type == XS_TDATA) {
			if ((xseg->xs_seg&(SEL_LDT|SEL_RPL)) !=
			    (SEL_LDT|SEL_RPL))
				return 0; /* must be in LDT at ring 3 */
			up->ex_doffset = xseg->xs_filpos;
			up->ex_datorg  = xseg->xs_rbase;

			/*
			 * Some xenix binaries (mainly 8086/80286) treat
			 * data that is initialized to 0's as though it
			 * was BSS, so we have to recalculate the dsize
			 * and bsize, so we won't get errors in mapping
			 * segment, since the 'non-existent' initialized
			 * 0 data may extend past the actual end of the
			 * file.  Olson, 5/87
			 */
			if (!(xouthdr->x_renv & (XE_LTEXT|XE_LDATA)) &&
			    (u_int) xseg->xs_psize < up->ex_dsize) {
				up->ex_dsize = xseg->xs_psize;
				up->ex_bsize = xseg->xs_vsize - xseg->xs_psize;
			}
		}
	}

/* Enhanced Application Compatibility Support */
	/*
	 * Make sure that the text fits below (numerically) the data and that
	 *  the beginning data address is page aligned.
	 */
	if (((xouthdr->x_cpu & XC_386) == XC_386) &&
	    (xouthdr->x_renv & XE_SEP) == 0) {
		if (up->ex_tsize + up->ex_txtorg +
		    (up->ex_toffset % (VBSIZE(vp))) >
		    (up->ex_datorg & 0xfffc0000))
			return 0;
		if (up->ex_datorg & PAGEOFFSET)
			return 0;
	}
/* End Enhanced Application Compatibility Support */

	return 1;	/* OK */
}


/*
 * int
 * xoutcore(vnode_t *vp, proc_t *pp, cred_t *credp, rlim_t rlimit, int sig)
 *	X.OUT specific core routine, called through the execsw table.
 *
 * Calling/Exit State:
 *	Same as elfcore().
 *
 * Description:
 *	Just use elfcore, and dump it in ELF format.
 */
int
xoutcore(vnode_t *vp, proc_t *pp, cred_t *credp, rlim_t rlimit, int sig)
{
	extern int elfcore(vnode_t *, proc_t *, cred_t *, rlim_t, int);

	return elfcore(vp, pp, credp, rlimit, sig);
}
/* End XENIX Support */
