/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */


#ifndef	NOIDENT
#ident	"@(#)olmisc:OpenLook.c	1.52"
#endif

#include <stdio.h>
#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <Xol/OlClients.h>

	/* property structures */

typedef struct {
	CARD32	flags;
	CARD32	menu_type;
	CARD32	pushpin_initial_state;	/* for "_OL_PIN_STATE" */
} xPropWMDecorationHints;

#define NumPropWMDecorationHintsElements 3

typedef struct {
	CARD32		state;
	Window		icon;
} xPropWMState;

#define NumPropWMStateElements 2

/***********************************************************************
 *
 * WARNING. see notes about OlWinAttrs in OlClients.h
 *
 ***********************************************************************/

	/* The structure below should match OlClients.h:OLWinAttr. */
typedef struct {
	CARD32	flags;
	CARD32	win_type;
	CARD32	menu_type;
	CARD32	pin_state;
	CARD32	cancel;
} xPropOLWinAttr;

#define NumPropOLWinAttrElements (sizeof(xPropOLWinAttr) / sizeof(CARD32))

typedef struct {
	CARD32	flags;
	CARD32	fg_red;
	CARD32	fg_green;
	CARD32	fg_blue;
	CARD32	bg_red;
	CARD32	bg_green;
	CARD32	bg_blue;
	CARD32	bd_red;
	CARD32	bd_green;
	CARD32	bd_blue;
} xPropOLWinColors;

#define NumPropOLWinColorsElements 10


char * GetCharProperty();
Status GetLongProperty();


void
InitializeOpenLook(dpy)
	Display *	dpy;
{
	/* This routine is no longer needed because XInternAtom
	 * does atom caching in R5, so we don't need to keep
	 * these variables around. Other reason is that
	 * Atom is tight with a Display. Caching the atom value
	 * like this won't help when we enable "multiple" displays
	 * in the toolkit...
	 *
	 * Keep a stub here for backward binary compatiabilty,
	 * you can find the actual copy in Compat.c...
	 */
}


	/* Get Property routines for Open Look Properties */

Status
GetHelpKeyMessage(dpy, ev, window_return, x_return, y_return, root_x_return, root_y_return)
	Display *dpy;
	XEvent *ev;
	Window *window_return;
	int *x_return;
	int *y_return;
	int *root_x_return;
	int *root_y_return;
{
	if (ev->type != ClientMessage ||
	    ev->xclient.message_type != XA_OL_HELP_KEY(dpy))
	{
		*window_return = (Window) NULL;
		*x_return = 0;
		*y_return = 0;
		*root_x_return = 0;
		*root_y_return = 0;
		return ~Success;
	}

	*window_return = ev->xclient.data.l[0];
	*x_return = ev->xclient.data.l[1];
	*y_return = ev->xclient.data.l[2];
	*root_x_return = ev->xclient.data.l[3];
	*root_y_return = ev->xclient.data.l[4];

	return Success;
}


GetWMDecorationHints(dpy, w, wmdh)
	Display *dpy;
	Window w;
	WMDecorationHints *wmdh;
{
	Atom			atr,
				wm_deco_hints;
	int			afr;
	unsigned long		nir,
				bar;
	xPropWMDecorationHints	*prop;
	int			Failure;

	wm_deco_hints = XA_WM_DECORATION_HINTS(dpy);
	if ((Failure = XGetWindowProperty(dpy, w, wm_deco_hints, 0L,
				NumPropWMDecorationHintsElements, False,
				wm_deco_hints, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop))) != Success)
		return Failure;

        if (atr != wm_deco_hints ||
			nir < NumPropWMDecorationHintsElements || afr != 32)
	{
		if (prop != (xPropWMDecorationHints *) 0)
			free((char *)prop);

                return BadValue;
	}
	else
	{
		wmdh->flags = prop->flags;
		wmdh->menu_type = prop->menu_type;
		wmdh->pushpin_initial_state = prop->pushpin_initial_state;
		if (prop != (xPropWMDecorationHints *) 0)
			free((char *)prop);

		return Success;
	}
}

typedef struct {
	CARD32		state;
} xPropOLManagerState;

#define NumPropOLManagerStateElements 1

