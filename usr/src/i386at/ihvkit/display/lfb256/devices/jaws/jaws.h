#ident	"@(#)ihvkit:display/lfb256/devices/jaws/jaws.h	1.1"

/*
 *	Copyright (c) 1991 1992 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*	Copyright (c) 1993  Intel Corporation	*/
/*		All Rights Reserved		*/

#ifndef _JAWS_H_
#define _JAWS_H_

#include <sys/types.h>
#include <sys/inline.h>

#include <sidep.h>
#include <lfb.h>
#include <jawsregs.h>

#define JAWS_NUM_CURSORS 4

#define PAUSE getpid()

extern SIint32 *jaws_regs, *jaws_curs, *jaws_cmap;

extern JawsRegs jaws_reg_vals[];
extern int jaws_num_reg_vals;

extern JawsRegs *jaws_regP;

extern SIVisual jaws_visuals[];
extern int jaws_num_visuals;

/* 
 * Detect ESMP vs standard SVR4.2.  SI86IOPL is a new sysi86 function 
 * in ESMP.  Prior to ESMP, the IOPL was set using a VPIX 
 * sub-function.  This will be used during the init routine to disable 
 * the TSS IO bitmap by setting the IOPL to 3.
 *
 */

#include <sys/sysi86.h>

#ifdef SI86IOPL
#define ESMP
#else
/* From <sys/v86.h> */
#define V86SC_IOPL      4               /* v86iopriv () system call     */
#endif

#ifdef ESMP
#define SET_IOPL(iopl) _abi_sysi86(SI86IOPL, (iopl))
#else
#define SET_IOPL(iopl) _abi_sysi86(SI86V86, V86SC_IOPL, (iopl) << 12)
#endif

/* This list is abbreviated	*/

/*	MISCELLANEOUS ROUTINES 		*/
/*		MANDATORY		*/

extern SIBool DM_InitFunction();
extern SIBool jawsShutdown();
extern SIBool jawsVTSave();
extern SIBool jawsVTRestore();
extern SIBool jawsVideoBlank();
extern SIBool jawsInitCache();
extern SIBool jawsFlushCache();
extern SIBool jawsSelectScreen();

/*	COLORMAP MANAGEMENT ROUTINES	*/
/*		MANDATORY		*/

extern SIBool jawsSetCmap();
extern SIBool jawsGetCmap();

/*	CURSOR CONTROL ROUTINES		*/
/*		MANDATORY		*/

extern SIBool jawsDownLoadCurs();
extern SIBool jawsTurnOnCurs();
extern SIBool jawsTurnOffCurs();
extern SIBool jawsMoveCurs();

#endif /* _JAWS_H_ */
