#ifndef	NOIDENT
#ident	"@(#)notice:ModalM.c	1.8"
#endif

/*******************************file*header*******************************
    Description: Modal.c - OPEN LOOK(TM) Modal Widget - Motif-mode
*/

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ModalP.h>
#include <Xol/VendorI.h>

#define ClassName ModalM
#include <Xol/NameDefs.h>

/*
 * Convenient macros:
 */

#define MODAL_C(wc) (((ModalShellWidgetClass)(wc))->modal_class)
#define CORE_C(wc) (((WidgetClass)(wc))->core_class)
#define COMPOSITE_C(wc) (((CompositeWidgetClass)(wc))->composite_class)
#define SUPER_C(wc) CORE_C(wc).superclass

#define CORE_P(w) ((Widget)(w))->core
#define COMPOSITE_P(w) ((ModalShellWidget)(w))->composite
#define MODAL_P(w) ((ModalShellWidget)(w))->modal_shell
#define MPART(w)		( &((ModalShellWidget)(w))->modal_shell )

#define LeaveParams		LeaveWindowMask, False, LeaveEH, NULL
#define AddLeaveEH(w)		XtAddEventHandler(w, LeaveParams)
#define RemoveLeaveEH(w)	XtRemoveEventHandler(w, LeaveParams)

#define MapParams		StructureNotifyMask, False, MapEH, NULL
#define AddMapEH(w)		XtAddEventHandler(w, MapParams)
#define RemoveMapEH(w)		XtRemoveEventHandler(w, MapParams)

#define MAX_PICTURE_WIDTH	22
#define MAX_PICTURE_HEIGHT	24

/**************************forward*declarations***************************

    Forward function definitions listed by category:
		1. Private functions
		2. Class   functions
		3. Action  functions
		4. Public  functions
 */
						/* private procedures */
						/* class procedures */
						/* action procedures */

static void GetGlyph OL_ARGS((
	ModalShellWidget w
));
static void LeaveEH OL_ARGS((
	ModalShellWidget,
	XtPointer,
	XEvent *,
	Boolean *
));
static void MapEH OL_ARGS((
	ModalShellWidget,
	XtPointer,
	XEvent *,
	Boolean *
));
static void SizeMessageGlyph OL_ARGS((
	ModalShellWidget w,
	char ** data,
	unsigned int * width,
	unsigned int * height
));

						/* public procedures */
extern void _OlmMDAddEventHandlers OL_ARGS((
	Widget
));
extern Boolean _OlmMDCheckSetValues OL_ARGS((
	ModalShellWidget,
	ModalShellWidget
));
extern void	_OlmMDLayout OL_ARGS((
	Widget			w,
	Boolean			resizable,
	Boolean			query_only,
	Boolean			cached_best_fit_hint,
	Widget			who_asking,
	XtWidgetGeometry *	request,
	XtWidgetGeometry *	response
));
extern void		_OlmMDRedisplay OL_ARGS((
	Widget			w,
	XEvent *		event,
	Region			region
));
extern void _OlmMDRemoveEventHandlers OL_ARGS((
	Widget
));

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables
*/
/* XtNnoticeType bitmaps */

