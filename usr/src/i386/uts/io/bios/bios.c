#ident	"@(#)kern-i386:io/bios/bios.c	1.1"
#ident	"$Header$"

#include <acc/priv/privilege.h>
#include <acc/priv/priv_hier.h>
#include <acc/priv/lpm/lpm.h>
#include <util/types.h>
#include <svc/v86bios.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/mod/moddefs.h>
#include <util/ipl.h>
#include <svc/systm.h>

/*
 * BIOS driver using the v86bios fuctionality. The spec is not public, and
 * currently used only by X-Window.
 *
 */

int biosdevflag = 0;

#define DRVNAME "BIOS Driver"
STATIC int bios_load(), bios_unload();
MOD_DRV_WRAPPER(bios, bios_load, bios_unload, NULL, DRVNAME);

STATIC int
bios_load(void)
{
	return(0);
}

STATIC int
bios_unload(void)
{
	return(0);
}

int
biosopen(dev_t *dev, int flag, int otyp, cred_t *crp)
{

	if (pm_denied (CRED (), P_SYSOPS)) {
		return EACCES;
	}
	else {
		return (0);
	}
}

int
biosclose(dev_t *dev, int flag, int otyp, cred_t *crp)
{

	if (pm_denied (CRED (), P_SYSOPS)) {
		return EACCES;
	} else {
		return(0);
	}
}


/*
 *
 * Remarks:
 *	No locks are needed because v86bios is serialized any way.
 *	If the routine needs data besides values in registers, use of mmap()
 *	has race conditions.
 *	In that case, V86BIOS_USEBUF must be used. At this moment, this
 *	functionality is only available to the kernel.
 */
int
biosioctl(dev_t *dev, int cmd, caddr_t arg, int mode, cred_t *crp,
	  int *rvalp)
{
	int status = 0;
	intregs_t regs;

	switch(cmd) {
	case V86BIOS_INT16:          /* Execute BIOS in 16-bit bios mode */
	case V86BIOS_INT32:          /* Execute BIOS in 32-bit bios mode */
	case V86BIOS_CALL:           /* Call ROM in bios mode */
		if(copyin(arg, &regs, sizeof(intregs_t))) {
			status = EFAULT;
			break;
		}

		v86bios(&regs);

		if(copyout(&regs, arg, sizeof(intregs_t))) {
			status = EFAULT;
			break;
		}

		break;                 
	default:
		status = EINVAL;
		break;

	} /* End switch cmd */

	return(status);

}

