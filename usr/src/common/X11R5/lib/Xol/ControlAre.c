/*		copyright	"%c%" 	*/

/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <Xol/libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef NOIDENT
#ident	"@(#)control:ControlAre.c	1.68"
#endif

/*
**************************************************************************
**
** Description:	This is the code for the Control Area widget.
**
********************************file*header*******************************
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/ControlArP.h>
#include <Xol/CaptionP.h>

#define ClassName ControlArea
#include <Xol/NameDefs.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void	GetCaptionAttributes OL_ARGS((
	Widget		w,
	OlDefine *	position,
	Dimension *	width,
	Dimension *	space,
	Dimension *	child
));
static Boolean	AlignCaption OL_ARGS((
	OlCLayoutList *	layout,
	Dimension	columnX,
	Dimension	leftsize
));
static void		CheckResources OL_ARGS((
	ControlAreaWidget	new,
	ControlAreaWidget	current
));
static void	CLayout OL_ARGS((ControlAreaWidget, OlCLayoutList **,
				 int, Dimension *, Dimension *)),

		RLayout OL_ARGS((ControlAreaWidget, OlCLayoutList **,
				 int, Dimension *, Dimension *)),

		HLayout OL_ARGS((ControlAreaWidget, OlCLayoutList **,
				 int, Dimension *, Dimension *)),

		WLayout OL_ARGS((ControlAreaWidget, OlCLayoutList **,
				 int, Dimension *, Dimension *));

static void	FreeLayoutList OL_ARGS((OlCLayoutList **, register int));

static void	ForceSize OL_ARGS((OlCLayoutList **, int));

static void	GetSizes OL_ARGS((Dimension *, Dimension *, Dimension *,
				  Dimension *, OlCLayoutList **,
				  int, ControlAreaWidget));

static Boolean	LayoutChildren OL_ARGS((ControlAreaWidget, Boolean, Boolean));

static void	MakeLayoutList OL_ARGS((Widget, OlCLayoutList **, int,
					Dimension *, Dimension *));

					/* class procedures		*/

static void	ChangeManaged OL_ARGS((Widget));

static void	ClassInitialize OL_NO_ARGS();

static void	ClassPartInitialize OL_ARGS((WidgetClass));

static void	Destroy OL_ARGS((Widget));

static XtGeometryResult
		QueryGeometry OL_ARGS((Widget, XtWidgetGeometry *,
				       XtWidgetGeometry *));

static XtGeometryResult
		GeometryManager OL_ARGS((Widget, XtWidgetGeometry *,
					 XtWidgetGeometry *));

static void	Initialize OL_ARGS((Widget, Widget, ArgList, Cardinal *));

static void	InitializeHook OL_ARGS((Widget, ArgList, Cardinal *));

static void	InsertChild OL_ARGS((Widget));

static void	Resize OL_ARGS((Widget));

static Boolean	SetValues OL_ARGS((Widget, Widget, Widget,ArgList,Cardinal *));

static void	Redisplay OL_ARGS((Widget, XEvent *, Region));

static void	ConstraintInitialize OL_ARGS((
			Widget, Widget, ArgList, Cardinal *));

static void	ConstraintDestroy OL_ARGS((Widget));

static Boolean	ConstraintSetValues OL_ARGS((
			Widget, Widget, Widget, ArgList, Cardinal *));

					/* action procedures		*/

					/* public procedures		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define Max(x, y)       (((x) > (y)) ? (x) : (y))
#define Min(x, y)       (((x) < (y)) ? (x) : (y))

#define CONSTRAINT(W)	(*(ControlAreaConstraintRec **)&((W)->core.constraints))
#define CHANGEBARS(W)	(W)->control.allow_change_bars
#define CB(W)		(W)->control.cb

#if	!defined(Array)
# define Array(P,T,N) \
	((N)?								\
		  ((P)?							\
			  (T *)XtRealloc((char *)(P), sizeof(T) * (N))		\
			: (T *)XtMalloc(sizeof(T) * (N))		\
		  )							\
		: ((P)? (XtFree((char *)P),(T *)0) : (T *)0)			\
	)
#endif

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

#define DftMotifShadowThickness	'2'		/* see ClassInit   */

static char shadow_thickness[] = "0 points";	/* OL_OPENLOOK_GUI */

#define OFFSET(F)	XtOffset(ControlAreaWidget, control.F)

static XtResource resources[] = {

   { XtNmeasure, XtCMeasure, XtRInt, sizeof(int),
      OFFSET(measure), XtRImmediate, (XtPointer)1
   },
   { XtNlayoutType, XtCLayoutType, XtROlDefine, sizeof(OlDefine),
      OFFSET(layouttype), XtRImmediate, (XtPointer) ((OlDefine)OL_FIXEDROWS)
   },
   { XtNhSpace, XtCHSpace, XtRDimension, sizeof(Dimension),
      OFFSET(h_space), XtRImmediate, (XtPointer)4
   },
   { XtNvSpace, XtCVSpace, XtRDimension, sizeof(Dimension),
      OFFSET(v_space), XtRImmediate, (XtPointer)4
   },
   { XtNhPad, XtCHPad, XtRDimension, sizeof(Dimension),
      OFFSET(h_pad), XtRImmediate, (XtPointer)4
   },
   { XtNvPad, XtCVPad, XtRDimension, sizeof(Dimension),
      OFFSET(v_pad), XtRImmediate, (XtPointer)4
   },
   { XtNsameSize, XtCSameSize, XtROlDefine, sizeof(OlDefine),
      OFFSET(same_size), XtRImmediate, (XtPointer) ((OlDefine)OL_COLUMNS)
   },
   { XtNalignCaptions, XtCAlignCaptions, XtRBoolean, sizeof(Boolean),
      OFFSET(align_captions), XtRImmediate,  (XtPointer)False
   },
   { XtNcenter, XtCCenter, XtRBoolean, sizeof(Boolean),
      OFFSET(center), XtRImmediate,  (XtPointer)False
   },
   { XtNallowChangeBars, XtCAllowChangeBars, XtRBoolean, sizeof(Boolean),
      OFFSET(allow_change_bars), XtRImmediate,  (XtPointer)False
   },
	/* uom: pixel, see ClassInitialize for OL_MOTIF_GUI		*/
   { XtNshadowThickness, XtCShadowThickness, XtRDimension, sizeof(Dimension),
      XtOffset(ControlAreaWidget, manager.shadow_thickness),
      XtRString, (XtPointer)shadow_thickness
   },
};
#undef OFFSET

