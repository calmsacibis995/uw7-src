#pragma ident	"@(#)m1.2libs:Xm/Protocols.c	1.3"
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

#include <Xm/ProtocolsP.h>
#include "XmI.h"
#include "MessagesI.h"
#include "CallbackI.h"


#ifdef I18N_MSG
#include "XmMsgI.h"
#endif


#ifdef I18N_MSG
#define MSG1	catgets(Xm_catd,MS_PRotocol,MSG_PR_1,_XmMsgProtocols_0000)
#define MSG2	catgets(Xm_catd,MS_PRotocol,MSG_PR_2,_XmMsgProtocols_0001)
#define MSG3	catgets(Xm_catd,MS_PRotocol,MSG_PR_3,_XmMsgProtocols_0002)
#else
#define MSG1	_XmMsgProtocols_0000
#define MSG2	_XmMsgProtocols_0001
#define MSG3	_XmMsgProtocols_0002
#endif


#define XmCR_PROTOCOLS	6666	/* needs to be somewhere else */
#define MAX_PROTOCOLS		32
#define PROTOCOL_BLOCK_SIZE	4

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassInitialize() ;
static void ClassPartInitialize() ;
static void Initialize() ;
static void Destroy() ;
static void RemoveAllPMgrHandler() ;
static void RemoveAllPMgr() ;
static XmAllProtocolsMgr GetAllProtocolsMgr() ;
static void UpdateProtocolMgrProperty() ;
static void InstallProtocols() ;
static void RealizeHandler() ;
static void ProtocolHandler() ;
static XmProtocol GetProtocol() ;
static XmProtocolMgr AddProtocolMgr() ;
static XmProtocolMgr GetProtocolMgr() ;
static void RemoveProtocolMgr() ;
static void AddProtocols() ;
static void RemoveProtocols() ;

#else

static void ClassInitialize( void ) ;
static void ClassPartInitialize( 
                        WidgetClass w) ;
static void Initialize( 
                        Widget req,
                        Widget new_w,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget w) ;
static void RemoveAllPMgrHandler( 
                        Widget w,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *continue_to_dispatch) ;
static void RemoveAllPMgr( 
                        Widget w,
                        XtPointer closure,
                        XtPointer call_data) ;
static XmAllProtocolsMgr GetAllProtocolsMgr( 
                        Widget shell) ;
static void UpdateProtocolMgrProperty( 
                        Widget shell,
                        XmProtocolMgr p_mgr) ;
static void InstallProtocols( 
                        Widget w,
                        XmAllProtocolsMgr ap_mgr) ;
