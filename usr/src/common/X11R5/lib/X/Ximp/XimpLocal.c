#pragma ident	"@(#)R5Xlib:Ximp/XimpLocal.c	1.1"

/* $XConsortium$ */
/******************************************************************

              Copyright 1992 by Fuji Xerox Co., Ltd.

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Fuji Xerox
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.
Fuji Xerox make no representations about the suitability of this
software for any purpose.  It is provided
"as is" without express or implied warranty.

FUJI XEROX DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJI XEROX BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA
OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Kazunori Nishihara	Fuji Xerox

******************************************************************/

#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "Xlibint.h"
#include "Xlcint.h"

#include "Ximplc.h"

static Status _Ximp_Local_CloseIM();
static char * _Ximp_Local_GetIMValues();
static XIC _Ximp_Local_CreateIC();
static void _Ximp_Local_DestroyIC();
static void _Ximp_Local_SetFocus();
static void _Ximp_Local_UnSetFocus();
static char * _Ximp_Local_SetICValues();
static char * _Ximp_Local_GetICValues();
static char * _Ximp_Local_MbReset();
static wchar_t * _Ximp_Local_WcReset();
static int _Ximp_Local_MbLookupString();
static int _Ximp_Local_WcLookupString();

static Bool		_Ximp_PreSetAttributes();
static Bool		_Ximp_StatusSetAttributes();
static char *		_Ximp_SetICValueData();

static Bool 		 _Ximp_PreGetAttributes();
static Bool 		 _Ximp_StatusGetAttributes();

extern FILE * _XlcOpenLocaleFile();

extern void _Ximp_Local_OpenIM_hook();

XIMMethodsRec Ximp_local_im_methods = {
    _Ximp_Local_CloseIM,
    _Ximp_Local_GetIMValues,
    _Ximp_Local_CreateIC
};

static XICMethodsRec Ximp_ic_methods = {
    _Ximp_Local_DestroyIC, 	/* destroy */
    _Ximp_Local_SetFocus,  	/* set_focus */
    _Ximp_Local_UnSetFocus,	/* unset_focus */
    _Ximp_Local_SetICValues,	/* set_values */
    _Ximp_Local_GetICValues,	/* get_values */
    _Ximp_Local_MbReset,		/* mb_reset */
    _Ximp_Local_WcReset,		/* wc_reset */
    _Ximp_Local_MbLookupString,	/* mb_lookup_string */
    _Ximp_Local_WcLookupString,	/* wc_lookup_string */
};

static
CreateDefTree(im)
    Ximp_XIM im;
{
    FILE *fp;
    static char *buf[1024];
    char *s, *rhs;
    DefTree *elem;

    fp = _XlcOpenLocaleFile(NULL, ((XimpLCd)im->core.lcd)->locale.language, COMPOSE_FILE);
    im->ximp_impart->top = (DefTree *)NULL;
    if (fp == (FILE *)NULL) return;
#ifdef BACKTRACK_WHEN_UNMATCHED
    im->ximp_impart->num_save_rooms = Ximp_ParseStringFile(fp, &im->ximp_impart->top);
#else
    (void)Ximp_ParseStringFile(fp, &im->ximp_impart->top);
#endif
    fclose(fp);
}

void
_Ximp_Local_OpenIM_hook(im)
    Ximp_XIM im;
{
    CreateDefTree(im);
    im->ximp_impart->current_ic = (XIC)NULL;
}

static char *
_Ximp_Local_GetIMValues(im, values)
    Ximp_XIM im;
    XIMArg *values;
{
    XIMArg *p;
    XIMStyles **value;
    XIMStyles *styles;
    int i;

    for (p = values; p->name != NULL; p++) {
	if (strcmp(p->name, XNQueryInputStyle) == 0) {
	    if ((styles = (XIMStyles *)Xmalloc(sizeof(XIMStyles) +
	         sizeof(XIMStyle) *
		 im->ximp_impart->im_styles->count_styles)) == NULL) {
		break;
	    }
	    styles->count_styles = im->ximp_impart->im_styles->count_styles;
	    styles->supported_styles = (XIMStyle *)(&styles[1]);
	    for(i=0; i < styles->count_styles; i++) {
		styles->supported_styles[i] = im->ximp_impart->im_styles->supported_styles[i];
	    }
	    value = (XIMStyles **)p->value;
	    *value = styles;
	} else {
	    break;
	}
    }
    return (p->name);
}

static
FreeDefTreeElements(top)
DefTree *top;
{
    if (top->succession) FreeDefTreeElements(top->succession);
    if (top->next) FreeDefTreeElements(top->next);
    if (top->mb) Xfree(top->mb);
    if (top->wc) Xfree(top->wc);
    Xfree(top);
}

static Status
_Ximp_Local_CloseIM(im)
    Ximp_XIM im;
{
    XIC ic;

    while (ic = im->core.ic_chain) {
	XDestroyIC(ic);
    }
    FreeDefTreeElements(im->ximp_impart->top);
    if( im->ximp_impart->im_name )
	XFree( im->ximp_impart->im_name );
    XFree(im->ximp_impart->im_styles->supported_styles);
    XFree(im->ximp_impart->im_styles);
    XFree(im->ximp_impart);
    return (True);
}

