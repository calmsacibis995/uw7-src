/*
 *	@(#) nfbScrStr.h 12.1 95/05/09 
 *
 *	Copyright (C) The Santa Cruz Operation, 1991-1993.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 * SCO MODIFICATION HISTORY
 *
 * S013, 28-Aug-93, mikep
 *	Data freed in S011 now must be reserved for Pixmap privates
 *	Furthermore the pixmap structure grew by one pointer.
 * S012, 18-Apr-93, buckm
 * 	Change fontID in screen priv to unused.
 *	Arg to ClearFont changed.
 *	text8_on_card is an array of nfbFontPS's now.
 * S011, 16-Apr-93, buckm
 * 	Replace old layers and cmap fields by re-usable fields.
 * S010, 20-Nov-92, staceyc
 * 	new unrealize terminal 8 bit font routine added
 * S009, 04-Nov-92, mikep
 *	add cfb structure and nfb version number
 * S008, 29-Oct-92, staceyc
 * 	added font private index
 * S007, 13-Oct-92, mikep
 *	- Add wrapper holders for SaveGState and RestoreGState.  Now
 *	  nfb can know about screen switching.
 * S006, 14-Sep-92, mikep
 *	- Add save spots for old bit order support
 * S005, 04-Sep-92, mikep
 *	- Add missing arguments to definition of ValidateVisual
 * S004, 01-Sep-92, staceyc
 * 	- added hardware attributes
 * S003, 26-Aug-92, staceyc
 * 	- added fast text8 structures
 * S002, 24-Aug-92, staceyc
 *	- add hardware clip count
 * S001, Sun Sep 29 19:35:05 PDT 1991, mikep@sco.com
 *	- Remove all cursor routines from nfb
 *	- Add Font caching support.
 * S000, Wed Jun 26 00:21:26 PDT 1991, buckm@sco.com
 *	- Add LoadColormap routine to nfbScrnPriv structure.
 */
/*
 * nfbScrStr.h
 *
 * Copyright 1989, Silicon Graphics, Inc.
 *
 * Author: Jeff Weinstein
 *
 * structure definitions for No Frame Buffer(nfb) porting layer.
 *
 */
#ifndef _NFB_SCR_STR_H
#define _NFB_SCR_STR_H

#include "gcstruct.h"
#include "fontstruct.h"
#include "scrnintstr.h"
#include "screenint.h"

#ifdef PIXPRIV							/* S013 vvv */
#define NFB_PIXMAP_PRIV_PAD  12
#else
#define NFB_PIXMAP_PRIV_PAD  13
#endif								/* S013 ^^^ */

typedef struct _nfbScrFontData {
	int text8_total;
	int text8_free;
	struct _nfbFontPS **text8_on_card;			/* S012 */
	int text8_width;
	int text8_height;
	void (*DownloadFont8)(unsigned char **, int, int, int, int, int,
	    ScreenRec *);
	void (*ClearFont8)(int, ScreenPtr);
} nfbScrFontData;

typedef struct _nfbCFBPriv {					/* S009 vvv */
	unsigned long nfb_cfb_options;
	GCOps * gcops;
	void (* GetImage)();
	void (* GetSpans)();
	void (* PaintWindowBackground)();
	void (* PaintWindowBorder)();
	void (*PreCFBAccess)(struct _Drawable *);
	void (*PostCFBAccess)(struct _Drawable *);
} nfbCFBPriv, *nfbCFBPrivPtr;					/* S009 ^^^ */

typedef struct _nfbScrnPriv {
	PixmapRec pixmap ; /* this is here for mfb and cfb.  We give them a
			    * fake pixmap so that we can cause them to dump
			    * core if they try to access the frame buffer.
			    */

	/* S011, S013
	 * This space is now reserved for pixmap privates
	 */
	long _pix_reserved_[NFB_PIXMAP_PRIV_PAD];

	/* colormap installed on this screen */
	struct _ColormapRec *installedCmap ;

	struct _nfbGCPriv *protoGCPriv ;

	/* colormap routines */

	void (* SetColor)(
		unsigned int,
		unsigned int,
		unsigned short,
		unsigned short,
		unsigned short,
		struct _Screen * );

	void (* LoadColormap)( struct _ColormapRec * );		/* S000 */

	long _unused_2[1];					/* S012 */

	/* Font cache support */
	void (* ClearFont)(struct _nfbFontPS *, struct _Screen *);  /* S012 */

	/* DDX hook hang properties */
	void (* InitRootWindow)(
		struct _Screen * );

	/* misc screen routines */
	Bool (* BlankScreen)(
		int,
		struct _Screen * );

	/* window routines */
	void (* ValidateWindowPriv)( struct _Window * );

/* Post Version 4.1 additions must be below this line */

	/* number of hardware supported clip regions */
	unsigned int clip_count;

	/* fast text8 data */
	nfbScrFontData *font;

	/* private index for all fonts for this screen */
	int font_private_index;

	/* This driver's version number */
	unsigned long nfb_version;				/* S009 */

	/* special nfb options.  See nfbDefs.h */
	unsigned long nfb_options;
	nfbCFBPrivPtr cfbPriv;					/* S009 */

	/* Save spots for the RealizeCursor and Validate window routines */
	Bool (* SaveRealizeCursor)(struct _Screen *, struct _Cursor *);
	Bool (* SaveDisplayCursor)(struct _Screen *, struct _Cursor *);
	void (* SaveValidateWindowPriv)( struct _Window * );
        void (*SaveGState)();					/* S007 */
	void (*RestoreGState)();				/* S007 */
/* Post Version 5.0 additions must be below this line */

} nfbScrnPriv, *nfbScrnPrivPtr;


#endif /* _NFB_SCR_STR_H */
