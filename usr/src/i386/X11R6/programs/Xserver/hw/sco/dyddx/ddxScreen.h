/*
 *	@(#) ddxScreen.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Mon Aug 26 13:59:37 PDT 1991	mikep@sco.com
 *	- Add support for the grafinfo grafData pointer.
 *	S001	Tue Sep 10 16:47:50 PDT 1991	mikep@sco.com
 *	- Add bits per pixel and scanline padding to prequest.
 *	S002	Sat Sep 21 23:51:42 PDT 1991	mikep@sco.com
 *	- Remove CursorOn and format info from ddxScreenInfo
 *
 */
#ifndef DDXSCREEN_H
#define	DDXSCREEN_H 1

#include "grafinfo.h"

#define	DDX_ANY_DO_VERSION	0
#define	DDX_DO_VERSION		1
typedef	unsigned	ddxDOVersionID;

#define	DDX_BOARD_UNSPEC	255
#define	DDX_MAX_BOARD_NAME_LEN	16

#define	DDX_CLASS_UNSPEC	255
#define	DDX_DEPTH_UNSPEC	0

#define	DDX_LOAD_STATIC		1
#define	DDX_LOAD_DYNAMIC	2
#define	DDX_LOAD_UNSPEC		255

#define	DDX_TYPE_UNSPEC		NULL
#define	DDX_GRAF_UNSPEC		NULL

#define DDX_TYPE_DEFAULT	"mw"				/* S000 */

								/* S000 */
#define DDX_GRAFINFO(pScreen) \
			ddxActiveScreens[pScreen->myNum]->pRequest->grafinfo


typedef struct _ddxScreenRequest {
	char	*mode;
	char	*type;
	unsigned char	 ddxLoad;
	unsigned char	 boardNum;
	unsigned char	 dfltClass;
	unsigned char	 dfltDepth;
	unsigned char	 dfltBpp;				/* S001 */
	unsigned char	 dfltPad;				/* S001 */
	unsigned char	 nextLeft;
	unsigned char	 nextRight;
	unsigned char	 nextAbove;
	unsigned char	 nextBelow;
	grafData	*grafinfo;				/* S000 */
} ddxScreenRequest;

typedef	Bool	(*ddxProbeFunc)(ddxDOVersionID version,ddxScreenRequest *pReq);

struct _ddxScreenInfo {
	ddxProbeFunc	screenProbe;
	Bool		(* screenInit)();

	char *screenName ;
	unsigned short screenWidth ;
	unsigned short screenHeight ;
	struct _Screen *protoScreenRec ;

	int numvisuals ;
	struct _Visual *visuals ;
	VisualID *VIDs ;
	int defaultVisualClass ;
	int defaultVisualDepth ;

	ddxScreenRequest *pRequest;
	pointer		  osPriv;	/* private field for use by OS */
};
typedef struct _ddxScreenInfo ddxScreenInfo;

extern	int		  ddxNRequestedScreens;
extern	ddxScreenRequest  ddxRequestedScreens[];

extern	int  		  ddxNumActiveScreens ;
extern	ddxScreenInfo	 *ddxActiveScreens[] ;
extern	int		  ddxCurrentScreen;

#endif /* DDXSCREEN_H */
