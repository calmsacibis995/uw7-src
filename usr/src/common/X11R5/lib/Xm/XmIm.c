#pragma ident	"@(#)m1.2libs:Xm/XmIm.c	1.9"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
ifndef OSF_v1_2_4
 * Motif Release 1.2.3
else
 * Motif Release 1.2.4
endif
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */


#include "XmI.h"
#include <Xm/BaseClassP.h>
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <Xm/VendorSEP.h>
#include <Xm/VendorSP.h>
#include <Xm/DrawP.h>
#include <Xm/DisplayP.h>
#include "MessagesI.h"
#include <stdio.h>


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef _NO_PROTO
# include <varargs.h>
# define Va_start(a,b) va_start(a)
#else
# include <stdarg.h>
# define Va_start(a,b) va_start(a,b)
#endif


#ifdef IC_PER_SHELL
typedef struct _XmICStruct {
    struct _XmICStruct *next;
    Widget icw;
    Window focus_window;
    XtArgVal foreground;
    XtArgVal background;
    XtArgVal background_pixmap;
    XtArgVal font_list;
    XtArgVal line_space;
    int status_width;
    int status_height;
    int preedit_width;
    int preedit_height;
    Boolean has_focus;
    Boolean need_reset;
} XmICStruct;
#else
typedef struct _XmICStruct {
    struct _XmICStruct *next;
    Widget icw;
    XIC xic;
    Window focus_window;
    XIMStyle input_style;
    int status_width;
    int preedit_width;
    int sp_height;
    Boolean has_focus;
} XmICStruct;
#endif /* IC_PER_SHELL */

#ifdef IC_PER_SHELL
#define NO_ARG_VAL -1
#endif /* IC_PER_SHELL */

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static XIM get_xim() ;
static int add_sp() ;
static int add_p() ;
static int add_fs() ;
static int add_bgpxmp() ;
static XIMStyle check_style() ;
static int ImGetGeo() ;
static void ImSetGeo() ;
static void ImGeoReq() ;
static XFontSet extract_fontset() ;
static void remove_icstruct() ;
static XmICStruct * get_icstruct() ;
static XmICStruct * get_iclist() ;
static void draw_separator() ;
static void null_proc() ;
static void ImCountVaList() ;
static ArgList ImCreateArgList() ;
#ifndef IC_PER_SHELL
static void get_geom();
static void set_geom();
#endif /* IC_PER_SHELL */

#else

static XIM get_xim( 
                        Widget p) ;
static int add_sp( 
                        String name,
                        XtArgVal value,
                        ArgList *slp,
                        ArgList *plp,
                        ArgList *vlp) ;
static int add_p( 
                        String name,
                        XtArgVal value,
                        ArgList *slp,
                        ArgList *plp,
                        ArgList *vlp) ;
static int add_fs( 
                        String name,
                        XtArgVal value,
                        ArgList *slp,
                        ArgList *plp,
                        ArgList *vlp) ;
static int add_bgpxmp( 
                        String name,
                        XtArgVal value,
                        ArgList *slp,
                        ArgList *plp,
                        ArgList *vlp) ;
static XIMStyle check_style( 
			XIMStyles *styles,
                        XIMStyle preedit_style,
                        XIMStyle status_style) ;
static int ImGetGeo( 
                        Widget vw) ;
static void ImSetGeo( 
                        Widget vw) ;
static void ImGeoReq( 
                        Widget vw) ;
static XFontSet extract_fontset( 
                        XmFontList fl) ;
static void remove_icstruct( 
                        Widget w) ;
static XmICStruct * get_icstruct( 
                        Widget w) ;
static XmICStruct * get_iclist( 
                        Widget w) ;
static void draw_separator( 
                        Widget vw) ;
static void null_proc( 
                        Widget w,
                        XtPointer ptr,
                        XEvent *ev,
                        Boolean *bool) ;
static void ImCountVaList( 
                        va_list var,
                        int *total_count) ;
static ArgList ImCreateArgList( 
                        va_list var,
                        int total_count) ;
#ifndef IC_PER_SHELL
static void get_geom(Widget vw,
		     XmICStruct *icp);
static void set_geom(Widget vw,
		     XmICStruct *icp);
#endif /* IC_PER_SHELL */

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


#ifdef _NO_PROTO
typedef int (*XmImResLProc)() ;
#else
typedef int (*XmImResLProc)( String, XtArgVal, ArgList*, ArgList*, ArgList*) ;
#endif

typedef struct {
    String xmstring;
    String xstring;
    XrmName xrmname;
#ifdef IC_PER_SHELL
    Cardinal offset;
#endif /* IC_PER_SHELL */
    XmImResLProc proc;
} XmImResListStruct;

#ifdef IC_PER_SHELL
typedef XmICStruct * XmICStructPtr;

#define OFFSET(field) XtOffset(XmICStructPtr, field)

static XmImResListStruct XmImResList[] = {
    /*
     * backgroundPixmap needs to be processed right before background,
     * so that background can be reset in the case of XtUnspecifiedPixmap.
     */
    {XmNbackgroundPixmap, XNBackgroundPixmap, NULLQUARK,
     OFFSET(background_pixmap), add_bgpxmp},
    {XmNbackground, XNBackground, NULLQUARK, OFFSET(background), add_sp},
    {XmNforeground, XNForeground, NULLQUARK, OFFSET(foreground), add_sp},
    {XmNspotLocation, XNSpotLocation, NULLQUARK, 0, add_p},
    {XmNfontList, XNFontSet, NULLQUARK, OFFSET(font_list), add_fs},
    {XmNlineSpace, XNLineSpace, NULLQUARK, OFFSET(line_space), add_sp}
};
#else
static XmImResListStruct XmImResList[] = {
    {XmNbackground, XNBackground, NULLQUARK, add_sp},
    {XmNforeground, XNForeground, NULLQUARK, add_sp},
    {XmNbackgroundPixmap, XNBackgroundPixmap, NULLQUARK, add_bgpxmp},
    {XmNspotLocation, XNSpotLocation, NULLQUARK, add_p},
    {XmNfontList, XNFontSet, NULLQUARK, add_fs},
    {XmNlineSpace, XNLineSpace, NULLQUARK, add_sp}
};
#endif /* IC_PER_SHELL */

typedef struct {
    XIM xim;
    XIMStyles *styles;
} _XIMStruct;

#define MAXARGS 10
static Arg xic_vlist[MAXARGS];
static Arg status_vlist[MAXARGS];
static Arg preedit_vlist[MAXARGS];

#define OVERTHESPOT "overthespot"
#define OFFTHESPOT "offthespot"
#define ROOT "root"
#define MAXSTYLES 3
#define SEPARATOR_HEIGHT 2

#define GEO_CHG 0x1
#define BG_CHG 0x2
#ifdef IC_PER_SHELL
#define BGPXMP_CHG 0x4
#endif /* IC_PER_SHELL */

#ifdef IC_PER_SHELL
typedef struct {
    Widget im_widget;
    XIMStyle input_style;
    XIC xic;
    int status_width;
    int status_height;
    int preedit_width;
    int preedit_height;
    XmICStruct *iclist;
    XmICStruct *current;
} XmImInfo;
#else
typedef struct {
    Widget im_widget;
    XmICStruct *iclist;
    Widget current_widget;
} XmImInfo;
#endif

#ifdef I18N_MSG
#define MSG1	catgets(Xm_catd,MS_IM,MSG_IM_1,_XmMsgXmIm_0000)
#else
#define MSG1	_XmMsgXmIm_0000
#endif


void 
#ifdef _NO_PROTO
XmImRegister( w, reserved )
        Widget w ;
	unsigned int reserved ;
#else
XmImRegister(
        Widget w,
	unsigned int reserved )
