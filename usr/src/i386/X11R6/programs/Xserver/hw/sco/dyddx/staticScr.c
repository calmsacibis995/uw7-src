/*
 *	@(#) staticScr.c 11.1 97/10/22
 *
 *	Copyright (C) 1991-1993 The Santa Cruz Operation, Inc.
 *
 *	The information in this file is provided for the exclusive use of the
 *	licensees of The Santa Cruz Operation, Inc.  Such users have the right
 *	to use, modify, and incorporate this code into other products for
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 */
/*
 * staticScr.c
 *
 * this is the table of all screens supported by the server
 */
#include "X.h"
#include "misc.h"
#include "ddxScreen.h"
#include "commonDDX.h"
#include "staticScr.h"

#undef STATIC_XXX

#ifdef STATIC_MW
extern ddxScreenInfo mwScreenInfo;
#endif

#ifdef STATIC_DFB
extern ddxScreenInfo dfbScreenInfo;
#endif

#ifdef STATIC_IFB
extern ddxScreenInfo ifbScreenInfo;
#endif

#ifdef STATIC_EFF
extern ddxScreenInfo effScreenInfo;
#endif

#ifdef STATIC_XGA
extern ddxScreenInfo xgaScreenInfo;
#endif

#ifdef STATIC_TMS
extern ddxScreenInfo tms0ScreenInfo;
extern ddxScreenInfo tms3ScreenInfo;
#endif

#ifdef STATIC_JAWS
extern ddxScreenInfo jawsScreenInfo;
#endif

#ifdef STATIC_MVC
extern ddxScreenInfo mvcScreenInfo;
#endif

#ifdef STATIC_S3C
extern ddxScreenInfo s3cScreenInfo;
#endif

#ifdef STATIC_WD
extern ddxScreenInfo wdScreenInfo;
#endif

#ifdef STATIC_WD33
extern ddxScreenInfo wd33ScreenInfo;
#endif

#ifdef STATIC_QVIS
extern ddxScreenInfo qvisScreenInfo;
#endif

#ifdef STATIC_MPG
extern ddxScreenInfo mpgScreenInfo;
#endif

#ifdef STATIC_MFG
extern ddxScreenInfo mfgnfbScreenInfo;
extern ddxScreenInfo mgxnfbScreenInfo;
#endif

#ifdef STATIC_NON
extern ddxScreenInfo nonScreenInfo;
#endif

#ifdef STATIC_VGA4
extern ddxScreenInfo vga4ScreenInfo;
#endif
#ifdef STATIC_VGA8
extern ddxScreenInfo vga8ScreenInfo;
#endif

#ifdef STATIC_GD6
extern ddxScreenInfo gd6_8ScreenInfo;
extern ddxScreenInfo gd6_16ScreenInfo;
#endif

#ifdef STATIC_TTA
extern ddxScreenInfo ttaScreenInfo;
#endif

#ifdef STATIC_P9000
extern ddxScreenInfo p9000ScreenInfo;
#endif

#ifdef STATIC_P9100
extern ddxScreenInfo p9100ScreenInfo;
#endif

#ifdef STATIC_NTE
extern ddxScreenInfo nte8ScreenInfo;
extern ddxScreenInfo nte16ScreenInfo;
extern ddxScreenInfo nte24ScreenInfo;
extern ddxScreenInfo eox8ScreenInfo;
extern ddxScreenInfo eox16ScreenInfo;
extern ddxScreenInfo eox24ScreenInfo;
extern ddxScreenInfo nsf8ScreenInfo;
extern ddxScreenInfo nsf16ScreenInfo;
extern ddxScreenInfo nsf24ScreenInfo;
#endif

#ifdef STATIC_M32
extern ddxScreenInfo m32ScreenInfo;
#endif

#ifdef STATIC_M64
extern ddxScreenInfo m64ScreenInfo;
#endif

#ifdef STATIC_W32
extern ddxScreenInfo w32ScreenInfo;
#endif

#ifdef STATIC_AGX
extern ddxScreenInfo agxScreenInfo;
#endif

#ifdef STATIC_MGA
extern ddxScreenInfo mgaScreenInfo;
#endif

#ifdef STATIC_MIL
extern ddxScreenInfo milScreenInfo;
#endif

#ifdef STATIC_CXM
extern ddxScreenInfo cxmScreenInfo;
#endif

