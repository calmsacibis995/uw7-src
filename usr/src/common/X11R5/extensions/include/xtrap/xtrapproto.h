#ident	"@(#)r5extensions:include/xtrap/xtrapproto.h	1.1"


#ifndef __XTRAPPROTO__
#define __XTRAPPROTO__
/* RCSID = $Header$" */

/*****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991 by Digital Equipment Corp., Maynard, MA

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*****************************************************************************/
/*
 *
 *  CONTRIBUTORS:
 *
 *      Dick Annicchiarico
 *      Robert Chesler
 *      Dan Coutu
 *      Gene Durso
 *      Marc Evans
 *      Alan Jamison
 *      Mark Henry
 *      Ken Miller
 *
 *  DESCRIPTION:
 *      This header file contains the function prototypes for extension
 *      routines sorted by module (globally defined routines *only*).
 */
#if (defined(__STDC__) || defined(vaxc))
#define FUNCTION_PROTOS
#endif
#ifdef FUNCTION_PROTOS
# define	P(s) s
#else
# define P(s) ()
#endif
#ifndef Bool
# define Bool int
#endif
/* xtrapdi.c */
void XETrapDestroyEnv P((pointer value , XID id ));
void XETrapCloseDown P((void ));
Bool XETrapRedirectDevices P((void ));
void DEC_XTRAPInit P((void ));
int XETrapCreateEnv P((ClientPtr client ));
int XETrapDispatch P((ClientPtr client ));
int sXETrapDispatch P((ClientPtr client ));
int XETrapReset P((xXTrapReq *request , ClientPtr client ));
int XETrapGetAvailable P((xXTrapGetReq *request , ClientPtr client ));
int XETrapGetCurrent P((xXTrapReq *request , ClientPtr client ));
int XETrapGetStatistics P((xXTrapReq *request , ClientPtr client ));
int XETrapConfig P((xXTrapConfigReq *request , ClientPtr client ));
int XETrapStartTrap P((xXTrapReq *request , ClientPtr client ));
int XETrapStopTrap P((xXTrapReq *request , ClientPtr client ));
int XETrapGetVersion P((xXTrapGetReq *request , ClientPtr client ));
int XETrapGetLastInpTime P((xXTrapReq *request , ClientPtr client ));
int XETrapRequestVector P((ClientPtr client ));
int XETrapKeyboard P((xEvent *x_event , DevicePtr keybd , int count ));
#ifndef VECTORED_EVENTS
int XETrapPointer P((xEvent *x_event , DevicePtr ptrdev , int count ));
#else
int XETrapEventVector P((ClientPtr client , xEvent *x_event ));
#endif
void XETrapStampAndMail P((xEvent *x_event ));
void sReplyXTrapDispatch P((ClientPtr client , int size , char *reply ));
int XETrapWriteXLib P((XETrapEnv *penv , BYTE *data , CARD32 nbytes ));

/* xtrapddmi.c */
void XETrapPlatformSetup P((void ));
int XETrapSimulateXEvent P((xXTrapInputReq *request , ClientPtr client ));

