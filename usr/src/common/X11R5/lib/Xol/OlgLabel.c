#ifndef NOIDENT
#ident	"@(#)olg:OlgLabel.c	1.19"
#endif

/* Label Functions */

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#include <stdio.h>  
#include <string.h>
#include <ctype.h>

#ifdef I18N

#ifndef sun     /* or other porting that does care I18N */
#include <sys/euc.h>
#else
#define SS3     0x8e    /* copied from sys/euc.h */
#define SS2     0x8f
#endif

#endif /* I18N */

#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include <Xol/OlgP.h>

static char	mnemonicString [] = "(x)";

static char *
multiStrChr(labeldata, mnemonic)
  OlgTextLbl *labeldata;
  char mnemonic;
{
  OlStrSegment	olss;
  unsigned char	tmp[2], *str;
  int		cset1;
  unsigned char	*result = NULL;
  int 	 	len = 1;

#ifdef DEBUG
fprintf(stderr, "labeldata: %s, %c.\n", labeldata->label,
	mnemonic);
#endif

  if (!labeldata->font_list)
    return strchr(labeldata->label, mnemonic);
  else {
    char tmpstr[BUFSIZ];
    
    tmp[0] = (unsigned char)mnemonic;
    tmp[1] = '\0';
    str = &(tmp[0]);
    olss.str = (unsigned char *)malloc(sizeof(unsigned char) * 2);
    olss.len = 0;
    OlGetNextStrSegment(labeldata->font_list, &olss, &str, &len);
    cset1 = olss.code_set;
  
    olss.str = (unsigned char *)tmpstr;
    str = (unsigned char *)&(labeldata->label[0]);
    
    while(1) {
      result = str;
    
      len = strlen((char *)str);
      if (len == 0) {
#ifdef DEBUG
fprintf(stderr, "returning NULL 1.\n");
#endif      
	return NULL;
      }
      OlGetNextStrSegment(labeldata->font_list, &olss,
			  &str, &len); 
      if (olss.code_set == cset1) {
	char *tmpresult;
	if ((tmpresult = strchr((char *)olss.str, mnemonic))) {
#ifdef DEBUG
fprintf(stderr, "result = %s\n", result + (tmpresult - olss.str));
#endif
          return ((char *)result + (tmpresult - (char *)olss.str));
	}
      }
      else {
#ifdef DEBUG
fprintf(stderr, "mne_set: %d, code_set: %d.\n", cset1, olss.code_set);
#endif
      }
      if (!strcmp((char *)olss.str, (char *)str)) {
#ifdef DEBUG
fprintf(stderr, "returning NULL 2.\n");
#endif      
      return NULL;
      }
    }
  }
}

/* Determine the width and height in pixels of each of the component parts
 * of a label.  All parts are assumed to have the same height.
 */

static void
sizeLabelParts (labeldata, labelWidth, mnemonicWidth, popupWidth,
		padWidth, acceleratorWidth, height)
    OlgTextLbl	*labeldata;
    Dimension	*labelWidth, *mnemonicWidth, *popupWidth, *padWidth,
		*acceleratorWidth, *height;
{
    if (labeldata->font_list)
    {
        *labelWidth = OlTextWidth(labeldata->font_list,
                                  (unsigned char *)labeldata->label,
                                  strlen(labeldata->label));
        *height = labeldata->font_list->max_bounds.ascent +
	    labeldata->font_list->max_bounds.descent;
    } else
    {
	int		dummy, ascent, descent;
	XCharStruct	overall;

	XTextExtents (labeldata->font, labeldata->label,
		      strlen(labeldata->label), &dummy,
		      &ascent, &descent, &overall); 
    	*labelWidth = overall.width;
    	*height = ascent + descent;
    }

    *mnemonicWidth = 0;
    if (labeldata->mnemonic)
    {
	OlDefine	displayStyle;

	displayStyle = OlQueryMnemonicDisplay (0);
	if (displayStyle == OL_HIGHLIGHT || displayStyle == OL_UNDERLINE)
	{
	  
/*	    if (!strchr (labeldata->label, labeldata->mnemonic))*/
	    if (!multiStrChr(labeldata, labeldata->mnemonic))
	    {
		char	mnemonic;
		Boolean	found = False;

		if (isalpha (labeldata->mnemonic))
		{
		    mnemonic = islower (labeldata->mnemonic) ?
			toupper (labeldata->mnemonic) :
			tolower (labeldata->mnemonic);

/*		    if (strchr (labeldata->label, mnemonic))*/
		    if (multiStrChr(labeldata, mnemonic))
		    {
			found = True;
			labeldata->mnemonic = mnemonic;
		    }
		}

		if (!found)
		{
		    mnemonicString [1] = labeldata->mnemonic;
		    if (labeldata->font_list == NULL)
		        *mnemonicWidth = XTextWidth (labeldata->font,
						 mnemonicString, 3);
		    else
		        *mnemonicWidth = OlTextWidth (labeldata->font_list,
					 (unsigned char *)mnemonicString, 3);
		}
	    }
	}
    }

    if (labeldata->flags & TL_POPUP) {
      if (labeldata->font_list)
	*popupWidth = OlTextWidth (labeldata->font_list, (unsigned char *)
				   "...", 3);
      else
	*popupWidth = XTextWidth (labeldata->font, "...", 3);
    }
    else
	*popupWidth = 0;

    if (labeldata->accelerator && OlQueryAcceleratorDisplay (0) == OL_DISPLAY)
    {
	if (labeldata->font_list == NULL) {
	   *acceleratorWidth = XTextWidth (labeldata->font,
					labeldata->accelerator,
					strlen (labeldata->accelerator));
	   *padWidth = XTextWidth (labeldata->font, " ", 1);
	 }
	else {
	   *acceleratorWidth = OlTextWidth (labeldata->font_list,
					(unsigned char *)labeldata->accelerator,
					strlen (labeldata->accelerator));

	   *padWidth = OlTextWidth (labeldata->font_list,
				    (unsigned char *)" ", 1);
	 }
    }
    else
    {
	*acceleratorWidth = 0;
	*padWidth = 0;
    }
#ifdef DEBUG
fprintf(stderr, "sizelabelparts [LEAVE]\n");
#endif    

}

