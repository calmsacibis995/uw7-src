#ifndef EXM_WIDEPOSDEF_H
#define EXM_WIDEPOSDEF_H

#ifndef	NOIDENT
#pragma	ident	"@(#)libMDtI:WidePosDef.h	1.2"
#endif


/************************************************************************
  Description:
	This header file contains the `WidePosition' definition.

	Xt defines `Position' as `short'. This is fine for most widgets
	except for the widgets like icon_box, list, text etc.. The
	number of icons, list items, and text lines in a scrolled window
	can be very large especially when dealing with the virtual work
	window (i.e., the window size is equal to the view size). It is
	very easy to overflow (x, y) in these situations. Supporting
	`WidePosition' reduces such possibilities.

	Further, in many systems (e.g, 386/486), sizeof(int) equals to
	sizeof(long), so we can just use `int' because Xt supports XtRInt.
	You should enable USE_LONG_FOR_WIDE_POSITION if sizeof(int) does
	not equal to sizeof(long) (usually this means sizeof(int) equal to
	sizeof(short). In this case, you shall also provide a StringToLong
	converter. `USE_LONG_FOR_WIDE_POSITION' is undefined by default.
*/

#ifndef USE_LONG_FOR_WIDE_POSITION

typedef unsigned int	WideDimension;
typedef int		WidePosition;
#define XmRWidePosition	XmRInt

#else /* USE_LONG_FOR_WIDE_POSITION */

	/* shall provide a coverter in this case */
typedef unsigned long	WideDimension;
typedef long		WidePosition;
#define XmRLong		"Long"
#define XmRWidePosition	XmRLong

#endif /* USE_LONG_FOR_WIDE_POSITION */

#endif /* EXM_WIDEPOSDEF_H */