#ifdef STATIC_CT
extern ddxScreenInfo ct8ScreenInfo;
extern ddxScreenInfo ct16ScreenInfo;
#endif

#ifdef STATIC_I128
extern ddxScreenInfo i128ScreenInfo;
#endif

#ifdef STATIC_SVGA
extern ddxScreenInfo svgaScreenInfo;
#endif

#ifdef STATIC_GD5446
extern ddxScreenInfo gd5446_8bppScreenInfo;
extern ddxScreenInfo gd5446_16bppScreenInfo;
extern ddxScreenInfo gd5446_24bppScreenInfo;
#endif

#ifdef STATIC_CT65548
extern ddxScreenInfo ct65548_8ScreenInfo;
extern ddxScreenInfo ct65548_16ScreenInfo;
#endif

#ifdef STATIC_CT65550
extern ddxScreenInfo ct65550_8ScreenInfo;
extern ddxScreenInfo ct65550_16ScreenInfo;
#endif

#ifdef STATIC_GD5436
extern ddxScreenInfo gd5436_8bppScreenInfo;
extern ddxScreenInfo gd5436_16bppScreenInfo;
extern ddxScreenInfo gd5436_24bppScreenInfo;
#endif

/* XXXScreenExtern  - DO NOT DELETE THIS COMMENT! */

ddxScreenInfo *staticScreens[] = {
#ifdef STATIC_MW
	&mwScreenInfo,
#endif
#ifdef STATIC_DFB
	&dfbScreenInfo,
#endif
#ifdef STATIC_IFB
	&ifbScreenInfo,
#endif
#ifdef STATIC_EFF
	&effScreenInfo,
#endif
#ifdef STATIC_XGA
	&xgaScreenInfo,
#endif
#ifdef STATIC_TMS
	&tms0ScreenInfo,
	&tms3ScreenInfo,
#endif
#ifdef STATIC_MVC
	&mvcScreenInfo,
#endif
#ifdef STATIC_JAWS
	&jawsScreenInfo,
#endif
#ifdef STATIC_S3C
	&s3cScreenInfo,
#endif
#ifdef STATIC_WD
	&wdScreenInfo,
#endif
#ifdef STATIC_WD33
	&wd33ScreenInfo,
#endif
#ifdef STATIC_QVIS
	&qvisScreenInfo,
#endif
#ifdef STATIC_MPG
	&mpgScreenInfo,
#endif
#ifdef STATIC_MVC
	&mvcScreenInfo,
#endif
#ifdef STATIC_MFG
	&mfgnfbScreenInfo,
	&mgxnfbScreenInfo,
#endif
#ifdef STATIC_NON
	&nonScreenInfo;
#endif
#ifdef STATIC_VGA4
	&vga4ScreenInfo,
#endif
#ifdef STATIC_VGA8
	&vga8ScreenInfo,
#endif
#ifdef STATIC_GD6
	&gd6_8ScreenInfo,
	&gd6_16ScreenInfo,
#endif
#ifdef STATIC_TTA
	&ttaScreenInfo,
#endif
#ifdef STATIC_P9000
	&p9000ScreenInfo,
#endif
#ifdef STATIC_P9100
	&p9100ScreenInfo,
#endif
#ifdef STATIC_NTE
	&nte8ScreenInfo,
	&nte16ScreenInfo,
	&nte24ScreenInfo,
	&eox8ScreenInfo,
	&eox16ScreenInfo,
	&eox24ScreenInfo,
	&nsf8ScreenInfo,
	&nsf16ScreenInfo,
	&nsf24ScreenInfo,
#endif
#ifdef STATIC_M32
	&m32ScreenInfo,
#endif
#ifdef STATIC_W32
	&w32ScreenInfo,
#endif
#ifdef STATIC_AGX
	&agxScreenInfo,
#endif
#ifdef STATIC_MGA
	&mgaScreenInfo,
#endif
#ifdef STATIC_MIL
	&milScreenInfo,
#endif
#ifdef STATIC_CXM
	&cxmScreenInfo,
#endif
#ifdef STATIC_CT
	&ct8ScreenInfo,
	&ct16ScreenInfo,
#endif
#ifdef STATIC_I128
	&i128ScreenInfo,
#endif
#ifdef STATIC_I128
	&svgaScreenInfo,
#endif
#ifdef STATIC_GD5446
	&gd5446_8bppScreenInfo,
	&gd5446_16bppScreenInfo,
	&gd5446_24bppScreenInfo,
#endif
#ifdef STATIC_CT65548
	&ct65548_8ScreenInfo,
	&ct65548_16ScreenInfo,
#endif
#ifdef STATIC_CT65550
	&ct65550_8ScreenInfo,
	&ct65550_16ScreenInfo,
#endif
#ifdef STATIC_GD5436
        &gd5436_8bppScreenInfo,
        &gd5436_16bppScreenInfo,
        &gd5436_24bppScreenInfo,
#endif
#ifdef STATIC_M64
	&m64ScreenInfo,
#endif

/* XXXScreenInfo - DO NOT DELETE THIS COMMENT!  */
	NULL
} ;