static XtResource constraintResources[] = {
#define offset(F) XtOffset(ControlAreaConstraints, F)

    {
	XtNchangeBar, XtCChangeBar,
	XtROlDefine, sizeof(OlDefine), offset(change_bar),
	XtRImmediate, (XtPointer)OL_NONE
    }

#undef offset
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

ControlClassRec controlClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &managerClassRec,
    /* class_name         */    "Control",
    /* widget_size        */    sizeof(ControlRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */	ClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */    InitializeHook,
    /* realize            */    XtInheritRealize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterlv   */    TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    Resize,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus       */    XtInheritAcceptFocus,
    /* version            */	XtVersion,
    /* callback_private   */	NULL,
    /* tm_table           */	XtInheritTranslations,
    /* query_geometry     */	QueryGeometry,
  },{
/* composite_class fields */
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    ChangeManaged,
    /* insert_child	  */	InsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension          */    NULL
  },{
/* constraint_class fields */
    /* resources	  */	constraintResources,
    /* num_resources	  */	XtNumber(constraintResources),
    /* constraint_size	  */	sizeof(ControlAreaConstraintRec),
    /* initialize	  */	ConstraintInitialize,
    /* destroy		  */	ConstraintDestroy,
    /* set_values	  */	ConstraintSetValues,
    /* extension	  */	NULL
  },{
/* manager_class fields   */
    /* highlight_handler  */	NULL,
    /* focus_on_select	*/	True,
    /* traversal_handler  */    NULL,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* register_focus	  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ NULL, 0 },
    /* transparent_proc   */	XtInheritTransparentProc,
  },{
/* control panel class */
    /* display_layout_list */	MakeLayoutList,
 }	
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass controlAreaWidgetClass = (WidgetClass)&controlClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/**
 ** CheckResources()
 **/

static void
#if	OlNeedFunctionPrototypes
CheckResources (
	ControlAreaWidget			new,
	ControlAreaWidget			current
)
#else
CheckResources (new, current)
	ControlAreaWidget			new;
	ControlAreaWidget			current;
#endif
{
#define SET_OR_RESET(NEW,CURRENT,FIELD,DEFAULT) \
	if (CURRENT)							\
		NEW->control.FIELD = CURRENT->control.FIELD;\
	else								\
		NEW->control.FIELD = DEFAULT

	switch (new->control.layouttype) {
	case OL_FIXEDHEIGHT:
	case OL_FIXEDWIDTH:
		if (new->control.allow_change_bars) {
			OlVaDisplayWarningMsg (
				XtDisplayOfObject((Widget)new),
				"inconsistentResource", "set",
				OleCOlToolkitWarning,
				"Widget %s: XtN%s and XtN%s resources are inconsistent",
				XtName((Widget)new), XtNlayoutType, XtNallowChangeBars
			);
			new->control.allow_change_bars = False;
		}
		break;
	case OL_FIXEDROWS:
	case OL_FIXEDCOLS:
		break;
	default:
		OlVaDisplayWarningMsg (
			XtDisplayOfObject((Widget)new),
			"illegalResource", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource given illegal value",
			XtName((Widget)new), XtNlayoutType
		);
		SET_OR_RESET (new, current, layouttype, OL_FIXEDROWS);
		break;
	}

	switch (new->control.same_size) {
	case OL_NONE:
	case OL_COLUMNS:
	case OL_ALL:
		break;
	default:
		OlVaDisplayWarningMsg (
			XtDisplayOfObject((Widget)new),
			"illegalResource", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource given illegal value",
			XtName((Widget)new), XtNsameSize
		);
		SET_OR_RESET (new, current, same_size, OL_COLUMNS);
		break;
	}

	if (new->control.measure <= 0) {
		OlVaDisplayWarningMsg (
			XtDisplayOfObject((Widget)new),
			"illegalResource", "set",
			OleCOlToolkitWarning,
			"Widget %s: XtN%s resource given illegal value",
			XtName((Widget)new), XtNmeasure
		);
		SET_OR_RESET (new, current, measure, 1);
	}

#undef	SET_OR_RESET
	return;
} /* CheckResources */

/**
 ** GetCaptionAttributes()
 **/

static void
GetCaptionAttributes OLARGLIST((w, position, width, space, child))
	OLARG( Widget, w )
	OLARG( OlDefine *, position )
	OLARG( Dimension *, width )
	OLARG( Dimension *, space )
	OLGRA( Dimension *, child )
{
	int		_child;

	/*
	 * Keep these in sync with the code below!
	 */
	static Arg	args[] = {
		{ XtNposition     },
		{ XtNcaptionWidth },
		{ XtNspace        }
	};
	args[0].value = (XtArgVal)position;
	args[1].value = (XtArgVal)width;
	args[2].value = (XtArgVal)space;

	XtGetValues (w, args, XtNumber(args));
	_child = (int)CORE_P(w).width - (int)*space - (int)*width;
	*child = (_child < 0? 0 : _child);
	return;
} /* GetCaptionAttributes */

/**
 ** AlignCaption()
 **/

static Boolean
AlignCaption OLARGLIST((layout, columnX, leftsize))
	OLARG( OlCLayoutList *, layout)
	OLARG( Dimension, 	columnX )
	OLGRA( Dimension, 	leftsize )
{
	OlDefine		position;
	Dimension		width;	/* XtNcaptionWidth */
	Dimension		space;
	Dimension		child;	/* width of cap. child */
	Dimension		offset;

	GetCaptionAttributes (
		layout->widget, &position, &width, &space, &child
	);
	if (position != OL_LEFT && position != OL_RIGHT)
		return (False);

	layout->x = (int)columnX + (int)leftsize
		  - (position == OL_LEFT? (int)width : (int)child);
	return (True);
} /* AlignCaption */

/*
 *************************************************************************
 *
 * CLayout - Lay out children into w->control.measure columns
 *
 ****************************procedure*header*****************************
 */

static void
CLayout OLARGLIST((w, layoutList, layoutCount, managerWidth, managerHeight))
	OLARG( ControlAreaWidget,	w)
	OLARG( OlCLayoutList **,	layoutList)
	OLARG( int,			layoutCount)
	OLARG( Dimension *,		managerWidth)
	OLGRA( Dimension *,		managerHeight)
{
	OlCLayoutList **tempLayoutList;
	register int	i, j, k;
	Dimension *	rightList;
	Dimension *	leftList;
	Dimension *	widthList;
	Dimension *	heightList;
	Position	columnX, rowY;
	Dimension	widthSum;
	Dimension	hspace = w -> control.h_space;
	Dimension	vspace = w -> control.v_space;
	int		row, column;
	int 		limit = Min(layoutCount, w->control.measure);
	ChangeBar *	cb = CB(w);
	Dimension	cboffset = (cb? cb->width + cb->pad : 0);
	Position *	col_x  = (cb? w->control.col_x : 0);

/*
** Make all widgets within a column the same size?
*/
	if (w->control.same_size == OL_COLUMNS && !w->control.align_captions) {
		tempLayoutList =
			(OlCLayoutList **)
			XtMalloc( sizeof (OlCLayoutList *) * layoutCount);

		for (i = 0; i < w->control.measure; i++) {
			for (j=i, k=0; j < layoutCount; j += w->control.measure) {
				tempLayoutList[k++] = layoutList[j];
			}
			ForceSize(tempLayoutList,k);
		}
		XtFree((char *)tempLayoutList);
	}

	widthList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	heightList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	rightList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	leftList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);

	GetSizes(	widthList, heightList, rightList, leftList,
			layoutList, layoutCount, w);

