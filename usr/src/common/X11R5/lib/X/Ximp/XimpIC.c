#pragma ident	"@(#)R5Xlib:Ximp/XimpIC.c	1.4"

/* $XConsortium: XimpIC.c,v 1.7 92/07/29 10:15:50 rws Exp $ */
/******************************************************************

              Copyright 1991, 1992 by FUJITSU LIMITED
              Copyright 1991, 1992 by Sony Corporation
              Copyright 1991, 1992 by Sun Microsystems, Inc.

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of FUJITSU LIMITED
and Sony Corporation and Sun Microsystems, Inc. must not be used in
advertising or publicity pertaining to distribution of the software 
without specific, written prior permission.
FUJITSU LIMITED and Sony Corporation and Sun Microsystems, Inc. make no
representations about the suitability of this software for any purpose.  
It is provided "as is" without express or implied warranty.

FUJITSU LIMITED AND SONY CORPORATION AND SUN MICROSYTEMS, INC. DISCLAIM 
ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJITSU LIMITED AND
SONY CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Takashi Fujiwara     FUJITSU LIMITED 
          Makoto Wakamatsu     Sony Corporation
	  Hideki Hiura         Sun Microsystems, Inc.

******************************************************************/

#define NEED_EVENTS
#include <X11/Xatom.h>
#include "Xlibint.h"
#include "Xlcint.h"

#include "Ximplc.h"

static void		_Ximp_DestroyIC();
static void		_Ximp_SetFocus();
static void		_Ximp_UnSetFocus();
extern char 		*_Ximp_SetICValues();
extern char 		*_Ximp_GetICValues();
extern char		*_Ximp_MbReset();
extern wchar_t		*_Ximp_WcReset();
extern int		_Ximp_MbLookupString();
extern int		_Ximp_WcLookupString();

extern char 		*_Ximp_SetICValueData();
extern void		_Ximp_SetValue_Resource();
extern Bool		_Ximp_ConnectIC();

extern void		_Ximp_SetFocusWindowProp();
extern void		_Ximp_SetFocusWindowFilter();
extern void		_Ximp_SetPreeditAtr();
extern void		_Ximp_SetPreeditFont();
extern void		_Ximp_SetStatusAtr();
extern void		_Ximp_SetStatusFont();
extern Bool		_Ximp_XimFilter_Keypress();
extern Bool		_Ximp_XimFilter_Keyrelease();
extern Bool		_Ximp_XimFilter_Client();
extern void 		_Ximp_IM_SendMessage();

static XICMethodsRec Ximp_ic_methods = {
    _Ximp_DestroyIC, 		/* destroy */
    _Ximp_SetFocus,  		/* set_focus */
    _Ximp_UnSetFocus,		/* unset_focus */
    _Ximp_SetICValues,		/* set_values */
    _Ximp_GetICValues,		/* get_values */
    _Ximp_MbReset,		/* mb_reset */
    _Ximp_WcReset,		/* wc_reset */
    _Ximp_MbLookupString,	/* mb_lookup_string */
    _Ximp_WcLookupString,	/* wc_lookup_string */
};

XIC
_Ximp_CreateIC(im, values)
XIM		 im;
XIMArg		*values;
{
    Ximp_XIC		 ic;
    XimpChangeMaskRec	 dummy;
    XICXimpRec		*ximp_icpart;

    if((ic = (Ximp_XIC)Xmalloc(sizeof(Ximp_XICRec))) == (Ximp_XIC)NULL)
	return((XIC)NULL);
    if((ximp_icpart = (XICXimpRec *)Xmalloc(sizeof(XICXimpRec)))
	    == (XICXimpRec *)NULL) {
	Xfree(ic);
	return((XIC)NULL);
    }
    bzero((char *)ic, sizeof(Ximp_XICRec));
    bzero((char *)ximp_icpart, sizeof(XICXimpRec));

    ic->methods = &Ximp_ic_methods;
    ic->core.im = im;
    /* Filter Event : for Ximp Protocol */
    ic->core.filter_events
		= KeyPressMask | KeyReleaseMask | StructureNotifyMask;

    ximp_icpart->svr_mode = ((Ximp_XIM)im)->ximp_impart->def_svr_mode;

    ic->ximp_icpart = ximp_icpart;

    if(_Ximp_SetICValueData(ic, values, XIMP_CREATE_IC, &dummy))
	goto Set_Error;

    /* The Value must be set */
    if(!(ximp_icpart->value_mask & XIMP_INPUT_STYLE)) /* Input Style */
	goto Set_Error;

    if(ic->core.input_style & XIMPreeditCallbacks)
	if(!(ximp_icpart->value_mask & XIMP_PRE_CALLBAK)) /* Preedit Callback */
	    goto Set_Error;
    if(ic->core.input_style & XIMStatusCallbacks)
	if(!(ximp_icpart->value_mask & XIMP_STS_CALLBAK)) /* Status Callback */
	    goto Set_Error;

    if(IS_UNCONNECTABLE(im)) {
	if(_Ximp_ConnectIC(ic, XIMP_CREATE_IC) == False)
	    goto Set_Error;
    } else {
	if(IS_SERVER_CONNECTED(im)) {
	    if(_Ximp_ConnectIC(ic, XIMP_CREATE_IC))
		return((XIC)ic);
	}
	if( XIMP_CHK_FOCUSWINMASK(ic) ) {
	    _XRegisterFilterByType (ic->core.im->core.display,
				    ic->core.focus_window,
				    KeyPress, KeyPress,
				    _Ximp_XimFilter_Keypress,
				    (XPointer)ic);
	    _XRegisterFilterByType (ic->core.im->core.display,
				    ic->core.focus_window,
				    KeyRelease, KeyRelease,
				    _Ximp_XimFilter_Keyrelease,
				    (XPointer)ic);
	    ic->ximp_icpart->filter_mode |= 0x1;
	}
    }
    return((XIC)ic);

Set_Error :
    Xfree(ic);
    Xfree(ximp_icpart);
    return((XIC)NULL);
}

