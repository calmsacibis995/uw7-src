/*
 *	@(#)s3cHWCurs.c	6.1	3/20/96	10:23:16
 *
 *      Copyright (C) Xware, 1991-1992.
 *
 *      The information in this file is provided for the exclusive use
 *      of the licensees of Xware. Such users have the right to use, 
 *      modify, and incorporate this code into other products for 
 *      purposes authorized by the license agreement provided they 
 *      include this notice and the associated copyright notice with 
 *      any such product.
 *
 *	Copyright (C) The Santa Cruz Operation, 1993
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use
 *	of the licensees of SCO. Such users have the right to use, 
 *	modify, and incorporate this code into other products for 
 *	purposes authorized by the license agreement provided they include
 *	this notice and the associated copyright notice with any such
 *	product.
 *
 * Modification History
 *
 * S021, 30-Jun-93, staceyc
 * 	fix some problems with cursor color and multiheaded support
 * S020, 02-Jun-93, staceyc
 * 	major reworking of cursor color code, it mostly works now (i.e. it
 *	suffers from the problems you would expect when attempting to share
 *	dac entries with clients)
 * S019, 17-May-93, staceyc
 * 	multiheaded S3 card support
 * S018, 11-May-93, staceyc
 * 	include file cleanup
 * S017	Thu Apr 08 16:02:08 PDT 1993	hiramc@sco.COM
 *	Have the 512 K memory thing working OK now and
 *	source_y used properly.  For some unknown reason,
 *	on the 512 K memory and 4 plane configuration,
 *	the starting Y coordinate of the offscreen cursor
 *	image is only half of the specified coordinate.
 *	There used to be an if statement here that took
 *	care of that case, now the if statement is in s3cInit.c
 *	and code has been removed here by the use of source_y.
 * S016	Tue Apr 06 16:39:39 PDT 1993	hiramc@sco.COM
 *	Used source_y incorrectly.  May still on 512 K memory,
 *	need to check.
 * S015	Mon Apr 05 13:24:38 PDT 1993	hiramc@sco.COM
 *	From Kevin's updates for 86C80[15], 86C928, support
 *	2MB memory, and change queue usage
 * S014	Fri Nov 06 08:46:18 PST 1992	hiramc@sco.COM
 *	add gcstruct.h for new items in nfbScrStr.h
 * S013	Thu Oct 29 16:01:21 PST 1992	hiramc@sco.COM
 *	Fix error in stride adjustment when realizing the HW cursor.
 *	Don't use null cursor pointers.  Max cursor is 32x64 in
 *	16 bit modes.
 * S012	Thu Sep 24 08:45:29 PDT 1992	hiramc@sco.COM
 *	Change the bit swap stuff to reference the array
 *	in ddxMisc.c
 * X011	Wed Sep 09 10:09:22 PDT 1992	hiramc@sco.COM
 *	Add bit swap stuff and remove all mod comments before 1992
 * X010 07-Feb-92 kevin@xware.com
 *      fixed problem with cursor in 512K 16 colors modes.
 * X009 11-Jan-92 kevin@xware.com
 *      added workaround for the problem dealing with the cursor y offset in
 *      the interlaced display modes.
 * X008 11-Jan-92 kevin@xware.com
 *      modified to color cursor code to return a default value
 *      if the color map entry is not found.
 * X007 02-Jan-92 kevin@xware.com
 *      changed name to s3cHWCurs.c and added HW prefix to labels.
 * X006 01-Jan-92 kevin@xware.com
 *      updated copyright notice.
 */

#include "s3cConsts.h"
#include "s3cMacros.h"
#include "s3cDefs.h"
#include "s3cProcs.h"


static unsigned short
s3cHWConvert16(
        unsigned char byte);

/*
 *  s3cHWRealizeCursor() -- Realize the Hardware Cursor
 *
 */

