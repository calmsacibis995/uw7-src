#pragma ident	"@(#)m1.2libs:Xm/MainW.c	1.4"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/* #define CDB */

#include "XmI.h"
#include <X11/Xutil.h>
#include <Xm/SeparatoGP.h>
#include <Xm/ScrollBarP.h>
#include <Xm/ScrolledWP.h>
#include <Xm/Command.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/MainWP.h>
#include "MessagesI.h"


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MWMessage1	catgets(Xm_catd,MS_MWindow,MSG_MW_1,_XmMsgMainW_0000)
#define MWMessage2	catgets(Xm_catd,MS_MWindow,MSG_MW_2,_XmMsgMainW_0001)
#else
#define MWMessage1	_XmMsgMainW_0000
#define MWMessage2	_XmMsgMainW_0001
#endif



/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassPartInitialize() ;
static void Initialize() ;
static void KidKilled() ;
static void InsertChild() ;
static Boolean WidgetVisible() ;
static void LayMeOut() ;
static void Resize() ;
static void SetBoxSize() ;
static XtGeometryResult GeometryManager() ;
static void ChangeManaged() ;
static Boolean SetValues() ;
static XtGeometryResult QueryProc() ;

#else

static void ClassPartInitialize( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void KidKilled( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static void InsertChild( 
                        Widget w) ;
static Boolean WidgetVisible( 
                        Widget w) ;
static void LayMeOut( 
                        XmMainWindowWidget mw) ;
static void Resize( 
                        Widget wid) ;
static void SetBoxSize( 
                        XmMainWindowWidget mw) ;
static XtGeometryResult GeometryManager( 
                        Widget w,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;
static void ChangeManaged( 
                        Widget wid) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static XtGeometryResult QueryProc(
                        Widget w,
                        XtWidgetGeometry *request,
                        XtWidgetGeometry *reply) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static Arg Args[20];



/************************************************************************
 *									*
 * Main Window Resources						*
 *									*
 ************************************************************************/

static XtResource resources[] = 
{
    {
	XmNcommandWindow, XmCCommandWindow, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmMainWindowRec, mwindow.CommandWindow),
	XmRImmediate, NULL
    },
    {
	XmNcommandWindowLocation, XmCCommandWindowLocation, 
        XmRCommandWindowLocation, sizeof(unsigned char),
        XtOffsetOf( struct _XmMainWindowRec, mwindow.CommandLoc),
	XmRImmediate, (XtPointer) XmCOMMAND_ABOVE_WORKSPACE
    },
    {
	XmNmenuBar, XmCMenuBar, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmMainWindowRec, mwindow.MenuBar),
	XmRImmediate, NULL
    },
    {
	XmNmessageWindow, XmCMessageWindow, XmRWidget, sizeof(Widget),
        XtOffsetOf( struct _XmMainWindowRec, mwindow.Message),
	XmRImmediate, NULL
    },
    {
        XmNmainWindowMarginWidth, XmCMainWindowMarginWidth,
        XmRHorizontalDimension, sizeof (Dimension),
        XtOffsetOf( struct _XmMainWindowRec, mwindow.margin_width), 
	XmRImmediate, (XtPointer) 0
    },
    {   
        XmNmainWindowMarginHeight, XmCMainWindowMarginHeight,
        XmRVerticalDimension, sizeof (Dimension),
        XtOffsetOf( struct _XmMainWindowRec, mwindow.margin_height), 
	XmRImmediate, (XtPointer) 0
    },
    {
	XmNshowSeparator, XmCShowSeparator, XmRBoolean, sizeof(Boolean),
        XtOffsetOf( struct _XmMainWindowRec, mwindow.ShowSep),
	XmRImmediate, FALSE
    }
};

/****************
 *
 * Resolution independent resources
 *
 ****************/

static XmSyntheticResource get_resources[] =
{
   { XmNmainWindowMarginWidth, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmMainWindowRec, mwindow.margin_width), 
     _XmFromHorizontalPixels,
     _XmToHorizontalPixels },

   { XmNmainWindowMarginHeight, 
     sizeof (Dimension),
     XtOffsetOf( struct _XmMainWindowRec, mwindow.margin_height),
     _XmFromVerticalPixels,
     _XmToVerticalPixels },

};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

externaldef(xmmainwindowclassrec) XmMainWindowClassRec
             xmMainWindowClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &xmScrolledWindowClassRec,
    /* class_name         */    "XmMainWindow",
    /* widget_size        */    sizeof(XmMainWindowRec),
    /* class_initialize   */    NULL,
    /* class_partinit     */    ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* Init hook	  */    NULL,
    /* realize            */    XtInheritRealize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave*/	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    Resize,
    /* expose             */    XtInheritExpose,
    /* set_values         */    SetValues,
    /* set values hook    */    NULL,
    /* set values almost  */    XtInheritSetValuesAlmost,
    /* get values hook    */    NULL,
    /* accept_focus       */    NULL,
    /* Version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* tm_table		  */    XtInheritTranslations,
    /* query_geometry     */    QueryProc,
    /* display_accelerator*/    NULL,
    /* extension          */    NULL,
  },
  {
/* composite_class fields */
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    ChangeManaged,
    /* insert_child	  */	InsertChild,
    /* delete_child	  */	XtInheritDeleteChild,	/* Inherit from superclass */
    /* Extension          */    NULL,
  },{
/* Constraint class Init */
    NULL,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL
      
  },
/* Manager Class */
   {		
      XtInheritTranslations,    		/* translations        */    
      get_resources,				/* get resources      	  */
      XtNumber(get_resources),			/* num get_resources 	  */
      NULL,					/* get_cont_resources     */
      0,					/* num_get_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,					/* extension           */    
   },

 {
/* Scrolled Window class - none */     
     /* extension */            (XtPointer) NULL
 },
 {
/* Main Window class - none */     
     /* extension */            (XtPointer) NULL
 }	
};

externaldef(xmmainwindowwidgetclass) WidgetClass
             xmMainWindowWidgetClass = (WidgetClass)&xmMainWindowClassRec;




