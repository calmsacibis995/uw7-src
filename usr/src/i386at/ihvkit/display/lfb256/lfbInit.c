#ident	"@(#)ihvkit:display/lfb256/lfbInit.c	1.1"

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

#include <lfb.h>

/*	MISCELLANEOUS ROUTINES 		*/
/*		MANDATORY		*/

SIBool lfbInitLFB(int fd, SIScreenRec *screenP, void (*flush)())

{
    SIConfig *configP = screenP->cfgPtr;
    SIFlags *flagsP = screenP->flagsPtr;
    SIFunctions *functions = screenP->funcsPtr;

    if (strcmp(configP->resource, "display") != 0) {
	fprintf(stderr,
		"Init: configP->resource is bad (%s)\n",
		configP->resource);
	return(SI_FAIL);
    }

    if (lfbstrcasecmp(configP->class, "LFB") != 0) {
	fprintf(stderr, "Init: configP->class is bad (%s)\n", configP->class);
	return(SI_FAIL);
    }

    if ((configP->screen != -1) && (configP->screen != 0)) {
	fprintf(stderr, "Init: Bad screen number (%d)\n", configP->screen);
	return (SI_FAIL);
    }

    lfb.fd = fd;

    *functions = lfbDisplayInterface;

    flagsP->SIstatecnt = LFB_NUM_GSTATES;
    flagsP->SIavail_bitblt = (SIAvail)(SSBITBLT_AVAIL |
				      MSBITBLT_AVAIL |
				      SMBITBLT_AVAIL);

    flagsP->SIavail_point = (SIAvail)PLOTPOINT_AVAIL;

    flagsP->SItilewidth = 32;
    flagsP->SItileheight = 32;
    flagsP->SIstipplewidth = 32;
    flagsP->SIstippleheight = 32;

    lfbVendorFlush = flush;

    return(SI_SUCCEED);
}

SIBool lfbShutdownLFB()

{
    return(SI_SUCCEED);
}
