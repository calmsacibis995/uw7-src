#ident  "@(#)i2o_osmhd.h	1.2"
#ident	"$Header$"

#ifndef I2O_OSMHDRS_H
#define I2O_OSMHDRS_H

#ifdef  _KERNEL_HEADERS

#include <svc/errno.h>
#include <util/types.h>
#include <util/param.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <io/dma.h>
#include <util/mod/moddefs.h>
#include <util/ksynch.h>
#include <io/stream.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/autoconf/ca/ca.h>

/* These must come last: */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

#ifdef DDICHECK
#include <io/ddicheck.h>
#endif


#elif defined(_KERNEL)

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
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
#include <sys/dynstructs.h>
#include <sys/proc.h>
#include <sys/moddefs.h>
#include <sys/dma.h>
#include <sys/stream.h>
#include <sys/moddrv.h>
#include <sys/ddi.h>
#include <sys/ddi_i386at.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/ksynch.h>
#include <sys/ddicheck.h>

#endif /* _KERNEL_HEADERS */

#endif /* I2O_OSMHDRS_H */
