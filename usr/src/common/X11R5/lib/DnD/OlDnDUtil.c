#ifndef	NOIDENT
#ident	"@(#)oldnd:OlDnDUtil.c	1.28"
#endif

#include <stdio.h>

#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#ifdef GUI_DEP
#include <Xol/OpenLook.h>
#endif

#include <DnD/OlDnDVCXP.h>

static void	MemMove OL_ARGS((XtPointer, OLconst XtPointer, size_t));

static void
MemMove OLARGLIST((s1, s2, n))
	OLARG( XtPointer,		s1)
	OLARG( OLconst XtPointer,	s2)
	OLGRA( size_t,			n)
{
#if defined(SVR4) || defined(SVR4_0)
	memmove(s1, s2, n);
#else
	size_t				k;
	int				i;

	if (s1 < s2)
		for (i = 0; i < n; i++)
			*s1++ = *s2++;
	else
		for (i = n - 1; i >= 0; i--)
			s1[i] = s2[i];
#endif
} /* end fo MemMove */


/*
 * Part A -
 *
 * The following convenience routines are for application writers
 * who wish to use Desktop Drag-and-drop protocol but don't/can't
 * change the widget code.
 *
 * This set includes two (2) convenience routines:
 *
 *	OlDnDRegisterDDI()
 *	OlDnDRegisterProcessIcon()
 */

	/* This data structure keeps all info passed to		*/
	/* OlDnDRegisterDDI...					*/
typedef struct {
		/* info from caller....				*/
	Widget			originator;
	OlDnDSitePreviewHints	preview_hints;
	OlDnDTMNotifyProc	tm_proc;
	OlDnDPMNotifyProc	pm_proc;
	XtPointer		closure;
	Boolean			on_interest;
		/* private data....				*/
	Boolean			pending_for;
	OlDnDDropSiteID 	drop_site_id;
	Dimension		old_width;
	Dimension		old_height;
} DDIDataRec, *DDIData;

#define REG_DS		OlDnDRegisterWidgetDropSite(		\
				w, ddi_data->preview_hints,	\
				&dsrect, 1, ddi_data->tm_proc,	\
				ddi_data->pm_proc,		\
				ddi_data->on_interest,		\
				ddi_data->closure)
#define DS		ddi_data->drop_site_id
#define PENDING_FOR	ddi_data->pending_for
#define OLD_WIDTH	ddi_data->old_width
#define OLD_HEIGHT	ddi_data->old_height

static void	DSDestroyCB OL_ARGS((Widget, XtPointer, XtPointer));

static Boolean	DifferentDimension OL_ARGS((Widget, Dimension *, Dimension *,
					    OlDnDSiteRect *));

static void	TrackDropSiteEH OL_ARGS((Widget, XtPointer,
					 XEvent *, Boolean *));

static void	TrackParentConfEH OL_ARGS((Widget, XtPointer,
					 XEvent *, Boolean *));

static void
DSDestroyCB OLARGLIST((w, client_data, call_data))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLGRA( XtPointer,	call_data)
{
	XtRemoveEventHandler(w, (EventMask)StructureNotifyMask, False,
				TrackDropSiteEH, client_data);
	if (!XtIsShell(w))
		XtRemoveEventHandler(XtParent(w),
				(EventMask)StructureNotifyMask,
				False, TrackParentConfEH, client_data);
	XtFree(client_data);
} /* end of DSDestroyCB */

/*
 * DifferentDimension - this routine figures out the relationship
 *	between the given "w" and its parent and then determines
 *	the value of (x, y, w, h). These values will be used to
 *	update the DnD geometry. (w, h) will be cached for reducing
 *	the call to OlDnDUpdateGeometry(). If "width" and "height"
 *	didn't change then we don't need to update the DnD geometry
 *	even when (x, y) is changed. i.e., this routine returns
 *	"True" if no changes on (w, h) otherwise it returns "False".
 *
 *	This routine has two assumptions, the first one can be
 *	extended easily by using this algorithm (minor update for
 *	(x, y). The second one is a non-trivial one.
 *
 *		a. assume the whole window is a dropable area;
 *		b. assume no sibling window overlapps the given "w".
 *
 * Note that "c" = child window and "p" = parent window.
 *
 *  A-1
 *    x = 0
 *    y = 0
 *    w = 0
 *    h = 0
 *  c----c p----p
 *  |    | |    |
 *  |    | |    |
 *  c----c p----p
 *
 *
 *   B-1            B-2             C-1             C-2
 *     x = 0          x = -c.x        x = 0           x = -c.x
 *     y = 0          y = -c.y        y = -c.y        y = 0
 *     w = c.w        w = p.w         w = c.w         w = p.w
 *     h = c.h        h = p.h         h = p.h         h = c.h
 *   p--------p     c--------c         c---c           p---p
 *   | c----c |     | p----p |         |   |           |   |
 *   | |xxxx| |     | |xxxx| |      p__.....__p     c---------c
 *   | |xxxx| |     | |xxxx| |      |  |xxx|  |     |  .xxx.  |
 *   | |xxxx| |     | |xxxx| |      p__.....__p     c---------c
 *   | c----c |     | p----p |         |   |           |   |
 *   p--------p     c--------c         c---c           p---p
 *
 *
 *   D-1            D-2             E-1             E-2
 *     x = 0          x = -c.x        x = 0           x = -c.x
 *     y = 0          y = -c.y        y = -c.y        y = 0
 *     w = px+pw-cx   w = cx+cw-px    w = px+pw-cx    w = cx+cw-px
 *     h = py+ph-cy   h = cy+ch-py    h = cy+ch-py    h = py+ph-cy
 *   p----p         c----c            c----c          p----p
 *   |    |         |    |            |    |          |    |
 *   |  c----c      |  p.|--p       p-|..p |        c----c |
 *   |  |x.  |      |  .x|  |       | |xx. |        | .xx| |
 *   p--|.p  |      c----c  |       | c----c        | p..|-p
 *      |    |         |    |       |    |          |    |
 *      c----c         p----p       p----p          c----c
 *
 *
 *   F-1            F-2             G-1             G-2
 *     x = 0          x = -c.x        x = -c.x        x = 0
 *     y = 0          y = -c.y        y = 0           y = -c.y
 *     w = px+pw-cx   w = cx+cw-px    w = p.w         w = c.w
 *     h = c.h        h = p.h         h = py+ph-cy    h = cy+ch-py
 *   p----p         c----c             p----p          c----c
 *   |    |         |    |             |    |          |    |
 *   |  c----c      |  p.|--p       c----------c    p__|....|__p
 *   |  |x.  |      |  .x|  |       |  .xxxx.  |    |  |xxxx|  |
 *   |  c----c      |  p.|--p       |  p----p  |    |  c----c  |
 *   |    |         |    |          |          |    |          |
 *   p----p         c----c          c----------c    p----------p
 *
 *
 *   H-1            H-2             I-1             I-2
 *     x = -c.x       x = 0           x = -c.x        x = 0
 *     y = 0          y = -c.y        y = -c.y        y = 0
 *     w = cx+cw-px   w = px+pw-cx    w = p.w         w = c.w
 *     h = c.h        h = p.h         h = cy+ch-py    h = py+ph-cy
 *      p----p         c----c       c----------c    p----------p
 *      |    |         |    |       |          |    |          |
 *   c-----c |      p__|..p |       |  p....p  |    |  c----c  |
 *   |  .xx| |      |  |xx. |       |  .xxxx.  |    |  |xxxx|  |
 *   c-----c |      p__|..p |       c----------c    p__|....|__p
 *      |    |         |    |          |    |          |    |
 *      p----p         c----c          p----p          c----c
 *
 */
