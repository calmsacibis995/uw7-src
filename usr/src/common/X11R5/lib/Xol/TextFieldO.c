#ifndef	NOIDENT
#ident	"@(#)textfield:TextFieldO.c	2.7"
#endif

#include "X11/IntrinsicP.h"
#include "X11/StringDefs.h"

#include "Xol/OpenLookP.h"
#include "Xol/TextFieldP.h"

#define ClassName TextFieldO
#include <Xol/NameDefs.h>

/*
 * Convenient macros:
 */

#define TEXTEDIT_P(w) ((TextFieldWidget)(w))->textedit

#define TE(W) ((TextEditWidget)(W))

#define CanStep(W) (TEXTFIELD_C(XtClass(W)).step != (OlTextFieldStepProc)0)
#define CanScroll(W) TEXTFIELD_P(W).can_scroll

#define LeftArrowActive(W) (CanScroll(W) && TEXTEDIT_P(W).xOffset != 0)

#define RightArrowActive(W) \
	(CanScroll(W)							\
	 && ((int)(TEXTEDIT_P(W).maxX + TEXTEDIT_P(W).xOffset)		\
					> (int)PAGE_R_MARGIN(TE(W))))

/*
 * Private routines:
 */

static void		Initialize OL_ARGS((
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
));
static Boolean 		Scroll OL_ARGS((
	Widget			w,
	Arrow			arrow
));
static void		ClearArrow OL_ARGS((
	Widget			w,
	Arrow			arrow
));
static Boolean		CursorVisible OL_ARGS((
	Widget			w
));
static void		KeepTime OL_ARGS((
	XtPointer		client_data,
	XtIntervalId *		id		/*NOTUSED*/
));
static void		ResetMargin OL_ARGS((
	Widget			w
));
static void		DrawButton OL_ARGS((
	Widget			w,
	XRectangle *		rect,
	Boolean			depressed
));
static void		OverrideDefaultTextEditResources OL_ARGS((
	Widget			w,
	Dimensions *		d
));
static void 		MotionVerificationCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
));

/**
 ** _OloTFClassInitialize()
 **/

void
#if	OlNeedFunctionPrototypes
_OloTFClassInitialize (
	Indirect *		indirect
)
#else
_OloTFClassInitialize (indirect)
	Indirect *		indirect;
#endif
{
	TEXTFIELD_C(textFieldWidgetClass).scroll = Scroll;

	indirect->initialize = Initialize;
	indirect->set_values = 0;
	indirect->reset_margin = ResetMargin;
	indirect->draw_button = DrawButton;
	indirect->override_default_textedit_resources
				= OverrideDefaultTextEditResources;

	return;
} /* _OloTFClassInitialize */

/**
 ** Initialize()
 **/

static void
#if	OlNeedFunctionPrototypes
Initialize (
	Widget			request,	/*NOTUSED*/
	Widget			new,
	ArgList			args,		/*NOTUSED*/
	Cardinal *		num_args	/*NOTUSED*/
)
#else
Initialize (request, new, args, num_args)
	Widget			request;
	Widget			new;
	ArgList			args;
	Cardinal *		num_args;
#endif
{
	XtAddCallback (new, XtNmotionVerification, MotionVerificationCB, (XtPointer)0);
	return;
} /* Initialize */

/**
 ** Scroll()
 **/

static Boolean 
#if	OlNeedFunctionPrototypes
Scroll (
	Widget			w,
	Arrow			arrow
)
#else
Scroll (w, arrow)
	Widget			w;
	Arrow			arrow;
