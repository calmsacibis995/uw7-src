#ident	"@(#)kern-i386at:io/autoconf/hpci/hpcipci.c	1.1.1.2"
#ident	"$Header$"

#ifndef _KERNEL_HEADERS

#include <sys/types.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/psm.h>
#include <sys/ipl.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/page.h>
#include <sys/seg.h>
#include <sys/immu.h>
#include <sys/user.h>
#include <sys/systm.h>          /* for Hz */
#include <sys/file.h>
#include <sys/poll.h>
#include <sys/hpci.h>
#include <sys/proc.h>
#include <sys/kmem.h>
#include <sys/fcntl.h>
#include <sys/moddefs.h>
#include <sys/user.h>
#include <sys/cmn_err.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/ca.h>
#include <sys/pci.h>
#include <sys/ddi.h>
#include <sys/cred.h>
#include <sys/privilege.h>
#include <sys/f_ddi.h>

#else

#include "util/types.h"
#include "util/param.h"
#include "svc/psm.h"
#include "util/types.h"
#include "util/ipl.h"
#include "proc/signal.h"
#include "svc/errno.h"
#include "io/conf.h"
#include "mem/page.h"
#include "proc/seg.h"
#include "mem/immu.h"
#include "proc/user.h"
#include "svc/systm.h"		/* for Hz */
#include "fs/file.h"
#include "io/poll.h"
#include "io/autoconf/hpci/hpci.h"
#include "proc/proc.h"
#include "mem/kmem.h"
#include "fs/fcntl.h"
#include "util/mod/moddefs.h"
#include "proc/user.h"
#include "util/cmn_err.h"
#include "io/autoconf/resmgr/resmgr.h"
#include "io/autoconf/confmgr/confmgr.h"
#include "io/autoconf/confmgr/cm_i386at.h"
#include "io/autoconf/ca/ca.h"
#include "io/autoconf/ca/pci/pci.h"
#include "io/ddi.h"
#include "proc/cred.h"
#include "acc/priv/privilege.h"
#include "io/f_ddi.h"

#endif
/*
#define HPCI_DEBUG
*/
extern struct config_info *get_pci_device_cip(
		ms_cgnum_t cgnum, uchar_t bus, uchar_t dev,
                uchar_t fun, uchar_t header, int *rvalp,
                struct config_info *cips);


#define	PCI_REVISION_ID_OFFSET	0x8 /* Not defined in pci.h */
#define	PCI_BASE_ADDRESS_OFFSET	0x10 /* Not defined in pci.h */

#define	PCI_BASE_ADDRESS_COUNT	6

void memzero(char *ptr, int size)
{
	while(size-- > 0)
		*ptr++ = '\0';
}


void cm_add_entry(struct config_info *cip, rm_key_t key);
int 
registerPciFunctions(socket_info_t *socketInfo)
{
	int	funcnum, nfuncs, i;
	struct config_info *cip;
	static struct config_info cips[4];
	ms_cgnum_t cgnum = 0;
	function_info_t *funcPtr = socketInfo->function;
	int retval;

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "registerPciFunctions entered");
#endif

	nfuncs = 0;
	for(funcnum = 0; funcnum < socketInfo->function_cnt; funcnum++, funcPtr = funcPtr->next)
	{
		pci_function_t *pciFunc = &funcPtr->ufunc.pciFunc;
#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "calling get_pci_device_cip(%d, %d, %d, %d, %x)",
				cgnum, pciFunc->busid, pciFunc->devnum,
		                    pciFunc->fnum, pciFunc->header);
#endif
/*
		memset(cips, '\0', sizeof(cips));
*/
		memzero((char *) cips, sizeof(cips));
		cip = get_pci_device_cip(cgnum, pciFunc->busid, pciFunc->devnum,
                    pciFunc->fnum, pciFunc->header, &retval, cips);
		if (cip == NULL)
		{
#ifdef HPCI_DEBUG
			cmn_err(CE_NOTE, "get_pci_device_cip(%d, %d, %d, %d, %x) failed",
				cgnum, pciFunc->busid, pciFunc->devnum,
		                    pciFunc->fnum, pciFunc->header);
#endif
			continue;
		}
		if((funcPtr->rmkey = cm_findkey( cip )) == RM_NULL_KEY)
		{
			funcPtr->rmkey = cm_newkey(0, 1); /* persistent key */
			cm_add_entry(cip, funcPtr->rmkey);
			cm_end_trans(funcPtr->rmkey);

#ifdef HPCI_DEBUG
			cmn_err(CE_NOTE, "cm_add_entry() for func %d returns rmkey %d", 
				nfuncs,
				funcPtr->rmkey );
#endif

		} else {
#ifdef HPCI_DEBUG
		cmn_err(CE_NOTE, "cm_findkey() for func %d returns rmkey %d", 
				nfuncs,
				funcPtr->rmkey );
#endif
		}
		nfuncs++;
	}
	return nfuncs;
}

int 
unregisterPciFunctions(socket_info_t *socketInfo)
{
	int	funcnum;
	ms_cgnum_t cgnum = 0;
	function_info_t *funcPtr = socketInfo->function;
	struct rm_args rarg;

#ifdef HPCI_DEBUG
	cmn_err(CE_NOTE, "unregisterPciFunctions entered");
#endif

	for(funcnum = 0; funcnum < socketInfo->function_cnt; funcnum++, funcPtr = funcPtr->next)
	{
		cm_delkey(funcPtr->rmkey);
	}
	return 0;
}