/* Draw a text label in the given box.  Clip to the box, if necessary. */

void
OlgDrawTextLabel OLARGLIST((scr, win, pInfo, x, y, width, height, labeldata))
    OLARG( Screen *,		scr)
    OLARG( Drawable,		win)
    OLARG( OlgAttrs *,		pInfo)
    OLARG( Position,		x)
    OLARG( Position,		y)
    OLARG( Dimension,		width)
    OLARG( Dimension,		height)
    OLGRA( OlgTextLbl *,	labeldata)
{
    Position	xLbl, yLbl;	/* baseline position of text */
    Position	acceleratorPos;	/* left position of accelerator text */
    Dimension	lblWidth, lblHeight;	/* dimensions of label in pixels */
    Dimension	mnemonicWidth, popupWidth, padWidth, acceleratorWidth;
    Dimension	totalWidth;
    int		lblLen;		/* length of string in chars */
    GC		gc, inverseGC;
    OlDefine	displayStyle;
    unsigned	clip = False;
    OlFontList	*font_list = labeldata->font_list;

    /* Get the size of the label, mnemonic, elipses, and accelerator, if any */
    sizeLabelParts (labeldata, &lblWidth, &mnemonicWidth, &popupWidth,
		    &padWidth, &acceleratorWidth, &lblHeight);

    totalWidth = lblWidth + mnemonicWidth + popupWidth + padWidth +
	acceleratorWidth;
    if (totalWidth > width)
    {
	/* Label is too wide.  Do not draw the accelerator. */
	totalWidth -= padWidth + acceleratorWidth;
	acceleratorWidth = 0;
    }

    /* Position the text. */
    acceleratorPos = x + width - acceleratorWidth;
    if (labeldata->justification == TL_LEFT_JUSTIFY || totalWidth > width)
	xLbl = x;
    else
    {
	if (labeldata->justification == TL_CENTER_JUSTIFY)
	{
	    xLbl = x + (int) (width - totalWidth) / 2;
	    acceleratorPos = xLbl + totalWidth - acceleratorWidth;
	}
	else
	    xLbl = x + width - totalWidth;
    }

      yLbl = y + (int) (height - lblHeight) / 2 +
	  OlFontAscent(labeldata->font, font_list);

    /* If the text is too wide to fit in the box, throw out the mnemonic
     * and see if there's room.  If not, discard characters
     * at the end until it will fit (leave room for the arrow).
     */
    if ((Dimension) (lblWidth+popupWidth) > width)
    {
	register char	*pC;
	register int	len;

	len = width - pInfo->pDev->arrowText.width * 2;
	pC = labeldata->label;

	if (!font_list)
	   while (*pC)
	   {
	       len -= XTextWidth (labeldata->font, pC, 1);
	       if (len < 0)
		   break;

	       pC++;
	   }
	else
	    while (*pC)
	   {
 		unsigned char	c = *pC;
 		int		cwidth;
 
 		if (c == SS2)
 			cwidth = 1+font_list->cswidth[2];
 		else if (c == SS3)
 			cwidth = 1+font_list->cswidth[3];
 		else if (c >= 128)
 			cwidth = font_list->cswidth[1];
 		else
 			cwidth = font_list->cswidth[0];
 
                len -= OlTextWidth (font_list, (unsigned char *)pC, cwidth);

               if (len < 0)
		   break;

		pC += cwidth;
	   }

	lblLen = pC - labeldata->label;
    }
    else
	lblLen = strlen (labeldata->label);

    /* Clip if must */
    if (height < lblHeight)
    {
	XRectangle	clipRect;

	clip = True;
	clipRect.x = x;
	clipRect.y = y;
	clipRect.width = width;
	clipRect.height = height;

	XSetClipRectangles (DisplayOfScreen (scr), labeldata->normalGC,
			    0, 0, &clipRect, 1, YXBanded);
	XSetClipRectangles (DisplayOfScreen (scr), labeldata->inverseGC,
			    0, 0, &clipRect, 1, YXBanded);
    }

    gc = ((labeldata->flags & TL_SELECTED) && !OlgIs3d()) ?
	labeldata->inverseGC : labeldata->normalGC;

    /* Draw the label */
    if (lblLen > 0)
    {
	if (font_list == NULL)
		XDrawString (DisplayOfScreen (scr), win, gc, xLbl, yLbl,
		    	     labeldata->label, lblLen);
	else
		OlDrawString(DisplayOfScreen (scr), win, font_list,
			     gc, xLbl, yLbl,
			     (unsigned char *)labeldata->label, lblLen);

	xLbl += lblWidth;
    }

    /* Draw the mnemonic, if there is one and there's room */
    displayStyle = OlQueryMnemonicDisplay (0);
    if (labeldata->mnemonic && totalWidth <= width &&
	(displayStyle == OL_HIGHLIGHT || displayStyle == OL_UNDERLINE))
    {
	char		*pMnemonic;
	Position	highlightPos;
	Dimension	highlightWidth;

/*	pMnemonic = strchr (labeldata->label, labeldata->mnemonic);*/
	pMnemonic = multiStrChr(labeldata, labeldata->mnemonic);
	if (pMnemonic)
	{
	    int		mnemonicPos;

	    /* The mnemonic is included in the label */
	    mnemonicPos = pMnemonic - labeldata->label;
	    if (font_list == NULL)
	       {
	       highlightPos = xLbl - lblWidth + XTextWidth (labeldata->font,
					     labeldata->label, mnemonicPos);
	       highlightWidth = XTextWidth (labeldata->font,
					 labeldata->label + mnemonicPos, 1);
	       }
	    else
	       {
	       highlightPos = xLbl - lblWidth +
		 OlTextWidth (font_list, (unsigned char *)labeldata->label,
			      mnemonicPos); 
	       highlightWidth = OlTextWidth (font_list, (unsigned char *)labeldata->label +
					     mnemonicPos, 1); 
	       }
	}
	else
	{
	    /* The mnemonic is not part of the label. */
	    mnemonicString [1] = labeldata->mnemonic;
	    if (font_list == NULL)
		{
	        XDrawString (DisplayOfScreen (scr), win, gc, xLbl, yLbl,
			     mnemonicString, 3);
	        highlightPos = xLbl + XTextWidth (labeldata->font,
					     mnemonicString, 1);
	        highlightWidth = XTextWidth (labeldata->font,
					mnemonicString + 1, 1);
		}
	else
	        {
            	OlDrawString(DisplayOfScreen (scr), win, font_list,
                             gc, xLbl, yLbl,
			     (unsigned char *)mnemonicString, 3);
	        highlightPos = xLbl + OlTextWidth (font_list,
				     (unsigned char *)mnemonicString, 1);
	        highlightWidth = OlTextWidth (font_list,
				(unsigned char *)mnemonicString + 1, 1);
		}
	    xLbl += mnemonicWidth;
	}

	/* Either underline it or reverse the fg and bg */
	if (displayStyle == OL_HIGHLIGHT)
	{
	    GC	textGC;

	    textGC = (gc == labeldata->normalGC) ? labeldata->inverseGC :
		labeldata->normalGC;

	    XFillRectangle (DisplayOfScreen (scr), win, gc,
			    highlightPos, yLbl - labeldata->font->ascent,
			    highlightWidth, lblHeight);
            if (font_list == NULL)
	        XDrawString (DisplayOfScreen (scr), win, textGC,
			     highlightPos, yLbl, &labeldata->mnemonic, 1);
	    else
            	OlDrawString(DisplayOfScreen (scr), win, font_list, textGC,
			     highlightPos, yLbl,
			     (unsigned char *)&labeldata->mnemonic, 1);
	}
	else
	{
	    XFillRectangle (DisplayOfScreen (scr), win, gc,
			    highlightPos, yLbl+OlgGetVerticalStroke(pInfo)+1,
			    highlightWidth, OlgGetVerticalStroke (pInfo));
	}
    }

    /* If the label was truncated, draw the arrow.  If it wasn't, draw
     * the elipses, if any.
     */
    if ((Dimension) (lblWidth+popupWidth) > width)
	OlgDrawObject (scr, win, gc, &pInfo->pDev->arrowText,
		       x + width - pInfo->pDev->arrowText.width / 2,
		       y + height / 2);
    else
	/* do not need to i18n'd the following XDrawString() function */
	if (labeldata->flags & TL_POPUP)
	    XDrawString (DisplayOfScreen (scr), win, gc, xLbl, yLbl, "...", 3);

    /* Draw the accelerator, if there's room. */
    if (acceleratorWidth > 0)
    {
        if (font_list == NULL)
		XDrawString (DisplayOfScreen (scr), win, gc, acceleratorPos, 
			     yLbl, labeldata->accelerator, 
			     strlen (labeldata->accelerator));
	else
		OlDrawString (DisplayOfScreen (scr), win, font_list, gc, 
			      acceleratorPos, yLbl,
			      (unsigned char *)labeldata->accelerator, 
			      strlen (labeldata->accelerator));
    }

    /* If the label is insensitive, create a stipple gc and stipple the area */
    if (labeldata->flags & TL_INSENSITIVE)
    {
	GC	gc;

	if (labeldata->stippleColor == BlackPixelOfScreen (scr))
	    gc = pInfo->pDev->dimGrayGC;
	else if (labeldata->stippleColor == WhitePixelOfScreen (scr))
	    gc = pInfo->pDev->lightGrayGC;
	else
	{
	    gc = pInfo->pDev->scratchGC;
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	    XSetStipple (DisplayOfScreen (scr), gc,
			 pInfo->pDev->inactiveStipple);
	    XSetForeground (DisplayOfScreen (scr), gc,
			    labeldata->stippleColor);
	}

	XFillRectangle (DisplayOfScreen (scr), win, gc, x, y, width, height);
    }

    if (clip)
    {
	XSetClipMask (DisplayOfScreen (scr), labeldata->normalGC, None);
	XSetClipMask (DisplayOfScreen (scr), labeldata->inverseGC, None);
    }
}

