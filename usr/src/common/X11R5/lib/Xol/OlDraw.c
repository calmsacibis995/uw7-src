/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)olmisc:OlDraw.c	1.14"
#endif

/*
 *************************************************************************
 *
 * Description:
 * 		This file contains routines that are common to
 *		OPEN LOOK (TM - AT&T) widgets.
 * 
 ****************************file*header**********************************
 */

						/* #includes go here	*/
#include <stdio.h>

#ifdef I18N

#ifndef sun	/* or other porting that does care I18N */
#include <sys/euc.h>
#else
#define SS3	0x8e	/* copied from sys/euc.h */
#define SS2	0x8f
#endif

#endif /* I18N */

#include <X11/Xatom.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <OpenLookP.h>

#ifndef MAX_SIZE
#define	MAX_SIZE	512
#endif

/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */


static	int	_OlDraw();		/* real Drawing utility		*/


/*
  This routine is obsolete.  The 'max' metrics for the fonts are now stored
  in the new font list structure.  - mpz

  The OlMaxFontInfo() computes the max. ascent, descent, width and height
  of various fonts specified in the "fontlist" argument. This is useful
  in determining a caret position after drawing an encoded string in which
  fonts may have different dimensions based on the character set they 
  belong to
*/

extern OlFontInfo *
OlMaxFontInfo OLARGLIST((fontlist))
OLGRA(OlFontList, *fontlist)
{
	register int i;
	OlFontInfo * fontinfo;

	if (fontlist == NULL)
	   return (NULL);

	fontinfo = (OlFontInfo *)XtMalloc(sizeof(OlFontInfo));

	fontinfo->ascent = 0;
	fontinfo->descent = 0;
	fontinfo->width = 0;
	fontinfo->height = 0;

	for (i=0; i < fontlist->num; i++)
    {
	_OlAssignMax(fontinfo->ascent, fontlist->fontl[i]->ascent);
	_OlAssignMax(fontinfo->descent, fontlist->fontl[i]->descent);
	_OlAssignMax(fontinfo->width, fontlist->fontl[i]->max_bounds.width);
    }

	_OlAssignMax(fontinfo->height, fontinfo->ascent + fontinfo->descent);

	return(fontinfo);

} /* end of OlMaxFontInfo() */

extern int
OlDrawString OLARGLIST((dpy, drawable, fontlist, gc, x, y, str, len))
OLARG(Display, *dpy)
OLARG(Drawable, drawable)
OLARG(OlFontList,      *fontlist)
OLARG(GC,   gc)
OLARG(int,  x)
OLARG(int,  y)
OLARG(unsigned char,    *str)
OLGRA(int,    len)
{
	return(_OlDraw(dpy, drawable, fontlist, gc, x, y, str, len, True));

} /* end of OlDrawString() */

extern int
OlDrawImageString OLARGLIST((dpy, drawable, fontlist, gc, x, y, str, len))
OLARG(Display, *dpy)
OLARG(Drawable, drawable)
OLARG(OlFontList,      *fontlist)
OLARG(GC,   gc)
OLARG(int,  x)
OLARG(int,  y)
OLARG(unsigned char,    *str)
OLGRA(int,    len)
{
	return(_OlDraw(dpy, drawable, fontlist, gc, x, y, str, len, False));

} /* end of OlDrawImageString() */