static void
_Ximp_DestroyIC(ic)
Ximp_XIC	 ic;
{
    if(ic->ximp_icpart->filter_mode & 0x1) {
	_XUnregisterFilter (ic->core.im->core.display,
			    ic->core.focus_window,
			    _Ximp_XimFilter_Keypress,
			    (XPointer)ic);
	_XUnregisterFilter (ic->core.im->core.display,
			    ic->core.focus_window,
			    _Ximp_XimFilter_Keyrelease,
			    (XPointer)ic);
    }
    if(ic->ximp_icpart->filter_mode & 0x2) {
	_XUnregisterFilter(ic->core.im->core.display,
			   ic->ximp_icpart->back_focus_win,
			   _Ximp_XimFilter_Client, (XPointer)ic);
    }
    if(ic->ximp_icpart->filter_mode & 0x4) {			/* XXXXX */
	_XUnregisterFilter(ic->core.im->core.display,
			   ic->core.client_window,
			   _Ximp_XimFilter_Client, (XPointer)ic);
    }
    if(IS_IC_CONNECTED(ic))
    	_Ximp_IM_SendMessage(ic, XIMP_DESTROY(ic), NULL, NULL, NULL);
    Xfree(ic->ximp_icpart);
    return;
}

static void
_Ximp_SetFocus(ic)
Ximp_XIC	ic;
{
    if(IS_IC_CONNECTED(ic))
	_Ximp_IM_SendMessage(ic, XIMP_SETFOCUS(ic), NULL, NULL, NULL);
    if(!(ic->ximp_icpart->filter_mode & 0x1)) {
	_XRegisterFilterByType (ic->core.im->core.display,
				ic->core.focus_window,
				KeyPress, KeyPress,
				_Ximp_XimFilter_Keypress,
				(XPointer)ic);
	_XRegisterFilterByType (ic->core.im->core.display,
				ic->core.focus_window,
				KeyRelease, KeyRelease,
				_Ximp_XimFilter_Keyrelease,
				(XPointer)ic);
	ic->ximp_icpart->filter_mode |= 0x1;
    }
    return;
}

static void
_Ximp_UnSetFocus(ic)
Ximp_XIC	ic;
{
    if(IS_IC_CONNECTED(ic))
	_Ximp_IM_SendMessage(ic, XIMP_UNSETFOCUS(ic), NULL, NULL, NULL);
    if(ic->ximp_icpart->filter_mode & 0x1) {
	_XUnregisterFilter (ic->core.im->core.display,
			    ic->core.focus_window,
			    _Ximp_XimFilter_Keypress,
			    (XPointer)ic);
	_XUnregisterFilter (ic->core.im->core.display,
			    ic->core.focus_window,
			    _Ximp_XimFilter_Keyrelease,
			    (XPointer)ic);
	ic->ximp_icpart->filter_mode &= ~(0x1);
    }
    return;
}

