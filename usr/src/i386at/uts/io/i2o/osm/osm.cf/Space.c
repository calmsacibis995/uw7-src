#ident  "@(#)Space.c	1.3"
#ident	"$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated as Confidential.
 *
 */

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>
#include "config.h"

#define I2O_OSM_CNTLR_ID        7

#ifdef  I2O_OSM_CPUBIND

#define I2O_OSM_VER             HBA_UW21_IDATA | HBA_IDATA_EXT

#else

#define I2O_OSM_VER             HBA_UW21_IDATA  /* UnixWare 2.1 driver */

#endif  /* I2O_OSM_CPUBIND */

struct hba_idata_v5  i2oOSM_old_idata[] = {
  { I2O_OSM_VER, "(i2oOSM,1) I2O SCSI OSM",
    I2O_OSM_CNTLR_ID, 0, -1, 0, -1, 0, 0, 0, MAX_BUS, MAX_EXTCS, MAX_EXLUS }
};

/*
 * This value is the max number of IO's that the OSM will allow to be
 * outstanding to a TID at one time.
 */
unsigned long i2oOSMMaxAllowedIo = 64;

/*
 * i2oOSMBlockStorageSupport: If this is set to zero then the OSM will not
 *                            use the block storage interface.  Otherwise
 *                            it will use block storage and SCSI interfaces.
 */
unsigned char i2oOSMBlockStorageSupport = 1;

/* i2oOSMInitWaitTime: maximum time (in seconds) the OSM will wait for all the
 *                     I2O devices to finish initializing before calling
 *                     sdi_register.  This value is system dependant and may
 *                     need to be increased if there are a large number of
 *                     devices.
 */
unsigned long i2oOSMInitWaitTime = 120;

/*
 *  Determines if we go into the debugger at load time or not.
 *      1 = enter the debugger
 *      0 = bypass
 */

unsigned char I2O_OSM_DEBUG_ENTER = 0;