static Bool
_Ximp_Local_Filter(d, w, ev, client_data)
    Display *d;
    Window w;
    XEvent *ev;
    XPointer client_data;
{
    Ximp_XIC ic = (Ximp_XIC)client_data;
    KeySym keysym;
    static char buf[128];
#ifdef BACKTRACK_WHEN_UNMATCHED
    static long must_be_through = 0;
#endif

    DefTree *p;

    if (ev->type != KeyPress) return (False);
    if (ev->xkey.keycode == 0) return (False);
    if (((Ximp_XIM)ic->core.im)->ximp_impart->top == (DefTree *)NULL) return (False);
#ifdef BACKTRACK_WHEN_UNMATCHED
    if (must_be_through) {
	must_be_through--;
	return (False);
    }
#endif

    (void)XLookupString((XKeyEvent *)ev, buf, sizeof(buf), &keysym, NULL);
    if (IsModifierKey(keysym)) return (False);
    for (p = ic->ximp_icpart->context; p; p = p->next) {
	if (((ev->xkey.state & p->modifier_mask) == p->modifier) &&
	    (keysym == p->keysym)) {
	    break;
	}
    }
    if (p) { /* Matched */
	if (p->succession) { /* Intermediate */
	    ic->ximp_icpart->context = p->succession;
#ifdef BACKTRACK_WHEN_UNMATCHED
	    /* Seve Event */
	    bcopy(ev, &ic->ximp_icpart.saved_event[ic->ximp_icpart.num_saved++], sizeof(XKeyEvent));
#endif
	    return (True);
	} else { /* Terminate (reached to leaf) */
	    ic->ximp_icpart->composed = p;
	    /* return back to client KeyPressEvent keycode == 0 */
	    ev->xkey.keycode = 0;
	    XPutBackEvent(d, ev);
	    /* initialize internal state for next key sequence */
	    ic->ximp_icpart->context = ((Ximp_XIM)ic->core.im)->ximp_impart->top;
#ifdef BACKTRACK_WHEN_UNMATCHED
	    ic->ximp_icpart.num_saved = 0;
#endif
	    return (True);
	}
    } else { /* Unmatched */
	if (ic->ximp_icpart->context == ((Ximp_XIM)ic->core.im)->ximp_impart->top) {
	    return (False);
	}
	/* Error (Sequence Unmatch occured) */
#ifdef BACKTRACK_WHEN_UNMATCHED
	XPutBackEvent(d, ev);
	while (ic->ximp_icpart.num_saved > 0) {
	    ic->ximp_icpart.num_saved--;
	    XPutBackEvent(d, &ic->ximp_icpart.saved_event[ic->ximp_icpart.num_saved]);
	}
	must_be_through = 1;
#endif
	/* initialize internal state for next key sequence */
	ic->ximp_icpart->context = ((Ximp_XIM)ic->core.im)->ximp_impart->top;
	return (True);
    }
}

static XIC
_Ximp_Local_CreateIC(im, values)
    XIM im;
    XIMArg *values;
{
    Ximp_XIC ic;
    XICXimpRec *ximp_icpart;
    XimpChangeMaskRec dummy;

    if ((ic = (Ximp_XIC)Xmalloc(sizeof(Ximp_XICRec))) == (Ximp_XIC)NULL) {
	return ((XIC)NULL);
    }
    if((ximp_icpart = (XICXimpRec *)Xmalloc(sizeof(XICXimpRec))) == (XICXimpRec *)NULL) {
	XFree(ic);
	return ((XIC)NULL);
    }
    bzero((char *)ic, sizeof(Ximp_XICRec));
    bzero((char *)ximp_icpart, sizeof(XICXimpRec));

    ic->methods = &Ximp_ic_methods;
    ic->core.im = im;
    ic->core.filter_events = KeyPressMask;
    ic->ximp_icpart = ximp_icpart;
    ic->ximp_icpart->context = ((Ximp_XIM)im)->ximp_impart->top;
    ic->ximp_icpart->composed = NULL;

    if (_Ximp_SetICValueData(ic, values, XIMP_CREATE_IC, &dummy)) {
	Xfree(ic);
	Xfree(ximp_icpart);
	return((XIC)NULL);
    }
    /* Not Yet */
    if (!(ximp_icpart->value_mask & XIMP_INPUT_STYLE)) {
	XFree(ic);
	XFree(ximp_icpart);
	return ((XIC)NULL);
    }
    if (ic->core.input_style & XIMPreeditNothing) {
    } else if (ic->core.input_style & XIMPreeditNone) {
    } else {
    }
    if (ic->core.input_style & XIMStatusNothing) {
    } else if (ic->core.input_style & XIMStatusNone) {
    } else {
    }
#ifdef BACKTRACK_WHEN_UNMATCHED
    if( (ximp_icpart->saved_event = Xcalloc(((Ximp_XIM)im)->ximp_impart->num_save_rooms, sizeof(XEvent))) == NULL ) {
	Xfree(ic);
	Xfree(ximp_icpart);
	return((XIC)NULL);
    }
#endif
    _XRegisterFilterByType(ic->core.im->core.display, ic->core.focus_window,
	KeyPress, KeyPress, _Ximp_Local_Filter, (XPointer)ic);
    return ((XIC)ic);
}

static void
_Ximp_Local_DestroyIC(ic)
    Ximp_XIC ic;
{
    if (((Ximp_XIM)ic->core.im)->ximp_impart->current_ic == (XIC)ic) {
	_Ximp_Local_UnSetFocus(ic);
    }
#ifdef BACKTRACK_WHEN_UNMATCHED
    Xfree(((XICXimpRec *)ic->ximp_icpart)->saved_event);
#endif
    Xfree(ic->ximp_icpart);
    return;
}