/*
** Place the child widgets
*/
	columnX = (int) w->control.h_pad;
	rowY = (int) w->control.v_pad + (int) *managerHeight;

	i = 0;
	while (i < layoutCount) {
		row = (i/w->control.measure);
		column = i%w->control.measure;
/*
** are we starting a new row?
*/
		if (column == 0) {
			if (row != 0)
				rowY += (int) (heightList[row-1] + vspace);

			columnX = (int) (w->control.h_pad) + cboffset;
		}

/*
** Should we align the captions?
*/
		if (
		   w->control.align_captions
		&& XtIsSubclass(layoutList[i]->widget, captionWidgetClass)
		&& AlignCaption(layoutList[i], columnX, leftList[column])
		)
			;

		else if (w->control.center) {
			layoutList[i]->x =  (int) (
				columnX +
				((int)(widthList[column] -
					(layoutList[i]->border +
					layoutList[i]->width)) / 2));
		}

		else {
			layoutList[i]->x = columnX;
		}

		/*
		 * If we are showing change bars, save the x-location of
		 * each column (child widget location, not change bar
		 * location, for future reasons....hmmm).
		 */
		if (!row && col_x)   /* if row 0 and doing change bars */
			col_x[column] = columnX;

		CONSTRAINT(layoutList[i]->widget)->col = column;

		if (w->control.align_captions) {
			int	v1, v2;

			v1 = rightList[column] + leftList[column];
			v2 = widthList[column];
			columnX += (int)(Max(v1, v2) + hspace);
		}
		else {
			columnX += (int) (widthList[column] + hspace);
		}

		columnX += cboffset;

		layoutList[i]->y = rowY;
		i++;
	}

/*
** Figure out total height and width.
*/

	for (widthSum = (2 * w->control.h_pad) - hspace, i = 0;
			i < limit;
			i++) {
		if (w->control.align_captions) {
			int	v1, v2;

			v1 = rightList[i] + leftList[i];
			v2 = widthList[i];
			widthSum += Max (v1, v2) + hspace;
		}
		else {
			widthSum += (widthList[i] + hspace);
		}
		widthSum += cboffset;
	}

	*managerWidth = Max (widthSum, *managerWidth);
	*managerHeight = (int) rowY + heightList[row] + w->control.v_pad;

	XtFree((char *)widthList);
	XtFree((char *)heightList);
	XtFree((char *)leftList);
	XtFree((char *)rightList);
} /* end of CLayout */

/*
 *************************************************************************
 *
 * RLayout - Lay out children into w->control.measure rows
 *
 ****************************procedure*header*****************************
 */

static void
RLayout OLARGLIST((w, layoutList, layoutCount, managerWidth, managerHeight))
	OLARG( ControlAreaWidget,	w)
	OLARG( OlCLayoutList **,	layoutList)
	OLARG( int,			layoutCount)
	OLARG( Dimension *,		managerWidth)
	OLGRA( Dimension *,		managerHeight)
{
	OlCLayoutList	**tempLayoutList;
	register int	i, j;
	Position	columnX, rowY;
    	Dimension	hspace = w -> control.h_space;
	Dimension	vspace = w -> control.v_space;
	Dimension *	widthList;
	Dimension *	heightList;
	Dimension *	leftList;
	Dimension *	rightList;
	int	       	column, row;
	Dimension	heightSum;
	int 		limit = Min(layoutCount, w->control.measure);
	ChangeBar *	cb = CB(w);
	Dimension	cboffset = (cb? cb->width + cb->pad : 0);
	Position *	col_x  = (cb? w->control.col_x : 0);

/*
** Make all widgets within a column the same size?
*/
	if (w->control.same_size == OL_COLUMNS && !w->control.align_captions) {
		tempLayoutList =
			(OlCLayoutList **)
			XtMalloc( sizeof (OlCLayoutList *) * layoutCount);

		for (i = 0; i < layoutCount; i += w->control.measure) {
			for (j=0; j < w->control.measure; j++) {
				if (i+j >= layoutCount)
					break;
				tempLayoutList[j] = layoutList[i+j];
			}
			ForceSize(tempLayoutList,j);
		}
		XtFree((char *)tempLayoutList);
	}

	widthList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	heightList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	rightList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	leftList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);

	GetSizes(	widthList, heightList ,rightList, leftList,
			layoutList, layoutCount, w);

/*
** Lay out the widgets one column at a time, with measure
** rows per column.
*/

	columnX = (int) (w -> control.h_pad) + cboffset;
	rowY = (int) (*managerHeight + w->control.v_pad);

	i = 0;
	while (i < layoutCount) {
		row = i%w->control.measure;
		column = (i/w->control.measure);
/*
** are we starting a new column?
*/
		if ((row == 0) && (i != 0)) {
			if (w->control.align_captions) {
				int	v1, v2;

				v1 = rightList[column-1] + leftList[column-1];
				v2 = widthList[column-1];
				columnX += (int) (Max(v1, v2) + hspace);
			}
			else {
				columnX += (int) (widthList[column-1] + hspace);
			}
			columnX += cboffset;
			rowY = (int) (*managerHeight + w->control.v_pad);
		}
/*
** Should we align the captions?
*/

		if (
		   w->control.align_captions
		&& XtIsSubclass(layoutList[i]->widget, captionWidgetClass)
		&& AlignCaption(layoutList[i], columnX, leftList[column])
		)
			;

/*
** Should we center the children?
*/

		else if (w->control.center) {
			layoutList[i]->x = (int) (
				columnX +
				((int)(widthList[column] -
						(layoutList[i]->border +
						layoutList[i]->width)) / 2));
		}

/*
** Default.  Just make them left-flush.
*/
		else {
			layoutList[i]->x = columnX;
		}

		layoutList[i]->y = rowY;

		/*
		 * If we are showing change bars, save the x-location of
		 * each column (child widget location, not change bar
		 * location, for future reasons....hmmm).
		 */
		if (!row && col_x)   /* if row 0 and doing change bars */
			col_x[column] = columnX;

		CONSTRAINT(layoutList[i]->widget)->col = column;

		rowY += (int) (heightList[row] + vspace);

		i++;
	}

	for (heightSum = 0, i = 0; i < limit; i++) {
		heightSum += (heightList[i] + vspace);
	}

	*managerHeight += (heightSum + (2 * w->control.v_pad) - vspace);

