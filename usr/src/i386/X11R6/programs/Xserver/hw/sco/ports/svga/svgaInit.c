/*
 * @(#)svgaInit.c 11.1
 *
 * Copyright (C) 1991-1994 The Santa Cruz Operation, Inc.
 *
 * The information in this file is provided for the exclusive use of the
 * licensees of The Santa Cruz Operation, Inc.  Such users have the right 
 * to use, modify, and incorporate this code into other products for purposes 
 * authorized by the license agreement provided they include this notice 
 * and the associated copyright notice with any such product.  The 
 * information in this file is provided "AS IS" without warranty.
 * 
 */
/*
 * svgaInit.c
 *
 * Initialize the svga Graphics Display Driver
 */

#include <sys/types.h>

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"
#include "ddxScreen.h"
#include "scoext.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbGlyph.h"
#include "nfb/nfbProcs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"

#include "gen/genDefs.h"
#include "gen/genProcs.h"

#include "svgaDefs.h"
#include "svgaProcs.h"

extern scoScreenInfo svgaSysInfo;
extern VisualRec svgaVisual, svgaVisual16, svgaVisual24;
extern nfbGCOps svgaSolidPrivOps;

static int svgaGeneration = -1;
unsigned int svgaScreenPrivateIndex = -1;

typedef void (*SetColorPtr)(unsigned int cmap,
                            unsigned int index,
                            unsigned short r,
                            unsigned short g,
                            unsigned short b,
                            ScreenPtr pScreen);

/*
 * svgaSetup() - initializes information needed by the core server
 * prior to initializing the hardware.
 *
 */
Bool
svgaSetup(ddxDOVersionID version,ddxScreenRequest *pReq)
{
  return ddxAddPixmapFormat(pReq->dfltDepth, pReq->dfltBpp, pReq->dfltPad);
}

/*
 * svgaInitHW()
 *
 * Initialize hardware that only needs to be done ONCE.  This routine will
 * not be called on a screen switch.  It may just call svgaSetGraphics()
 */
Bool
svgaInitHW(pScreen)
     ScreenPtr pScreen;
{
  grafData *grafinfo = DDX_GRAFINFO(pScreen);
  svgaPrivatePtr svgaPriv = SVGA_PRIVATE_DATA(pScreen);

#ifdef DEBUG_PRINT
  ErrorF("svgaInitHW()\n");
#endif

  /*
   * Only do this if you need memory mapped 
   */

  if (!grafGetMemInfo(grafinfo, NULL, NULL, NULL, &svgaPriv->fbBase)) {
    ErrorF ("svga: Missing MEMORY in grafinfo file.\n");
    return (FALSE);
  }

  svgaSetGraphics(pScreen);

  return (TRUE);
}

/*
 * svgaInit() - template for machine dependent screen init
 *
 * This routine is the template for a machine dependent screen init.
 * Once you start doing multiple visuals or need a screen priv you 
 * should check out all the stuff in effInit.c.
 */
Bool
svgaInit(index, pScreen, argc, argv)
     int index;
     ScreenPtr pScreen;
     int argc;
     char **argv;
{
  grafData *grafinfo = DDX_GRAFINFO(pScreen);
  nfbScrnPrivPtr pNfb;
  svgaPrivatePtr svgaPriv;
  int mmx, mmy;

#ifdef DEBUG_PRINT
  ErrorF("svgaInit(%d)\n", index);
#endif

  if (svgaGeneration != serverGeneration)
    {
      svgaGeneration = serverGeneration;
      svgaScreenPrivateIndex = AllocateScreenPrivateIndex();
      if ( svgaScreenPrivateIndex < 0 )
        return FALSE;
    }

  svgaPriv = (svgaPrivatePtr)xalloc(sizeof(svgaPrivate));
  if ( svgaPriv == NULL )
    return FALSE;

  pScreen->devPrivates[svgaScreenPrivateIndex].ptr =
    (unsigned char *)svgaPriv;

  /* Get mode and monitor info */
  if ( !grafGetInt(grafinfo, "PIXWIDTH",  &svgaPriv->width)  ||
       !grafGetInt(grafinfo, "PIXHEIGHT", &svgaPriv->height) ||
       !grafGetInt(grafinfo, "DEPTH", &svgaPriv->depth) ||
       !grafGetInt(grafinfo, "PIXBYTES", &svgaPriv->fbStride) )
    {
      ErrorF("svga: can't find pixel info in grafinfo file.\n");
      return FALSE;
    }

  mmx = 300; mmy = 300;  /* Reasonable defaults */

  grafGetInt(grafinfo, "MON_WIDTH",  &mmx);
  grafGetInt(grafinfo, "MON_HEIGHT", &mmy);

  if (!nfbScreenInit(pScreen, svgaPriv->width, svgaPriv->height, mmx, mmy))
    return FALSE;


  switch (svgaPriv->depth)
    {
    case 8:
      if (!nfbAddVisual(pScreen, &svgaVisual))
        return FALSE;
      pNfb = NFB_SCREEN_PRIV(pScreen);
      pNfb->protoGCPriv->ops = &svgaSolidPrivOps;
      pNfb->SetColor = svgaSetColor;
      pNfb->LoadColormap = genLoadColormap;
      break;

    case 16:
      if (!nfbAddVisual(pScreen, &svgaVisual16))
        return FALSE;
      pNfb = NFB_SCREEN_PRIV(pScreen);
      pNfb->protoGCPriv->ops = &svgaSolidPrivOps;
      pNfb->SetColor = (SetColorPtr)NoopDDA;
      pNfb->LoadColormap = NoopDDA;
      break;

    default:
      return FALSE;
    }
  pNfb->BlankScreen = svgaBlankScreen;
  pNfb->ValidateWindowPriv = svgaValidateWindowPriv;

  if (!svgaInitHW(pScreen))
    return FALSE;

  /*
   * Call one of the following.  
   *
   * scoSWCursorInitialize(pScreen);
   * svgaCursorInitialize(pScreen);
   *
   * If you implement a hardware cursor it's always handy to 
   * have a grafinfo variable which will switch back to the
   * software cursor for debugging purposes.
   *
   */
  scoSWCursorInitialize(pScreen);

  /*
   * This should work for most cases.
   */
  if (((pScreen->rootDepth == 1) ? mfbCreateDefColormap(pScreen) :
       cfbCreateDefColormap(pScreen)) == 0 )
    return FALSE;

  /* 
   * Give the sco layer our screen switch functions.  
   * Always do this last.
   */
  scoSysInfoInit(pScreen, &svgaSysInfo);

  /*
   * Set any NFB runtime options here - see potential list
   *	in ../../nfb/nfbDefs.h
   */
  nfbSetOptions(pScreen, NFB_VERSION, NFB_POLYBRES, 0);

  return TRUE;

}

/*
 * svgaCloseScreen()
 *
 * Anything you allocate in svgaInit() above should be freed here.
 *
 * Do not call SetText() here or change the state of your adaptor!
 */
void
svgaCloseScreen(index, pScreen)
     int index;
     ScreenPtr pScreen;
{
  svgaPrivatePtr svgaPriv = SVGA_PRIVATE_DATA(pScreen);

  xfree(svgaPriv);
}

