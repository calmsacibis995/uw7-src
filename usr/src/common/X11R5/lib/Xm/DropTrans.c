#pragma ident	"@(#)m1.2libs:Xm/DropTrans.c	1.3"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include <Xm/DropTransP.h>
#include <Xm/DragCP.h>
#include "DragCI.h"
#include "DragICCI.h"
#include <Xm/AtomMgr.h>
#include <stdio.h>


/********    Static Function Declarations    ********/
#ifdef _NO_PROTO

static void ClassPartInit() ;
static void Initialize() ;
static Boolean SetValues() ;
static void Destroy() ;
static void SourceNotifiedCB() ;
static void TerminateTransfer() ;
static void ProcessTransferEntry() ;
static void DropTransferSelectionCB() ;
static Widget StartDropTransfer() ;
static void AddDropTransfer() ;

#else

static void ClassPartInit( 
                        WidgetClass wc) ;
static void Initialize( 
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static Boolean SetValues( 
                        Widget cw,
                        Widget rw,
                        Widget nw,
                        ArgList args,
                        Cardinal *num_args) ;
static void Destroy( 
                        Widget w) ;
static void SourceNotifiedCB( 
                        Widget w,
                        XtPointer client_data,
                        Atom *selection,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static void TerminateTransfer( 
                        XmDropTransferObject dt,
                        Atom *selection) ;
static void ProcessTransferEntry( 
                        XmDropTransferObject dt,
                        Cardinal which) ;
static void DropTransferSelectionCB( 
                        Widget w,
                        XtPointer client_data,
                        Atom *selection,
                        Atom *type,
                        XtPointer value,
                        unsigned long *length,
                        int *format) ;
static Widget StartDropTransfer( 
                        Widget refWidget,
                        ArgList args,
                        Cardinal argCount) ;
static void AddDropTransfer( 
                        Widget widget,
                        XmDropTransferEntry transfers,
                        Cardinal num_transfers) ;

#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


static XtResource resources[] = {
	{   XmNdropTransfers, XmCDropTransfers,
		XmRDropTransfers, sizeof(XmDropTransferEntry),
		XtOffsetOf( struct _XmDropTransferRec,
			dropTransfer.drop_transfers),
		XmRImmediate, (XtPointer) NULL
	},
	{   XmNnumDropTransfers, XmCNumDropTransfers,
		XmRCardinal, sizeof(Cardinal),
		XtOffsetOf( struct _XmDropTransferRec,
			dropTransfer.num_drop_transfers),
		XmRImmediate, (XtPointer) 0
	},
	{   XmNincremental, XmCIncremental, XmRBoolean, sizeof(Boolean),
		XtOffsetOf( struct _XmDropTransferRec, dropTransfer.incremental),
		XmRImmediate, (XtPointer) FALSE
	},
	{   XmNtransferProc, XmCTransferProc,
		XmRCallbackProc, sizeof(XtSelectionCallbackProc),
		XtOffsetOf( struct _XmDropTransferRec, dropTransfer.transfer_callback),
		XmRImmediate, (XtPointer) 0
	},
	{   XmNtransferStatus, XmCTransferStatus, XmRTransferStatus,
		sizeof(unsigned char),
		XtOffsetOf( struct _XmDropTransferRec, dropTransfer.transfer_status),
		XmRImmediate, (XtPointer) XmTRANSFER_SUCCESS
	},
};


/*  class record definition  */

externaldef(xmgadgetclassrec) XmDropTransferClassRec
	xmDropTransferClassRec = {
   {
      (WidgetClass) &objectClassRec,    /* superclass	         */	
      "XmDropTransfer",                 /* class_name	         */	
      sizeof(XmDropTransferRec),        /* widget_size	         */	
      NULL,                             /* class_initialize      */
      ClassPartInit,                    /* class part initialize */
      False,                  		/* class_inited          */	
      Initialize,                      	/* initialize	         */	
      NULL,                             /* initialize_hook       */
      NULL,	                       	/* obj1  	         */	
      NULL,				/* obj2                  */	
      0,				/* obj3	                 */	
      resources,                        /* resources	         */	
      XtNumber(resources),              /* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      True,                             /* obj4                  */
      True,           			/* obj5                  */	
      True,                             /* obj6                  */
      False,                            /* obj7                  */
      Destroy,                          /* destroy               */	
      NULL,                             /* obj8                  */	
      NULL,				/* obj9                  */	
      SetValues,                        /* set_values	         */	
      NULL,                             /* set_values_hook       */
      NULL,         			/* obj10                 */
      NULL,                             /* get_values_hook       */
      NULL,                             /* obj11    	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      NULL,                             /* obj12                 */
      NULL,                             /* obj13                 */
      NULL,				/* obj14                 */
      NULL,                             /* extension             */
   },
   {
      StartDropTransfer,           	/* start_drop_transfer   */
      AddDropTransfer,             	/* add_drop_transfer     */
      NULL,                        	/* extension             */
   },
};

externaldef(xmdroptransferobjectclass) WidgetClass
	xmDropTransferObjectClass = (WidgetClass)
		&xmDropTransferClassRec;

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
ClassPartInit( wc )
        WidgetClass wc ;
#else
ClassPartInit(
        WidgetClass wc )
#endif /* _NO_PROTO */
{
#ifdef notdef
	_XmFastSubclassInit(wc, XmDROP_TRANSFER_BIT);
#endif /* notdef */
}

/*ARGSUSED*/
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
	XmDropTransferObject new_w = (XmDropTransferObject) nw;
	XmDropTransferPart *dtp =
		(XmDropTransferPart *) &(new_w->dropTransfer);

	if (dtp->num_drop_transfers != 0)
	{
		dtp->num_drop_transfer_lists = 1;
		dtp->drop_transfer_lists = (XmDropTransferList)
			XtMalloc(sizeof(XmDropTransferListRec) *
				dtp->num_drop_transfer_lists);
		dtp->drop_transfer_lists[0].transfer_list =
		        (XmDropTransferEntry)_XmAllocAndCopy(
			dtp->drop_transfers, sizeof(XmDropTransferEntryRec)
				* dtp->num_drop_transfers);
		dtp->drop_transfer_lists[0].num_transfers =
			dtp->num_drop_transfers;
		
		/* strictly for hygene... */

		dtp->drop_transfers = dtp->drop_transfer_lists[0].transfer_list;
	}
	else
	{
		dtp->drop_transfer_lists = NULL;
		dtp->num_drop_transfer_lists = 0;
	}

	dtp->motif_drop_atom = XmInternAtom(
		XtDisplayOfObject(nw), "_MOTIF_DROP", False);

	dtp->cur_drop_transfer_list = (Cardinal) -1;
	dtp->cur_xfer = (Cardinal) -1;
	dtp->cur_targets = (Atom *) NULL;
	dtp->cur_client_data = (XtPointer *) NULL;
}

/*ARGSUSED*/
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
	/* Stub */
	/* !!! Should disallow any changes !!! */
	return True;
}