/*
** Total width is current x position plus width of last column.
*/
	column = (layoutCount-1) / w->control.measure;

	if (w->control.align_captions) {
		int	v1, v2;

		v1 = rightList[column] + leftList[column];
		v2 = widthList[column];
		*managerWidth = (int) (columnX +
				       Max(v1, v2) +
				       w->control.h_pad);
	}

	else {
		*managerWidth =	(int) (
				(int) columnX +
				(int) widthList[column] +
				(int) w->control.h_pad);
	}

	XtFree((char *)widthList);
	XtFree((char *)heightList);
	XtFree((char *)leftList);
	XtFree((char *)rightList);
} /* end of RLayout */


/*
 *************************************************************************
 *
 * HLayout - Lay out the children in columns, with no column taller than
 * w->control.measure.  If this is not set, default to height of
 * tallest child.
 *
 ****************************procedure*header*****************************
 */


static void
HLayout OLARGLIST((w, layoutList, layoutCount, managerWidth, managerHeight))
	OLARG( ControlAreaWidget,	w)
	OLARG( OlCLayoutList **,	layoutList)
	OLARG( int,			layoutCount)
	OLARG( Dimension *,		managerWidth)
	OLGRA( Dimension *,		managerHeight)
{
	register int	i, j;
	Position	columnX, rowY;
	Dimension	hspace = w -> control.h_space;
	Dimension	vspace = w -> control.v_space;
	Dimension	maxWidthForColumn = 0, maxHeight = 0;
	Dimension	columnHeight;

/*
** Find size of highest widget.
*/

	for (i = 0; i < layoutCount; i++) {
		if ((Dimension)(layoutList[i]->height + layoutList[i]->border)
			 > maxHeight)
			maxHeight = layoutList[i]->height +
					layoutList[i]->border;
	}


/*
** Lay out the widgets in columns, advancing to the next
** row when each column becomes bigger than the larger of
** measure or the tallest widget.
*/
	columnX = (int) (w->control.h_pad);
	rowY = (int) (w->control.v_pad + *managerHeight);

	{
		int	v1, v2;

		v1 = w->control.measure;
		v2 = maxHeight + 2 * w->control.v_pad;
		columnHeight = Max(v1, v2);
	}

	i = 0;
	while (i < layoutCount) {

/*
** Do we need to start a new column?
*/
		if (	(Dimension) (rowY +
					layoutList[i]->height +
					layoutList[i]->border +
					w->control.v_pad) >
			columnHeight) {

			rowY = (int) (w->control.v_pad + *managerHeight);
			columnX += (int) (hspace + maxWidthForColumn);
			maxWidthForColumn = layoutList[i]->width +
					layoutList[i]->border;
		}

		layoutList[i]->x = columnX;
		layoutList[i]->y = rowY;

		rowY += (int) (vspace + layoutList[i]->height +
				layoutList[i]->border);

		if (maxWidthForColumn <
			(Dimension)(layoutList[i]->width +
				    layoutList[i]->border))
			maxWidthForColumn = layoutList[i]->width +
					layoutList[i]->border;

		i++;
	}

	*managerWidth = (int) columnX + maxWidthForColumn + w->control.h_pad;
	*managerHeight += columnHeight;

} /* end of HLayout */

/*
 *************************************************************************
 *
 * Lay out the children in rows, with no row wider than
 * mw->control.measure.  If this is not set, default to width of
 * fattest child.
 *
 ****************************procedure*header*****************************
 */


static void
WLayout OLARGLIST((w, layoutList, layoutCount, managerWidth, managerHeight))
	OLARG( ControlAreaWidget,	w)
	OLARG( OlCLayoutList **,	layoutList)
	OLARG( int,			layoutCount)
	OLARG( Dimension *,		managerWidth)
	OLGRA( Dimension *,		managerHeight)
{
	register int	i, j;
	Position	columnX, rowY;
	Dimension	hspace = w -> control.h_space;
	Dimension	vspace = w -> control.v_space;
	Dimension	maxWidgetWidth= 0, maxHeightForColumn  = 0;
	Dimension	columnWidth;

/*
** Find size of widest widget.
*/

	for (i = 0; i < layoutCount; i++) {
		if ((Dimension)(layoutList[i]->width + layoutList[i]->border) >
				maxWidgetWidth)
			maxWidgetWidth = layoutList[i]->width +
					layoutList[i]->border;
	}

/*
** Lay out the widgets, advancing to the next
** row as needed.
*/
	columnX = (int) (w->control.h_pad);
	rowY = (int) (w->control.v_pad + *managerHeight);

	{
		int	v1, v2;

		v1 = w->control.measure;
		v2 = maxWidgetWidth + 2 * w->control.h_pad;
		columnWidth = Max(v1, v2);
	}

	for (i = 0; i < layoutCount; i++) {

		if ((Dimension)(columnX +
				layoutList[i]->width + layoutList[i]->border +
				w->control.h_pad) >
				columnWidth) {
			columnX = (int) (w->control.h_pad);
			rowY += (int) (vspace + maxHeightForColumn);
			maxHeightForColumn = layoutList[i]->height +
					layoutList[i]->border;
		}

		layoutList[i]->x = columnX;
		layoutList[i]->y = rowY;

		columnX += (int) (hspace + layoutList[i]->width +
				layoutList[i]->border);

		if (maxHeightForColumn < (Dimension)(layoutList[i]->height +
				layoutList[i]->border))
			maxHeightForColumn = layoutList[i]->height +
					layoutList[i]->border;

	}

	*managerHeight = (int) rowY + maxHeightForColumn + w->control.v_pad;
	*managerWidth = Max (*managerWidth, columnWidth);
} /* end of WLayout */

/*
 *************************************************************************
 *
 * FreeLayoutList - de-allocates memory for the layout list that was
 * created in Layout().
 *
 ****************************procedure*header*****************************
 */

static void
FreeLayoutList OLARGLIST((layoutList, num_children))
	OLARG( OlCLayoutList **,	layoutList)
	OLGRA( register int,		num_children)
{
	while(num_children--) {
		XtFree((char *)layoutList[num_children]);
	}
	XtFree((char *)layoutList);
} /* end of FreeLayoutList */