#endif
{
	Boolean			continue_to_poll = False;

	int			delta;

	XRectangle		rect;


	/*
	 * Scrolling is pretty simple: We shift the position of the text
	 * left or right by changing the .xOffset field in the TextEdit
	 * widget part, then tell TextEdit to redraw the text (using an
	 * undocumented routine--oh well). We don't worry about the text
	 * colliding with the scrolling arrows, we'll use the margin
	 * callback (TextEdit issues it when it draws text) to redraw
	 * the scrolling arrows on top of the text.
	 *
	 * The only tricky thing is to see if the text cursor has been
	 * shifted underneath the scrolling arrows. If it has, we need to
	 * turn it off and keep it off until it shifts back into view.
	 */

	switch (arrow) {
	case ArrowLeft:
		/*
		 * "Max", not "Min", because .xOffset is negative. But we
		 * do want the smallest absolute value.
		 */
		delta = _OlMax(
		    -FONTWID(TE(w)), TEXTEDIT_P(w).xOffset
		);
		TEXTEDIT_P(w).xOffset -= delta;
		if (LeftArrowActive(w))
			continue_to_poll = True;
		break;

	case ArrowRight:
		delta = _OlMin(
		    FONTWID(TE(w)),
		    TEXTEDIT_P(w).maxX + TEXTEDIT_P(w).xOffset
				       - PAGEWID(TE(w))
		);
		TEXTEDIT_P(w).xOffset -= delta;
		if (RightArrowActive(w))
			continue_to_poll = True;
		break;
	default:
		return (False);
	}

	/*
	 * Scrolling to the left may cause the right scrolling arrow
	 * to no longer be needed, and vise versa. When this happens
	 * (see the use of LeftArrowActive and RightArrowActive, above)
	 * we need to erase all traces of the arrow.
	 */
	if (!continue_to_poll)
		ClearArrow (w, arrow);

	rect.x =
	rect.y = 0;
	rect.width = CORE_P(w).width;
	rect.height = CORE_P(w).height;
	_DisplayText (w, &rect);

	/*
	 * Check the visibility of the cursor after redisplaying the text,
	 * since _DisplayText will update the cursor position.
	 */
	if (!CursorVisible(w))
	    _TurnTextCursorOff(w);

	return (continue_to_poll);
} /* Scroll */

/**
 ** ClearArrow()
 **/

static void
#if	OlNeedFunctionPrototypes
ClearArrow (
	Widget			w,
	Arrow			arrow
)
#else
ClearArrow (w, arrow)
	Widget			w;
	Arrow			arrow;
#endif
{
	Rectangles		r;
	XRectangle *		pr;

	_OlTFComputeRectangles (w, &r);
	pr = arrow == ArrowLeft? &r.left : &r.right;
	XClearArea (
		XtDisplay(w), XtWindow(w),
		pr->x, pr->y, pr->width, pr->height,
		False
	);
	return;
} /* ClearArrow */

/**
 ** CursorVisible()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
CursorVisible (
	Widget			w
)
#else
CursorVisible (w)
	Widget			w;
#endif
{
	int			xL;
	int			xR;
	Rectangles		r;
	Boolean                 visible = TRUE;

	_OlTFComputeRectangles (w, &r);
	xL = TEXTEDIT_P(w).cursor_x - TEXTEDIT_P(w).CursorP->xoffset;
	xR = xL + TEXTEDIT_P(w).CursorP->width;

	if (LeftArrowActive(w) && (r.left.x + (int)r.left.width >= xL))
	    visible = FALSE;

	if (RightArrowActive(w) && (xR >= (int)r.right.x))
	    visible = FALSE;

	return visible;

} /* CursorVisible */


/**
 ** ResetMargin()
 **/

static void
#if	OlNeedFunctionPrototypes
ResetMargin (
	Widget			w
)
#else
ResetMargin (w)
	Widget			w;
#endif
{
#define RESET(W,ATTRACT,REPEL) \
	int min = TEXTEDIT_P(w).ATTRACT + TEXTEDIT_P(w).lineHeight;	\
	if ((int)CORE_P(w).height > min)				\
		TEXTEDIT_P(w).REPEL = CORE_P(w).height - min;		\
	else								\
		TEXTEDIT_P(w).REPEL = 0

#if	defined(NORTH_GRAVITY)
	RESET (w, topMargin, bottomMargin);
#else
	RESET (w, bottomMargin, topMargin);
#endif
	return;
} /* ResetMargin */

/**
 ** DrawButton()
 **/

static void
#if	OlNeedFunctionPrototypes
DrawButton (
	Widget			w,
	XRectangle *		rect,
	Boolean			depressed
)
#else
DrawButton (w, rect, depressed)
	Widget			w;
	XRectangle *		rect;
	Boolean			depressed;