static void 
#ifdef _NO_PROTO
Destroy( w )
        Widget w ;
#else
Destroy(
        Widget w )
#endif /* _NO_PROTO */
{
    XmDropTransferObject new_w = (XmDropTransferObject) w;
    Cardinal 		 i;
    XmDragContext	 dc;

    /*
     * clean up the hanging dragContext 
     */
    dc = (XmDragContext)XmGetDragContext((Widget)new_w, 
					 new_w->dropTransfer.timestamp);
    if (dc && dc->drag.sourceIsExternal)
      XtDestroyWidget((Widget)dc);

    for (i = 0; i < new_w->dropTransfer.num_drop_transfer_lists; i++)
      {
	  XmDropTransferList	currEntry =
	    &(new_w->dropTransfer.drop_transfer_lists[i]);
	  
	  XtFree((char *)currEntry->transfer_list);
      }
    XtFree((char *)new_w->dropTransfer.drop_transfer_lists);
}

/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
SourceNotifiedCB( w, client_data, selection, type, value, length, format )
        Widget w ;
        XtPointer client_data ;
        Atom *selection ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
SourceNotifiedCB(
        Widget w,
        XtPointer client_data,
        Atom *selection,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
    XmDropTransferObject	dt = (XmDropTransferObject)client_data;

    if (value != NULL)
      XtFree((char *) value);
    /* self-immolution aaaaaaiii */
    XtDestroyWidget((Widget)dt);
}

static void 
#ifdef _NO_PROTO
TerminateTransfer( dt, selection )
        XmDropTransferObject dt ;
        Atom *selection ;
#else
TerminateTransfer(
        XmDropTransferObject dt,
        Atom *selection )
#endif /* _NO_PROTO */
{
	Atom status;
	XmDropTransferPart *dtp =
		(XmDropTransferPart *) &(dt->dropTransfer);
	XmDragContext	dc = (XmDragContext) dtp->dragContext;

	if (dtp->transfer_status == XmTRANSFER_SUCCESS)
		status = XmInternAtom(XtDisplayOfObject((Widget)dt),
			"XmTRANSFER_SUCCESS", False);
	else /* XmTRANSFER_FAILURE */
		status = XmInternAtom(XtDisplayOfObject((Widget)dt),
			"XmTRANSFER_FAILURE", False);
	
	/*
	 * we need to pass in the shell since that is the only widget
	 * visible to the initiator.
	 */

	XtGetSelectionValue(dc->drag.currReceiverInfo->shell,
				*selection, status,
				SourceNotifiedCB, (XtPointer)dt,
				dtp->timestamp);
}