static Boolean
DifferentDimension OLARGLIST((w, old_width, old_height, rect))
	OLARG( Widget,		w)
	OLARG( Dimension *,	old_width)
	OLARG( Dimension *,	old_height)
	OLGRA( OlDnDSiteRect *,	rect)
{
	Position		cx1, cx2, cy1, cy2;
	Position		px1, px2, py1, py2;
	Widget			parent = XtParent(w);

	if (parent == (Widget)NULL) /* MUST be a SHELL widget...	*/
		parent = w;

		/* translate child's coords to parent's coords...	*/
	cx1 = w->core.x + parent->core.x;
	cy1 = w->core.y + parent->core.y;

	px1 = parent->core.x;
	py1 = parent->core.y;

	cx2 = cx1 + w->core.width;
	cy2 = cy1 + w->core.height;

	px2 = px1 + parent->core.width;
	py2 = py1 + parent->core.height;


#define LmH(L,m,H)	(L <= m && m <= H)
#define cxLmH(tx)	LmH(px1,tx,px2)
#define cyLmH(ty)	LmH(py1,ty,py2)
#define pxLmH(tx)	LmH(cx1,tx,cx2)
#define pyLmH(ty)	LmH(cy1,ty,cy2)
#define CX		w->core.x
#define CY		w->core.y
#define CW		w->core.width
#define CH		w->core.height
#define PW		parent->core.width
#define PH		parent->core.height
#define XYWH(X,Y,W,H)	rect->x		= (Position)((X));	\
			rect->y		= (Position)((Y));	\
			rect->width	= (Dimension)((W));	\
			rect->height	= (Dimension)((H))

	if (cxLmH(cx1) && cyLmH(cy1))
	{
		if (cxLmH(cx2) && cyLmH(cy2))
		{
			XYWH(0, 0, CW, CH);			/* B-1 */
		}
		else if (cxLmH(cx1) && cyLmH(cy2))
		{
			XYWH(0, 0, px2 - cx1, CH);		/* F-1 */
		}
		else if (cxLmH(cx2) && cyLmH(cy1))
		{
			XYWH(0, 0, CW, py2 - cy1);		/* I-2 */
		}
		else
		{
			XYWH(0, 0, px2 - cx1, py2 - cy1);	/* D-1 */
		}
	}
	else
	{
		if (cxLmH(cx2) && cyLmH(cy2))
		{
			if (cxLmH(cx2) && cyLmH(cy1))
			{
				XYWH(-CX, 0, cx2 - px1, CH);	/* H-1 */
			}
			else if (cxLmH(cx1) && cyLmH(cy2))
			{
				XYWH(0, -CY, CW, cy2 - py1);	/* G-2 */
			}
			else
			{
				XYWH(-CX, -CY, cx2 - px1, cy2 - py1); /* D-2 */
			}
		}
		else
		{
			if (cxLmH(cx2) && cyLmH(cy1))
			{
				XYWH(-CX, 0, cx2 - px1, py2 - cy1);   /* E-2 */
			}
			else if (cxLmH(cx1) && cyLmH(cy2))
			{
				XYWH(0, -CY, px2 - cx1, cy2 - py1);   /* E-1 */
			}
			else
			{
				if (pxLmH(px1) && pyLmH(py1))
				{
					if (pxLmH(px2) && pyLmH(py2))
					{
				/* B-2 */	XYWH(-CX, -CY, PW, PH);
					}
					else if (pxLmH(px1) && pyLmH(py2))
					{
				/* F-2 */	XYWH(-CX, -CY, cx2 - px1, PH);
					}
					else
					{
				/* I-1 */	XYWH(-CX, -CY, PW, cy2-py1);
					}
				}
				else
				{
					if (pxLmH(px2) && pyLmH(py2))
					{
						if (pxLmH(px1) && pyLmH(py2))
						{
				/* G-1 */		XYWH(-CX,0,CW,cy2-py1);
						}
						else
						{
				/* H-2 */		XYWH(0,-CY,px2-cx1,PH);
						}
					}
					else
					{
						if (cxLmH(cx1) && cxLmH(cx2))
						{
				/* C-1 */		XYWH(0, -CY, CW, PH);
						}
						else if (pxLmH(px1) &&
							 pxLmH(px2))
						{
				/* C-2 */		XYWH(-CX, 0, PW, CH);
						}
						else
						{
				/* A-1 */		XYWH(0, 0, 0, 0);
						}
					}
				}
			}
		}
	}


	if (*old_width == rect->width && *old_height == rect->height)
		return(False);
	else
	{
		*old_width  = rect->width;
		*old_height = rect->height;
		return(True);
	}

#undef LmH
#undef cxLmH
#undef cyLmH
#undef pxLmH
#undef pyLmH
#undef CX
#undef CY
#undef CW
#undef CH
#undef PW
#undef PH
#undef XYWH
} /* end of DifferentDimension */

