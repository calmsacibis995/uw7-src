#ifndef	NOIDENT
#ident	"@(#)oldattlib:Xprint.h	1.1"
#endif
/*
 Xprint.h (C header file)
	Acc: 575327072 Fri Mar 25 16:04:32 1988
	Mod: 575321562 Fri Mar 25 14:32:42 1988
	Sta: 575321562 Fri Mar 25 14:32:42 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	author:
		Ross Hilbert
		AT&T 12/09/87
************************************************************************/

#ifndef _XPRINT_H
#define _XPRINT_H

extern void fpDisplay ();
extern void fpScreen ();
extern void fpVisual ();
extern void fpDepth ();
extern void fpScreenFormat ();
extern void fpGC ();
extern void fpXGCValues ();
extern void fpXEvent ();
extern void fpXCharStruct ();
extern void fpXFontProp ();
extern void fpXFontStruct ();
extern void fpXWindowAttributes ();
extern void fpXWindowChanges ();
extern void fpXSetWindowAttributes ();

#define pDisplay(p,verbose)		fpDisplay(stdout,p,verbose)
#define pScreen(p,verbose)		fpScreen (stdout,p,verbose)
#define pVisual(p,verbose)		fpVisual(stdout,p,verbose)
#define pDepth(p,verbose)		fpDepth(stdout,p,verbose)
#define pScreenFormat(p,verbose)	fpScreenFormat(stdout,p,verbose)
#define pGC(mask,gc)			fpGC(stdout,mask,gc)
#define pXGCValues(mask,gc)		fpXGCValues(stdout,mask,gc)
#define pXEvent(event,verbose)		fpXEvent(stdout,event,verbose)
#define pXCharStruct(p,verbose)		fpXCharStruct(stdout,p,verbose)
#define pXFontProp(p,verbose)		fpXFontProp(stdout,p,verbose)
#define pXFontStruct(p,verbose)		fpXFontStruct(stdout,p,verbose)
#define pXWindowAttributes(p,verbose)	fpXWindowAttributes(stdout,p,verbose)
#define pXWindowChanges(mask,p)		fpXWindowChanges(stdout,mask,p)
#define pXSetWindowAttributes(mask,p)	fpXSetWindowAttributes(stdout,mask,p)

#endif