GetOLManagerState(dpy,scr)
	Display *dpy;
	Screen *scr;
{
	OLManagerState	wms;
	Atom		atr,
			mgr_state;
	int		afr;
	unsigned long	nir,
			bar;
	xPropOLManagerState	*prop;
	Window rootwin = RootWindowOfScreen(scr);

#define XA_OL_MANAGER_STATE(d)	XInternAtom(d, "OL_MANAGER_STATE", False)
	mgr_state = XA_OL_MANAGER_STATE(dpy);
#undef XA_OL_MANAGER_STATE

	if (XGetWindowProperty(dpy, rootwin, mgr_state, 0L,
				NumPropOLManagerStateElements, False,
				mgr_state, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop)) != Success)
	{
		return(0);
	}

        if (atr != mgr_state || nir < NumPropOLManagerStateElements ||
								 afr != 32)
	{
		if (prop != (xPropOLManagerState *) 0)
			free ((char *)prop);

		return(0);
	}
	else
	{
		wms.state = prop->state;
		if (prop != (xPropOLManagerState *) 0)
			free((char *)prop);
	}

	return wms.state;
}

Bool
OlIsFMRunning(dpy, scr)
	Display *	dpy;
	Screen *	scr;
{
	OLManagerState wms;
	wms.state = (long) 0;
	wms.state = GetOLManagerState(dpy,scr);
	if (wms.state & OLFM_STATE)
		return(True);
	else
		return(False);
}

Bool
OlIsWSMRunning(dpy, scr)
	Display *	dpy;
	Screen *	scr;
{
	OLManagerState wms;
	wms.state = (long) 0;
	wms.state = GetOLManagerState(dpy,scr);
	if (wms.state & OLWSM_STATE)
		return(True);
	else
		return(False);
}

Bool
OlIsWMRunning(dpy,scr)
	Display *	dpy;
	Screen *	scr;
{
	OLManagerState wms;
	wms.state = (long) 0;
	wms.state = GetOLManagerState(dpy,scr);
	if (wms.state & OLWM_STATE)
		return(True);
	else
		return(False);
}

GetWMState(dpy, w)
	Display *dpy;
	Window w;
{
	WMState		wms;
	Atom		atr,
			wm_state;
	int		afr;
	unsigned long	nir,
			bar;
	xPropWMState	*prop;

	wm_state = XA_WM_STATE(dpy);
	if (XGetWindowProperty(dpy, w, wm_state, 0L,
				NumPropWMStateElements, False,
				wm_state, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop)) != Success)
	{
		return NormalState;		/* Punt */
	}

        if (atr != wm_state || nir < NumPropWMStateElements || afr != 32)
	{
		if (prop != (xPropWMState *) 0)
			free ((char *)prop);

		wms.state =  NormalState;		/* Punt */
	}
	else
	{
		wms.state = prop->state;
		if (prop != (xPropWMState *) 0)
			free((char *)prop);
	}

	return wms.state;
}


GetWMWindowBusy(dpy, w, state)
	Display *dpy;
	Window w;
	long *state;
{
	Atom		atr;
	int		afr;
	unsigned long	nir,
			bar;
	long		*value;
	int		Failure;

	if ((Failure = XGetWindowProperty(dpy, w, XA_OL_WIN_BUSY(dpy), 0L,
				1L, False,
				XA_INTEGER, &atr, &afr, &nir, &bar,
				(unsigned char **) (&value))) != Success)
		return Failure;

        if (atr != XA_INTEGER || nir < 1L || afr != 32)
	{
		if (value != (long *) 0)
			free ((char *) value);

                return BadValue;
	}
	else
	{
		*state = *value;
		if (value != (long *) 0)
			free((char *) value);

		return Success;
	}
}


GetWMPushpinState(dpy, w, state)
	Display *dpy;
	Window w;
	long *state;
{
	Atom		atr;
	int		afr;
	unsigned long	nir,
			bar;
	long		*value;
	int		Failure;

	if ((Failure = XGetWindowProperty(dpy, w, XA_OL_PIN_STATE(dpy), 0L,
				1L, False,
				XA_INTEGER, &atr, &afr, &nir, &bar,
				(unsigned char **) (&value))) != Success)
		return Failure;

        if (atr != XA_INTEGER || nir < 1L || afr != 32)
	{
		if (value != (long *) 0)
			free ((char *) value);

                return BadValue;
	}
	else
	{
		*state = *value;
		if (value != (long *) 0)
			free((char *) value);

		return Success;
	}
}