static void
_Ximp_Local_SetFocus(ic)
    Ximp_XIC ic;
{
    XIC current_ic = ((Ximp_XIM)ic->core.im)->ximp_impart->current_ic;

    if (current_ic != (XIC)NULL) {
	_Ximp_Local_UnSetFocus(current_ic);
    }
    ((Ximp_XIM)ic->core.im)->ximp_impart->current_ic = (XIC)ic;
    _XRegisterFilterByType(ic->core.im->core.display, ic->core.focus_window,
	KeyPress, KeyPress, _Ximp_Local_Filter, (XPointer)ic);
}

static void
_Ximp_Local_UnSetFocus(ic)
    Ximp_XIC ic;
{
    ((Ximp_XIM)ic->core.im)->ximp_impart->current_ic = (XIC)NULL;
    _XUnregisterFilter (ic->core.im->core.display, ic->core.focus_window,
	_Ximp_Local_Filter, (XPointer)ic);
}

static char *
_Ximp_SetICValues(ic, values)
Ximp_XIC	 ic;
XIMArg		*values;
{
    XIM		 im;
    char		*ret;
    XimpChangeMaskRec	 change_mask;

    XIMP_SET_NULLMASK(change_mask);
    if((ret = _Ximp_SetICValueData(ic, values, XIMP_SET_IC, &change_mask)))
	return(ret);

    if(   (ic->ximp_icpart->value_mask & XIMP_RES_NAME)
       || (ic->ximp_icpart->value_mask & XIMP_RES_CLASS) )
	_Ximp_SetValue_Resource(ic, &change_mask);
    return(ret);
}

static char *
_Ximp_SetICValueData(ic, values, mode, change_mask)
    Ximp_XIC	 ic;
    XIMArg		*values;
    int		 mode;
    XimpChangeaMask  change_mask;
{
    XIMArg			*p;
    char			*return_name = NULL;

    for(p = values; p->name != NULL; p++) {
	if(strcmp(p->name, XNInputStyle) == 0) {
	    if(mode == XIMP_CREATE_IC) {
		ic->core.input_style = (XIMStyle)p->value;
		ic->ximp_icpart->value_mask |= XIMP_INPUT_STYLE;
	    } else {
		; /* Currently Fixed value */
	    }
	} else if(strcmp(p->name, XNClientWindow)==0) {
	    if(!(ic->ximp_icpart->value_mask & XIMP_CLIENT_WIN)) {
		ic->core.client_window = (Window)p->value;
		ic->ximp_icpart->value_mask |= XIMP_CLIENT_WIN;
		if(!(XIMP_CHK_FOCUSWINMASK(ic))) {
		    ic->core.focus_window = ic->core.client_window;
                    XIMP_SET_FOCUSWINMASK2(ic, change_mask);
		}
	    } else {
		return_name = p->name;
		break; /* Can't change this value */
	    }
	    
	} else if(strcmp(p->name, XNFocusWindow)==0) {
	    ic->ximp_icpart->back_focus_win = ic->core.focus_window;
	    ic->core.focus_window = (Window)p->value;
            XIMP_SET_FOCUSWINMASK2(ic, change_mask);
	    
	} else if(strcmp(p->name, XNResourceName)==0) {
	    ic->core.im->core.res_name = (char *)p->value;
	    ic->ximp_icpart->value_mask |= XIMP_RES_NAME;
	    
	} else if(strcmp(p->name, XNResourceClass)==0) {
	    ic->core.im->core.res_class = (char *)p->value;
	    ic->ximp_icpart->value_mask |= XIMP_RES_CLASS;
	    
	} else if(strcmp(p->name, XNGeometryCallback)==0) {
	    ic->core.geometry_callback.client_data =
		((XIMCallback *)p->value)->client_data;
	    ic->core.geometry_callback.callback =
		((XIMCallback *)p->value)->callback;
	    ic->ximp_icpart->value_mask |= XIMP_GEOMETRY_CB;
	    
	} else if(strcmp(p->name, XNPreeditAttributes)==0) {
	    if( _Ximp_PreSetAttributes(ic,
				       &(ic->ximp_icpart->preedit_attr),
				       p->value, mode, change_mask,
				       return_name) == False )
		break;
	    
	} else if(strcmp(p->name, XNStatusAttributes)==0) {
	    if( _Ximp_StatusSetAttributes(ic,
					  &(ic->ximp_icpart->status_attr),
					  p->value, mode, change_mask,
					  return_name) == False )
		break;
	    
	} else {
	    return_name = p->name;
	    break;
	}
    }
    return(return_name);
}
		
