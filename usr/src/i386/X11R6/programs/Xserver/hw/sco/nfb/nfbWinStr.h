/*
 *	@(#) nfbWinStr.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1992-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * SCO MODIFICATION HISTORY
 *
 * S006, 13-Nov-93, buckm	
 *	Put back previous args changed in S005 conditional on
 *	TBIRD_TEXT_INTERFACE so that we can get cleaner compiles.
 * S005, 19-Apr-93, buckm	
 *	Args to DrawMonoGlyphs and DrawFontText changed.
 *	Add RevOps structure.
 * S004, 16-Apr-93, buckm	
 *	Eliminate unneeded stuff from window private.
 * S003, 30-Oct-92, mikep	
 *	Remove WinClipOps.  Put DrawFontText into WinOps.
 * S002, 30-Oct-92, mikep	
 *	Change SetClipRegions to match other window ops
 *	Move ImageText8 into the WinClipOps structure
 * S001, 14-Sep-92, mikep@sco.com
 *	add a spot to save the old winOps pointer
 * S000, 26-Aug-92, staceyc
 * 	chew up two reserved window ops, one for setting clip regions
 *	and one for image text8
 *
 */

/*
 * Copyright 1989, Silicon Graphics, Inc.
 *
 * Author: Jeff Weinstein
 *
 * structure definitions for No Frame Buffer(nfb) porting layer.
 *
 */

#ifndef _NFB_WIN_STR_H
#define _NFB_WIN_STR_H

#include "gcstruct.h"
#include "nfbGlyph.h"

/*
 * This struct contains primitives for dealing with rectangles.  These
 * are non-replicated simple rectangle primitives that are used to paint
 * window backgrounds and borders, and will be used by generic nfb routines
 * to draw the higher level primitives.
 * This struct will hang off the window(drawable??) rec, and will probably
 * be switched based on depth or visual.
 *
 * NOTE: these fields are ordered so that we can use short constant
 * instruction formats to access.  Thanks Paul.
 *
 * The above comment is a MIPS 3000ism inherited from SGI.
 *
 */
typedef struct _nfbWinOps {
	void (* CopyRect) ( struct _Box *, struct _DDXPoint *, unsigned char,
					unsigned long, struct _Drawable * ) ;
	void (* DrawSolidRects) ( struct _Box *, unsigned int, unsigned long,
					unsigned char, unsigned long,
					struct _Drawable * ) ;
	void (* DrawImage) ( struct _Box *, void *, unsigned int,
					unsigned char, unsigned long,
					struct _Drawable * ) ;
	void (* DrawMonoImage) ( struct _Box *, void *, unsigned int,
					unsigned int, unsigned long,
					unsigned char, unsigned long,
					struct _Drawable * ) ;
	void (* DrawOpaqueMonoImage) ( struct _Box *, void *, unsigned int,
					unsigned int, unsigned long,
					unsigned long, unsigned char,
					unsigned long, struct _Drawable * ) ;
	void (* ReadImage) ( struct _Box *, void *, unsigned int,
					struct _Drawable * ) ;
	void (* DrawPoints) ( struct _DDXPoint *, unsigned int, unsigned long,
					unsigned char, unsigned long,
					struct _Drawable * ) ;
	void (* TileRects) ( struct _Box *, unsigned int, void *, unsigned int,
					unsigned int, unsigned int,
					struct _DDXPoint *, unsigned char,
					unsigned long, struct _Drawable * ) ;
#if defined(TBIRD_TEXT_INTERFACE)
	void (* DrawMonoGlyphs) ( struct _nfbGlyphInfo *, unsigned int,
					unsigned long, unsigned char,
					unsigned long, unsigned long,
					struct _Drawable * ) ;
	void (* DrawFontText) ( struct _Box *, unsigned char *chars, 
					unsigned int, unsigned short, int,
					unsigned long, unsigned long, 
					unsigned char, unsigned long, 
					unsigned char,
					struct _Drawable * ) ;
#else
	void (* DrawMonoGlyphs) ( struct _nfbGlyphInfo *, unsigned int,
					unsigned long, unsigned char,
					unsigned long, struct _nfbFontPS *,
					struct _Drawable * ) ;
	void (* DrawFontText) ( struct _Box *, unsigned char *chars, 
					unsigned int, struct _nfbFontPS *,
					unsigned long, unsigned long, 
					unsigned char, unsigned long, 
					unsigned char,
					struct _Drawable * ) ; /* S003 */
#endif
	void (* WinOp11) () ;
	void (* WinOp12) () ;
	void (* WinOp13) () ;
	void (* SetClipRegions) ( struct _Box *, int, struct _Drawable * ) ;
	void (* ValidateWindowGC) ( struct _GC *, unsigned long, 
					struct _Drawable * ) ;
	} nfbWinOps, *nfbWinOpsPtr ;

/*
 * Revised Ops structure.
 * Only one member now;
 * but let's have a struct in case there are more later.
 */
typedef struct _nfbWinRevOps {
	void (* DrawFontTextR) ( struct _Box *, unsigned char *chars, 
					unsigned int, struct _nfbFontPS *,
					unsigned long, unsigned long, 
					unsigned char, unsigned long, 
					unsigned char,
					struct _Drawable * ) ;
	} nfbWinRevOps, *nfbWinRevOpsPtr ;

typedef struct _nfbWindowPriv{
	nfbWinOpsPtr	ops;	
	nfbWinOpsPtr	saveops;	/* Saved copy of swapped ops */
	nfbWinRevOps	revops;	
	} nfbWindowPriv, *nfbWindowPrivPtr ;

#endif /* _NFB_WIN_STR_H */
