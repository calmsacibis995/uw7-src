
/**
 * @(#) qvisWin.c 11.1 97/10/22
 *
 * Copyright (C) 1991 The Santa Cruz Operation, Inc.
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
 * qvisWin.c
 */

/**
 * Copyright 1991, COMPAQ COMPUTER CORPORATION.
 *
 * Developer   Date    Modification
 * ========  ========  =======================================================
 * mal       12/09/91  Originated
 * mjk       04/07/92  See RCS log
 *
 */

#include "xyz.h"
#include "X.h"
#include "Xproto.h"
#include "pixmap.h"
#include "gcstruct.h"
#include "colormap.h"
#include "windowstr.h"

#include "nfb/nfbGCStr.h"
#include "nfb/nfbDefs.h"
#include "nfb/nfbWinStr.h"
#include "nfb/nfbScrStr.h"
#include "nfb/nfbProcs.h"

#include "qvisMacros.h"
#include "qvisDefs.h"

/*
 * qvisValidateWindowPriv() - set win ops structure based on depth/visual
 * 
 * This routine sets the win ops pointer in the nfb window priv. If you are
 * supporting multiple depths/visuals you have to test here and set ops to
 * the correct version.
 */

extern nfbWinOpsPtr qvisWinOps;

void
qvisValidateWindowPriv(pWin)
    WindowPtr       pWin;
{
    qvisPrivateData *qvisPriv = QVIS_PRIVATE_DATA(pWin->drawable.pScreen);
    XYZ("qvisValidateWindowPriv-entered");
    (NFB_WINDOW_PRIV(pWin))->ops = qvisPriv->qvisWinOps;
}