static void RealizeHandler( 
                        Widget w,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static void ProtocolHandler( 
                        Widget w,
                        XtPointer closure,
                        XEvent *event,
                        Boolean *cont) ;
static XmProtocol GetProtocol( 
                        XmProtocolMgr p_mgr,
                        Atom p_atom) ;
static XmProtocolMgr AddProtocolMgr( 
                        XmAllProtocolsMgr ap_mgr,
                        Atom property) ;
static XmProtocolMgr GetProtocolMgr( 
                        XmAllProtocolsMgr ap_mgr,
                        Atom property) ;
static void RemoveProtocolMgr( 
                        XmAllProtocolsMgr ap_mgr,
                        XmProtocolMgr p_mgr) ;
static void AddProtocols( 
                        Widget shell,
                        XmProtocolMgr p_mgr,
                        Atom *protocols,
                        Cardinal num_protocols) ;
static void RemoveProtocols( 
                        Widget shell,
                        XmProtocolMgr p_mgr,
                        Atom *protocols,
                        Cardinal num_protocols) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/***************************************************************************
 *
 * ProtocolObject Resources
 *
 ***************************************************************************/

static XContext	allProtocolsMgrContext = (XContext) NULL;


#define Offset(field) XtOffsetOf( struct _XmProtocolRec, protocol.field)

static XtResource protocolResources[] =
{
    {
	XmNextensionType,
	XmCExtensionType, XmRExtensionType, sizeof (unsigned char),
	XtOffsetOf( struct _XmExtRec, ext.extensionType),
	XmRImmediate, (XtPointer)XmPROTOCOL_EXTENSION,
    },
    {
	XmNprotocolCallback,
	XmCProtocolCallback, XmRCallback, sizeof (XtCallbackList),
	Offset (callbacks),
	XmRImmediate, (XtPointer)NULL,
    },
#ifdef notdef
    {
	XmNatom,
	XmCAtom, XmRAtom, sizeof (Atom),
	Offset (atom),
	XmRImmediate, (XtPointer)NULL,
    },
    {
	XmNactive,
	XmCActive, XmRBoolean, sizeof (Boolean),
	Offset (active),
	XmRImmediate, (XtPointer)FALSE,
    },
#endif /* notdef */
};
#undef Offset


externaldef(xmprotocolclassrec)
XmProtocolClassRec xmProtocolClassRec = {
    {	
	(WidgetClass) &xmExtClassRec,/* superclass 		*/   
	"protocol",			/* class_name 		*/   
	sizeof(XmProtocolRec),	 	/* size 		*/   
	ClassInitialize, 		/* Class Initializer 	*/   
	ClassPartInitialize, 		/* class_part_init 	*/ 
	FALSE, 				/* Class init'ed ? 	*/   
	Initialize, 			/* initialize         	*/   
	NULL, 				/* initialize_notify    */ 
	NULL,	 			/* realize            	*/   
	NULL,	 			/* actions            	*/   
	0,				/* num_actions        	*/   
	protocolResources,		/* resources          	*/   
	XtNumber(protocolResources),	/* resource_count     	*/   
	NULLQUARK, 			/* xrm_class          	*/   
	FALSE, 				/* compress_motion    	*/   
	FALSE, 				/* compress_exposure  	*/   
	FALSE, 				/* compress_enterleave	*/   
	FALSE, 				/* visible_interest   	*/   
	Destroy,			/* destroy            	*/   
	NULL,		 		/* resize             	*/   
	NULL, 				/* expose             	*/   
	NULL,		 		/* set_values         	*/   
	NULL, 				/* set_values_hook      */ 
	NULL,			 	/* set_values_almost    */ 
	NULL,				/* get_values_hook      */ 
	NULL, 				/* accept_focus       	*/   
	XtVersion, 			/* intrinsics version 	*/   
	NULL, 				/* callback offsets   	*/   
	NULL,				/* tm_table           	*/   
	NULL, 				/* query_geometry       */ 
	NULL, 				/* display_accelerator  */ 
	NULL, 				/* extension            */ 
    },	
    {
	NULL,				/* synthetic resources	*/
	0,				/* num syn resources	*/
    },
    {
	NULL,				/* extension		*/
    },
};

externaldef(xmprotocolobjectclass) WidgetClass 
  xmProtocolObjectClass = (WidgetClass) (&xmProtocolClassRec);

/************************************************************************
 *
 *  ClassInitialize
 *    Initialize the vendorShell class structure.  This is called only
 *    the first time a vendorShell widget is created.  It registers the
 *    resource type converters unique to this class.
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
ClassInitialize()
#else
ClassInitialize( void )
#endif /* _NO_PROTO */
{

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
    XmProtocolObjectClass wc = (XmProtocolObjectClass) w;
    
    if (wc == (XmProtocolObjectClass)xmProtocolObjectClass)
      return;
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
    XmProtocol				ne = (XmProtocol) new_w;
    XmWidgetExtData			extData;

    /*
     * we should free this in ExtObject's destroy proc, but since all
     * gadgets would need to change to not free it in thier code we'll
     * do it here. |||
     */
    extData = _XmGetWidgetExtData(ne->ext.logicalParent,
				  ne->ext.extensionType);
    _XmExtObjFree(extData->reqWidget);
    extData->reqWidget = NULL;
}

/************************************************************************
 *
 *  Destroy
 *
 ************************************************************************/
static void 
#ifdef _NO_PROTO
Destroy( w )
        Widget w ;
#else
Destroy(
        Widget w )
#endif /* _NO_PROTO */
{
}

static void
#ifdef _NO_PROTO
RemoveAllPMgrHandler( w, closure, event, continue_to_dispatch)
        Widget w ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *continue_to_dispatch ;
#else
RemoveAllPMgrHandler(
        Widget w,
        XtPointer closure,
        XEvent *event,
        Boolean *continue_to_dispatch)
#endif /* _NO_PROTO */
{   
    XmAllProtocolsMgr ap_mgr = (XmAllProtocolsMgr) closure ;
    Cardinal	i;
    
    for (i = 0; i < ap_mgr->num_protocol_mgrs; i++)
      {
	  RemoveProtocolMgr(ap_mgr, ap_mgr->protocol_mgrs[i]);
      }
    /* free the context manager entry ||| */
    XDeleteContext(XtDisplay(w), 
		   (Window)w, 
		   allProtocolsMgrContext);
    XtFree((char *)ap_mgr->protocol_mgrs);
    XtFree((char *)ap_mgr);

    *continue_to_dispatch = False;
    return ;
    } 

/************************************<+>*************************************
 *
 *   RemoveAllPMgr
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
RemoveAllPMgr( w, closure, call_data )
        Widget w ;
        XtPointer closure ;
        XtPointer call_data ;
#else
RemoveAllPMgr(
        Widget w,
        XtPointer closure,
        XtPointer call_data )
#endif /* _NO_PROTO */
{ 
	XKeyEvent ev ;
        Boolean save_sensitive = w->core.sensitive ;
        Boolean save_ancestor_sensitive = w->core.ancestor_sensitive ;

    XtInsertEventHandler( w, KeyPressMask, TRUE, RemoveAllPMgrHandler,
                                                         closure, XtListHead) ;
    ev.type = KeyPress ;
    ev.display = XtDisplay( w) ;
    ev.time = XtLastTimestampProcessed( XtDisplay( w)) ;
    ev.send_event = True ;
    ev.serial = LastKnownRequestProcessed( XtDisplay( w)) ;
    ev.window = XtWindow( w) ;

    /* make sure we get it even if we're unrealized, or if widget
     * is insensitive.
     */
    XtAddGrab( w, True, True) ;
    w->core.sensitive = TRUE ;
    w->core.ancestor_sensitive = TRUE ;
    XtDispatchEvent( (XEvent *) &ev) ;
    w->core.sensitive = save_sensitive ;
    w->core.ancestor_sensitive = save_ancestor_sensitive ;
    XtRemoveGrab( w) ;

    XtRemoveEventHandler(w, (EventMask)NULL, TRUE, RemoveAllPMgrHandler,
			 closure) ;
    }

/************************************<+>*************************************
 *
 *   GetAllProtocolsMgr
 *
 *************************************<+>************************************/
static XmAllProtocolsMgr 
#ifdef _NO_PROTO
GetAllProtocolsMgr( shell )
        Widget shell ;
#else
GetAllProtocolsMgr(
        Widget shell )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr;
    Display		*display;
    
    if (!XmIsVendorShell(shell))
      {
	  _XmWarning(NULL, MSG1);
	  return ((XmAllProtocolsMgr)0);
      }
    else
      {
	  display = XtDisplay(shell);
	  
	  if (allProtocolsMgrContext == (XContext) NULL)
	    allProtocolsMgrContext = XUniqueContext();
	  
	  if (XFindContext(display,
			   (Window) shell,
			   allProtocolsMgrContext,
			   (char **)&ap_mgr))
	    {
		ap_mgr = XtNew(XmAllProtocolsMgrRec);
		
		ap_mgr->shell = shell;
		ap_mgr->num_protocol_mgrs = 
		  ap_mgr->max_protocol_mgrs = 0;
		ap_mgr->protocol_mgrs = NULL;
		(void) XSaveContext(display, 
				    (Window) shell, 
				    allProtocolsMgrContext, 
				    (XPointer) ap_mgr);
		
		/* !!! should this be in some init code for vendor shell ? */
		/* if shell isn't realized, add an event handler for everybody */
		
		if (!XtIsRealized(shell))
		  {
		      XtAddEventHandler((Widget) shell, StructureNotifyMask,
                                    FALSE, RealizeHandler, (XtPointer) ap_mgr);
		  }
		XtAddCallback((Widget) shell, XmNdestroyCallback, 
                                             RemoveAllPMgr, (XtPointer)ap_mgr);
		
	    }
	  return ap_mgr;
      }
}
/************************************<+>*************************************
 *
 *   SetProtocolProperty
 *
 *************************************<+>************************************/
