#ident	"@(#)kern-i386:fs/profs/profs_mdep.c	1.1.5.1"
#ident	"$Header$"

#include <fs/fski.h>
#include <fs/profs/prosrfs.h>
#include <svc/cpu.h>
#include <svc/fp.h>
#include <util/debug.h>
#include <util/plocal.h>

/*
 * void
 * pro_get_chip_fpu(struct plocal *plp, pfsproc_t *pfsprocp)
 *
 *	Get the chip type and fpu kind.
 *
 * Calling/Exit State:
 *
 *	This function is called from the VOP proread().
 *	The calling LWP must hold the inode's rwlock in at least
 *	*shared* mode on entry; rwlock remains held on exit.
 *
 * Description:
 *
 *	This function gets the chip type and the fpu type
 *	and populates the appropriate members of the
 *	struct pfsproc, whose pointer is passed in as an
 *	argument.
 */
void
pro_get_chip_fpu(struct plocal *plp, pfsproc_t *pfsprocp)
{
	char *p;

	switch (plp->cpu_id) {
	case CPU_386:
		pfsprocp->pfs_chip = I_386;	/* i386 */
		break;
	case CPU_486:
		pfsprocp->pfs_chip = I_486;	/* i486 */
		break;
	case CPU_P5:
		pfsprocp->pfs_chip = I_586;	/* P5 (Pentium) */
		break;
	case CPU_P6:
		pfsprocp->pfs_chip = I_686;	/* P6 (Pentium Pro) */
		break;
	default:
		pfsprocp->pfs_chip = 0;		/* unknown */
		break;
	}

	switch (fp_kind) {
	case FP_287:
		pfsprocp->pfs_fpu = FPU_287;	/* 287 present */
		break;
	case FP_387:
		pfsprocp->pfs_fpu = FPU_387;	/* 387 present */
		break;
	default:
		pfsprocp->pfs_fpu = FPU_NONE;	/* no FPU present */
		break;
	}

	/*
	 * The chip type (I_XXX, above) tells you what kind of chip we're
	 * treating this as, whereas the pfs_name string is the actual
	 * processor type as best we could identify it.  For example, the
	 * chip type might be I_586 for some Pentium clone, while the name
	 * string would be the vendor's trademarked name for their chip.
	 */
	p = (plp->cpu_fullname == NULL)? "????" : (char *)plp->cpu_fullname;
	strncpy(pfsprocp->pfs_name, p, PFS_NAMELEN - 1);
	pfsprocp->pfs_name[PFS_NAMELEN - 1] = '\0';

	/*
	 * There are no requirements regarding the contents of the pfs_model
	 * string, and it is intended only for display to the user, describing
	 * the processor subtype within the processor type given by pfs_name.
	 * Avoid using English words in the string, as it may be displayed
	 * untranslated.
	 */
	p = pfsprocp->pfs_model;
	*p++ = '"';
	strncpy(p, plp->cpu_vendor, NCPUVENDCH);
	p[NCPUVENDCH] = '\0';
	p += strlen(p);
	*p++ = '"';
	*p++ = ',';
	*p++ = ' ';
	*p++ = 'f';
	numtos(plp->cpu_family, p);
	p += strlen(p);
	*p++ = ',';
	*p++ = ' ';
	*p++ = 'm';
	numtos(plp->cpu_model, p);
	p += strlen(p);
	*p++ = ',';
	*p++ = ' ';
	*p++ = 's';
	numtos(plp->cpu_stepping, p);

	/* we "know" this is all going to fit */
	ASSERT(p + strlen(p) <= &pfsprocp->pfs_model[PFS_NAMELEN - 1]);
}