/* /usr/users1/miller/golly/work/xtrap/xtrapdiswap.c */
int sXETrapReset P((xXTrapReq *request , ClientPtr client ));
int sXETrapGetAvailable P((xXTrapGetReq *request , ClientPtr client ));
int sXETrapConfig P((xXTrapConfigReq *request , ClientPtr client ));
int sXETrapStartTrap P((xXTrapReq *request , ClientPtr client ));
int sXETrapStopTrap P((xXTrapReq *request , ClientPtr client ));
int sXETrapGetCurrent P((xXTrapReq *request , ClientPtr client ));
int sXETrapGetStatistics P((xXTrapReq *request , ClientPtr client ));
int sXETrapSimulateXEvent P((xXTrapInputReq *request , ClientPtr client ));
int sXETrapGetVersion P((xXTrapGetReq *request , ClientPtr client ));
int sXETrapGetLastInpTime P((xXTrapReq *request , ClientPtr client ));
void sReplyXETrapGetAvail P((ClientPtr client , int size , char *reply ));
void sReplyXETrapGetVers P((ClientPtr client , int size , char *reply ));
void sReplyXETrapGetLITim P((ClientPtr client , int size , char *reply ));
void sReplyXETrapGetCur P((ClientPtr client , int size , char *reply ));
void sReplyXETrapGetStats P((ClientPtr client , int size , char *reply ));
void sXETrapHeader P((XETrapHeader *hdr ));
void XETSwSimpleReq P((xReq *data ));
void XETSwResourceReq P((xResourceReq *data ));
void XETSwCreateWindow P((xCreateWindowReq *data ));
void XETSwChangeWindowAttributes P((xChangeWindowAttributesReq *data ));
void XETSwReparentWindow P((xReparentWindowReq *data ));
void XETSwConfigureWindow P((xConfigureWindowReq *data ));
void XETSwInternAtom P((xInternAtomReq *data ));
void XETSwChangeProperty P((xChangePropertyReq *data ));
void XETSwDeleteProperty P((xDeletePropertyReq *data ));
void XETSwGetProperty P((xGetPropertyReq *data ));
void XETSwSetSelectionOwner P((xSetSelectionOwnerReq *data ));
void XETSwConvertSelection P((xConvertSelectionReq *data ));
void XETSwSendEvent P((xSendEventReq *data ));
void XETSwGrabPointer P((xGrabPointerReq *data ));
void XETSwGrabButton P((xGrabButtonReq *data ));
void XETSwUngrabButton P((xUngrabButtonReq *data ));
void XETSwChangeActivePointerGrab P((xChangeActivePointerGrabReq *data ));
void XETSwGrabKeyboard P((xGrabKeyboardReq *data ));
void XETSwGrabKey P((xGrabKeyReq *data ));
void XETSwUngrabKey P((xUngrabKeyReq *data ));
void XETSwGetMotionEvents P((xGetMotionEventsReq *data ));
void XETSwTranslateCoords P((xTranslateCoordsReq *data ));
void XETSwWarpPointer P((xWarpPointerReq *data ));
void XETSwSetInputFocus P((xSetInputFocusReq *data ));
void XETSwOpenFont P((xOpenFontReq *data ));
void XETSwListFonts P((xListFontsReq *data ));
void XETSwListFontsWithInfo P((xListFontsWithInfoReq *data ));
void XETSwSetFontPath P((xSetFontPathReq *data ));
void XETSwCreatePixmap P((xCreatePixmapReq *data ));
void XETSwCreateGC P((xCreateGCReq *data ));
void XETSwChangeGC P((xChangeGCReq *data ));
void XETSwCopyGC P((xCopyGCReq *data ));
void XETSwSetDashes P((xSetDashesReq *data ));
void XETSwSetClipRectangles P((xSetClipRectanglesReq *data ));
void XETSwClearToBackground P((xClearAreaReq *data ));
void XETSwCopyArea P((xCopyAreaReq *data ));
void XETSwCopyPlane P((xCopyPlaneReq *data ));
void XETSwPoly P((xPolyPointReq *data ));
void XETSwFillPoly P((xFillPolyReq *data ));
void XETSwPutImage P((xPutImageReq *data ));
void XETSwGetImage P((xGetImageReq *data ));
void XETSwPolyText P((xPolyTextReq *data ));
void XETSwImageText P((xImageTextReq *data ));
void XETSwCreateColormap P((xCreateColormapReq *data ));
void XETSwCopyColormapAndFree P((xCopyColormapAndFreeReq *data ));
void XETSwAllocColor P((xAllocColorReq *data ));
void XETSwAllocNamedColor P((xAllocNamedColorReq *data ));
void XETSwAllocColorCells P((xAllocColorCellsReq *data ));
void XETSwAllocColorPlanes P((xAllocColorPlanesReq *data ));
void XETSwFreeColors P((xFreeColorsReq *data ));
void XETSwStoreColors P((xStoreColorsReq *data ));
void XETSwStoreNamedColor P((xStoreNamedColorReq *data ));
void XETSwQueryColors P((xQueryColorsReq *data ));
void XETSwLookupColor P((xLookupColorReq *data ));
void XETSwCreateCursor P((xCreateCursorReq *data ));
void XETSwCreateGlyphCursor P((xCreateGlyphCursorReq *data ));
void XETSwRecolorCursor P((xRecolorCursorReq *data ));
void XETSwQueryBestSize P((xQueryBestSizeReq *data ));
void XETSwQueryExtension P((xQueryExtensionReq *data ));
void XETSwChangeKeyboardMapping P((xChangeKeyboardMappingReq *data ));
void XETSwChangeKeyboardControl P((xChangeKeyboardControlReq *data ));
void XETSwChangePointerControl P((xChangePointerControlReq *data ));
void XETSwSetScreenSaver P((xSetScreenSaverReq *data ));
void XETSwChangeHosts P((xChangeHostsReq *data ));
void XETSwRotateProperties P((xRotatePropertiesReq *data ));
void XETSwNoOperation P((xReq *data ));
#ifdef vms
void SwapLongs P((long *list , unsigned long count ));
void SwapShorts P((short *list , unsigned long count ));
int SwapColorItem P((xColorItem *pItem ));
#endif /* vms */
void  NotImplemented P((void ));
int   ProcBadRequest P((ClientPtr client ));

#undef P

#endif /* __XTRAPPROTO__ */