#define SetProtocolProperty(shell, property, prop_type, atoms, num_atoms) \
  XChangeProperty((shell)->core.screen->display, XtWindow(shell), \
		  property, prop_type, 32, PropModeReplace, \
		  atoms, num_atoms)


/************************************<+>*************************************
 *
 *   UpdateProtocolMgrProperty
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
UpdateProtocolMgrProperty( shell, p_mgr )
        Widget shell ;
        XmProtocolMgr p_mgr ;
#else
UpdateProtocolMgrProperty(
        Widget shell,
        XmProtocolMgr p_mgr )
#endif /* _NO_PROTO */
{
    Cardinal	i, num_active = 0;
    static Atom	active_protocols[MAX_PROTOCOLS];
    XmProtocolList	protocols = p_mgr->protocols;
    
    for (i = 0; i < p_mgr->num_protocols; i++) {
	if (protocols[i]->protocol.active)
	  active_protocols[num_active++] = protocols[i]->protocol.atom;
    }
    SetProtocolProperty(shell, p_mgr->property, XA_ATOM, 
		(unsigned char *)active_protocols, num_active);
}


/************************************<+>*************************************
 *
 *   InstallProtocols
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
InstallProtocols( w, ap_mgr )
        Widget w ;
        XmAllProtocolsMgr ap_mgr ;
#else
InstallProtocols(
        Widget w,
        XmAllProtocolsMgr ap_mgr )
#endif /* _NO_PROTO */
{
    Cardinal		i;
    
    XtAddRawEventHandler(w, (EventMask)0, TRUE, 
			 ProtocolHandler, (XtPointer) ap_mgr);
    XtRemoveEventHandler(w,StructureNotifyMask , FALSE, 
			 RealizeHandler, ap_mgr);
    
    for (i=0; i < ap_mgr->num_protocol_mgrs; i++)
      UpdateProtocolMgrProperty(w, ap_mgr->protocol_mgrs[i]);
    
}

