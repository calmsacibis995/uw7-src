#pragma ident	"@(#)R5Xlib:Ximp/XimpCallbk.c	1.6"

/* $XConsortium: XimpCallbk.c,v 1.6 92/07/29 10:15:36 rws Exp $ */
/******************************************************************

              Copyright 1991, 1992 by Fuji Xerox Co.,Ltd.
              Copyright 1991, 1992 by FUJITSU LIMITED
              Copyright 1991, 1992 by Sun Microsystems, Inc.
              Copyright 1991, 1992 by Sony Corporation

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Fuji Xerox Co.,Ltd.,
FUJITSU LIMITED, Sun Microsystems, Inc. and Sony Corporation not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.
Fuji Xerox Co.,Ltd., FUJITSU LIMITED and Sony Corporation make
no representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.

FUJI XEROX CO.,LTD., FUJITSU LIMITED, SUN MICROSYSTEMS AND
SONY CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL FUJI XEROX CO.,LTD., FUJITSU LIMITED,
SUN MICROSYSTEMS AND SONY CORPORATION BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  Auther: Kazunori Nishihara,  Fuji Xerox Co.,Ltd.
                               kaz@ssdev.ksp.fujixerox.co.jp
          Takashi Fujiwara     FUJITSU LIMITED
                               fujiwara@a80.tech.yk.fujitsu.co.jp
          Hideki Hiura         Sun Microsystems, Inc.
                               hhiura@Sun.COM
******************************************************************/

#define NEED_EVENTS
#include "Xlibint.h"
#include "Xlcint.h"
#include "Xlibnet.h"

#include "Ximplc.h"
#ifdef SYSV386
#include <sys/byteorder.h>
#endif

extern void	_Ximp_IM_SendMessage();
extern Bool     _Ximp_CMPredicate8();

void
_Ximp_CallGeometryCallback(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;

    cb = &xic->core.geometry_callback;
    if (cb->callback) {
	(*cb->callback) (xic, cb->client_data, NULL);
    }
}

void
_Ximp_CallPreeditStartCallback(ic, event)
    Ximp_XIC        ic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;
             int          data;

    cb = &ic->core.preedit_attr.callbacks.start;
    if (cb->callback) {
	data = (*(int (*) ()) cb->callback) (ic, cb->client_data, NULL);
	ic->ximp_icpart->cbstatus |= XIMPCBPREEDITACTIVE ;
    } else {
	data = -1;
    }
    _Ximp_IM_SendMessage(ic, XIMP_PREEDITSTART_RETURN(ic), data, NULL, NULL);
}

void
_Ximp_CallPreeditDoneCallback(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;

    cb = &xic->core.preedit_attr.callbacks.done;
    if (cb->callback) {
	(*cb->callback) (xic, cb->client_data, NULL);
	xic->ximp_icpart->cbstatus &= ~XIMPCBPREEDITACTIVE ;
    }
}

