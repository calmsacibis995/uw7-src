#pragma ident	"@(#)m1.2libs:Xm/XpmCrIFData.c	1.1"
/************************************************************************* 
 **  (c) Copyright 1993, 1994 Hewlett-Packard Company
 **  (c) Copyright 1993, 1994 International Business Machines Corp.
 **  (c) Copyright 1993, 1994 Sun Microsystems, Inc.
 **  (c) Copyright 1993, 1994 Novell, Inc.
 *************************************************************************/
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$"
#endif
#endif
/* Copyright 1990-92 GROUPE BULL -- See license conditions in file COPYRIGHT */
/*****************************************************************************\
* XpmCrIFData.c:                                                              *
*                                                                             *
*  XPM library                                                                *
*  Parse an Xpm array and create the image and possibly its mask              *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#include "_xpmI.h"

int
_XmXpmCreateImageFromData(display, data, image_return,
		       shapeimage_return, attributes)
    Display *display;
    char **data;
    XImage **image_return;
    XImage **shapeimage_return;
    XpmAttributes *attributes;
{
    xpmData mdata;
    int ErrorStatus;
    xpmInternAttrib attrib;

    /*
     * initialize return values 
     */
    if (image_return)
	*image_return = NULL;
    if (shapeimage_return)
	*shapeimage_return = NULL;

    _XmxpmOpenArray(data, &mdata);
    _XmxpmInitInternAttrib(&attrib);

    ErrorStatus = _XmxpmParseData(&mdata, &attrib, attributes);

    if (ErrorStatus == XpmSuccess)
	ErrorStatus = _XmxpmCreateImage(display, &attrib, image_return,
				     shapeimage_return, attributes);

    if (ErrorStatus >= 0)
	_XmxpmSetAttributes(&attrib, attributes);
    else if (attributes)
	_XmXpmFreeAttributes(attributes);

    _XmxpmFreeInternAttrib(&attrib);
    _XmXpmDataClose(&mdata);

    return (ErrorStatus);
}