static Bool
_Ximp_PreSetAttributes(ic, attr, vl, mode, change_mask, return_name)
Ximp_XIC		 ic;
Ximp_PreeditPropRec4	*attr;
XIMArg			*vl;
int			 mode;
XimpChangeaMask 	 change_mask;
char			*return_name;
{
	XIMArg			*p;
	XStandardColormap	*colormap_ret;
	int			 list_ret;
	XFontStruct		**struct_list;
	char			**name_list;
	int 			 i, len;
	int			 count;
	char 			*tmp;

	for(p = vl; p->name != NULL; p++) {
		if(strcmp(p->name, XNArea)==0) {
			ic->core.preedit_attr.area.x = ((XRectangle *)p->value)->x;
			ic->core.preedit_attr.area.y = ((XRectangle *)p->value)->y;
			ic->core.preedit_attr.area.width = ((XRectangle *)p->value)->width;
			ic->core.preedit_attr.area.height = ((XRectangle *)p->value)->height;
			attr->Area.x      = ic->core.preedit_attr.area.x;
			attr->Area.y      = ic->core.preedit_attr.area.y;
			attr->Area.width  = ic->core.preedit_attr.area.width;
			attr->Area.height = ic->core.preedit_attr.area.height;
			XIMP_SET_PREAREAMASK(ic, change_mask);

		} else if(strcmp(p->name, XNAreaNeeded)==0) {
			ic->core.preedit_attr.area_needed.width  = ((XRectangle *)p->value)->width;
			ic->core.preedit_attr.area_needed.height = ((XRectangle *)p->value)->height;
			attr->AreaNeeded.width  = ic->core.preedit_attr.area_needed.width;
			attr->AreaNeeded.height = ic->core.preedit_attr.area_needed.height;
			XIMP_SET_PREAREANEEDMASK(ic, change_mask);

		} else if(strcmp(p->name, XNSpotLocation)==0) {
			ic->core.preedit_attr.spot_location.x = ((XPoint *)p->value)->x;
			ic->core.preedit_attr.spot_location.y = ((XPoint *)p->value)->y;
			attr->SpotLocation.x = ic->core.preedit_attr.spot_location.x;
			attr->SpotLocation.y = ic->core.preedit_attr.spot_location.y;
			XIMP_SET_PRESPOTLMASK(ic, change_mask);

		} else if(strcmp(p->name, XNColormap)==0) {
			ic->core.preedit_attr.colormap = (Colormap)p->value;
			attr->Colormap = ic->core.preedit_attr.colormap;
			XIMP_SET_PRECOLORMAPMASK(ic, change_mask);

		} else if(strcmp(p->name, XNStdColormap)==0) {
			if( XGetRGBColormaps(ic->core.im->core.display,
					ic->core.focus_window, &colormap_ret,
					&count, (Atom)p->value) != 0) {
				ic->core.preedit_attr.std_colormap = (Atom)p->value;
				attr->StdColormap = ic->core.preedit_attr.std_colormap;
				XIMP_SET_PRESTDCOLORMAPMASK(ic, change_mask);
			} else {
				return_name = p->name;
				return(False);
			}

		} else if(strcmp(p->name, XNBackground)==0) {
			ic->core.preedit_attr.background = (unsigned long)p->value;
			attr->Background = ic->core.preedit_attr.background;
			XIMP_SET_PREBGMASK(ic, change_mask);

		} else if(strcmp(p->name, XNForeground)==0) {
			ic->core.preedit_attr.foreground = (unsigned long)p->value;
			attr->Foreground = ic->core.preedit_attr.foreground;
			XIMP_SET_PREFGMASK(ic, change_mask);

		} else if(strcmp(p->name, XNBackgroundPixmap)==0) {
			ic->core.preedit_attr.background_pixmap = (Pixmap)p->value;
			attr->Bg_Pixmap = ic->core.preedit_attr.background_pixmap;
			XIMP_SET_PREBGPIXMAPMASK(ic, change_mask);

		} else if(strcmp(p->name, XNFontSet)==0) {
			ic->core.preedit_attr.fontset = (XFontSet)p->value;
			if(p->value != NULL) {
				if(ic->ximp_icpart->preedit_font)
					Xfree(ic->ximp_icpart->preedit_font);
				list_ret = XFontsOfFontSet(ic->core.preedit_attr.fontset,
								&struct_list, &name_list);
				for(i = 0, len = 0; i < list_ret; i++) {
					len += (strlen(name_list[i]) + sizeof(char));
				}
				if( (tmp = Xmalloc(len + list_ret + sizeof(char))) == NULL ) {
				    return_name = p->name;
				    return( False );
				}
				tmp[0] = NULL;
				for(i = 0; i < list_ret; i++) {
					strcat(tmp, name_list[i]);
					strcat(tmp, ",");
				}
				tmp[len + i - 1] = NULL;
				ic->ximp_icpart->preedit_font = tmp;
				XIMP_SET_PREFONTMASK(ic, change_mask);
			} else {
				return_name = p->name;
				return(False);
			}

		} else if(strcmp(p->name, XNLineSpace)==0) {
			ic->core.preedit_attr.line_space = (long)p->value;
			attr->LineSpacing = ic->core.preedit_attr.line_space;
			XIMP_SET_PRELINESPMASK(ic, change_mask);

		} else if(strcmp(p->name, XNCursor)==0) {
			ic->core.preedit_attr.cursor = (Cursor)p->value;
			attr->Cursor = ic->core.preedit_attr.cursor;
			XIMP_SET_PRECURSORMASK(ic, change_mask);

		} else if(strcmp(p->name, XNPreeditStartCallback)==0) {
			ic->core.preedit_attr.callbacks.start.client_data =
				((XIMCallback *)p->value)->client_data;
			ic->core.preedit_attr.callbacks.start.callback =
				((XIMCallback *)p->value)->callback;
			ic->ximp_icpart->value_mask |= XIMP_PRE_CALLBAK;

		} else if(strcmp(p->name, XNPreeditDoneCallback)==0) {
			ic->core.preedit_attr.callbacks.done.client_data =
				((XIMCallback *)p->value)->client_data;
			ic->core.preedit_attr.callbacks.done.callback =
			((XIMCallback *)p->value)->callback;
			ic->ximp_icpart->value_mask |= XIMP_PRE_CALLBAK;

		} else if(strcmp(p->name, XNPreeditDrawCallback)==0) {
			ic->core.preedit_attr.callbacks.draw.client_data =
				((XIMCallback *)p->value)->client_data;
			ic->core.preedit_attr.callbacks.draw.callback =
				((XIMCallback *)p->value)->callback;
			ic->ximp_icpart->value_mask |= XIMP_PRE_CALLBAK;

		} else if(strcmp(p->name, XNPreeditCaretCallback)==0) {
			ic->core.preedit_attr.callbacks.caret.client_data =
				((XIMCallback *)p->value)->client_data;
			ic->core.preedit_attr.callbacks.caret.callback =
				((XIMCallback *)p->value)->callback;
			ic->ximp_icpart->value_mask |= XIMP_PRE_CALLBAK;
		}
	}
	return(True);
}

