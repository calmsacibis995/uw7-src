#ident	"@(#)kern-i386at:io/hba/qlc1020/qlc1020.cf/Space.c	1.2.1.1"
/*
 *   QLogic ISP1020 SCO UnixWare version X.X (Gemini)
 *   Host Bus Adapter driver space.c file.
 */

/*************************************************************************
**                                                                      **
**                             NOTICE                                   **
**              COPYRIGHT (C) 1996-1997 QLOGIC CORPORATION              **
**                        ALL RIGHTS RESERVED                           **
**                                                                      **
** This computer program is CONFIDENTIAL and contains TRADE SECRETS of  **
** QLOGIC CORPORATION.  The receipt or possession of this program does  **
** not convey any rights to reproduce or disclose its contents, or to   **
** manufacture, use, or sell anything that it may describe, in whole or **
** in part, without the specific written consent of QLOGIC CORPORATION. **
** Any reproduction of this program without the express written consent **
** of QLOGIC CORPORATION is a violation of the copyright laws and may   **
** subject you to civil liability and criminal prosecution.             **
**                                                                      **
*************************************************************************/

#include <sys/types.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/conf.h>

int qlc1020_gtol[SDI_MAX_HBAS];    /* map global hba # to local # */

struct ver_no qlc1020_sdi_ver;

#ifndef QLC1020_CNTLS
#define QLC1020_CNTLS  1
#endif

#ifdef  QLC1020_CPUBIND
#define QLC1020_VER     HBA_UW21_IDATA | HBA_IDATA_EXT | HBA_EXT_ADDRESS
struct  hba_ext_info    qlc1020_extinfo = {
    0, QLC1020_CPUBIND
};
#else
#define QLC1020_VER     HBA_UW21_IDATA | HBA_EXT_ADDRESS
#endif  /* QLC1020_CPUBIND */

struct  hba_idata_v5    _qlc1020idata[]    = {
    { QLC1020_VER, "(qlc1020,1) QLogic QLA10XX",
      7, 0, -1, 0, -1, 0 }
#ifdef  QLC1020_CPUBIND
    ,
    { HBA_EXT_INFO, (char *)&qlc1020_extinfo }
#endif
};

int qlc1020_cntls  = QLC1020_CNTLS;
