#ident	"@(#)ktool:i386at/ktool/idtools/mdep.c	1.27.4.2"
#ident	"$Header$"

/*
 * Machine-specific routines for ID/TP.
 */

#include "inst.h"
#include "defines.h"
#include "devconf.h"
#include "mdep.h"
#include <locale.h>
#include <pfmt.h>

/* Per-vector table */
typedef struct per_vector ivec_t;
struct per_vector {
	short		itype;		/* interrupt type for this vector */
	short		ipl;		/* interrupt priority level */
	ctlr_t		*ctlrs;		/* list of devices using this vector */
	int		bind_cpu;	/* bind to the cpu specified */
} ivec_info[256];

struct entry_def *edef_intr;

#define MAXINTR	256

/* check if vector, I/O addresses, and/or device memory addresses overlap */

int
mdep_check(sdp, drv)
	struct sdev	*sdp;
	driver_t	*drv;
{
        register ctlr_t *ctlr2;
	register ivec_t *ivec;
	int hwmod = (INSTRING(drv->mdev.mflags, HARDMOD) ||
		     drv->mdev.over < 2);

        /* check itype */
	if (sdp->itype < SITYP || sdp->itype > EITYP) {
		pfmt(stderr, MM_ERROR, RITYP, sdp->itype, SITYP, EITYP);
		error(1);
		return(0);
	}

	if (sdp->itype != 0 && sdp->vector != 0) {
		if (!hwmod) {
not_hw:
			pfmt(stderr, MM_ERROR, HWREQ, sdp->name);
			error(1);
			return(0);
		}

		/* check ipl value - must be 1 to 8, inclusive */
		if (sdp->ipl < SIPL || sdp->ipl > EIPL) {
			pfmt(stderr, MM_ERROR, RIPL, sdp->ipl, SIPL, EIPL);
			error(1);
			return(0);
		}

                /* check range of IVN */
                if (sdp->vector < SIVN || sdp->vector > EIVN) {
                        pfmt(stderr, MM_ERROR, RIVN, sdp->vector, SIVN, EIVN);
                        error(1);
                        return(0);
                }

		/* check for inconsistent use of vector */
		ivec = &ivec_info[sdp->vector];
		if (ivec->itype == 0) {
			ivec->itype = sdp->itype;
			ivec->ipl = sdp->ipl;
		} else if (ivec->itype != sdp->itype) {
			pfmt(stderr, MM_ERROR, VECDIFF, ivec->itype, ivec->ipl);
			pfmt(stderr, MM_ERROR, CVEC, sdp->name,
					ivec->ctlrs->sdev.name);
			error(1);
			return(0);
		} else {
			switch (sdp->itype) {
			case 1:	/* no sharing */
				pfmt(stderr, MM_ERROR, CVEC, sdp->name,
					ivec->ctlrs->sdev.name);
				error(1);
				return(0);
			case 2: /* sharing within a driver only */
				for (ctlr2 = ivec->ctlrs; ctlr2 != NULL;) {
					if (ctlr2->driver != drv) {
						pfmt(stderr, MM_ERROR, CVEC, sdp->name,
							ctlr2->sdev.name);
						error(1);
						return(0);
					}
					ctlr2 = ctlr2->vec_link;
				}
				break;
			}
		}
        }

        /* check I/O address */
	if (sdp->sioa > sdp->eioa) {
		/* out of order */
		pfmt(stderr, MM_ERROR, OIOA, sdp->sioa, sdp->eioa);
		error(1);
		return(0);
	}
        if (sdp->eioa != 0) {
		if (!hwmod)
			goto not_hw;
        }

        /* check controller memory address */
	if (sdp->scma > sdp->ecma) {
		/* out of order */
		pfmt(stderr, MM_ERROR, OCMA, sdp->scma, sdp->ecma);
		error(1);
		return(0);
	}
        if (sdp->ecma != 0) {
		if (!hwmod)
			goto not_hw;

                /* check range of device memory address */
                if (sdp->scma < SCMA) {
                        pfmt(stderr, MM_ERROR, RCMA, sdp->scma, SCMA);
                        error(1);
                        return(0);
                }
        }

        /* check DMA channel */
        if (sdp->dmachan < -1 || sdp->dmachan > DMASIZ) {
                pfmt(stderr, MM_ERROR, RDMA, sdp->dmachan, DMASIZ);
                error(1);
                return(0);
        } else if (sdp->dmachan != -1 && !hwmod) {
		goto not_hw;
	}

	for (ctlr2 = ctlr_info; ctlr2 != NULL; ctlr2 = ctlr2->next) {
                /* check I/O address conflicts */
                if (sdp->eioa != 0 && ctlr2->sdev.eioa != 0) {
                        if (OVERLAP(sdp->sioa, sdp->eioa,
				    ctlr2->sdev.sioa, ctlr2->sdev.eioa) &&
			    !(INSTRING(drv->mdev.mflags, IOOVLOK) &&
			      INSTRING(ctlr2->driver->mdev.mflags, IOOVLOK))) {
				pfmt(stderr, MM_ERROR, CIOA, ctlr2->sdev.name,
						sdp->name);
				error(1);
				return(0);
                        }
		}


/*
 *  Device memory addresses conflict if there is an overlap.
 *  Overlaps are allowed if both devices have the MEMSHR flag.
 */

                if (sdp->ecma != 0 &&
		    ctlr2->sdev.ecma != 0 &&
		    OVERLAP(sdp->scma, sdp->ecma, ctlr2->sdev.scma,
			ctlr2->sdev.ecma) &&
		    (!INSTRING(drv->mdev.mflags, MEMSHR) ||
		     !INSTRING(ctlr2->driver->mdev.mflags, MEMSHR)))
		{
			pfmt(stderr, MM_ERROR, CCMA, ctlr2->sdev.name,
					sdp->name);
			error(1);
			return(0);
		}

/*
 *  Check for DMA channel conflicts.  In order for a conflict to be
 *  legal, both drivers must have the DMASHR flag.
 *
 *  However, there was a flaw in a previous version of this test such that,
 *  for pre-version 2 System files, we must allow a conflict if only one of
 *  the drivers has the DMASHR flag.
 *
 *  Thus, two overlapping drivers do not conflict if both have the DMASHR
 *  flag, or one driver has DMASHR, and the other driver's oversion is < 2.
 *
 *  Let a = first driver has DMASHR
 *  Let b = second driver has DMASHR
 *
 *  Test passes if:
 *
 *	(a && b) || (a && b_over < 2) || (b && a_over < 2)
 */

                if (sdp->dmachan != -1 && sdp->dmachan == ctlr2->sdev.dmachan)
		{
			int a = INSTRING(drv->mdev.mflags, DMASHR);
			int b = INSTRING(ctlr2->driver->mdev.mflags, DMASHR);
			int a_over = sdp->over;
			int b_over = ctlr2->sdev.over;

			int legal =	((a && b) ||
					(a && b_over < 2) ||
					(b && a_over < 2));

			if (!legal)
			{
				pfmt(stderr, MM_ERROR, CDMA, ctlr2->sdev.name,
						sdp->name);
				error(1);
				return(0);
			}
		}
        }

        return(1);
}