static Bool
_Ximp_StatusSetAttributes(ic, attr, vl, mode, change_mask, return_name)
Ximp_XIC		 ic;
Ximp_StatusPropRec4	*attr;
XIMArg			*vl;
int			 mode;
XimpChangeaMask 	 change_mask;
char			*return_name;
{
	XIMArg			*p;
	XStandardColormap 	*colormap_ret;
	int			 list_ret;
	XFontStruct		**struct_list;
	char			**name_list;
	int 			 i, len;
	int			 count;
	char 			*tmp;

	for(p = vl; p->name != NULL; p++) {
		if(strcmp(p->name, XNArea)==0) {
			ic->core.status_attr.area.x = ((XRectangle *)p->value)->x;
			ic->core.status_attr.area.y = ((XRectangle *)p->value)->y;
			ic->core.status_attr.area.width = ((XRectangle *)p->value)->width;
			ic->core.status_attr.area.height = ((XRectangle *)p->value)->height;
			attr->Area.x      = ic->core.status_attr.area.x;
			attr->Area.y      = ic->core.status_attr.area.y;
			attr->Area.width  = ic->core.status_attr.area.width;
			attr->Area.height = ic->core.status_attr.area.height;
			XIMP_SET_STSAREAMASK(ic, change_mask);

		} else if(strcmp(p->name, XNAreaNeeded)==0) {
			ic->core.status_attr.area_needed.width  = ((XRectangle *)p->value)->width;
			ic->core.status_attr.area_needed.height = ((XRectangle *)p->value)->height;
			attr->AreaNeeded.width  = ic->core.status_attr.area_needed.width;
			attr->AreaNeeded.height = ic->core.status_attr.area_needed.height;
			XIMP_SET_STSAREANEEDMASK(ic, change_mask);

		} else if(strcmp(p->name, XNColormap)==0) {
			ic->core.status_attr.colormap = (Colormap)p->value;
			attr->Colormap = ic->core.status_attr.colormap;
			XIMP_SET_STSCOLORMAPMASK(ic, change_mask);

		} else if(strcmp(p->name, XNStdColormap)==0) {
			if(XGetRGBColormaps(ic->core.im->core.display,
					ic->core.focus_window, &colormap_ret,
					&count, (Atom)p->value) !=0) {
				ic->core.status_attr.std_colormap = (Atom)p->value;
				attr->StdColormap = ic->core.status_attr.std_colormap;
				XIMP_SET_STSSTDCOLORMAPMASK(ic, change_mask);
			} else {
				return_name = p->name;
				return(False);
			}

		} else if(strcmp(p->name, XNBackground)==0) {
			ic->core.status_attr.background = (unsigned long)p->value;
			attr->Background = ic->core.status_attr.background;
			XIMP_SET_STSBGMASK(ic, change_mask);

		} else if(strcmp(p->name, XNForeground)==0) {
			ic->core.status_attr.foreground = (unsigned long)p->value;
			attr->Foreground = ic->core.status_attr.foreground;
			XIMP_SET_STSFGMASK(ic, change_mask);

		} else if(strcmp(p->name, XNBackgroundPixmap)==0) {
			ic->core.status_attr.background_pixmap = (Pixmap)p->value;
			attr->Bg_Pixmap = ic->core.status_attr.background_pixmap;
			XIMP_SET_STSBGPIXMAPMASK(ic, change_mask);

		} else if(strcmp(p->name, XNFontSet)==0) {
			ic->core.status_attr.fontset = (XFontSet)p->value;
			if (p->value != NULL) {
				if(ic->ximp_icpart->status_font)
					Xfree(ic->ximp_icpart->status_font);
				list_ret = XFontsOfFontSet(ic->core.status_attr.fontset,
								&struct_list, &name_list);
				for(i = 0, len = 0; i < list_ret; i++) {
					len += (strlen(name_list[i]) + sizeof(char));
				}
				if((tmp = Xmalloc(len + list_ret + sizeof(char))) == NULL){
				    return_name = p->name;
				    return( False );
				}
				tmp[0] = NULL;
				for(i = 0; i < list_ret; i++) {
					strcat(tmp, name_list[i]);
					strcat(tmp, ",");
				}
				tmp[len + i - 1] = NULL;
				ic->ximp_icpart->status_font = tmp;
				XIMP_SET_STSFONTMASK(ic, change_mask);
			} else {
				return_name = p->name;
				return(False);
			}

		} else if(strcmp(p->name, XNLineSpace)==0) {
			ic->core.status_attr.line_space = (long)p->value;
			attr->LineSpacing = ic->core.status_attr.line_space;
			XIMP_SET_STSLINESPMASK(ic, change_mask);

		} else if(strcmp(p->name, XNCursor)==0) {
			ic->core.status_attr.cursor = (Cursor)p->value;
			attr->Cursor = ic->core.status_attr.cursor;
			XIMP_SET_STSCURSORMASK(ic, change_mask);

		} else if(strcmp(p->name, XNStatusStartCallback)==0) {
			ic->core.status_attr.callbacks.start.client_data =
				((XIMCallback *)p->value)->client_data;
			ic->core.status_attr.callbacks.start.callback =
				((XIMCallback *)p->value)->callback;
			ic->ximp_icpart->value_mask |= XIMP_STS_CALLBAK;

		} else if(strcmp(p->name, XNStatusDoneCallback)==0) {
			ic->core.status_attr.callbacks.done.client_data =
				((XIMCallback *)p->value)->client_data;
			ic->core.status_attr.callbacks.done.callback =
				((XIMCallback *)p->value)->callback;
			ic->ximp_icpart->value_mask |= XIMP_STS_CALLBAK;

		} else if(strcmp(p->name, XNStatusDrawCallback)==0) {
			ic->core.status_attr.callbacks.draw.client_data =
				((XIMCallback *)p->value)->client_data;
			ic->core.status_attr.callbacks.draw.callback =
				((XIMCallback *)p->value)->callback;
			ic->ximp_icpart->value_mask |= XIMP_STS_CALLBAK;
		}
	}
	return(True);
}

