/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:f_layout.c	1.45.2.9"

/******************************file*header********************************

    Description:
	This file contains the source code related to laying out items in a folder.
*/
						/* #includes go here	*/
#include <ctype.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <Xm/Xm.h>

#include "Dtm.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Public  Procedures 
*/
					/* private procedures		*/
static char *	GetFilePermission(DmItemPtr itemp);
static int ItemLabelsMaxLen(DmItemPtr item, 
			    int count, 
			    Boolean no_iconlabel);

static int colncpy(char *dst, char *src, int n);

					/* public procedures		*/
void		DmComputeLayout(Widget, DmItemPtr, int, int, Dimension,
				DtAttrs, DtAttrs);

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/

/***************************private*procedures****************************

    Private Procedures
*/

/****************************procedure*header*****************************
    GetFilePermission-
*/
static char *
GetFilePermission(DmItemPtr itemp)
{
#define MAX_PERM_CHAR 10
    register int	x;
    DmFileInfoPtr	finfo = FILEINFO_PTR(itemp);
    static char		buf[MAX_PERM_CHAR];
    static const int	perms[] = {
	S_IRUSR,S_IWUSR,S_IXUSR,
	S_IRGRP,S_IWGRP,S_IXGRP,
	S_IROTH,S_IWOTH,S_IXOTH
    };
    static const char permc[] = {'r','w','x', 'r','w','x', 'r','w','x'};

    for (x = 0; x < MAX_PERM_CHAR - 1; x++)
	buf[x] = (finfo->mode & perms[x]) ? permc[x] : '-';

    return(buf);
}					/* end of GetFilePermission */

/***************************private*procedures****************************

    Public Procedures
*/