static Bool
s3cHWRealizeCursor(
        ScreenPtr       pScreen,
        CursorPtr       pCursor)
{
        return TRUE;
}


/*
 *  s3cHWUnrealizeCursor() -- Unrealize the Hardware Cursor
 *
 */

static Bool
s3cHWUnrealizeCursor(
        ScreenPtr       pScreen,
        CursorPtr       pCursor)
{
        return TRUE;
}

void
S3CNAME(HWColorCursor)(
        ScreenPtr       pScreen,
	CursorPtr	pCursor)
{
	s3cPrivateData_t *s3cPriv = S3C_PRIVATE_DATA(pScreen);
        s3cHWCursorData_t *s3cHWCurs = S3C_HWCURSOR_DATA(pScreen);
	ColormapPtr pmap = NFB_SCREEN_PRIV(pScreen)->installedCmap;

	if (pCursor &&
	    ! (pCursor->foreRed == s3cHWCurs->fore.red &&
	    pCursor->foreGreen == s3cHWCurs->fore.green &&
	    pCursor->foreBlue == s3cHWCurs->fore.blue &&
	    pCursor->backRed == s3cHWCurs->mask.red &&
	    pCursor->backGreen == s3cHWCurs->mask.green &&
	    pCursor->backBlue == s3cHWCurs->mask.blue) ||
	    pmap != s3cHWCurs->current_colormap ||
	    pCursor != s3cHWCurs->current_cursor)
	{
		s3cHWCurs->current_colormap = pmap;
		s3cHWCurs->current_cursor = pCursor;

		s3cHWCurs->fore.red = pCursor->foreRed;
		s3cHWCurs->fore.green = pCursor->foreGreen;
		s3cHWCurs->fore.blue = pCursor->foreBlue;
		s3cHWCurs->fore.pixel = 0;
		FakeAllocColor (pmap, &s3cHWCurs->fore);

		s3cHWCurs->mask.red = pCursor->backRed;
		s3cHWCurs->mask.green = pCursor->backGreen;
		s3cHWCurs->mask.blue = pCursor->backBlue;
		s3cHWCurs->mask.pixel = 0;
		FakeAllocColor (pmap, &s3cHWCurs->mask);

		/* "free" the pixels right away, don't let this confuse you */
		FakeFreeColor(pmap, s3cHWCurs->fore.pixel);
		FakeFreeColor(pmap, s3cHWCurs->mask.pixel);

		S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);
        	S3C_OUTB(s3cPriv->crx, S3C_CR0E);
        	S3C_OUTB(s3cPriv->crd, s3cHWCurs->fore.pixel);
        	S3C_OUTB(s3cPriv->crx, S3C_CR0F);
        	S3C_OUTB(s3cPriv->crd, s3cHWCurs->mask.pixel);
	}
}


/* 
 *  s3cHWMoveCursor() -- Move Hardware Cursor
 *
 *      This routine will move the hardware cursor in either the 4 or 8
 *      bit display modes.
 *
 */