void
_Ximp_SetFocusWindowFilter(ic)
Ximp_XIC	 ic;
{
    if(ic->ximp_icpart->filter_mode & 0x1) {
	_XUnregisterFilter (ic->core.im->core.display,
		ic->ximp_icpart->back_focus_win,
		_Ximp_XimFilter_Keypress,
		(XPointer)ic);
	_XUnregisterFilter (ic->core.im->core.display,
		ic->ximp_icpart->back_focus_win,
		_Ximp_XimFilter_Keyrelease,
		(XPointer)ic);
    }
    _XRegisterFilterByType (ic->core.im->core.display,
		ic->core.focus_window,
		KeyPress, KeyPress,
		_Ximp_XimFilter_Keypress,
		(XPointer)ic);
    _XRegisterFilterByType (ic->core.im->core.display,
		ic->core.focus_window,
		KeyRelease, KeyRelease,
		_Ximp_XimFilter_Keyrelease,
		(XPointer)ic);
    ic->ximp_icpart->filter_mode |= 0x1;

    if(ic->ximp_icpart->filter_mode & 0x2) {
	_XUnregisterFilter(ic->core.im->core.display,
		ic->ximp_icpart->back_focus_win,
		_Ximp_XimFilter_Client, (XPointer)ic);
    }
    _XRegisterFilterByType(ic->core.im->core.display,
		ic->core.focus_window,
		ClientMessage, ClientMessage,
		_Ximp_XimFilter_Client, (XPointer)ic);
    ic->ximp_icpart->filter_mode |= 0x2;

    if(!(ic->ximp_icpart->filter_mode & 0x4)) {		/* XXXXX */
	_XRegisterFilterByType(ic->core.im->core.display,
				ic->core.client_window,
				ClientMessage, ClientMessage,
				_Ximp_XimFilter_Client, (XPointer)ic);
	ic->ximp_icpart->filter_mode |= 0x4;
    }
    return;
}

static void
_Ximp_AttributesSetL(data, setdata, cnt)
char	*data;
long	 setdata;
int	 cnt;
{
    long	*ptr;

    ptr = (long *)&data[cnt];
    *ptr = setdata;
    return;
}

void
_Ximp_SetFocusWindowProp(ic)
Ximp_XIC	 ic;
{
    XChangeProperty(ic->core.im->core.display, ic->core.client_window,
		((Ximp_XIM)ic->core.im)->ximp_impart->focus_win_id,
		XA_WINDOW, 32, PropModeReplace,
		(unsigned char *)&ic->core.focus_window, 1);
    return;
}

void
_Ximp_SetPreeditAtr(ic)
Ximp_XIC		 ic;
{
    Ximp_PreeditPropRec4	*preedit_atr;
    unsigned char		 prop_data[XIMP_PREEDIT_MAX_CHAR4];

    preedit_atr = &(ic->ximp_icpart->preedit_attr);
    _Ximp_AttributesSetL(prop_data, preedit_atr->Area.x, 0);
    _Ximp_AttributesSetL(prop_data, preedit_atr->Area.y, 4);
    _Ximp_AttributesSetL(prop_data, preedit_atr->Area.width, 8);
    _Ximp_AttributesSetL(prop_data, preedit_atr->Area.height, 12);
    if(ISXimp4(ic)) {
	_Ximp_AttributesSetL(prop_data, preedit_atr->AreaNeeded.width, 16);
	_Ximp_AttributesSetL(prop_data, preedit_atr->AreaNeeded.height, 20);
	_Ximp_AttributesSetL(prop_data, preedit_atr->SpotLocation.x, 24);
	_Ximp_AttributesSetL(prop_data, preedit_atr->SpotLocation.y, 28);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Colormap, 32);
	_Ximp_AttributesSetL(prop_data, preedit_atr->StdColormap, 36);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Foreground, 40);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Background, 44);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Bg_Pixmap,	48);
	_Ximp_AttributesSetL(prop_data, preedit_atr->LineSpacing, 52);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Cursor, 56);
    } else {
	_Ximp_AttributesSetL(prop_data, preedit_atr->Foreground, 16);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Background, 20);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Colormap, 24);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Bg_Pixmap, 28);
	_Ximp_AttributesSetL(prop_data, preedit_atr->LineSpacing, 32);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Cursor, 36);
	_Ximp_AttributesSetL(prop_data, preedit_atr->AreaNeeded.width, 40);
	_Ximp_AttributesSetL(prop_data, preedit_atr->AreaNeeded.height, 44);
	_Ximp_AttributesSetL(prop_data, preedit_atr->SpotLocation.x, 48);
	_Ximp_AttributesSetL(prop_data, preedit_atr->SpotLocation.y, 52);
    }

    XChangeProperty(ic->core.im->core.display, ic->core.client_window,
		((Ximp_XIM)ic->core.im)->ximp_impart->preedit_atr_id,
		((Ximp_XIM)ic->core.im)->ximp_impart->preedit_atr_id,
		32, PropModeReplace, prop_data, XIMP_PREEDIT_MAX_LONG(ic));
    return;
}

void
_Ximp_SetPreeditFont(ic)
Ximp_XIC		 ic;
{
    if (ic->core.preedit_attr.fontset != NULL) {
	XChangeProperty(ic->core.im->core.display, ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->preeditfont_id,
			XA_STRING, 8, PropModeReplace,
			(unsigned char *)(ic->ximp_icpart->preedit_font),
			strlen(ic->ximp_icpart->preedit_font));
    }
}