/************************************************************************
 *									*
 *  ClassPartInitialize - Set up the fast subclassing.			*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitialize( wc )
        WidgetClass wc ;
#else
ClassPartInitialize(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
   _XmFastSubclassInit (wc, XmMAIN_WINDOW_BIT);
}


/************************************************************************
 *									*
 *  Initialize								*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
Initialize( rw, nw, args, num_args )
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
        XmMainWindowWidget request = (XmMainWindowWidget) rw ;
        XmMainWindowWidget new_w = (XmMainWindowWidget) nw ;
    int   k;
    
/****************
 *
 *  Bounds check the size.
 *
 ****************/
    if (request->core.width==0 || (request->core.height==0))
    {
        if (request->core.width==0)
	    new_w->core.width = 50;
	if (request->core.height==0)
	    new_w->core.height = 50;
    }

    new_w->swindow.GivenWidth = request->core.width;
    new_w->swindow.GivenHeight = request->core.height;

    XtAugmentTranslations((Widget) new_w, (XtTranslations)
              ((XmManagerClassRec *)XtClass(new_w))->manager_class.translations);

    if ((new_w->mwindow.CommandLoc != XmCOMMAND_ABOVE_WORKSPACE) &&
        (new_w->mwindow.CommandLoc != XmCOMMAND_BELOW_WORKSPACE))
        new_w->mwindow.CommandLoc = XmCOMMAND_ABOVE_WORKSPACE;

    new_w->swindow.InInit = TRUE;

    k = 0;
    XtSetArg (Args[k],XmNorientation,(XtArgVal) XmHORIZONTAL); k++;
    XtSetArg (Args[k],XmNhighlightThickness,(XtArgVal) 0); k++;
    new_w->mwindow.Sep1 = (XmSeparatorGadget) 
	XtCreateManagedWidget("Separator1", 
			      xmSeparatorGadgetClass,(Widget) new_w,Args, k);
    
    k = 0;
    XtSetArg (Args[k],XmNorientation,(XtArgVal) XmHORIZONTAL); k++;
    XtSetArg (Args[k],XmNhighlightThickness,(XtArgVal) 0); k++;
    new_w->mwindow.Sep2 = (XmSeparatorGadget) 
	XtCreateManagedWidget("Separator2", 
			      xmSeparatorGadgetClass,(Widget) new_w,Args, k);
    k = 0;
    XtSetArg (Args[k],XmNorientation,(XtArgVal) XmHORIZONTAL); k++;
    XtSetArg (Args[k],XmNhighlightThickness,(XtArgVal) 0); k++;
    new_w->mwindow.Sep3 = (XmSeparatorGadget) 
	XtCreateManagedWidget("Separator3", 
			      xmSeparatorGadgetClass,(Widget) new_w,Args, k);

    new_w->mwindow.ManagingSep = FALSE;    
    new_w->swindow.InInit = FALSE;
    new_w->swindow.XOffset = new_w->mwindow.margin_width;    
    new_w->swindow.YOffset = new_w->mwindow.margin_height;    
    new_w->swindow.WidthPad = new_w->mwindow.margin_width;    
    new_w->swindow.HeightPad = new_w->mwindow.margin_height;    
}

/************************************************************************
 *									*
 *  KidKilled								*
 *  Destroy callback for the kid widgets.				*
 *									*
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
KidKilled( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
KidKilled(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{
    XmMainWindowWidget mw;
    
    mw = (XmMainWindowWidget )(w->core.parent);
    if (w == mw->mwindow.CommandWindow)
        mw->mwindow.CommandWindow = NULL;
    if (w == mw->mwindow.MenuBar)
        mw->mwindow.MenuBar = NULL;
    if (w == mw->mwindow.Message)
        mw->mwindow.Message = NULL;
}


/************************************************************************
 *									*
 *  InsertChild								*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
InsertChild( w )
        Widget w ;
#else
InsertChild(
        Widget w )
#endif /* _NO_PROTO */
{
    XmScrolledWindowWidgetClass     superclass;
    XmManagerWidgetClass     superduperclass;    
    XmMainWindowWidget   mw = (XmMainWindowWidget)w->core.parent;
    Boolean 		 punt;

    if (!XtIsRectObj(w))
        return;

    /* do a sanity check first */
    if( mw->swindow.WorkWindow != NULL &&
       mw->swindow.WorkWindow->core.being_destroyed ) {
	mw->swindow.WorkWindow = NULL;
    }
    if( mw->swindow.hScrollBar != NULL &&
       mw->swindow.hScrollBar->core.being_destroyed ) {
	mw->swindow.hScrollBar = NULL;
    }
    if( mw->swindow.vScrollBar != NULL &&
       mw->swindow.vScrollBar->core.being_destroyed ) {
	mw->swindow.vScrollBar = NULL;
    }
    if( mw->mwindow.CommandWindow != NULL &&
       mw->mwindow.CommandWindow->core.being_destroyed ) {
	mw->mwindow.CommandWindow = NULL;
    }
    if( mw->mwindow.MenuBar != NULL &&
       mw->mwindow.MenuBar->core.being_destroyed ) {
	mw->mwindow.MenuBar = NULL;
    }
    if( mw->mwindow.Message != NULL &&
       mw->mwindow.Message->core.being_destroyed ) {
	mw->mwindow.Message = NULL;
    }
    
/****************
 *
 * If the kid is a command window or a menubar, or if the scrolled window
 * is in Variable layout mode, just insert it.
 *
 ****************/

    superclass = (XmScrolledWindowWidgetClass)xmScrolledWindowWidgetClass;
    superduperclass = (XmManagerWidgetClass)xmManagerWidgetClass;
    
    punt = FALSE;
    if (XmIsRowColumn(w))
    {
        Arg arg[1];
        unsigned char menutype;
        XtSetArg (arg[0], XmNrowColumnType, &menutype );
        XtGetValues (w, arg, 1);

	punt = (menutype == XmMENU_BAR);
	if (punt && !mw->mwindow.MenuBar)         /* If it's a menubar, and */
        {
  	    mw->mwindow.MenuBar = w;	/* we don't have one yet, use it. */
        }
    }

    if (XmIsCommandBox(w))
    {
	if (!mw->mwindow.CommandWindow)   	   /* If it's a command, and */
	{				/* we don't have one, get it */
	    punt = TRUE;
	    mw->mwindow.CommandWindow = w;
	}
	
    }
    if ((mw->swindow.InInit)                   ||
        (punt))
    {
        XtAddCallback((Widget) w, XmNdestroyCallback,KidKilled,NULL);
	(*superduperclass->composite_class.insert_child)(w);
	return;
    }
/****************
 *
 *  Else let the scrolled window have a crack at it.
 *
 ****************/
    else
	(*superclass->composite_class.insert_child)(w);

}



