#pragma ident	"@(#)m1.2libs:Xm/ExtObject.c	1.4"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993, 1994 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.4
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1989, 1990  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*
*  (c) Copyright 1988 MICROSOFT CORPORATION */

#include <Xm/ExtObjectP.h>
#include <Xm/BaseClassP.h>
#include <string.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInitPrehook() ;
static void ClassPartInitPosthook() ;
static void ClassPartInitialize() ;
static void InitializePrehook() ;
static void Initialize() ;
static Boolean SetValuesPrehook() ;
static void GetValuesPrehook() ;
static Boolean SetValues() ;
static void GetValuesHook() ;
static void Destroy() ;
static void UseParent() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInitPrehook( 
                        WidgetClass w) ;
static void ClassPartInitPosthook( 
                        WidgetClass w) ;
static void ClassPartInitialize( 
                        WidgetClass w) ;
static void InitializePrehook( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Initialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValuesPrehook( 
                        Widget req,
                        Widget curr,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesPrehook( 
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValues( 
                        Widget old,
                        Widget ref,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void GetValuesHook( 
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget wid) ;
static void UseParent( 
                        Widget w,
                        int offset,
                        XrmValue *value) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/

/***************************************************************************
 *
 * ExtObject Resources
 *
 ***************************************************************************/


#define Offset(field) XtOffsetOf( struct _XmExtRec, ext.field)

static XtResource extResources[] =
{
    {
	XmNlogicalParent,
	XmCLogicalParent, XmRWidget, sizeof (Widget),
	Offset (logicalParent),
	XmRCallProc, (XtPointer)UseParent,
    },
    {
	XmNextensionType,
	XmCExtensionType, XmRExtensionType, sizeof (unsigned char),
	Offset (extensionType),
	XmRImmediate, (XtPointer)XmDEFAULT_EXTENSION,
    },
};
#undef Offset

typedef union {
  XmExtCache	cache;
  double	force_alignment;
} Aligned_XmExtCache;

static Aligned_XmExtCache extarray[XmNUM_ELEMENTS];


static XmBaseClassExtRec       myBaseClassExtRec = {
    NULL,                                     /* Next extension       */
    NULLQUARK,                                /* record type XmQmotif */
    XmBaseClassExtVersion,                    /* version              */
    sizeof(XmBaseClassExtRec),                /* size                 */
    InitializePrehook,		              /* initialize prehook   */
    SetValuesPrehook,		              /* set_values prehook   */
    NULL,			              /* initialize posthook  */
    NULL,			              /* set_values posthook  */
    NULL,				      /* secondary class      */
    NULL,			              /* creation proc        */
    NULL,	                              /* getSecRes data       */
    {0},                                      /* fast subclass        */
    GetValuesPrehook,		              /* get_values prehook   */
    NULL,			              /* get_values posthook  */
    ClassPartInitPrehook,		      /* class_part_prehook   */
    ClassPartInitPosthook,		      /* class_part_posthook  */
    NULL,	 			      /* compiled_ext_resources*/   
    NULL,	 			      /* ext_resources       	*/   
    0,					      /* resource_count     	*/   
    FALSE,				      /* use_sub_resources	*/
};

externaldef(xmextclassrec)
XmExtClassRec xmExtClassRec = {
    {	
	(WidgetClass) &objectClassRec, /* superclass 		*/   
	"dynamic",			/* class_name 		*/   
	sizeof(XmExtRec),	 	/* size 		*/   
	ClassInitialize, 		/* Class Initializer 	*/   
	ClassPartInitialize, 		/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	Initialize, 			/* initialize         	*/   
	NULL, 				/* initialize_notify    */ 
	NULL,	 			/* realize            	*/   
	NULL,	 			/* actions            	*/   
	0,				/* num_actions        	*/   
	extResources, 			/* resources          	*/   
	XtNumber(extResources),		/* resource_count     	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	FALSE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	Destroy,			/* destroy            	*/   
	NULL,		 		/* resize             	*/   
	NULL, 				/* expose             	*/   
	SetValues,	 		/* set_values         	*/   
	NULL, 				/* set_values_hook      */ 
	NULL,			 	/* set_values_almost    */ 
	GetValuesHook,			/* get_values_hook      */ 
	NULL, 				/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	NULL,				/* tm_table           	*/   
	NULL, 				/* query_geometry       */ 
	NULL, 				/* display_accelerator  */ 
	(XtPointer)&myBaseClassExtRec,/* extension         */ 
    },	
    {
	NULL,				/* synthetic resources	*/
	0,				/* num syn resources	*/
    },
};

externaldef(xmextobjectclass) WidgetClass 
  xmExtObjectClass = (WidgetClass) (&xmExtClassRec);


/************************************************************************
 *
 *  ClassInitialize
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{
    myBaseClassExtRec.record_type = XmQmotif;
}

/************************************************************************
 *
 *  ClassPartInitPrehook
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitPrehook( w )
        WidgetClass w ;
#else
ClassPartInitPrehook(
        WidgetClass w )
#endif /* _NO_PROTO */
{
    XmExtObjectClass wc = (XmExtObjectClass) w;
    
    if ((WidgetClass)wc != xmExtObjectClass)
      {
	  XmBaseClassExt	*scePtr;
	  XmExtObjectClass sc = (XmExtObjectClass) w->core_class.superclass;

	  scePtr = _XmGetBaseClassExtPtr(sc, XmQmotif);
	  /*
	   * if our superclass uses sub resources, then we need to
	   * temporarily fill it's core resource fields so that objectClass
	   * classPartInit will be able to find them for merging. We
	   * assume that we only need to set things up for the
	   * superclass and not any deeper ancestors
	   */
	  if ((*scePtr)->use_sub_resources)
	    {
		sc->object_class.resources = 
		  (*scePtr)->compiled_ext_resources;
		sc->object_class.num_resources = 
		  (*scePtr)->num_ext_resources;
	    }
      }
}
/************************************************************************
 *
 *  ClassPartInitPosthook
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassPartInitPosthook( w )
        WidgetClass w ;
#else
ClassPartInitPosthook(
        WidgetClass w )
#endif /* _NO_PROTO */
{
    XmExtObjectClass 	wc = (XmExtObjectClass) w;
    XmBaseClassExt	*wcePtr;

    wcePtr = _XmGetBaseClassExtPtr(wc, XmQmotif);
    
    if ((WidgetClass)wc != xmExtObjectClass)
      {
	  XmExtObjectClass sc = (XmExtObjectClass) w->core_class.superclass;
	  XmBaseClassExt	*scePtr;

	  scePtr = _XmGetBaseClassExtPtr(sc, XmQmotif);

	  if ((*scePtr) && (*scePtr)->use_sub_resources)
	    {
#ifdef CALLBACKS_USE_RES_LIST
		sc->object_class.resources = NULL;
		sc->object_class.num_resources = 0;
#endif
	    }
      }
    if ((*wcePtr) && (*wcePtr)->use_sub_resources)
      {
	  /*
	   * put our compiled resources back and zero out oject class so
	   * it's invisible to object class create processing
	   */
	  (*wcePtr)->compiled_ext_resources =
	    wc->object_class.resources;
	  (*wcePtr)->num_ext_resources = 
	    wc->object_class.num_resources;
#ifdef CALLBACKS_USE_RES_LIST
	  /*
	   * Xt currently uses the offset table to do callback
	   * resource processing. This means that we can't use this
	   * trick of nulling out the resource list in order to avoid
	   * an extra round of resource processing
	   */
	  wc->object_class.resources = NULL;
	  wc->object_class.num_resources = 0;
#endif
      }
}

/************************************************************************
 *
 *  ClassPartInitialize
 *    Set up the inheritance mechanism for the routines exported by
 *    vendorShells class part.
 *
 ************************************************************************/

static void 
#ifdef _NO_PROTO
ClassPartInitialize( w )
        WidgetClass w ;
#else
ClassPartInitialize(
        WidgetClass w )
#endif /* _NO_PROTO */
{
    XmExtObjectClass wc = (XmExtObjectClass) w;
    
    if (wc == (XmExtObjectClass)xmExtObjectClass)
      return;
    _XmBaseClassPartInitialize(w);
    _XmBuildExtResources((WidgetClass) wc);
}

static void 
#ifdef _NO_PROTO
InitializePrehook( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
InitializePrehook(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmBaseClassExt		*wcePtr;
    XmExtObjectClass		ec = (XmExtObjectClass) XtClass(new_w);


    wcePtr = _XmGetBaseClassExtPtr(ec, XmQmotif);

    if ((*wcePtr)->use_sub_resources)
      {
	  /*
	   * get a uncompiled resource list to use with
	   * XtGetSubresources. We can't do this in
	   * ClassPartInitPosthook because Xt doesn't set class_inited at
	   * the right place and thereby mishandles the
	   * XtGetResourceList call
	   */
	  if ((*wcePtr)->ext_resources == NULL)
	    {
		ec->object_class.resources =
		  (*wcePtr)->compiled_ext_resources;
		ec->object_class.num_resources =		
		  (*wcePtr)->num_ext_resources;

		XtGetResourceList((WidgetClass) ec,
				  &((*wcePtr)->ext_resources),
				  &((*wcePtr)->num_ext_resources));

#ifdef CALLBACKS_USE_RES_LIST
		ec->object_class.resources = NULL;
		ec->object_class.num_resources = 0;
#endif /* CALLBACKS_USE_RES_LIST */
	    }
	  XtGetSubresources(XtParent(new_w),
			    (XtPointer)new_w,
			    NULL, NULL,
			    (*wcePtr)->ext_resources,
			    (*wcePtr)->num_ext_resources,
			    args, *num_args);
      }
}

static void 
#ifdef _NO_PROTO
Initialize( req, new_w, args, num_args )
        Widget req ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
Initialize(
        Widget req,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmBaseClassExt		*wcePtr;
    XmExtObject			ne = (XmExtObject) new_w;
    XmExtObjectClass		ec = (XmExtObjectClass) XtClass(new_w);
    Widget			resParent = ne->ext.logicalParent;
    XmWidgetExtData		extData;


    wcePtr = _XmGetBaseClassExtPtr(ec, XmQmotif);

    if (!(*wcePtr)->use_sub_resources)
      {
	  if (resParent)
	    {
		extData = (XmWidgetExtData) XtCalloc(1, sizeof(XmWidgetExtDataRec));
		_XmPushWidgetExtData(resParent, extData, ne->ext.extensionType);
		
		extData->widget = new_w;
		extData->reqWidget = (Widget)
		  _XmExtObjAlloc(XtClass(new_w)->core_class.widget_size);
		memcpy((char *)extData->reqWidget, (char *)req,
		      XtClass(new_w)->core_class.widget_size);
		
		/*  Convert the fields from unit values to pixel values  */
		
		_XmExtImportArgs(new_w, args, num_args);
	    }
      }
}

static Boolean 
#ifdef _NO_PROTO
SetValuesPrehook( req, curr, new_w, args, num_args )
        Widget req ;
        Widget curr ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValuesPrehook(
        Widget req,
        Widget curr,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmExtObjectClass		ec = (XmExtObjectClass) XtClass(new_w);
    XmBaseClassExt		*wcePtr;

    wcePtr = _XmGetBaseClassExtPtr(ec, XmQmotif);

    if ((*wcePtr)->use_sub_resources)
      {
	  XtSetSubvalues((XtPointer)new_w,
			 (*wcePtr)->ext_resources,
			 (*wcePtr)->num_ext_resources,
			 args, *num_args);
      }
    return False;
}

static void 
#ifdef _NO_PROTO
GetValuesPrehook( new_w, args, num_args )
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesPrehook(
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmExtObjectClass		ec = (XmExtObjectClass) XtClass(new_w);
    XmBaseClassExt		*wcePtr;

    wcePtr = _XmGetBaseClassExtPtr(ec, XmQmotif);

    if ((*wcePtr)->use_sub_resources)
      {
	  XtGetSubvalues((XtPointer)new_w,
			 (*wcePtr)->ext_resources,
			 (*wcePtr)->num_ext_resources,
			 args, *num_args);
      }
}

/************************************************************************
 *
 *  SetValues
 *
 ************************************************************************/
/* ARGSUSED */
static Boolean 
#ifdef _NO_PROTO
SetValues( old, ref, new_w, args, num_args )
        Widget old ;
        Widget ref ;
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
SetValues(
        Widget old,
        Widget ref,
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmExtObject			ne = (XmExtObject) new_w;
    Widget			resParent = ne->ext.logicalParent;
    XmWidgetExtData		ext = _XmGetWidgetExtData(resParent, ne->ext.extensionType);
    Cardinal			extSize;

    if (resParent)
      {
	  extSize = XtClass(new_w)->core_class.widget_size;
	  
	  ext->widget = new_w;
	  
	  ext->oldWidget = (Widget) _XmExtObjAlloc(extSize);
	  memcpy((char *)ext->oldWidget, (char *)old, extSize); 
	  
	  ext->reqWidget = (Widget) _XmExtObjAlloc(extSize);
	  memcpy((char *)ext->reqWidget, (char *)ref, extSize); 
	  
	  /*  Convert the necessary fields from unit values to pixel values  */
	  
	  _XmExtImportArgs(new_w, args, num_args);
      }
    return FALSE;
}

/************************************************************************
 *
 *  GetValuesHook
 *
 ************************************************************************/
/* ARGSUSED */
static void 
#ifdef _NO_PROTO
GetValuesHook( new_w, args, num_args )
        Widget new_w ;
        ArgList args ;
        Cardinal *num_args ;
#else
GetValuesHook(
        Widget new_w,
        ArgList args,
        Cardinal *num_args )
#endif /* _NO_PROTO */
{
    XmExtObject			ne = (XmExtObject) new_w;
    Widget			resParent = ne->ext.logicalParent;
    
    if (resParent)
      {
	  XmWidgetExtData		ext =
	    _XmGetWidgetExtData(resParent, ne->ext.extensionType);
	  
	  ext->widget = new_w;
	  
	  _XmExtGetValuesHook(new_w, args, num_args);
      }
}

/************************************************************************
 *
 *  Destroy
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Destroy( wid )
        Widget wid ;
#else
Destroy(
        Widget wid )
#endif /* _NO_PROTO */
{
        XmExtObject extObj = (XmExtObject) wid ;
    Widget			resParent = extObj->ext.logicalParent;
 
    if (resParent)
      {
	  XmWidgetExtData		extData;
	  
	  _XmPopWidgetExtData(resParent, 
			      &extData,
			      extObj->ext.extensionType);

#ifdef notdef
	  /*
	   * we can't do this here cause the gadgets have already
	   * freed it |||
	   */
	  XtFree(extData->reqWidget);
#endif /* notdef */
	  XtFree((char *) extData);
      }
}

char * 
#ifdef _NO_PROTO
_XmExtObjAlloc( size )
        int size ;
#else
_XmExtObjAlloc(
        int size )
#endif /* _NO_PROTO */
{
  short i;

  if (size <= XmNUM_BYTES)
    {
      for (i = 0; i < XmNUM_ELEMENTS; i++)
	if (! extarray[i].cache.inuse)
	  {
	    extarray[i].cache.inuse = TRUE;
	    return extarray[i].cache.data;
	  }
    }

  return XtMalloc(size);
}

void 
#ifdef _NO_PROTO
_XmExtObjFree( element )
        XtPointer element ;
#else
_XmExtObjFree(
        XtPointer element )
#endif /* _NO_PROTO */
{
 short i;

 for (i = 0; i < XmNUM_ELEMENTS; i++)
   if (extarray[i].cache.data == element)
     {
       extarray[i].cache.inuse = FALSE;
       return;
     }
 XtFree((char *) element);
}

static void 
#ifdef _NO_PROTO
UseParent( w, offset, value )
        Widget w ;
        int offset ;
        XrmValue *value ;
#else
UseParent(
        Widget w,
        int offset,
        XrmValue *value )
#endif /* _NO_PROTO */
{
    value->addr = (XPointer) &(w->core.parent);
}

/**********************************************************************
 *
 *  _XmBuildExtResources
 *	Build up the ext's synthetic 
 *	resource processing list by combining the super classes with 
 *	this class.
 *
 **********************************************************************/
void 
#ifdef _NO_PROTO
_XmBuildExtResources( c )
        WidgetClass c ;
#else
_XmBuildExtResources(
        WidgetClass c )
#endif /* _NO_PROTO */
{
        XmExtObjectClass wc = (XmExtObjectClass) c ;
	XmExtObjectClass 		sc;

	_XmInitializeSyntheticResources(wc->ext_class.syn_resources,
				     wc->ext_class.num_syn_resources);

	if (wc != (XmExtObjectClass) xmExtObjectClass)
	  {
	      sc = (XmExtObjectClass) wc->object_class.superclass;

	      _XmBuildResources (&(wc->ext_class.syn_resources),
			      &(wc->ext_class.num_syn_resources),
			      sc->ext_class.syn_resources,
			      sc->ext_class.num_syn_resources);
	  }
}
