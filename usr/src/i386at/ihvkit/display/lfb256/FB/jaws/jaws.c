#ident	"@(#)ihvkit:display/lfb256/FB/jaws/jaws.c	1.1"

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

#ifndef _KERNEL
#define _KERNEL
#endif

#include <sys/types.h>
#include <stddef.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/vmparam.h>
#include <sys/immu.h>
#include <sys/kmem.h>
#include <sys/ioctl.h>
#include <sys/cmn_err.h>
#include <sys/ddi.h>
#include <sys/moddefs.h>
#include <sys/cred.h>

/* Detect ESMP vs standard SVR4.2.  SI86IOPL is a new sysi86 function
 * in ESMP.  This is not a clean way to detect ESMP, but the 
 * alternative was to have the makefile run uname and detect ESMP.
 *
 * This will be used during the start routine to grant permission to 
 * the calling process to access the ATI ports.
 *
 */

#include <sys/sysi86.h>

#ifdef SI86IOPL
#define ESMP
#endif

#ifdef ESMP
#include <sys/iobitmap.h>
#endif

#include "jawsregs.h"

paddr_t jaws_fb_location = 0;

int jawsdevflag = 0;

char jaws_id_string[] = "Jaws v1.0";
char jaws_copyright[] = "Copyright (c) 1993 Intel Corp., All Rights Reserved";

/* 
 * Jaws ports that the user may need access to.  For completeness, the
 * ID ports are included.
 *
 */

unsigned short jaws_ports[] = {
    JAWS_ID_PORT1,
    JAWS_ID_PORT2,
    JAWS_ID_PORT3,
    JAWS_ID_PORT4,
    JAWS_CNTL_PORT,
    0x0,
};

/* 
 * Make the JAWS driver demand loadable.  This allows the X support to
 * be an add-on package without requiring a kernel rebuild.
 *
 */

static int jaws_load();
void jawsstart();

MOD_DRV_WRAPPER(jaws, jaws_load, NULL, NULL, "JAWS driver");

static int jaws_load()

{
    jawsstart();
    if (!jaws_fb_location)
	return(ENODEV);

    return(0);
}

void jawsstart()

{
    int i;

    cmn_err (CE_CONT, "%s %s\n", jaws_id_string, jaws_copyright);

    if ((inb(JAWS_ID_PORT1) != JAWS_ID_VAL1) &&
	(inb(JAWS_ID_PORT2) != JAWS_ID_VAL2) &&
	(inb(JAWS_ID_PORT3) != JAWS_ID_VAL3) &&
	(inb(JAWS_ID_PORT4) != JAWS_ID_VAL4)) {

	cmn_err(CE_WARN, "Jaws board not present.");
	return;
    }

    i = inb(JAWS_CNTL_PORT);

    switch(i & JAWS_CNTL_MMAP_SELECT_MASK) {
      case JAWS_CNTL_MMAP_SELECT_512M:
	jaws_fb_location = 0x20000000;
	break;
      case JAWS_CNTL_MMAP_SELECT_640M:
	jaws_fb_location = 0x28000000;
	break;
      case JAWS_CNTL_MMAP_SELECT_768M:
	jaws_fb_location = 0x30000000;
	break;
      case JAWS_CNTL_MMAP_SELECT_NONE:
	cmn_err(CE_WARN, "Jaws FB disabled.  Enable via ECU.");
	break;
    }

    if (jaws_fb_location)
	cmn_err(CE_CONT, "Jaws FB found at %dM\n", jaws_fb_location >> 20);

    return;
}

/* ARGSUSED */
int jawsopen(devp, flag, type, cr)
dev_t *devp;
int flag;
int type;
struct cred *cr;

{
    /* 
     * The only thing that would prevent us from using the Jaws card 
     * is if the card were not detected.  If the FB address is 0, the 
     * card was not detected, or was disabled by the BIOS (FB location 
     * was set to NONE).
     *
     */

    if (! jaws_fb_location)
	return(ENXIO);

    /* Enable the IO ports for the user */
#ifdef ESMP
    iobitmapctl(IOB_ENABLE, jaws_ports);
#else
    enableio(jaws_ports);
#endif

    return(0);
}

/* ARGSUSED */
int jawsclose(dev, flag, cr)
dev_t dev;
int flag;
struct cred *cr;

{
    return(0);
}

/* 
 * IOCTL is provided as an expansion option
 *
 */

/* ARGSUSED */
int jawsioctl(dev, cmd, arg, flag, cr, rvalp)
dev_t dev;
int cmd;
int arg;
int flag;
struct cred *cr;
int *rvalp;

{
    switch(cmd) {
      default:
	break;
    }
    
    return(0);
}

/* 
 * The kernel entry point for mmap() will provide a 4M page on three
 * conditions:
 *
 *     - The kernel is 4M page aware.  The code for this in Intel 
 *       Proprietary, so this is not a given.
 *     - The request is 4M aligned
 *     - The request is for a multiple of 4M
 *
 * This means that the if the user wants a 4M page, and all the
 * benefits associated with it, they will need to ask for 4M, and this 
 * routine will need to return a 4M aligned address for offset 0.  
 * This routine is still called for every 4K page, though.
 *
 * The upshot is that even though there isn't always 4M on FB, allow 
 * the user to map in the 4M range.  The init code will attempt to 
 * force the address to a 4M alligned address.  If the user walks off 
 * the end of the world, the sea monsters will get them.
 *
 */

/*ARGSUSED*/
jawsmmap(dev, off, prot)
dev_t dev;
register off_t off;

{
    if ((off < 0) ||
	(off >= 0x400000))
	return(NOPAGE);

    return(hat_getppfnum(jaws_fb_location + off, PSPACE_MAINSTORE));
}