/* Machine-dependent controller initialization. */

void
mdep_ctlr_init(ctlr)
	register ctlr_t *ctlr;
{
        register ctlr_t *ctlr2;

	ctlr->vec_link = NULL;
	if (ctlr->sdev.itype) {
		ivec_t *ivec;

		/*
		 * Indicate that the driver wants interrupts.
		 */
		ctlr->driver->intr_decl = 1;

		/*
		 * Add this controller to the list for the vector,
		 * but only if this driver isn't already in the list.
		 */
		ivec = &ivec_info[ctlr->sdev.vector];
		for (ctlr2 = ivec->ctlrs; ctlr2; ctlr2 = ctlr2->vec_link) {
			if (ctlr2->driver == ctlr->driver)
				break;
		}
		if (ctlr2 == NULL) {
			ctlr->vec_link = ivec->ctlrs;
			ivec->ctlrs = ctlr;
		}

		/*
		 * As a special case for backward compatibility with version 0
		 * Master files, create an implied "intr" entry-point (even if
		 * one wasn't given explicitly) for each device configured for
		 * an interrupt.  This is needed because the "intr" entry-point
		 * was not explicitly specified in the version 0 Master file,
		 * even though others were.
		 */
		if (ctlr->driver->mdev.over == 0 &&
		    !drv_has_entry(ctlr->driver, edef_intr)) {
			if (lookup_entry(edef_intr->suffix,
					&ctlr->driver->mdev.entries, 0) == -1) {
				pfmt(stderr, MM_ERROR, TABMEM, "entry-point list");
				fatal(0);
			}
		}
	}
}


/* Print out interrupt vector information. */