/************************************************************************
 *									*
 * WidgetVisible - return TRUE if the widget is there and managed.	*
 *									*
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
WidgetVisible( w )
        Widget w ;
#else
WidgetVisible(
        Widget w )
#endif /* _NO_PROTO */
{
    return(w && XtIsManaged(w));
}

/************************************************************************
 *									*
 * LayMeOut - Layout the main window.					*
 *                                                                      *
 * This code should be razed and re-written asap.                       *
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
LayMeOut( mw )
        XmMainWindowWidget mw ;
#else
LayMeOut(
        XmMainWindowWidget mw )
#endif /* _NO_PROTO */
{
    Position mbx,mby, cwx,cwy, swy, mwx, mwy, sepy, sep2y = 0;
    Dimension mbwidth, mbheight, cwwidth= 0, cwheight;
    Dimension MyXpad, MyYpad, mwwidth, mwheight;
    Dimension	bw = 0, sep2h, sep3h;
    XtWidgetGeometry  desired, preferred;
    int tmp ; /* used for checking negative Dimension value */

    /* do a sanity check first */
    if( mw->swindow.WorkWindow != NULL &&
       mw->swindow.WorkWindow->core.being_destroyed ) {
	mw->swindow.WorkWindow = NULL;
    }
    if( mw->swindow.hScrollBar != NULL &&
       mw->swindow.hScrollBar->core.being_destroyed ) {
	mw->swindow.hScrollBar = NULL;
    }
    if( mw->swindow.vScrollBar != NULL &&
       mw->swindow.vScrollBar->core.being_destroyed ) {
	mw->swindow.vScrollBar = NULL;
    }
    if( mw->mwindow.CommandWindow != NULL &&
       mw->mwindow.CommandWindow->core.being_destroyed ) {
	mw->mwindow.CommandWindow = NULL;
    }
    if( mw->mwindow.MenuBar != NULL &&
       mw->mwindow.MenuBar->core.being_destroyed ) {
	mw->mwindow.MenuBar = NULL;
    }
    if( mw->mwindow.Message != NULL &&
       mw->mwindow.Message->core.being_destroyed ) {
	mw->mwindow.Message = NULL;
    }

 /****************
 *
 * Query the kids - and we have definite preferences as to their sizes.
 * The Menubar gets top billing - we tell it it how wide it is going to be ,
 * and let it have whatever height it wants. The command box gets to stay
 * it's current height, but has to go to the new width. The scrolled window 
 * gets the leftovers.
 *
 ****************/
    MyXpad = mw->mwindow.margin_width;
    MyYpad = mw->mwindow.margin_height;
    mw->swindow.HeightPad = MyYpad;
    
    cwx = MyXpad;

    cwy = swy = MyYpad;
    
    mw->mwindow.ManagingSep = TRUE;    
	
    if (WidgetVisible(mw->mwindow.MenuBar))
    {
	bw = mw->mwindow.MenuBar->core.border_width;
	mbx = MyXpad;
	mby = MyYpad;
	tmp = mw->core.width - (2 * (MyXpad + bw));
	if (tmp <= 0) mbwidth = 10; else  mbwidth = tmp ;
	mbheight = mw->mwindow.MenuBar->core.height;

	desired.x = mbx;	
	desired.y = mby;
	desired.border_width = bw;
        desired.width = mbwidth;
        desired.height = mbheight;
        desired.request_mode = (CWWidth);
        if (XtQueryGeometry(mw->mwindow.MenuBar, &desired, &preferred) != XtGeometryYes)
        {
   	    bw = preferred.border_width;
	    mbheight = preferred.height;
        }
        _XmConfigureObject( mw->mwindow.MenuBar, mbx, mby, mbwidth, mbheight,bw);

        if (mw->mwindow.ShowSep)
        {
	    XtManageChild((Widget) mw->mwindow.Sep1);
            _XmConfigureObject( (Widget) mw->mwindow.Sep1, 0, mby + mbheight + (2 * bw),
	        	       mw->core.width,  mw->mwindow.Sep1->rectangle.height, 0);
            cwy = swy = mw->mwindow.Sep1->rectangle.height + 
                        mw->mwindow.Sep1->rectangle.y;
        }
        else
        {
            XtUnmanageChild((Widget) mw->mwindow.Sep1);
            cwy = swy = mby + mbheight + (2 * bw);
        }
    }
    else
    {
	XtUnmanageChild((Widget) mw->mwindow.Sep1);
    }

    if (WidgetVisible(mw->mwindow.CommandWindow))
    {
        bw = mw->mwindow.CommandWindow->core.border_width;
	cwx = MyXpad;
	tmp = mw->core.width - (2 * (MyXpad + bw));
	if (tmp <= 0) cwwidth = 10; else cwwidth = tmp ;
	cwheight = mw->mwindow.CommandWindow->core.height;

	desired.x = cwx;	
	desired.y = cwy;
	desired.border_width = bw;
        desired.width = cwwidth;
        desired.height = cwheight;
        desired.request_mode = (CWWidth);
        if (XtQueryGeometry(mw->mwindow.CommandWindow, &desired, &preferred) 
            != XtGeometryYes)
        {
   	    bw = preferred.border_width;
	    cwheight = preferred.height;
        }

        if ((cwheight + cwy + (2 * bw)) > (mw->core.height - (2 * MyYpad ))) {
	    tmp = mw->core.height - (2 * bw) - MyYpad - cwy;
	    if (tmp <= 0) cwheight = 10 ; else cwheight = tmp;
	}

        if (mw->mwindow.ShowSep)
            sep2h = mw->mwindow.Sep2->rectangle.height;
        else
            sep2h = 0;

        sep2y = (cwheight +  cwy) + 2 * bw;
        swy = sep2h + (cwheight +  cwy) + 2 * bw;
        if (mw->mwindow.CommandLoc == XmCOMMAND_BELOW_WORKSPACE)
        {
            mby = swy; 
            sep2y = cwy + (mw->core.height - swy - MyYpad);
            swy = cwy;
            mw->swindow.HeightPad = sep2h + cwheight;
            if (mw->mwindow.ShowSep)
                cwy = sep2y + mw->mwindow.Sep2->rectangle.height;
            else
                cwy = sep2y;
        }
    }    
    else
    {
	XtUnmanageChild((Widget) mw->mwindow.Sep2);
        sep2h = 0;
        cwheight = 0;
    }

    if (WidgetVisible(mw->mwindow.Message))
    {
        bw = mw->mwindow.Message->core.border_width;
	mwx = MyXpad;
	tmp = mw->core.width - (2 * (MyXpad + bw));
	if (tmp <= 0) mwwidth = 10 ; else mwwidth = tmp ;
	mwheight = mw->mwindow.Message->core.height;

	desired.x = mwx;	
	desired.y = swy;
	desired.border_width = bw;
        desired.width = mwwidth;
        desired.height = mwheight;
        desired.request_mode = (CWWidth);
        if (XtQueryGeometry(mw->mwindow.Message, &desired, &preferred) 
            != XtGeometryYes)
        {
   	    bw = preferred.border_width;
	    mwheight = preferred.height;
        }
        if (mw->mwindow.ShowSep)
            sep3h = mw->mwindow.Sep3->rectangle.height;
        else
            sep3h = 0;

        sepy = mw->core.height - mwheight - (2 * bw) - MyYpad - sep3h;

        if (mw->mwindow.ShowSep)
            mwy = sepy + sep3h;
        else
            mwy = sepy;

        if (mw->mwindow.CommandLoc == XmCOMMAND_BELOW_WORKSPACE)
        {
            mw->swindow.HeightPad = sep2h + cwheight + sep3h + mwheight;
            sep2y -= (sep3h + mwheight);
            cwy -= (sep3h + mwheight);
        }
        else
            mw->swindow.HeightPad = sep3h + mwheight;

        _XmConfigureObject( mw->mwindow.Message, mwx, mwy, mwwidth, mwheight, bw);
        if (mw->mwindow.ShowSep)
        {
	    XtManageChild((Widget) mw->mwindow.Sep3);
            _XmConfigureObject( (Widget) mw->mwindow.Sep3, 0, sepy, mw->core.width,  
                               mw->mwindow.Sep3->rectangle.height, 0);
        }
        else
            XtUnmanageChild((Widget) mw->mwindow.Sep3);
    }    
    else
    {
	XtUnmanageChild((Widget) mw->mwindow.Sep3);
    }
    if (WidgetVisible(mw->mwindow.CommandWindow))
    {
        _XmConfigureObject( mw->mwindow.CommandWindow, cwx, cwy, cwwidth, cwheight, bw);
        if (mw->mwindow.ShowSep)
        {
	    XtManageChild((Widget) mw->mwindow.Sep2);
            _XmConfigureObject( (Widget) mw->mwindow.Sep2, 0, sep2y, mw->core.width,  
                               mw->mwindow.Sep2->rectangle.height, 0);
        }
        else
            XtUnmanageChild((Widget) mw->mwindow.Sep2);
    }

    mw->swindow.XOffset = MyXpad;    
    mw->swindow.YOffset = swy;    
    mw->swindow.WidthPad = MyXpad;
    mw->mwindow.ManagingSep = FALSE;    
}

