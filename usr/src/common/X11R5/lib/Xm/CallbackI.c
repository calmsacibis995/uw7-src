#pragma ident	"@(#)m1.2libs:Xm/CallbackI.c	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
#ifdef REV_INFO
#ifndef lint
static char rcsid[] = "$RCSfile$ $Revision$ $Date$"
#endif
#endif
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#include "CallbackI.h"


/********************************************************************
 * VendorShell and Dialog Shell's extension objects are no longer 
 * full fledged Xt objects and therefore  cannot be passed as an
 * argument  to the Xt calls XtAddCallback(), XtRemoveCallback(),
 * and XtCallCallbackList(). The functions _XmAddCallback()
 * _XmRemoveCallback() and _XmCallCallbackList() replace these 
 * Xt calls for non Xt objects.
 ********************************************************************/

/********    Static Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Static Function Declarations    ********/


/* However it doesn't contain a final NULL record */
#define ToList(p) ((XtCallbackList) ((p)+1))

void
#ifdef _NO_PROTO
_XmAddCallback(callbacks, callback, closure)
    InternalCallbackList*   callbacks;
    XtCallbackProc	    callback;
    XtPointer		    closure;
#else
_XmAddCallback(
    InternalCallbackList*   callbacks,
    XtCallbackProc          callback,
    XtPointer               closure )
#endif /* _NO_PROTO */
{
    register InternalCallbackList icl;
    register XtCallbackList cl;
    register int count;
    
    icl = *callbacks;
    count = icl ? icl->count : 0;

    if (icl && icl->call_state) {
	icl->call_state |= _XtCBFreeAfterCalling;
	icl = (InternalCallbackList)
	    XtMalloc(sizeof(InternalCallbackRec) +
		     sizeof(XtCallbackRec) * (count + 1));

        memcpy((char *)ToList(icl), (char *)ToList(*callbacks),
              sizeof(XtCallbackRec) * count);

    } else {
	icl = (InternalCallbackList)
	    XtRealloc((char *) icl, sizeof(InternalCallbackRec) +
		      sizeof(XtCallbackRec) * (count + 1));
    }
    *callbacks = icl;
    icl->count = count + 1;
    icl->is_padded = 0;
    icl->call_state = 0;
    cl = ToList(icl) + count;
    cl->callback = callback;
    cl->closure = closure;
} /* _XmAddCallback */



void
#ifdef _NO_PROTO
 _XmRemoveCallback (callbacks, callback, closure)
    InternalCallbackList   *callbacks;
    XtCallbackProc	    callback;
    XtPointer		    closure;
#else
 _XmRemoveCallback (
    InternalCallbackList   *callbacks,
    XtCallbackProc          callback,
    XtPointer               closure)
#endif /* _NO_PROTO */
{
    register InternalCallbackList icl;
    register int i, j;
    register XtCallbackList cl, ncl, ocl;

    icl = *callbacks;
    if (!icl) return;

    cl = ToList(icl);
    for (i=icl->count; --i >= 0; cl++) {
	if (cl->callback == callback && cl->closure == closure) {
	    if (icl->call_state) {
		icl->call_state |= _XtCBFreeAfterCalling;
		if (icl->count == 1) {
		    *callbacks = NULL;
		} else {
		    j = icl->count - i - 1;
		    ocl = ToList(icl);
		    icl = (InternalCallbackList)
			XtMalloc(sizeof(InternalCallbackRec) +
				 sizeof(XtCallbackRec) * (i + j));
		    icl->count = i + j;
		    icl->is_padded = 0;
		    icl->call_state = 0;
		    ncl = ToList(icl);
		    while (--j >= 0)
			*ncl++ = *ocl++;
		    while (--i >= 0)
			*ncl++ = *++cl;
		    *callbacks = icl;
		}
	    } else {
		if (--icl->count) {
		    ncl = cl + 1;
		    while (--i >= 0)
			*cl++ = *ncl++;
		    icl = (InternalCallbackList)
			XtRealloc((char *) icl, sizeof(InternalCallbackRec)
				  + sizeof(XtCallbackRec) * icl->count);
		    icl->is_padded = 0;
		    *callbacks = icl;
		} else {
		    XtFree((char *) icl);
		    *callbacks = NULL;
		}
	    }
	    return;
	}
    }
} /* _XmRemoveCallback */

void
#ifdef _NO_PROTO
_XmRemoveAllCallbacks( callbacks )
        InternalCallbackList *callbacks ;
#else
_XmRemoveAllCallbacks(
        InternalCallbackList *callbacks)
#endif /* _NO_PROTO */
{
    register InternalCallbackList icl = *callbacks;

    if (icl) {
	if (icl->call_state)
	    icl->call_state |= _XtCBFreeAfterCalling;
	else
	    XtFree((char *) icl);
	*callbacks = NULL;
    }
} /* _XmRemoveAllCallbacks */


void 
#ifdef _NO_PROTO
_XmCallCallbackList(widget, callbacks, call_data)
    Widget widget;
    XtCallbackList callbacks;
    XtPointer call_data;
#else
_XmCallCallbackList(
    Widget widget,
    XtCallbackList callbacks,
    XtPointer call_data)
#endif /* _NO_PROTO */
{
    register InternalCallbackList icl;
    register XtCallbackList cl;
    register int i;
    char ostate;

    if (!callbacks) return;
    icl = (InternalCallbackList)callbacks;
    cl = ToList(icl);
    if (icl->count == 1) {
	(*cl->callback) (widget, cl->closure, call_data);
	return;
    }
    ostate = icl->call_state;
    icl->call_state = _XtCBCalling;
    for (i = icl->count; --i >= 0; cl++)
	(*cl->callback) (widget, cl->closure, call_data);
    if (ostate)
	icl->call_state |= ostate;
    else if (icl->call_state & _XtCBFreeAfterCalling)
	XtFree((char *)icl);
    else
	icl->call_state = 0;
} /* XmCallCallbackList */