/*
 *************************************************************************
 *
 * ForceSize - Force the children's widths to match.
 *
 ****************************procedure*header*****************************
 */

static void
ForceSize OLARGLIST((layoutList, layoutCount))
	OLARG( OlCLayoutList **,	layoutList)
	OLGRA( int,			layoutCount)
{
	register int	 i;
	Dimension	width;

	width = 0;

	for (i = 0; i < layoutCount; i++) {
		width = Max (width, layoutList[i]->width);
	}

	for (i = 0; i < layoutCount; i++) {
		layoutList[i]->width = width;
	}
} /* end of ForceSize */


/*
 *************************************************************************
 *
 * GetSizes - Find width of widest widget for each column, and height of
 * highest widget in each row.  In case we're aligning the captions, keep
 * track of the largest right and left halves of any caption widgets.
 * Space between caption and child is counted as part of left half.
 * Border widths of caption and child are counted in the right half.
 *
 ****************************procedure*header*****************************
 */


static void
GetSizes OLARGLIST((widthList, heightList, rightList, leftList, layoutList, layoutCount, w))
	OLARG( Dimension *,		widthList)
	OLARG( Dimension *,		heightList)
	OLARG( Dimension *,		rightList)
	OLARG( Dimension *,		leftList)
	OLARG( OlCLayoutList **,	layoutList)
	OLARG( int,			layoutCount)
	OLGRA( ControlAreaWidget,	w)
{
	int	i;
	int	row, column;
	Dimension	tempLeft, tempRight;

	for (i = 0; i < layoutCount; i++) {
		widthList[i] = 0;
		heightList[i] = 0;
		rightList[i] = 0;
		leftList[i] = 0;
	}

	for (i = 0; i < layoutCount; i++) {

		switch (w -> control.layouttype ) {
			case OL_FIXEDROWS:
				row = i%w->control.measure;
				column = (i/w->control.measure);
			break;

			case OL_FIXEDCOLS:
				row = (i/w->control.measure);
				column = i%w->control.measure;
			break;
		}

		if (XtIsSubclass(layoutList[i]->widget, captionWidgetClass)) {
			OlDefine	position;
			Dimension	width;	/* XtNcaptionWidth */
			Dimension	space;
			Dimension	child;	/* width of cap. child */

			GetCaptionAttributes (
				layoutList[i]->widget,
				&position, &width, &space, &child
			);
			if (position == OL_RIGHT) {
				tempLeft = child;
				tempRight = width + layoutList[i]->border + space;
			} else {
				tempLeft = width;
				tempRight = child + layoutList[i]->border + space;
			}
			if (leftList[column] < tempLeft)
				leftList[column] =  tempLeft;

			if (rightList[column] < tempRight)
				rightList[column] =  tempRight;
		}

		if ( (Dimension)(layoutList[i]->height +
				 layoutList[i]->border) >
				heightList[row]) {
			heightList[row] =
				layoutList[i]->height + layoutList[i]->border;
		}

		if ( (Dimension)(layoutList[i]->width +
				 layoutList[i]->border) >
				widthList[column]) {
			widthList[column] =
				layoutList[i]->width + layoutList[i]->border;
		}
	}
} /* end of GetSizes */

/*
 *************************************************************************
 *
 * LayoutChildren -  Calculate the positions of each managed child and
 * try to resize yourself to fit.  Return True if the resize is okay,
 * False otherwise.
 *
 * Don't actually move the children unless doit is True.
 * Don't resize yourself unless resizeok is True.
 *
 ****************************procedure*header*****************************
 */


static Boolean
LayoutChildren OLARGLIST((mw, doit, resizeok))
	OLARG( ControlAreaWidget,	mw)
	OLARG( Boolean,			doit)
	OLGRA( Boolean,			resizeok)
{
	OlCLayoutList	**layoutList;
	int		layoutCount = 0;
	Widget		child;
	register int	i;
	Dimension	managerWidth, managerHeight;
	Dimension	requestWidth, requestHeight;
	Dimension	replyWidth, replyHeight;
	Boolean		moveFlag = False;
	Boolean		resizeFlag = False;

/*
** Allocate a layout list to use in layout processing, then
** loop through the children to get their preferred sizes.
*/
	layoutList =
		(OlCLayoutList **) XtMalloc( sizeof (OlCLayoutList *) *
			mw->composite.num_children);

	for (i = 0; i < mw -> composite.num_children; i++) {
		layoutList[i] = (OlCLayoutList *)
			XtMalloc (sizeof (OlCLayoutList));
		layoutList[i]->widget = NULL;
		child = mw -> composite.children[i];

		if (child -> core.managed) {
			layoutList[layoutCount]->widget = child;
			layoutList[layoutCount]->x = 0;
			layoutList[layoutCount]->y = 0;
			layoutList[layoutCount]->width = child->core.width;
			layoutList[layoutCount]->height = child->core.height;
			layoutList[layoutCount]->border =
					child -> core.border_width * 2;
			layoutCount++;
		}
	}

	if (layoutCount == 0)  {
		FreeLayoutList(layoutList, mw->composite.num_children);
		return(True);	/*  no managed children  */
	}

	if (mw->control.same_size == OL_ALL && !mw->control.align_captions) {
		ForceSize (layoutList, layoutCount);
	}

	managerWidth = managerHeight = 0;

	controlClassRec.control_class.display_layout_list(
			(Widget)mw, layoutList, layoutCount,
			&managerWidth, &managerHeight);

/*
** We now have an array with the set of minimum position for each
** widget.  We also have a manager width and height needed for
** this layout.
** First see if a geometry request needs to be made.  If so, and if
** resizing is okay, make it.
*/

	requestWidth = managerWidth;
	requestHeight = managerHeight;

	if (	resizeok &&
		(requestWidth != mw->core.width ||
		requestHeight != mw->core.height)) {
		switch (XtMakeResizeRequest (	(Widget) mw,
						requestWidth, requestHeight,
				                &replyWidth, &replyHeight)) {
		case XtGeometryYes:
			break;

		case XtGeometryNo:
/*
** This picks up the case where parent answers "No" but sets replyHeight and
** replyWidth to what I asked for.  Don't ask why; I don't understand either.
*/
			FreeLayoutList(layoutList, mw->composite.num_children);

			if ((replyWidth <= mw->core.width) &&
	    			(replyHeight <= mw->core.height)) {
	    			return (True);
			}
			else
				return (False);
			break;

		case XtGeometryAlmost:
			XtMakeResizeRequest (	(Widget) mw,
						replyWidth, replyHeight,
						&replyWidth, &replyHeight);
			if (XtIsRealized ((Widget) mw)) {
				FreeLayoutList(layoutList,
						mw->composite.num_children);
				return (True);
			}
			break;
		}
	}

/*
** Are we really doing the layout?  If not, just return True to show
** that it would be possible.
*/

	if (!doit) {
		FreeLayoutList(layoutList, mw->composite.num_children);
		return(True);
	}

/*
** Relayout the children, making only necessary layout changes.
*/

	for (i = 0; i < layoutCount; i++) {
		moveFlag = resizeFlag = False;

		if (layoutList[i]->x != layoutList[i]->widget->core.x ||
				layoutList[i]->y!=layoutList[i]->widget->core.y)
			moveFlag = True;

		if (layoutList[i]->width != layoutList[i]->widget->core.width ||
				layoutList[i]->height !=
				layoutList[i]->widget -> core.height)
			resizeFlag = True;

		if (moveFlag && resizeFlag)
			XtConfigureWidget (layoutList[i]->widget,
					layoutList[i]->x,
					layoutList[i]->y,
					layoutList[i]->width,
					layoutList[i]->height,
					layoutList[i]->widget->core.border_width);
		else if (moveFlag)
			XtMoveWidget (layoutList[i]->widget,
					layoutList[i]->x, layoutList[i]->y);
		else if (resizeFlag)
			XtResizeWidget (layoutList[i]->widget,
					layoutList[i]->width,
					layoutList[i]->height,
					layoutList[i]->widget->core.border_width);
	}

   FreeLayoutList(layoutList, mw->composite.num_children);
   return (True);
} /* end of LayoutChildren */