/************************************************************************
 *                                                                      *
 *  Recompute the size of the main window.				* 
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Resize( wid )
        Widget wid ;
#else
Resize(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmMainWindowWidget mw = (XmMainWindowWidget) wid ;
    XmScrolledWindowWidgetClass superclass;

    superclass = (XmScrolledWindowWidgetClass) xmScrolledWindowWidgetClass;
    LayMeOut(mw);
    (*superclass->core_class.resize)((Widget) mw);
}



/************************************************************************
 *									*
 * SetBoxSize - set the size of the Main window to enclose all the	*
 * visible widgets.							*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
SetBoxSize( mw )
        XmMainWindowWidget mw ;
#else
SetBoxSize(
        XmMainWindowWidget mw )
#endif /* _NO_PROTO */
{
    Dimension	    newWidth,newHeight;
    XmScrollBarWidget	    hsb, vsb;
    Widget 	    w;
    Dimension	    hsheight,vmwidth,ht,hsbht,vsbht;
    Dimension	    width, MyXpad, MyYpad;
    XtWidgetGeometry  preferred;

    ht = mw->manager.shadow_thickness  * 2;
    hsbht = vsbht = 0;
    MyXpad = mw->mwindow.margin_width * 2;
    MyYpad = mw->mwindow.margin_height * 2;

    hsb =  mw->swindow.hScrollBar;
    vsb =  mw->swindow.vScrollBar;
    
    if (mw->swindow.ScrollPolicy == XmAPPLICATION_DEFINED)
        w = mw->swindow.WorkWindow;
    else
        w = (Widget )mw->swindow.ClipWindow;
	
    if (WidgetVisible((Widget) vsb) &&
        ((Dimension)vsb->core.x < mw->core.width)) 
    {
       	vsbht = 2 * vsb->primitive.highlight_thickness;
	vmwidth = vsb->core.width + mw->swindow.pad +
	          (2 * vsb->primitive.highlight_thickness);
    }
    else
	vmwidth = 0;

    if (WidgetVisible((Widget) hsb) &&
        ((Dimension)hsb->core.y < mw->core.height)) 
    {
       	hsbht = 2 * hsb->primitive.highlight_thickness;
	hsheight = hsb->core.height + mw->swindow.pad +
		   (2 * hsb->primitive.highlight_thickness);
    }
    else
	hsheight = 0;
/****************
 *
 * Use the work window as the basis for our height. If the mode is
 * constant, and we are not realized, use the areawidth and areaheight
 * variables instead of the clipwindow width and height, since they are a
 * match for the workspace until the swindow is realized.
 *
 ****************/
    if (WidgetVisible(w)) 
    {
        if ((mw->swindow.ScrollPolicy == XmAUTOMATIC) &&
	    !XtIsRealized(mw))
	{
  	    newWidth = mw->swindow.AreaWidth + (w->core.border_width * 2) + 
		       hsbht + vmwidth + ht + MyXpad;
            newHeight = mw->swindow.AreaHeight + (w->core.border_width * 2) + 
		        vsbht + hsheight + ht + MyYpad;
        }
	else
	{
            XtQueryGeometry(w, NULL, &preferred);
	    newWidth = preferred.width + (w->core.border_width * 2) + 
		       hsbht + vmwidth + ht + MyXpad;
            newHeight = preferred.height  + (w->core.border_width * 2) + 
		        vsbht + hsheight + ht + MyYpad;
	}
    }
    else
    {
	newWidth = mw->core.width + MyXpad;
        newHeight = mw->core.height + MyYpad;
    }
    
    
    if (WidgetVisible(mw->mwindow.CommandWindow))
    {   
        XtQueryGeometry(mw->mwindow.CommandWindow, NULL, &preferred);
        width = preferred.width + 
	        (2 * mw->mwindow.CommandWindow->core.border_width);
    	if (newWidth < width) newWidth = width;
	newHeight += preferred.height + 
  	            (2 * mw->mwindow.CommandWindow->core.border_width);
        if (mw->mwindow.Sep2 && mw->mwindow.ShowSep) 
	    newHeight += mw->mwindow.Sep2->rectangle.height;

    }

    if (WidgetVisible(mw->mwindow.MenuBar))
    {   
        XtQueryGeometry(mw->mwindow.MenuBar, NULL, &preferred);
        width = preferred.width +
	        (2 * mw->mwindow.MenuBar->core.border_width);
    	if (newWidth < width) newWidth = width;
	newHeight += preferred.height +
  	            (2 * mw->mwindow.MenuBar->core.border_width);
        if (mw->mwindow.Sep1  && mw->mwindow.ShowSep) 
	    newHeight += mw->mwindow.Sep1->rectangle.height;
    }

    if (WidgetVisible(mw->mwindow.Message))
    {   
        XtQueryGeometry(mw->mwindow.Message, NULL, &preferred);
        width = preferred.width + 
	        (2 * mw->mwindow.Message->core.border_width);
    	if (newWidth < width) newWidth = width;
	newHeight += preferred.height + 
  	            (2 * mw->mwindow.Message->core.border_width);
        if (mw->mwindow.Sep3 && mw->mwindow.ShowSep) 
	    newHeight += mw->mwindow.Sep3->rectangle.height;

    }
    
/****************
 *
 *
 * If we're not realized, and we have a width and height, use it.
 *
 ******************/
    if (!XtIsRealized(mw))
    {
        if (mw->swindow.GivenWidth)
            newWidth = mw->swindow.GivenWidth;
        if (mw->swindow.GivenHeight)
            newHeight = mw->swindow.GivenHeight;
    }
    if (XtMakeResizeRequest((Widget) mw,newWidth,newHeight,NULL,NULL) == XtGeometryYes)
    	    (* (mw->core.widget_class->core_class.resize)) ((Widget) mw);
}