static void
s3cHWMoveCursor(
        ScreenPtr               pScreen,
        int                     x,
        int                     y)
{
        int                     xpos;
        int                     ypos;
        int                     xoff;
        int                     yoff;
        s3cPrivateData_t        *s3cPriv;
        s3cHWCursorData_t       *s3cHWCurs;
        
        s3cPriv = S3C_PRIVATE_DATA(pScreen);
        s3cHWCurs = S3C_HWCURSOR_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

        /*
         * calculate cusror position and offset
         */

        if ( ( xpos = x - s3cHWCurs->xhot ) < 0 )
        {
                xoff = xpos * -1;
                xpos = 0;
        }
        else
        {
                xoff = 0;
        }

        if ( ( ypos = y - s3cHWCurs->yhot ) < 0 )
        {
                yoff = ypos * -1 & ( s3cHWCurs->intl_fix ? ~1 : ~0 ); 
                ypos = 0;
        }
        else
        {
                yoff = 0;
        }

        /*
         * set cusror position and offset
         */

        S3C_OUTB(s3cPriv->crx, S3C_HGC_DX);
        S3C_OUTB(s3cPriv->crd, xoff);

        S3C_OUTB(s3cPriv->crx, S3C_HGC_DY);
        S3C_OUTB(s3cPriv->crd, yoff);
                
        S3C_OUTB(s3cPriv->crx, S3C_HGC_ORG_X0);
        S3C_OUTB(s3cPriv->crd, xpos);

        S3C_OUTB(s3cPriv->crx, S3C_HGC_ORG_X1);
        S3C_OUTB(s3cPriv->crd, xpos >> 8);

        S3C_OUTB(s3cPriv->crx, S3C_HGC_ORG_Y0);
        S3C_OUTB(s3cPriv->crd, ypos);

        S3C_OUTB(s3cPriv->crx, S3C_HGC_ORG_Y1);
        S3C_OUTB(s3cPriv->crd, ypos >> 8);

	S3CNAME(HWColorCursor)(pScreen, s3cHWCurs->current_cursor);
}


/* 
 *  s3cHWMoveCursor16() -- Move Hardware Cursor 16
 *
 *      This routine will move the hardware cursor in the 16 bit display
 *      modes.
 *
 */

static void
s3cHWMoveCursor16(
        ScreenPtr               pScreen,
        int                     x,
        int                     y)
{
        int                     xpos;
        int                     ypos;
        int                     xoff;
        int                     yoff;
        s3cPrivateData_t        *s3cPriv;
        s3cHWCursorData_t       *s3cHWCurs;
        
        s3cPriv = S3C_PRIVATE_DATA(pScreen);
        s3cHWCurs = S3C_HWCURSOR_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

        /*
         * calculate cusror position and offset
         */

        if ( ( xpos = ( x - s3cHWCurs->xhot ) << 1 ) < 0 )
        {
                xoff = xpos * -1;
                xpos = 0;
        }
        else
        {
                xoff = 0;
        }

        if ( ( ypos = y - s3cHWCurs->yhot ) < 0 )
        {
                yoff = ypos * -1 & ( s3cHWCurs->intl_fix ? ~1 : ~0 ); 
                ypos = 0;
        }
        else
        {
                yoff = 0;
        }

        /*
         * set cusror position and offset
         */

        S3C_OUTB(s3cPriv->crx, S3C_HGC_DX);
        S3C_OUTB(s3cPriv->crd, xoff);

        S3C_OUTB(s3cPriv->crx, S3C_HGC_DY);
        S3C_OUTB(s3cPriv->crd, yoff);
                
        S3C_OUTB(s3cPriv->crx, S3C_HGC_ORG_X0);
        S3C_OUTB(s3cPriv->crd, xpos);

        S3C_OUTB(s3cPriv->crx, S3C_HGC_ORG_X1);
        S3C_OUTB(s3cPriv->crd, xpos >> 8);

        S3C_OUTB(s3cPriv->crx, S3C_HGC_ORG_Y0);
        S3C_OUTB(s3cPriv->crd, ypos);

        S3C_OUTB(s3cPriv->crx, S3C_HGC_ORG_Y1);
        S3C_OUTB(s3cPriv->crd, ypos >> 8);
}


/* 
 *  s3cHWSetCursor() -- Set Hardware Cursor
 *
 *      This routine will set the hardware cursor shape if specified and
 *      move the hardware cursor in either the 4 or 8 bit display modes.
 *
 */