#endif /* _NO_PROTO */
{
    Widget p;
    register XmICStruct *icp;
    register XmICStruct *bcp = NULL;
    XmICStruct *curic;
    XIMStyle input_style = 0;
    char tmp[BUFSIZ];
    char *cp, *tp, *cpend;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;

    XmDisplay	xmDisplay;
    _XIMStruct *xim_struct;
    register XIMStyles *styles;

    p = XtParent(w);

    while (!XtIsShell(p))
	p = XtParent(p);

    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);

    /* check extension data since app could be attempting to create
     * a text widget as child of menu shell. This is illegal, and will
     * be detected later, but check here so we don't core dump.
     */
    if (extData == NULL)
	return;

    ve = (XmVendorShellExtObject) extData->widget;

    if (get_xim(p) == NULL)
	return;

    if (ve->vendor.im_info == NULL)
    {
#ifndef IC_PER_SHELL
	if ((im_info = (XmImInfo *)XtMalloc(sizeof(XmImInfo))) == NULL)
		return;

	im_info->im_widget = NULL;
	im_info->iclist = NULL;
	im_info->current_widget = NULL;
	ve->vendor.im_info = (XtPointer)im_info;
    }
    else
	im_info = (XmImInfo *)ve->vendor.im_info;

    if (im_info->iclist == NULL)
    {
	im_info->iclist = (XmICStruct *)XtMalloc(sizeof(XmICStruct));
	curic = im_info->iclist;
    }
    else
    {
	icp = im_info->iclist;
	while (icp != NULL)
	{
	    if (icp->icw == w)		/* im widget already registered */
		return;
	    bcp = icp;
	    icp = icp->next;
	}
	bcp->next = (XmICStruct *)XtMalloc(sizeof(XmICStruct));
	curic = bcp->next;
    }

    if (curic == NULL)			/* malloc failed */
	return;

    curic->icw = w;
    curic->xic = 0;
    curic->focus_window = 0;
    curic->status_width = 0;
    curic->preedit_width = 0;
    curic->sp_height = 0;
    curic->has_focus = False;
    curic->next = NULL;

#endif /* IC_PER_SHELL */
    /* Now determine the input style to be used for this XIC */

    xmDisplay = (XmDisplay) XmGetXmDisplay(XtDisplay(p));
    xim_struct = (_XIMStruct *)xmDisplay->display.xmim_info;
    styles = xim_struct->styles;

    input_style = 0;
    XtVaGetValues(p,XmNpreeditType,&cp,NULL);
    if (cp != NULL)
    {
	/* parse for the successive commas */

	cp = strcpy(tmp,cp);
	cpend = &tmp[strlen(tmp)];
	while(cp < cpend)
	{
	    tp = strchr(cp,',');
	    if (tp)
		*tp = 0;
	    else
		tp = cpend;

	    if (_XmStringsAreEqual(cp,OVERTHESPOT))
	    {
		if ((input_style = check_style(styles, XIMPreeditPosition,
			XIMStatusArea|XIMStatusNothing|XIMStatusNone)) != 0)
		    break;
	    }
	    else if (_XmStringsAreEqual(cp,OFFTHESPOT))
	    {
		if ((input_style = check_style(styles, XIMPreeditArea,
                        XIMStatusArea|XIMStatusNothing|XIMStatusNone)) != 0)
		    break;
	    }
	    else if (_XmStringsAreEqual(cp,ROOT))
	    {
		if ((input_style = check_style(styles, XIMPreeditNothing,
			XIMStatusNothing|XIMStatusNone)) != 0)
		    break;
	    }
	    cp = tp+1;
	}
    }
    if (input_style == 0)
    {
	if ((input_style = check_style(styles, XIMPreeditNone, 
		XIMStatusNone)) == 0)
	{
	    /* no input style supported - will use XLookupString.
             * remove from list
	     */ 
#ifndef IC_PER_SHELL
	    if (curic == im_info->iclist)
		im_info->iclist = NULL;
	    else
		bcp->next = NULL;

	    XtFree((char *)curic);
#endif /* IC_PER_SHELL */
	    return;
	}
    }

#ifdef IC_PER_SHELL
	if ((im_info = (XmImInfo *)XtMalloc(sizeof(XmImInfo))) == NULL)
		return;
#else
    curic->input_style = input_style;
#endif /* IC_PER_SHELL */

	/* We need to create this widget whenever there is a non-simple
	 * input method in order to stop the intrinsics from calling
	 * XMapSubwindows, thereby improperly mapping input method
	 * windows which have been made children of the client or
	 * focus windows.
	 */

#ifdef IC_PER_SHELL
	if (input_style & (XIMStatusArea | XIMPreeditArea | XIMPreeditPosition))
	    im_info->im_widget = XtVaCreateWidget("xmim_wrapper",
			coreWidgetClass, p, XmNwidth, 10, XmNheight, 10, NULL);
	else
	    im_info->im_widget = NULL;
	im_info->input_style = input_style;
	im_info->xic = NULL;
	im_info->status_width = 0;
	im_info->status_height = 0;
	im_info->preedit_width = 0;
	im_info->preedit_height = 0;
	im_info->iclist = NULL;
	im_info->current = NULL;
	ve->vendor.im_info = (XtPointer)im_info;
    }
    else
	im_info = (XmImInfo *)ve->vendor.im_info;

    if (im_info->iclist == NULL)
    {
	im_info->iclist = (XmICStruct *)XtMalloc(sizeof(XmICStruct));
	curic = im_info->iclist;
    }
    else
    {
	icp = im_info->iclist;
	while (icp != NULL)
	{
	    if (icp->icw == w)		/* im widget already registered */
		return;
	    bcp = icp;
	    icp = icp->next;
	}
	bcp->next = (XmICStruct *)XtMalloc(sizeof(XmICStruct));
	curic = bcp->next;
    }

    if (curic == NULL)			/* malloc failed */
	return;

    curic->icw = w;
    curic->focus_window = 0;
    curic->foreground = NO_ARG_VAL;
    curic->background = NO_ARG_VAL;
    curic->background_pixmap = NO_ARG_VAL;
    curic->font_list = NO_ARG_VAL;
    curic->line_space = NO_ARG_VAL;
    curic->status_width = 0;
    curic->status_height = 0;
    curic->preedit_width = 0;
    curic->preedit_height = 0;
    curic->has_focus = False;
    curic->need_reset = False;
    curic->next = NULL;
#else
    if ((im_info->im_widget == NULL) &&
	(input_style & (XIMStatusArea | XIMPreeditArea | XIMPreeditPosition)))
	im_info->im_widget = XtVaCreateWidget("xmim_wrapper", coreWidgetClass,
				 p, XmNwidth, 10, XmNheight, 10, NULL);
#endif /* IC_PER_SHELL */
}


void 
#ifdef _NO_PROTO
XmImUnregister( w )
        Widget w ;
#else
XmImUnregister(
        Widget w )
#endif /* _NO_PROTO */
{
    register XmICStruct *icp;

    if ((icp = get_icstruct(w)) == NULL)
	return;

#ifndef IC_PER_SHELL
    if (icp->xic)
	XDestroyIC(icp->xic);
#endif
    remove_icstruct(w);
}


#ifdef HP_MOTIF
void
#ifdef _NO_PROTO
_XHP_XmImCloseIM( w )
        Widget w ;
#else
_XHP_XmImCloseIM(
        Widget w )