/************************************<+>*************************************
 *
 *   RealizeHandler
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
RealizeHandler( w, closure, event, cont )
        Widget w ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
RealizeHandler(
        Widget w,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr = (XmAllProtocolsMgr)closure;
    
    switch (event->type) 
      {
	case MapNotify:
	  InstallProtocols(w, ap_mgr);
	default:
	  break;
      }
}

/************************************<+>*************************************
 *
 *   ProtocolHandler
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
ProtocolHandler( w, closure, event, cont )
        Widget w ;
        XtPointer closure ;
        XEvent *event ;
        Boolean *cont ;
#else
ProtocolHandler(
        Widget w,
        XtPointer closure,
        XEvent *event,
        Boolean *cont )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr = (XmAllProtocolsMgr)closure;
    XmProtocolMgr	p_mgr;
    XmProtocol		protocol;
    XmAnyCallbackStruct	call_data_rec;
    XtCallbackProc	func;
    
    call_data_rec.reason = XmCR_PROTOCOLS;
    call_data_rec.event = event;
    
    switch (event->type) 
      {
	case ClientMessage:
	  {
	      XClientMessageEvent	*p_event = (XClientMessageEvent *) event;
	      Atom			p_atom = (Atom) p_event->data.l[0];
	      
	      if (((p_mgr = GetProtocolMgr(ap_mgr, (Atom)p_event->message_type)) 
		  == (XmProtocolMgr)0) ||
		  ((protocol = GetProtocol(p_mgr, p_atom)) == (XmProtocol)0))
		return;
	      else {
		  if ((func = protocol->protocol.pre_hook.callback) != (XtCallbackProc)0)
		    (*func) (w, protocol->protocol.pre_hook.closure, (XtPointer) &call_data_rec);
		  
		  if (protocol->protocol.callbacks)
		    _XmCallCallbackList(w,
				       protocol->protocol.callbacks, 
				       (XtPointer) &call_data_rec);
		  
		  if ((func = protocol->protocol.post_hook.callback) != (XtCallbackProc)0)
		    (*func) (w, protocol->protocol.post_hook.closure, (XtPointer) &call_data_rec);
	      }
	      break;
	    default:
	      break;
	  }
      }
}



/************************************<+>*************************************
 *
 *   GetProtocol
 *
 *************************************<+>************************************/
static XmProtocol 
#ifdef _NO_PROTO
GetProtocol( p_mgr, p_atom )
        XmProtocolMgr p_mgr ;
        Atom p_atom ;
#else
GetProtocol(
        XmProtocolMgr p_mgr,
        Atom p_atom )
