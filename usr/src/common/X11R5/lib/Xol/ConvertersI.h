/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)olmisc:ConvertersI.h	1.1"
#endif

#if	!defined(_CONVERTERSI_H_)
#define	_CONVERTERSI_H_

#include <Xol/Converters.h>

#define DeclareConversionClass(display,type,class) \
	XtAppContext	__app	= XtDisplayToApplicationContext(display);\
	String		__type	= type;				\
	String		__class	= (class? class : "OlToolkitError")

#define DeclareDestructionClass(app,type,class) \
	XtAppContext	__app	= app;				\
	String		__type	= type;				\
	String		__class	= (class? class : "OlToolkitError")

#define ConversionError(name,dflt,params,num_params) \
	XtAppErrorMsg (__app, name, __type, __class, dflt, params, num_params)

#define ConversionWarning(name,dflt,params,num_params) \
	XtAppWarningMsg (__app, name, __type, __class, dflt, params, num_params)

#define DestructionWarning	ConversionWarning
#define DestructionError	ConversionError

#define ConversionDone(type,value) \
{								\
	if (to->addr) {						\
		if (to->size < sizeof(type)) {			\
			to->size = sizeof(type);		\
			return (False);				\
		}						\
		*(type *)(to->addr) = (value);			\
	} else {						\
		static type		static_value;		\
								\
		static_value = (value);				\
		to->addr = (XtPointer)&static_value;		\
	}							\
	to->size = sizeof(type);				\
} return (True)

#define BOOL	1
#define FONT	2

#if defined(__STDC__)
#define OLET(x) OleTmsg ## x
#define OLEM(x) OleMfileConverters_msg ## x
#else
#define OLET(x)	OleTmsg/**/x
#define OLEM(x) OleMfileConverters_msg/**/x  
#endif

#endif