/*
 * TrackDropSiteEH -
 *
 *	This event handler is registered when using
 *	OlDnDRegisterDDI(). The purpose of this routine is
 *	to track any sizing/mapping changes.
 *
 */
static void
TrackDropSiteEH OLARGLIST((w, client_data, xevent, cont_to_dispatch))
	OLARG( Widget,		w)
	OLARG( XtPointer,	client_data)
	OLARG( XEvent *,	xevent)
	OLGRA( Boolean *,	cont_to_dispatch)
{
	Boolean			dim_changed;
	DDIData			ddi_data;
	OlDnDSiteRect		dsrect;

	ddi_data = (DDIData)client_data;
	if (PENDING_FOR == True)
		return;

	switch(xevent->type)
	{
			/* too bad, Intrinsic won't give me CreateNotify*/
			/* If you need an invisible window then you have*/
			/* to realize widget to have OlDnDRegisterDDI	*/
			/* work...					*/
		case MapNotify:		/* FALL THROUGH			*/
		case ConfigureNotify:
			dim_changed = DifferentDimension(
					w, &OLD_WIDTH, &OLD_HEIGHT, &dsrect);

			PENDING_FOR = True;
			if (DS == 0)
			{
				DS = REG_DS;
			}
			else
			{
				if (dim_changed)
					OlDnDUpdateDropSiteGeometry(
						DS, &dsrect, 1);
			}
			PENDING_FOR = False;
			break;
		default:
			break;
	}
} /* end of TrackDropSiteEH */

/*
 * TrackParentConfEH -
 *
 * This routine is used to trap parent's geometry changes. A drop site
 * geometry needs to be updated if changes are made for parent's geometry.
 *
 * Note that parent and grand-parent of this drop site should register
 * this EH. This is a buttom-up method and we need this because our
 * geometry policy is GeometryYes (so either put burdens on OlDnDRegisgerDDI
 * or on all composites.
 *
 * Note that it turned out DifferentDimension can only handle relationship
 * between a child and it's immediate parent, we need to make that routine
 * more generic so it won't rely on its immediate parent. Currently, I
 * only attach this EH to its immediate parent. This works fine if an
 * application writers don't do "odd" things. e.g., the following
 * widget tree will fail on "animation":
 *	Caption
 *		ControlArea
 *			TextField (this is a manager and has an atuo child,
 *					textedit, that is interested in DnD)
 */
static void
TrackParentConfEH OLARGLIST((w, client_data, xevent, cont_to_dispatch))
	OLARG( Widget,		w)		/* parent id	*/
	OLARG( XtPointer,	client_data)
	OLARG( XEvent *,	xevent)
	OLGRA( Boolean *,	cont_to_dispatch)
{
	DDIData			ddi_data;
	OlDnDSiteRect		dsrect;

	ddi_data = (DDIData)client_data;
	if (PENDING_FOR == True || DS == 0)
		return;

	switch(xevent->type)
	{
		case ConfigureNotify:
			(void)DifferentDimension(
					ddi_data->originator, &OLD_WIDTH,
					&OLD_HEIGHT, &dsrect
			);

			PENDING_FOR = True;
			OlDnDUpdateDropSiteGeometry(DS, &dsrect, 1);
			PENDING_FOR = False;
			break;
	}
} /* end of TrackParentConfEH */

/*
 * OlDnDRegisterDDI -
 *
 * This routine is used to register the Desktop DnD from an application.
 * This routine assumes that the whole window can take the drop, i.e.,
 * "site rectangle" is the whole window.
 *
 * See OlDnDRegisterWidgetDropSite for more details.
 *
 */
extern void
OlDnDRegisterDDI OLARGLIST((w, preview_hints, tm_proc, pm_proc, on_interest, closure))
	OLARG( Widget,			w)
	OLARG( OlDnDSitePreviewHints,	preview_hints)
	OLARG( OlDnDTMNotifyProc,	tm_proc)
	OLARG( OlDnDPMNotifyProc,	pm_proc)
	OLARG( Boolean,			on_interest)
	OLGRA( XtPointer,		closure)
{
	DDIData				ddi_data;

	ddi_data = (DDIData)XtMalloc(sizeof(DDIDataRec));

	ddi_data->originator	= w;
	ddi_data->preview_hints	= preview_hints;
	ddi_data->tm_proc	= tm_proc;
	ddi_data->pm_proc	= pm_proc;
	ddi_data->on_interest	= on_interest;
	ddi_data->closure	= closure;
	ddi_data->pending_for	= False;
	ddi_data->old_width	= 0;
	ddi_data->old_height	= 0;

	if (!XtIsRealized(w))
	{
		ddi_data->drop_site_id = 0;
	}
	else
	{
		OlDnDSiteRect	dsrect;

		(void)DifferentDimension(
			w, &OLD_WIDTH, &OLD_HEIGHT, &dsrect);

		PENDING_FOR = True;
		ddi_data->drop_site_id	= REG_DS;
		PENDING_FOR = False;
	}

	XtAddEventHandler(w, (EventMask)StructureNotifyMask, False,
				TrackDropSiteEH, (XtPointer)ddi_data);

		/* See TrackParentConfEH for comments...	*/
	if (!XtIsShell(w))
		XtAddEventHandler(XtParent(w), (EventMask)StructureNotifyMask,
				False, TrackParentConfEH, (XtPointer)ddi_data);

	XtAddCallback(
		w, XtNdestroyCallback, DSDestroyCB, (XtPointer)ddi_data);

} /* end of OlDnDRegisterDDI */