/*
 *************************************************************************
 *
 * MakeLayoutList - Switch on the type of layout and fill the widget's
 * layout list appropriately.  Return the required height and width in
 * pmanagerHeight and pmanagerWidth.
 *
 ****************************procedure*header*****************************
 */

static void
MakeLayoutList OLARGLIST((w, layoutList, layoutCount, pmanagerWidth, pmanagerHeight))
	OLARG( Widget,			w)
	OLARG( OlCLayoutList **,	layoutList)
	OLARG( int,			layoutCount)
	OLARG( Dimension *,		pmanagerWidth)
	OLGRA( Dimension *,		pmanagerHeight)
{
	ControlAreaWidget	mw = (ControlAreaWidget)w;
	/*
	 * If change bars are being displayed, allocate an array
	 * large enough to hold the x-position of each column.
	 * We do this only for row-major or column-major
	 * layout types.
	 *
	 * Note: This array may have already been allocated, so check
	 * its previous size and reallocate if different. THIS TAKES
	 * CARE OF HANDLING A CHANGE IN XtNMeasure IN XtSetValues()!
	 */
	if (CHANGEBARS(mw)) {
		ChangeBar *		cb = CB(mw);

		Cardinal		ncolumns;


		switch (mw -> control.layouttype) {
		case OL_FIXEDCOLS:
			ncolumns = mw->control.measure;
			goto Alloc;
		case OL_FIXEDROWS:
			ncolumns = (mw->composite.num_children + mw->control.measure-1)
				 / mw->control.measure;
			if (ncolumns < 1)
				ncolumns = 1;
Alloc:			if (mw->control.ncolumns != ncolumns) {
				mw->control.ncolumns = ncolumns;
				mw->control.col_x = Array(
				        mw->control.col_x,
					Position,
					mw->control.ncolumns
				);
			}
			break;
		}
	}

	switch ( mw -> control.layouttype ) {

	case OL_FIXEDROWS:
		RLayout (mw, layoutList, layoutCount,
				pmanagerWidth, pmanagerHeight);
		break;

	case OL_FIXEDCOLS:
		CLayout (mw, layoutList, layoutCount,
				pmanagerWidth, pmanagerHeight);
		break;

	case OL_FIXEDHEIGHT:
		HLayout (mw, layoutList, layoutCount,
				pmanagerWidth, pmanagerHeight);
		break;

	case OL_FIXEDWIDTH:
		WLayout (mw, layoutList, layoutCount,
				pmanagerWidth, pmanagerHeight);
		break;
	}

		/* include shadowThickness...		*/
	if (mw->manager.shadow_thickness != 0)
	{
		Cardinal	i;

		for (i = 0; i < layoutCount; i++)
		{
			layoutList[i]->x += mw->manager.shadow_thickness;
			layoutList[i]->y += mw->manager.shadow_thickness;
		}
		*pmanagerWidth += 2 * mw->manager.shadow_thickness;
		*pmanagerHeight += 2 * mw->manager.shadow_thickness;
	}
} /* end of MakeLayoutList */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 *
 * ClassInitialize - Register OlDefine string values.
 *
 ****************************procedure*header*****************************
 */

static void
ClassInitialize OL_NO_ARGS()
{
	_OlAddOlDefineType ("fixedrows",   OL_FIXEDROWS);
	_OlAddOlDefineType ("fixedcols",   OL_FIXEDCOLS);
	_OlAddOlDefineType ("fixedwidth",  OL_FIXEDWIDTH);
	_OlAddOlDefineType ("fixedheight", OL_FIXEDHEIGHT);
	_OlAddOlDefineType ("columns",     OL_COLUMNS);
	_OlAddOlDefineType ("all",         OL_ALL);
	_OlAddOlDefineType ("none",        OL_NONE);
	_OlAddOlDefineType ("normal",      OL_NORMAL);
	_OlAddOlDefineType ("dim",         OL_DIM);

	if (OlGetGui() == OL_MOTIF_GUI)
	{
		shadow_thickness[0] = DftMotifShadowThickness;
	}
} /* end of ClassInitialize */

/*
 *************************************************************************
 *
 * Resize - Lay out the children, not allowing resizing.
 *
 ****************************procedure*header*****************************
 */

static void
Resize OLARGLIST((w))
	OLGRA( Widget,	w)
{
	(void) LayoutChildren((ControlAreaWidget)w, True, False);
} /* end of Resize */

/****************************procedure*header*****************************
 * Destroy-
 */
static void
Destroy OLARGLIST((w))
	OLGRA( Widget,	w)
{
	ControlAreaWidget	widget = (ControlAreaWidget)w;

	XtFree((char *)widget->control.post_select);

	if (CHANGEBARS(widget))
		_OlDestroyChangeBar (w, CB(widget));
} /* end of Destroy */

/**
 ** Redisplay()
 **/