static void 
#ifdef _NO_PROTO
ProcessTransferEntry( dt, which )
        XmDropTransferObject dt ;
        Cardinal which ;
#else
ProcessTransferEntry(
        XmDropTransferObject dt,
        Cardinal which )
#endif /* _NO_PROTO */
{
	XmDropTransferPart *dtp =
		(XmDropTransferPart *) &(dt->dropTransfer);
	XmDropTransferList	tl = &(dtp->drop_transfer_lists[which]);
	XmDragContext dc = (XmDragContext)dtp->dragContext;
	Cardinal i;
	Arg args[1];
	Atom real_selection_atom;

	dtp->cur_drop_transfer_list = which;
	dtp->cur_targets = (Atom *)
		XtMalloc((tl->num_transfers * sizeof(Atom)));
	dtp->cur_client_data = (XtPointer *)
		XtMalloc((tl->num_transfers * sizeof(XtPointer)));

	i = 0;
	XtSetArg(args[i], XmNiccHandle, &real_selection_atom); i++;
	XtGetValues(dtp->dragContext, args, i);

	for (i=0; i < tl->num_transfers; i++)
	{
		dtp->cur_targets[i] = tl->transfer_list[i].target;
		/* 
		 * all of the client data have to point to us so that we can
		 * bootstrap 
		 */
		dtp->cur_client_data[i] = (XtPointer)dt;
	}

	dtp->cur_xfer = 0;

	/*
	 * we need to pass in the destShell since that is the only widget
	 * visible to the initiator.
	 */

	if (dtp->incremental)
	{
		XtGetSelectionValuesIncremental(
			dc->drag.currReceiverInfo->shell,
			real_selection_atom, dtp->cur_targets,
			tl->num_transfers, DropTransferSelectionCB,
			(XtPointer *)dtp->cur_client_data, 
			dtp->timestamp);
	}
	else
	{
		XtGetSelectionValues(dc->drag.currReceiverInfo->shell,
			real_selection_atom, dtp->cur_targets,
			tl->num_transfers, DropTransferSelectionCB,
			(XtPointer *)dtp->cur_client_data, 
			dtp->timestamp);
	}
}


/* 
 * This routine is called with the shell as the widget and us as the
 * client data. We can't pass ourselves since we're not windowed
 */
/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
DropTransferSelectionCB( w, client_data, selection, type, value, length, format )
        Widget w ;
        XtPointer client_data ;
        Atom *selection ;
        Atom *type ;
        XtPointer value ;
        unsigned long *length ;
        int *format ;
#else
DropTransferSelectionCB(
        Widget w,
        XtPointer client_data,
        Atom *selection,
        Atom *type,
        XtPointer value,
        unsigned long *length,
        int *format )
#endif /* _NO_PROTO */
{
	XmDropTransferObject dt = (XmDropTransferObject) client_data;
	XmDropTransferPart *dtp =
		(XmDropTransferPart *) &(dt->dropTransfer);
	XmDropTransferList	tl =
		&(dtp->drop_transfer_lists[dtp->cur_drop_transfer_list]);


	(*(dtp->transfer_callback)) 
		((Widget)dt, tl->transfer_list[dtp->cur_xfer].client_data,
			selection, type, value, length, format);

       /* The transfer list needs to be reassigned at this point in case
	* an XmDropTransferAdd() was called in the callback.
        */
	tl = &(dtp->drop_transfer_lists[dtp->cur_drop_transfer_list]);

	if ( !(dtp->incremental)
		||
		((dtp->incremental) && (value != NULL) && (*length == 0)))
	{
		dtp->cur_xfer++;

		if (dtp->cur_xfer == tl->num_transfers)
		{
			XtFree((char *)dtp->cur_targets);
			XtFree((char *)dtp->cur_client_data);

			if (++(dtp->cur_drop_transfer_list) <
				dtp->num_drop_transfer_lists)
			{
			/* Get the next transfer entry in the list and process it */
				ProcessTransferEntry(dt, dtp->cur_drop_transfer_list);
			}
			else /* notify the source that we're done */
				TerminateTransfer(dt, selection);
		}
	}
}


/*ARGSUSED*/
static void 
#ifdef _NO_PROTO
StartDropTimer( clientData, id )
        XtPointer clientData ;
        XtIntervalId *id ;