#undef REG_DS
#undef DS
#undef PENDING_FOR
#undef OLD_WIDTH
#undef OLD_HEIGHT


/*
 * Part B -
 *
 * The following convenience routines are for application writers
 * who wish to use Desktop Drag-and-drop protocol in other Xt-based
 * toolkits, e.g., Athena widget library (libXaw) or
 *		   Motif widget library (libXm).
 *
 * The idea is that DnD is still implemeneted as a vendor class extension
 * record and you have to call OlDnDVCXInitialize() to attach this record
 * to the vendor class record. This routine has to be called before
 * calling any Xt functions. For MooLIT, this call will be made in
 * Vendor:_OlLoadVendorShell(). For other toolkits, this call should be
 * made in the beginning of main().
 *
 * This set includes one (1) public convenience routine.
 *
 *	OlDnDVCXInitialize()
 *
 * All GUI independent changes are #ifdef'd with "GUI_DEP".
 *
 */

#ifndef GUI_DEP

#define NULL_WIDGET	((Widget)NULL)

#define VWC		((VendorShellClassRec *)vendorShellWidgetClass)
#define VendorClass	VWC->vendor_shell_class
#define VCoreClass	VWC->core_class


static XtProc			orig_vendor_class_initialize;

static XtWidgetClassProc	orig_vendor_class_part_initialize;

static XtWidgetProc		orig_vendor_destroy;

static XtRealizeProc		orig_vendor_realize;

static XtInitProc		orig_vendor_initialize;

static XtArgsProc		orig_vendor_get_values_hook;

static XtSetValuesFunc		orig_vendor_set_values;


static void	DnDClassInitialize OL_NO_ARGS();

static void	DnDClassPartInitialize OL_ARGS((WidgetClass));

static void	DnDDestroy OL_ARGS((Widget));

static void	DnDGetValuesHook OL_ARGS((Widget, ArgList, Cardinal *));

static void	DnDInitialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));

static void	DnDRealize OL_ARGS((Widget, XtValueMask *,
				    XSetWindowAttributes *));

static Boolean	DnDSetValues OL_ARGS((Widget, Widget, Widget,
					ArgList, Cardinal *));

static void	SaveOrigAndUseNewClassMethods OL_NO_ARGS();

/*
 * OlDnDVCXInitialize -
 *
 * This function attaches the DnD vendor class extension record to
 * vendorShellWidgetClass. One important assumption is that this dnd
 * software can only work with a Xt Intrinsic based toolkit. This
 * function won't work without such assumption...
 *
 * DnD software is implemented as a vendor class extension, thus the
 * software is tied with a toolkit (i.e., Olit and/or MooLIT). The
 * requirement states that this software should work with other Xt
 * Intrinsics based toolkits (e.g., libXaw * and libXm). One way to
 * achieve this requirement is still implemented as a vendor extension
 * and enabling this thru a function call...
 *
 */
extern void
OlDnDVCXInitialize OL_NO_ARGS()
{
	static Boolean		flag = False;

	XtPointer		last_extension,
				next_extension;

	if (flag == True)
	{
		return;
	}
		/* this routine can only be used before a vendor shell	*/
		/* is created...					*/
	if (VCoreClass.class_inited)
	{
		XtError("You have to call OlDnDVCXInitialize() in the\
 beginning of the program");
	}

	flag = True;

	last_extension =
	next_extension = VendorClass.extension;

		/* no vendor extension in this toolkit...		*/
	if (last_extension == (XtPointer)NULL)
	{
		VendorClass.extension = (XtPointer)&dnd_vendor_extension_rec;
	}
	else	/* have some...						*/
	{
		OlDnDClassExtensionHdrPtr	class_extension;

			/* assume DnD extension is not in yet so loop	*/
			/* through the chain and get the last one...	*/
		while (next_extension != NULL)
		{
			class_extension = (OlDnDClassExtensionHdrPtr)
							next_extension;

			last_extension = next_extension;
			next_extension = class_extension->next_extension;
		}

			/* make dnd as the last vendor extension...	*/
		class_extension = (OlDnDClassExtensionHdrPtr)last_extension;
		class_extension->next_extension =
				(XtPointer)&dnd_vendor_extension_rec;
	}

	SaveOrigAndUseNewClassMethods();

} /* end of OlDnDVCXInitialize */

static void
DnDClassInitialize OL_NO_ARGS()
{
	if (orig_vendor_class_initialize != (XtProc)NULL)
	{
		(*orig_vendor_class_initialize)();
	}

	_OlDnDDoExtensionClassInit(&dnd_vendor_extension_rec);
} /* end of DnDClassInitialize */

static void
DnDClassPartInitialize OLARGLIST((wc))
	OLGRA( WidgetClass,	wc)
{
	if (orig_vendor_class_part_initialize != (XtWidgetClassProc)NULL)
	{
		(*orig_vendor_class_part_initialize)(wc);
	}

	_OlDnDDoExtensionClassPartInit(wc);
} /* end of DnDClassPartInitialize */

