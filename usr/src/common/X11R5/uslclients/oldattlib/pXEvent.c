#ifndef	NOIDENT
#ident	"@(#)oldattlib:pXEvent.c	1.1"
#endif
/*
 pXEvent.c (C source file)
	Acc: 575322380 Fri Mar 25 14:46:20 1988
	Mod: 575321572 Fri Mar 25 14:32:52 1988
	Sta: 575321572 Fri Mar 25 14:32:52 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	void fpXEvent (stream, event, verbose)
	FILE * stream;
	Xevent * event;
	int verbose;

	author:
		Ross Hilbert
		AT&T 10/20/87
************************************************************************/

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Xprint.h"
#include "pID.h"

#define WIDTH	25

/*
	standard print
*/
#ifdef __STDC__
#define PR(MEMBER, FORMAT) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprintf (stream, FORMAT, ev->MEMBER); \
	fprintf (stream, "\n"); \
}
#else
#define PR(MEMBER, FORMAT) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprintf (stream, FORMAT, ev->MEMBER); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print window with name
*/
#ifdef __STDC__
#define PW(MEMBER) \
{ \
	char * name; \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprintf (stream, "0x%lx", ev->MEMBER); \
	if (ev->MEMBER && XFetchName (ev->display, ev->MEMBER, &name)) \
	{ \
		fprintf (stream, " (%s)", name); \
		XFree (name); \
	} \
	fprintf (stream, "\n"); \
}
#else
#define PW(MEMBER) \
{ \
	char * name; \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprintf (stream, "0x%lx", ev->MEMBER); \
	if (ev->MEMBER && XFetchName (ev->display, ev->MEMBER, &name)) \
	{ \
		fprintf (stream, " (%s)", name); \
		XFree (name); \
	} \
	fprintf (stream, "\n"); \
}
#endif
/*
	print atom with name
*/
#ifdef __STDC__
#define PA(MEMBER) \
{ \
	char * name; \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprintf (stream, "0x%lx", ev->MEMBER); \
	if (ev->MEMBER && (name = XGetAtomName(ev->display,ev->MEMBER))) \
		fprintf (stream, " (%s)", name); \
	fprintf (stream, "\n"); \
}
#else
#define PA(MEMBER) \
{ \
	char * name; \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprintf (stream, "0x%lx", ev->MEMBER); \
	if (ev->MEMBER && (name = XGetAtomName(ev->display,ev->MEMBER))) \
		fprintf (stream, " (%s)", name); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print keycode with name
*/
#ifdef __STDC__
#define PK(MEMBER) \
{ \
	int n; \
	KeySym key = XLookupKeysym (ev, 0); \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprintf (stream, "0x%lx", ev->MEMBER); \
	if (key) fprintf (stream, " key(%s)", XKeysymToString (key)); \
	n = XLookupString (ev, buf, MAX_BUF, &key, NULL); \
	if (key) fprintf (stream, " mod(%s)", XKeysymToString (key)); \
	if (n) dump_chars (stream, buf, n); \
	fprintf (stream, "\n"); \
}
#else
#define PK(MEMBER) \
{ \
	int n; \
	KeySym key = XLookupKeysym (ev, 0); \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprintf (stream, "0x%lx", ev->MEMBER); \
	if (key) fprintf (stream, " key(%s)", XKeysymToString (key)); \
	n = XLookupString (ev, buf, MAX_BUF, &key, NULL); \
	if (key) fprintf (stream, " mod(%s)", XKeysymToString (key)); \
	if (n) dump_chars (stream, buf, n); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print keysym name for keycode
*/
#define PC(CODE) \
{ \
	int code = CODE; \
	KeySym key = XKeycodeToKeysym (ev->display, code, 0); \
	fprintf (stream, "%*s = ", WIDTH, "key_vector[]"); \
	fprintf (stream, "0x%x", code); \
	if (key) fprintf (stream, " %s", XKeysymToString (key)); \
	fprintf (stream, "\n"); \
}
/*
	print match from id set
*/
#ifdef __STDC__
#define PM(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprint_match (stream, (unsigned long)ev->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#else
#define PM(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprint_match (stream, (unsigned long)ev->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print group (mask) from id set
*/
#ifdef __STDC__
#define PG(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #MEMBER); \
	fprint_mask (stream, (unsigned long)ev->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#else
#define PG(MEMBER,IDS) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "MEMBER"); \
	fprint_mask (stream, (unsigned long)ev->MEMBER, IDS); \
	fprintf (stream, "\n"); \
}
#endif
/*
	print event type
*/
#ifdef __STDC__
#define PE(TYPE) \
{ \
	fprintf (stream, "%*s = ", WIDTH, #TYPE); \
	fprint_match (stream, (unsigned long)TYPE, _events); \
	fprintf (stream, "\n"); \
}
#else
#define PE(TYPE) \
{ \
	fprintf (stream, "%*s = ", WIDTH, "TYPE"); \
	fprint_match (stream, (unsigned long)TYPE, _events); \
	fprintf (stream, "\n"); \
}
#endif

#define MAX_BUF		32
static char		buf[MAX_BUF];

static void dump_chars (stream, buf, n)
FILE * stream;
char * buf;
int n;
{
	int i, c;

	fprintf (stream, " str(");

	for (i = 0; i < n; ++i)
	{
		if ((c = buf[i]) < 32 || c > 126)
			fprintf (stream, "<0x%x>", c);
		else
			fprintf (stream, "%c", c);
	}
	fprintf (stream, ")");
}

void fpXEvent (stream, event, verbose)
FILE * stream;
XEvent *event;
int verbose;
{
	int		type = event->type;
	XAnyEvent *	ev = (XAnyEvent *) event;

	PE (type);
	PR (serial, LONGHEX);
	PM (send_event, _boolean);

	if (! verbose)
	{
		PW (window);
		return;
	}
	switch (type) {
	      case KeyPress:
	      case KeyRelease:
	        {
			register XKeyEvent * ev = (XKeyEvent *) event;

			PW (window);
			PW (root);
			PW (subwindow);
			PR (time, LONGHEX);
			PR (x, INT);
			PR (y, INT);
			PR (x_root, INT);
			PR (y_root, INT);
			PG (state, _mousekeystate);
			PK (keycode);
			PM (same_screen, _boolean);
		}
	      	break;
	      case ButtonPress:
	      case ButtonRelease:
	        {
			register XButtonEvent * ev = (XButtonEvent *) event;

			PW (window);
			PW (root);
			PW (subwindow);
			PR (time, LONGHEX);
			PR (x, INT);
			PR (y, INT);
			PR (x_root, INT);
			PR (y_root, INT);
			PG (state, _mousekeystate);
			PM (button, _mousebutton);
			PM (same_screen, _boolean);
		}
	        break;
	      case MotionNotify:
	        {
			register XMotionEvent * ev = (XMotionEvent *) event;

			PW (window);
			PW (root);
			PW (subwindow);
			PR (time, LONGHEX);
			PR (x, INT);
			PR (y, INT);
			PR (x_root, INT);
			PR (y_root, INT);
			PG (state, _mousekeystate);
			PM (is_hint, _notifyhint);
			PM (same_screen, _boolean);
		}
	        break;
	      case EnterNotify:
	      case LeaveNotify:
		{
			register XCrossingEvent * ev = (XCrossingEvent *) event;

			PW (window);
			PW (root);
			PW (subwindow);
			PR (time, LONGHEX);
			PR (x, INT);
			PR (y, INT);
			PR (x_root, INT);
			PR (y_root, INT);
			PM (mode, _notifymode);
			PM (detail, _notifydetail);
			PM (same_screen, _boolean);
			PM (focus, _boolean);
			PG (state, _mousekeystate);
		}
		  break;
	      case FocusIn:
	      case FocusOut:
		{
			register XFocusChangeEvent * ev = (XFocusChangeEvent *) event;

			PW (window);
			PM (mode, _notifymode);
			PM (detail, _notifydetail);
		}
		  break;
	      case KeymapNotify:
		{
			register XKeymapEvent * ev = (XKeymapEvent *) event;
			register i, j, k;

			PW (window);

			for (i = 1; i < 32; ++i)
			{
				j = ev->key_vector[i];

				for (k = 0; k < 8; ++k, j>>=1)
					if (j & 1)
						PC (i*8+k);
			}
		}
		break;
	      case Expose:
		{
			register XExposeEvent * ev = (XExposeEvent *) event;

			PW (window);
			PR (x, INT);
			PR (y, INT);
			PR (width, INT);
			PR (height, INT);
			PR (count, INT);
		}
		break;
	      case GraphicsExpose:
		{
			register XGraphicsExposeEvent * ev = (XGraphicsExposeEvent *) event;

			PR (drawable, LONGHEX);
			PR (x, INT);
			PR (y, INT);
			PR (width, INT);
			PR (height, INT);
			PR (count, INT);
			PR (major_code, INT);
			PR (minor_code, INT);
		}
		break;
	      case NoExpose:
		{
			register XNoExposeEvent * ev = (XNoExposeEvent *) event;

			PR (drawable, LONGHEX);
			PR (major_code, INT);
			PR (minor_code, INT);
		}
		break;
	      case VisibilityNotify:
		{
			register XVisibilityEvent * ev = (XVisibilityEvent *) event;

			PW (window);
			PM (state, _visibilitystate);
		}
		break;
	      case CreateNotify:
		{
			register XCreateWindowEvent * ev = (XCreateWindowEvent *) event;

			PW (parent);
			PW (window);
			PR (x, INT);
			PR (y, INT);
			PR (width, INT);
			PR (height, INT);
			PR (border_width, INT);
			PM (override_redirect, _boolean);
		}
		break;
	      case DestroyNotify:
		{
			register XDestroyWindowEvent * ev = (XDestroyWindowEvent *) event;

			PW (event);
			PW (window);
		}
		break;
	      case UnmapNotify:
		{
			register XUnmapEvent * ev = (XUnmapEvent *) event;

			PW (event);
			PW (window);
			PM (from_configure, _boolean);
		}
		break;
	      case MapNotify:
		{
			register XMapEvent * ev = (XMapEvent *) event;

			PW (event);
			PW (window);
			PM (override_redirect, _boolean);
		}
		break;
	      case MapRequest:
		{
			register XMapRequestEvent * ev = (XMapRequestEvent *) event;

			PW (parent);
			PW (window);
		}
		break;
	      case ReparentNotify:
		{
			register XReparentEvent * ev = (XReparentEvent *) event;

			PW (event);
			PW (window);
			PW (parent);
			PR (x, INT);
			PR (y, INT);
			PM (override_redirect, _boolean);
		}
		break;
	      case ConfigureNotify:
		{
			register XConfigureEvent * ev = (XConfigureEvent *) event;

			PW (event);
			PW (window);
			PR (x, INT);
			PR (y, INT);
			PR (width, INT);
			PR (height, INT);
			PR (border_width, INT);
			PW (above);
			PM (override_redirect, _boolean);
		}
		break;
	      case GravityNotify:
		{
			register XGravityEvent * ev = (XGravityEvent *) event;

			PW (event);
			PW (window);
			PR (x, INT);
			PR (y, INT);
		}
		break;
	      case ResizeRequest:
		{
			register XResizeRequestEvent * ev = (XResizeRequestEvent *) event;

			PW (window);
			PR (width, INT);
			PR (height, INT);
		}
		break;
	      case ConfigureRequest:
		{
			register XConfigureRequestEvent * ev = (XConfigureRequestEvent *) event;

			PW (parent);
			PW (window);
			PR (x, INT);
			PR (y, INT);
			PR (width, INT);
			PR (height, INT);
			PR (border_width, INT);
			PW (above);
			PM (detail, _configuredetail);
			PG (value_mask, _configuremask);
		}
		break;
	      case CirculateNotify:
		{
			register XCirculateEvent * ev = (XCirculateEvent *) event;

			PW (event);
			PW (window);
			PM (place, _circulateplace);
		}
		break;
	      case CirculateRequest:
		{
			register XCirculateRequestEvent * ev = (XCirculateRequestEvent *) event;

			PW (parent);
			PW (window);
			PM (place, _circulateplace);
		}
		break;
	      case PropertyNotify:
		{
			register XPropertyEvent * ev = (XPropertyEvent *) event;

			PW (window);
			PA (atom);
			PR (time, LONGHEX);
			PM (state, _propertystate);
		}
		break;
	      case SelectionClear:
		{
			register XSelectionClearEvent * ev = (XSelectionClearEvent *) event;

			PW (window);
			PA (selection);
			PR (time, LONGHEX);
		}
		break;
	      case SelectionRequest:
		{
			register XSelectionRequestEvent * ev = (XSelectionRequestEvent *) event;

			PW (owner);
			PW (requestor);
			PA (selection);
			PA (target);
			PA (property);
			PR (time, LONGHEX);
		}
		break;
	      case SelectionNotify:
		{
			register XSelectionEvent * ev = (XSelectionEvent *) event;

			PW (requestor);
			PA (selection);
			PA (target);
			PA (property);
			PR (time, LONGHEX);
		}
		break;
	      case ColormapNotify:
		{
			register XColormapEvent * ev = (XColormapEvent *) event;

			PW (window);
			PR (colormap, LONGHEX);
			PM (new, _boolean);
			PM (state, _colormapstate);
	        }
		break;
	      case ClientMessage:
		{
			register XClientMessageEvent * ev = (XClientMessageEvent *) event;

			PW (window);
			PA (message_type);
			PR (format, INT);

			switch (ev->format) {
			case 8:	
			   PR (data.b[0], HEX);
			   PR (data.b[1], HEX);
			   PR (data.b[2], HEX);
			   PR (data.b[3], HEX);
			   PR (data.b[4], HEX);
			   PR (data.b[5], HEX);
			   PR (data.b[6], HEX);
			   PR (data.b[7], HEX);
			   PR (data.b[8], HEX);
			   PR (data.b[9], HEX);
			   PR (data.b[10], HEX);
			   PR (data.b[11], HEX);
			   PR (data.b[12], HEX);
			   PR (data.b[13], HEX);
			   PR (data.b[14], HEX);
			   PR (data.b[15], HEX);
			   PR (data.b[16], HEX);
			   PR (data.b[17], HEX);
			   PR (data.b[18], HEX);
			   PR (data.b[19], HEX);
			   break;
			case 16:
			   PR (data.s[0], HEX);
			   PR (data.s[1], HEX);
			   PR (data.s[2], HEX);
			   PR (data.s[3], HEX);
			   PR (data.s[4], HEX);
			   PR (data.s[5], HEX);
			   PR (data.s[6], HEX);
			   PR (data.s[7], HEX);
			   PR (data.s[8], HEX);
			   PR (data.s[9], HEX);
			   break;
			case 32:
			   PR (data.l[0], HEX);
			   PR (data.l[1], HEX);
			   PR (data.l[2], HEX);
			   PR (data.l[3], HEX);
			   PR (data.l[4], HEX);
			   break;
			default: /* XXX should never occur */
			   break;
			}
	        }
		break;
	      case MappingNotify:
		{
			register XMappingEvent * ev = (XMappingEvent *) event;

			PM (request, _mappingrequest);
			PR (first_keycode, HEX);
			PR (count, INT);
		}
		break;
	      default:
		{
			register XAnyEvent * ev = (XAnyEvent *) event;

			PW (window);
		}
		break;
	}
}