/************************************************************************
 *									*
 *  GeometryManager							*
 *									*
 ************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
GeometryManager( w, request, reply )
        Widget w ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else
GeometryManager(
        Widget w,
        XtWidgetGeometry *request,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
    XmMainWindowWidget mw;
    XtGeometryResult res;
    Widget	    mb;
    Dimension	    newWidth,newHeight, OldHeight;
    Dimension	    bw;
    XmScrolledWindowWidgetClass superclass;
    XtWidgetGeometry  desired, preferred;
    XtWidgetGeometry  parent_request ;

    superclass = (XmScrolledWindowWidgetClass) xmScrolledWindowWidgetClass;

    mw = (XmMainWindowWidget ) w->core.parent;
    while (!XmIsMainWindow(mw))
    mw = (XmMainWindowWidget )mw->core.parent;

#define QUERYONLY (request->request_mode & XtCWQueryOnly) 

/****************
 *
 * Disallow any X or Y changes.
 * 
 ****************/
    if ((request -> request_mode & CWX || request -> request_mode & CWY))
	if (request->request_mode & CWWidth || 
	    request->request_mode & CWHeight)
	{
	    reply->x = w->core.x;
	    reply->y = w->core.y;
	    if (request->request_mode & CWWidth)
	        reply->width = request->width;
	    if (request->request_mode & CWHeight)
	        reply->height = request->height;
	    reply->request_mode = request->request_mode & ~(CWX | CWY);
	    return(XtGeometryAlmost);
	}
	else
	    return(XtGeometryNo);


    mb = mw->mwindow.MenuBar;


/****************
 *
 * If it's not a mainwindow kid, let the scrolled window deal with it.
 * If it's from the workwindow, and the width changed, resize the menubar
 * and ask for a new height so my layout routine doesn't clip the workwindow.
 *
 ****************/
    if (w != mw->mwindow.MenuBar && 
        w != mw->mwindow.Message &&
        w != mw->mwindow.CommandWindow &&
        w != (Widget )mw->mwindow.Sep1 && 
        w != (Widget) mw->mwindow.Sep2 &&
        w != (Widget) mw->mwindow.Sep3) {
        res = (*superclass->composite_class.geometry_manager)
	    (w, request, reply);
        if (res == XtGeometryYes) {
            if ((w == mw->swindow.WorkWindow) && 
                (request->request_mode & CWWidth) && 
                WidgetVisible(mb)) {
                desired.x = mb->core.x;	
	        desired.y = mb->core.y;
	        desired.border_width = mb->core.border_width;
                desired.width = mw->core.width - 
                                (2 * mw->mwindow.margin_width);
                desired.height = mb->core.height;
                desired.request_mode = (CWWidth);
                XtQueryGeometry(mw->mwindow.MenuBar, &desired, &preferred);
                if (preferred.height != mb->core.height) {
                    parent_request.request_mode = CWWidth | CWHeight;
		    if (QUERYONLY) 
			parent_request.request_mode |= XtCWQueryOnly;
		    parent_request.width = mw->core.width ;
		    parent_request.height = newHeight = mw->core.height - 
			(mb->core.height - (2 * mb->core.border_width)) +
			    preferred.height + (2 *preferred.border_width);
                    if (XtMakeGeometryRequest((Widget) mw, 
					      &parent_request, NULL)
                        == XtGeometryYes) {
			if (!QUERYONLY) 
			    _XmResizeObject(mw->mwindow.MenuBar, 
					    preferred.width, 
					    preferred.height,
					    preferred.border_width);
			else return XtGeometryYes ;
		    }
		}
	    }
	    (* (mw->core.widget_class->core_class.resize)) ((Widget) mw);
	}	   
	return(res);
    }
    

    if(request->request_mode & CWBorderWidth)
	bw = request->border_width;
    else
        bw = w->core.border_width;

    if(request->request_mode & CWWidth) 
	newWidth = request->width + 2 * (bw + mw->mwindow.margin_width);
    else
        newWidth = w->core.width + 2 * (bw + mw->mwindow.margin_width);

     if (newWidth < mw->core.width) newWidth = mw->core.width;
     
/****************
 *
 * Margins are already included in the old width & height
 *
 ****************/
     if(request->request_mode & CWHeight)
         newHeight = mw->core.height - 
	             (w->core.height - (2 * w->core.border_width)) +
	    	     request->height + 2 * bw;
    else 
         newHeight = mw->core.height;

    OldHeight = mw->core.height;
        
    parent_request.request_mode = CWWidth | CWHeight;
    if (QUERYONLY) parent_request.request_mode |= XtCWQueryOnly;
    parent_request.width = newWidth ;
    parent_request.height = newHeight;
    res = XtMakeGeometryRequest((Widget) mw, &parent_request, NULL) ;
    if (res == XtGeometryYes) {
	if (!QUERYONLY) {
	    if(request->request_mode & CWWidth)
		w->core.width = request->width;
	    if(request->request_mode & CWHeight)
		w->core.height = request->height;
	    mw->swindow.YOffset = mw->swindow.YOffset +
		(newHeight - OldHeight);
	    (* (mw->core.widget_class->core_class.resize)) ((Widget) mw);
	}
    }
#undef QUERYONLY
    return(res);
}



/************************************************************************
 *									*
 *  ChangeManaged - called whenever there is a change in the managed	*
 *		    set.						*
 *									*
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ChangeManaged( wid )
        Widget wid ;
#else
ChangeManaged(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmMainWindowWidget mw = (XmMainWindowWidget) wid ;
    XmScrolledWindowWidgetClass superclass;
    CompositeWidget cw = (CompositeWidget) mw->swindow.ClipWindow;
    Widget   w;
    register int i;

    if (mw->mwindow.ManagingSep) return;
    superclass = (XmScrolledWindowWidgetClass) xmScrolledWindowWidgetClass;
/****************
 *
 * This is an ugly bit of work... It's possible for the clip window to get
 * "extra" kids that really want to be mainwindow widgets. So, we fix
 * that... Who says the intrinsics can't reparent widgets? :-)
 *
 ****************/
    if (mw->swindow.ScrollPolicy == XmAUTOMATIC)
    {
        if ((cw->composite.num_children > 1) &&
            (mw->swindow.WorkWindow != NULL))
        {
            for (i = 0; i < cw->composite.num_children; i++)
                if (cw->composite.children[i] != mw->swindow.WorkWindow)
                {
                    w = cw->composite.children[i];
                    if (mw->composite.num_children == mw->composite.num_slots) 
                    {
                        mw->composite.num_slots +=  (mw->composite.num_slots / 2) + 2;
                        mw->composite.children = (WidgetList) XtRealloc(
                                                 (char *) mw->composite.children,
                                                 (unsigned) (mw->composite.num_slots) 
                                                     * sizeof(Widget));
                    }
                    mw->composite.children[mw->composite.num_children++] = w;
                    w->core.parent = (Widget )mw;
                }
            cw->composite.num_children = 1;
            cw->composite.children[0] = mw->swindow.WorkWindow;
        }
                
                
    }
    if (!XtIsRealized(mw)) SetBoxSize(mw);
    (*superclass->composite_class.change_managed)((Widget) mw);	
    if (XtIsRealized(mw)) SetBoxSize(mw);
    _XmInitializeScrollBars((Widget) mw);
}


/***************************************************************************
 *									   *
 *  QueryProc (stub for now)						   *
 *									   *
 ***************************************************************************/
static XtGeometryResult 
#ifdef _NO_PROTO
QueryProc( w, request, reply )
        Widget w ;
        XtWidgetGeometry *request ;
        XtWidgetGeometry *reply ;
#else
QueryProc(
        Widget w,
        XtWidgetGeometry *request,
        XtWidgetGeometry *reply )
#endif /* _NO_PROTO */
{
/*    XmMainWindowWidget mw = (XmMainWindowWidget) w;*/
    return(XtGeometryYes);
}