static void
DnDDestroy OLARGLIST((w))
	OLGRA( Widget,		w)
{
	if (orig_vendor_destroy != (XtWidgetProc)NULL)
	{
		(*orig_vendor_destroy)(w);
	}

	(void)CallDnDVCXExtensionMethods(
			CallDnDVCXDestroy, XtClass(w),
			w, NULL_WIDGET, NULL_WIDGET,
			(ArgList)NULL, (Cardinal *)NULL
	);
} /* end of DnDDestroy */

static void
DnDGetValuesHook OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	if (orig_vendor_get_values_hook != (XtArgsProc)NULL)
	{
		(*orig_vendor_get_values_hook)(w, args, num_args);
	}

	(void)CallDnDVCXExtensionMethods(
			CallDnDVCXGetValues, XtClass(w),
			w, NULL_WIDGET, NULL_WIDGET,
			args, num_args
	);
} /* end of DnDGetValuesHook */

static void
DnDInitialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	Display *	dpy = XtDisplayOfObject(new);
	static Boolean	flag = False;

		/* wait for Sun code...		*/
	if (flag == False)
	{
		OlDnDInitialize(dpy);
		flag = True;
	}

	if (orig_vendor_initialize != (XtInitProc)NULL)
	{
		(*orig_vendor_initialize)(request, new, args, num_args);
	}

	(void)CallDnDVCXExtensionMethods(
			CallDnDVCXInitialize, XtClass(new),
			NULL_WIDGET, request, new,
			args, num_args
	);
} /* end of DnDInitialize */

static void
DnDRealize OLARGLIST((w, value_mask, attributes))
	OLARG( Widget,			w)
	OLARG( XtValueMask *,		value_mask)
	OLGRA( XSetWindowAttributes *,	attributes)
{
	if (orig_vendor_realize != (XtRealizeProc)NULL)
	{
		(*orig_vendor_realize)(w, value_mask, attributes);
	}

	_OlDnDCallVCXPostRealizeSetup(w);
} /* end of DnDRealize */

static Boolean
DnDSetValues OLARGLIST((current, request, new, args, num_args))
	OLARG( Widget,		current)
	OLARG( Widget,		request)
	OLARG( Widget,		new)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	Boolean			flag = False;

	if (orig_vendor_set_values != (XtSetValuesFunc)NULL)
	{
		flag = (*orig_vendor_set_values)(
					current, request, new, args, num_args);
	}

	flag |= CallDnDVCXExtensionMethods(
			CallDnDVCXSetValues, XtClass(new),
			current, request, new,
			args, num_args
		);

	return(flag);
} /* end of DnDSetValues */

static void
SaveOrigAndUseNewClassMethods OL_NO_ARGS()
{
		/* save original class methods...			*/
	orig_vendor_class_initialize	 = VCoreClass.class_initialize;
	orig_vendor_class_part_initialize= VCoreClass.class_part_initialize;
	orig_vendor_destroy		 = VCoreClass.destroy;
	orig_vendor_get_values_hook	 = VCoreClass.get_values_hook;
	orig_vendor_initialize		 = VCoreClass.initialize;

		/* This method can be inherited from its superclass so	*/
		/* we have to resolve it beforehand...			*/
	if (VCoreClass.realize != XtInheritRealize)
		orig_vendor_realize	 = VCoreClass.realize;
	else
	{
		WidgetClass		superclass;

		superclass = VCoreClass.superclass;

#define SUPER_CORE	superclass->core_class

		while (SUPER_CORE.realize == XtInheritRealize)
			superclass = SUPER_CORE.superclass;

		orig_vendor_realize	 = SUPER_CORE.realize;

#undef SUPER_CORE
	}

	orig_vendor_set_values		 = VCoreClass.set_values;

		/* and then use new class methods...			*/
	VCoreClass.class_initialize	 = DnDClassInitialize;
	VCoreClass.class_part_initialize = DnDClassPartInitialize;
	VCoreClass.destroy		 = DnDDestroy;
	VCoreClass.get_values_hook	 = DnDGetValuesHook;
	VCoreClass.initialize		 = DnDInitialize;
	VCoreClass.realize		 = DnDRealize;
	VCoreClass.set_values		 = DnDSetValues;

} /* end of SaveOrigAndUseNewClassMethods */

#undef VCoreClass
#undef VendorClass
#undef VWC
#undef NULL_WIDGET

#endif	/* GUI_DEP */


/*
 * Part C -
 *
 * The following convenience routines are for application/widget
 * writers using Desktop Drag-and-drop protocol. These routines
 * are used to initiate, to monitor/track, and to terminate the
 * Drag-and-drop gesture (i.e., through a mouse pointer and/or
 * a keyboard).
 *
 * By default, such gesture can only be operated on a mouse pointer.
 * To enable the "mouseless" drag-and-drop, application/widget
 * writers can register a "OlDnDDragKeyProc" procedure.
 * For MooLIT toolkit, this will be done in Vendor.c:_OlLoadVendorShell().
 * For other tookits, this can be done before starting a drag-and-drop
 * operation. Note that you only need to do registration once.
 *
 * This set includes five (5) convenience routines:
 *
 *	OlDnDDragAndDrop()
 *	OlDnDGrabDragCursor()
 *	OlDnDRegisterDragKeyProc()
 *	OlDnDTrackDragCursor()
 *	OlDnDUngrabDragCursor()
 */

#define GRAB_PTR_MASK		ButtonReleaseMask | PointerMotionMask
#define DISPLAY_CURSOR(d,w,c)	XGrabPointer(d,w,False,GRAB_PTR_MASK,	   \
					GrabModeAsync, GrabModeAsync, None,\
					c, CurrentTime)

typedef struct {
	Boolean			done;
	Boolean			retval;
	XEvent *		grabevent;
	OlDnDPreviewAnimateCbP	proc;
	XtPointer		closure;
	Boolean			status;
	Boolean			show_initial_cursor;
} _DND_PD;