#endif /* _NO_PROTO */
{
    Cardinal	i;
    XmProtocol	protocol;
    
    for (i = 0; 
	 i < p_mgr->num_protocols && p_mgr->protocols[i]->protocol.atom != p_atom;
	 i++){}
    
    if (i < p_mgr->num_protocols)
      {
	  protocol = p_mgr->protocols[i];
      }
    else 
      {
	  protocol = (XmProtocol)0;
      }
    return(protocol);
}


/************************************<+>*************************************
 *
 *   AddProtocolMgr
 *
 *************************************<+>************************************/
static XmProtocolMgr 
#ifdef _NO_PROTO
AddProtocolMgr( ap_mgr, property )
        XmAllProtocolsMgr ap_mgr ;
        Atom property ;
#else
AddProtocolMgr(
        XmAllProtocolsMgr ap_mgr,
        Atom property )
#endif /* _NO_PROTO */
{
    XmProtocolMgr	p_mgr;
    Cardinal		i;
    
    for (i = 0; 
	 i < ap_mgr->num_protocol_mgrs &&
	 ap_mgr->protocol_mgrs[i]->property != property;
	 i++){}
    
    if (i < ap_mgr->num_protocol_mgrs)
      {
	  _XmWarning(NULL, MSG2);
      }
    
    if (ap_mgr->num_protocol_mgrs + 2 >= ap_mgr->max_protocol_mgrs) 
      {
	  ap_mgr->max_protocol_mgrs += 2;
	  ap_mgr->protocol_mgrs = (XmProtocolMgrList) 
	    XtRealloc((char *) ap_mgr->protocol_mgrs ,
		      ((unsigned) (ap_mgr->max_protocol_mgrs) 
		       * sizeof(XmProtocolMgr)));
      }
    ap_mgr->protocol_mgrs[ap_mgr->num_protocol_mgrs++] 
      = p_mgr = XtNew(XmProtocolMgrRec);
    
    p_mgr->property = property;
    p_mgr->num_protocols =
      p_mgr->max_protocols = 0;
    
    p_mgr->protocols = NULL;
    
    return(p_mgr);
}
/************************************<+>*************************************
 *
 *   GetProtcolMgr
 *
 *************************************<+>************************************/
static XmProtocolMgr 
#ifdef _NO_PROTO
GetProtocolMgr( ap_mgr, property )
        XmAllProtocolsMgr ap_mgr ;
        Atom property ;
#else
GetProtocolMgr(
        XmAllProtocolsMgr ap_mgr,
        Atom property )
#endif /* _NO_PROTO */
{
    XmProtocolMgr	p_mgr = (XmProtocolMgr)0;
    Cardinal		i;
    
    if (!ap_mgr) return p_mgr;
    
    for (i = 0; 
	 i < ap_mgr->num_protocol_mgrs &&
	 ap_mgr->protocol_mgrs[i]->property != property;
	 i++){}
    
    if (i < ap_mgr->num_protocol_mgrs)
      {
	  p_mgr = ap_mgr->protocol_mgrs[i];
      }
    else
      p_mgr = (XmProtocolMgr)0;

    return p_mgr;
}


