#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/xtarg.h	1.9"
#endif

#ifndef _XTARG_H
#define _XTARG_H

#define CLR_ARGS()		_XTN_ = 0
#define SET_ARGS(name, value)	XtSetArg(_XTA_[_XTN_],(name),(value));++_XTN_
#define SET_VALUES(w)		XtSetValues((w), _XTA_, _XTN_)
#define GET_VALUES(w)		XtGetValues((w), _XTA_, _XTN_)
#define CREATE_WIDGET(n, c, p)	XtCreateManagedWidget((n),(c),(p),_XTA_,_XTN_)
#define CREATE_U_WIDGET(n,c,p)	XtCreateWidget((n),(c),(p),_XTA_,_XTN_)
#define CREATE_POPUP(n, c, p)	XtCreatePopupShell((n),(c),(p),_XTA_,_XTN_)

#define DCL_ARGS(n) \
	static Arg		_XTA_[(n)]; \
	static Cardinal		_XTN_

/*
 *	For handling the attributes of individual items
 *	in a flattened widget...
 */

#define FSET_VALUES(w, i)	OlFlatSetValues((w), (i), _XTA_, _XTN_)
#define FGET_VALUES(w, i)	OlFlatGetValues((w), (i), _XTA_, _XTN_)

#endif