typedef struct {
	OlDnDAnimateCursorsPtr	user_data;
	Boolean			reset_flag;
} AnimateCBDataRec, *AnimateCBData;

static OlDnDDragKeyProc		handle_drag_key_proc = (OlDnDDragKeyProc)NULL;


static void			AnimateCB OL_ARGS((
					Widget, int, Time, Boolean, XtPointer));

static void			DisplayInitialCursor OL_ARGS((
					Widget, XtPointer, Boolean *));

static void			_poll_for_drag_event OL_ARGS((
					Widget, XtPointer,
					XEvent *, Boolean *));

static Boolean			DnDDragAndDrop OL_ARGS((
					Widget, Window *, Position *,
					Position *, OlDnDDragDropInfoPtr,
					OlDnDPreviewAnimateCbP, XtPointer,
					Boolean *));

/*
 * AnimateCB -
 *
 *	This is the default animate callback that is provided by
 *	the toolkit. This routine assumes that "client_data" is
 *	the AnimateCBData type and resets the static value if
 *	"reset_flag" is True.
 *
 *	This routine will be invoked when receiving
 *	either EnterNotify or LeaveNotify (I don't
 *	think we care about MotionNotify). "client_data" contains
 *	the info of the YES/NO cursors.
 *
 *	Note that to reduce the request from X server, a static value
 *		  is used in the routine.
 *
 *	Also Note that the beta software has bugs on invoking this
 *			callback. It relied on the existence of the
 *			preview message notify callback in the
 *			destination. The fix is to move that part code
 *			out of "if (deliver_preview)" and also check
 *			the eventcode value before invoking this callback.
 */
static void
AnimateCB OLARGLIST((w, event_type, timestamp, sensitivity, client_data))
	OLARG( Widget,			w)
	OLARG( int,			event_type) /* Enter/Leave	*/
	OLARG( Time,			timestamp)
	OLARG( Boolean,			sensitivity)/* not used in this sw */
	OLGRA( XtPointer,		client_data)/* cursor info	*/
{
#define INIT_VAL	0	/* neither EnterNotify nor LeaveNotify	*/
#define VALUE_INITED	(last_event_type == INIT_VAL ? False : True)
#define SAME_TYPE	(event_type == last_event_type ? True : False)

	AnimateCBData	cursor = (AnimateCBData)client_data;
	static int	last_event_type = INIT_VAL;

	if (cursor->reset_flag == True)	/* reset last_event_type	*/
	{
		last_event_type = INIT_VAL;
		cursor->reset_flag = False;
	}

	if (VALUE_INITED && SAME_TYPE)
	{
		return;
	}

	last_event_type	= event_type;

#define YES_CURSOR	(cursor->user_data)->yes_cursor
#define NO_CURSOR	(cursor->user_data)->no_cursor

	DISPLAY_CURSOR(
		XtDisplayOfObject(w),
		XtWindowOfObject(w),
		event_type == EnterNotify ? YES_CURSOR : NO_CURSOR
	);

#undef NO_CURSOR
#undef YES_CURSOR
#undef INIT_VAL
#undef VALUE_INITED
#undef SAME_TYPE
} /* end of AnimateCB */

static void
DisplayInitialCursor OLARGLIST((w, closure, show_initial_cursor))
	OLARG( Widget,			w)
	OLARG( XtPointer,		closure)
	OLGRA( Boolean *,		show_initial_cursor)
{
#define YES_CURSOR	(stuff->user_data)->yes_cursor
#define NO_CURSOR	(stuff->user_data)->no_cursor

	AnimateCBData		stuff = (AnimateCBData)closure;
	Arg			args[2];
	Boolean			flag,
				pending = True,
				has_dsdm = False;
	Cursor			cursor;
	Display *		dpy;
	Window			win;
	Widget			shell;
	Window			root,
				ignored_win;
	int			root_x,
				root_y,
				ignored_xy;
	unsigned int		mask;

	dpy = XtDisplayOfObject(w);
	win = XtWindowOfObject(w);

	*show_initial_cursor = False;

	shell = w;
	while(shell != (Widget)NULL && !XtIsShell(shell))
		shell = shell->core.parent;

	XtSetArg(args[0], XtNpendingDSDMInfo, (XtArgVal)&pending);
	XtSetArg(args[1], XtNdsdmPresent, (XtArgVal)&has_dsdm);
	XtGetValues(shell, args, 2);

	if (pending == True || has_dsdm == False)
	{
			/* we already show drag cursor, so do	*/
			/* nothing but return...		*/
		return;
	}
	else
	{
		XQueryPointer(
			dpy, win,
			&root, &ignored_win, &root_x, &root_y,
			&ignored_xy, &ignored_xy, &mask
		);

			/* where am I ? On a drop site or not... */
		flag = OlDnDPreviewAndAnimate(
				w, root, root_x, root_y,
				XtLastTimestampProcessed(dpy),
				(OlDnDPreviewAnimateCbP)NULL,
				(XtPointer)NULL
		);
	}

	cursor = flag == True ? YES_CURSOR : NO_CURSOR;

		/* show animate cursor				*/
	DISPLAY_CURSOR(dpy, win, cursor);

#undef YES_CURSOR
#undef NO_CURSOR
} /* end of DisplayInitialCursor */

/*
 *  _poll_for_drag_event -
 */