#endif /* _NO_PROTO */
{
    XmDisplay	xmDisplay;
    _XIMStruct *xim_struct;
    XmVendorShellExtObject ve;
    XmWidgetExtData     extData;
    int height, base_height;
    Arg               args[1];
    XtWidgetGeometry    my_request;

    extData = _XmGetWidgetExtData((Widget)w, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    height =ve->vendor.im_height;

    xmDisplay = (XmDisplay) XmGetXmDisplay(XtDisplay(w));
    xim_struct = (_XIMStruct *)xmDisplay->display.xmim_info;
    XFree(xim_struct->styles);
    if (height != 0){
       XtSetArg(args[0], XtNbaseHeight, &base_height);
       XtGetValues(w, args, 1);
       base_height -= height;
       if (base_height < 0)
          base_height = 0;
       XtSetArg(args[0], XtNbaseHeight, base_height);
       XtSetValues(w, args, 1);
       if(!(XtIsRealized(w)))
          w->core.height -= height;
       else {
          my_request.height = w->core.height - height;
          my_request.request_mode = CWHeight;
          XtMakeGeometryRequest(w, &my_request, NULL);
       }
       ve->vendor.im_height = 0;
    }
    XCloseIM(xim_struct->xim);
    XtFree((XtPointer)xim_struct);
    xmDisplay->display.xmim_info = NULL;
}
#endif /* HP_MOTIF */

void 
#ifdef _NO_PROTO
XmImSetFocusValues( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal num_args ;
#else
XmImSetFocusValues(
        Widget w,
        ArgList args,
        Cardinal num_args )
#endif /* _NO_PROTO */
{
    register XmICStruct *icp;
    Widget p;
    Pixel bg;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;

#ifdef IC_PER_SHELL
    p = w;
    while (!XtIsShell(p))
	p = XtParent(p);
 
    if ((icp = get_icstruct(w)) == NULL)
	return;
 
    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    im_info = (XmImInfo *)ve->vendor.im_info;
 
    if (icp->focus_window == (Window) NULL)
    {
	icp->focus_window = XtWindow(w);
    }
    icp->has_focus = True;
    icp->need_reset = True;
    im_info->current = icp;
    XmImSetValues(w, args, num_args);
    icp->need_reset = False;
    XSetICFocus(im_info->xic);
    if (ve->vendor.im_height)
    {
	XtVaGetValues(w, XmNbackground, &bg, NULL);
	XtVaSetValues(p, XmNbackground, bg, NULL);
	draw_separator(p);
    }
#else
    XmImSetValues(w, args, num_args);
 
    p = w;
    while (!XtIsShell(p))
	p = XtParent(p);
 
    if ((icp = get_icstruct(w)) == NULL)
	return;
 
    if (icp->focus_window == (Window) NULL)
    {
	XSetICValues(icp->xic, XNFocusWindow, XtWindow(w), NULL);
	icp->focus_window = XtWindow(w);
    }
 
    XSetICFocus(icp->xic);
 
    icp->has_focus = True;
    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
 
    if (ve->vendor.im_height)
    {
	im_info = (XmImInfo *)ve->vendor.im_info;
	im_info->current_widget = icp->icw;
	XtVaGetValues(w, XmNbackground, &bg, NULL);
	XtVaSetValues(p, XmNbackground, bg, NULL);
	draw_separator(p);
    }
#endif
}


void 
#ifdef _NO_PROTO
XmImSetValues( w, args, num_args )
        Widget w ;
        ArgList args ;
        Cardinal num_args ;
#else
XmImSetValues(
        Widget w,
        ArgList args,
        Cardinal num_args )
#endif /* _NO_PROTO */
{
    register XmICStruct *icp;
    XmImResListStruct *rlp;
    register int i, j;
    register ArgList argp = args;
    ArgList tslp = &status_vlist[0];
    ArgList tplp = &preedit_vlist[0];
    ArgList tvlp = &xic_vlist[0];
#ifdef IC_PER_SHELL
    XrmName names[MAXARGS];
    XtArgVal arg_val;
    XtArgVal reset_arg_val;
    XtArgVal *val_addr;
#else
    XrmName name;
#endif /* IC_PER_SHELL */
    Widget p;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;
    int flags = 0;
#ifdef IC_PER_SHELL
    int reset_flags = 0;
#endif /* IC_PER_SHELL */
    Pixel bg;
    char *ret;
    unsigned long mask;
    Boolean unrecognized = False;
#ifdef IBM_MOTIF
    XRectangle rect;
#endif /* IBM_MOTIF */

    p = w;
    while (!XtIsShell(p))
	p = XtParent(p);

    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);
    if ( extData == NULL )
	return;

    ve = (XmVendorShellExtObject) extData->widget;

    if ((icp = get_icstruct(w)) == NULL)
	return;

    im_info = (XmImInfo *)ve->vendor.im_info;

    if (!XtIsRealized(p))
    {
	/* if vendor widget not realized, then the current info
	 * is that for the last widget to set values.
	 */

#ifdef IC_PER_SHELL
	im_info->current = icp;
#else
	im_info->current_widget = icp->icw;
#endif /* IC_PER_SHELL */
    }

    for (i = num_args; i > 0; i--, argp++)
    {
#ifdef IC_PER_SHELL
	names[i] = XrmStringToName(argp->name);
#else
	name = XrmStringToName(argp->name);
#endif /* IC_PER_SHELL */

	for (rlp = XmImResList, j = sizeof(XmImResList)/
			sizeof(XmImResListStruct); j != 0; j--, rlp++)
	{
#ifdef IC_PER_SHELL
	    if (rlp->xrmname == names[i])
            {
#else
	    if (rlp->xrmname == name)
            {
		flags |= (*rlp->proc) (rlp->xstring, argp->value, &tslp, 
								&tplp, &tvlp);
#endif /* IC_PER_SHELL */
		break;
            }
	}
	if (j == 0)		/* simply pass unrecognized values along */
	{
	    tvlp->name = argp->name;
	    tvlp->value = argp->value;
	    tvlp++;
	    unrecognized = True;
        }
    }

#ifdef IC_PER_SHELL
    for (rlp = XmImResList, j = sizeof(XmImResList)/
			sizeof(XmImResListStruct); j != 0; j--, rlp++)
    {
	arg_val = NO_ARG_VAL;
	for (i = num_args, argp = args; i > 0; i--, argp++)
	{
	    if (rlp->xrmname == names[i])
	    {
		arg_val = argp->value;
		break;
	    }
	}

	reset_arg_val = NO_ARG_VAL;
	if (rlp->offset)
        {
	    val_addr = (XtArgVal *)((char *)icp + rlp->offset);
	    if (arg_val == NO_ARG_VAL)
	    {
		/* if no new value, use old value in XmICStruct,
		 * when the widget has become the current one.
		 */
		if (icp->need_reset == True ||
			flags & BGPXMP_CHG)
		{
		    reset_arg_val = *val_addr;
		    flags &= ~BGPXMP_CHG;
		}
	    }
	    else
	    {
		/* if new value, store it in XmICStruct.
		 */
		*val_addr = arg_val;
	    }
	}

	if (arg_val != NO_ARG_VAL)
	{
	    flags |= (*rlp->proc) (rlp->xstring, arg_val, &tslp,
							&tplp, &tvlp);
        }
	else if (reset_arg_val != NO_ARG_VAL)
	{
	    reset_flags |= (*rlp->proc) (rlp->xstring, reset_arg_val, &tslp,
							&tplp, &tvlp);
	}

    }
#endif /* IC_PER_SHELL */

    tslp->name = NULL;
    tslp->value = (XtArgVal) NULL;
    tplp->name = NULL;
    tplp->value = (XtArgVal) NULL;

    tvlp->name = XNStatusAttributes;
    tvlp->value = (XtArgVal)&status_vlist[0];
    tvlp++;
    tvlp->name = XNPreeditAttributes;
    tvlp->value = (XtArgVal)&preedit_vlist[0];
    tvlp++;
    tvlp->name = NULL;
    tvlp->value = (XtArgVal) NULL;

    /* we do not create the IC until the initial data is ready to be passed */

#ifdef IC_PER_SHELL
    if ((get_xim(p) != NULL) && (im_info->xic == NULL))
#else
    if ((get_xim(p) != NULL) && (icp->xic == NULL))
#endif /* IC_PER_SHELL */
    {
	if (XtIsRealized(p))
	{
	    XSync(XtDisplay(p), False);
	    tvlp->name = XNClientWindow;
	    tvlp->value = (XtArgVal)XtWindow(p);
	    tvlp++;
	}
	if (icp->focus_window)
	{
	    tvlp->name = XNFocusWindow;
	    tvlp->value = (XtArgVal)icp->focus_window;
	    tvlp++;
#ifdef IBM_MOTIF
#ifdef IC_PER_SHELL
	    if(im_info->input_style & XIMPreeditPosition)
#else
	    if(icp->input_style & XIMPreeditPosition)
#endif
	    {
		rect.x = 0;
		rect.y = 0;
		rect.width = (unsigned short)icp->icw->core.width;
		rect.height = (unsigned short)icp->icw->core.height;
		tplp->name = XNArea;
		tplp->value = &rect;
		tplp++;
		tplp->name = NULL;
		tplp->value = NULL;
	    }
#endif /* IBM_MOTIF */
	}
	tvlp->name = XNInputStyle;
#ifdef IC_PER_SHELL
	tvlp->value = (XtArgVal)im_info->input_style;
#else
	tvlp->value = (XtArgVal)icp->input_style;
#endif /* IC_PER_SHELL */
	tvlp++;
	tvlp->name = NULL;
	tvlp->value = (XtArgVal) NULL;
#ifdef IC_PER_SHELL
	im_info->xic = XCreateIC(get_xim(p), XNVaNestedList, &xic_vlist[0],
									NULL);
	if (im_info->xic == NULL)
#else
	icp->xic = XCreateIC(get_xim(p), XNVaNestedList, &xic_vlist[0], NULL);
	if (icp->xic == NULL)
#endif /* IC_PER_SHELL */
	{
	    remove_icstruct(w);
	    return;
	}
#ifdef IC_PER_SHELL
	XGetICValues(im_info->xic, XNFilterEvents, &mask, NULL);
#else
	XGetICValues(icp->xic, XNFilterEvents, &mask, NULL);
#endif /* IC_PER_SHELL */
	if (mask)
	{
	    XtAddEventHandler(p, (EventMask)mask, False, null_proc, NULL);
	}
	if (XtIsRealized(p))
	{
#ifdef IC_PER_SHELL
	    im_info->current = icp;
#endif
 	    if (XmIsDialogShell(p)) {
 	      int i;
 	      for (i = 0; 
 		   i < ((CompositeWidget)p)->composite.num_children; 
 		   i++)
 		if (XtIsManaged(((CompositeWidget)p)->composite.children[i])) {
 		  ImGeoReq(p);
 		  break;
 		}
 	    } else
 	      ImGeoReq(p);
#ifndef IC_PER_SHELL
	    im_info->current_widget = icp->icw;
#endif
	}
    }
#ifdef IC_PER_SHELL
    else if (icp == im_info->current)
    {
      if (icp->need_reset)
	{
	    tvlp->name = XNFocusWindow;
	    tvlp->value = (XtArgVal)icp->focus_window;
	    tvlp++;
	    tvlp->name = NULL;
	    tvlp->value = NULL;
#ifdef IBM_MOTIF
	    if(im_info->input_style & XIMPreeditPosition)
	    {
		rect.x = 0;
		rect.y = 0;
		rect.width = (unsigned short)icp->icw->core.width;
		rect.height = (unsigned short)icp->icw->core.height;
		tplp->name = XNArea;
		tplp->value = &rect;
		tplp++;
		tplp->name = NULL;
		tplp->value = NULL;
	    }
#endif /* IBM_MOTIF */
	}

	ret = XSetICValues(im_info->xic, XNVaNestedList, &xic_vlist[0], NULL);
#else
    else
    {
	ret = XSetICValues(icp->xic, XNVaNestedList, &xic_vlist[0], NULL);
#endif /* IC_PER_SHELL */
	if (ret != NULL && !unrecognized)
	{
	    XVaNestedList slist, plist;
	    unsigned long status_bg, status_fg;
	    unsigned long preedit_bg, preedit_fg;

	    /* We do this in case an input method does not support
	     * change of some value, but does allow it to be set on
	     * create.  If however the value is not one of the
	     * standard values, this im may not support it so we
	     * should ignore it.
	     */

#ifdef IC_PER_SHELL
	    XGetICValues(im_info->xic, 
#else
	    XGetICValues(icp->xic, 
#endif /* IC_PER_SHELL */
			XNStatusAttributes, slist = XVaCreateNestedList(0, 
					XNBackground, &status_bg,
					XNForeground, &status_fg, NULL),
			XNPreeditAttributes, plist = XVaCreateNestedList(0, 
					XNBackground, &preedit_bg,
					XNForeground, &preedit_fg, NULL),
			NULL);
	    XFree(slist);
	    XFree(plist);
#ifdef IC_PER_SHELL
	    XDestroyIC(im_info->xic);
#else
	    XDestroyIC(icp->xic);
#endif /* IC_PER_SHELL */

	    tslp->name = XNBackground;
	    tslp->value = (XtArgVal)status_bg;
	    tslp++;
	    tslp->name = XNForeground;
	    tslp->value = (XtArgVal)status_fg;
	    tslp++;
	    tslp->name = NULL;

	    tplp->name = XNBackground;
	    tplp->value = (XtArgVal)preedit_bg;
	    tplp++;
	    tplp->name = XNForeground;
	    tplp->value = (XtArgVal)preedit_fg;
	    tplp++;
	    tplp->name = NULL;

	    if (XtIsRealized(p))
	    {
		XSync(XtDisplay(p), False);
		tvlp->name = XNClientWindow;
		tvlp->value = (XtArgVal)XtWindow(p);
		tvlp++;
	    }
#ifdef IC_PER_SHELL
	    if (icp->focus_window && !icp->need_reset)
#else
	    if (icp->focus_window)
#endif /* IC_PER_SHELL */
	    {
		tvlp->name = XNFocusWindow;
		tvlp->value = (XtArgVal)icp->focus_window;
		tvlp++;
#ifdef IBM_MOTIF
#ifdef IC_PER_SHELL
		if(im_info->input_style & XIMPreeditPosition)
#else
		if(icp->input_style & XIMPreeditPosition)
#endif
		{
		    rect.x = 0;
		    rect.y = 0;
		    rect.width = (unsigned short)icp->icw->core.width;
		    rect.height = (unsigned short)icp->icw->core.height;
		    tplp->name = XNArea;
		    tplp->value = &rect;
		    tplp++;
		    tplp->name = NULL;
		    tplp->value = NULL;
		}
#endif /* IBM_MOTIF */
	    }
	    tvlp->name = XNInputStyle;
#ifdef IC_PER_SHELL
	    tvlp->value = (XtArgVal)im_info->input_style;
#else
	    tvlp->value = (XtArgVal)icp->input_style;
#endif /* IC_PER_SHELL */
	    tvlp++;
	    tvlp->name = NULL;
	    tvlp->value = (XtArgVal) NULL;
#ifdef IC_PER_SHELL
	    im_info->xic = XCreateIC(get_xim(p),XNVaNestedList,&xic_vlist[0],
									NULL);
	    if (im_info->xic == NULL)
#else
	    icp->xic = XCreateIC(get_xim(p),XNVaNestedList,&xic_vlist[0],NULL);
	    if (icp->xic == NULL)
#endif /* IC_PER_SHELL */
	    {
		remove_icstruct(w);
		return;
	    }
	    ImGeoReq(p);
	    if (icp->has_focus == True)
#ifdef IC_PER_SHELL
		XSetICFocus(im_info->xic);
#else
		XSetICFocus(icp->xic);
#endif /* IC_PER_SHELL */
	    return;
	}
#ifdef IC_PER_SHELL
	if (flags & GEO_CHG || ret != 0)
#else
	if (flags & GEO_CHG)
#endif /* IC_PER_SHELL */
	{
#ifdef IC_PER_SHELL
	    icp->status_height = 0;
	    icp->preedit_height = 0;
#endif /* IC_PER_SHELL */
	    ImGeoReq(p);
    	    if (icp->has_focus == True)
#ifdef IC_PER_SHELL
		XSetICFocus(im_info->xic);
#else
		XSetICFocus(icp->xic);
#endif
	}
    }
#ifdef IC_PER_SHELL
    else
    {
	if (flags & GEO_CHG)
	{
	    icp->status_height = 0;
	    icp->preedit_height = 0;
	    ImGeoReq(p);
	}
    }
#endif /* IC_PER_SHELL

    /* Since we do not know whether a set values may have been done
     * on top shadow or bottom shadow (used for the separator), we
     * will redraw the separator in order to keep the visuals in sync
     * with the current text widget. Also repaint background if needed.
     */

#ifdef IC_PER_SHELL
    if (im_info->current == icp &&
#else
    if (im_info->current_widget == icp->icw &&
#endif /* IC_PER_SHELL */
	flags & BG_CHG)
    {
	XtVaGetValues(w, XmNbackground, &bg, NULL);
	XtVaSetValues(p, XmNbackground, bg, NULL);
    }
}


void 
#ifdef _NO_PROTO
XmImUnsetFocus( w )
        Widget w ;
#else
XmImUnsetFocus(
        Widget w )
#endif /* _NO_PROTO */
{
    register XmICStruct *icp;
#ifdef IC_PER_SHELL
    Widget p;
    XmVendorShellExtObject ve;
    XmWidgetExtData extData;
    XmImInfo *im_info;
#endif /* IC_PER_SHELL */

    if ((icp = get_icstruct(w)) == NULL)
	return;

#ifdef IC_PER_SHELL
    p = w;
    while (!XtIsShell(p))
        p = XtParent(p);

    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    im_info = (XmImInfo *)ve->vendor.im_info;

    if (im_info->xic)
	XUnsetICFocus(im_info->xic);
    icp->focus_window = 0;
#else
    if (icp->xic)
	XUnsetICFocus(icp->xic);
#endif /* IC_PER_SHELL */
    icp->has_focus = False;
}


XIM 
#ifdef _NO_PROTO
XmImGetXIM(w)
	Widget w;
#else
XmImGetXIM( 
	Widget w)
#endif /* _NO_PROTO */
{
    Widget p;

    p = w;
    while (!XtIsShell(p))
	p = XtParent(p);

    return get_xim(p);
}


int 
#ifdef _NO_PROTO
XmImMbLookupString( w, event, buf, nbytes, keysym, status )
        Widget w ;
        XKeyPressedEvent *event ;
        char *buf ;
        int nbytes ;
        KeySym *keysym ;
        int *status ;
#else
XmImMbLookupString(
        Widget w,
        XKeyPressedEvent *event,
        char *buf,
        int nbytes,
        KeySym *keysym,
        int *status )
#endif /* _NO_PROTO */
{
    register XmICStruct *icp;
#ifdef IC_PER_SHELL
    Widget p;
    XmVendorShellExtObject ve;
    XmWidgetExtData     extData;
    XmImInfo *im_info;

    p = w;
    while (!XtIsShell(p))
        p = XtParent(p);

    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    im_info = (XmImInfo *)ve->vendor.im_info;

    if ((icp = get_icstruct(w)) == NULL  ||  im_info->xic == NULL)
#else
    if ((icp = get_icstruct(w)) == NULL  ||  icp->xic == NULL)
#endif /* IC_PER_SHELL */
    {
	if (status)
	    *status = XLookupBoth;
	return XLookupString(event, buf, nbytes, keysym, 0);
    }

#ifdef IC_PER_SHELL
    return XmbLookupString( im_info->xic, event, buf, nbytes, keysym, status );
#else
    return XmbLookupString( icp->xic, event, buf, nbytes, keysym, status );
#endif /* IC_PER_SHELL */
}

/* Private Functions */

void 
#ifdef _NO_PROTO
_XmImChangeManaged( vw )
        Widget vw ;
#else
_XmImChangeManaged(
        Widget vw )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    register int height, old_height;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;

    old_height = ve->vendor.im_height;

    height = ImGetGeo(vw);
    if (!ve->vendor.im_vs_height_set) {
      Arg args[1];
        int base_height;
        XtSetArg(args[0], XtNbaseHeight, &base_height);
        XtGetValues(vw, args, 1);
        if (base_height < 0) 
           base_height = 0;
	if (base_height > 0) {
           base_height += (height - old_height);
           XtSetArg(args[0], XtNbaseHeight, base_height);
           XtSetValues(vw, args, 1);
        }
        vw->core.height += (height - old_height);
      }


#ifdef NON_OSF_FIX
    else{
        ShellWidget sw = (ShellWidget)vw;
        Widget childwid;
        int i;
        int y;

        y = sw->core.height - ve->vendor.im_height;
        for(i = 0; i < sw->composite.num_children; i++) {
            if(XtIsManaged(sw->composite.children[i])) {
                childwid = sw->composite.children[i];
                XtResizeWidget(childwid, sw->core.width, y,
                               childwid->core.border_width);
            }
        }
    }
#endif /* NON_OSF_FIX */
}


void
#ifdef _NO_PROTO
_XmImRealize( vw )
        Widget vw ;
#else
_XmImRealize(
        Widget vw )
#endif /* _NO_PROTO */
{
    XmICStruct *icp;
    Pixel bg;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    im_info = (XmImInfo *)ve->vendor.im_info;

    if ( (icp = get_iclist(vw)) == NULL )
	return;

    /* We need to synchronize here to make sure the server has created
     * the client window before the input server attempts to reparent
     * any windows to it
     */

    XSync(XtDisplay(vw), False);
#ifdef IC_PER_SHELL
    XSetICValues(im_info->xic, XNClientWindow, 
			XtWindow(vw), NULL);
#else
    for (; icp != NULL; icp = icp->next)
    {
      if (!icp->xic)
	continue;
	XSetICValues(icp->xic, XNClientWindow, 
			    XtWindow(vw), NULL);
    }
#endif /* IC_PER_SHELL */

    if (ve->vendor.im_height == 0) {
       ShellWidget shell = (ShellWidget)(vw);
       Boolean resize = shell->shell.allow_shell_resize;

       if (!resize) shell->shell.allow_shell_resize = True;
       ImGeoReq(vw);
       if (!resize) shell->shell.allow_shell_resize = False;
    } else 
       ImSetGeo(vw);
 
    /* For some reason we need to wait till now before we set the 
     * initial background pixmap.
     */

#ifdef IC_PER_SHELL
    if (ve->vendor.im_height && im_info->current)
    {
	icp = im_info->current;
	XtVaGetValues(icp->icw, XmNbackground, &bg, NULL);
#else
    if (ve->vendor.im_height && im_info->current_widget)
    {
	XtVaGetValues(im_info->current_widget, XmNbackground, &bg, NULL);
#endif /* IC_PER_SHELL */
	XtVaSetValues(vw, XmNbackground, bg, NULL);
    }
}

void
#ifdef _NO_PROTO
_XmImResize( vw )
        Widget vw ;
#else
_XmImResize(
        Widget vw )
#endif /* _NO_PROTO */
{
    ImGetGeo(vw);
    ImSetGeo(vw);
}

void
#ifdef _NO_PROTO
_XmImRedisplay( vw )
        Widget vw ;
#else
_XmImRedisplay(
        Widget vw )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;

    if ((extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION)) == NULL)
	return;

    ve = (XmVendorShellExtObject) extData->widget;

    if (ve->vendor.im_height == 0)
	return;

    draw_separator(vw);

}

void
#ifdef _NO_PROTO
_XmImFreeData( im_info)
        XtPointer im_info ;
#else
_XmImFreeData(
        XtPointer im_info)
#endif /* _NO_PROTO */
{
  XmICStruct *icl = ((XmImInfo *)im_info)->iclist ;
  while(    icl != NULL    )
    {
      XmICStruct *icp = icl ;
      icl = icp->next ;
      XtFree( (char *)icp) ;
    }
  XtFree( im_info) ;
}


/* Begin static functions */


static XIM 
#ifdef _NO_PROTO
get_xim(p)
	Widget p;
#else
get_xim( 
	Widget p)
#endif /* _NO_PROTO */
{
    XmDisplay	xmDisplay;
    char tmp[BUFSIZ];
    char *cp;
    XmImResListStruct *rlp;
    register int i;
    _XIMStruct *xim_struct;
    String name, w_class;

    xmDisplay = (XmDisplay) XmGetXmDisplay(XtDisplay(p));
    xim_struct = (_XIMStruct *)xmDisplay->display.xmim_info;

    if (xim_struct == NULL)
    {
	xim_struct = (_XIMStruct *)XtMalloc(sizeof (_XIMStruct));

	if (xim_struct == NULL)
	    return NULL;

	xmDisplay->display.xmim_info = (XtPointer)xim_struct;

	XtVaGetValues(p,XmNinputMethod,&cp,NULL);
	if (cp != NULL)
	{
#ifdef IBM_MOTIF
	    /* Allow an old style string to work
	     */
	    if (!strncmp(cp, "@im=", 4))
		strcpy(tmp, "");
	    else
#endif /* IBM_MOTIF */
		strcpy(tmp,"@im=");
	    strcat(tmp,cp);
	    XSetLocaleModifiers(tmp);
	}

	XtGetApplicationNameAndClass(XtDisplay(p), &name, &w_class);

	xim_struct->xim = XOpenIM(XtDisplay(p), XtDatabase(XtDisplay(p)), 
								name, w_class);
	xim_struct->styles = NULL;
	if (xim_struct->xim == NULL)
	{
#ifdef XOPENIM_WARNING
	    _XmWarning ((Widget)p, MSG1);
#endif
	    return NULL;
	}

	if (XGetIMValues(xim_struct->xim, 
			XNQueryInputStyle, &xim_struct->styles, NULL) != NULL)
	{
	    XCloseIM(xim_struct->xim);
	    xim_struct->xim = NULL;
	    _XmWarning ((Widget)p, MSG1);
	    return NULL;
	}

	/* initialize the list of xrm names */

	for (rlp = XmImResList, i = sizeof(XmImResList)/
			sizeof(XmImResListStruct); i != 0; i--, rlp++)
	{
	    rlp->xrmname = XrmStringToName(rlp->xmstring);
	}

    }
    return xim_struct->xim;
}

static int 
#ifdef _NO_PROTO
add_sp( name, value, slp, plp, vlp )
        String name ;
        XtArgVal value ;
        ArgList *slp ;
        ArgList *plp ;
        ArgList *vlp ;
#else
add_sp(
        String name,
        XtArgVal value,
        ArgList *slp,
        ArgList *plp,
        ArgList *vlp )
#endif /* _NO_PROTO */
{
    register ArgList tp;

    tp = *slp;
    tp->value = value;
    tp->name = name;
    tp++;
    *slp = tp;

    tp = *plp;
    tp->value = value;
    tp->name = name;
    tp++;
    *plp = tp;

    return BG_CHG;
}

static int 
#ifdef _NO_PROTO
add_p( name, value, slp, plp, vlp )
        String name ;
        XtArgVal value ;
        ArgList *slp ;
        ArgList *plp ;
        ArgList *vlp ;
#else
add_p(
        String name,
        XtArgVal value,
        ArgList *slp,
        ArgList *plp,
        ArgList *vlp )
#endif /* _NO_PROTO */
{
    register ArgList tp;

    tp = *plp;
    tp->value = value;
    tp->name = name;
    tp++;
    *plp = tp;

    return 0;
}

static int 
#ifdef _NO_PROTO
add_fs( name, value, slp, plp, vlp )
        String name ;
        XtArgVal value ;
        ArgList *slp ;
        ArgList *plp ;
        ArgList *vlp ;
#else
add_fs(
        String name,
        XtArgVal value,
        ArgList *slp,
        ArgList *plp,
        ArgList *vlp )
#endif /* _NO_PROTO */
{
    register ArgList tp;
    XFontSet fs;

#ifdef IC_PER_SHELL
    if ( value == NO_ARG_VAL ||
	(fs = extract_fontset((XmFontList)value)) == NULL)
#else
    if ( (fs = extract_fontset((XmFontList)value)) == NULL)
#endif /* IC_PER_SHELL */
	return 0;

    tp = *slp;
    tp->value = (XtArgVal)fs;
    tp->name = name;
    tp++;
    *slp = tp;

    tp = *plp;
    tp->value = (XtArgVal)fs;
    tp->name = name;
    tp++;
    *plp = tp;

    return GEO_CHG;
}

static int 
#ifdef _NO_PROTO
add_bgpxmp( name, value, slp, plp, vlp )
        String name ;
        XtArgVal value ;
        ArgList *slp ;
        ArgList *plp ;
        ArgList *vlp ;
#else
add_bgpxmp(
        String name,
        XtArgVal value,
        ArgList *slp,
        ArgList *plp,
        ArgList *vlp )
#endif /* _NO_PROTO */
{
    if ( (Pixmap)value == XtUnspecifiedPixmap )
#ifdef IC_PER_SHELL
	return BGPXMP_CHG;
#else
	return 0;
#endif /* IC_PER_SHELL */

    return add_sp( name, value, slp, plp, vlp );
}

static XIMStyle 
#ifdef _NO_PROTO
check_style( styles, preedit_style, status_style )
	XIMStyles *styles;
        XIMStyle preedit_style ;
        XIMStyle status_style ;
#else
check_style(
	XIMStyles *styles,
        XIMStyle preedit_style,
        XIMStyle status_style )
#endif /* _NO_PROTO */
{
    register int i;

    for (i=0; i < styles->count_styles; i++)
    {
	if ((styles->supported_styles[i] & preedit_style) &&
	    (styles->supported_styles[i] & status_style))
	    return styles->supported_styles[i];
    }
    return 0;
}

#ifndef IC_PER_SHELL
static void
#ifdef _NO_PROTO
get_geom(vw, icp)
     Widget vw;
     XmICStruct *icp;
#else
get_geom(Widget vw,
	 XmICStruct *icp)
#endif
{
  XRectangle rect;
  XRectangle *rp;
  
  if (!icp->xic)
    return;

  if (icp->input_style & XIMStatusArea)
    {
      xic_vlist[0].value = (XtArgVal)&rect;
      rect.width = vw->core.width;
      rect.height = 0;
      
      XSetICValues(icp->xic, 
		   XNStatusAttributes, &xic_vlist[0], 
		   NULL);
      
      xic_vlist[0].value = (XtArgVal)&rp;
      XGetICValues(icp->xic, 
		   XNStatusAttributes, &xic_vlist[0], 
		   NULL);
      
      icp->status_width = MIN(rp->width, vw->core.width);
      icp->sp_height = rp->height;
      XFree(rp);
    }
    if (icp->input_style &  XIMPreeditArea)
      {
        xic_vlist[0].value = (XtArgVal)&rect;
        rect.width = vw->core.width;
        rect.height = 0;
        
        XSetICValues(icp->xic, 
  		   XNPreeditAttributes, &xic_vlist[0], 
  		   NULL);
        
        xic_vlist[0].value = (XtArgVal)&rp;
        XGetICValues(icp->xic, 
  		   XNPreeditAttributes, &xic_vlist[0], 
  		   NULL);
        
        icp->preedit_width = MIN(rp->width, vw->core.width - icp->status_width);
        if (icp->sp_height < rp->height)
  	icp->sp_height = rp->height;
        XFree(rp);
      }
}
#endif /* IC_PER_SHELL */

#ifdef IC_PER_SHELL
static int 
#ifdef _NO_PROTO
ImGetGeo( vw )
        Widget vw ;
#else
ImGetGeo(
        Widget vw )
#endif /* _NO_PROTO */
{
    XmICStruct *icp;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;
    int width = 0;
    int height = 0;
    XRectangle rect;
    XRectangle *rp;
    int old_height;
    Arg args[1];
    int base_height;
    XFontSet fs;
    XFontSet fss = NULL;
    XFontSet fsp = NULL;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;

    if ( (icp = get_iclist(vw)) == NULL )
    {
	ve->vendor.im_height = 0;
	return 0;
    }

    im_info = (XmImInfo *)ve->vendor.im_info;
    if (im_info->xic == NULL)
    {
	ve->vendor.im_height = 0;
	return 0;
    }

    status_vlist[0].name = XNFontSet;
    status_vlist[1].name = NULL;
    preedit_vlist[0].name = XNFontSet;
    preedit_vlist[1].name = NULL;

    xic_vlist[0].name = XNAreaNeeded;
    xic_vlist[1].name = NULL;

    im_info->status_width = 0;
    im_info->status_height = 0;
    im_info->preedit_width = 0;
    im_info->preedit_height = 0;
    for (; icp != NULL; icp = icp->next)
    {
	if (im_info->input_style & XIMStatusArea)
	{
	    if (icp->status_height == 0)
	    {
		if (icp->font_list == NO_ARG_VAL ||
		    (fss = extract_fontset((XmFontList)icp->font_list)) == NULL)
		    continue;

		status_vlist[0].value = (XtArgVal)fss;
		XSetICValues(im_info->xic,
		    XNStatusAttributes, &status_vlist[0],
		    NULL);

		xic_vlist[0].value = (XtArgVal)&rp;
		XGetICValues(im_info->xic, 
		    XNStatusAttributes, &xic_vlist[0], 
		    NULL);

	        icp->status_width = rp->width;
	        icp->status_height = rp->height;
	        XFree(rp);
	    }

	    if (icp->status_width > im_info->status_width)
		im_info->status_width = icp->status_width;
	    if (icp->status_height > im_info->status_height)
		im_info->status_height = icp->status_height;
	}
	if (im_info->input_style & XIMPreeditArea)
	{
	    if (icp->preedit_height == 0)
	    {
		if (icp->font_list == NO_ARG_VAL ||
		    (fsp = extract_fontset((XmFontList)icp->font_list)) == NULL)
		    continue;

		preedit_vlist[0].value = (XtArgVal)fsp;
		XSetICValues(im_info->xic,
		    XNPreeditAttributes, &preedit_vlist[0],
		    NULL);

		xic_vlist[0].value = (XtArgVal)&rp;
		XGetICValues(im_info->xic, 
		    XNPreeditAttributes, &xic_vlist[0], 
		    NULL);

		icp->preedit_width = rp->width;
		icp->preedit_height = rp->height;
		XFree(rp);
	    }

	    if (icp->preedit_width > im_info->preedit_width)
		im_info->preedit_width = icp->preedit_width;
	    if (icp->preedit_height > im_info->preedit_height)
		im_info->preedit_height = icp->preedit_height;
	}
    }

    if (im_info->current != NULL && (fss != NULL || fsp != NULL))
    {
	if(im_info->current->font_list != NO_ARG_VAL &&
	    (fs = extract_fontset((XmFontList)im_info->current->font_list))
		!= NULL)
	{
	    if (fss != NULL)
		status_vlist[0].value = (XtArgVal)fs;
	    else
		status_vlist[0].name = NULL;
	    if (fsp != NULL)
		preedit_vlist[0].value = (XtArgVal)fs;
	    else
		preedit_vlist[0].name = NULL;
	    XSetICValues(im_info->xic,
		XNStatusAttributes, &status_vlist[0],
		XNPreeditAttributes, &preedit_vlist[0],
		NULL);
	}
    }

    if (im_info->status_height > im_info->preedit_height)
	height = im_info->status_height;
    else
	height = im_info->preedit_height;
    old_height = ve->vendor.im_height;
    if (height)
	height += SEPARATOR_HEIGHT;

    ve->vendor.im_height = height;

    XtSetArg(args[0], XtNbaseHeight, &base_height);
    XtGetValues(vw, args, 1);
#ifdef NO_NEED
    base_height += (height - old_height);
    if (!XtIsRealized(vw)){
       vw->core.height += (height - old_height);
    }
#endif /* NO_NEED */
    if (base_height < 0)
       base_height = 0;
    XtSetArg(args[0], XtNbaseHeight, base_height);
    XtSetValues(vw, args, 1);

    return height;
}
#else  /* OSF VERSION */
static int 
#ifdef _NO_PROTO
ImGetGeo( vw )
        Widget vw ;
#else
ImGetGeo(
        Widget vw )
#endif /* _NO_PROTO */
{
    XmICStruct *icp;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    int height = 0;

    if ( (icp = get_iclist(vw)) == NULL )
	return 0;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;

    xic_vlist[0].name = XNAreaNeeded;
    xic_vlist[1].name = NULL;

    for (; icp != NULL; icp = icp->next)
    {
      get_geom(vw, icp);
      if (icp->sp_height > height)
	height = icp->sp_height;
    }

    if (height)
	height += SEPARATOR_HEIGHT;

    ve->vendor.im_height = height;
    return height;
}
#endif /* IC_PER_SHELL */

#ifndef IC_PER_SHELL
static void 
#ifdef _NO_PROTO
set_geom(vw, icp)
     Widget vw;
     XmICStruct *icp;
#else
set_geom(Widget vw,
	 XmICStruct *icp)
#endif
{
    ArgList tslp;
    ArgList tplp;
    XRectangle rect_status;
    XRectangle rect_preedit;

    if (!icp->xic)
      return;

    if (!(icp->input_style & XIMPreeditArea ||
	  icp->input_style & XIMStatusArea))
      return;

    tslp = &status_vlist[0];
    if (icp->input_style & XIMStatusArea)
      {
	rect_status.x = 0;
	rect_status.y = vw->core.height - icp->sp_height;
	rect_status.width = icp->status_width;
	rect_status.height = icp->sp_height;
	
	tslp->name = XNArea;
	tslp->value = (XtArgVal)&rect_status;
	tslp++;
      }
    tslp->name = NULL;
    
    tplp = &preedit_vlist[0];
    if (icp->input_style & XIMPreeditArea)
      {
	rect_preedit.x = icp->status_width;
	rect_preedit.y = vw->core.height - icp->sp_height;
	rect_preedit.width = icp->preedit_width;
	rect_preedit.height = icp->sp_height;
	
	tplp->name = XNArea;
	tplp->value = (XtArgVal)&rect_preedit;
	tplp++;
      }
    tplp->name = NULL;
    
    XSetICValues(icp->xic, XNStatusAttributes, &status_vlist[0], 
		 XNPreeditAttributes, &preedit_vlist[0], 
		 NULL);
}
#endif /* IC_PER_SHELL */

#ifdef IC_PER_SHELL
static void 
#ifdef _NO_PROTO
ImSetGeo( vw )
        Widget vw ;
#else
ImSetGeo(
        Widget vw )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    ArgList tslp = &status_vlist[0];
    ArgList tplp = &preedit_vlist[0];
    register XmICStruct *icp;
    XRectangle rect_status;
    XRectangle rect_preedit;
    XmImInfo *im_info;

    if ( (icp = get_iclist(vw)) == NULL)
	return;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    im_info = (XmImInfo *)ve->vendor.im_info;
    if (im_info->xic == NULL)
	return;

    if (im_info->status_height != 0)
    {
	rect_status.x = 0;
	rect_status.y = vw->core.height - im_info->status_height;
	rect_status.width = MIN(im_info->status_width, vw->core.width);
	rect_status.height = im_info->status_height;

	tslp->name = XNArea;
	tslp->value = (XtArgVal)&rect_status;
	tslp++;
    }
    else if (im_info->input_style & XIMStatusArea)
    {
	rect_status.x = vw->core.width;
	rect_status.y = vw->core.height;
	rect_status.width = 1;
	rect_status.height = 1;

	tslp->name = XNArea;
	tslp->value = (XtArgVal)&rect_status;
	tslp++;
    }
    else
    {
	rect_status.width = 0;
    }
    tslp->name = NULL;

    if (im_info->preedit_height != 0)
    {
	rect_preedit.x = rect_status.width;
	rect_preedit.y = vw->core.height - im_info->preedit_height;
	rect_preedit.width = vw->core.width - rect_status.width;
        if(rect_preedit.width == 0){
            rect_preedit.width++;
            rect_status.width--;
        }
	rect_preedit.height = im_info->preedit_height;

	tplp->name = XNArea;
	tplp->value = (XtArgVal)&rect_preedit;
	tplp++;
    }
    else if (im_info->input_style & XIMPreeditArea)
    {
	rect_preedit.x = vw->core.width;
	rect_preedit.y = vw->core.height;
	rect_preedit.width = 1;
	rect_preedit.height = 1;

	tplp->name = XNArea;
	tplp->value = (XtArgVal)&rect_status;
	tplp++;
    }
    else if (im_info->input_style & XIMPreeditPosition &&
        im_info->current != NULL && im_info->current->icw != NULL)
    {
	rect_preedit.x = 0;
	rect_preedit.y = 0;
	rect_preedit.width = im_info->current->icw->core.width;
	rect_preedit.height = im_info->current->icw->core.height;

	tplp->name = XNArea;
	tplp->value = (XtArgVal)&rect_preedit;
	tplp++;
    }
    tplp->name = NULL;

    if (tslp != &status_vlist[0] || tplp != &preedit_vlist[0])
    XSetICValues(im_info->xic, XNStatusAttributes, &status_vlist[0], 
			XNPreeditAttributes, &preedit_vlist[0], 
			NULL);
}
#else /* OSF VERSION */
static void 
#ifdef _NO_PROTO
ImSetGeo( vw )
        Widget vw ;
#else
ImSetGeo(
        Widget vw )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    register XmICStruct *icp;

    if ( (icp = get_iclist(vw)) == NULL)
	return;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;

    if (ve->vendor.im_height == 0)
	return;

    for (; icp != NULL; icp = icp->next)
    {
      set_geom(vw, icp);
    }
}
#endif /* IC_PER_SHELL */


static void 
#ifdef _NO_PROTO
ImGeoReq( vw )
        Widget vw ;
#else
ImGeoReq(
        Widget vw )
#endif /* _NO_PROTO */
{
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XtWidgetGeometry 	my_request;
    int old_height;
    int delta_height;
    ShellWidget 	shell = (ShellWidget)(vw);

#if defined(IC_PER_SHELL)
    /* We may not need change, before realized.
     */

    if (!XtIsRealized(vw))
	return;

    /* allow_shell_resize should be ignored for the following cases
     *     register -> realize -> unregister
     *     realize -> register -> resize
     */
     /*
     if (!(shell->shell.allow_shell_resize) && XtIsRealized(vw))
	return;
     */
#else
    if (!(shell->shell.allow_shell_resize) && XtIsRealized(vw))
	return;
#endif /* IC_PER_SHELL */

    extData = _XmGetWidgetExtData(vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;

    old_height = ve->vendor.im_height;
    ImGetGeo(vw);
    if ((delta_height = ve->vendor.im_height - old_height) != 0)
    {
        int base_height;
        Arg args[1];
        XtSetArg(args[0], XtNbaseHeight, &base_height);
        XtGetValues(vw, args, 1);
        if (base_height > 0) {
           base_height += delta_height;
           XtSetArg(args[0], XtNbaseHeight, base_height);
           XtSetValues(vw, args, 1);
        }
	my_request.height = vw->core.height + delta_height;
	my_request.request_mode = CWHeight;
	XtMakeGeometryRequest(vw, &my_request, NULL);
    }
    ImSetGeo(vw);
}


static XFontSet 
#ifdef _NO_PROTO
extract_fontset( fl )
        XmFontList fl ;
#else
extract_fontset(
        XmFontList fl )
#endif /* _NO_PROTO */
{
    XmFontContext context;
    XmFontListEntry next_entry;
    XmFontType type_return;
    XtPointer tmp_font;
    XFontSet first_fs = NULL;
    char *font_tag;

    if (!XmFontListInitFontContext(&context, fl))
       return NULL;

    do {
	next_entry = XmFontListNextEntry(context);
	if (next_entry)
	{
	    tmp_font = XmFontListEntryGetFont(next_entry, &type_return);
	    if (type_return == XmFONT_IS_FONTSET)
	    {
		font_tag = XmFontListEntryGetTag(next_entry);
		if (!strcmp(font_tag, XmFONTLIST_DEFAULT_TAG))
		{
		    XmFontListFreeFontContext(context);
		    return (XFontSet)tmp_font;
		}
		if (first_fs == NULL)
		    first_fs = (XFontSet)tmp_font;
	    }
#ifdef HP_MOTIF
	    else
	    {
		if (first_fs == NULL)
		    first_fs = (XFontSet)_XGetFontAssociateFontSet(
					    ((XFontStruct *)tmp_font)->fid);
	    }
#endif
	}
    } while (next_entry);

    XmFontListFreeFontContext(context);
    return first_fs;
}

static void
#ifdef _NO_PROTO
remove_icstruct( w )
	Widget w;
#else
remove_icstruct(
	Widget w )
#endif /* _NO_PROTO */
{
    register XmICStruct *icp, *bicp;
    Widget p;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;

    p = w;
    while (!XtIsShell(p))
	p = XtParent(p);

    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    if ((im_info = (XmImInfo *)ve->vendor.im_info) == NULL)
	return;
    icp = im_info->iclist;
    if (icp == NULL)
	return;

    bicp = NULL;
    while (icp != NULL && icp->icw != w) 
    {
	bicp = icp;
	icp = icp->next;
    }

#ifdef IC_PER_SHELL
    if (bicp == NULL && icp->next == NULL)
    {
	/* if it's the last one, destroy IC.
	 */
	if (im_info->xic)
	{
	    XDestroyIC(im_info->xic);
	    im_info->xic = NULL;
	}
	im_info->current = NULL;
    }
    else
    {
	if (icp->has_focus && im_info->xic)
	    XUnsetICFocus(im_info->xic);

	if (icp == im_info->current)
	{
	    if (bicp != NULL)
		im_info->current = bicp;
	    else
		im_info->current = icp->next;
	    im_info->current->need_reset = True;
	}
    }
#else  /* IC_PER_SHELL */

#ifdef NON_OSF_FIX
    if (im_info->current_widget == w)
	im_info->current_widget = NULL;
#endif

#endif /* IC_PER_SHELL */
    if (bicp == NULL)			/* removing list head */
    {
	im_info->iclist = icp->next;
    }
    else
    {
	bicp->next = icp->next;
    }

    XtFree((char *)icp);

#ifdef IC_PER_SHELL
    if (im_info->current && im_info->current->need_reset)
    {
	/* reset ic with values in the XmICStruct, which will be the current
	 */
	XmImSetFocusValues(im_info->current->icw, NULL, 0);
	im_info->current->need_reset = False;
    }
    ImGeoReq(p);
#endif /* IC_PER_SHELL */
}

static XmICStruct * 
#ifdef _NO_PROTO
get_icstruct( w )
        Widget w ;
#else
get_icstruct(
        Widget w )
#endif /* _NO_PROTO */
{
    register XmICStruct *icp;
    Widget p;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;

    p = w;
    while (!XtIsShell(p))
	p = XtParent(p);

    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);
    if (extData == NULL)
	return NULL;

    ve = (XmVendorShellExtObject) extData->widget;
    if ((im_info = (XmImInfo *)ve->vendor.im_info) == NULL)
	return NULL;

    icp = im_info->iclist;
    while (icp != NULL && icp->icw != w) 
	icp = icp->next;

    return icp;
}

static XmICStruct * 
#ifdef _NO_PROTO
get_iclist( w )
        Widget w ;
#else
get_iclist(
        Widget w )
#endif /* _NO_PROTO */
{
    Widget p;
    XmVendorShellExtObject	ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;

    p = w;
    while (!XtIsShell(p))
	p = XtParent(p);

    extData = _XmGetWidgetExtData((Widget)p, XmSHELL_EXTENSION);
    if (extData == NULL)
	return NULL;

    ve = (XmVendorShellExtObject) extData->widget;
    if ((im_info = (XmImInfo *)ve->vendor.im_info) == NULL)
	return NULL;
    else
	return im_info->iclist;
}


static void 
#ifdef _NO_PROTO
draw_separator( vw )
        Widget vw ;
#else
draw_separator(
        Widget vw )
#endif /* _NO_PROTO */
{
#ifdef IC_PER_SHELL
    XmPrimitiveWidget pw;
    XmManagerWidget mw;
    GC top_shadow_GC;
    GC bottom_shadow_GC;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmICStruct *icp;
    XmImInfo *im_info;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    if ((im_info = (XmImInfo *)ve->vendor.im_info) == NULL)
	return; 
    if ((icp = im_info->current) == NULL)
	return;
    if (XmIsPrimitive(icp->icw))
    {
	pw = (XmPrimitiveWidget)icp->icw;
	top_shadow_GC = pw->primitive.top_shadow_GC;
	bottom_shadow_GC = pw->primitive.bottom_shadow_GC;
    }
    else if (XmIsManager(icp->icw))
    {
	mw = (XmManagerWidget)icp->icw;
	top_shadow_GC = mw->manager.top_shadow_GC;
	bottom_shadow_GC = mw->manager.bottom_shadow_GC;
    }
    else
	return;

    _XmDrawSeparator(XtDisplay(vw), XtWindow(vw),
		  top_shadow_GC,
		  bottom_shadow_GC,
		  0,
		  0,
		  vw->core.height - ve->vendor.im_height,
		  vw->core.width,
		  SEPARATOR_HEIGHT,
		  SEPARATOR_HEIGHT,
                  0, 			/* separator.margin */
                  XmHORIZONTAL,		/* separator.orientation */
                  XmSHADOW_ETCHED_IN); /* separator.separator_type */
#else /* OSF VERSION */
    XmPrimitiveWidget pw;
    XmVendorShellExtObject ve;
    XmWidgetExtData	extData;
    XmImInfo *im_info;

    extData = _XmGetWidgetExtData((Widget)vw, XmSHELL_EXTENSION);
    ve = (XmVendorShellExtObject) extData->widget;
    if ((im_info = (XmImInfo *)ve->vendor.im_info) == NULL)
	return; 
    pw = (XmPrimitiveWidget)im_info->current_widget;
    if (!pw || !XmIsPrimitive(pw))
	return;

    _XmDrawSeparator(XtDisplay(vw), XtWindow(vw),
		  pw->primitive.top_shadow_GC,
		  pw->primitive.bottom_shadow_GC,
		  0,
		  0,
		  vw->core.height - ve->vendor.im_height,
		  vw->core.width,
		  SEPARATOR_HEIGHT,
		  SEPARATOR_HEIGHT,
                  0, 			/* separator.margin */
                  XmHORIZONTAL,		/* separator.orientation */
                  XmSHADOW_ETCHED_IN); /* separator.separator_type */
#endif /* IC_PER_SHELL */
}

static void 
#ifdef _NO_PROTO
null_proc( w, ptr, ev, bool )
        Widget w ;
        XtPointer ptr ;
        XEvent *ev ;
        Boolean *bool ;
#else
null_proc(
        Widget w,
        XtPointer ptr,
        XEvent *ev,
        Boolean *bool )
#endif /* _NO_PROTO */
{
    /* This function does nothing.  It is only there to allow the
     * event mask required by the input method to be added to
     * the client window.
     */
}

/* The following section contains the varargs functions */


void 
#ifdef _NO_PROTO
XmImVaSetFocusValues( w, va_alist )
        Widget w ;
        va_dcl
#else
XmImVaSetFocusValues(
        Widget w,
        ... )
#endif /* _NO_PROTO */
{
    va_list	var;
    int	    	total_count;
    ArgList     args;

    Va_start(var,w);
    ImCountVaList(var, &total_count);
    va_end(var);

    Va_start(var,w);
    args  = ImCreateArgList(var, total_count);
    va_end(var);

    XmImSetFocusValues(w, args, total_count);
    XtFree((char *)args);
}

void
#ifdef _NO_PROTO
XmImVaSetValues( w, va_alist )
        Widget w ;
        va_dcl
#else
XmImVaSetValues(
        Widget w,
        ... )
#endif /* _NO_PROTO */
{
    va_list	var;
    int	    	total_count;
    ArgList     args;

    Va_start(var,w);
    ImCountVaList(var, &total_count);
    va_end(var);

    Va_start(var,w);
    args  = ImCreateArgList(var, total_count);
    va_end(var);

    XmImSetValues(w, args, total_count);
    XtFree((char *)args);
}


static void 
#ifdef _NO_PROTO
ImCountVaList( var, total_count )
        va_list var ;
        int *total_count ;
#else
ImCountVaList(
        va_list var,
        int *total_count )
#endif /* _NO_PROTO */
{
    String          attr;
    
    *total_count = 0;
 
    for(attr = va_arg(var, String) ; attr != NULL; attr = va_arg(var, String)) 
    {
	(void) va_arg(var, XtArgVal);
	++(*total_count);
    }
}

static ArgList
#ifdef _NO_PROTO
ImCreateArgList(var, total_count)
        va_list var ;
        int total_count ;
#else
ImCreateArgList(
        va_list var,
        int total_count )
#endif /* _NO_PROTO */
{
    ArgList args = (ArgList)XtMalloc(total_count * sizeof(Arg));
    register int i;

    for (i = 0; i < total_count; i++)
    {
	args[i].name = va_arg(var,String);
	args[i].value = va_arg(var,XtArgVal);
    }

    return args;
}