static int
_OlDraw(dpy, drawable, fontlist, gc, x, y, str, len, flag)
Display	*dpy;
Drawable	drawable;
OlFontList	*fontlist;
GC	gc;
int	x, y;
unsigned char	*str;
int	len;
Boolean flag;
{

	static OlStrSegment * parse;
	static int firsttime = 1;
 static int buffer_size = MAX_SIZE;
	GC tmp_GC;
	XGCValues values;
	int new_position;
	unsigned char *tmp;
	int tmp_len = len;
 int l;

	/* validate arguments, return -1, if any of 'em is invalid */
	if ((dpy == NULL) || (fontlist == NULL))
		{
		  OlVaDisplayWarningMsg(dpy,
					OleNfileOlDraw,
					OleTmsg3,
					OleCOlToolkitWarning,
					OleMfileOlDraw_msg3);
		return (-1);
		}

	if ((str == NULL) || (len <= 0))
		{
		  OlVaDisplayWarningMsg(dpy,
					OleNfileOlDraw,
					OleTmsg2,
					OleCOlToolkitWarning,
					OleMfileOlDraw_msg2);
		return (0);	/* return success any way */
		}

	if (firsttime)
	   {
	   firsttime = 0;
	   parse = (OlStrSegment *) XtMalloc(sizeof(OlStrSegment));
	   parse->str = (unsigned char *) XtMalloc(MAX_SIZE);
	   parse->len = 0;
	   }

	/* Optimization Note: we can not simply check things like
	   if current locale is "C" (european languages may use extended 
	   ASCII but may not be in "C" locale), nor can we call a function 
	   getwidth() to examine width of chars. in each code set since 
	   this function uses default values as 1,0,0 for supplementary 
	   code sets even if no code set (other than code set 0) is present.
	   (use of "_multibyte" field is also not possible).
	   The only possible optimization for now, is to check the 'num' 
	   field in the fontlist argument. If it is 1, we assume, that 
	   there is only one code set - essentially code set 0 for 
	   ASCII and call XDrawString() directly.
	*/
	
/*	XCopyGC(dpy, gc, (unsigned long)~0, tmp_GC); */
	if (fontlist->num == 1)
		{
/*		XSetFont(dpy, tmp_GC, fontlist->fontl[0]->fid); */
		XSetFont(dpy, gc, fontlist->fontl[0]->fid); 
		flag ? XDrawString(dpy, drawable, gc, x, y, 
				   (char *)str, len)
		     : XDrawImageString(dpy, drawable, gc, x, y, 
				   (char *)str, len);
		return(0);
		}
	else
		{
		/* parse->str can not be a static storage since the
		   length of the string to be drawn could be different.
		   Allocating a big buffer (e.g 512 bytes) and then realloc()
		   if necessary is the way to optimize in most cases.
		*/
		if (buffer_size <= (l = (strlen((char *)str) + 1)))
			{
			parse->str = (unsigned char *) XtRealloc(
				     (char *)parse->str, l);
			buffer_size = l;
			}
		tmp = str;
	 	while (tmp_len > 0)
		    {
	   	  parse->len = 0;
		     OlGetNextStrSegment(fontlist, parse, &tmp, &tmp_len);
/*		     XSetFont(dpy, tmp_GC, fontlist->fontl[parse->code_set]->fid); */
		     XSetFont(dpy, gc, fontlist->fontl[parse->code_set]->fid);
		     if (fontlist->cswidth[parse->code_set] == 1)
			 {
		         flag ? XDrawString(dpy, drawable, gc, x, y, 
			     (char *)parse->str, parse->len)
		     	  : XDrawImageString(dpy, drawable, gc, x, y, 
			    (char *)parse->str, parse->len);
			 new_position = XTextWidth(
				        fontlist->fontl[parse->code_set],
					(char *)parse->str, parse->len);
			  }
		     else if (fontlist->cswidth[parse->code_set] == 2)
		          {
		          flag ? XDrawString16(dpy, drawable, gc, x, y, 
			                       (XChar2b *)parse->str, parse->len/2)
		     	       : XDrawImageString16(dpy, drawable, gc, x, y, 
			                 (XChar2b *)parse->str, parse->len/2);

			  new_position =  XTextWidth16(
				          fontlist->fontl[parse->code_set],
					  (XChar2b *)parse->str, parse->len/2);
			  }

		     /* this should not happen in most cases, but if a bad
			drawing problem ever occurs, we know what it is 
			(either we had bad character width in parse struct or
			 we are dealing with characters longer than 2 bytes).
		     */
		     else OlVaDisplayWarningMsg(dpy,
						OleNfileOlDraw,
						OleTmsg1,
						OleCOlToolkitWarning,
						OleMfileOlDraw_msg1);

		     /* advance 'x' position by new_position */
		     x += new_position;
		     }
		}
	return (0);

} /* end of _OlDraw() */