static void
s3cHWSetCursor(
        ScreenPtr               pScreen,
        CursorPtr               pCursor,
        int                     x,
        int                     y)
{
        unsigned short          *source;
        unsigned short          *mask;
        unsigned char          *ucsource;
        unsigned char          *ucmask;
        unsigned short          usTmp;
        int                     w;
        int                     h;
        s3cPrivateData_t        *s3cPriv;
        s3cHWCursorData_t       *s3cHWCurs;
	int	stride;					/*	S013	*/
        
        s3cPriv = S3C_PRIVATE_DATA(pScreen);
        s3cHWCurs = S3C_HWCURSOR_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

        /*
         * unlock registers
         */

        S3C_OUTB(s3cPriv->crx, S3C_S3R9);
        S3C_OUTB(s3cPriv->crd, S3C_S3R9_KEY2);

        if (pCursor == (CursorPtr)0)
        {
                S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
                S3C_OUTB(s3cPriv->crd,
		    S3C_INB(s3cPriv->crd) & ~S3C_HGC_MODE_HWCENA);
		return;
        }

        /*
         * initialize vars and enable the hw cursor 
         */

        s3cHWCurs->xhot = pCursor->bits->xhot;
        s3cHWCurs->yhot = pCursor->bits->yhot;

        S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
        S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) | S3C_HGC_MODE_HWCENA);

        /*
         * set the foreground and background colors for the
         * cursor
         */

	S3CNAME(HWColorCursor)(pScreen, pCursor);

        /*
         * specify the off screen address  of the 
         * cursor image and mask data.
         */

	/*	Code removed	S017	*/
	S3C_OUTB(s3cPriv->crx, S3C_HGC_SRC_Y0);
	S3C_OUTB(s3cPriv->crd, s3cHWCurs->source_y );

	S3C_OUTB(s3cPriv->crx, S3C_HGC_SRC_Y1);
	S3C_OUTB(s3cPriv->crd, s3cHWCurs->source_y >> 8 );

        /*
         * copy the cursor into off screen memory 
         */

        S3C_CLEAR_QUEUE(7);			/* was 8, S015	*/
        S3C_PLNWENBL(S3C_ALLPLANES);    
        S3C_SETMODE(S3C_M_ONES);
        S3C_SETFN1(S3C_FNVAR, S3C_FNREPLACE);
        S3C_SETLX(s3cHWCurs->off_screen.w - 1); 
        S3C_SETLY(s3cHWCurs->off_screen.h - 1);
        S3C_SETX0(s3cHWCurs->off_screen.x);
        S3C_SETY0(s3cHWCurs->off_screen.y);

	S3C_WAIT_FOR_IDLE();			/*	S015	*/
        S3C_COMMAND(S3C_CURS_X_Y_DATA);

        source = (unsigned short *)pCursor->bits->source;
        mask = (unsigned short *)pCursor->bits->mask;
	stride = PixmapBytePad( pCursor->bits->width, 1); /* S013 */

        for ( h = 0; h < S3C_MAX_CURSOR_SIZE; h++ )     
        {
                if ( h < pCursor->bits->height )
                {
			ucsource = (unsigned char *) source;
			ucmask = (unsigned char *) mask;
                        for ( w = 0; w < pCursor->bits->width; w += 16 )
                        {
#if	BITMAP_BIT_ORDER == LSBFirst		/*	X011	vvv	*/
		usTmp = (unsigned short) (0xff & ~(DDXBITSWAP(*ucmask)));
		usTmp |= (unsigned short)
			(0xff & ~(DDXBITSWAP(*(ucmask+1)))) << 8;
		S3C_OUT(S3C_VARDATA, usTmp );
		usTmp = (unsigned short)
			(0xff & DDXBITSWAP( *ucsource++ & *ucmask++ ));
		usTmp |= (unsigned short) (0xff & (DDXBITSWAP( *ucsource++ &
				*ucmask++ ) ) ) << 8;
		S3C_OUT(S3C_VARDATA, usTmp );
		++mask;
		++source;
#else
				S3C_OUT(S3C_VARDATA, ~(*mask));
				S3C_OUT(S3C_VARDATA, *source++ & *mask++);
#endif						/*	X011	^^^	*/
                        }

			if ( (w >> 3) != stride )	/*	S013 */
                        {
                                source++;
                                mask++;
                        }
                
                        for ( ; w < S3C_MAX_CURSOR_SIZE; w += 16 )
                        {
                                S3C_OUT(S3C_VARDATA, 0xFFFF);
                                S3C_OUT(S3C_VARDATA, 0x0000);
                        }
                }
                else
                {
                        for ( w = 0; w < S3C_MAX_CURSOR_SIZE; w += 16 )
                        {
                                S3C_OUT(S3C_VARDATA, 0xFFFF);
                                S3C_OUT(S3C_VARDATA, 0x0000);
                        }
                }
        }

        /*
         * position cursor
         */

        s3cHWMoveCursor(pScreen, x, y);
}