void
mdep_prvec(fp)
        register FILE *fp;
{
        register ctlr_t *ctlr;
	register driver_t *drv;
	char *pfx;
	int was_psm;
        int i, maxintr;

	fprintf(fp, "static int updevflag = 0;\n\n");

        fprintf(fp, "/* Modules which need interrupts */\n\n");

	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!drv->intr_decl || !drv->conf_static || drv->autoconf)
			continue;

		fprintf(fp, "extern void %sintr();\n", drv->mdev.prefix);
        }

        fprintf(fp, "\nstruct intr_list static_intr_list[] = {\n");

	for (drv = driver_info; drv != NULL; drv = drv->next) {
		if (!drv->intr_decl || !drv->conf_static || drv->autoconf)
			continue;

		pfx = "up";
		if (INSTRING(drv->mdev.mflags, BLOCK) ||
		    INSTRING(drv->mdev.mflags, CHAR) ||
		    INSTRING(drv->mdev.mflags, STREAM)) {
			pfx = drv->mdev.prefix;
		}
		fprintf(fp, "\t{ \"%s\", &%sdevflag, %sintr },\n",
			drv->mdev.extname, pfx, drv->mdev.prefix);
        }

	fprintf(fp, "\t{0}\n};\n\n");
}

void
mdep_drvpostproc()
{
        ctlr_t *ctlr, *ctlr2;
	int i, cpu, cpu2;

        for (i = 0; i < MAXINTR; i++) {
		cpu = -1;
		if (ivec_info[i].ctlrs != NULL) {
			for (ctlr2 = ivec_info[i].ctlrs; ctlr2 != NULL;
			     ctlr2 = ctlr2->vec_link) {
				/* ignore the entry if no cpu binding */
				if ((cpu2 = ctlr2->driver->bind_cpu) == -1)
					continue;
				if (cpu == -1) {
					cpu = cpu2;
					ctlr = ctlr2;
					continue;
				}
				if (cpu2 != cpu) {
					pfmt(stderr, MM_ERROR, CPUDIFF,
						ctlr2->driver->mdev.name,
						ctlr->driver->mdev.name); 
					error(0);
					return;
				}
			}
		}
		ivec_info[i].bind_cpu = cpu;
	}
}

void
mdep_prdrvconf(fp, drv, caps)
	FILE	*fp;
	register driver_t *drv;
	char	*caps;
{
	register ctlr_t *ctlr;
	int	dmachan, itype;
	int	same_dmachan, same_itype;

	if (drv->mdev.over >= 2)
		return;

	/* These used to be per-driver, but are now per-controller;
	   for compatibility, if all controllers have the same value,
	   define it here. */
	ctlr = drv->ctlrs;
	dmachan = ctlr->sdev.dmachan;
	itype = ctlr->sdev.itype;
	same_dmachan = same_itype = 1;
	while ((ctlr = ctlr->drv_link) != NULL) {
		if (ctlr->sdev.dmachan != dmachan)
			same_dmachan = 0;
		if (ctlr->sdev.itype != itype)
			same_itype = 0;
	}
	if (same_dmachan)
		fprintf(fp, "#define\t%s_CHAN\t%hd\n", caps, dmachan);
	if (same_itype)
		fprintf(fp, "#define\t%s_TYPE\t%hd\n", caps, itype);
}


void
mdep_prctlrconf(fp, ctlr, caps)
	FILE	*fp;
	register ctlr_t *ctlr;
	char	*caps;
{
	fprintf(fp, "#define\t%s_%hd_VECT\t%hd\n",
		caps, ctlr->num, ctlr->sdev.vector);
	fprintf(fp, "#define\t%s_%hd_SIOA\t%ld\n",
		caps, ctlr->num, ctlr->sdev.sioa);
	fprintf(fp, "#define\t%s_%hd_EIOA\t%ld\n",
		caps, ctlr->num, ctlr->sdev.eioa);
	fprintf(fp, "#define\t%s_%hd_SCMA\t%ld\n",
		caps, ctlr->num, ctlr->sdev.scma);
	fprintf(fp, "#define\t%s_%hd_ECMA\t%ld\n",
		caps, ctlr->num, ctlr->sdev.ecma);
	fprintf(fp, "#define\t%s_%hd_CHAN\t%ld\n",
		caps, ctlr->num, ctlr->sdev.dmachan);
	fprintf(fp, "#define\t%s_%hd_TYPE\t%ld\n",
		caps, ctlr->num, ctlr->sdev.itype);
	fprintf(fp, "#define\t%s_%hd_IPL\t%ld\n",
		caps, ctlr->num, ctlr->sdev.ipl);
}


/*ARGSUSED*/
void
mdep_devsw_decl(fp, drv)
	FILE	*fp;
	register driver_t *drv;
{
}


/*ARGSUSED*/
void
mdep_bdevsw(fp, drv)
	FILE	*fp;
	register driver_t *drv;
{
}


/*ARGSUSED*/
void
mdep_cdevsw(fp, drv)
	FILE	*fp;
	register driver_t *drv;
{
}


/*ARGSUSED*/
void
mdep_prconf(fp)
	FILE	*fp;
{
}