static void
Redisplay OLARGLIST((widget, pe, region))
	OLARG( Widget,		widget)
	OLARG( XEvent *,	pe)
	OLGRA( Region,		region)
{
	Cardinal		i;
	ControlAreaWidget	w = (ControlAreaWidget)widget;


#define SUPERCLASS	\
	((ControlClassRec *)controlClassRec.core_class.superclass)

		/* use superclass method to draw the border shadow	*/
	(*SUPERCLASS->core_class.expose)(widget, pe, region);

#undef SUPERCLASS
	/*
	 * This exposure routine is used to display change bars,
	 * but only if we can handle them.
	 */
	if (!CHANGEBARS(w))
		return;

	/*
	 * Expose each change bar, if inside the exposure region.
	 */
	for (i = 0; i < w->composite.num_children; i++) {
		Widget			child = w->composite.children[i];

		ControlAreaConstraintRec * constraint = CONSTRAINT(child);

		if (XtIsManaged(child) && constraint->change_bar != OL_NONE)
			_OlDrawChangeBar (
				(Widget)w,
				CB(w),
				constraint->change_bar,
				False,
				w->control.col_x[constraint->col]
						- OlChangeBarSpan(CB(w)),
				child->core.y,
				region
			);
	}

	return;
} /* end of Redisplay */

/*
 *************************************************************************
 *
 * ChangeManaged - Lay out the children, allowing resizing.
 *
 ****************************procedure*header*****************************
 */

static void
ChangeManaged OLARGLIST((w))
	OLGRA( Widget,	w)
{
	(void) LayoutChildren ((ControlAreaWidget)w, True, True);
} /* end of ChangeManaged */

/*
 *************************************************************************
 *
 * Initialize
 *
 ****************************procedure*header*****************************
 */

/* ARGSUSED */
static void
Initialize OLARGLIST((request_w, new_w, args, num_args))
	OLARG( Widget,		request_w)
	OLARG( Widget,		new_w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	ControlAreaWidget new = (ControlAreaWidget)new_w;
	ControlAreaWidget request = (ControlAreaWidget)request_w;

	CheckResources(new, (ControlAreaWidget)0);

	if (new->core.width == 0)
	{
		new->core.width =
			new->control.h_space != 0 ? new->control.h_space : 10;
	}

	if (new->core.height == 0)
	{
		new->core.height =
			new->control.v_space != 0 ? new->control.v_space : 10;
	}

	if (	(	new->control.layouttype == OL_FIXEDHEIGHT
		     || new->control.layouttype == OL_FIXEDWIDTH
		)
	     && CHANGEBARS(new)
	) {

		OlVaDisplayWarningMsg(	XtDisplay(new_w),
					OleNfileControlArea,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileControlArea_msg1);
		CHANGEBARS(new) = False;
	}

	if (CHANGEBARS(new))
		CB(new) = (ChangeBar *)_OlCreateChangeBar((Widget)new);
	else
		CB(new) = 0;

	new->control.col_x    = 0;
	new->control.ncolumns = 0;
} /* end of Initialize */

/*
 *************************************************************************
 *
 * InitializeHook
 *
 ****************************procedure*header*****************************
 */

static void
InitializeHook OLARGLIST((w, args, num_args))
	OLARG( Widget,		w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	ControlAreaWidget	cw = (ControlAreaWidget)w;
	static XtCallbackList	list;
	static MaskArg		marg[] = {
		{ XtNpostSelect, (XtArgVal) &list, OL_COPY_SOURCE_VALUE },
		{ NULL, (XtArgVal) sizeof(XtCallbackList), OL_COPY_SIZE }
		};

	list = NULL;
	cw->control.post_select = NULL;

	/* We don't need to supply a count or a return list pointer
	 * since this rule does not generate a return list
	 */
	_OlComposeArgList(args, *num_args, marg, XtNumber(marg), NULL, NULL);

	/* save caller's callback list in widget struct.  this will be passed
	 * to button children (InsertChild)
	 */
	if (list != NULL) {
		int i;
		XtCallbackList cb;

		for (i=0; list[i].callback != NULL; i++)
			;			/* no-op for counting */
		
		i++;				/* for NULL delimiters */
		cb = (XtCallbackList) XtMalloc(i * sizeof (XtCallbackRec));
		cw->control.post_select = cb;

		while (i--) {
			cb[i].callback	= list[i].callback;
			cb[i].closure	= list[i].closure;
		}
	}
} /* end of InitializeHook */

/*
 *************************************************************************
 *
 * SetValues - real stupid
 *
 ****************************procedure*header*****************************
 */

/* ARGSUSED */
static Boolean
SetValues OLARGLIST((cur_w, req_w, new_w, args, num_args))
	OLARG( Widget, 		cur_w)
	OLARG( Widget, 		req_w)
	OLARG( Widget, 		new_w)
	OLARG( ArgList,		args)
	OLGRA( Cardinal *,	num_args)
{
	ControlAreaWidget current = (ControlAreaWidget)cur_w;
	ControlAreaWidget request = (ControlAreaWidget)req_w;
	ControlAreaWidget new = (ControlAreaWidget)new_w;
	Boolean			ret		= False;

	CheckResources(new, current);

	if (	CHANGEBARS(new)
	     && (	new->control.layouttype == OL_FIXEDHEIGHT
		     || new->control.layouttype == OL_FIXEDWIDTH
		)
	) {
		OlVaDisplayWarningMsg(	XtDisplayOfObject(new_w),
					OleNfileControlArea,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileControlArea_msg1);
		CHANGEBARS(new) = False;
	}

	/*
	 * If the XtNchangeBars resource has changed, either we have
	 * to create new auxiliary structures for the widget or we
	 * have to destroy the structures.
	 */
	if (CHANGEBARS(new) != CHANGEBARS(current)) {
		if (!CHANGEBARS(new))
			_OlDestroyChangeBar ((Widget)new, CB(new));
		else
			CB(new) = (ChangeBar *)_OlCreateChangeBar((Widget)new);
	}

	/*
	 * If the background color has changed, we have to create new GCs.
	 * We don't actually get the GCs here, just free the current
	 * ones (if any) and clear the references to GCs. When we
	 * really need the GCs we'll get them.
	 */
	if (	CHANGEBARS(new)
	     && new->core.background_pixel != current->core.background_pixel
	)
		_OlFreeChangeBarGCs ((Widget)new, CB(new));

	/*
	 * If anything affecting the layout of the children has changed,
	 * lay them out (again).
	 */
	if (	new->control.layouttype != current->control.layouttype
	     || new->control.measure != current->control.measure
	     || CHANGEBARS(new) != CHANGEBARS(current)
	) {
		LayoutChildren (new, True, True);
		ret = True;
	}

	return (ret);
} /* end of SetValues */

/*
 *************************************************************************
 *
 * ClassPartInitialize - ensure that any widgets subclassed off
 * control area can inherit the layout routines.
 *
 ****************************procedure*header*****************************
 */