/************************************<+>*************************************
 *
 *   RemoveProtocolMgr
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
RemoveProtocolMgr( ap_mgr, p_mgr )
        XmAllProtocolsMgr ap_mgr ;
        XmProtocolMgr p_mgr ;
#else
RemoveProtocolMgr(
        XmAllProtocolsMgr ap_mgr,
        XmProtocolMgr p_mgr )
#endif /* _NO_PROTO */
{
    Widget	shell = ap_mgr->shell;
    Cardinal 	i;
    
    for (i = 0; i < p_mgr->num_protocols; i++)
      {
          _XmRemoveAllCallbacks(
		(InternalCallbackList *)&(p_mgr->protocols[i]->protocol.callbacks) );
          XtFree((char *) p_mgr->protocols[i]);
      }
    if (XtIsRealized(shell))
	XDeleteProperty(XtDisplay(shell), 
			XtWindow(shell), 
			p_mgr->property);
    
    for (i = 0;  i < ap_mgr->num_protocol_mgrs;  i++)
      if (ap_mgr->protocol_mgrs[i] == p_mgr)
	break;

    XtFree((char *) p_mgr->protocols);
    XtFree((char *) p_mgr);

    /* ripple mgrs down */
    for ( ; i < ap_mgr->num_protocol_mgrs-1; i++)
      ap_mgr->protocol_mgrs[i] = ap_mgr->protocol_mgrs[i+1];
}
/************************************<+>*************************************
 *
 *  AddProtocols
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
AddProtocols( shell, p_mgr, protocols, num_protocols )
        Widget shell ;
        XmProtocolMgr p_mgr ;
        Atom *protocols ;
        Cardinal num_protocols ;
#else
AddProtocols(
        Widget shell,
        XmProtocolMgr p_mgr,
        Atom *protocols,
        Cardinal num_protocols )
#endif /* _NO_PROTO */
{	
    Cardinal		new_num_protocols, i, j;
    XtPointer           newSec;
    WidgetClass         wc;
    Cardinal            size;

    wc = XtClass(shell);
    size = wc->core_class.widget_size;
    
    new_num_protocols = p_mgr->num_protocols + num_protocols;
    
    if (new_num_protocols >= p_mgr->max_protocols) 
      {
	  /* Allocate more space */
	  Cardinal	add_size;
	  
	  if (num_protocols >= PROTOCOL_BLOCK_SIZE)
	    add_size = num_protocols + PROTOCOL_BLOCK_SIZE;
	  else
	    add_size = PROTOCOL_BLOCK_SIZE;
	  
	  p_mgr->max_protocols +=  add_size;
	  p_mgr->protocols = (XmProtocolList) 
	    XtRealloc((char *) p_mgr->protocols ,
		      (unsigned) (p_mgr->max_protocols) * sizeof(XmProtocol));
      }
    
    for (i = p_mgr->num_protocols, j = 0;
	 i < new_num_protocols; 
	 i++,j++)
      {
	  
          newSec = XtMalloc(size);

          ((XmProtocol) newSec)->protocol.atom = protocols[j];
	  ((XmProtocol)newSec)->protocol.active = TRUE; /*default */
	  ((XmProtocol)newSec)->protocol.callbacks = (XtCallbackList)0;
	  ((XmProtocol)newSec)->protocol.pre_hook.callback = 
          ((XmProtocol)newSec)->protocol.post_hook.callback = (XtCallbackProc)0;
	  ((XmProtocol)newSec)->protocol.pre_hook.closure = 
	  ((XmProtocol)newSec)->protocol.post_hook.closure = (XtPointer)0;

          p_mgr->protocols[i] = (XmProtocol)newSec;
      }
    p_mgr->num_protocols = new_num_protocols;
    
}

/************************************<+>*************************************
 *
 *   RemoveProtocols
 *
 *************************************<+>************************************/
static void 
#ifdef _NO_PROTO
RemoveProtocols( shell, p_mgr, protocols, num_protocols )
        Widget shell ;
        XmProtocolMgr p_mgr ;
        Atom *protocols ;
        Cardinal num_protocols ;
#else
RemoveProtocols(
        Widget shell,
        XmProtocolMgr p_mgr,
        Atom *protocols,
        Cardinal num_protocols )
#endif /* _NO_PROTO */
{
    static Boolean	match_list[MAX_PROTOCOLS];
    Cardinal		i, j;
    
    if (!p_mgr || !p_mgr->num_protocols || !num_protocols) return;
    
    if (p_mgr->num_protocols > MAX_PROTOCOLS)
      _XmWarning(NULL, MSG3);
    
    for (i = 0; i <= p_mgr->num_protocols; i++)
      match_list[i] = FALSE;
    
    /* setup the match list */
    for (i = 0; i < num_protocols; i++)
      {
	  for (j = 0 ; 
	       ((j < p_mgr->num_protocols) &&
		(p_mgr->protocols[j]->protocol.atom != protocols[i]));
	       j++) 
	    {};
	  if (j < p_mgr->num_protocols)
	    match_list[j] = TRUE;
      }
    
    /* 
     * keep only the protocols that arent in the match list. 
     */
    for (j = 0, i = 0; i < p_mgr->num_protocols; i++)
      {
	  if ( ! match_list[i] ) {
	      p_mgr->protocols[j] = p_mgr->protocols[i];
	      j++;
	  }
	  else 
          {
            _XmRemoveAllCallbacks((InternalCallbackList *) &(p_mgr->protocols[i]->protocol.callbacks));
            XtFree((char *) p_mgr->protocols[i]);
          }
      }

    p_mgr->num_protocols = j;
    
}