static char *
_Ximp_Local_SetICValues(ic, values)
    Ximp_XIC ic;
    XIMArg *values;
{
    /* Not Yet */
    return(NULL);
}

static char *
_Ximp_Local_GetICValues(ic, values)
	Ximp_XIC	 ic;
	XIMArg		*values;
{
    XIMArg		*p;
    char		*p_char;
    char		*return_name = NULL;
    int		 len;

    for(p = values; p->name != NULL; p++) {
	if(strcmp(p->name, XNInputStyle) == 0) {
	    if(ic->ximp_icpart->value_mask & XIMP_INPUT_STYLE) {
		*((XIMStyle *)(p->value)) = ic->core.input_style;
	    } else {
		return_name = p->name;
		break;
	    }
	} else if(strcmp(p->name, XNClientWindow)==0) {
	    if(ic->ximp_icpart->value_mask & XIMP_CLIENT_WIN) {
		*((Window *)(p->value)) = ic->core.client_window;
	    } else {
		return_name = p->name;
		break;
	    }
	} else if(strcmp(p->name, XNFocusWindow)==0) {
	    if(XIMP_CHK_FOCUSWINMASK(ic)) {
		*((Window *)(p->value)) = ic->core.focus_window;
	    } else {
		return_name = p->name;
		break;
	    }
	} else if(strcmp(p->name, XNResourceName)==0) {
	    if(ic->core.im->core.res_name != (char *)NULL) {
		    len = strlen(ic->core.im->core.res_name);
		if((p_char = Xmalloc(len+1)) == NULL) {
		    return_name = p->name;
		    break;
		}
		strcpy(p_char, ic->core.im->core.res_name);
		*((char **)(p->value)) = p_char;
	    } else {
		return_name = p->name;
		break;
	    }
	} else if(strcmp(p->name, XNResourceClass)==0) {
	    if(ic->core.im->core.res_class != (char *)NULL) {
		len = strlen(ic->core.im->core.res_class);
		if((p_char = Xmalloc(len+1)) == NULL) {
		    return_name = p->name;
		    break;
		}
		strcpy(p_char, ic->core.im->core.res_class);
		*((char **)(p->value)) = p_char;
	    } else {
		return_name = p->name;
		break;
	    }
	} else if(strcmp(p->name, XNGeometryCallback)==0) {
	    if(ic->ximp_icpart->value_mask & XIMP_GEOMETRY_CB) {
		*((XIMCallback *)(p->value)) = ic->core.geometry_callback;
	    } else {
		return_name = p->name;
		break;
	    }
	} else if(strcmp(p->name, XNFilterEvents)==0) {
	    *((unsigned long *)(p->value)) = ic->core.filter_events;
	} else if(strcmp(p->name, XNPreeditAttributes)==0) {
	    if( _Ximp_PreGetAttributes(ic, p->value,
		&return_name) == False)
		break;
	} else if(strcmp(p->name, XNStatusAttributes)==0) {
	    if( _Ximp_StatusGetAttributes(ic, p->value,
		&return_name) == False)
		break;
	} else {
	    return_name = p->name;
	    break;
	}
    }
    return(return_name);
}