static void
ClassPartInitialize OLARGLIST((widgetClass))
	OLGRA(WidgetClass,	widgetClass)
{
	ControlAreaWidgetClass wc =
		(ControlAreaWidgetClass) widgetClass;
	ControlAreaWidgetClass super =
		(ControlAreaWidgetClass) wc->core_class.superclass;
#ifdef SHARELIB
	void **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit		(*__libXol__XtInherit)
#endif

	if (wc->control_class.display_layout_list ==
			XtInheritDisplayLayoutList) {
		wc->control_class.display_layout_list =
				super->control_class.display_layout_list;
	}
} /* end of ClassPartInitialize */

/*
 *************************************************************************
 *
 * QueryGeometry
 *
 ****************************procedure*header*****************************
 */

static XtGeometryResult
QueryGeometry OLARGLIST((w, constraint, preferred))
	OLARG( Widget,			w)
	OLARG( XtWidgetGeometry *,	constraint)
	OLGRA( XtWidgetGeometry *,	preferred)
{
	/*
	 * MORE: SHOULD QUERY OUR CHILDREN ABOUT THIS.
	 */
	return (XtGeometryNo);
} /* end of QueryGeometry */

/*
 *************************************************************************
 *
 * Geometry Manager
 *
 ****************************procedure*header*****************************
 */

static XtGeometryResult
GeometryManager OLARGLIST((w, request, reply))
	OLARG( Widget,			w)
	OLARG( XtWidgetGeometry	*,	request)
	OLGRA( XtWidgetGeometry	*,	reply)

{
	Dimension	width, height, borderWidth;
	ControlAreaWidget cw;

/*
** Children aren't allowed to specify their position
*/
	if ((request->request_mode & CWX && request->x != w->core.x) ||
		(request->request_mode & CWY && request->y != w->core.y))
		return (XtGeometryNo);

/*
** Ensure all three fields in the request are valid
*/
	if (request->request_mode & (CWWidth | CWHeight | CWBorderWidth)) {
		if ((request->request_mode & CWWidth) == 0)
			request->width = w->core.width;
		if ((request->request_mode & CWHeight) == 0)
			request->height = w->core.height;
		if ((request->request_mode & CWBorderWidth) == 0)
			request->border_width = w->core.border_width;
	}

/*
** Save current size and try new size
*/
	width = w->core.width;
	w->core.width = request->width;
	height = w->core.height;
	w->core.height = request->height;
	borderWidth = w->core.border_width;
	w->core.border_width = request->border_width;

	cw = (ControlAreaWidget) w->core.parent;
/*
** See if it's possible to lay out the children now.
** If not, reset core values and return No.  Else, really do the
** layout and return Yes.
*/
	if (LayoutChildren(cw,False,True) == False) {
		w->core.width = width;
		w->core.height = height;
		w->core.border_width = borderWidth;
		return (XtGeometryNo);
	}

/*
	(*XtClass((Widget)cw)->core_class.resize)((Widget)cw);
*/
	LayoutChildren(cw,True,True);
	return (XtGeometryYes);
} /* end of GeometryManager */

/*
 *************************************************************************
 *
 * InsertChild - If the control area has its post-select resource set,
 * and the child is a button, add these callbacks to the child's post-select.
 *
 * Call the superclass's insert-child.
 *
 ****************************procedure*header*****************************
 */


static void
InsertChild OLARGLIST((w))
	OLGRA( Widget,	w)
{
    ControlAreaWidget	cw = (ControlAreaWidget) XtParent(w);
    XtWidgetProc	insert_child = ((CompositeWidgetClass)
	(controlClassRec.core_class.superclass))->composite_class.insert_child;

    if ((cw->control.post_select != (XtCallbackList) NULL) &&
	(XtHasCallbacks(w, XtNpostSelect) != XtCallbackNoList))
	XtAddCallbacks(w, XtNpostSelect, cw->control.post_select);

    if (insert_child)
	(*insert_child)(w);
} /* end of InsertChild */

/**
 ** ConstraintInitialize()
 **/

static void
ConstraintInitialize OLARGLIST((request, new, args, num_args))
	OLARG( Widget,			request)
	OLARG( Widget,			new)
	OLARG( ArgList,			args)
	OLGRA( Cardinal *,		num_args)
{
#if	defined(NEED_THIS)
	ControlAreaConstraintRec *	constraint = CONSTRAINT(new);
	return;
#endif
} /* end of ConstraintInitialize */

/**
 ** ConstraintDestroy()
 **/

static void
ConstraintDestroy OLARGLIST((w))
	OLGRA( Widget,			w)
{
#if	defined(NEED_THIS)
	ControlAreaConstraintRec *	constraint = CONSTRAINT(w);
	return;
#endif
} /* end of ConstraintDestroy */

/**
 ** ConstraintSetValues()
 **/

static Boolean
ConstraintSetValues OLARGLIST((old, request, new, args, num_args))
	OLARG( Widget,			old)
	OLARG( Widget,			request)
	OLARG( Widget,			new)
	OLARG( ArgList,			args)
	OLGRA( Cardinal *,		num_args)
{
	ControlAreaWidget	parent = (ControlAreaWidget)XtParent(new);

	ControlAreaConstraintRec * curConstraint = CONSTRAINT(old);
	ControlAreaConstraintRec * newConstraint = CONSTRAINT(new);

	Boolean			needs_redisplay	 = False;


	/*
	 * We play a subtle trick here: We aren't supposed to do any
	 * (re)displaying in this routine, but should just return a
	 * Boolean that indicates whether the Intrinsics should force
	 * a redisplay. But the Intrinsics do this by ``calling the
	 * Xlib XClearArea() function on the [parent] widget's window.''
	 * Instead of returning True here, which will cause the entire
	 * ControlArea to be redisplayed, we instead call XClearArea
	 * ourselves on just the area of the change bar and return False.
	 * The XClearArea() (done inside DrawChangeBar()) will generate
	 * a much smaller expose event. Now this will cause another call
	 * to DrawChangeBar() for the same change bar, but that can't
	 * be helped--we don't know what other events might have occurred,
	 * so we can't do the real drawing here.
	 */
	if (
		CHANGEBARS(parent)
	     && newConstraint->change_bar != curConstraint->change_bar
	     && XtIsRealized((Widget)parent)
	     && parent->control.col_x
	)
		_OlDrawChangeBar (
			(Widget)parent,
			CB(parent),
			newConstraint->change_bar,
			True,
			parent->control.col_x[newConstraint->col]
					- OlChangeBarSpan(CB(parent)),
			new->core.y,
			(Region)0
		);

	return (needs_redisplay);
} /* end of ConstraintSetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */


/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */
