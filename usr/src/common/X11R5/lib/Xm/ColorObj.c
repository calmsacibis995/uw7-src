#pragma ident	"@(#)m1.2libs:Xm/ColorObj.c	1.8"
/*** ColorObj.c ***/

#include <Xm/ColorObjP.h>
#include <Xm/VendorSEP.h>
#include <Xm/RowColumnP.h>
#include "CallbackI.h"

#ifdef I18N_MSG
#include "XmMsgI.h"
#endif

#define  OBJ_CLASS(w)   (((ApplicationShellWidget)(w))->application.class)

#ifdef I18N_MSG
#define WARNING1     catgets(Xm_catd,MS_ColObj,MSG_CO_1,\
			   "Could not allocate memory for color object data.")
#define WARNING2     catgets(Xm_catd,MS_ColObj,MSG_CO_2,\
			   "Bad screen number from color server selection.")
#else
#define  WARNING1    "Could not allocate memory for color object data."
#define  WARNING2    "Bad screen number from color server selection."
#endif

/** default should not be killed unless application is dying **/
static ColorObj DefaultColorObj = NULL;
static XContext ColorObjCache = NULL;


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void Destroy() ;
static void DisplayDestroy() ;
static int  GetAppScreenNum() ;
static void Initialize() ;
static void GetSelection() ;
static void MakeRCHook() ;
static void RCHook() ;
static void UpdatePixelSet() ;
static void UpdateXrm() ;

#else

static void Destroy( 
                        Widget wid) ;
static void DisplayDestroy( 
                        Widget wid,
			XtPointer clientData,
			XtPointer callData) ;
static int GetAppScreenNum( 
                        Display *display,
                        Screen *screen) ;
static void Initialize( 
                        Widget rq,
                        Widget nw,
                        ArgList Args,
                        Cardinal *numArgs) ;
static void GetSelection( 
                        Widget w,
                        XtPointer client_data,
                        Atom *selection,
                        Atom *type,
                        XtPointer val,
                        unsigned long *length,
                        int *format) ;
static void MakeRCHook( ColorObj colorObj) ;
static void RCHook( 
                        Widget w,
                        ArgList alIn,
                        Cardinal *acPtrIn) ;
static void UpdatePixelSet( 
                        PixelSet *toSet,
                        PixelSet *fromSet) ;
static void UpdateXrm( 
                        Colors colors,
                        int screen,
                        ColorObj tmpColorObj);

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

#define UNSPECIFIED_USE_MULTI_COLOR_ICONS 2

static XtResource resources[] = {
    {  
	XmNprimaryColorSetId,
	XmCPrimaryColorSetId,
	XmRInt,
	sizeof(int),
	XtOffset (ColorObj, color_obj.primary),
	XmRImmediate,
	(XtPointer) 5,
    },
    { 
	XmNsecondaryColorSetId,
	XmCSecondaryColorSetId,
	XmRInt,
	sizeof(int),
	XtOffset (ColorObj, color_obj.secondary),
	XmRImmediate,
	(XtPointer) 6,
    },
    { 
	XmNtextColorSetId,
	XmCTextColorSetId,
	XmRInt,
	sizeof(int),
	XtOffset (ColorObj, color_obj.text),
	XmRImmediate,
	(XtPointer) 4,
    },
    { 
	XmNuseTextColor,
	XmCUseTextColor,
	XmRBoolean,
	sizeof(Boolean),
	XtOffset (ColorObj, color_obj.useText),
	XmRImmediate,
	(XtPointer) True,
    },
    { 
	XmNuseTextColorForList,
	XmCUseTextColorForList,
	XmRBoolean,
	sizeof(Boolean),
	XtOffset (ColorObj, color_obj.useTextForList),
	XmRImmediate,
	(XtPointer) True,
    },
    {  
	XmNactiveColorSetId,
	XmCActiveColorSetId,
	XmRInt,
	sizeof(int),
	XtOffset (ColorObj, color_obj.active),
	XmRImmediate,
	(XtPointer) 1,
    },
    {  
	XmNinactiveColorSetId,
	XmCInactiveColorSetId,
	XmRInt,
	sizeof(int),
	XtOffset (ColorObj, color_obj.inactive),
	XmRImmediate,
	(XtPointer) 2,
    },
    {  
	XmNuseColorObj,
	XmCUseColorObj,
	XmRBoolean,
	sizeof(Boolean),
	XtOffset (ColorObj, color_obj.useColorObj),
	XmRImmediate,
	(XtPointer) True,
    },
    {  
	XmNuseMask,
	XmCUseMask,
	XmRBoolean,
	sizeof(Boolean),
	XtOffset (ColorObj, color_obj.useMask),
	XmRImmediate,
	(XtPointer) True,
    },
    {  
	XmNuseMultiColorIcons,
	XmCUseMultiColorIcons,
	XmRBoolean,
	sizeof(Boolean),
	XtOffset (ColorObj, color_obj.useMultiColorIcons),
	XmRImmediate,
	(XtPointer) UNSPECIFIED_USE_MULTI_COLOR_ICONS,
    },
    {  
	XmNuseIconFileCache,
	XmCUseIconFileCache,
	XmRBoolean,
	sizeof(Boolean),
	XtOffset (ColorObj, color_obj.useIconFileCache),
	XmRImmediate,
	(XtPointer) True,
    },
};


ColorObjClassRec _xmColorObjClassRec = 
{
    {
	/*
	 * make it a topLevelShell subclass in order to avoid
	 * baseClass recursion.  This is due to the posthook logic and
	 * the nested created of ColorObj inside of the first appShell
	 */
        (WidgetClass)&wmShellClassRec,    /* superclass       */
        "ColorObj",                       /* class_name            */
        sizeof(ColorObjRec),              /* widget_size           */
        NULL,                             /* class_initialize      */
        NULL,                             /* class_part_initialize */
        FALSE,                            /* class_inited          */
        Initialize,                       /* initialize            */
        NULL,                             /* initialize_hook       */
        XtInheritRealize,                 /* realize               */
        NULL,                             /* actions               */
        0,                                /* num_actions           */
        resources,                        /* resources             */
        XtNumber(resources),              /* num_resources         */
        NULLQUARK,                        /* xrm_class             */
        FALSE,                            /* compress_motion       */
        FALSE,                            /* compress_exposure     */
        FALSE,                            /* compress_enterleave   */
        FALSE,                            /* visible_interest      */
        Destroy,                          /* destroy               */
        NULL,                             /* resize                */
        NULL,                             /* expose                */
        NULL,                             /* set_values            */
        NULL,                             /* set_values_hook       */
        NULL,                             /* set_values_almost     */
        NULL,                             /* get_values_hook       */
        NULL,                             /* accept_focus          */
        XtVersion,                        /* version               */
        NULL,                             /* callback_offsets      */
        NULL,                             /* tm_table              */
        NULL,                             /* query_geometry        */
        NULL,                             /* display_accelerator   */
        NULL                              /* extension             */
    },
    { 					/* composite class record */
	NULL,		                /* geometry_manager 	*/
	NULL,	 			/* change_managed	*/
	XtInheritInsertChild,		/* insert_child		*/
	XtInheritDeleteChild, 		/* from the shell 	*/
	NULL, 				/* extension record     */
    },
    { 					/* shell class record 	*/
	NULL, 				/* extension record     */
    },
    { 					/* wm shell class record */
	NULL, 				/* extension record     */
    },
    {					/* colorObj class	*/
	NULL,				/* extension		*/
    },
};

WidgetClass _xmColorObjClass = (WidgetClass)&_xmColorObjClassRec;



/**********************************************************************/
/** _XmColorObjCreate() - initialize_hook() from Display object...   **/
/**         Used to create a ColorObj.  Updated to support one per   **/
/**         display.  There will be a "default" display and ColorObj **/
/**         for the client, and each new Display Object will have    **/
/**         a new ColorObj associated with it.  This allows for      **/
/**         things like the dialog server (which hangs around and    **/
/**         runs clients as if they were seperate applications) to   **/
/**         utilize seperate resource databases for "pseudo-apps".   **/
/**                                                                  **/
/**********************************************************************/
/*ARGSUSED*/
void 
#ifdef _NO_PROTO
_XmColorObjCreate( w, al, acPtr )
        Widget w ;
        ArgList al ;
        Cardinal *acPtr ;
#else
_XmColorObjCreate(
        Widget w,
        ArgList al,
        Cardinal *acPtr )
#endif /* _NO_PROTO */
{
    String	name, class;

    /** don't create if in initialization of the color server **/
    if (XtIsApplicationShell(w))
            if ( strcmp(OBJ_CLASS(w), COLOR_SRV_NAME) == 0 )  return;
    
    /** this is really gross but it makes the resources work right **/
    XtGetApplicationNameAndClass(XtDisplay(w), &name, &class);
    _xmColorObjClass->core_class.class_name = class;
    XtAppCreateShell(name, class, _xmColorObjClass, XtDisplay(w), NULL, 0);

    /** set up destroy callback on display object for this ColorObj **/
    XtAddCallback(w, XmNdestroyCallback, DisplayDestroy, NULL);
}


/**********************************************************************/
/** DisplayDestroy()                                                 **/
/**        Display object is being destroyed... destroy associated   **/
/**        colorObj if there is one.                                 **/
/**                                                                  **/
/**********************************************************************/
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DisplayDestroy( wid, clientData, callData )
        Widget wid ;
	XtPointer clientData, callData;
#else
DisplayDestroy( Widget wid, XtPointer clientData, XtPointer callData )
#endif /* _NO_PROTO */
{
    ColorObj tmpColorObj=NULL;

    if ( XFindContext(XtDisplay(wid), (XID)XtDisplay(wid), ColorObjCache,
	 (XPointer *)&tmpColorObj) == 0)
    {
	if (tmpColorObj)
	{
 	    XtDestroyWidget((Widget)tmpColorObj);
	}
    }
}


/**********************************************************************/
/** Destroy()                                                        **/
/**        Free the data allocated for this ColorObj                 **/
/**                                                                  **/
/**********************************************************************/
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
Destroy( wid )
        Widget wid ;
#else
Destroy( Widget wid )
#endif /* _NO_PROTO */
{
    ColorObj tmpColorObj = (ColorObj)wid;

    if (tmpColorObj->color_obj.colors)
       XtFree ((char *) tmpColorObj->color_obj.colors);
    if (tmpColorObj->color_obj.atoms)
       XtFree ((char *) tmpColorObj->color_obj.atoms);
    if (tmpColorObj->color_obj.colorUse)
       XtFree ((char *) tmpColorObj->color_obj.colorUse);

    XDeleteContext(XtDisplay(wid), (XID)tmpColorObj->color_obj.display, 
		   ColorObjCache);

    if (tmpColorObj == DefaultColorObj)
	DefaultColorObj = NULL;
}


/**********************************************************************/
/** GetAppScreenNum()                                                **/
/**                                                                  **/
/**********************************************************************/
static int 
#ifdef _NO_PROTO
GetAppScreenNum( display, screen )
        Display *display ;
        Screen *screen ;
#else
GetAppScreenNum(
        Display *display,
        Screen *screen )
#endif /* _NO_PROTO */
{
    int i;
    int retVal = -1;

    for (i = 0; i < XScreenCount(display); i++)
    {
        if (screen == XScreenOfDisplay(display, i)) 
        {
            retVal = i;
            break;
        }
    }
    return retVal;
}


/**********************************************************************/
/** Initialize()                                                     **/
/**                                                                  **/
/**********************************************************************/
static void 
#ifdef _NO_PROTO
Initialize( rq, nw, Args, numArgs )
        Widget rq ;
        Widget nw ;
        ArgList Args ;
        Cardinal *numArgs ;
#else
Initialize(
        Widget rq,
        Widget nw,
        ArgList Args,
        Cardinal *numArgs)
#endif /* _NO_PROTO */
{
    ColorObj new = (ColorObj) nw ;
    char     customizer[24];
    int      i, nscreens;
    Atom     tmpAtom;
    unsigned long time_out = 0;


    new->color_obj.colorIsRunning = False;
    new->color_obj.colors = NULL;
    new->color_obj.atoms = NULL;
    new->color_obj.colorUse = NULL;

    new->color_obj.display = XtDisplay(new);
    new->color_obj.numScreens = nscreens = ScreenCount(new->color_obj.display);
	
    /** initialize default colorObj and context if needed **/
    if (ColorObjCache == NULL)
        ColorObjCache = XUniqueContext();

    if (DefaultColorObj == NULL)
        DefaultColorObj = new;

    /** add new colorObj to the cache **/
    XSaveContext(XtDisplay(new), (XID)new->color_obj.display, 
		 ColorObjCache, (XPointer)new);

    /** set up initialize_hook for row column **/
    MakeRCHook(new);

    /** if useColorObj = False, don't initialize or allocate color data **/
    if (new->color_obj.useColorObj) 
    {
	
        /** get screen info and allocate space for colors per screen **/
	new->color_obj.colors = (Colors *)XtCalloc(nscreens, sizeof(Colors));
	new->color_obj.atoms = (Atom *)XtCalloc(nscreens, sizeof(Atom));
	new->color_obj.colorUse = (int *)XtCalloc(nscreens, sizeof(int));
	
	if ( !new->color_obj.colors || !new->color_obj.atoms || 
	    !new->color_obj.colorUse )
	{
	      _XmWarning( (Widget)new, WARNING1); /* couldn't allocate memory */
	      new->color_obj.colorIsRunning = False;
	      return;
	}
	
	/** set screen and color info for this application **/
	
	new->color_obj.myScreen = 
	  GetAppScreenNum(new->color_obj.display, XtScreen(new));
	new->color_obj.myColors = 
	  new->color_obj.colors[new->color_obj.myScreen];
	
	
	/* check valid value, then -1 from colors, to index arrays */
	
	if (new->color_obj.primary < 1 || new->color_obj.primary > NUM_COLORS)
	  new->color_obj.primary = 1;
	if (new->color_obj.secondary < 1 ||
	    new->color_obj.secondary > NUM_COLORS)
	  new->color_obj.secondary = 1;
	if (new->color_obj.active < 1 || new->color_obj.active > NUM_COLORS)
	  new->color_obj.active = 1;
	if (new->color_obj.inactive < 1 || new->color_obj.inactive > NUM_COLORS)
	  new->color_obj.inactive = 1;
	if (new->color_obj.text < 1 || new->color_obj.text > NUM_COLORS)
	  new->color_obj.text = 1;
	
	new->color_obj.primary   = new->color_obj.primary   - 1; 
	new->color_obj.secondary = new->color_obj.secondary - 1; 
	new->color_obj.active    = new->color_obj.active    - 1; 
	new->color_obj.inactive  = new->color_obj.inactive  - 1; 
	new->color_obj.text      = new->color_obj.text  - 1; 
	
	new->core.mapped_when_managed = False;
	new->core.width = 1;
	new->core.height = 1;
	/* realize the widget .. */
	if(!XtIsRealized((Widget) new))
	  XtRealizeWidget((Widget) new);
	
	/** get screen information for each screen **/
	tmpAtom = XmInternAtom(new->color_obj.display, PIXEL_SET, FALSE);

 	/** Save SelectionTimeout **/
	time_out = XtAppGetSelectionTimeout(
			XtWidgetToApplicationContext((Widget) new));

	/** set the selection timeout to 900 seconds (for now) **/
	XtAppSetSelectionTimeout(XtWidgetToApplicationContext(
						(Widget) new),
				 		(unsigned long)900000);
	
	for (i = 0; i < nscreens; i++)
	{
	      /* initialize the selection atoms */
	      sprintf(customizer,"%s%d", CUST_DATA, i);
	      new->color_obj.atoms[i] = XInternAtom(new->color_obj.display, 
						    customizer, FALSE);
	      
	      /* get the values associated with CUST_DATA and PIXEL_SET */
	      new->color_obj.done = FALSE;
#ifdef NOT_SAMPLE
#ifdef SUN_MOTIF
	      if(strcmp( ServerVendor(XtDisplay(new)),
 			"X11/NeWS - Sun Microsystems Inc.") ||
 			XGetSelectionOwner(XtDisplay(new),
 					new->color_obj.atoms[i])) {
#endif
#endif
	      		XtGetSelectionValue((Widget) new,
			  new->color_obj.atoms[i], tmpAtom,
			  GetSelection, (XtPointer) 1, CurrentTime);

	      		/* wait for the reply */
	      		while(new->color_obj.done == FALSE)
			  XtAppProcessEvent(
			    XtWidgetToApplicationContext((Widget) new),
		   	    XtIMAll);
#ifdef NOT_SAMPLE
#ifdef SUN_MOTIF
		}
#endif
#endif
	      
	      if (!new->color_obj.colorIsRunning) 
		break; /* don't bother with rest */
	}

	/** Reset SelectionTimeout to saved value **/
	XtAppSetSelectionTimeout(XtWidgetToApplicationContext(
					(Widget) new),
 				 	time_out);
    }


    if (new->color_obj.useMultiColorIcons == UNSPECIFIED_USE_MULTI_COLOR_ICONS)
    {
	  if (new->color_obj.colorUse) {
	      if (new->color_obj.colorUse[0] == HIGH_COLOR
		  || new->color_obj.colorUse[0] == MEDIUM_COLOR)
	      {
		new->color_obj.useMultiColorIcons = True;
	      }
	      else
		new->color_obj.useMultiColorIcons = False;
	  }
	  else /* no color server ??? */
	      new->color_obj.useMultiColorIcons = False;
    }
    /* unrealize the widget, we could destroy it... */
    XtUnrealizeWidget((Widget) new);
}


/**********************************************************************/
/** GetSelection()                                                   **/
/**        colorIsRunning = False on entry, gets set to True if      **/
/**        color info successfully read in.                          **/
/**                                                                  **/
/**********************************************************************/
static void 
#ifdef _NO_PROTO
GetSelection( w, client_data, selection, type, val, length, format )
        Widget w ;
        XtPointer client_data ;
        Atom *selection ;
        Atom *type ;
        XtPointer val ;
        unsigned long *length ;
        int *format ;
#else
GetSelection(
        Widget w,
        XtPointer client_data,
        Atom *selection,
        Atom *type,
        XtPointer val,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    ColorObj tmpColorObj = (ColorObj)w;
    char *   value = (XtPointer) val ;
    Colors   colors;
    int      i, count, screen, colorUse;
    char     tmp[50];

    /** get screen number **/
    screen = -1;
    for (i = 0; i < tmpColorObj->color_obj.numScreens; i++)
    {
        if (*selection == tmpColorObj->color_obj.atoms[i])
        {
            screen = i;
            break;
        }
    }
    if (screen == -1) 
    {
        _XmWarning( (Widget) tmpColorObj, WARNING2);   /* bad screen number */
        return;
    }

    if (value != NULL)
    {
        /* read color use */
        count = 0;
        sscanf (&(value[count]), "%x_", &colorUse);
        sprintf(tmp, "%x_", colorUse);
        count += strlen(tmp);
        tmpColorObj->color_obj.colorUse[screen] = colorUse;

        for (i = 0; i < NUM_COLORS; i++)
        {
            /* read bms data into PixelSet */
            sscanf (&(value[count]), "%x_%x_%x_%x_%x_", &(colors[i].bg), 
                    &(colors[i].fg), &(colors[i].ts),
                    &(colors[i].bs), &(colors[i].sc));
            sprintf(tmp,"%x_%x_%x_%x_%x_", colors[i].bg, colors[i].fg, 
                    colors[i].ts, colors[i].bs, colors[i].sc);
            count += strlen(tmp);
        }
        UpdateXrm (colors, screen, tmpColorObj);
        tmpColorObj->color_obj.colorIsRunning = True;
        XFree (value);
    }

    tmpColorObj->color_obj.done = TRUE;
}


/**********************************************************************/
/** MakeRCHook()                                                     **/
/**         Do this for each ColorObj (rather than a global).  This  **/
/**         way if the useColorObj resource is set differently for   **/
/**         different displays, the RCHook will not get called for   **/
/**         cases where useColorObj is set to false.  (Not likely,   **/
/**         but it is possible.)                                     **/
/**                                                                  **/
/**********************************************************************/
static void 
#ifdef _NO_PROTO
MakeRCHook(colorObj)
           ColorObj colorObj;
#else
MakeRCHook(ColorObj colorObj)
#endif /* _NO_PROTO */
{
    /** save original function to encapsulate later **/
    /** make sure we haven't already encapsulated it **/
    static XtArgsProc initialize_hook = NULL;
    static Boolean firstTime = True;

    if (firstTime)
    {
        initialize_hook = xmRowColumnClassRec.core_class.initialize_hook;
        xmRowColumnClassRec.core_class.initialize_hook = RCHook;
        firstTime = False;
    }

    colorObj->color_obj.RowColInitHook = initialize_hook;
}


/**********************************************************************/
/** RCHook                                                           **/
/**    With new "per-display" color objects, we need to check to see **/
/**    if this ColorObj is using the color server before we set up   **/
/**    any special colors.                                           **/
/**                                                                  **/
/**********************************************************************/
static void 
#ifdef _NO_PROTO
RCHook( w, alIn, acPtrIn )
        Widget w ;
        ArgList alIn ;
        Cardinal *acPtrIn ;
#else
RCHook(
        Widget w,
        ArgList alIn,
        Cardinal *acPtrIn )
#endif /* _NO_PROTO */
{
    Arg al[10];
    int ac;
    unsigned char rcType;
    static int mono, color, colorPrim, init=0;
    static Screen *screen;
    Pixmap  ditherPix, solidPix;
    ColorObj tmpColorObj=NULL;
    Pixel  defaultBackground;

    /** get the colorObj for this display connection **/
    if (XFindContext(XtDisplay(w), (XID)XtDisplay(w), 
		     ColorObjCache, (XPointer *)&tmpColorObj) != 0)
    {   /* none found, use default */
	if (DefaultColorObj)
	    tmpColorObj = DefaultColorObj;
	else  /* this should NEVER happen... RowColInitHook won't get called */
	    return;
    }

    /** call original initialize_hook if there was one **/
    if (tmpColorObj->color_obj.RowColInitHook != NULL)
        (*tmpColorObj->color_obj.RowColInitHook) (w, alIn, acPtrIn);

    /** don't set colors if this display isn't using the color server **/
    if (tmpColorObj->color_obj.colorIsRunning == NULL)
	return;

    ac = 0;
    XtSetArg (al[ac], XmNrowColumnType, &rcType);                ac++;
    XtSetArg (al[ac], XmNbackground, &defaultBackground);        ac++;
    XtGetValues (w, al, ac);

    if (rcType == XmMENU_BAR)  /* set to secondary, rather than primary */
    {
        if (!init)
        {
            if (tmpColorObj->color_obj.colorUse[tmpColorObj->color_obj.myScreen]==B_W)
                mono = 1;
            else 
                mono = 0;

            color = tmpColorObj->color_obj.secondary;
            colorPrim = tmpColorObj->color_obj.primary;
            screen = XtScreen(tmpColorObj);
            init = 1;
        }

	/** if background didn't default to ColorObj, don't overwrite colors **/
	if (defaultBackground != tmpColorObj->color_obj.myColors[colorPrim].bg)
	    return;

        ac = 0;
        XtSetArg (al[ac], XmNbackground, 
                  tmpColorObj->color_obj.myColors[color].bg);        ac++;
        XtSetArg (al[ac], XmNforeground, 
                  tmpColorObj->color_obj.myColors[color].fg);        ac++;
        XtSetArg (al[ac], XmNtopShadowColor, 
                  tmpColorObj->color_obj.myColors[color].ts);        ac++;
        XtSetArg (al[ac], XmNbottomShadowColor, 
                  tmpColorObj->color_obj.myColors[color].bs);        ac++;

        /** put dithers for top shadows if needed **/
        if (DitherTopShadow(tmpColorObj->color_obj.display, 
			    tmpColorObj->color_obj.myScreen,
                            &tmpColorObj->color_obj.myColors[color]))
        {
            if (mono)
                ditherPix = XmGetPixmap (screen, "50_foreground",
                                         BlackPixelOfScreen(screen),
                                         WhitePixelOfScreen(screen));
            else
                ditherPix = XmGetPixmap (screen, "50_foreground",
                                         tmpColorObj->color_obj.myColors[color].bg,
                                         WhitePixelOfScreen(screen));

            XtSetArg (al[ac], XmNtopShadowPixmap, ditherPix);         ac++;
        }
        else      /** see if we need to "undo" primary dither **/
        if (DitherTopShadow(tmpColorObj->color_obj.display, 
			    tmpColorObj->color_obj.myScreen,
                            &tmpColorObj->color_obj.myColors[colorPrim]))
        {   /* simulate solid white (will happen for B_W case only)*/
            solidPix = XmGetPixmap (screen, "background",
                                    WhitePixelOfScreen(screen),
                                    WhitePixelOfScreen(screen));
            XtSetArg (al[ac], XmNtopShadowPixmap, solidPix);          ac++;
        }

        /** put dithers for bottom shadows if needed **/
        if (DitherBottomShadow(tmpColorObj->color_obj.display, 
			       tmpColorObj->color_obj.myScreen,
                               &tmpColorObj->color_obj.myColors[color]))
        {
            if (mono)
                ditherPix = XmGetPixmap (screen, "50_foreground",
                                         BlackPixelOfScreen(screen),
                                         WhitePixelOfScreen(screen));
            else
                ditherPix = XmGetPixmap (screen, "50_foreground",
                                         tmpColorObj->color_obj.myColors[color].bg,
                                         BlackPixelOfScreen(screen));
            XtSetArg (al[ac], XmNbottomShadowPixmap, ditherPix);      ac++;
        }
        else      /** see if we need to "undo" primary dither **/
        if (DitherBottomShadow(tmpColorObj->color_obj.display, 
			       tmpColorObj->color_obj.myScreen,
                               &tmpColorObj->color_obj.myColors[colorPrim]))
        {   /* simulate solid black (will happen for B_W case only)*/
            solidPix = XmGetPixmap (screen, "background",
                                    BlackPixelOfScreen(screen),
                                    BlackPixelOfScreen(screen));
            XtSetArg (al[ac], XmNbottomShadowPixmap, solidPix);       ac++;
        }

        XtSetValues (w, al, ac);
    }
}



/**********************************************************************/
/** UpdatePixelSet()                                                 **/
/**                                                                  **/
/**********************************************************************/
static void 
#ifdef _NO_PROTO
UpdatePixelSet( toSet, fromSet )
        PixelSet *toSet ;
        PixelSet *fromSet ;
#else
UpdatePixelSet(
        PixelSet *toSet,
        PixelSet *fromSet )
#endif /* _NO_PROTO */
{
    toSet->bg = fromSet->bg;
    toSet->fg = fromSet->fg;
    toSet->ts = fromSet->ts;
    toSet->bs = fromSet->bs;
    toSet->sc = fromSet->sc;
}


/**********************************************************************/
/** UpdateXrm()                                                      **/
/**                                                                  **/
/**********************************************************************/
static void 
#ifdef _NO_PROTO
UpdateXrm( colors, screen, tmpColorObj )
        Colors colors ;
        int screen ;
	ColorObj tmpColorObj;
#else
UpdateXrm(
        Colors colors,
        int screen , 
	ColorObj tmpColorObj)
#endif /* _NO_PROTO */
{
    XrmDatabase     db;
    XrmValue        value;
    XmColorData     cacheRec;
    int             i;
    int             doList;
    
    /** update the internal color information **/
    for (i = 0; i < NUM_COLORS; i++)
        UpdatePixelSet (&(tmpColorObj->color_obj.colors[screen][i]),&colors[i]);

    /** if this is not the application screen, do not update the database **/
    if (screen != tmpColorObj->color_obj.myScreen)  return;

    /** update the color cache in motif with just primary, secondary **/
    i = tmpColorObj->color_obj.primary;
    cacheRec.screen = XtScreen(tmpColorObj);
    cacheRec.color_map = DefaultColormapOfScreen(cacheRec.screen);
    cacheRec.background.pixel = colors[i].bg;
    cacheRec.foreground.pixel = colors[i].fg;
    cacheRec.top_shadow.pixel = colors[i].ts;
    cacheRec.bottom_shadow.pixel = colors[i].bs;
    cacheRec.select.pixel = colors[i].sc;
    cacheRec.allocated = XmBACKGROUND | XmFOREGROUND | XmTOP_SHADOW | 
                         XmBOTTOM_SHADOW | XmSELECT;
    _XmAddToColorCache (&cacheRec);      /* would be nice to put RGB values */

    i = tmpColorObj->color_obj.secondary;
    cacheRec.screen = XtScreen(tmpColorObj);
    cacheRec.color_map = DefaultColormapOfScreen(cacheRec.screen);
    cacheRec.background.pixel = colors[i].bg;
    cacheRec.foreground.pixel = colors[i].fg;
    cacheRec.top_shadow.pixel = colors[i].ts;
    cacheRec.bottom_shadow.pixel = colors[i].bs;
    cacheRec.select.pixel = colors[i].sc;
    cacheRec.allocated = XmBACKGROUND | XmFOREGROUND | XmTOP_SHADOW | 
                         XmBOTTOM_SHADOW | XmSELECT;
    _XmAddToColorCache (&cacheRec);      /* would be nice to put RGB values */

    /** Xm does funny stuff with non-window widgets, it's being fixed. **/
    db = XtDatabase(tmpColorObj->color_obj.display);

    /** update the clients database with new colors **/

    value.size = sizeof(Pixel);

    /** update the highlight color information to use the active color - bg **/
    value.addr = (XtPointer) &(colors[tmpColorObj->color_obj.active].bg);
    XrmPutResource (&db, "*highlightColor", "Pixel", &value);

    /** update the primary color set information **/

    i = tmpColorObj->color_obj.primary;
    value.addr = (XtPointer) &(colors[i].bg);
    XrmPutResource (&db, "*background", "Pixel", &value);

/*
 * keep foreground for athena widgets and 2.0 compatability.
 * DONT write out the other dependent colors in order to allow motif
 * to generate them.
 */

    value.addr = (XtPointer) &(colors[i].fg);
    XrmPutResource (&db, "*foreground", "Pixel", &value);

#ifdef notdef
    value.addr = (XtPointer) &(colors[i].sc);
    XrmPutResource (&db, "*selectColor", "Pixel", &value);
    value.addr = (XtPointer) &(colors[i].sc);
    XrmPutResource (&db, "*armColor", "Pixel", &value);
    value.addr = (XtPointer) &(colors[i].ts);
    XrmPutResource (&db, "*topShadowColor", "Pixel", &value);
    value.addr = (XtPointer) &(colors[i].bs);
    XrmPutResource (&db, "*bottomShadowColor", "Pixel", &value);
#endif /* notdef */


    if (DitherTopShadow(tmpColorObj->color_obj.display, screen, &colors[i])) 
        XrmPutStringResource (&db, "*topShadowPixmap", DITHER);
    else 
        XrmPutStringResource (&db, "*topShadowPixmap", NO_DITHER);
    if (DitherBottomShadow(tmpColorObj->color_obj.display, screen, &colors[i])) 
        XrmPutStringResource (&db, "*bottomShadowPixmap", DITHER);

    /** update the secondary color set information **/
    i = tmpColorObj->color_obj.secondary;
    value.addr = (XtPointer) &(colors[i].bg);
    XrmPutResource (&db, "*XmDialogShell*background", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*background", "Pixel",&value);
    XrmPutResource (&db, "*XmCascadeButton*background", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*background", "Pixel", &value);

/*
 * keep foreground for athena widgets and 2.0 compatability.
 * DONT write out the other dependent colors in order to allow motif
 * to generate them.
 */
    value.addr = (XtPointer) &(colors[i].fg);
    XrmPutResource (&db, "*XmDialogShell*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*foreground", "Pixel",&value);
    XrmPutResource (&db, "*XmCascadeButton*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*foreground", "Pixel", &value);

#ifdef notdef
    value.addr = (XtPointer) &(colors[i].sc);
    XrmPutResource (&db, "*XmDialogShell*selectColor", "Pixel", &value);
    XrmPutResource (&db, "*XmDialogShell*armColor", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*selectColor", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*armColor", "Pixel", &value);

    value.addr = (XtPointer) &(colors[i].ts);
    XrmPutResource (&db, "*XmDialogShell*topShadowColor", "Pixel",&value);
    XrmPutResource (&db, "*XmMenuShell*topShadowColor", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButton*topShadowColor", "Pixel",&value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*topShadowColor", "Pixel",
                      &value);

    value.addr = (XtPointer) &(colors[i].bs);
    XrmPutResource (&db, "*XmDialogShell*bottomShadowColor", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*bottomShadowColor","Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButton*bottomShadowColor", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*bottomShadowColor",
                    "Pixel", &value);
#endif


    if (DitherTopShadow(tmpColorObj->color_obj.display, screen, &colors[i]))
    {
        XrmPutStringResource (&db, "*XmDialogShell*topShadowPixmap", DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*topShadowPixmap", DITHER);
        XrmPutStringResource (&db, "*XmCascadeButton*topShadowPixmap", 
                              DITHER);
        XrmPutStringResource (&db,"*XmCascadeButtonGadget*topShadowPixmap",
                              DITHER);
    }
    else if (DitherTopShadow(tmpColorObj->color_obj.display, screen, 
                             &(colors[tmpColorObj->color_obj.primary]))) 
    {
        XrmPutStringResource (&db, "*XmDialogShell*topShadowPixmap", NO_DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*topShadowPixmap", NO_DITHER);
        XrmPutStringResource (&db, "*XmCascadeButton*topShadowPixmap", 
                              NO_DITHER);
        XrmPutStringResource (&db,"*XmCascadeButtonGadget*topShadowPixmap",
                              NO_DITHER);
    }

    if (DitherBottomShadow(tmpColorObj->color_obj.display, screen, &colors[i])) 
    {
        XrmPutStringResource (&db, "*XmDialogShell*bottomShadowPixmap",
                              DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*bottomShadowPixmap",
                              DITHER);
        XrmPutStringResource (&db, "*XmCascadeButton*bottomShadowPixmap",
                              DITHER);
        XrmPutStringResource (&db, "*XmCascadeButtonGadget*bottomShadowPixmap",
                              DITHER);
    }
    else if (DitherBottomShadow(tmpColorObj->color_obj.display, screen, 
                             &(colors[tmpColorObj->color_obj.primary]))) 
    {
        XrmPutStringResource (&db, "*XmDialogShell*bottomShadowPixmap",
                              NO_DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*bottomShadowPixmap",
                              NO_DITHER);
        XrmPutStringResource (&db, "*XmCascadeButton*bottomShadowPixmap",
                              NO_DITHER);
        XrmPutStringResource (&db, "*XmCascadeButtonGadget*bottomShadowPixmap",
                              NO_DITHER);
    }


    /** set all of the text areas to the textColorSetId **/
    /** continue to write fg for backward compatibility **/

    if ( !tmpColorObj->color_obj.useText )
	return;

    doList = tmpColorObj->color_obj.useTextForList;

    i = tmpColorObj->color_obj.text;

    /** add this color to the Motif color cache **/
    cacheRec.screen = XtScreen(tmpColorObj);
    cacheRec.color_map = DefaultColormapOfScreen(cacheRec.screen);
    cacheRec.background.pixel = colors[i].bg;
    cacheRec.foreground.pixel = colors[i].fg;
    cacheRec.top_shadow.pixel = colors[i].ts;
    cacheRec.bottom_shadow.pixel = colors[i].bs;
    cacheRec.select.pixel = colors[i].sc;
    cacheRec.allocated = XmBACKGROUND | XmFOREGROUND | XmTOP_SHADOW | 
                         XmBOTTOM_SHADOW | XmSELECT;
    _XmAddToColorCache (&cacheRec);      /* would be nice to put RGB values */


    /** set for primary color areas **/

    value.addr = (XtPointer) &(colors[i].bg);
    XrmPutResource (&db, "*XmText*background", "Pixel", &value);
    XrmPutResource (&db, "*XmTextField*background", "Pixel", &value);
    XrmPutResource (&db, "*DtTerm*background", "Pixel", &value);
    if (doList) 
      XrmPutResource (&db, "*XmList*background", "Pixel", &value);

    value.addr = (XtPointer) &(colors[i].fg);
    XrmPutResource (&db, "*XmText*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmTextField*foreground", "Pixel", &value);
    XrmPutResource (&db, "*DtTerm*foreground", "Pixel", &value);
    if (doList) 
      XrmPutResource (&db, "*XmList*foreground", "Pixel", &value);

    /** set for all secondary color areas **/

    value.addr = (XtPointer) &(colors[i].bg);  /* do background first */

    XrmPutResource (&db, "*XmDialogShell*XmText*background", "Pixel", &value);
    XrmPutResource (&db, "*XmDialogShell*XmTextField*background", "Pixel", 
		    &value);
    XrmPutResource (&db, "*XmDialogShell*DtTerm*background", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*XmText*background", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*XmTextField*background", "Pixel",&value);
    XrmPutResource (&db, "*XmMenuShell*DtTerm*background", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButton*XmText*background", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButton*XmTextField*background", "Pixel", 
		    &value);
    XrmPutResource (&db, "*XmCascadeButton*DtTerm*background", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*XmText*background", "Pixel", 
		    &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*XmTextField*background", 
		    "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*DtTerm*background", "Pixel", 
		    &value);
    if (doList) {
      XrmPutResource (&db, "*XmDialogShell*XmList*background", "Pixel", &value);
      XrmPutResource (&db, "*XmMenuShell*XmList*background", "Pixel", &value);
    }

    value.addr = (XtPointer) &(colors[i].fg);  /* now do foreground */

    XrmPutResource (&db, "*XmDialogShell*XmText*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmDialogShell*XmTextField*foreground", "Pixel", 
		    &value);
    XrmPutResource (&db, "*XmDialogShell*DtTerm*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*XmText*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmMenuShell*XmTextField*foreground", "Pixel",&value);
    XrmPutResource (&db, "*XmMenuShell*DtTerm*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButton*XmText*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButton*XmTextField*foreground", "Pixel", 
		    &value);
    XrmPutResource (&db, "*XmCascadeButton*DtTerm*foreground", "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*XmText*foreground", "Pixel", 
		    &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*XmTextField*foreground", 
		    "Pixel", &value);
    XrmPutResource (&db, "*XmCascadeButtonGadget*DtTerm*foreground", "Pixel", 
		    &value);
    if (doList) {
      XrmPutResource (&db, "*XmDialogShell*XmList*foreground", "Pixel", &value);
      XrmPutResource (&db, "*XmMenuShell*XmList*foreground", "Pixel", &value);
    }

    /** dither (or "undither") top shadow if needed **/

    if (DitherTopShadow(tmpColorObj->color_obj.display, screen, &colors[i])) 
    {
        XrmPutStringResource (&db, "*XmText*topShadowPixmap", DITHER);
        XrmPutStringResource (&db, "*XmTextField*topShadowPixmap", DITHER);
        XrmPutStringResource (&db, "*DtTerm*topShadowPixmap", DITHER);
        if (doList) 
	  XrmPutStringResource (&db, "*XmList*topShadowPixmap", DITHER);

        XrmPutStringResource (&db, "*XmDialogShell*XmText*topShadowPixmap", 
			      DITHER);
        XrmPutStringResource (&db, "*XmDialogShell*XmTextField*topShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, "*XmDialogShell*DtTerm*topShadowPixmap", 
			      DITHER);
        if (doList) 
	  XrmPutStringResource (&db, "*XmDialogShell*XmList*topShadowPixmap", 
			      DITHER);

        XrmPutStringResource (&db, "*XmMenuShell*XmText*topShadowPixmap", 
			      DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*XmTextField*topShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*DtTerm*topShadowPixmap", 
			      DITHER);
        if (doList) 
	  XrmPutStringResource (&db, "*XmMenuShell*XmList*topShadowPixmap", 
			      DITHER);

        XrmPutStringResource (&db, "*XmCascadeButton*XmText*topShadowPixmap", 
			      DITHER);
        XrmPutStringResource (&db, 
			      "*XmCascadeButton*XmTextField*topShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, "*XmCascadeButton*DtTerm*topShadowPixmap", 
			      DITHER);

        XrmPutStringResource (&db, 
			      "*XmCascadeButtonGadget*XmText*topShadowPixmap", 
			      DITHER);
        XrmPutStringResource (&db, 
			   "*XmCascadeButtonGadget*XmTextField*topShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, 
			      "*XmCascadeButtonGadget*DtTerm*topShadowPixmap", 
		              DITHER);
    }
    else  /** undo dithers set for primary and secondary if needed **/
    {
      if (DitherTopShadow(tmpColorObj->color_obj.display, screen, 
                             &(colors[tmpColorObj->color_obj.primary]))) 
      {
        XrmPutStringResource (&db, "*XmText*topShadowPixmap", NO_DITHER);
        XrmPutStringResource (&db, "*XmTextField*topShadowPixmap", NO_DITHER);
        XrmPutStringResource (&db, "*DtTerm*topShadowPixmap", NO_DITHER);
        if (doList)
	  XrmPutStringResource (&db, "*XmList*topShadowPixmap", NO_DITHER);
      }
      if (DitherTopShadow(tmpColorObj->color_obj.display, screen, 
                             &(colors[tmpColorObj->color_obj.secondary]))) 
      {

        XrmPutStringResource (&db, "*XmDialogShell*XmText*topShadowPixmap", 
			      NO_DITHER);
        XrmPutStringResource (&db, "*XmDialogShell*XmTextField*topShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, "*XmDialogShell*DtTerm*topShadowPixmap", 
			      NO_DITHER);
        if (doList)
	  XrmPutStringResource (&db, "*XmDialogShell*XmList*topShadowPixmap", 
			      NO_DITHER);

        XrmPutStringResource (&db, "*XmMenuShell*XmText*topShadowPixmap", 
			      NO_DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*XmTextField*topShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*DtTerm*topShadowPixmap", 
			      NO_DITHER);
        if (doList)
	  XrmPutStringResource (&db, "*XmMenuShell*XmList*topShadowPixmap", 
			      NO_DITHER);

        XrmPutStringResource (&db, "*XmCascadeButton*XmText*topShadowPixmap", 
			      NO_DITHER);
        XrmPutStringResource (&db, 
			      "*XmCascadeButton*XmTextField*topShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, "*XmCascadeButton*DtTerm*topShadowPixmap", 
			      NO_DITHER);

        XrmPutStringResource (&db, 
			      "*XmCascadeButtonGadget*XmText*topShadowPixmap", 
			      NO_DITHER);
        XrmPutStringResource (&db, 
			   "*XmCascadeButtonGadget*XmTextField*topShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, 
			      "*XmCascadeButtonGadget*DtTerm*topShadowPixmap", 
		              NO_DITHER);
      }
    }

    /** dither (or "undither") bottom shadow if needed **/

    if (DitherBottomShadow(tmpColorObj->color_obj.display, screen, &colors[i])) 
    {
        XrmPutStringResource (&db, "*XmText*bottomShadowPixmap", DITHER);
        XrmPutStringResource (&db, "*XmTextField*bottomShadowPixmap", DITHER);
        XrmPutStringResource (&db, "*DtTerm*bottomShadowPixmap", DITHER);
        if (doList)
	  XrmPutStringResource (&db, "*XmList*bottomShadowPixmap", DITHER);

        XrmPutStringResource (&db, "*XmDialogShell*XmText*bottomShadowPixmap", 
			      DITHER);
        XrmPutStringResource (&db, 
			      "*XmDialogShell*XmTextField*bottomShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, "*XmDialogShell*DtTerm*bottomShadowPixmap", 
			      DITHER);
        if (doList)
	  XrmPutStringResource (&db, "*XmDialogShell*XmList*bottomShadowPixmap",
			      DITHER);

        XrmPutStringResource (&db, "*XmMenuShell*XmText*bottomShadowPixmap", 
			      DITHER);
        XrmPutStringResource (&db,"*XmMenuShell*XmTextField*bottomShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*DtTerm*bottomShadowPixmap", 
			      DITHER);
        if (doList)
	  XrmPutStringResource (&db, "*XmMenuShell*XmList*bottomShadowPixmap",
			      DITHER);

        XrmPutStringResource (&db, "*XmCascadeButton*XmText*bottomShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, 
			      "*XmCascadeButton*XmTextField*bottomShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, "*XmCascadeButton*DtTerm*bottomShadowPixmap",
			      DITHER);

        XrmPutStringResource (&db, 
			     "*XmCascadeButtonGadget*XmText*bottomShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, 
			"*XmCascadeButtonGadget*XmTextField*bottomShadowPixmap",
			      DITHER);
        XrmPutStringResource (&db, 
			     "*XmCascadeButtonGadget*DtTerm*bottomShadowPixmap",
		              DITHER);
    }
    else  /** undo dithers set for primary and secondary if needed **/
    {
      if (DitherBottomShadow(tmpColorObj->color_obj.display, screen, 
                             &(colors[tmpColorObj->color_obj.primary]))) 
      {
        XrmPutStringResource (&db, "*XmText*bottomShadowPixmap", NO_DITHER);
        XrmPutStringResource (&db, "*XmTextField*bottomShadowPixmap",NO_DITHER);
        XrmPutStringResource (&db, "*DtTerm*bottomShadowPixmap", NO_DITHER);
        if (doList)
	  XrmPutStringResource (&db, "*XmList*bottomShadowPixmap", NO_DITHER);
      }
      if (DitherBottomShadow(tmpColorObj->color_obj.display, screen, 
                             &(colors[tmpColorObj->color_obj.secondary]))) 
      {

        XrmPutStringResource (&db, "*XmDialogShell*XmText*bottomShadowPixmap", 
			      NO_DITHER);
        XrmPutStringResource (&db, 
			      "*XmDialogShell*XmTextField*bottomShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, "*XmDialogShell*DtTerm*bottomShadowPixmap", 
			      NO_DITHER);
        if (doList)
	  XrmPutStringResource (&db, "*XmDialogShell*XmList*bottomShadowPixmap",
			      NO_DITHER);

        XrmPutStringResource (&db, "*XmMenuShell*XmText*bottomShadowPixmap", 
			      NO_DITHER);
        XrmPutStringResource (&db,"*XmMenuShell*XmTextField*bottomShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, "*XmMenuShell*DtTerm*bottomShadowPixmap", 
			      NO_DITHER);
        if (doList) 
	  XrmPutStringResource (&db, "*XmMenuShell*XmList*bottomShadowPixmap", 
			      NO_DITHER);

        XrmPutStringResource (&db, "*XmCascadeButton*XmText*bottomShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, 
			      "*XmCascadeButton*XmTextField*bottomShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, "*XmCascadeButton*DtTerm*bottomShadowPixmap",
			      NO_DITHER);

        XrmPutStringResource (&db, 
			     "*XmCascadeButtonGadget*XmText*bottomShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, 
			"*XmCascadeButtonGadget*XmTextField*bottomShadowPixmap",
			      NO_DITHER);
        XrmPutStringResource (&db, 
			     "*XmCascadeButtonGadget*DtTerm*bottomShadowPixmap",
		              NO_DITHER);
      }
    }
}


/**********************************************************************/
/** _XmGetPixelData()                                                **/
/**              pixelSet should be an array of NUM_COLORS PixelSets **/
/**              ie.  PixelSet  pixelSet[NUM_COLORS];                **/
/**                                                                  **/
/**********************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmGetPixelData( screen, colorUse, pixelSet, a, i, p, s )
        int screen ;
        int *colorUse ;
        PixelSet *pixelSet ;
        short *a ;
        short *i ;
        short *p ;
        short *s ;
#else
_XmGetPixelData(
        int screen,
        int *colorUse,
        PixelSet *pixelSet,
        short *a,
        short *i,
        short *p,
        short *s )
#endif /* _NO_PROTO */
{
    ColorObj tmpColorObj = DefaultColorObj;
    int k;

    /* return False if color srv is not running, or color obj not used */
    if (!tmpColorObj ||
	!tmpColorObj->color_obj.useColorObj || 
	!tmpColorObj->color_obj.colorIsRunning)
      return False;

    /* return False if screen invalid */
    if (screen < 0 || screen >= tmpColorObj->color_obj.numScreens) 
        return False;

    *colorUse = tmpColorObj->color_obj.colorUse[screen];

    for (k = 0; k < NUM_COLORS; k++)
    {
        pixelSet[k].fg = tmpColorObj->color_obj.colors[screen][k].fg;
        pixelSet[k].bg = tmpColorObj->color_obj.colors[screen][k].bg;
        pixelSet[k].ts = tmpColorObj->color_obj.colors[screen][k].ts;
        pixelSet[k].bs = tmpColorObj->color_obj.colors[screen][k].bs;
        pixelSet[k].sc = tmpColorObj->color_obj.colors[screen][k].sc;
    }

    *a = (short)tmpColorObj->color_obj.active;
    *i = (short)tmpColorObj->color_obj.inactive;
    *p = (short)tmpColorObj->color_obj.primary;
    *s = (short)tmpColorObj->color_obj.secondary;

    return True;
}


/**********************************************************************/
/** _XmGetIconControlInfo                                            **/
/**              Get information needed for XmpGetIconFileName       **/
/**********************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmGetIconControlInfo( screen, useMaskRtn, useMultiColorIconsRtn, useIconFileCacheRtn)
        Screen  *screen ;
        Boolean *useMaskRtn;
        Boolean *useMultiColorIconsRtn;
        Boolean *useIconFileCacheRtn;
#else
_XmGetIconControlInfo(
        Screen  *screen,
	Boolean *useMaskRtn,
        Boolean *useMultiColorIconsRtn,
        Boolean *useIconFileCacheRtn)
#endif /* _NO_PROTO */
{
    ColorObj tmpColorObj = DefaultColorObj;

    /* return False if color srv is not running, or color obj not used */
    if (!tmpColorObj || !tmpColorObj->color_obj.colorIsRunning ||
	!tmpColorObj->color_obj.useColorObj) 
    {
	*useMaskRtn = *useMultiColorIconsRtn = *useIconFileCacheRtn = True;
        return False;
    }
    *useMaskRtn = tmpColorObj->color_obj.useMask;
    *useMultiColorIconsRtn = tmpColorObj->color_obj.useMultiColorIcons;
    *useIconFileCacheRtn = tmpColorObj->color_obj.useIconFileCache;
    return True;
}

/**********************************************************************/
/** _XmUseColorObj()                                                 **/
/**           Return False if color is not working for some reason.  **/
/**                                                                  **/
/**           Could be due to useColorObj resource == False, or any  **/
/**           problem with the color server or color object.         **/
/**                                                                  **/
/**********************************************************************/
Boolean 
#ifdef _NO_PROTO
_XmUseColorObj()
#else
_XmUseColorObj( void )
#endif /* _NO_PROTO */
{
    ColorObj tmpColorObj = DefaultColorObj;

    if (!tmpColorObj ||
      !tmpColorObj->color_obj.colorIsRunning ||
      !tmpColorObj->color_obj.useColorObj)
      return False;
    else
      return True;
}