/*
 *  
 * PUBLIC INTERFACES
 *
 */


/************************************<+>*************************************
 *
 *   _XmInstallProtocols
 *
 *************************************<+>************************************/
void 
#ifdef _NO_PROTO
_XmInstallProtocols( w )
        Widget w ;
#else
_XmInstallProtocols(
        Widget w )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr;

    if ((ap_mgr = GetAllProtocolsMgr(w)) != NULL)
      InstallProtocols(w, ap_mgr);
}



/************************************<+>*************************************
 *
 *   XmAddProtocols
 *
 *************************************<+>************************************/
void 
#ifdef _NO_PROTO
XmAddProtocols( shell, property, protocols, num_protocols )
        Widget shell ;
        Atom property ;
        Atom *protocols ;
        Cardinal num_protocols ;
#else
XmAddProtocols(
        Widget shell,
        Atom property,
        Atom *protocols,
        Cardinal num_protocols )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr; 
    XmProtocolMgr	p_mgr ;
   
    if (shell->core.being_destroyed)
        return;
    if (((ap_mgr = GetAllProtocolsMgr(shell)) == 0) ||	!num_protocols)
      return;
    if ((p_mgr = GetProtocolMgr(ap_mgr, property)) == 0)
      p_mgr = AddProtocolMgr(ap_mgr, property);

    /* get rid of duplicates and then append to end */
    RemoveProtocols(shell, p_mgr, protocols, num_protocols);
    AddProtocols(shell, p_mgr, protocols, num_protocols);
    
    if (XtIsRealized(shell))
      UpdateProtocolMgrProperty(shell, p_mgr);
}



/************************************<+>*************************************
 *
 *   XmRemoveProtocols
 *
 *************************************<+>************************************/
void 
#ifdef _NO_PROTO
XmRemoveProtocols( shell, property, protocols, num_protocols )
        Widget shell ;
        Atom property ;
        Atom *protocols ;
        Cardinal num_protocols ;
#else
XmRemoveProtocols(
        Widget shell,
        Atom property,
        Atom *protocols,
        Cardinal num_protocols )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr; 
    XmProtocolMgr	p_mgr ;
   
    if (shell->core.being_destroyed)
        return;
    if (((ap_mgr = GetAllProtocolsMgr(shell)) == 0) 		||
	((p_mgr = GetProtocolMgr(ap_mgr, property)) == 0) 	||
	!num_protocols)
      return;

    
    RemoveProtocols(shell, p_mgr, protocols, num_protocols);

    if (XtIsRealized(shell))
      UpdateProtocolMgrProperty(shell, p_mgr);
}

/************************************<+>*************************************
 *
 *   XmAddProtocolCallback
 *
 *************************************<+>************************************/
void 
#ifdef _NO_PROTO
XmAddProtocolCallback( shell, property, proto_atom, callback, closure )
        Widget shell ;
        Atom property ;
        Atom proto_atom ;
        XtCallbackProc callback ;
        XtPointer closure ;
#else
XmAddProtocolCallback(
        Widget shell,
        Atom property,
        Atom proto_atom,
        XtCallbackProc callback,
        XtPointer closure )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr; 
    XmProtocolMgr	p_mgr ;
    XmProtocol		protocol;
   
    if (shell->core.being_destroyed)
         return;
    if ((ap_mgr = GetAllProtocolsMgr(shell)) == (XmAllProtocolsMgr)0)
      return;	
    if ((p_mgr = GetProtocolMgr(ap_mgr, property)) == (XmProtocolMgr)0)
      p_mgr = AddProtocolMgr(ap_mgr, property);
    if ((protocol = GetProtocol(p_mgr, proto_atom)) == (XmProtocol)0)
      {
	  XmAddProtocols(shell, property, &proto_atom, 1);
	  protocol = GetProtocol(p_mgr, proto_atom);
      }

    _XmAddCallback((InternalCallbackList *) &(protocol->protocol.callbacks),
                    callback,
                    closure) ;
}

/************************************<+>*************************************
 *
 *   XmRemoveProtocolCallback
 *
 *************************************<+>************************************/
void 
#ifdef _NO_PROTO
XmRemoveProtocolCallback( shell, property, proto_atom, callback, closure )
        Widget shell ;
        Atom property ;
        Atom proto_atom ;
        XtCallbackProc callback ;
        XtPointer closure ;