/*****************************************************************************
 *  	DmComputeLayout
 *	INPUTS:	icon box
 *		item list, count of items
 *		view type (DM_ICONIC, DM_NAME, DM_LONG)
 *		geom_options:
 *			DM_B_CALC_SIZE: compute items sizes
 *		layout_options:
 *			SAVE_ICON_POS: 	copy item (x,y) values to objects 
 *					before calculating new positions.  Used for
 *					.dtinfo file (ICONIC_VIEW)
 *			UPDATE_LABEL: 	create new labels for the items
 *			RESTORE_ICON_POS:
 *					copy item (x,y) from object
 *			
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/
void
DmComputeLayout(Widget icon_box, DmItemPtr itemp, int count, int type,
		Dimension width,
		DtAttrs geom_options,	/* size, icon position options */
		DtAttrs layout_options)	/* layout attributes */
{
    DmItemPtr	item;
    int		maxlen = 0;
    Dimension	margin = XmConvertUnits(icon_box, XmHORIZONTAL, Xm100TH_POINTS, 
					ICON_MARGIN * 100, XmPIXELS);
    Dimension	pad = 	XmConvertUnits(icon_box, XmHORIZONTAL, Xm100TH_POINTS, 
				       INTER_ICON_PAD * 100, XmPIXELS);
    WidePosition	x = margin;
    WidePosition	y = margin;
    WidePosition	center_x;
    Dimension	grid_width;
    Dimension	row_height;
    Dimension	item_width;
    Dimension	font_height; /* used only for ICONIC view */
    Dimension	label_width, label_height; /* used only for ICONIC view */
    Dimension	max_descend; /* used only for ICONIC view */

    WidePosition icon_x, icon_y;

    if (layout_options & SAVE_ICON_POS)
	DmSaveXandY(itemp, count);

    if (width < GRID_WIDTH(Desktop))
	width = GRID_WIDTH(Desktop);

    switch(type)
    {
    case DM_NAME:	
        DmInitSmallIcons(DESKTOP_SHELL(Desktop));

	/* Compute row height and grid width outside of loop */
	row_height = (geom_options & DM_B_CALC_SIZE) ?
	    DM_NameRowHeight(icon_box) + pad : ITEM_HEIGHT(itemp) + pad;
	grid_width = GRID_WIDTH(Desktop);

	for (item = itemp; item < itemp + count; item++)
	{
	    if(!ITEM_MANAGED(item))
		continue;

	    if (layout_options & UPDATE_LABEL)
	    {
		FREE_LABEL(ITEM_LABEL(item));
		MAKE_LABEL(item->label, Dm__MakeItemLabel(item, type, 0));
	    }

	    if (geom_options & DM_B_CALC_SIZE) {
		Dimension icon_width;
		Dimension icon_height;

		DmComputeItemSize(icon_box,item, type, &icon_width, &icon_height);
		item->icon_width = (XtArgVal)icon_width;
		item->icon_height = (XtArgVal)icon_height;
            }
	    item_width = ITEM_WIDTH(item);

	    /* Wrap now if item will extend beyond "wrap" width */
	    if ((x != margin) &&
		((Dimension)(x + item_width) > width))

	    {
		x = margin;
		y += row_height;
	    }

	    item->x = x;
	    item->y = y;

	    x += (item_width <= grid_width) ? grid_width :
		((Dimension)(item_width + grid_width - 1) / grid_width) *
		    grid_width;
	}
    break;

    case DM_ICONIC:
	/* Compute row height and grid width outside of loop */
	row_height = GRID_HEIGHT(Desktop);
	grid_width = GRID_WIDTH(Desktop);
	center_x = x + (grid_width / 2);

	for (item = itemp; item < itemp + count; item++)
	{
	    if(!ITEM_MANAGED(item))
		continue;

	    if (layout_options & UPDATE_LABEL)
	    {
		FREE_LABEL(ITEM_LABEL(item));
		if (type == DM_ICONIC)
		   MAKE_WRAPPED_LABEL(item->label, FPART(icon_box).font,
			DmGetObjectName(ITEM_OBJ(item)));
		else
		   MAKE_LABEL(item->label, Dm__MakeItemLabel(item, type, 0));
	    }

	    if (geom_options & DM_B_CALC_SIZE) {
		Dimension icon_width;
		Dimension icon_height;

		DmComputeItemSize(icon_box,item, type, &icon_width, &icon_height);
		item->icon_width = (XtArgVal)icon_width;
		item->icon_height = (XtArgVal)icon_height;
            }

	    if (layout_options & RESTORE_ICON_POS) {
		if (((ITEM_OBJ(item))->x != 0) ||
		    ((ITEM_OBJ(item))->y != 0)) {
		    item->x = ITEM_OBJ(item)->x;
		    item->y = ITEM_OBJ(item)->y;
		}
	    }
	}

	if (!(layout_options & RESTORE_ICON_POS)) {
		font_height = DM_FontHeight(FPART(icon_box).font);
		max_descend = 0;
	}

	for (item = itemp; item < itemp + count; item++)
	{
	    if(!ITEM_MANAGED(item))
		continue;

	    /* Since icons are moveable in iconic view, x & y are
	       saved/restored when switching views.
	    */
	    if (layout_options & RESTORE_ICON_POS)
	    {
		if (((ITEM_OBJ(item))->x == 0) &&
		    ((ITEM_OBJ(item))->y == 0))
		{
		    DmGetAvailIconPos(icon_box, itemp, count,
			ITEM_WIDTH(item), ITEM_HEIGHT(item),
			width, GRID_WIDTH(Desktop), GRID_HEIGHT(Desktop),
			&icon_x, &icon_y );
	
		    item->x = (XtArgVal)(icon_x);
		    item->y = (XtArgVal)(icon_y);

		} else
		{
		    item->x = ITEM_OBJ(item)->x;
		    item->y = ITEM_OBJ(item)->y;
		}

	    } else
	    {
		WidePosition next_x;

		item_width = ITEM_WIDTH(item);
		DM_TextExtent(icon_box, item, &label_width, &label_height);
again:
		/* Horiz: centered */
		center_x = x + (grid_width / 2);
		next_x = center_x - (item_width / 2);

		/* Wrap now if item will extend beyond "wrap" width */
		if ((x != margin) &&
		    ((Dimension)(x + grid_width) > width))
		{
		    x = margin;
		    y += row_height + max_descend;
		    max_descend = 0; /* reset */
		    goto again;
		}

		if ((label_height - font_height) > max_descend)
			max_descend = label_height - font_height;

		/* Vert: bottom justified */
		item->y = VertWrapJustifyInGrid(y, ITEM_HEIGHT(item),
				row_height, font_height, label_height);
		item->x = next_x;

		x += grid_width;
		center_x += grid_width / 2;		/* next center */
	    }
	}
	break;

    case DM_LONG:
    {
	Dimension icon_width;
	Dimension icon_height;

        DmInitSmallIcons(DESKTOP_SHELL(Desktop));

	/* Compute row height and max label length outside of loop */
	row_height = DM_LongRowHeight(icon_box) + pad;
	maxlen = ItemLabelsMaxLen(itemp, count, (layout_options & NO_ICONLABEL));

	for (item = itemp; item < itemp + count; item++)
	{
	    if (!ITEM_MANAGED(item))
		continue;
	    /* We need to recalculate *all* labels in Long View because
	     * they labels are used for alignment.  The last item added
	     * may have affected the alignment (if it was the longest).
	     */ 

	    FREE_LABEL(ITEM_LABEL(item));
	    MAKE_LABEL(item->label, 
		       DmGetLongName(item, maxlen +3, 
				     (layout_options & NO_ICONLABEL)) );

	    DmComputeItemSize(icon_box,item, DM_LONG, &icon_width, &icon_height);
            item->icon_width = (XtArgVal)icon_width;
            item->icon_height = (XtArgVal)icon_height;

	    item->x = x;
	    item->y = y;

	    /* we always assume one column in long format */
	    y += row_height;
	}
    }
	break;

    default:
	break;
    }
}


