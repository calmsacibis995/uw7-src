#pragma ident	"@(#)m1.2libs:Xm/VendorSEP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */
#ifndef  _XmVendorSEP_h
#define _XmVendorSEP_h

#include <Xm/ShellEP.h>
#include <Xm/MwmUtil.h>

#ifdef __cplusplus
extern "C" {
#endif


#ifndef XmIsVendorShellExt
#define XmIsVendorShellExt(w)	XtIsSubclass(w, xmVendorShellExtObjectClass)
#endif /* XmIsVendorShellExt */

typedef struct _XmVendorShellExtRec *XmVendorShellExtObject;
typedef struct _XmVendorShellExtClassRec *XmVendorShellExtObjectClass;
externalref WidgetClass xmVendorShellExtObjectClass;


#define XmInheritProtocolHandler	((XtCallbackProc)_XtInherit)

typedef struct _XmVendorShellExtClassPart{
    XtCallbackProc	delete_window_handler;
    XtCallbackProc	offset_handler;
    XtPointer		extension;
}XmVendorShellExtClassPart, *XmVendorShellExtClassPartPtr;

typedef struct _XmVendorShellExtClassRec{
    ObjectClassPart		object_class;
    XmExtClassPart		ext_class;
    XmDesktopClassPart 		desktop_class;
    XmShellExtClassPart		shell_class;
    XmVendorShellExtClassPart 	vendor_class;
}XmVendorShellExtClassRec;

typedef struct {
 XmFontList		default_font_list;
 unsigned char		focus_policy;
 XmFocusData		focus_data;
 unsigned char		delete_response;
 unsigned char		unit_type;
 MwmHints		mwm_hints;
 MwmInfo		mwm_info;
 String			mwm_menu;
 XtCallbackList		focus_moved_callback;
 /*
  * internal fields
  */
 Widget			old_managed;
 Position		xAtMap, yAtMap, xOffset, yOffset;
 unsigned long		lastOffsetSerial;
 unsigned long		lastMapRequest;
 Boolean		externalReposition;
 unsigned char		mapStyle;
 XtCallbackList		realize_callback;
 XtGrabKind		grab_kind;
 Boolean		audible_warning;
 XmFontList             button_font_list;
 XmFontList             label_font_list;
 XmFontList             text_font_list;
 String			input_method_string;
 String			preedit_type_string;
 unsigned int           light_threshold;
 unsigned int           dark_threshold;
 unsigned int           foreground_threshold;
 unsigned int		im_height;
 XtPointer		im_info;
 Boolean		im_vs_height_set;
} XmVendorShellExtPart, *XmVendorShellExtPartPtr;

externalref XmVendorShellExtClassRec 	xmVendorShellExtClassRec;

typedef struct _XmVendorShellExtRec{
    ObjectPart			object;
    XmExtPart			ext;
    XmDesktopPart		desktop;
    XmShellExtPart		shell;
    XmVendorShellExtPart 	vendor;
}XmVendorShellExtRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO

extern unsigned char _XmGetAudibleWarning() ;
extern char * _XmGetIconPixmapName() ;
extern void   _XmClearIconPixmapName() ;
#else

extern unsigned char _XmGetAudibleWarning( 
                        Widget w) ;
extern char * _XmGetIconPixmapName( void ) ;
extern void   _XmClearIconPixmapName( void ) ;

#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* _XmVendorSEP_h */