#else
StartDropTimer(
        XtPointer clientData,
        XtIntervalId *id )
#endif /* _NO_PROTO */
{
	XmDropTransferObject dt = (XmDropTransferObject)clientData;
	XmDropTransferPart *dtp;
	Arg my_args[1];
	int i;
	Atom selection;
	
	dtp = (XmDropTransferPart *) &(dt->dropTransfer);

	if (dtp->num_drop_transfer_lists)
	{
		/* Get the first transfer entry in the list and process it */
		ProcessTransferEntry(dt, 0);
	}
	else /* notify the source that we've changed our mind */
	{
		i = 0;
		XtSetArg(my_args[i], XmNiccHandle, &selection); i++;
		XtGetValues(dtp->dragContext, my_args, i);

		TerminateTransfer(dt, &selection);
	}
}

static Widget 
#ifdef _NO_PROTO
StartDropTransfer( refWidget, args, argCount )
        Widget refWidget ;
        ArgList args ;
        Cardinal argCount ;
#else
StartDropTransfer(
        Widget refWidget,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
	static int which = 0;
	XmDropTransferObject dt;
	char buf[30];
	
	sprintf(buf, "Transfer%d", which++);
	dt = (XmDropTransferObject) XtCreateWidget(buf,
		xmDropTransferObjectClass,
		(Widget) XmGetXmDisplay(XtDisplayOfObject(refWidget)),
		args, argCount);

	dt->dropTransfer.dragContext = refWidget;
	dt->dropTransfer.timestamp =
	  ((XmDragContext)refWidget)->drag.dragFinishTime;

	/*
	 * The processing of the dropTransfer should happen after the
	 * dropStart message is echoed to the initiator. Since we're
	 * being called out of the dropProc of the receiver we can't
	 * just proceed or else the entire transfer could happen
	 * inline. We therefore add ourselves as a zero length timer
	 * which will allow us to get called after the dropProc has
	 * returned. 
	 */

	XtAppAddTimeOut(XtWidgetToApplicationContext( (Widget)dt),
			0, StartDropTimer, (XtPointer)dt);

	return((Widget) dt);
}



Widget 
#ifdef _NO_PROTO
XmDropTransferStart( refWidget, args, argCount )
        Widget refWidget ;
        ArgList args ;
        Cardinal argCount ;
#else
XmDropTransferStart(
        Widget refWidget,
        ArgList args,
        Cardinal argCount )
#endif /* _NO_PROTO */
{
	Widget ddo = (Widget) XmGetXmDisplay(XtDisplayOfObject(refWidget));
	XmDropTransferObjectClass wc;
	Arg lclArgs[1];
	int n;

	n = 0;
	XtSetArg(lclArgs[n], XmNdropTransferClass, &wc); n++;
	XtGetValues(ddo, lclArgs, n);

	return ((*(wc->dropTransfer_class.start_drop_transfer))
		(refWidget, args, argCount));
}



static void 
#ifdef _NO_PROTO
AddDropTransfer( widget, transfers, num_transfers )
        Widget widget ;
        XmDropTransferEntry transfers ;
        Cardinal num_transfers ;
#else
AddDropTransfer(
        Widget widget,
        XmDropTransferEntry transfers,
        Cardinal num_transfers )
#endif /* _NO_PROTO */
{
        XmDropTransferObject dto = (XmDropTransferObject) widget;
	XmDropTransferPart *dtp =
		(XmDropTransferPart *) &(dto->dropTransfer);
	Cardinal index = dtp->num_drop_transfer_lists++;

	dtp->drop_transfer_lists = (XmDropTransferList)
		XtRealloc((char *)dtp->drop_transfer_lists,
			sizeof(XmDropTransferListRec) *
			dtp->num_drop_transfer_lists);
	dtp->drop_transfer_lists[index].transfer_list =
	        (XmDropTransferEntry)_XmAllocAndCopy(
		transfers, sizeof(XmDropTransferEntryRec) * num_transfers);
	dtp->drop_transfer_lists[index].num_transfers = num_transfers;
}


void 
#ifdef _NO_PROTO
XmDropTransferAdd( widget, transfers, num_transfers )
        Widget widget ;
        XmDropTransferEntry transfers ;
        Cardinal num_transfers ;
#else
XmDropTransferAdd(
        Widget widget,
        XmDropTransferEntry transfers,
        Cardinal num_transfers )
#endif /* _NO_PROTO */
{
	XmDropTransferObject dt = (XmDropTransferObject) widget;
	XmDropTransferObjectClass wc = (XmDropTransferObjectClass)
		XtClass(dt);

	((*(wc->dropTransfer_class.add_drop_transfer))
		(widget, transfers, num_transfers));
}

