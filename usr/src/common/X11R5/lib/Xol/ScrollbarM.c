#ifndef NOIDENT
#ident	"@(#)scrollbar:ScrollbarM.c	1.5"
#endif

/* #includes go here    */
#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/ScrollbarP.h>

extern Boolean _OlmSBFindOp OL_ARGS((Widget, XEvent *, unsigned char *));
extern void _OlmSBHighlightHandler OL_ARGS((Widget, OlDefine));
extern void _OlmSBMakePageInd OL_ARGS((ScrollbarWidget));
extern Widget _OlmSBCreateMenu OL_ARGS((Widget, OlDefine));
extern Boolean _OlmSBMenu  OL_ARGS((Widget, XEvent *));
extern void _OlmSBUpdatePageInd OL_ARGS((ScrollbarWidget, Boolean, Boolean));

#define SWB             sw->scroll
#define HORIZ(W)        ((W)->scroll.orientation == OL_HORIZONTAL)

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */
/*
 *************************************************************************
 * _OlmSBHighlightHandler - changes the colors when this widget gains or
 *  loses focus.
 ****************************procedure*header*****************************
 */
extern void
_OlmSBHighlightHandler OLARGLIST((w, highlight_type))
	OLARG(Widget,	w)
	OLGRA(OlDefine,	highlight_type)
{
#define SUPERCLASS	\
	((ScrollbarClassRec *)scrollbarClassRec.core_class.superclass)

	/*  Call the standard Motif HighlightHandler in Primitive. */
	(*SUPERCLASS->primitive_class.highlight_handler)(w, highlight_type);

#undef SUPERCLASS
} /* END OF _OlmSBHighlightHandler() */

/*
 *************************************************************************
 * _OlmSBMakePageInd - called from the Realize and SetValues to create the
 *  scrollbar page indicator for Open Look.  A NoOp for Motif.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
extern void
_OlmSBMakePageInd OLARGLIST((sw))
    OLGRA(ScrollbarWidget, sw)
{
	/*  Does nothing in Motif since there is no page indicator. */
	return;
}  /* END OF _OlmSBMakePageInd() */

/*
 *************************************************************************
 * _OlmSBUpdatePageInd - called from the MoveSlider and highlight to position
 * and draw the scrollbar page indicator for Open Look.  A NoOp for Motif.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
extern void
_OlmSBUpdatePageInd OLARGLIST((sw, draw, move))
    OLARG(ScrollbarWidget, sw)
    OLARG(Boolean, draw)
    OLGRA(Boolean, move)
{
	/*  Does nothing in Motif since there is no page indicator. */
	return;
}  /* END OF _OlmSBUpdatePageInd() */


/*
 *************************************************************************
 * _OlmSBMenu - called from the SBButtonHandler and SBActivageWidget to
 *  popup the scrollbar's menu.  This is a NoOp in Motif.
 ****************************procedure*header*****************************
 */
/* ARGSUSED */
extern Boolean _OlmSBMenu  OLARGLIST((w, event))
    OLARG(Widget, w)
    OLGRA(XEvent *, event)
{
	/*  Does nothing in Motif since there is no menu. */
	return(False);
}  /* END OF _OlmSBMenu() */

/* ARGSUSED */
extern Widget
_OlmSBCreateMenu OLARGLIST((parent, orientation))
	OLARG(Widget, parent)
	OLGRA(OlDefine, orientation)
{
	return((Widget) NULL);
}  /* end of _OlmSBCreateMenu() */

/*
 *************************************************************************
 * _OlmSBFindOp - called from the SelectDown to determine the type of
 *  operation the user intends.
 ****************************procedure*header*****************************
 */
extern Boolean
_OlmSBFindOp OLARGLIST((w, event, op_p))
    OLARG(Widget, w)
    OLARG(XEvent *, event)
    OLGRA(unsigned char *, op_p)
{
	ScrollbarWidget sw = (ScrollbarWidget)w;
	int point;
	int bottomAnchorPos, topAnchorPos, elevEndPos;
	unsigned char opcode = NOOP;

	if (HORIZ (sw))
	{
	    if (event->xbutton.y < sw->scroll.offset ||
		event->xbutton.y >= sw->scroll.offset +
		    (Position) sw->scroll.anchlen)
		return(False);
	    point = event->xbutton.x;
	    topAnchorPos = sw->scroll.anchwidth +
				sw->primitive.shadow_thickness;
	    bottomAnchorPos = sw->core.width - (sw->scroll.anchwidth +
				sw->primitive.shadow_thickness);
	    elevEndPos = sw->scroll.elevwidth;
	}
	else
	{
	    if (event->xbutton.x < sw->scroll.offset ||
		event->xbutton.x >= sw->scroll.offset +
		    (Position) sw->scroll.anchwidth)
		return(False);
	    point = event->xbutton.y;
	    topAnchorPos = sw->scroll.anchlen +
				sw->primitive.shadow_thickness;
	    bottomAnchorPos = sw->core.height - (sw->scroll.anchlen +
				sw->primitive.shadow_thickness);
	    elevEndPos = sw->scroll.elevheight;
	}

	if (sw->scroll.type != SB_MINIMUM) {
       		if (point < topAnchorPos)  {
               		opcode = GRAN_DEC;
		}
       		else if (point >= bottomAnchorPos) {
               		opcode = GRAN_INC;
		}
		else
               		/* all subsequent checks are relative to elevator */
               		point -= sw->scroll.sliderPValue;
	}

	if (opcode == NOOP) {
               if (point < 0)  {
                       opcode = PAGE_DEC;
		}
               else if (point >= elevEndPos) {
                       opcode = PAGE_INC;
		}
	       else  {
               	       opcode = DRAG_ELEV;
		}
       }

       if (opcode == DRAG_ELEV) {
		if (sw->scroll.type == SB_REGULAR) {
			/* record pointer based pos. for dragging */
			sw->scroll.dragbase = SWB.sliderPValue - (HORIZ(sw) ?
				   event->xbutton.x : event->xbutton.y) -
				   topAnchorPos;
		}
	}

	*op_p = opcode;

	/*  Retrun true if event is to be consumed; otherwise false. */
	return(True);

}  /* end of _OlmSBFindOp() */