extern int
OlGetNextStrSegment OLARGLIST((fontlist, parse, str, len))
OLARG(OlFontList, *fontlist)
OLARG(OlStrSegment, *parse)
OLARG(unsigned char,  **str)
OLGRA(int, *len)
{
	int i = 0;
	int cset = -1;
	unsigned char  *tmp;

	tmp = parse->str;
	parse->len = 0;

	/* some validation */
	if ((fontlist == NULL) || (parse == NULL))
		return (-1);	
	if (**str == NULL)
	   {	
	   parse->str = NULL;
	   return (0);
	   }

	/* determine code set that the first character in string belongs to */
	if (**str == SS3) 
	    cset = 3;
	else if (**str == SS2)
	    cset = 2;
	else if (**str & 0x80)
	    cset = 1;
	else cset = 0;	

	/* parse 'str', discard SS2, SS3 etc. character as appropriate,
	   copy next (multi)byte into parse->str, fix pointers and
	   'len' as appropriate. The 'str' argument may not be NULL padded,
	   so check for 'len' is necessary.
	*/
	while(*len > 0)
	     {

	       /* would not it be nice if we converterd part the following
		  redundant code into a MACRO or a separate function !!
	       */
	      if (**str == SS3)
		 {
		 if (cset != 3)   break;
	         (*str)++;
	         strncpy((char *)tmp, (char *)*str, fontlist->cswidth[3]);
		 if (fontlist->cswidth[3] == 2)
                        {
                        *tmp = *tmp & ~0x80;
                        tmp++;
                        *tmp = *tmp & ~0x80;
                        tmp++;
                        }
                 else tmp += fontlist->cswidth[3];
	         *str += fontlist->cswidth[3];
	         parse->len += fontlist->cswidth[3];
	         *len -= (fontlist->cswidth[3] + 1);
		 }
	      else if (**str == SS2)
		 {
		 if (cset != 2 ) break;
	         (*str)++;
	         strncpy((char *)tmp, (char *)*str, fontlist->cswidth[2]);
		 if (fontlist->cswidth[2] == 2)
                        {
                        *tmp = *tmp & ~0x80;
                        tmp++;
                        *tmp = *tmp & ~0x80;
                        tmp++;
                        }
                 else tmp += fontlist->cswidth[2];
	         *str += fontlist->cswidth[2];
	         parse->len += fontlist->cswidth[2];
	         *len -= (fontlist->cswidth[2] + 1);
		 }
	      else if (**str & 0x80)
		 {
		 if (cset != 1) break;
	         strncpy((char *)tmp, (char *)*str, fontlist->cswidth[1]);
		 if (fontlist->cswidth[1] == 2)
			{
		 	*tmp = *tmp & ~0x80;
		 	tmp++;
		 	*tmp = *tmp & ~0x80;
		 	tmp++;
			}
		 else tmp += fontlist->cswidth[1];
	         *str += fontlist->cswidth[1];
	         parse->len += fontlist->cswidth[1];
	         *len -= fontlist->cswidth[1];
		 }
	      else
		 {
		 if (cset != 0) break;
		 tmp[i] = **str;
		 (*str)++;
		 parse->len += 1;
		 *len -= 1;
		 }
	      i += fontlist->cswidth[cset];
	      }
	parse->str[i] = '\0';
	parse->code_set = cset;
	return (0);

} /* end of OlGetNextStrSegment() */


extern int
OlTextWidth OLARGLIST((fontlist, str, len))
OLARG(OlFontList, *fontlist)
OLARG(unsigned char,  *str)
OLGRA(int,   len)
{
	static OlStrSegment * parse;
	static int firsttime = 1;
	static int buffer_size = MAX_SIZE;
	int text_width = 0;
	unsigned char *tmp;
	int tmp_len = len;
	int l;

	if (str == NULL)
		return(0);

	if (firsttime)
	    {
	    firsttime = 0;
	    parse = (OlStrSegment *) XtMalloc(sizeof(OlStrSegment));
            parse->str = (unsigned char *) XtMalloc(MAX_SIZE);
            parse->len = 0;
	    }

	if (fontlist == NULL)
	   return (-1);

	/* some optimization for now */
	if (fontlist->num == 1)
		return(XTextWidth(fontlist->fontl[0], (char *)str, len));
	else
		{
      	        if (buffer_size < (l = (strlen((char *) str) + 1)))
		     {
		     parse->str = (unsigned char *) XtRealloc((char *)parse->str, l);
		     buffer_size = l;
	 	     }
		tmp = str;
		while (tmp_len > 0)
		    {
		     parse->len = 0;
		     OlGetNextStrSegment(fontlist, parse, &tmp, &tmp_len);
		     if (fontlist->cswidth[parse->code_set] == 1)
			text_width += XTextWidth(
				      fontlist->fontl[parse->code_set],
				      (char *)parse->str, parse->len);
		     else if (fontlist->cswidth[parse->code_set] == 2)
			text_width += XTextWidth16(
				      fontlist->fontl[parse->code_set],
				      (XChar2b *)parse->str, parse->len/2);

		     }
		}
	return (text_width);

} /* end of OlTextWidth() */