static OLconst unsigned char NoticeTypeErrorPic[] = {
   0x00, 0x00, 0x00, 0xc0, 0x0f, 0x00, 0xf0, 0x3a, 0x00, 0x58, 0x55, 0x00,
   0x2c, 0xa0, 0x00, 0x56, 0x40, 0x01, 0xaa, 0x80, 0x02, 0x46, 0x81, 0x01,
   0x8a, 0x82, 0x02, 0x06, 0x85, 0x01, 0x0a, 0x8a, 0x02, 0x06, 0x94, 0x01,
   0x0a, 0xe8, 0x02, 0x14, 0x50, 0x01, 0x28, 0xb0, 0x00, 0xd0, 0x5f, 0x00,
   0xa0, 0x2a, 0x00, 0x40, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static OLconst unsigned char NoticeTypeInfoPic[] = {
   0x00, 0x00, 0x78, 0x00, 0x54, 0x00, 0x2c, 0x00, 0x54, 0x00, 0x28, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x2a, 0x00, 0x5c, 0x00, 0x28, 0x00,
   0x58, 0x00, 0x28, 0x00, 0x58, 0x00, 0x28, 0x00, 0x58, 0x00, 0x28, 0x00,
   0x58, 0x00, 0xae, 0x01, 0x56, 0x01, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00
};

static OLconst  unsigned char NoticeTypeQuestionPic[] = {
   0xf0, 0x3f, 0x00, 0x58, 0x55, 0x00, 0xac, 0xaa, 0x00, 0xd6, 0x5f, 0x01,
   0xea, 0xbf, 0x02, 0xf6, 0x7f, 0x01, 0xea, 0xba, 0x02, 0xf6, 0x7d, 0x05,
   0xea, 0xba, 0x0a, 0x56, 0x7d, 0x15, 0xaa, 0xbe, 0x1e, 0x56, 0x5f, 0x01,
   0xac, 0xaf, 0x02, 0x58, 0x57, 0x01, 0xb0, 0xaf, 0x00, 0x60, 0x55, 0x01,
   0xa0, 0xaa, 0x00, 0x60, 0x17, 0x00, 0xa0, 0x2f, 0x00, 0x60, 0x17, 0x00,
   0xb0, 0x2a, 0x00, 0x50, 0x55, 0x00
};

static OLconst unsigned char NoticeTypeWarningPic[] = {
   0x00, 0x00, 0x18, 0x00, 0x2c, 0x00, 0x56, 0x00, 0x2a, 0x00, 0x56, 0x00,
   0x2a, 0x00, 0x56, 0x00, 0x2c, 0x00, 0x14, 0x00, 0x2c, 0x00, 0x14, 0x00,
   0x2c, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x14, 0x00,
   0x2c, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00
};

static OLconst unsigned char NoticeTypeWorkingPic[] = {
   0x00, 0x00, 0x00, 0xfe, 0xff, 0x0f, 0xaa, 0xaa, 0x0a, 0x44, 0x55, 0x06,
   0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06, 0xcc, 0x2a, 0x02, 0x84, 0x15, 0x06,
   0x8c, 0x2a, 0x02, 0x04, 0x15, 0x06, 0x0c, 0x0a, 0x02, 0x04, 0x06, 0x06,
   0x0c, 0x0b, 0x02, 0x84, 0x15, 0x06, 0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06,
   0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06, 0xcc, 0x2a, 0x02, 0x44, 0x55, 0x06,
   0xfe, 0xff, 0x0f, 0x56, 0x55, 0x05, 0x00, 0x00, 0x00
};


/******************************function*header****************************
    GetGlyph.
*/
static void
GetGlyph OLARGLIST((w))
	OLGRA(ModalShellWidget, w)
{
	char *picture_data;
	unsigned int picture_width, picture_height;

	SizeMessageGlyph(w, &picture_data, &picture_width, &picture_height);

	if (w->modal_shell.pixmap)  {
		XFreePixmap(XtDisplay(w), w->modal_shell.pixmap);
	}
	w->modal_shell.pixmap = XCreatePixmapFromBitmapData(
		XtDisplay(w), XtWindow(w),
		picture_data,
		picture_width, picture_height,
		0, w->core.background_pixel,
		DefaultDepthOfScreen(XtScreen(w)));

}  /* end of GetGlyph() */

/******************************function*header****************************
    SizeMessageGlyph.
*/
static void
SizeMessageGlyph OLARGLIST((w, data, width, height))
	OLARG(ModalShellWidget, w)
	OLARG(char **, data)
	OLARG(unsigned int *, width)
	OLGRA(unsigned int *, height)
{
	switch(w->modal_shell.noticeType) {
		case OL_INFORMATION:
			*data = (char *)NoticeTypeInfoPic;
			*width = (unsigned)11;
			*height = (unsigned)24;
			break;
		case OL_QUESTION:
			*data = (char *)NoticeTypeQuestionPic;
			*width = (unsigned)22;
			*height = (unsigned)22;
			break;
		case OL_WARNING:
			*data = (char *)NoticeTypeWarningPic;
			*width = (unsigned)9;
			*height = (unsigned)22;
			break;
		case OL_WORKING:
			*data = (char *)NoticeTypeWorkingPic;
			*width = (unsigned)21;
			*height = (unsigned)23;
			break;
		default:
			w->modal_shell.noticeType = OL_ERROR;
			/* FALLTHROUGH */
		case OL_ERROR:
			*data = (char *)NoticeTypeErrorPic;
			*width = (unsigned)20;
			*height = (unsigned)20;
			break;
	} /* switch */
}  /* end of SizeMessageGlyph() */

/**
 ** _OlmMDAddEventHandlers()
 **/
/*ARGSUSED*/
extern void
_OlmMDAddEventHandlers OLARGLIST((w))
	OLGRA(Widget, w)
{
}  /* end of _OlmMDAddEventHandlers() */

/**
 ** _OlmMDCheckSetValues()
 **/
extern Boolean
_OlmMDCheckSetValues OLARGLIST((current,new))
	OLARG(ModalShellWidget, current)
	OLGRA(ModalShellWidget, new)
{
	Boolean layout = False;

	new->modal_shell.warp_pointer = FALSE;

	if ((MPART(new)->noticeType != MPART(current)->noticeType)) {
		GetGlyph(new);
		layout = True;
	}

	return(layout);
}  /* end of _OlmMDCheckSetValues() */


/**
 ** _OlmMDLayout()
 **/
/*ARGSUSED*/
extern void
_OlmMDLayout OLARGLIST((w, resizable, query_only, cached_best_fit_hint,
	who_asking, request, response))
	OLARG(Widget,			w)
	OLARG(Boolean,			resizable)
	OLARG(Boolean,			query_only)
	OLARG(Boolean,			cached_best_fit_hint)
	OLARG(Widget,			who_asking)
	OLARG(XtWidgetGeometry *,	request)
	OLGRA(XtWidgetGeometry *,	response)
{
	Cardinal		nchildren	= COMPOSITE_P(w).num_children;
	Widget *		child		= COMPOSITE_P(w).children;
	Widget			static_text = NULL;
	Widget			control_area = NULL;
	Dimension		core_height, core_width;
	Dimension		preferred_ca_height, preferred_st_height;
	Dimension		preferred_ca_width, preferred_st_width;
	Dimension		_2points = OlPointToPixel(OL_HORIZONTAL, 2);
	Dimension		_10points = OlPointToPixel(OL_HORIZONTAL, 10);
	Dimension		_350points = OlPointToPixel(OL_HORIZONTAL, 350);
	char *			picture_data;
	unsigned int		picture_width, picture_height;
	XtWidgetGeometry	preferred;
	XtWidgetGeometry	available;
#define CA_BW (2*CORE_P(control_area).border_width)
#define ST_BW (2*CORE_P(static_text).border_width)

	/*
	 * Compute optimum fit based on the size of the children.
	 * It always has a 10 point margin and 10 point space between
	 * children.
	 */
	SizeMessageGlyph((ModalShellWidget)w, &picture_data,
					&picture_width, &picture_height);

	preferred.width	= (_10points * 3) + picture_width;
	preferred.height	= (_10points * 2);

	/*  For the Modal, we assume that the child[0] is the 
	    static text widget and the child[1] is the control
	    area (or other type used for the buttons.)  */
	if (nchildren > (unsigned) 0 && XtIsManaged(child[0])) {
		XtWidgetGeometry req, reply;

		static_text = child[0];
		if (who_asking == static_text)  {
			/*  Allow the requested height.  */
			preferred_st_height = request->height;
			preferred_st_width = _OlMin(request->width, _350points);
		}
		else if (CORE_P(static_text).width > _350points)  {
			/*  Ask that the static text be 350 points wide. */
			req.width = _350points;
			req.request_mode = CWWidth;
			XtQueryGeometry(static_text, &req, &reply);
			if (reply.request_mode & CWHeight)
				preferred_st_height = reply.height;
			else
				preferred_st_height = CORE_P(static_text).height;
			if (reply.request_mode  & CWWidth)
				preferred_st_width = reply.width;
			else
				preferred_st_width = _350points;

		}
		else  {  /* let it be shorter. */
			preferred_st_height = CORE_P(static_text).height;
			preferred_st_width = CORE_P(static_text).width;
		}
		preferred.height += _OlMax((preferred_st_height + ST_BW),
					picture_height);
		preferred.width += preferred_st_width + ST_BW;
	}
	if (nchildren > 1 && XtIsManaged(child[1]))  {
		XtWidgetGeometry req, reply;

		control_area = child[1];

		if (who_asking == control_area)  {
			preferred_ca_width = request->width;
			preferred_ca_height = request->height;
		}
		else  {
			/*  Ask the control area its preferred size. */
			req.request_mode = NULL;
			XtQueryGeometry(control_area, &req, &reply);
			if (reply.request_mode & CWHeight)
				preferred_ca_height = reply.height;
			else
				preferred_ca_height = CORE_P(control_area).height;
			if (reply.request_mode  & CWWidth)
				preferred_ca_width = reply.width;
			else
				preferred_ca_width = CORE_P(control_area).width;
		}
		preferred.height += preferred_ca_height + CA_BW;
		preferred.width = _OlMax((int)(preferred_ca_width +
			(_10points * 2) + CA_BW), (int)preferred.width);
	}

        /*  Protect against the zero size protocol. */
	if (preferred.width == 0)
		preferred.width = 1;
	if (preferred.height == 0)
		preferred.height = 1;

        /*  Determine the available geometry according to the flags given */
	/*  Avoid the OlAvailableGeometry call when we know that it is a
	    query_only situation.  Assume that the window manager will grant
	    the preferred size.  The alternative is that the root 
	    geometry manager ignores the QueryOnly flag and actually sets
	    the size of the window to be the preferred (queried) size.
	    Then later the widget gets out of sync with the size of the
	    window.
	*/
	if (query_only)
		available = preferred;
	else
		OlAvailableGeometry(w, resizable, query_only, who_asking,
			request, &preferred, &available);

	/*
	 * Skip the rest of this if our parent is just asking.
	 */
	if (who_asking == w) {
		if (response) {
			response->x            = CORE_P(w).x;
			response->y            = CORE_P(w).y;
			response->width        = available.width;
			response->height       = available.height;
			response->border_width = CORE_P(w).border_width;
		}
		return;
	}

	/*
	 * Place all of the children in the space given in available.
	 */
	core_height = available.height;
	core_width = available.width;

	if (static_text  && XtIsManaged(static_text))  {
		/*
		 * Set up the maximum preferred size.  We let the static text
		 * be its preferred preferred size from the query above.
		 * If this is too high, then it will be clipped, and the
		 * stacking order of the control area will
		 * insure that the buttons are accessible.
		 */
		if (who_asking != static_text && !query_only)  {
			/*  The Modal is being resized from the parent.
			    Use the size of the window. */
			if ((_10points * 3) + picture_width >= core_width) 
				preferred_st_width = 1;
			else
				preferred_st_width = core_width - 
					(_10points * 3) - picture_width - ST_BW;
			if ((int)((_10points * 2) + preferred_ca_height + CA_BW + ST_BW) >= (int)core_height)
				preferred_st_height = 1;
			else  {
				preferred_st_height = core_height -
				    _10points * 2 - preferred_ca_height - CA_BW;
				preferred_st_height -= ST_BW;
			}
			XtConfigureWidget (
				static_text,
				(_10points*2) + picture_width,
				_10points,
				preferred_st_width,
				preferred_st_height,
				CORE_P(static_text).border_width);
		}
		else  if (who_asking == static_text)  {
			if (response) {
				response->x            = (_10points*2) + picture_width;
				response->y            = _10points;
				response->width        = preferred_st_width;
				response->height       = preferred_st_height;
				response->border_width = CORE_P(static_text).border_width;
			}
			if (query_only)
				return;
		}
	}
	
	/*  Now size and position the control area widget if it isn't the
	    one asking.  The control area will be centered and placed from
	    the bottom of the window leaving a 10 point margin. */
	if (control_area && XtIsManaged(control_area))  {
		Position	ca_x, ca_y;

		/*  Center the control area in the modal.
		 */
		if ((int)core_width >(int)(preferred_ca_width + CA_BW))
			ca_x = (Position)(core_width - (preferred_ca_width + CA_BW)) / 2;
		else
			ca_x = 0;
		ca_y = ((int)core_height > (int)(preferred_ca_height + CA_BW)) ?
			core_height - preferred_ca_height - CA_BW :
			0;
		if (who_asking != control_area && !query_only)  {
			/*  Clear the line based on the old ca geometry */
			if (XtIsRealized(w))  {
				if (MODAL_P(w).line_y == 0)
					MODAL_P(w).line_y = ca_y - _2points;
				XClearArea(XtDisplay(w), XtWindow(w), 
					(Position) OlPointToPixel(OL_HORIZONTAL, 1),
					MODAL_P(w).line_y,
					core_width,
					_2points, 
					True);
			}
			XtConfigureWidget ( control_area,
				ca_x, ca_y,
				preferred_ca_width, preferred_ca_height,
				CORE_P(control_area).border_width);
		}
		else if (who_asking == control_area)  {
			if (response) {
				response->x            = ca_x;
				response->y            = ca_y;
				response->width        = preferred_ca_width;
				response->height       = preferred_ca_height;
				response->border_width = CORE_P(control_area).border_width;
			}
			if (query_only)
				return;
		}
	}

	return;
} /* end of _OlmMDLayout() */

/**
 ** _OlmMDRedisplay()
 **/
/*ARGSUSED*/
extern void
_OlmMDRedisplay OLARGLIST((w, event, region))
	OLARG(Widget,	w)
	OLARG(XEvent *,	event)
	OLGRA(Region,	region)
{
	ModalShellWidget mw = (ModalShellWidget) w;
	Position H1point = (Position) OlPointToPixel(OL_HORIZONTAL, 1);
	Position _10points = (Position) OlPointToPixel(OL_HORIZONTAL, 10);
	Position _2points = (Position) OlPointToPixel(OL_HORIZONTAL, 2);
	char *picture_data;
	unsigned int picture_width, picture_height;
	Widget ca;
	Cardinal		nchildren	= COMPOSITE_P(w).num_children;
	Widget *		child		= COMPOSITE_P(w).children;

	if (mw->modal_shell.pixmap == NULL)
		GetGlyph((ModalShellWidget)w);
	SizeMessageGlyph(mw, &picture_data, &picture_width, &picture_height);

	/*  Draw the pixmap.  */
	XCopyArea(XtDisplay(w),
		mw->modal_shell.pixmap, XtWindow(w),
		(OlgIs3d() ? OlgGetBg3GC(mw->modal_shell.attrs) :
			OlgGetFgGC(mw->modal_shell.attrs)),
		0, 0,
		picture_width, picture_height,
		_10points, _10points);

	/*  Find the control_area */
	ca = NULL;
	if (nchildren >= 2)  {
		int i;
		Boolean st_found = False;

		for (i=0; i < nchildren; i++)  {
			if (XtIsManaged(child[i]))  {
				if (st_found)
					ca = child[i];
				else
					st_found = True;
			}
		}
	}

	/*  Draw a two point thick etched line between the Modal children. */
	if (ca && (int)mw->core.height > (int)(ca->core.height + _2points + (2*ca->core.border_width))) {
		mw->modal_shell.line_y = (Position) (mw->core.height -
					ca->core.height - (2*ca->core.border_width) - _2points);
		OlgDrawLine( XtScreen(w),
			XtWindow(w),
			mw->modal_shell.attrs,
			(Position)H1point,
			mw->modal_shell.line_y,
			(Dimension)(mw->core.width - (H1point * 2)),
			(unsigned) 2, (unsigned) False);
	}
			
}  /*  end of _OlmMDRedisplay() */

/**
 ** _OlmMDRemoveEventHandlers()
 **/
/*ARGSUSED*/
extern void
_OlmMDRemoveEventHandlers OLARGLIST((w))
	OLGRA(Widget, w)
{
	/*  Nothing to remove. */
}  /* end of _OlmMDRemoveEventHandlers() */