#if 0
/***************************************************************************
 *									   *
 *  CalcSize (stub for now- to be called by QueryProc)			   *
 *									   *
 ***************************************************************************/
static void
#ifdef _NO_PROTO
CalcSize( mw, replyWidth, replyHeight )
        XmMainWindowWidget mw ;
        Dimension *replyWidth ;
        Dimension *replyHeight ;
#else
CalcSize(
        XmMainWindowWidget mw,
        Dimension *replyWidth ,
        Dimension *replyHeight )
#endif /* _NO_PROTO */
{
    *replyWidth = mw->core.width;
    *replyHeight = mw->core.height;

}
#endif

/************************************************************************
 *									*
 *  SetValues								*
 *									*
 ************************************************************************/
static Boolean 
#ifdef _NO_PROTO
SetValues( cw, rw, nw, args, num_args )
        Widget cw ;
        Widget rw ;
        Widget nw ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget cw,
        Widget rw,
        Widget nw,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
        XmMainWindowWidget current = (XmMainWindowWidget) cw ;
        XmMainWindowWidget request = (XmMainWindowWidget) rw ;
        XmMainWindowWidget new_w = (XmMainWindowWidget) nw ;
    Boolean ret = FALSE;
/****************
 *
 * Visual attributes
 *
 ****************/

    if ((new_w->mwindow.margin_width != current->mwindow.margin_width) ||
       (new_w->mwindow.margin_height != current->mwindow.margin_height))
            ret = TRUE;

    if ((new_w->mwindow.CommandLoc != current->mwindow.CommandLoc) &&
        ((new_w->mwindow.CommandLoc == XmCOMMAND_ABOVE_WORKSPACE) ||
         (new_w->mwindow.CommandLoc == XmCOMMAND_BELOW_WORKSPACE)))
            ret = TRUE;
    else
        new_w->mwindow.CommandLoc = current->mwindow.CommandLoc;

    if ((new_w->mwindow.MenuBar != current->mwindow.MenuBar) &&
        (new_w->mwindow.MenuBar == NULL))
    {
        _XmWarning( (Widget) new_w, MWMessage1);
	new_w->mwindow.MenuBar = current->mwindow.MenuBar;
	request->mwindow.MenuBar = current->mwindow.MenuBar;
    }
    if ((new_w->mwindow.CommandWindow != current->mwindow.CommandWindow) &&
        (new_w->mwindow.CommandWindow == NULL))
    {
        _XmWarning( (Widget) new_w, MWMessage2);    
	new_w->mwindow.CommandWindow = current->mwindow.CommandWindow;
	request->mwindow.CommandWindow = current->mwindow.CommandWindow;
    }

    if (request->swindow.WorkWindow != current->swindow.WorkWindow) 
    {
        if ((request->swindow.WorkWindow == current->mwindow.CommandWindow) &&
	    ( current->mwindow.CommandWindow ==  new_w->mwindow.CommandWindow))
            new_w->mwindow.CommandWindow = NULL;
        if ((request->swindow.WorkWindow == current->mwindow.MenuBar) &&
	    ( current->mwindow.MenuBar ==  new_w->mwindow.MenuBar))
            new_w->mwindow.MenuBar = NULL;
        if ((request->swindow.WorkWindow == (Widget) current->swindow.vScrollBar) &&
	    ( (Widget) current->swindow.vScrollBar ==  (Widget) new_w->swindow.vScrollBar))
            new_w->swindow.vScrollBar= NULL;
        if ((request->swindow.WorkWindow == (Widget) current->swindow.hScrollBar) &&
	    ( (Widget) current->swindow.hScrollBar ==  (Widget) new_w->swindow.hScrollBar))
            new_w->swindow.hScrollBar= NULL;
    }

    if (request->mwindow.MenuBar != current->mwindow.MenuBar) 
    {
        if ((request->mwindow.MenuBar == current->mwindow.CommandWindow) &&
	    ( current->mwindow.CommandWindow ==  new_w->mwindow.CommandWindow))
            new_w->mwindow.CommandWindow = NULL;
        if ((request->mwindow.MenuBar == current->swindow.WorkWindow) &&
	    ( current->swindow.WorkWindow ==  new_w->swindow.WorkWindow))
            new_w->swindow.WorkWindow = NULL;
        if ((request->mwindow.MenuBar == (Widget) current->swindow.vScrollBar) &&
	    ( (Widget) current->swindow.vScrollBar ==  (Widget) new_w->swindow.vScrollBar))
            new_w->swindow.vScrollBar= NULL;
        if ((request->mwindow.MenuBar == (Widget) current->swindow.hScrollBar) &&
	    ( (Widget) current->swindow.hScrollBar ==  (Widget) new_w->swindow.hScrollBar))
            new_w->swindow.hScrollBar= NULL;
    }

    if (request->mwindow.CommandWindow != current->mwindow.CommandWindow) 
    {
        if ((request->mwindow.CommandWindow == current->mwindow.MenuBar) &&
	    ( current->mwindow.MenuBar ==  new_w->mwindow.MenuBar))
            new_w->mwindow.MenuBar = NULL;
        if ((request->mwindow.CommandWindow == current->swindow.WorkWindow) &&
	    ( current->swindow.WorkWindow ==  new_w->swindow.WorkWindow))
            new_w->swindow.WorkWindow = NULL;
        if ((request->mwindow.CommandWindow == (Widget) current->swindow.vScrollBar) &&
	    ( (Widget) current->swindow.vScrollBar ==  (Widget) new_w->swindow.vScrollBar))
            new_w->swindow.vScrollBar= NULL;
        if ((request->mwindow.CommandWindow == (Widget) current->swindow.hScrollBar) &&
	    ( (Widget) current->swindow.hScrollBar ==  (Widget) new_w->swindow.hScrollBar))
            new_w->swindow.hScrollBar= NULL;
    }

    if (request->mwindow.Message != current->mwindow.Message) 
    {
        if ((request->mwindow.Message == current->mwindow.MenuBar) &&
	    ( current->mwindow.MenuBar ==  new_w->mwindow.MenuBar))
            new_w->mwindow.MenuBar = NULL;
        if ((request->mwindow.Message == current->swindow.WorkWindow) &&
	    ( current->swindow.WorkWindow ==  new_w->swindow.WorkWindow))
            new_w->swindow.WorkWindow = NULL;
        if ((request->mwindow.Message == (Widget) current->swindow.vScrollBar) &&
	    ( (Widget) current->swindow.vScrollBar ==  (Widget) new_w->swindow.vScrollBar))
            new_w->swindow.vScrollBar= NULL;
        if ((request->mwindow.Message == (Widget) current->swindow.hScrollBar) &&
	    ( (Widget) current->swindow.hScrollBar ==  (Widget) new_w->swindow.hScrollBar))
            new_w->swindow.hScrollBar= NULL;
    }

    if ((new_w->mwindow.MenuBar != current->mwindow.MenuBar) ||
        (new_w->mwindow.Message != current->mwindow.Message) ||
        (new_w->mwindow.CommandWindow != current->mwindow.CommandWindow ) ||
        (new_w->mwindow.ShowSep != current->mwindow.ShowSep ) ||
        (ret))
    {
	SetBoxSize(new_w);
	_XmInitializeScrollBars((Widget) new_w);
    (* (new_w->core.widget_class->core_class.resize)) ((Widget) new_w);
    }
           
    return (FALSE);
 }