/****************************procedure*header*****************************
    colncpy -
*/

#define SS2     0x8e
#define SS3     0x8f

#ifdef __STDC__
#define multibyte (__ctype[520]>1)
#define eucw1 __ctype[514]
#define eucw2 __ctype[515]
#define eucw3 __ctype[516]
#define scrw1 __ctype[517]
#define scrw2 __ctype[518]
#define scrw3 __ctype[519]
#else
#define multibyte (_ctype[520]>1)
#define eucw1 _ctype[514]
#define eucw2 _ctype[515]
#define eucw3 _ctype[516]
#define scrw1 _ctype[517]
#define scrw2 _ctype[518]
#define scrw3 _ctype[519]
#endif


static int
colncpy(char *dst, char *src, int n)
{
     register unsigned char   c;
     register int        scrw;     /* screen width */
     register int        strw;     /* string width */
     register int        cnt = n;
     char           *dststart = dst;

     if (!multibyte)
          return sprintf(dst, "%-*s", n, src);

     while ((c = (unsigned char) *src) != 0 && cnt > 0)
     {
          if (c < 0x80)       /* codeset 0 */
          { *dst++ = *src++; --cnt; continue; }
          else if (c >= 0xa0) /* codeset 1 */
          { strw = eucw1; scrw = scrw1; }
          else if (c == SS2)  /* codeset 2 */
          { strw = (1+eucw2); scrw = scrw2; }
          else if (c == SS3)  /* codeset 3 */
          { strw = (1+eucw3); scrw = scrw3; }
          else
          { *dst++ = *src++; --cnt; continue; }

          if (scrw > cnt)
               break;
          cnt -= scrw;

          while (strw)
          { *dst++ = *src++; --strw; }
     }

     while (cnt > 0)
     { *dst++ = ' '; --cnt; }

     *dst = '\0';

     return (dst - dststart);
}

/****************************procedure*header*****************************
    DmGetLongName-
*/
char *
DmGetLongName(DmItemPtr item, int len, Boolean no_iconlabel)
{
    static char		buffer[512];
    struct passwd *	pw;
    struct group *	gr;
    int			length;
    char		user_buf[512];
    char		group_buf[512];
    char		time_buf[256];
    char *		user_name;
    char *		group_name;

    buffer[0] = '\0';

    /* For user and group names, use the number if no group defined */
    if ( (pw = getpwuid(FILEINFO_PTR(item)->uid)) == NULL )
    {
	sprintf(user_buf, "%d", FILEINFO_PTR(item)->uid);
	user_name = user_buf;

    } else
	user_name = pw->pw_name;

    if ( (gr = getgrgid(FILEINFO_PTR(item)->gid)) == NULL )
    {
	sprintf(group_buf, "%d", FILEINFO_PTR(item)->gid);
	group_name = group_buf;

    } else
	group_name = gr->gr_name;

    (void)strftime(time_buf, sizeof(time_buf), TIME_FORMAT,
	     localtime(&FILEINFO_PTR(item)->mtime));
	length = colncpy(buffer, no_iconlabel ? (ITEM_OBJ(item))->name :
					DmGetObjectName(ITEM_OBJ(item)), len);
	length += sprintf(buffer+length, "%-10s%-10s%-10s%7ld %s",
				GetFilePermission(item),
				user_name,
				group_name,
				FILEINFO_PTR(item)->size, time_buf);

    return(buffer);
}				/* end of DmGetLongName */