static void
_poll_for_drag_event OLARGLIST((w, closure, ev, decide_to_dispatch))
	OLARG(Widget,		w)
	OLARG(XtPointer,	closure)
	OLARG(XEvent *,		ev)
	OLGRA(Boolean *,	decide_to_dispatch)
{
	Window			root;
	int			x_root,
				y_root;
	Time			time_stamp;

	_DND_PD	*pd = (_DND_PD *)closure;

	if (pd->done) return;

	switch (ev->type) {
		case ButtonRelease:
			pd->done = (Boolean)True;
			*decide_to_dispatch = False;
			*pd->grabevent = *ev;

			root		= ev->xbutton.root;
			x_root		= ev->xbutton.x_root;
			y_root		= ev->xbutton.y_root;
			time_stamp	= ev->xbutton.time;
			break;

		case KeyPress:
			*decide_to_dispatch = False;

			if (handle_drag_key_proc == (OlDnDDragKeyProc)NULL)
			{
				return;
			}
			switch((*handle_drag_key_proc)(w, ev))
			{
				case OlDnDCanceled:
					pd->retval = False;
					pd->done = True;
					break;
				case OlDnDDropped:
					pd->done = True;
					break;
				case OlDnDStillDragging:
					break;
			}
			*pd->grabevent = *ev;

			root		= ev->xkey.root;
			x_root		= ev->xkey.x_root;
			y_root		= ev->xkey.y_root;
			time_stamp	= ev->xkey.time;
			break;

		case MotionNotify:
			root		= ev->xmotion.root;
			x_root		= ev->xmotion.x_root;
			y_root		= ev->xmotion.y_root;
			time_stamp	= ev->xmotion.time;

			*decide_to_dispatch = False;
			break;

	}

	pd->status = OlDnDPreviewAndAnimate(
			w, root, x_root, y_root, time_stamp,
			pd->proc, pd->closure
	);

		/* we found a valid drop site when "status" is	*/
		/* true so turn "show_initial_cursor" off...	*/
	if (pd->show_initial_cursor == True && pd->status == True)
	{
		pd->show_initial_cursor = False;
	}
} /* end of _poll_for_drag_event */

/*
 * DnDDragAndDrop -
 */
static Boolean
DnDDragAndDrop OLARGLIST((w, dst_win, dst_x, dst_y, rootinfo, animate, closure, status))
	OLARG( Widget,			w)
	OLARG( Window *,		dst_win)
	OLARG( Position *,		dst_x)
	OLARG( Position *,		dst_y)
	OLARG( OlDnDDragDropInfoPtr,	rootinfo)
	OLARG( OlDnDPreviewAnimateCbP,	animate)
	OLARG( XtPointer,		closure)
	OLGRA( Boolean *,		status)
{
	Display *			dpy;
	XEvent				grabevent;
	Window				parent;
	Window				child;
	Window				temp;
	Window				grab_window;
	int				x;
	int				y;
	Time				timestamp;
	_DND_PD				pd;

	dpy		= XtDisplayOfObject(w);
	grab_window	= XtWindowOfObject(w);

	pd.done		= False;
	pd.retval	= True;
	pd.grabevent	= &grabevent;
	pd.proc		= animate;
	pd.closure	= closure;
	pd.status	= False;

		/*
		 * if the caller is not OlDnDTrackDragCursor or
		 * OlDnDTrackDragCursor doesn't have "cursors"
		 * data, then the "drag-cursor" is already displayed
		 * by the caller of OlDnDTrackDragCursor, otherwise
		 * we set "show_initial_cursor" to True to make sure
		 * the right "cursor" is displayed during the DnD
		 * operation.
		 */
	if (status != (Boolean *)NULL && closure != (XtPointer)NULL)
		pd.show_initial_cursor	= True;
	else
		pd.show_initial_cursor	= False;

#define EM      (ButtonReleaseMask | KeyPressMask | PointerMotionMask)

	XtInsertRawEventHandler(w, EM, False, _poll_for_drag_event,
				(XtPointer)&pd, XtListHead);

		/*
		 * The routine below will fetch dsdm database and
		 * have an internal loop waiting for either data
		 * or timeout. Thus it is necessary to have a
		 * "show_initial_cursor" flag to make sure we display
		 * the right cursor after the loop. Note that
		 * _poll_for_drag_event will turn off this flag once
		 * a valid drop site is found...
		 */
	OlDnDInitializeDragState(w);

	if (pd.show_initial_cursor == True)
	{
		DisplayInitialCursor(w, closure, &pd.show_initial_cursor);
	}

	while (!pd.done) {
		XWindowEvent(dpy, grab_window, EM, &grabevent);
		XtDispatchEvent(&grabevent);
	}

	XtRemoveRawEventHandler(
		w, EM, False, _poll_for_drag_event, (XtPointer)&pd);

#undef EM

	OlDnDClearDragState(w);

		/* no need to continue if the operation is aborted...	*/
	if (pd.retval == False)
	{
		return(pd.retval);
	}

	if (status != (Boolean *)NULL)
	{
		*status = pd.status;
	}

	switch (grabevent.type) {
		case ButtonRelease:
			parent = child = grabevent.xbutton.root;
			x = grabevent.xbutton.x_root;
			y = grabevent.xbutton.y_root;
			timestamp = grabevent.xbutton.time;
			break;

		case KeyPress:
			parent = child = grabevent.xkey.root;
			x = grabevent.xkey.x_root;
			y = grabevent.xkey.y_root;
			timestamp = grabevent.xkey.time;
			break;
	}

	if (rootinfo != (OlDnDDragDropInfoPtr)NULL) {
		rootinfo->root_x = x;
		rootinfo->root_y = y;
		rootinfo->root_window = parent;
		rootinfo->drop_timestamp = timestamp;
	}

	while (child != 0)
	{
		temp = parent;
		parent = child;
		XTranslateCoordinates(
			dpy, temp, parent, x, y, &x, &y, &child);
	}

	*dst_win = parent;
	*dst_x = (Position)x;
	*dst_y = (Position)y;

	return(pd.retval);
} /* end of DnDDragAndDrop */

/*
 *  OlDnDDragAndDrop -
 */
