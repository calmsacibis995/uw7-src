#ident	"@(#)kern-pdi:io/hba/sony/cd31a.c	1.3"
#ident	"$Header$"

/*******************************************************************************
 *
 *	INCLUDES
 *
 ******************************************************************************/


#ifdef _KERNEL_HEADERS
#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <util/cmn_err.h>
#include <fs/buf.h>
#include <mem/immu.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/sdi.h>
#if (PDI_VERSION <= 1)
#include <io/target/dynstructs.h>
#else /* !(PDI_VERSION <= 1) */
#include <io/target/sdi/dynstructs.h>
#endif /* !(PDI_VERSION <= 1) */

#include <io/hba/sony/sony.h>
#include <io/hba/hba.h>
#include <util/mod/moddefs.h>
#include <io/dma.h>

#if PDI_VERSION >= PDI_SVR42MP
#include <util/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <io/ddi.h>
#include <io/ddi_i386at.h>
#else /* _KERNEL_HEADERS */
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/signal.h>
#include <sys/cmn_err.h>
#include <sys/buf.h>
#include <sys/immu.h>
#include <sys/conf.h>

#include <sys/cred.h>
#include <sys/uio.h>
#include <sys/kmem.h>
#include <sys/debug.h>
#include <sys/scsi.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>

#include <sys/dynstructs.h>

#include <sys/moddefs.h>
#include <sys/dma.h>

#include <sys/hba.h>
#if PDI_VERSION >= PDI_SVR42MP
#include <sys/ksynch.h>
#endif /* PDI_VERSION >= PDI_SVR42MP */

#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#ifdef DDICHECK
#include <io/ddicheck.h>
#endif
#endif /* _KERNEL_HEADERS */

int
cd31a_load()
{
	return (0);
}
int
cd31a_unload()
{
	return (0);
}

MOD_MISC_WRAPPER(cd31a, cd31a_load, cd31a_unload, "DUMMY SONY31a");