void
DmSaveXandY(DmItemPtr item, int count)
{

    DmItemPtr itemp;
    DtAttrs attrs;
    int	i;

    for (i=0, itemp = item; i < count; i++, itemp++)
	if (ITEM_MANAGED(itemp))
	{
	    ITEM_OBJ(itemp)->x = itemp->x;
	    ITEM_OBJ(itemp)->y = itemp->y;

/*
 * Will re-enable this feature once we put in timestamping.
 */
#ifdef NOT_USE
	    if (DtGetProperty(&(ITEM_OBJ(itemp)->plist), OBJCLASS, &attrs)) {
		/*
		 * Don't set property if it is already set and has either
		 * DONTCHG or LOCKED attributes set.
		 */
		if (attrs & (DT_PROP_ATTR_DONTCHANGE | DT_PROP_ATTR_LOCKED))
			continue;
	    }
	    DtSetProperty(&(ITEM_OBJ(itemp)->plist), OBJCLASS, 
		((DmFnameKeyPtr)(FCLASS_PTR(itemp))->key)->name, 0);
#endif
	}
}

/****************************procedure*header*****************************
    DmComputeItemSize-
*/
void
DmComputeItemSize(Widget icon_box, DmItemPtr item, DmViewFormatType view_type,
		    Dimension * width, Dimension * height)
{
    DmFmodeKeyPtr fmkptr = DmFtypeToFmodeKey(ITEM_OBJ(item)->ftype);
    Dimension 	pad = IPART(icon_box).hpad;
    Dimension	shadow_w = PPART(icon_box).shadow_thickness;
    Dimension	highlight_w = PPART(icon_box).highlight_thickness;


#ifdef FDEBUG
    if (Debug > 3){
	XmString xmstr = _XmStringCreateExternal(NULL, item->label);
	char *char_str =  _XmStringGetTextConcat(xmstr);
	fprintf(stderr,"DmComputeItemSize: %s\n", char_str);
	XmStringFree(xmstr);
	XtFree(char_str);
    }
#endif

    switch(view_type)
    {
    case DM_ICONIC :
	/* NOTE: DmSizeIcon will set the dimension in the item instead of
	   just returning the width and height.  There is currently no
	   interface to just get the dimension of an item in ICONIC view.
	*/
	DmSizeIcon(icon_box, item);
	*width = (Dimension)item->icon_width;
	*height = (Dimension)item->icon_height;
	break;

	/* In NAME (SHORT) view, the width is the sum of the
	 *  glyph, horizontal pad, label, plus a single highlight 
	 *  and shadow that surround the glyph and text.  (The
	 *  highlight is drawn by DrawIcon when the item has input
	 *  focus.  The shadow is actually drawn by DrawIcon when the 
	 *  item is selected
	 */
    case DM_NAME :
        DmInitSmallIcons(DESKTOP_SHELL(Desktop));
	*width = fmkptr->small_icon->width + pad + DmTextWidth(icon_box, item) +
	    + 2 * shadow_w + 2 * highlight_w;
	*height = DM_NameRowHeight(icon_box);
	break;

    case DM_LONG :
	/* LONG view width calculation is the same as SHORT view.
	   The difference in width is produced by the different font
	   used in the two views.  The icon_box font is reset when
	   the view type changes.
	*/
        DmInitSmallIcons(DESKTOP_SHELL(Desktop));
	*width = fmkptr->small_icon->width + pad + DmTextWidth(icon_box, item) +
	    + 2 * shadow_w + 2 * highlight_w;
	*height = DM_LongRowHeight(icon_box);
	break;

    default:
	break;
    }
}					/* end of DmComputeItemSize */

/*****************************************************************************
 *  	ItemLabelsMaxLen: calculate the maximum number of characters in a
 *	file-name (or icon_label) so long view can align the file names.
 *	
 *	WARNING!!: This routine deals with char strings stored in the
 *	object name (and object property list); it cannot not deal with
 *	_XmStrings (stored in items).  
 *	INPUTS:
 *	OUTPUTS:
 *	GLOBALS:
 *****************************************************************************/

static int
ItemLabelsMaxLen(DmItemPtr item, int count, Boolean no_iconlabel)
{
    register int	i;
    register int	len;
    register int	max_len;

    max_len = 0;

    for (i = 0; i < count; i++, item++)
	if (ITEM_MANAGED(item) &&
	    (len = strlen(no_iconlabel ? (ITEM_OBJ(item))->name :DmGetObjectName(ITEM_OBJ(item)))) > max_len)
	{
	    max_len = len;
	}

    return(max_len);
}				/* end of ItemLabelsMaxLen() */