/************************************************************************
 *									*
 * Spiffy API Functions							*
 *									*
 ************************************************************************/

/************************************************************************
 *									*
 * XmMainWindowSetAreas - set a new widget set.				*
 *									*
 ************************************************************************/
void 
#ifdef _NO_PROTO
XmMainWindowSetAreas( w, menu, command, hscroll, vscroll, wregion )
        Widget w ;
        Widget menu ;
        Widget command ;
        Widget hscroll ;
        Widget vscroll ;
        Widget wregion ;
#else
XmMainWindowSetAreas(
        Widget w,
        Widget menu,
        Widget command,
        Widget hscroll,
        Widget vscroll,
        Widget wregion )
#endif /* _NO_PROTO */
{
     int       k;
     k = 0;

     if (XtIsRealized(w))
     {
	if (menu) {
	    XtSetArg (Args[k],XmNmenuBar,(XtArgVal) menu); k++;
	}
	if (command) {
	    XtSetArg (Args[k],XmNcommandWindow,(XtArgVal) command); k++;
	}
	if (hscroll) {
	    XtSetArg (Args[k],XmNhorizontalScrollBar,(XtArgVal) hscroll); k++;
	}
	if (vscroll) {
	    XtSetArg (Args[k],XmNverticalScrollBar,(XtArgVal) vscroll); k++;
	}
	if (wregion) {
	    XtSetArg (Args[k],XmNworkWindow,(XtArgVal) wregion); k++;
	}
	XtSetValues(w, Args, k);

     } else
     {
	/* The story of XtRealize() and XtMainWindowSetAreas() ...
	 * We don't want to go through XtSetValues() if the MainWindow is not
	 * yet realized.  the XtSetValues() interface causes children to
	 * potentially undergo geometry modifications.  This happens either
	 * as the result of the call to SetBoxSize() from SetValues() or the
	 * resulting call to the resize method after returning to the
	 * Intrinsics.  This is disasterous if the child (manager) has not 
	 * yet fully laid out and receives a default height and width, as in 
	 * the case of Form.  Then, when the application finally calls
	 * XtRealize() and the MainWindow layout once again for real, the
	 * children are confused because their height and width are not the
	 * default zero and don't allow resizing because they think the
	 * application is imposing its own special geometry.
	 * Delete this comment if you wish.
	 */
	XmMainWindowWidget mw = (XmMainWindowWidget) w;

	if (menu) mw->mwindow.MenuBar = menu;
	if (command) mw->mwindow.CommandWindow = command;
	if (hscroll) mw->swindow.hScrollBar = (XmScrollBarWidget)hscroll;
	if (vscroll) mw->swindow.vScrollBar = (XmScrollBarWidget)vscroll;
	if (wregion) mw->swindow.WorkWindow = wregion;
     }
}


/************************************************************************
 *									*
 * XmMainWindowSep1 - return the id of the top seperator widget.	*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmMainWindowSep1( w )
        Widget w ;
#else
XmMainWindowSep1(
        Widget w )
#endif /* _NO_PROTO */
{
    XmMainWindowWidget   mw = (XmMainWindowWidget) w;
    return ((Widget) mw->mwindow.Sep1);
}

/************************************************************************
 *									*
 * XmMainWindowSep2 - return the id of the bottom seperator widget.	*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmMainWindowSep2( w )
        Widget w ;
#else
XmMainWindowSep2(
        Widget w )
#endif /* _NO_PROTO */
{
    XmMainWindowWidget   mw = (XmMainWindowWidget) w;
    return ((Widget) mw->mwindow.Sep2);
}


/************************************************************************
 *									*
 * XmMainWindowSep3 - return the id of the bottom seperator widget.	*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmMainWindowSep3( w )
        Widget w ;
#else
XmMainWindowSep3(
        Widget w )
#endif /* _NO_PROTO */
{
    XmMainWindowWidget   mw = (XmMainWindowWidget) w;
    return ((Widget) mw->mwindow.Sep3);
}


/************************************************************************
 *									*
 * XmCreateMainWindow - hokey interface to XtCreateWidget.		*
 *									*
 ************************************************************************/
Widget 
#ifdef _NO_PROTO
XmCreateMainWindow( parent, name, args, argCount )
        Widget parent ;
        char *name ;
        ArgList args ;
        Cardinal argCount ;
#else
XmCreateMainWindow(
        Widget parent,
        char *name,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{

    return ( XtCreateWidget( name, 
			     xmMainWindowWidgetClass, 
			     parent, 
			     args, 
			     argCount ) );
}