/* 
 *  s3cHWSetCursor16() -- Set Hardware Cursor 16
 *
 *      This routine will set the hardware cursor shape if specified and
 *      move the hardware cursor in the 16 bit display modes.
 *
 */

static void
s3cHWSetCursor16(
        ScreenPtr               pScreen,
        CursorPtr               pCursor,
        int                     x,
        int                     y)
{
        unsigned char           *source;
        unsigned char           *mask;
        int                     w;
        int                     h;
        s3cPrivateData_t        *s3cPriv;
        s3cHWCursorData_t       *s3cHWCurs;
	int	stride;					/*	S013	*/
	int	bytes;					/*	S013	*/
	int	MaxCurBytes;				/*	S013	*/
        
        s3cPriv = S3C_PRIVATE_DATA(pScreen);
        s3cHWCurs = S3C_HWCURSOR_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

        /*
         * unlock registers
         */

        S3C_OUTB(s3cPriv->crx, S3C_S3R9);
        S3C_OUTB(s3cPriv->crd, S3C_S3R9_KEY2);

        if ( pCursor == (CursorPtr)0 )
        {
                S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
                S3C_OUTB(s3cPriv->crd, 
                        S3C_INB(s3cPriv->crd) & ~S3C_HGC_MODE_HWCENA);
        }
        else
        {
                /*
                 * initialize vars and enable the hw cursor 
                 */

		/*	Not beyond x = 32 please		S013	*/
                s3cHWCurs->xhot = pCursor->bits->xhot >
		((S3C_MAX_CURSOR_SIZE >> 1) - 1) ?
		((S3C_MAX_CURSOR_SIZE >> 1) - 1) : pCursor->bits->xhot;/*S013*/
                s3cHWCurs->yhot = pCursor->bits->yhot;

                S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
                S3C_OUTB(s3cPriv->crd, 
                        S3C_INB(s3cPriv->crd) | S3C_HGC_MODE_HWCENA);

                /*
                 * set the foreground and background colors for the
                 * cursor
                 *
                 * NOTE: in the 16 bit display modes we can only
                 * support black and white cursors.
                 */

                S3C_OUTB(s3cPriv->crx, S3C_CR0E);
                S3C_OUTB(s3cPriv->crd,
                        ( pCursor->foreRed 
                        | pCursor->foreGreen 
                        | pCursor->foreBlue ) ? 0xFF : 0x00 ); 

                S3C_OUTB(s3cPriv->crx, S3C_CR0F);
                S3C_OUTB(s3cPriv->crd, 
                        ( pCursor->backRed 
                        | pCursor->backGreen 
                        | pCursor->backBlue ) ? 0xFF : 0x00 ); 

                /*
                 * specify the off screen address  of the 
                 * cursor image and mask data.
                 */

                S3C_OUTB(s3cPriv->crx, S3C_HGC_SRC_Y0);
                S3C_OUTB(s3cPriv->crd, 
                        s3cHWCurs->source_y );	/*	S015 S016	*/
        
                S3C_OUTB(s3cPriv->crx, S3C_HGC_SRC_Y1);
                S3C_OUTB(s3cPriv->crd, 
                        s3cHWCurs->source_y >> 8 );	/* S015 S016	*/

                /*
                 * copy the cursor into off screen memory 
                 */

                S3C_CLEAR_QUEUE(7);		/*	was 8, S015	*/
                S3C_PLNWENBL(S3C_ALLPLANES);    
                S3C_SETMODE(S3C_M_ONES);
                S3C_SETFN1(S3C_FNVAR, S3C_FNREPLACE);
                S3C_SETLX(( s3cHWCurs->off_screen.w >> 2 ) - 1); 
                S3C_SETLY(s3cHWCurs->off_screen.h - 1);
                S3C_SETX0(s3cHWCurs->off_screen.x);
                S3C_SETY0(s3cHWCurs->off_screen.y);

		S3C_WAIT_FOR_IDLE();
                S3C_COMMAND(S3C_CURS_X_Y_DATA);
        
                mask = pCursor->bits->mask;
		stride = PixmapBytePad( pCursor->bits->width, 1); /* S013 vvv */
		/*	Can only handle 32x64 cursors in 16 bit modes	*/
		MaxCurBytes = ((S3C_MAX_CURSOR_SIZE >> 1) + 7) >> 3;
		bytes = (pCursor->bits->width + 7) >> 3;
		bytes = bytes  > MaxCurBytes ? MaxCurBytes : bytes;

                for ( h = 0; h < S3C_MAX_CURSOR_SIZE; h++ )     
                {
                        if ( h < pCursor->bits->height )
                        {
				for ( w = 0; w < bytes; ++ w ) {
                                     S3C_OUT(S3C_VARDATA,
				s3cHWConvert16(~(DDXBITSWAP(*(mask+w)))));
				}
				mask += stride;
				for ( ; w < MaxCurBytes; ++w ) {
                                     S3C_OUT(S3C_VARDATA, 0xFFFF );
				}

                        }
                        else
                        {
                                for ( w = 0; w < MaxCurBytes; ++w )
                                {
                                        S3C_OUT(S3C_VARDATA, 0xFFFF);
                                }
                        }
                }						/* S013 ^^^ */

                S3C_CLEAR_QUEUE(2);
                S3C_SETX0(s3cHWCurs->off_screen.x + 1024);
                S3C_SETY0(s3cHWCurs->off_screen.y);

		S3C_WAIT_FOR_IDLE();
                S3C_COMMAND(S3C_CURS_X_Y_DATA);

                source = pCursor->bits->source;
                mask = pCursor->bits->mask;

                for ( h = 0; h < S3C_MAX_CURSOR_SIZE; h++ )     
                {
                        if ( h < pCursor->bits->height )
                        {
				for ( w = 0; w < bytes; ++ w ) { /* S013 vvv */
                                     S3C_OUT(S3C_VARDATA,
	s3cHWConvert16(DDXBITSWAP(*(source+w)) & DDXBITSWAP(*(mask+w))));
				}
				mask += stride;
				source += stride;

                                for ( ; w < MaxCurBytes; ++w )
                                {
                                        S3C_OUT(S3C_VARDATA, 0x0000);
                                }
                        }
                        else
                        {
                                for ( w = 0; w < MaxCurBytes; ++w )/*S013 ^^^ */
                                {
                                        S3C_OUT(S3C_VARDATA, 0x0000);
                                }
                        }
                }
        }

        /*
         * position cursor
         */

        s3cHWMoveCursor16(pScreen, x, y);
}