#endif
{
	Screen *		screen	= XtScreen(w);

	Window			window	= XtWindow(w);

	_OlgDevice *		pDev = TEXTFIELD_P(w).attrs->pDev;

	_OlgDesc *		ul;
	_OlgDesc *		ur;
	_OlgDesc *		ll;
	_OlgDesc *		lr;


	/*
	 * Fetch the four corners of the rectangles surrounding
	 * each arrow.
	 */
	if (OlgIs3d()) {
		ul = &pDev->rect3UL;
		ur = &pDev->rect3UR;
		ll = &pDev->rect3LL;
		lr = &pDev->rect3LR;
	} else {
		ul = &pDev->rect2UL;
		ur = &pDev->rect2UR;
		ll = &pDev->rect2LL;
		lr = &pDev->rect2LR;
	}

	OlgDrawFilledRBox (
		screen, window, TEXTFIELD_P(w).attrs,
		rect->x, rect->y, rect->width, rect->height,
		ul, ur, ll, lr,
		depressed? 0 : FB_UP
	);
	OlgDrawRBox (
		screen, window, TEXTFIELD_P(w).attrs,
		rect->x, rect->y, rect->width, rect->height,
		ul, ur, ll, lr,
		depressed? 0 : RB_UP
	);

	return;
} /* DrawButton */

/**
 ** OverrideDefaultTextEditResources()
 **/

static void
#if	OlNeedFunctionPrototypes
OverrideDefaultTextEditResources (
	Widget			w,
	Dimensions *		d
)
#else
OverrideDefaultTextEditResources (w, d)
	Widget			w;
	Dimensions *		d;
#endif
{
	TEXTEDIT_P(w).topMargin = 0;
	TEXTEDIT_P(w).bottomMargin = d->line_thickness;
	return;
} /* OverrideDefaultTextEditResources */

/**
 ** MotionVerificationCB
 **/

static void 
#if	OlNeedFunctionPrototypes
MotionVerificationCB (
	Widget			w,
	XtPointer		client_data,	/*NOTUSED*/
	XtPointer		call_data
)
#else
MotionVerificationCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	OlTextMotionCallData *	cd = (OlTextMotionCallData *)call_data;

#if	defined(I18N)
# define TF_HORIZONTAL_SHIFT(w) \
    (_OlMax((TEXTEDIT_P(w).charsVisible /3) , 1) * ENSPACE(TE(w)))
#else
# define TF_HORIZONTAL_SHIFT(w) \
    (_OlMax((TEXTEDIT_P(w).charsVisible /3) , 1) * ENSPACE(PRIMITIVE_P(w).font))
#endif

#define SHIFT(w) \
    (TEXTEDIT_P(w).selectMode == 6 || TEXTEDIT_P(w).charsVisible < 10?	\
		FONTWID(TE(w)) : TF_HORIZONTAL_SHIFT(w))

	if (LeftArrowActive(w) || RightArrowActive(w)) {
		Rectangles		r;
		Position		left;
		Position		right;
		Position		x;
		int			delta = 0;
#if	defined(I18N)
		BufferElement *		p;
#else
		String			p;
#endif

		_OlTFComputeRectangles (w, &r);
		left = r.left.x + r.left.width;
		right = r.right.x;

#if	defined(I18N)
		p = wcGetTextBufferLocation(
#else
		p = GetTextBufferLocation(
#endif
			TEXTEDIT_P(w).textBuffer, 0, (TextLocation *)0
		);

		x = _StringWidth(
			(TextEditWidget) w, 0, p, 0, cd->new_cursor - 1,
			PRIMITIVE_P(w).font_list,
			PRIMITIVE_P(w).font, TEXTEDIT_P(w).tabs
		) + TEXTEDIT_P(w).leftMargin + TEXTEDIT_P(w).xOffset;

		if (LeftArrowActive(w) && x <= left)
			delta = _OlMax(TEXTEDIT_P(w).xOffset, -(left - x + SHIFT(TE(w))));
		else if (RightArrowActive(w) && x >= right)
			delta = x - right + SHIFT(TE(w));

		if (delta) {
			TEXTEDIT_P(w).xOffset -= delta;
			TEXTEDIT_P(w).cursorPosition = cd->new_cursor;
			TEXTEDIT_P(w).cursorLocation = LocationOfPosition(
				TEXTEDIT_P(w).textBuffer, cd->new_cursor
			);
			TEXTEDIT_P(w).selectStart = cd->select_start;
			TEXTEDIT_P(w).selectEnd = cd->select_end;
			(void)OlTextEditRedraw (w);
			cd->ok = False;
		}
	}

	return;
} /* MotionVerificationCB */