static Bool
_Ximp_PreGetAttributes(ic, vl, return_name)
Ximp_XIC	 ic;
XIMArg		*vl;
char		**return_name;
{
    XIMArg		*p;
    XRectangle	*p_rect;
    XPoint		*p_point;
    XIMCallback 	*p_callback;

    for(p = vl; p->name != NULL; p++) {
	if(strcmp(p->name, XNArea)==0) {
	    if(XIMP_CHK_PREAREAMASK(ic)) {
		if((p_rect = (XRectangle *)Xmalloc(sizeof(XRectangle))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_rect->x       = ic->core.preedit_attr.area.x;
		p_rect->y       = ic->core.preedit_attr.area.y;
		p_rect->width   = ic->core.preedit_attr.area.width;
		p_rect->height  = ic->core.preedit_attr.area.height;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	    *((XRectangle **)(p->value)) = p_rect;
	} else if(strcmp(p->name, XNAreaNeeded)==0) {
	    if(XIMP_CHK_PREAREANEEDMASK(ic)) {
		if((p_rect = (XRectangle *)Xmalloc(sizeof(XRectangle))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_rect->x  = p_rect->y  = 0;
		p_rect->width   = ic->core.preedit_attr.area_needed.width;
		p_rect->height  = ic->core.preedit_attr.area_needed.height;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	    *((XRectangle **)(p->value)) = p_rect;
	} else if(strcmp(p->name, XNSpotLocation)==0) {
	    if(XIMP_CHK_PRESPOTLMASK(ic)) {
		if((p_point = (XPoint *)Xmalloc(sizeof(XPoint))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_point->x = ic->core.preedit_attr.spot_location.x;
		p_point->y = ic->core.preedit_attr.spot_location.y;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	    *((XPoint **)(p->value)) = p_point;
	} else if(strcmp(p->name, XNColormap)==0) {
	    if(XIMP_CHK_PRECOLORMAPMASK(ic)) {
		*((Colormap *)(p->value)) = ic->core.preedit_attr.colormap;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNStdColormap)==0) {
	    if(XIMP_CHK_PRESTDCOLORMAPMASK(ic))
		*((Atom *)(p->value)) = ic->core.preedit_attr.std_colormap;
	    else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNBackground)==0) {
	    if(XIMP_CHK_PREBGMASK(ic)) {
		*((unsigned long *)(p->value)) = ic->core.preedit_attr.background;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNForeground)==0) {
	    if(XIMP_CHK_PREFGMASK(ic)) {
		*((unsigned long *)(p->value)) = ic->core.preedit_attr.foreground;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNBackgroundPixmap)==0) {
	    if(XIMP_CHK_PREBGPIXMAPMASK(ic)) {
		*((Pixmap *)(p->value)) = ic->core.preedit_attr.background_pixmap;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNFontSet)==0) {
	    if(XIMP_CHK_PREFONTMASK(ic)) {
		*((XFontSet *)(p->value)) = ic->core.preedit_attr.fontset;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNLineSpace)==0) {
	    if(XIMP_CHK_PRELINESPMASK(ic)) {
		*((int *)(p->value)) = ic->core.preedit_attr.line_space;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNCursor)==0) {
	    if(XIMP_CHK_PRECURSORMASK(ic)) {
		*((Cursor *)(p->value)) = ic->core.preedit_attr.cursor;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNPreeditStartCallback)==0) {
	    if((int)ic->core.preedit_attr.callbacks.start.callback) {

		if((p_callback = (XIMCallback *)Xmalloc(sizeof(XIMCallback))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_callback->client_data =
		    ic->core.preedit_attr.callbacks.start.client_data;
		p_callback->callback =
		    ic->core.preedit_attr.callbacks.start.callback;
		*((XIMCallback **)(p->value)) = p_callback;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNPreeditDrawCallback)==0) {
	    if((int)ic->core.preedit_attr.callbacks.draw.callback) {
		if((p_callback = (XIMCallback *)Xmalloc(sizeof(XIMCallback))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_callback->client_data =
		    ic->core.preedit_attr.callbacks.draw.client_data;
		p_callback->callback =
		    ic->core.preedit_attr.callbacks.draw.callback;
		*((XIMCallback **)(p->value)) = p_callback;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNPreeditDoneCallback)==0) {
	    if((int)ic->core.preedit_attr.callbacks.done.callback) {
		if((p_callback = (XIMCallback *)Xmalloc(sizeof(XIMCallback))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_callback->client_data =
		    ic->core.preedit_attr.callbacks.done.client_data;
		p_callback->callback =
		    ic->core.preedit_attr.callbacks.done.callback;
		*((XIMCallback **)(p->value)) = p_callback;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNPreeditCaretCallback)==0) {
	    if((int)ic->core.preedit_attr.callbacks.caret.callback) {
		if((p_callback = (XIMCallback *)Xmalloc(sizeof(XIMCallback))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_callback->client_data =
		    ic->core.preedit_attr.callbacks.caret.client_data;
		p_callback->callback =
		    ic->core.preedit_attr.callbacks.caret.callback;
		*((XIMCallback **)(p->value)) = p_callback;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	}
    }
    return(True);
}

static Bool
_Ximp_StatusGetAttributes(ic, vl, return_name)
    Ximp_XIC	 ic;
    XIMArg	 	*vl;
    char		**return_name;
{
    XIMArg		*p;
    XRectangle	*p_rect;
    XIMCallback 	*p_callback;

    for(p = vl; p->name != NULL; p++) {
	if(strcmp(p->name, XNArea)==0) {
	    if(XIMP_CHK_STSAREAMASK(ic)) {
		if((p_rect = (XRectangle *)Xmalloc(sizeof(XRectangle))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_rect->x       = ic->core.status_attr.area.x;
		p_rect->y       = ic->core.status_attr.area.y;
		p_rect->width   = ic->core.status_attr.area.width;
		p_rect->height  = ic->core.status_attr.area.height;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	    *((XRectangle **)(p->value)) = p_rect;
	} else if(strcmp(p->name, XNAreaNeeded)==0) {
	    if(XIMP_CHK_STSAREANEEDMASK(ic)) {
		if((p_rect = (XRectangle *)Xmalloc(sizeof(XRectangle))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_rect->x  = p_rect->y  = 0;
		p_rect->width   = ic->core.status_attr.area_needed.width;
		p_rect->height  = ic->core.status_attr.area_needed.height;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	    *((XRectangle **)(p->value)) = p_rect;
	} else if(strcmp(p->name, XNColormap)==0) {
	    if(XIMP_CHK_STSCOLORMAPMASK(ic)) {
		*((Colormap *)(p->value)) = ic->core.status_attr.colormap;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNStdColormap)==0) {
	    if(XIMP_CHK_STSSTDCOLORMAPMASK(ic)) {
		*((Atom *)(p->value)) = ic->core.status_attr.std_colormap;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNBackground)==0) {
	    if(XIMP_CHK_STSBGMASK(ic)) {
		*((unsigned long *)(p->value)) = ic->core.status_attr.background;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNForeground)==0) {
	    if(XIMP_CHK_STSFGMASK(ic)) {
		*((unsigned long *)(p->value)) = ic->core.status_attr.foreground;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNBackgroundPixmap)==0) {
	    if(XIMP_CHK_STSBGPIXMAPMASK(ic)) {
		*((Pixmap *)(p->value)) = ic->core.status_attr.background_pixmap;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNFontSet)==0) {
	    if(XIMP_CHK_STSFONTMASK(ic)) {
		*((XFontSet *)(p->value)) = ic->core.status_attr.fontset;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNLineSpace)==0) {
	    if(XIMP_CHK_STSLINESPMASK(ic)) {
		*((int *)(p->value)) = ic->core.status_attr.line_space;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNCursor)==0) {
	    if(XIMP_CHK_STSCURSORMASK(ic)) {
		*((Cursor *)(p->value)) = ic->core.status_attr.cursor;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNStatusStartCallback)==0) {
	    if((int)ic->core.status_attr.callbacks.start.callback) {
		if((p_callback = (XIMCallback *)Xmalloc(sizeof(XIMCallback))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_callback->client_data =
		    ic->core.status_attr.callbacks.start.client_data;
		p_callback->callback =
		    ic->core.status_attr.callbacks.start.callback;
		*((XIMCallback **)(p->value)) = p_callback;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNStatusDrawCallback)==0) {
	    if((int)ic->core.status_attr.callbacks.draw.callback) {
		if((p_callback = (XIMCallback *)Xmalloc(sizeof(XIMCallback))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_callback->client_data =
		    ic->core.status_attr.callbacks.draw.client_data;
		p_callback->callback =
		    ic->core.status_attr.callbacks.draw.callback;
		*((XIMCallback **)(p->value)) = p_callback;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	} else if(strcmp(p->name, XNStatusDoneCallback)==0) {
	    if((int)ic->core.status_attr.callbacks.done.callback) {
		if((p_callback = (XIMCallback *)Xmalloc(sizeof(XIMCallback))) == NULL) {
		    *return_name = p->name;
		    return(False);
		}
		p_callback->client_data =
		    ic->core.status_attr.callbacks.done.client_data;
		p_callback->callback =
		    ic->core.status_attr.callbacks.done.callback;
		*((XIMCallback **)(p->value)) = p_callback;
	    } else {
		*return_name = p->name;
		return(False);
	    }
	}
    }
    return(True);
}

static char *
_Ximp_Local_MbReset(ic)
    Ximp_XIC ic;
{
    ic->ximp_icpart->composed = (DefTree *)NULL;
    ic->ximp_icpart->context = ((Ximp_XIM)ic->core.im)->ximp_impart->top;
    return(NULL);
}

static wchar_t *
_Ximp_Local_WcReset(ic)
    Ximp_XIC ic;
{
    ic->ximp_icpart->composed = (DefTree *)NULL;
    ic->ximp_icpart->context = ((Ximp_XIM)ic->core.im)->ximp_impart->top;
    return(NULL);
}

static int
_Ximp_Local_MbLookupString(ic, ev, buffer, bytes, keysym, status)
    Ximp_XIC ic;
    XKeyEvent *ev;
    char * buffer;
    int bytes;
    KeySym *keysym;
    Status *status;
{
    int ret;

    if (ev->type != KeyPress) {
	if (status) *status = XLookupNone;
	return (0);
    }
    if (ev->keycode == 0) { /* Composed Event */
	ret = strlen(ic->ximp_icpart->composed->mb);
	if (ret > bytes) {
	    if (status) *status = XBufferOverflow;
	    return (ret);
	}
	bcopy(ic->ximp_icpart->composed->mb, buffer, ret);
	if (keysym) *keysym = NoSymbol;
	if (status) *status = XLookupChars;
	return (ret);
    } else { /* Throughed Event */
	ret = _Ximp_LookupMBText(ic, ev, buffer, bytes, keysym, NULL);
	if(ret > 0) {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupBoth;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
    }
    return (ret);
}

static int
_Ximp_Local_WcLookupString(ic, ev, buffer, wlen, keysym, status)
    Ximp_XIC ic;
    XKeyEvent *ev;
    wchar_t * buffer;
    int wlen;
    KeySym *keysym;
    Status *status;
{
    int ret;

    if (ev->type != KeyPress) {
	if (status) *status = XLookupNone;
	return (0);
    }
    if (ev->keycode == 0) { /* Composed Event */
	ret = _Xwcslen(ic->ximp_icpart->composed->wc);
	if (ret > wlen) {
	    if (status) *status = XBufferOverflow;
	    return (ret);
	}
	bcopy(ic->ximp_icpart->composed->wc, buffer, ret * sizeof(wchar_t));
	if (keysym) *keysym = NoSymbol;
	if (status) *status = XLookupChars;
	return (ret);
    } else { /* Throughed Event */
	ret = _Ximp_LookupWCText(ic, ev, buffer, wlen, keysym, NULL);
	if(ret > 0) {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupBoth;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
    }
    return (ret);
}