extern Boolean
OlDnDDragAndDrop OLARGLIST((w, window, xPosition, yPosition, rootinfo, animate, closure))
	OLARG( Widget,			w)
	OLARG( Window *,		window)
	OLARG( Position *,		xPosition)
	OLARG( Position *,		yPosition)
	OLARG( OlDnDDragDropInfoPtr,	rootinfo)
	OLARG( OlDnDPreviewAnimateCbP,	animate)
	OLGRA( XtPointer,		closure)
{
	return(DnDDragAndDrop(
			w, window, xPosition, yPosition,
			rootinfo, animate, closure, (Boolean *)NULL)
	);
} /* for SUN API */

/*
 * OlDnDGrabDragCursor
 *
 * The OlDnDGrabDragCursor procedure is used to effect an active
 * grab of the mouse pointer and the keyboard.  This function is normally
 * called after a mouse drag operation is experienced and prior to calling
 * the OlDragAndDrop/OlDnDDragAndDrop procedure which is used to monitor
 * a drag operation.
 *
 */
extern void
OlDnDGrabDragCursor OLARGLIST((w, cursor, window))
	OLARG( Widget,	w)
	OLARG( Cursor,	cursor)
	OLGRA( Window,	window)
{
	Display *	dpy = XtDisplayOfObject(w);
	Window		win = XtWindowOfObject(w);

	XGrabPointer(
		dpy, win, False,
		GRAB_PTR_MASK, GrabModeAsync,
		GrabModeAsync, window, cursor, CurrentTime
	);

		/* doit only when necessary...			*/
	if (handle_drag_key_proc != (OlDnDDragKeyProc)NULL)
	{
		XGrabKeyboard(
			dpy, win, False,
			GrabModeAsync, GrabModeAsync, CurrentTime
		);
	}
} /* end of OlDnDGrabDragCursor */

/*
 * OlDnDRegisterDragKeyProc -
 *
 * This routine is used register a OlDnDDragKeyProc procedure.
 * This OlDnDDragKeyProc procedure will be called in the DnD operation when
 * a KeyPress event is generated.
 *
 */
extern void
OlDnDRegisterDragKeyProc OLARGLIST((the_proc))
	OLGRA( OlDnDDragKeyProc,	the_proc)
{
	handle_drag_key_proc = the_proc;
} /* end of OlDnDRegisterDragKeyProc */

/*
 * OlDnDTrackDragCursor -
 *
 * This function is used to track the drag cursor from a pointer device
 * and a keyboard device in the drag-and-drop operation. This routine
 * will first grab the mouse pointer and the keyboard and will relinquish
 * the active pointer and keyboard grabs at end. This routine will provide
 * the DnD user feedback in the DnD operation if "cursors" is not NULL.
 *
 * The routine returns one of three values:
 *
 *	OlDnDDropSucceeded - the drop is succeeded because the destination
 *			     client has registerred the Desktop DnD.
 *	OlDnDDropFailed    - otherwise.
 *	OlDnDCanceled	   - the operation is aborted. e.g., this means
 *			     cancelKey is pressed in MooLIT/OLIT.
 *
 * Note that you will get all information even when OlDnDDropFailed
 *	     is returned. It is up to an application to interpreted the
 *	     returned value.
 */
extern OlDnDDropStatus
OlDnDTrackDragCursor OLARGLIST((w, cursors, dst_info, root_info))
	OLARG( Widget,			w)
	OLARG( OlDnDAnimateCursorsPtr,	cursors)
	OLARG( OlDnDDestinationInfoPtr,	dst_info)
	OLGRA( OlDnDDragDropInfoPtr,	root_info)	/* assume non-NULL */
{
	AnimateCBDataRec		animate_cb_data;
	Boolean				status;
	Cursor				cursor;
	OlDnDDropStatus			drop_status;
	OlDnDPreviewAnimateCbP		cb;
	XtPointer			cb_data;

	if (cursors == (OlDnDAnimateCursorsPtr)NULL)
	{
		cursor	= (Cursor)None;
		cb_data = (XtPointer)NULL;
		cb	= (OlDnDPreviewAnimateCbP)NULL;
	}
	else
	{
		cursor				= cursors->yes_cursor;
		animate_cb_data.user_data	= cursors;
		animate_cb_data.reset_flag	= True;

		cb_data				= (XtPointer)&animate_cb_data;
		cb				= AnimateCB;
	}

	OlDnDGrabDragCursor(w, cursor, (Window)None);

	if (DnDDragAndDrop(w, &dst_info->window, &dst_info->x, &dst_info->y,
			   root_info, cb, cb_data, &status) == False)
	{
		drop_status = OlDnDDropCanceled;
	}
	else
	{
		drop_status = (status == True) ? OlDnDDropSucceeded :
						 OlDnDDropFailed;
	}

	OlDnDUngrabDragCursor(w);

	return(drop_status);

} /* end of OlDnDTrackDragCursor */

/*
 * OlDnDUngrabDragCursor
 *
 * The OlDnDUngrabDragCursor procedure is used to relinquish the
 * active pointer and keyboard grabs which were initiated by the
 * OlDnDGrabDragCursor procedure. This function simply ungrabs
 * the pointer and the keyboard (when necessary).
 *
 */
extern void
OlDnDUngrabDragCursor OLARGLIST((w))
	OLGRA( Widget,	w)
{
	Display *	dpy = XtDisplayOfObject(w);

	XUngrabPointer(dpy, CurrentTime);

		/* doit only when necessary...			*/
	if (handle_drag_key_proc != (OlDnDDragKeyProc)NULL)
	{
		XUngrabKeyboard(dpy, CurrentTime);
	}
} /* end of OlDnDUngrabDragCursor */

#undef GRAB_PTR_MASK
#undef DISPLAY_CURSOR
