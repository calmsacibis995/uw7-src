#ifndef _IO_ND_DLPI_INCLUDE_H
#define _IO_ND_DLPI_INCLUDE_H

#ident "@(#)include.h	26.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#ifdef DEBUG
#define DLPI_DEBUG
#endif 

#ifdef _KERNEL_HEADERS

#include <util/types.h>
#include <util/param.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <io/nd/sys/mdi.h>
#include <io/nd/sys/dlpimod.h>
#include <io/nd/sys/sr.h>
#include <io/nd/dlpi/prototype.h>
#include <io/nd/dlpi/ether.h>
#include <io/nd/dlpi/token.h>
#include <io/nd/dlpi/fddi.h>
#include <io/nd/dlpi/isdn.h>
#include <io/nd/sys/scodlpi.h>
#include <io/nd/sys/scoisdn.h>
#include <util/mod/moddefs.h>
#include <util/debug.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>
#include <io/ddi.h>

#elif defined(_KERNEL)

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/cmn_err.h>
#include <sys/mdi.h>
#include <sys/dlpimod.h>
#include <sys/sr.h>
#include "prototype.h"
#include "ether.h"
#include "token.h"
#include "fddi.h"
#include "isdn.h"
#include <sys/scodlpi.h>
#include <sys/scoisdn.h>
#include <sys/moddefs.h>
#include <sys/debug.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/ddi.h>

#endif

#ifdef DLPI_DEBUG
# define DLPI_PRINTF(arg)	dlpi_printf arg
extern unsigned int dlpi_debug;

/* open/close */
# define DLPI_PRINTF01(arg)	if (dlpi_debug & 0x00000001) dlpi_printf arg

/* mem/lock alloc/free */
# define DLPI_PRINTF02(arg)	if (dlpi_debug & 0x00000002) dlpi_printf arg

/* media operations */
# define DLPI_PRINTF04(arg)	if (dlpi_debug & 0x00000004) dlpi_printf arg

/* data flow */
# define DLPI_PRINTF08(arg)	if (dlpi_debug & 0x00000008) dlpi_printf arg

/* SAP demultiplexing */
# define DLPI_PRINTF10(arg)	if (dlpi_debug & 0x00000010) dlpi_printf arg

/* BINDs */
# define DLPI_PRINTF20(arg)	if (dlpi_debug & 0x00000020) dlpi_printf arg

/* primitives */
# define DLPI_PRINTF40(arg)	if (dlpi_debug & 0x00000040) dlpi_printf arg

/* general streams processing */
# define DLPI_PRINTF80(arg)	if (dlpi_debug & 0x00000080) dlpi_printf arg

/* ioctls */
# define DLPI_PRINTF100(arg)	if (dlpi_debug & 0x00000100) dlpi_printf arg

/* source routing - also compile with -DSR_DEBUG */
# define DLPI_PRINTF200(arg)	if (dlpi_debug & 0x00000200) dlpi_printf arg

/* multicasting */
# define DLPI_PRINTF400(arg)	if (dlpi_debug & 0x00000400) dlpi_printf arg

/* mdi_tx_* routines */
# define DLPI_PRINTF800(arg)	if (dlpi_debug & 0x00000800) dlpi_printf arg

/* ddi8 CFG_SUSPEND/CFG_RESUME in MDI drivers and when dlpi drops frames */
# define DLPI_PRINTF1000(arg)	if (dlpi_debug & 0x00001000) dlpi_printf arg

/* HWFAIL_IND/txmon information */
# define DLPI_PRINTF2000(arg) if (dlpi_debug & 0x00002000) dlpi_printf arg

#else
# define DLPI_PRINTF(arg)
# define DLPI_PRINTF01(arg)
# define DLPI_PRINTF02(arg)
# define DLPI_PRINTF04(arg)
# define DLPI_PRINTF08(arg)
# define DLPI_PRINTF10(arg)
# define DLPI_PRINTF20(arg)
# define DLPI_PRINTF40(arg)
# define DLPI_PRINTF80(arg)
# define DLPI_PRINTF100(arg)
# define DLPI_PRINTF200(arg)
# define DLPI_PRINTF400(arg)
# define DLPI_PRINTF800(arg)
# define DLPI_PRINTF1000(arg)
# define DLPI_PRINTF2000(arg)
#endif

#endif	/* _IO_ND_DLPI_INCLUDE_H */
