#ifndef NOIDENT
#pragma ident	"@(#)Dt:DtLock.c	1.2"
#endif

#include <Xt/Intrinsic.h>

#include <DtLock.h>

#define POLL_IS_USEFUL

/*
 * DtLock.c
 *
 */

static int cursor_locked = False;

/*
 * DtLockCursor
 *
 */

extern DtCallbackInfo *
DtLockCursor(Widget w, unsigned long interval, void (*f)(), XtPointer client_data, Cursor cursor)
{

   Display *             display = XtDisplayOfObject(w);
   Window                window  = XtWindowOfObject(w);
   XtAppContext          context = XtDisplayToApplicationContext(display);

   static DtCallbackInfo cbinfo;

   if (!cursor_locked)
   {
      int cnt = 0;

      while (cnt < 100 && XGrabPointer(display, window, True,
             ButtonReleaseMask,
             GrabModeAsync, GrabModeAsync,
             None, cursor, CurrentTime) != GrabSuccess)
                        cnt++;
#ifdef POLL_IS_USEFUL
      poll(NULL, 0, DtMinimumCursorLock);
      if (interval <= DtMinimumCursorLock)
         return NULL;
      interval -= DtMinimumCursorLock;
#endif

/*
 * Unfortunately, the toolkit does XUngrabPointers in lots of places.
 * Each of these places could look for the lock and avoid turning
 * off the visual too soon.  When this is the case, the POLL_IS_USEFUL
 * definition can be removed.  Until then the lock will enforce a
 * minimum visual display time of half the specified interval.
 * 
 */

      cbinfo.display     = display;
      cbinfo.f           = f;
      cbinfo.client_data = client_data;
      cbinfo.timer_id    = 
         XtAppAddTimeOut(context, interval, DtUnlockCursor, &cbinfo);
      cursor_locked = True;
   }

   return (&cbinfo);

} /* end of DtLockCursor */
/*
 * DtUnlockCursor
 *
 */

extern void
DtUnlockCursor(XtPointer client_data, XtIntervalId * id)
{
   DtCallbackInfo * cbinfo = (DtCallbackInfo *)client_data;

   if (cbinfo->f)
      (*cbinfo->f)(cbinfo->client_data);

   XUngrabPointer(cbinfo->display, CurrentTime);
   cursor_locked = False;

} /* end of DtUnlockCursor */
