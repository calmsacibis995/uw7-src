#ifndef EXM_SCROLLUITL_H
#define EXM_SCROLLUITL_H

#ifndef NOIDENT
#pragma ident	"@(#)libMDtI:ScrollUtil.h	1.7"
#endif

extern Boolean
ExmSWinCreateScrollbars(
	Widget,		/* w   - child of the XmScrolledWindow widget	*/
	Widget *,	/* hsb - horizontal scrollbar widget id		*/
	Widget *,	/* vsb - vertical scrollbar widget id		*/
	WidePosition *,	/* x_offset - initial offset values in vertical */
	WidePosition *,	/* y_offset - and horizontal directions...	*/
	XtCallbackProc	/* cb -	 callback for both horiz/vert scrollbars*/
);

extern void
ExmSWinSetupScrollbars(
	Widget,		/* w   - child of XmScrolledWindowWidget	   */
	Widget,		/* hsb - Horizontal XmScrollBarWidget id	   */
	Widget,		/* vsb - Vertical XmScrollBarWidget id	     	   */
	XtCallbackProc	/* cb  - callback for both horiz/vert scrollbars   */
);

extern void
ExmSWinHandleValueChange(
	Widget,		/* w - child of XmScrolledWindowWidget		*/
	Widget,		/* sb - Hori/Vertical XmScrollBarWidget		*/
	GC,		/* gc - GCGraphicsExposures or not ? 		*/
	int,		/* new_val - new slider value in Hor or Ver dir */
	Dimension,	/* x_uom					*/
	Dimension,	/* y_uom - slider units in x/y directions	*/
	WidePosition *,	/* x_offset					*/
	WidePosition *	/* y_offset - current offset values		*/
);

extern XtGeometryResult
ExmSWinHandleResize(
	Widget,		/* w - child of XmScrolledWindow		*/
	Widget,		/* hsb - Horizontal XmScrollBarWidget		*/
	Widget,		/* vsb - Vertical XmScrollBarWidget		*/
	WidePosition,	/* min_x - for hsb's XmNminimum calculation	*/
	WidePosition,	/* min_y - for vsb's XmNminimum	calculation	*/
	WideDimension,	/* width - actual width/height			*/
	WideDimension,	/* height					*/
	Dimension,	/* x_uom					*/
	Dimension,	/* y_uom - slider units in x/y directions	*/
	WidePosition *,	/* x_offset - in: current offset values,	*/
	WidePosition *,	/* y_offset	out: new offset values...	*/
	XtWidgetGeometry *, /* request					*/
	XtWidgetGeometry *  /* reply - reply/preferred			*/
);

extern Boolean
ExmSWinCalcViewSize(
	Widget,		/* w - child of XmScrolledWindow		*/
	Widget,		/* hsb - Horizontal XmScrollBarWidget		*/
	Widget,		/* vsb - Vertical XmScrollBarWidget		*/
	Dimension,	/* x_uom					*/
	Dimension,	/* y_uom - slider units in x/y directions	*/
	Dimension,	/* req_wd - use them if the value is not 0,	*/
	Dimension,	/* req_hi   otherwise compute from its parent	*/
	WidePosition,	/* min_x - for hsb's XmNminimum calculation	*/
	WidePosition,	/* min_y - for vsb's XmNminimum	calculation	*/
	WidePosition *,	/* x_offset - in: current offset values,	*/
	WidePosition *,	/* y_offset	out: new offset values...	*/
	WideDimension *,/* width - in: actual child window size,	*/
	WideDimension *	/* height	  out: view size...		*/
);

#endif /* EXM_SCROLLUTIL_H */
