#ifndef	NOIDENT
#pragma ident	"@(#)libMDtI:sizeicon.c	1.6"
#endif

#include <locale.h>

#include <X11/Xatom.h>
#include "DtI.h"
#include "FIconBoxP.h"	/* to get fontlist */

/*****************************************************************************
 *  	DmSizeIcon: calculate the size of an item (in ICONIC view)
 *	See drawicon.c, and FIconBoxI.c for drawing method.  
 *		SHADOWS: space is maintained around the glyph and the label
 *			 for shadows to indicate item selection
 *		HIGHLIGHT: space is maintained around the glyph to indicate
 *			   input focus on the item
 *		PADDING: a vertical pad is maintained between the glyph
 *			 and label to allow for the link glyph
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
DmSizeIcon(Widget flat, DmItemPtr ip)
{
    Dimension	lbl_w, lbl_h;
    Dimension	glyph_w, glyph_h;
    Dimension 	pad = IPART(flat).vpad;
    Dimension	shadow_thickness = PPART(flat).shadow_thickness;
    Dimension	highlight_thickness = PPART(flat).highlight_thickness;

    /*
     *	Width is the maximum of the space used for the glyph and the label,
     *	including shadows (and a highlight for the glyph).
     *
     *  	glyph_w = glyph_w + 2*shadow_w + 2*highlight_w
     *  	label_w = string_w + 2*shadow_w
     */
    DM_TextExtent(flat, ip, &lbl_w, &lbl_h);
    
    glyph_w = GLYPH_PTR(ip) ? GLYPH_PTR(ip)->width + 2*highlight_thickness : 0;
    ip->icon_width = (XtArgVal) (DM_Max(glyph_w, lbl_w) + 2*shadow_thickness);
				
    
    /*
     * 	Height is the sum of the glyph height, text height, and vertical pad
     *	(used for the link glyph).
     * 
     *  glyph_h = glyph_h + 2*shadow_w + 2*highlight_w
     *	text_h = string_h + 2*shadow_w
     *	
     * (The shadow_thickness is added at the end to simplify calculations)
     */
    glyph_h = GLYPH_PTR(ip) ? GLYPH_PTR(ip)->height + 2*highlight_thickness : 0;
    ip->icon_height = (XtArgVal)(glyph_h + lbl_h + pad + 4 * shadow_thickness);
}				/* end of DmSizeIcon() */

/*****************************************************************************
 *  	DM_TextWidth
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Dimension
DM_TextWidth(Widget flat, DmItemPtr item)
{
    XmFontList font = FPART(flat).font;
    _XmString label = ITEM_LABEL(item);
    
    return(_XmStringWidth(font, label));
} /* end of DM_TextWidth */
/*****************************************************************************
 *  	DM_TextHeight
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Dimension
DM_TextHeight(Widget flat, DmItemPtr item)
{
    XmFontList font = FPART(flat).font;
    _XmString label = ITEM_LABEL(item);

    return(_XmStringHeight(font, label));
} /* end of DM_TextHeight */
/*****************************************************************************
 *  	DM_TextExtent
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
DM_TextExtent(Widget flat, DmItemPtr item, Dimension *width, Dimension *height)
{
    XmFontList font = FPART(flat).font;
    _XmString label = ITEM_LABEL(item);

    _XmStringExtent(font, label, width, height);
} /* end of DM_TextExtent */

/*****************************************************************************
 *  	DM_FontHeight
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Dimension
DM_FontHeight(XmFontList font)
{
    Dimension	width, height;

    DM_FontExtent(font, &width, &height);
    return(height);
} /* end of DM_FontHeight */
/*****************************************************************************
 *  	DM_FontWidth
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
Dimension
DM_FontWidth(XmFontList font)
{
    Dimension	width, height;
    
    DM_FontExtent(font, &width, &height);
    return(width);
} /* end of DM_FontWidth */