#else
XmRemoveProtocolCallback(
        Widget shell,
        Atom property,
        Atom proto_atom,
        XtCallbackProc callback,
        XtPointer closure )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr; 
    XmProtocolMgr	p_mgr ;
    XmProtocol		protocol;
   
    if (shell->core.being_destroyed)
        return;
    if (((ap_mgr = GetAllProtocolsMgr(shell)) == 0) 		||
	((p_mgr = GetProtocolMgr(ap_mgr, property)) == 0) 	||
	((protocol = GetProtocol(p_mgr, proto_atom)) == 0))
      return;

    _XmRemoveCallback((InternalCallbackList *) &(protocol->protocol.callbacks),
                    callback,
                    closure) ;
}

/************************************<+>*************************************
 *
 *   XmActivateProtocol
 *
 *************************************<+>************************************/
void 
#ifdef _NO_PROTO
XmActivateProtocol( shell, property, proto_atom )
        Widget shell ;
        Atom property ;
        Atom proto_atom ;
#else
XmActivateProtocol(
        Widget shell,
        Atom property,
        Atom proto_atom )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr; 
    XmProtocolMgr	p_mgr ;
    XmProtocol		protocol;

    if (shell->core.being_destroyed)
         return;
    if (((ap_mgr = GetAllProtocolsMgr(shell)) == 0) 		||
	((p_mgr = GetProtocolMgr(ap_mgr, property)) == 0) 	||
	((protocol = GetProtocol(p_mgr, proto_atom)) == 0) 	||
	protocol->protocol.active)
      return;
    else
      {
	  protocol->protocol.active = TRUE;
	  if (XtIsRealized(shell))
	    UpdateProtocolMgrProperty(shell, p_mgr);
      }
}

/************************************<+>*************************************
 *
 *   XmDeactivateProtocol
 *
 *************************************<+>************************************/
void 
#ifdef _NO_PROTO
XmDeactivateProtocol( shell, property, proto_atom )
        Widget shell ;
        Atom property ;
        Atom proto_atom ;
#else
XmDeactivateProtocol(
        Widget shell,
        Atom property,
        Atom proto_atom )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr; 
    XmProtocolMgr	p_mgr ;
    XmProtocol		protocol;
   
    if (shell->core.being_destroyed)
        return;
    if (((ap_mgr = GetAllProtocolsMgr(shell)) == 0) 		||
	((p_mgr = GetProtocolMgr(ap_mgr, property)) == 0) 	||
	((protocol = GetProtocol(p_mgr, proto_atom)) == 0) 	||
	!protocol->protocol.active)
      return;
    else
      {
	  protocol->protocol.active = FALSE;
	  if (XtIsRealized(shell))
	    UpdateProtocolMgrProperty(shell, p_mgr);
      }
}

/************************************<+>*************************************
 *
 *   XmSetProtocolHooks
 *
 *************************************<+>************************************/
void 
#ifdef _NO_PROTO
XmSetProtocolHooks( shell, property, proto_atom, pre_hook, pre_closure, post_hook, post_closure )
        Widget shell ;
        Atom property ;
        Atom proto_atom ;
        XtCallbackProc pre_hook ;
        XtPointer pre_closure ;
        XtCallbackProc post_hook ;
        XtPointer post_closure ;
#else
XmSetProtocolHooks(
        Widget shell,
        Atom property,
        Atom proto_atom,
        XtCallbackProc pre_hook,
        XtPointer pre_closure,
        XtCallbackProc post_hook,
        XtPointer post_closure )
#endif /* _NO_PROTO */
{
    XmAllProtocolsMgr	ap_mgr; 
    XmProtocolMgr	p_mgr ;
    XmProtocol		protocol;

    if (shell->core.being_destroyed)
        return;
    if (((ap_mgr = GetAllProtocolsMgr(shell)) == 0) 		||
	((p_mgr = GetProtocolMgr(ap_mgr, property)) == 0) 	||
	((protocol = GetProtocol(p_mgr, proto_atom)) == 0))
      return;
    
    protocol->protocol.pre_hook.callback = pre_hook;
    protocol->protocol.pre_hook.closure = pre_closure;
    protocol->protocol.post_hook.callback = post_hook;
    protocol->protocol.post_hook.closure = post_closure;
}