void
_Ximp_SetStatusAtr(ic)
Ximp_XIC		 ic;
{
    Ximp_StatusPropRec4		*status_atr;
    unsigned char		 prop_data[XIMP_STATUS_MAX_CHAR4];

    status_atr = &(ic->ximp_icpart->status_attr);
    _Ximp_AttributesSetL(prop_data, status_atr->Area.x, 0);
    _Ximp_AttributesSetL(prop_data, status_atr->Area.y, 4);
    _Ximp_AttributesSetL(prop_data, status_atr->Area.width, 8);
    _Ximp_AttributesSetL(prop_data, status_atr->Area.height, 12);
    if(ISXimp4(ic)) {
	_Ximp_AttributesSetL(prop_data, status_atr->AreaNeeded.width,  16);
	_Ximp_AttributesSetL(prop_data, status_atr->AreaNeeded.height, 20);
	_Ximp_AttributesSetL(prop_data, status_atr->Colormap, 24);
	_Ximp_AttributesSetL(prop_data, status_atr->StdColormap, 28);
	_Ximp_AttributesSetL(prop_data, status_atr->Foreground, 32);
	_Ximp_AttributesSetL(prop_data, status_atr->Background, 36);
	_Ximp_AttributesSetL(prop_data, status_atr->Bg_Pixmap, 40);
	_Ximp_AttributesSetL(prop_data, status_atr->LineSpacing, 44);
	_Ximp_AttributesSetL(prop_data, status_atr->Cursor, 48);
	_Ximp_AttributesSetL(prop_data, status_atr->window, 52);
    } else {
	_Ximp_AttributesSetL(prop_data, status_atr->Foreground, 16);
	_Ximp_AttributesSetL(prop_data, status_atr->Background, 20);
	_Ximp_AttributesSetL(prop_data, status_atr->Colormap, 24);
	_Ximp_AttributesSetL(prop_data, status_atr->Bg_Pixmap, 28);
	_Ximp_AttributesSetL(prop_data, status_atr->LineSpacing, 32);
	_Ximp_AttributesSetL(prop_data, status_atr->Cursor, 36);
	_Ximp_AttributesSetL(prop_data, status_atr->AreaNeeded.width, 40);
	_Ximp_AttributesSetL(prop_data, status_atr->AreaNeeded.height, 44);
	_Ximp_AttributesSetL(prop_data, status_atr->window, 48);
    }

    XChangeProperty(ic->core.im->core.display, ic->core.client_window,
		((Ximp_XIM)ic->core.im)->ximp_impart->status_atr_id,
		((Ximp_XIM)ic->core.im)->ximp_impart->status_atr_id,
		32, PropModeReplace, prop_data, XIMP_STATUS_MAX_LONG(ic));
    return;
}

void
_Ximp_SetStatusFont(ic)
Ximp_XIC		ic;
{
    if (ic->core.status_attr.fontset != NULL) {
	XChangeProperty(ic->core.im->core.display, ic->core.client_window,
		((Ximp_XIM)ic->core.im)->ximp_impart->statusfont_id,
		XA_STRING, 8, PropModeReplace,
		(unsigned char *)(ic->ximp_icpart->status_font),
		strlen(ic->ximp_icpart->status_font));
    }
}

void
_Ximp_IM_SendMessage(ic, request, data1, data2, data3)
Ximp_XIC	ic;
unsigned long	request;
unsigned long	data1, data2, data3;
{
    XEvent		Message;

    if(!(IS_IC_CONNECTED(ic)) && (request != XIMP_CREATE(ic)))
	    return;

    /* ClientMessage Send */
    Message.xclient.type         = ClientMessage;
    Message.xclient.display      = ic->core.im->core.display;
    Message.xclient.window       = ((Ximp_XIM)ic->core.im)->
					ximp_impart->fe_window;
    Message.xclient.message_type = ((Ximp_XIM)ic->core.im)->
					ximp_impart->improtocol_id;
    Message.xclient.format       = 32;
    Message.xclient.data.l[0]    = request;
    if(request == XIMP_CREATE(ic))
	Message.xclient.data.l[1] = (long)ic->core.client_window;
    else
	Message.xclient.data.l[1] = ic->ximp_icpart->icid;
    Message.xclient.data.l[2]    = data1;
    Message.xclient.data.l[3]    = data2;
    Message.xclient.data.l[4]    = data3;
    XSendEvent(ic->core.im->core.display,
	((Ximp_XIM)ic->core.im)->ximp_impart->fe_window,
	False, NoEventMask, &Message);
    XFlush(ic->core.im->core.display);
    return;
}
