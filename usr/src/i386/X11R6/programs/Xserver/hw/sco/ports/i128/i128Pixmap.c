/*
 * @(#) i128Pixmap.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	10/20/95	kylec@sco.com
 *	- create
 */


#include "X.h"
#include "Xproto.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "regionstr.h"

#include "ddxScreen.h"

#include "nfb/nfbDefs.h"
#include "nfb/nfbGCStr.h"
#include "nfb/nfbWinStr.h"

#include "Xmd.h"
#include "servermd.h"
#include "gc.h"

#include "i128Procs.h"
#include "i128GCFuncs.h"


PixmapPtr
i128CreatePixmap (
                  ScreenPtr pScreen,
                  unsigned int width,
                  unsigned int height,
                  unsigned int depth)
{
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
    PixmapPtr pPixmap;
    int size;

#if 1
    if (width == 600 && height == 600)
    {
        size = PixmapBytePad(width, depth);
        pPixmap = (PixmapPtr)xalloc(sizeof(PixmapRec) + (height * size));
        if (!pPixmap)
            return NullPixmap;
        pPixmap->drawable.type = DRAWABLE_PIXMAP;
        pPixmap->drawable.class = 0;
        pPixmap->drawable.pScreen = pScreen;
        pPixmap->drawable.depth = depth;
        pPixmap->drawable.bitsPerPixel =
            depth == 1 ? 1 : ( depth <= 8 ? 8 : ( depth <= 16 ? 16 : 32 ) ) ;
        pPixmap->drawable.id = 0;
        pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
        pPixmap->drawable.x = 0;
        pPixmap->drawable.y = 0;
        pPixmap->drawable.width = width;
        pPixmap->drawable.height = height;
        pPixmap->devKind = size;
        pPixmap->refcnt = 1;
        pPixmap->devPrivate.ptr = (pointer)(pPixmap + 1);
        return pPixmap;
    }
#endif

    pScreen->CreatePixmap = i128Priv->CreatePixmap;
    pPixmap = (*pScreen->CreatePixmap)(pScreen, width,
                                       height, depth);
    pScreen->CreatePixmap = i128CreatePixmap;

ErrorF("CreatePixmap(%d) %dx%d-%d\n", pPixmap, width, height, depth);

    return pPixmap;
}


Bool
i128DestroyPixmap (PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    i128PrivatePtr i128Priv = I128_PRIVATE_DATA(pScreen);
    Bool ret;

ErrorF("DestroyPixmap(%d)\n", pPixmap);

    if (pPixmap->drawable.type == DRAWABLE_WINDOW)
        return TRUE;

    pScreen->DestroyPixmap = i128Priv->DestroyPixmap;
    ret = (*pScreen->DestroyPixmap)(pPixmap);
    pScreen->DestroyPixmap = i128DestroyPixmap;
    return ret;
}