static void
s3cRecolorCursor(
	ScreenPtr pScreen,
	CursorPtr pCursor,
	Bool displayed)
{
        s3cHWCursorData_t *s3cHWCurs = S3C_HWCURSOR_DATA(pScreen);

	if (pCursor && displayed)
		S3CNAME(HWColorCursor)(pScreen, pCursor);
}

/* 
 *  s3cHWCursorOn() -- Turn Hardware Cursor On or Off
 *
 *      This routine will turn the hardware cursor on or off.
 *      This routine will be called by the multi-head code
 *      to turn the cursor off when the pointer moves to
 *      another screen.  
 *
 */

void
S3CNAME(HWCursorOn)(
        int                     on,
        ScreenPtr               pScreen)
{
        s3cPrivateData_t        *s3cPriv;
        
        s3cPriv = S3C_PRIVATE_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

        if ( on )
        {
                S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
                S3C_OUTB(s3cPriv->crd, 
                        S3C_INB(s3cPriv->crd) | S3C_HGC_MODE_HWCENA);
        }
        else
        {
                S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
                S3C_OUTB(s3cPriv->crd, 
                        S3C_INB(s3cPriv->crd) & ~S3C_HGC_MODE_HWCENA);
        }
}


/*
 *  s3cHWConvert16() -- Convert Cursor Image 16 
 *
 *      This routine will expand a byte of cursor data into a word
 *      performing a 2 to 1 zoom on the byte of bit mapped data.
 *      This function is used to convert a cursor image for the
 *      16 bit display modes.
 *
 */