GetOLWinColors(dpy, win, win_colors)
	Display *dpy;
	Window win;
	OLWinColors *win_colors;
{
	Atom			atr,
				ol_win_colors;
	int			afr;
	unsigned long		nir,
				bar;
	xPropOLWinColors	*color_struct;
	int			Failure;

	ol_win_colors = XA_OL_WIN_COLORS(dpy);
	if ((Failure = XGetWindowProperty(dpy, win, ol_win_colors,
				0L, NumPropOLWinColorsElements, False,
				ol_win_colors, &atr, &afr, &nir, &bar,
				(unsigned char **) (&color_struct))) != Success)
		return Failure;

        if (atr != ol_win_colors || nir < NumPropOLWinColorsElements ||
								afr != 32)
	{
		if (color_struct != (xPropOLWinColors *) 0)
			free((char *) color_struct);

                return BadValue;
	}
	else
	{
		win_colors->flags = color_struct->flags;
		win_colors->fg_red = color_struct->fg_red;
		win_colors->fg_green = color_struct->fg_green;
		win_colors->fg_blue = color_struct->fg_blue;
		win_colors->bg_red = color_struct->bg_red;
		win_colors->bg_green = color_struct->bg_green;
		win_colors->bg_blue = color_struct->bg_blue;
		win_colors->bd_red = color_struct->bd_red;
		win_colors->bd_green = color_struct->bd_green;
		win_colors->bd_blue = color_struct->bd_blue;

		if (color_struct != (xPropOLWinColors *) 0)
			free((char *) color_struct);

		return Success;
	}
}

GetOLWinAttr(dpy, client_window, olwa)
	Display *dpy;
	Window client_window;
	OLWinAttr *olwa;
{
	Atom		atr,
			ol_win_attr;
	int		afr;
	unsigned long	nir,
			bar;
	xPropOLWinAttr	*prop;
	int		Failure;

	ol_win_attr = XA_OL_WIN_ATTR(dpy);
	if ((Failure = XGetWindowProperty(dpy, client_window, ol_win_attr, 0L,
				NumPropOLWinAttrElements, False,
				ol_win_attr, &atr, &afr, &nir, &bar,
				(unsigned char **) (&prop))) != Success)
		return Failure;

        if (atr != ol_win_attr || nir < NumPropOLWinAttrElements || afr != 32)
	{
		if (prop != (xPropOLWinAttr *) 0)
			free((char *) prop);

                return BadValue;
	}
	else
	{
		olwa->flags = prop->flags;
		olwa->win_type = prop->win_type;
		olwa->menu_type = prop->menu_type;
		olwa->pin_state = prop->pin_state;
		olwa->cancel = prop->cancel;
		free((char *) prop);

		return Success;
	}
}

	/* Set Property routines for Open Look Properties */

void
SendProtocolMessage(dpy, w, protocol, time)
Display *dpy;
Window w;
Atom protocol;
unsigned long time;
{
	XEvent	sev;

	sev.xclient.type = ClientMessage;
	sev.xclient.display = dpy;
	sev.xclient.window = w;
	sev.xclient.message_type = XA_WM_PROTOCOLS(dpy);
	sev.xclient.format = 32;
	sev.xclient.data.l[0] = (long) protocol;
	sev.xclient.data.l[1] = time;

	XSendEvent(dpy, w, False, NoEventMask, &sev);
}


void
SetWMDecorationHints(dpy, w, wmdh)
Display *dpy;
Window w;
WMDecorationHints *wmdh;
{
	xPropWMDecorationHints	prop;
	Atom			wm_deco_hints;

	if (wmdh == (WMDecorationHints *) 0)
		return;

	prop.flags = wmdh->flags;
	prop.menu_type = wmdh->menu_type;
	prop.pushpin_initial_state = wmdh->pushpin_initial_state;

	wm_deco_hints = XA_WM_DECORATION_HINTS(dpy);
	XChangeProperty(dpy, w, wm_deco_hints, wm_deco_hints, 32,
				PropModeReplace, (unsigned char *) &prop,
				NumPropWMDecorationHintsElements);

	XFlush(dpy);
}


void
SetWMState(dpy, w, wms)
Display *dpy;
Window w;
WMState *wms;
{
	xPropWMState	prop;
	Atom		wm_state;

	if (wms == (WMState *) 0)
		return;

	prop.state = wms->state;
	prop.icon = wms->icon;

	wm_state = XA_WM_STATE(dpy);
	XChangeProperty(dpy, w, wm_state, wm_state, 32,
				PropModeReplace, (unsigned char *) &prop,
				NumPropWMStateElements);

	XFlush(dpy);
}


void
SetWMWindowBusy(dpy, w, state)
Display *dpy;
Window w;
long state;
{
	long	value = state;

	XChangeProperty(dpy, w, XA_OL_WIN_BUSY(dpy), XA_INTEGER, 32,
				PropModeReplace, (unsigned char *) &value, 1);
	XFlush(dpy);
}


void
SetWMPushpinState(dpy, w, state)
Display *dpy;
Window w;
long state;
{
	long	value = state;

	XChangeProperty(dpy, w, XA_OL_PIN_STATE(dpy), XA_INTEGER, 32,
				PropModeReplace, (unsigned char *) &value, 1);
	XFlush(dpy);
}