int NumStaticScreens = ( sizeof staticScreens / sizeof staticScreens[0] ) - 1 ;

/***====================================================================***/

Bool
loadStaticDDX(ddxScreenRequest *pRequest)
{
int		 i;
Bool		 loaded= FALSE;
ddxScreenInfo	*pDDXInfo;

    if (((pRequest->ddxLoad!=DDX_LOAD_STATIC)&&
	 (pRequest->ddxLoad!=DDX_LOAD_UNSPEC))||
	((ddxDfltLoadType!=DDX_LOAD_STATIC)&&
	 (ddxDfltLoadType!=DDX_LOAD_UNSPEC))) {
	NOTICE1(DEBUG_INIT,"Ignoring static DDX for \"%s\"\n",pRequest->type);
	return(FALSE);
    }

    for (i=0;i<NumStaticScreens;i++) {
	if (strcmp(staticScreens[i]->screenName,pRequest->type)==0) {
	    if ((*staticScreens[i]->screenProbe)( DDX_DO_VERSION, pRequest )) {
		ErrorF("Using static DDX for \"%s\"\n", pRequest->type);
		pDDXInfo= (ddxScreenInfo *)xalloc(sizeof(ddxScreenInfo));
		if (pDDXInfo) {
		    *pDDXInfo= *staticScreens[i];
		    ddxActiveScreens[ddxNumActiveScreens++]= pDDXInfo;
		    pDDXInfo->pRequest=	pRequest;
		    loaded= TRUE;
		}
	    }
	    break;
	}
    }
    return(loaded);
}

Bool
loadDynamicDDX(ddxScreenRequest *pRequest)
{
    int		 i;
    Bool		 found = FALSE, loaded= FALSE;
    ddxScreenInfo	*pDDXInfo;

    ErrorF("Eeeekkk!!! Your static load failed!\nLet's see if we can figure out why.\n");

    ErrorF("Your driver name is %s\n", pRequest->type);

    if (((pRequest->ddxLoad!=DDX_LOAD_STATIC) &&
	 (pRequest->ddxLoad!=DDX_LOAD_UNSPEC))
	 ||
	((ddxDfltLoadType!=DDX_LOAD_STATIC)&&
	 (ddxDfltLoadType!=DDX_LOAD_UNSPEC)))
	 {
	 ErrorF("Your load type was specified incorrectly.\n");
	 ErrorF("Were you using the -load command line option?");
	 ErrorF("This binary is incapable of dynamic loading");
	 return;
	 }
	 
    ErrorF("Let's see if your driver is linked into the server....\n");
    for (i=0;i<NumStaticScreens;i++) {
	if (strcmp(staticScreens[i]->screenName,pRequest->type)==0) {
	    found = TRUE;
	    ErrorF("Hey, we found your driver!  Let's try your probe routine\n");
	    if ((*staticScreens[i]->screenProbe)( DDX_DO_VERSION, pRequest )) {
		ErrorF("Your probe worked!\n");
		pDDXInfo= (ddxScreenInfo *)xalloc(sizeof(ddxScreenInfo));
		if (pDDXInfo) {
		    *pDDXInfo= *staticScreens[i];
		    ddxActiveScreens[ddxNumActiveScreens++]= pDDXInfo;
		    pDDXInfo->pRequest=	pRequest;
		    loaded= TRUE;
		}
		else
		    ErrorF("xalloc failed! Something went wrong long before we got here.\n");
	    }
	    else
	       ErrorF("Bummer, it looks like your %sProbe routine is failing.\n", pRequest->type);
	    break;
	}
	else
	    ErrorF("%s != %s\n", pRequest->type, staticScreens[i]->screenName);
    }
    if (!found)
       ErrorF("Your driver isn't linked into the server.  Did you run xmkddx?\n");

    return(loaded);
}