static unsigned short
s3cHWConvert16(
        unsigned char           byte)
{
        static unsigned char    convert_table[] =
        {
                0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F, 
                0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF
        };

        return ( convert_table[ ( byte & 0xF0 ) >> 4 ] 
                | convert_table[ byte & 0x0F ] << 8 );
}

static miPointerSpriteFuncRec  s3cHWCursFuncs = 
{
        s3cHWRealizeCursor,
        s3cHWUnrealizeCursor,
        s3cHWSetCursor,
        s3cHWMoveCursor
};

/* 
 *  s3cHWCursorInitialize() -- Initialize the Hardware Cursor
 *
 *      This routine will register the cursor movement routines for the 
 *      hardware cursor.
 *
 */

void
S3CNAME(HWCursorInitialize)(
        ScreenPtr               pScreen)
{
        s3cPrivateData_t        *s3cPriv;
        s3cHWCursorData_t       *s3cHWCurs;
        
        s3cPriv = S3C_PRIVATE_DATA(pScreen);
        s3cHWCurs = S3C_HWCURSOR_DATA(pScreen);
	S3C_VALIDATE_SCREEN(s3cPriv, pScreen, pScreen->myNum);

        /*
         * if the pixel depth is 16, update the sprite funtion 
         * pointers to the 16 bit cursor functions
         */

        if ( s3cPriv->depth == 16 )
        {
                s3cHWCursFuncs.SetCursor = s3cHWSetCursor16;
                s3cHWCursFuncs.MoveCursor = s3cHWMoveCursor16;
        }

        /*
         * initialize the pointer functions
         */

        if( scoPointerInitialize(pScreen, &s3cHWCursFuncs, NULL, TRUE) == 0 )
            FatalError("s3cHWCursorInitialize(): Cannot initialize cursor\n");

        /*
         * set the flag for the x offfset workaround if were in
         * and interlaced display mode.
         */

        S3C_OUTB(s3cPriv->crx, S3C_S3R9);
        S3C_OUTB(s3cPriv->crd, S3C_S3R9_KEY2);

        S3C_OUTB(s3cPriv->crx, S3C_MODE_CTL);
        s3cHWCurs->intl_fix = S3C_INB(s3cPriv->crd) & S3C_MODE_CTL_INTL;

	/*
	 * turn cursor off
	 */
	S3C_OUTB(s3cPriv->crx, S3C_HGC_MODE);
	S3C_OUTB(s3cPriv->crd, S3C_INB(s3cPriv->crd) & ~S3C_HGC_MODE_HWCENA);

	s3cHWCurs->current_colormap = 0;
	s3cHWCurs->current_cursor = 0;

	pScreen->RecolorCursor = s3cRecolorCursor;
}