/*****************************************************************************
 *  	DM_FontExtent
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
DM_FontExtent(XmFontList font, Dimension *width, Dimension *height)
{
    XFontStruct 	*fs = NULL;
    XmFontContext 	context;
    XmFontListEntry	entry;
    XmFontType		entry_type;
    XtPointer		entry_data;
    XFontStruct		*entry_font;
    XFontSetExtents	*extents;
    Dimension		entry_w, entry_h;


    *width = *height = 0;

    if (!XmFontListInitFontContext(&context, font)){
	/*  We're in trouble with the current fontlist.  
	 *  Just return the geometry for the default font.
	 */
	_XmFontListGetDefaultFont(font, &fs);
	if (fs){
	    *width = fs->max_bounds.width;
	    *height = fs->max_bounds.ascent + fs->max_bounds.descent;
	    return;
	}
    }

    /* Loop through each entry in the fontlist and get its max bounds */

    entry = XmFontListNextEntry(context);
    while (entry){
	entry_data = XmFontListEntryGetFont(entry, &entry_type);
	if (entry_type == XmFONT_IS_FONT){
	    entry_font = (XFontStruct *) entry_data;
	    entry_w = entry_font->max_bounds.width;
	    entry_h = entry_font->max_bounds.ascent + entry_font->max_bounds.descent;
	}
	else{
	    /* entry_type == XmFONT_IS_FONTSET */
	    extents = XExtentsOfFontSet((XFontSet) entry_data);
	    entry_w = extents->max_logical_extent.width;
	    entry_h = extents->max_logical_extent.height;
	}

	if (entry_w > *width)
	    *width = entry_w;
	if (entry_h > *height)
	    *height = entry_h;
	
	/* Get next entry */
	entry = XmFontListNextEntry(context);
    }
    return ;
} /* end of DM_FontExtent */


/*****************************************************************************
 *  	DmCompareXmStrings - compare strings (i18n), return -1, 0, 1
 *	if str1 is lexicographically less than, equal to, or greater
 *	than str2.  See below for special cases.
 *	INPUTS:  two _XmStrings, str1, str2
 *
 *	OUTPUTS: 	str1 		str2 		output	
 *			========================================
 *			NULL		NULL		0
 *			NULL		non-NULL	-1
 *			non-NULL	NULL		+1	
 *			non-NULL	non-NULL	
 *				
 *	GLOBALS:
 *****************************************************************************/
int
DmCompareXmStrings(XmString str1, XmString str2)
{
    int retval;
    char *char_str1, *char_str2;
    

    /* Handle NULL values */
    
    if (!str1 || ((char_str1 = _XmStringGetTextConcat(str1)) == NULL)){
	/* str1 == NULL */
	if (!str2 || ((char_str2 = _XmStringGetTextConcat(str2)) == NULL)){
	    /* str1 == str2 == NULL */
	    retval = 0;
	}
	else{
	    /* str1 == NULL, str2 != NULL */
	    XtFree(char_str2);
	    retval = -1;
	}
    }
    else{
	/* str1 != NULL */
	if (!str2 || ((char_str2 = _XmStringGetTextConcat(str2)) == NULL)){
	    /* str1 != NULL, str2 == NULL */
	    XtFree(char_str1);
	    retval = 1;
	}
	else{
	    /* str1 != NULL, str2 != NULL */
	    /* strcmp assumes C locale, use strcoll for non-C locale */
	    if (strcmp(setlocale(LC_COLLATE, NULL), "C"))
		retval = strcoll(char_str1, char_str2);
	    else
		retval = strcmp(char_str1, char_str2);
#ifdef DEBUG
	    fprintf(stderr,"DmCompareXmStrings: %s %s %s\n", 
		    char_str1 ? char_str1 : "NULL", 
		    retval == -1 ? "<" : retval == 1 ? ">" : "=",
		    char_str2 ? char_str2 : "NULL");
#endif
	    XtFree(char_str1);
	    XtFree(char_str2);
	}
    }
    return(retval);
} /* end of DmCompareXmStrings */



/*****************************************************************************
 *  	DmCompareItemLabels - compare item labels (i18n), return -1, 0, 1
 *	if item_label1 is lexicographically less than, equal to, or greater
 *	than item_label2  See below for special cases.
 *	INPUTS:  two items, item1 item2
 *
 *	OUTPUTS: 	item1 		item2 		output	
 *			========================================
 *			NULL		NULL		0
 *			NULL		non-NULL	-1
 *			non-NULL	NULL		+1	
 *			non-NULL	non-NULL	

 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
int 
DmCompareItemLabels(DmItemPtr item1, DmItemPtr item2)
{
    XmString xmstr1, xmstr2;
    XmFontList	font = NULL;	/* FLH MORE: not used by XmStringCreateExternal*/
    int retval;

    /* convert from _XmString to XmString */

    xmstr1 = _XmStringCreateExternal(font, (_XmString) ITEM_LABEL(item1));
    xmstr2 = _XmStringCreateExternal(font, (_XmString) ITEM_LABEL(item2));
    retval = DmCompareXmStrings(xmstr1, xmstr2);
    XmStringFree(xmstr1);
    XmStringFree(xmstr2);
    return(retval);
} 	/* end of DmCompareItemLabels */