void
SetWMIconSize(dpy, w, wmis)
Display *dpy;
Window w;
WMIconSize *wmis;
{
	XSetIconSizes(dpy, w, wmis, 1);
}


/*
 * GetCharProperty (generic routine to get a char type property)
 */

char *
GetCharProperty(dpy, w, property, length)
	Display * dpy;
	Window w;
	Atom property;
	int * length;
{
	Atom			actual_type;
	int			actual_format;
	unsigned long		num_items;
	unsigned long		bytes_remaining = 1;
	char *                  buffer;
	char *                  Buffer;
	int			Buffersize = 0;
	int			Result;
	int			EndOfBuffer;
	register		i;

	Buffer = (char *) malloc(1);
	Buffer[0] = '\0';
	EndOfBuffer = 0;
	do
	{
	if ((Result = XGetWindowProperty(dpy, w, property, 
		(long) ((Buffersize+3) /4),
		(long) ((bytes_remaining+3) / 4), True,
		XA_STRING, &actual_type, &actual_format, 
		&num_items, &bytes_remaining,
		(unsigned char **) &buffer)) != Success)
		{
		if (buffer) free(buffer);
		if (Buffer) free(Buffer);
		*length = 0;
		return NULL;
		}
        
	if (buffer) 
		{
		register int i;
		Buffersize += num_items;
		Buffer = (char *) realloc(Buffer, Buffersize + 1);
		if (Buffer == NULL) 
			{
			free(buffer);
			*length = 0;
			return NULL;
			}
		for (i = 0; i < num_items; i++)
		   Buffer[EndOfBuffer++] = buffer[i];
		Buffer[EndOfBuffer] = '\0';
		free(buffer);
		}
	} while (bytes_remaining > 0);

	*length = Buffersize;
	if (Buffersize == 0 && Buffer != NULL)
		{
		free (Buffer);
		return NULL;
		}
	else
		return (Buffer);

} /* end of GetCharProperty */
/*
 * GetLongProperty
 */

Status
GetLongProperty(dpy, w, property, result)
	Display *dpy;
	Window  w;
	Atom property;
	long * result;
{
	Atom			actual_type;
	int			actual_format;
	unsigned long		num_items,
				bytes_remaining;
	int			Result;
	long *			value;

	if ((Result = XGetWindowProperty(dpy, w, property, 0L,
				1L, False,
				property, &actual_type, &actual_format, 
                                &num_items, &bytes_remaining,
				(unsigned char **) &value)) != Success)
		return Result;
	else
		{
		*result = *value;
		free(value);
	        if (actual_type != property)
			return 42 /* BadValue */;
		else
			return 15 /* Success */;
		}

} /* end of GetLongProperty */

Status
SendLongNotice(dpy, w, to, message, value)
	Display * dpy;
	Window w;
	Window to;
	Atom message;
	long value;
{
XEvent event;
Status Result;

event.xclient.type = ClientMessage;
event.xclient.display = dpy;
event.xclient.window = to;
event.xclient.message_type = message;
event.xclient.format = 32;
event.xclient.data.l[0] = w;
event.xclient.data.l[1] = value;

Result = XSendEvent(dpy, to, False, NoEventMask, &event);

XSync(dpy, False);

return (Result);

} /* end of SendLongNotice */

void
EnqueueCharProperty(dpy, w, atom, data, len)
	Display * dpy;
	Window w;
	Atom atom;
	char * data;
	int len;
{

	XChangeProperty(dpy, w, atom, XA_STRING, 8, PropModeAppend,
		(unsigned char *) data, len);

	XFlush(dpy);
} /* end of EnqueueCharProperty */


Atom *
GetAtomList(dpy, w, property, length, delete)
Display *dpy;
Window w;
Atom property;
int *length;
Bool delete;
{
	Atom			actual_type;
	unsigned long		num_items,
				bytes_remaining;
	Atom			*buffer = (Atom *) 0;
	int			actual_format;

	if (XGetWindowProperty(dpy, w, property, 
		(long) 0, (long) 1, False,
		XA_ATOM, &actual_type, &actual_format, 
		&num_items, &bytes_remaining,
		(unsigned char **) &buffer) != Success)
	{
		if (buffer)
			free(buffer);

		*length = 0;
		return (Atom *) 0;
	}

	if (buffer)
		free(buffer);

	if (XGetWindowProperty(dpy, w, property, 
		(long) 0,
		(long) (1 + bytes_remaining / 4), delete,
		actual_type, &actual_type, &actual_format, 
		&num_items, &bytes_remaining,
		(unsigned char **) &buffer) != Success)
	{
		if (buffer)
			free(buffer);
		*length = 0;
		return NULL;
	}
	
	*length = num_items;
	return buffer;
}