void
_Ximp_CallPreeditDrawCallback(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;
    XIMPreeditDrawCallbackStruct CallData;
    XIMText         cbtext;
    char           *ctext;
    int             length;
    Atom            type;
    int             format;
    unsigned long   nitems, after;
    Ximp_PreeditDrawDataProp data;

    bzero(&CallData, sizeof(XIMPreeditDrawCallbackStruct));
    bzero(&cbtext, sizeof(XIMText));
    bzero(&data, sizeof(Ximp_PreeditDrawDataProp));

    cb = &xic->core.preedit_attr.callbacks.draw;
    if (cb->callback) {
	if (XGetWindowProperty(xic->core.im->core.display,
			  ((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
			       event->data.l[2], 0, 3, True, AnyPropertyType,
			       &type, &format, &nitems, &after,
			       (unsigned char **) &data) == Success) {
	    if (data) {
		CallData.caret = data->caret;
		CallData.chg_first = data->chg_first;
		CallData.chg_length = data->chg_length;
		Xfree(data);
	    } else {
		CallData.caret = 0;
		CallData.chg_first = 0;
		CallData.chg_length = 0;
	    }
	} else {
	    /* Error */
	    CallData.chg_length = -1;
	}
	if (event->data.l[4]) {
	    if (XGetWindowProperty(xic->core.im->core.display,
				   ((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
				   event->data.l[4], 0, 4096, True, AnyPropertyType,
				   &type, &format, &nitems, &after,
				   (unsigned char **) &cbtext.feedback) == Success) {
		cbtext.length = nitems;
	    } else {
		cbtext.length = 0 ;
	    }
	    if (cbtext.length <=0) {
		if (cbtext.feedback)
		  Xfree(cbtext.feedback);
		cbtext.feedback = NULL ;
	    }
	} else {
	    cbtext.feedback = NULL ;
	}
	/*
	 * nitems == 0 usually means same feedback as before.
	 * But if text length is also 0, then deem it as
	 * feedback == NULL (text deletion)
	 */
	if (event->data.l[3]) {
	    if (XGetWindowProperty(xic->core.im->core.display,
				   ((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
				   event->data.l[3], 0, 4096, True, AnyPropertyType,
				   &type, &format, &nitems, &after,
				   (unsigned char **) &ctext) == Success) {
		if (nitems > 0) {
		    int  ctlen = nitems ;
		    if (ctlen > XIMP_MAXBUF) {
			ctlen = XIMP_MAXBUF;
		    }
		    length = ctlen;
		    /*
		     * wide_char is union with multi_byte.
		     */

		    if( (cbtext.string.wide_char = (wchar_t *) Xmalloc(ctlen * sizeof(wchar_t))) == NULL ) {
			length = 0;
		    }
		    else {
			bzero(cbtext.string.wide_char, sizeof(wchar_t) * ctlen);
			if (IS_USE_WCHAR(xic)) {
			    cbtext.encoding_is_wchar = True;
			    if (_Ximp_cttowcs(xic->core.im->core.lcd, ctext,
					      nitems, cbtext.string.wide_char,
					      &length, NULL) < 0) {
				length = 0;
			    }
			} else {
			    cbtext.encoding_is_wchar = False;
			    if (_Ximp_cttombs(xic->core.im->core.lcd, ctext,
					      nitems, cbtext.string.multi_byte,
					      &length, NULL) < 0) {
				length = 0;
			    }
			}
		    }

		    if (cbtext.feedback == NULL) {
			if (IS_USE_WCHAR(xic)) {
			    if (!(cbtext.length = length)) {
				if (cbtext.string.wide_char)
				  Xfree(cbtext.string.wide_char);
				cbtext.string.wide_char = NULL;
			    }
			} else {
			    if (strlen(cbtext.string.multi_byte) == 0) {
				cbtext.length = 0 ;
				if (cbtext.string.multi_byte)
				  Xfree(cbtext.string.multi_byte);
				cbtext.string.multi_byte = NULL;
				
			    } else {
				if ((length =
				     _Ximp_mbs_charlen(xic->core.im->core.lcd,
						       cbtext.string.multi_byte,
						       length)) < 0) {
				    length = 0 ;
				    if (cbtext.string.multi_byte)
					Xfree(cbtext.string.multi_byte);
				    cbtext.string.multi_byte = NULL;
				}
				cbtext.length = length;
			    }
			}
		    }
		} else {
		    /*
		     * No preedit string.
		     * feedback updates only
		     */
		    cbtext.string.multi_byte = NULL;
		}
		Xfree((XPointer) ctext);
	    } else {
		/*
		 * No preedit string.
		 * feedback updates only
		 */
		cbtext.string.multi_byte = NULL;
	    }
	} else {
	    cbtext.string.multi_byte = NULL;
	}
	if ((cbtext.string.multi_byte == NULL) && cbtext.feedback == NULL) {
	    /*
	     * text deletion
	     */
	    CallData.text = NULL ;
	} else {
	    CallData.text = &cbtext;
	}

	(*cb->callback) (xic, cb->client_data, &CallData);
	if (IS_USE_WCHAR(xic)) {
	    if (cbtext.string.wide_char) {
		Xfree((XPointer) (cbtext.string.wide_char));
	    }
	} else {
	    if (cbtext.string.multi_byte) {
		Xfree((XPointer) (cbtext.string.multi_byte));
	    }
	}
	if (cbtext.feedback)
	    Xfree((XPointer) cbtext.feedback);
    } else {
	XDeleteProperty(xic->core.im->core.display,
			((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
			event->data.l[2]);
	XDeleteProperty(xic->core.im->core.display,
			((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
			event->data.l[3]);
	XDeleteProperty(xic->core.im->core.display,
			((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
			event->data.l[4]);
    }
}

void
_Ximp_CallPreeditDrawCallback2(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb = &xic->core.preedit_attr.callbacks.draw ;
    XIMPreeditDrawCallbackStruct CallData;
    XIMText         cbtext;
    char           *text;
    int             length;
    XEvent          ev;
    short	    pdcbStatus = (short) ((event->data.l[2] >> 16) & 0xffffl);
    int             ctlen = 0;
    Atom            type;
    int             format;
    unsigned long   nitems, after;
    unsigned char	*tmp;		/* for multiple ClientMessage */
    unsigned char	*tmpp;		/* for multiple ClientMessage */
    int			 work;
    int		    i;
    XimpCMPredicateArgRec	Arg;

    tmpp = tmp = NULL;

    bzero(&CallData, sizeof(XIMPreeditDrawCallbackStruct));
    bzero(&cbtext, sizeof(XIMText));

/**
 * post Ximp 3.4 protocol maybe compliant. 
 * XIMP status flag will may contain the supplementary infomations to 
 * reassemble the XIMPreeditDrawCallbackStruct.
 *	  +-----------------------------------------+
 *	0 | XIMP_PREEDITDRAW_CM                     |
 *	  +-----------------------------------------+
 *	4 | ICID                                    |
 *	  +-------------------+---------------------+
 *	8 |PreeditDrawCBStatus|       caret         |
 *	  +-------------------+---------------------+
 *	12|      chg_first    |      chg_length     |
 *	  +-------------------+---------------------+
 *	16|               feedback                  |
 *	  +-----------------------------------------+
 * PreeditDrawCBStatus:
 *    0x0001 no_text:  if 1, string == NULL (no following client message.)
 *    0x0002 no_feedback: if 1 feedback == NULL
 *    0x0004 feedbacks_via_property: if 1 , feedback field is property atom#
 **/
    CallData.caret = (long)(event->data.l[2] & 0xffffl);
    CallData.chg_first = (long) ((event->data.l[3] >> 16) & 0xffffl);
    CallData.chg_length = (long) (event->data.l[3] & 0xffffl);
    CallData.text = &cbtext;

    if (cb->callback) {
	if (pdcbStatus & XIMP_PDCBSTATUS_NOTEXT) {
	    cbtext.string.multi_byte = NULL ;
	    if (pdcbStatus & XIMP_PDCBSTATUS_NOFEEDBACK) {
		CallData.text = NULL ;
	    } else {
		if (!(pdcbStatus & XIMP_PDCBSTATUS_FEEDBACKS_VIA_PROP)) {
		    /* error */
		} else {
		    /*  
		     * Not implemented yet.
		     */
		    if (XGetWindowProperty(xic->core.im->core.display,
					   ((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
					   event->data.l[4], 0, 4096, True, AnyPropertyType,
					   &type, &format, &nitems, &after,
					   (unsigned char **) &cbtext.feedback) == Success) {
			cbtext.length = nitems;
		    } else {
			cbtext.length = 0 ;
		    }
		}
	    }
	} else { /* if preedit text is exist */
	    unsigned char *ct = NULL ;

	    ctlen = _Ximp_CombineMultipleCM(xic, &ct);

	    length = ctlen * XIMP_MB_CUR_MAX(xic->core.im->core.lcd);
	    
	    if (IS_USE_WCHAR(xic)) {
		if( (cbtext.string.wide_char = (wchar_t *) Xmalloc((length + 1) * sizeof(wchar_t))) == NULL ) {
		    length = 0;
		}
		else {
		    bzero(cbtext.string.wide_char, sizeof(wchar_t) * (length + 1));
		    cbtext.encoding_is_wchar = True;
		    if (_Ximp_cttowcs(xic->core.im->core.lcd,
				      ct, ctlen,
				      cbtext.string.wide_char,
				      &length, NULL) < 0) {
			length = 0;
		    }
		}
		cbtext.length = length;
	    } else {
		if( (cbtext.string.multi_byte = Xmalloc(length + 1)) == NULL ) {
		    length = 0;
		}
		else {
		    bzero(cbtext.string.multi_byte, length + 1);
		    cbtext.encoding_is_wchar = False;
		    if (_Ximp_cttombs(xic->core.im->core.lcd,
				      ct, ctlen,
				      cbtext.string.multi_byte,
				      &length, NULL) < 0) {
			length = 0;
		    }
		    else if ((length = _Ximp_mbs_charlen(xic->core.im->core.lcd,
				       cbtext.string.multi_byte,
				       length)) < 0) {
			length = 0;
		    }
		}
		cbtext.length = length;
	    }
	    if(pdcbStatus & XIMP_PDCBSTATUS_NOFEEDBACK) {
		cbtext.feedback = NULL;
	    } else if(pdcbStatus & XIMP_PDCBSTATUS_FEEDBACKS_VIA_PROP) {
		    if (XGetWindowProperty(xic->core.im->core.display,
					   ((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
					   event->data.l[4], 0, 4096, True, AnyPropertyType,
					   &type, &format, &nitems, &after,
					   (unsigned char **) &cbtext.feedback) == Success) {
			cbtext.length = nitems;
		    } else {
			cbtext.length = 0 ;
		    }
	    } else {
		if( cbtext.feedback = (XIMFeedback *) Xmalloc(cbtext.length * sizeof(XIMFeedback)) )
		    for (i = 0; i < (int) cbtext.length; i++)
			cbtext.feedback[i] = event->data.l[4];
	    }
	}
	(*cb->callback) (xic, cb->client_data, &CallData);
	if (IS_USE_WCHAR(xic)) {
	    if (cbtext.string.wide_char)
	      Xfree((XPointer) (cbtext.string.wide_char));
	} else {
	    if (cbtext.string.multi_byte)
	      Xfree((XPointer) (cbtext.string.multi_byte));
	}
	if(tmp)
		Xfree((unsigned char *) tmp);
	if (cbtext.feedback)
	  Xfree((XPointer) cbtext.feedback);
    }
}

void
_Ximp_CallPreeditDrawCallback3(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb = &xic->core.preedit_attr.callbacks.draw ;
    XIMPreeditDrawCallbackStruct CallData;
    XIMText         cbtext;
    unsigned long text_data[2];
    static wchar_t local_buf[16] = {0};		/* rm bss data */
    int length = 16;

    bzero(&CallData, sizeof(XIMPreeditDrawCallbackStruct));
    bzero(&cbtext, sizeof(XIMText));

/**
 * post Ximp 3.4 protocol maybe compliant. 
 *	  +---------------------------------------------+
 *	0 | XIMP_PREEDITDRAW_CM_TINY                    |
 *	  +---------------------------------------------+
 *	4 | ICID                                        |
 *	  +-------------------+------------+------------+
 *	8 |    chg_first      | chg_length |   length   |
 *	  +-------------------+------------+------------+
 *	12|    string (COMPOUND TEXT, Network order)    |
 *	  +-------------------+-------------------------+
 *	16|    string (continued)                       |
 *	  +---------------------------------------------+
 * caret = chg_first + length_in_char_of_insert_string
 **/
    CallData.caret = (long)(event->data.l[2] & 0xffffl);

    CallData.chg_first = (long) ((event->data.l[2] >> 16) & 0xffffl);
    CallData.chg_length = (long) ((event->data.l[2] >> 8) & 0xffl);
    cbtext.feedback = (XIMFeedback *)NULL;
    CallData.text = &cbtext;
    text_data[0] = htonl(event->data.l[3]);
    text_data[1] = htonl(event->data.l[4]);

    if (cb->callback) {
	if (IS_USE_WCHAR(xic)) {
	    cbtext.encoding_is_wchar = True;
	    cbtext.string.wide_char = local_buf;
	    if (_Ximp_cttowcs(xic->core.im->core.lcd, (char *)text_data, (event->data.l[2] & 0xffl), cbtext.string.wide_char, &length, NULL) >= 0) {
		cbtext.length = length;
		(*cb->callback) (xic, cb->client_data, &CallData);
	    }
	} else {
	    cbtext.encoding_is_wchar = False;
	    cbtext.string.multi_byte = (char *)local_buf;
	    if (_Ximp_cttombs(xic->core.im->core.lcd, (char *)text_data, (event->data.l[2] & 0xffl), cbtext.string.multi_byte, &length, NULL) >= 0) {
		cbtext.length = _Ximp_mbs_charlen(xic->core.im->core.lcd, cbtext.string.multi_byte, length);
		(*cb->callback) (xic, cb->client_data, &CallData);
	    }
	}
    }
}

void
_Ximp_CallPreeditCaretCallback(ic, event)
    Ximp_XIC        ic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;
    XIMPreeditCaretCallbackStruct CallData;
#define ToXIMCaretStyle(x) ((XIMCaretStyle)(x))
#define ToXIMCaretDirection(x) ((XIMCaretDirection)(x))

    cb = &ic->core.preedit_attr.callbacks.caret;
    if (cb->callback) {
	CallData.position = event->data.l[2];
	CallData.direction = ToXIMCaretDirection(event->data.l[3]);
	CallData.style = ToXIMCaretStyle(event->data.l[4]);
	(*cb->callback) (ic, cb->client_data, &CallData);
	    _Ximp_IM_SendMessage(ic, XIMP_PREEDITCARET_RETURN(ic),
		CallData.position, NULL, NULL);
    }
}

void
_Ximp_CallStatusStartCallback(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;

    cb = &xic->core.status_attr.callbacks.start;
    if (cb->callback) {
	(*cb->callback) (xic, cb->client_data, NULL);
	xic->ximp_icpart->cbstatus |= XIMPCBSTATUSACTIVE ;
    }
}

void
_Ximp_CallStatusDoneCallback(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;

    cb = &xic->core.status_attr.callbacks.done;
    if (cb->callback) {
	(*cb->callback) (xic, cb->client_data, NULL);
	xic->ximp_icpart->cbstatus &= ~XIMPCBSTATUSACTIVE ;
    }
}

void
_Ximp_CallStatusDrawCallback(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;
    char           *text;
    int             length;
    XIMStatusDrawCallbackStruct CallData;
    XIMText         cbtext;

    bzero(&CallData, sizeof(XIMStatusDrawCallbackStruct));
    bzero(&cbtext, sizeof(XIMText));

#define ToXIMStatusDataType(x) ((XIMStatusDataType)(x))

    cb = &xic->core.status_attr.callbacks.draw;
    CallData.type = ToXIMStatusDataType(event->data.l[2]);
    if (CallData.type == XIMTextType) {
	Atom            type;
	int             format;
	unsigned long   nitems, after;
	CallData.data.text = &cbtext;
	if (event->data.l[4]) {
	    if (XGetWindowProperty(xic->core.im->core.display,
			      ((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
			       event->data.l[4], 0, 4096, True, AnyPropertyType,
				   &type, &format, &nitems, &after,
			      (unsigned char **) &cbtext.feedback) == Success) {
		cbtext.length = nitems;
	    } else {
		cbtext.feedback = NULL;
		cbtext.length = 0;
	    }
	} else {
	    cbtext.feedback = NULL;
	    cbtext.length = 0;
	}
	if (XGetWindowProperty(xic->core.im->core.display,
			  ((Ximp_XIM) xic->core.im)->ximp_impart->fe_window,
			   event->data.l[3], 0, 4096, True, AnyPropertyType,
			       &type, &format, &nitems, &after,
			       (unsigned char **) &text) == Success) {
	    if (IS_USE_WCHAR(xic)) {
		if( (cbtext.string.wide_char = (wchar_t *) Xmalloc((XIMP_MAXBUF + 1) * sizeof(wchar_t))) == NULL ) {
		    length = 0;
		}
		else {
		    bzero(cbtext.string.wide_char, (XIMP_MAXBUF + 1) * sizeof(wchar_t));
		    length = XIMP_MAXBUF;
		    if (_Ximp_cttowcs(xic->core.im->core.lcd, text, nitems,
				      cbtext.string.wide_char,
				      &length, NULL) < 0) {
			length = 0;
		    }
		}
		cbtext.length = length;
		Xfree((XPointer) text);
		cbtext.encoding_is_wchar = True;
	    } else {
		if( (cbtext.string.multi_byte = Xmalloc(XIMP_MAXBUF + 1)) == NULL ) {
		    length = 0;
		}
		else {
		    bzero(cbtext.string.multi_byte, XIMP_MAXBUF + 1);
		    length = XIMP_MAXBUF;
		    if (_Ximp_cttombs(xic->core.im->core.lcd, text, nitems,
				      cbtext.string.multi_byte,
				      &length, NULL) < 0) {
			length = 0;
		    }
		    if (cbtext.length == 0) {
			if ((length = _Ximp_mbs_charlen(xic->core.im->core.lcd,
					   cbtext.string.multi_byte,
					   length)) < 0) {
			    length = 0 ;
			}
		    }
		    cbtext.length  = length;
		}
		Xfree((XPointer) text);
		cbtext.encoding_is_wchar = False;
	    }
	} else {
	    if (IS_USE_WCHAR(xic)) {
		cbtext.string.wide_char = (wchar_t *) NULL;
		cbtext.length = 0;
	    } else {
		cbtext.string.multi_byte = NULL;
		cbtext.length = 0;
	    }
	}
	if (cb->callback) {
	    (*cb->callback) (xic, cb->client_data, &CallData);
	}
	if (IS_USE_WCHAR(xic) ) {
	    if( cbtext.string.wide_char ) {
		Xfree((XPointer) (cbtext.string.wide_char));
	    }
	}
	else {
	    if( cbtext.string.multi_byte ) {
		Xfree((XPointer) (cbtext.string.multi_byte));
	    }
	}
	if (cbtext.feedback)
	    Xfree((XPointer) cbtext.feedback);
    } else {			/* XIMBitmapType */
	CallData.data.bitmap = (Pixmap) event->data.l[3];
	if (cb->callback) {
	    (*cb->callback) (xic, cb->client_data, &CallData);
	}
    }
}

void
_Ximp_CallStatusDrawCallback2(xic, event)
    Ximp_XIC        xic;
    XClientMessageEvent *event;
{
    register XIMCallback *cb;
    unsigned char        *ct = NULL;
    int             length;
    XIMStatusDrawCallbackStruct CallData;
    XIMText         cbtext;
    int		    ctlen = 0;	/* for multiple ClientMessage */

    cb = &xic->core.status_attr.callbacks.draw;
    CallData.type = ToXIMStatusDataType(event->data.l[2]);
    if (CallData.type == XIMTextType) {
	CallData.data.text = &cbtext;
	
	ctlen = _Ximp_CombineMultipleCM(xic, &ct);
	
	if (cb->callback) {
	    length = ctlen * XIMP_MB_CUR_MAX(xic->core.im->core.lcd);
	    if (IS_USE_WCHAR(xic)) {
		cbtext.encoding_is_wchar = True;
		if( (cbtext.string.wide_char = (wchar_t *) Xmalloc((XIMP_MAXBUF + 1) * sizeof(wchar_t))) == NULL ) {
		    length = 0;
		}
		else {
		    bzero(cbtext.string.wide_char,(XIMP_MAXBUF + 1) * sizeof(wchar_t));
		    if (_Ximp_cttowcs(xic->core.im->core.lcd,
				      ct, ctlen,
				      cbtext.string.wide_char,
				      &length, NULL) < 0) {
			length = 0;
		    }
		}
	    } else {
		cbtext.encoding_is_wchar = False;
		if( (cbtext.string.multi_byte = Xmalloc(length + 1)) == NULL ) {
		    length = 0;
		}
		else {
		    bzero(cbtext.string.multi_byte, length + 1);
		    if (_Ximp_cttombs(xic->core.im->core.lcd,
				      ct, ctlen,
				      cbtext.string.multi_byte,
				      &length, NULL) < 0) {
			length = 0;
		    }
		    Xfree((XPointer) ct);
		    if ((length = _Ximp_mbs_charlen(xic->core.im->core.lcd,
					       cbtext.string.multi_byte,
					       length)) < 0) {
			length = 0;
		    }
		}
	    }
	    cbtext.length = length;
	    if (event->data.l[4] != -1) {
		int             i;

		if( cbtext.feedback = (XIMFeedback *) Xmalloc(cbtext.length * sizeof(long)) )
		    for (i = 0; i < (int) cbtext.length; i++)
			cbtext.feedback[i] = event->data.l[4];
	    } else {
		cbtext.feedback = NULL;
	    }
	    (*cb->callback) (xic, cb->client_data, &CallData);
	    if( IS_USE_WCHAR(xic) ) {
		if( cbtext.string.wide_char ) {
		    Xfree((XPointer) (cbtext.string.wide_char));
		}
	    }
	    else {
		if( cbtext.string.multi_byte ) {
		    Xfree((XPointer) (cbtext.string.multi_byte));
		}
	    }
	    if (cbtext.feedback)
		Xfree((XPointer) cbtext.feedback);
	}
    } else {			/* XIMBitmapType */
	if (cb->callback) {
	    CallData.data.bitmap = (Pixmap) event->data.l[3];
	    (*cb->callback) (xic, cb->client_data, &CallData);
	}
    }
}
