#pragma	ident	"@(#)r5extensions:lib/XIdle.c	1.1"
/*
 * Copyright 1989,1991 University of Wisconsin-Madison
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Wisconsin-Madison not
 * be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The University of
 * Wisconsin-Madison makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE UNIVERSITY OF WISCONSIN-MADISON DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF WISCONSIN-MADISON BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Author: Tim Theisen           Systems Programmer
 * Internet: tim@cs.wisc.edu       Department of Computer Sciences
 *     UUCP: uwvax!tim             University of Wisconsin-Madison
 *    Phone: (608)262-0438         1210 West Dayton Street
 *      FAX: (608)262-9777         Madison, WI   53706
 */

#define NEED_REPLIES
#include "Xlibint.h"
#include "Xext.h"
#include "extutil.h"
#include "xidlestr.h"

static XExtensionInfo _xidle_info_data;
static XExtensionInfo *xidle_info = &_xidle_info_data;
static /* const */ char *xidle_extension_name = XIDLENAME;

#define XidleCheckExtension(dpy,i,val) \
  XextCheckExtension (dpy, i, xidle_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *			   private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks xidle_extension_hooks = {
    NULL,				/* create_gc */
    NULL,				/* copy_gc */
    NULL,				/* flush_gc */
    NULL,				/* free_gc */
    NULL,				/* create_font */
    NULL,				/* free_font */
    close_display,			/* close_display */
    NULL,				/* wire_to_event */
    NULL,				/* event_to_wire */
    NULL,				/* error */
    NULL				/* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY (find_display, xidle_info,
				   xidle_extension_name, &xidle_extension_hooks,
				   XidleNumberEvents, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY (close_display, xidle_info)


/*****************************************************************************
 *                                                                           *
 *		    public Idle Time Extension routines                      *
 *                                                                           *
 *****************************************************************************/

Bool XidleQueryExtension (dpy, event_basep, error_basep)
    Display *dpy;
    int *event_basep, *error_basep;
{
    XExtDisplayInfo *info = find_display (dpy);

    if (XextHasExtension(info)) {
	*event_basep = info->codes->first_event;
	*error_basep = info->codes->first_error;
	return True;
    } else {
	return False;
    }
}


Status XGetIdleTime(dpy, IdleTime)
register Display *dpy;
Time *IdleTime;
{
    XExtDisplayInfo *info = find_display (dpy);
    register xGetIdleTimeReq *req;
    xGetIdleTimeReply rep;

    XidleCheckExtension(dpy,info,0)

    LockDisplay(dpy);
    GetReq(GetIdleTime, req);
    req->reqType = info->codes->major_opcode;
    req->xidleReqType = X_GetIdleTime;
    if (!_XReply(dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return 0;
    }
    *IdleTime = rep.time;
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