/* Calculate size of text label.  Size is the bounding box of the text. */

void
OlgSizeTextLabel OLARGLIST((scr, pInfo, labeldata, pWidth, pHeight))
    OLARG( Screen *,		scr)
    OLARG( OlgAttrs *,		pInfo)
    OLARG( OlgTextLbl *,	labeldata)
    OLARG( Dimension *,		pWidth)
    OLGRA( Dimension *,		pHeight)
{
    Dimension	labelWidth, mnemonicWidth, popupWidth, padWidth,
		acceleratorWidth;

    sizeLabelParts (labeldata, &labelWidth, &mnemonicWidth, &popupWidth,
		    &padWidth, &acceleratorWidth, pHeight);
    *pWidth = labelWidth + mnemonicWidth + popupWidth + padWidth +
	acceleratorWidth;
}

/* Draw a pixmap label in the given box.  Clip to the box, if necessary.
 * The flags member of the labeldata structure contains additional information
 * on how to draw the label.  If the tile flag is set, then the label image is
 * assumed to be a tile or stipple in lblGC; the label is not used here.  If
 * the insensitive flag is set, then the image is stippled with a 50% gray
 * using the stippleColor member.  Label images can be pixmaps, bitmaps, or
 * X Images, indicated by the type member.
 */

void
OlgDrawPixmapLabel OLARGLIST((scr, win, pInfo, x, y, width, height, labeldata))
    OLARG( Screen *,		scr)
    OLARG( Drawable,		win)
    OLARG( OlgAttrs *,		pInfo)
    OLARG( Position,		x)
    OLARG( Position,		y)
    OLARG( Dimension,		width)
    OLARG( Dimension,		height)
    OLGRA( OlgPixmapLbl	*,	labeldata)
{
    Position	xLbl, yLbl;		/* position in widow to draw label */
    Position	xSrc, ySrc;		/* origin of visible label */
    Dimension	widthLbl, heightLbl;	/* dimensions of visible label */
    Dimension	lblWidth, lblHeight;	/* desired dimensions of label */

    OlgSizePixmapLabel (scr, pInfo, labeldata, &lblWidth, &lblHeight);

    /* If the label is not tiled, then figure out where to draw it and put
     * it there.
     */
    if (!(labeldata->flags & PL_TILED))
    {
	/* label is always centered vertically.  x position depends on
	 * justification.
	 */
	yLbl = y + (Position) (height - lblHeight) / 2;
	switch (labeldata->justification) {
	case TL_LEFT_JUSTIFY:
	    xLbl = x;
	    break;

	case TL_RIGHT_JUSTIFY:
	    xLbl = x + (Position) (width - lblWidth);
	    break;

	default:
	case TL_CENTER_JUSTIFY:
	    xLbl = x + (Position) (width - lblWidth) / 2;
	    break;
	}

	/* clip to the label bounding box */
	xSrc = ySrc = 0;
	widthLbl = lblWidth;
	heightLbl = lblHeight;

	if (xLbl < x)
	{
	    widthLbl -= (xSrc = x - xLbl);
	    xLbl = x;
	}

	if (widthLbl > width)
	    widthLbl = width;

	if (yLbl < y)
	{
	    heightLbl -= (ySrc = y - yLbl);
	    yLbl = y;
	}

	if (heightLbl > height)
	    heightLbl = height;

	/* Draw the label */
	switch (labeldata->type) {
	case PL_IMAGE:
	    XPutImage (DisplayOfScreen (scr), win, labeldata->normalGC,
		       labeldata->label.image, xSrc, ySrc, xLbl, yLbl,
		       widthLbl, heightLbl);
	    break;

	case PL_PIXMAP:
	    XCopyArea (DisplayOfScreen (scr), labeldata->label.pixmap, win,
		       labeldata->normalGC, xSrc, ySrc, widthLbl, heightLbl,
		       xLbl, yLbl);
	    break;

	case PL_BITMAP:
	    XCopyPlane (DisplayOfScreen (scr), labeldata->label.pixmap, win,
			labeldata->normalGC, xSrc, ySrc, widthLbl, heightLbl,
			xLbl, yLbl, 1);
	    break;
	}
    }
    else	/* label is tiled */
    {
	/* Just fill the area.  The gc is assumed to have the proper fill
	 * style and tile or stipple
	 */
	XFillRectangle (DisplayOfScreen (scr), win, labeldata->normalGC,
			x, y, width, height);
    }

    /* If the label is insensitive, create a stipple gc and stipple the area */
    if (labeldata->flags & PL_INSENSITIVE)
    {
	GC	gc;

	if (labeldata->stippleColor == BlackPixelOfScreen (scr))
	    gc = pInfo->pDev->dimGrayGC;
	else if (labeldata->stippleColor == WhitePixelOfScreen (scr))
	    gc = pInfo->pDev->lightGrayGC;
	else
	{
	    gc = pInfo->pDev->scratchGC;
	    XSetFillStyle (DisplayOfScreen (scr), gc, FillStippled);
	    XSetStipple (DisplayOfScreen (scr), gc,
			 pInfo->pDev->inactiveStipple);
	    XSetForeground (DisplayOfScreen (scr), gc,
			    labeldata->stippleColor);
	}

	if (labeldata->flags & PL_TILED)
	    XFillRectangle (DisplayOfScreen (scr), win, gc, xLbl, yLbl,
			    widthLbl, heightLbl);
	else
	    XFillRectangle (DisplayOfScreen (scr), win, gc, x, y,
			    width, height);
    }
}

/* Calculate size of an image label. */

void
OlgSizePixmapLabel OLARGLIST((scr, pInfo, labeldata, pWidth, pHeight))
    OLARG( Screen *,		scr)
    OLARG( OlgAttrs *,		pInfo)
    OLARG( OlgPixmapLbl	*,	labeldata)
    OLARG( Dimension *,		pWidth)
    OLGRA( Dimension *,		pHeight)
{
    unsigned	pix_width, pix_height;
    unsigned	ignore_value;
    Drawable	ignore_window;

    switch (labeldata->type) {
    case PL_IMAGE:
	*pWidth = labeldata->label.image->width;
	*pHeight = labeldata->label.image->height;
	break;

    case PL_PIXMAP:
    case PL_BITMAP:
	(void) XGetGeometry(DisplayOfScreen (scr), labeldata->label.pixmap,
			    &ignore_window, (int *) &ignore_value,
			    (int *) &ignore_value,
			    &pix_width, &pix_height, &ignore_value,
			    &ignore_value);
	*pWidth = pix_width;
	*pHeight = pix_height;
	break;
    }
}
