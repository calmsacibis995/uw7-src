#ifndef NOIDENT
#ident	"@(#)olmisc:xpm.h	1.1"
#endif

#ifndef _xpm_h
#define _xpm_h

/* Copyright 1989 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/**************************************************************************
 ****  Constant used in xpm library functions :
 ****
 ****  int XWritePixmapFile(dpy, cmap, filename, pixmap, w, h)
 ****  int XReadPixmapFile(dpy, d, cmap, filename, w_return, h_return,
 ****                                                     depth, pixmap_return)
 ****  Pixmap XCreatePixmapFromData(dpy, d, cmap, w, h, depth, n, c, col, pix)
 ****
 ****  Daniel Dardailler - Bull RC (89/02/22) e-mail: daniel@mirsa.inria.fr
 **************************************************************************/

/*
 * From arpa!mirsa.inria.fr!Daniel.Dardailler Wed Nov 15 16:27:25 +0100 1989
 * Date: Wed, 15 Nov 89 16:27:25 +0100
 * From: Daniel Dardailler <Daniel.Dardailler@mirsa.inria.fr>
 * To: jmb@attunix.att.com
 * Subject: XPM
 * 
 * What is really meaned by this copyright is that:
 * 
 * Like the MIT distribution of the X Window System,it is publicly available,
 * but is NOT in the public domain (my fault).
 * The difference is that copyrights granting rights
 * for unrestricted use and redistribution have been placed on all of the
 * software to identify its authors.
 * You are allowed and encouraged to take
 * this software and build commercial products.
 * 
 * GROUPE BULL will let you use XPM as long as you do not pretend to have
 * written it.
 * 
 * You are also encouraged to re-distribute it freely with the same behaviour.
 * bye
 * 
 *    Daniel Dardailler                   |      Email : daniel@mirsa.inria.fr
 *    BULL  Centre de Sophia Antipolis    |      Phone : (33) 93 65 77 71
 *          2004, Route des Lucioles      |      Telex :      97 00 50 F
 *          06565 Valbonne CEDEX  France  |      Fax   : (33) 93 65 77 66
 */


#define XPM_FORMAT 1

/* we keep the same codes as for Bitmap management */
#ifndef _XUTIL_H_
#include <X11/Xutil.h>
#endif
#define PixmapSuccess        BitmapSuccess 
#define PixmapOpenFailed     BitmapOpenFailed
#define PixmapFileInvalid    BitmapFileInvalid
#define PixmapNoMemory       BitmapNoMemory


typedef struct _Cixel {
     char c1,c2 ;
   } Cixel ;                    /* 2 chars for one pixel value */

typedef struct _CmapEntry {
     Cixel cixel ;               
     unsigned short red,green,blue ;
   } CmapEntry ;                    


#define MAXPRINTABLE 93             /* number of printable ascii chars 
				       minus \ and " for string compat. */

static char * printable = " .XoO+@#$%&*=-;:?>,<1234567890qwertyuipasdfghjklzxcvbnmMNBVCZASDFGHJKLPIUYTREWQ!~^/()_`'][{}|" ;
           /* printable begin with a space, so in most case, due to
	      my algorythm, when the number of different colors is
	      less than MAXPRINTABLE, it will give a char follow by 
	      "nothing" (a space) in the readable xpm file */


#endif /* _xpm_h */
