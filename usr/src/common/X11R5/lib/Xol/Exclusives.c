/* XOL SHARELIB - start */
/* This header file must be included before anything else */
#ifdef SHARELIB
#include <libXoli.h>
#endif
/* XOL SHARELIB - end */

#ifndef	NOIDENT
#ident	"@(#)exclusives:Exclusives.c	1.60"
#endif

/*
 *************************************************************************
 *
 * Description: Exclusives.c - Exclusives widget
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/
#include <stdio.h>
#include <ctype.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>
#include <OpenLookP.h>
#include <RectButtoP.h>
#include <ExclusiveP.h>
#include <Olg.h>

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

static void CalcColsRows(); 
static void CalcNewMax(); 
static void GetDefaultButtons(); 
static void GetSetButtons(); 
static void Layout();
static void ResizeSelf();

					/* class procedures		*/
static void ChangedManaged();
static void ClassInitialize();
static void Destroy();
static XtGeometryResult GeometryManager();
static void Initialize();
static void InsertChild();
static void DeleteChild();
static XtGeometryResult QueryGeometry();
static void Realize();
static Boolean SetValues();

static int GetFocusItems();
static int NewIndex();
static Widget TraversalHandler();

					/* action procedures		*/
static void LeaveHandler();

					/* public procedures		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static int defNEITHER = 3;

#define offset(field) XtOffset(ExclusivesWidget, field)

#define ARRAY_SIZE 50

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/*
 ************************************************************
 *
 *  LeaveHandler - This function alerts when pointer leaves
 *	the Exclusives.
 *
 *********************function*header************************
 */

/* ARGSUSED */
static void LeaveHandler(widget,client_data,event,continue_to_dispatch)
	Widget		widget;
	XtPointer	client_data;
	XEvent *	event;
	Boolean *	continue_to_dispatch;
{
	ExclusivesWidget ew;
	Arg arg[2];
	XCrossingEvent *xce;

	if(!XtIsSubclass(widget,exclusivesWidgetClass)) return;

	if(event->type==EnterNotify || event->type==LeaveNotify) {

	xce = (XCrossingEvent *) &(event->xcrossing);
	if(xce->mode!=NotifyNormal) return; /* filter out pointer crossings */
	}

	ew= (ExclusivesWidget) widget;
	if(ew->composite.num_children==0) return;

	if(ew->exclusives.set_child!=ew->exclusives.looks_set) {
		if(ew->exclusives.looks_set!=(Widget)0) {
			XtSetArg(arg[0],XtNset,FALSE);
			XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
			XtSetValues(ew->exclusives.looks_set,arg,2);
			ew->exclusives.looks_set=(Widget)0;
		}
		if(ew->exclusives.set_child!=(Widget)0) {
			XtSetArg(arg[0],XtNset,TRUE);
			XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
			XtSetValues(ew->exclusives.set_child,arg,2);
			ew->exclusives.looks_set=ew->exclusives.set_child;
		}
	}
} /* LeaveHandler */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = { 

	{ XtNlayoutType, XtCLayoutType, XtROlDefine, sizeof(OlDefine),
	  	offset(exclusives.layout), XtRImmediate,
		(XtPointer) ((OlDefine) OL_FIXEDROWS) },

	{ XtNmeasure, XtCMeasure, XtRInt, sizeof(int),
	  	offset(exclusives.measure), XtRImmediate, (XtPointer) 1 },

	{ XtNnoneSet, XtCNoneSet, XtRBoolean, sizeof(Boolean),
	  	offset(exclusives.noneset), XtRImmediate, (XtPointer) False },

	{ XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
	  	offset(exclusives.recompute_size),XtRImmediate, (XtPointer) True },

	{ XtNshellBehavior, XtCShellBehavior, XtRInt, sizeof(int),
		offset(exclusives.shell_behavior), XtRImmediate, 
		(XtPointer) OtherBehavior },

	{ XtNresetSet, XtCResetSet, XtRInt, sizeof(int),
	  	offset(exclusives.reset_set), XtRInt,(XtPointer) &defNEITHER },

	{ XtNresetDefault, XtCResetDefault, XtRInt, sizeof(int),
	  	offset(exclusives.reset_default), XtRInt,(XtPointer) &defNEITHER},

	{ XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean),
	  	offset(exclusives.is_default), XtRImmediate,(XtPointer) False },

	{ XtNdefaultData, XtCDefaultData, XtRPointer, sizeof(XtPointer),
		offset(exclusives.default_data), XtRImmediate, (XtPointer) NULL},

	{ XtNpreview, XtCPreview, XtRWidget, sizeof(Widget), 
		offset(exclusives.preview), XtRImmediate, (XtPointer) NULL},

	{XtNpostSelect, XtCCallback, XtRCallback, sizeof(XtPointer), 
		offset(exclusives.postselect), XtRCallback, (XtPointer) NULL},
};

#undef offset

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

ExclusivesClassRec exclusivesClassRec = {
  {
    (WidgetClass) &(managerClassRec),	/* superclass		  */	
    "Exclusives",			/* class_name		  */
    sizeof(ExclusivesRec),		/* widget_size		  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    (XtInitProc) Initialize,		/* initialize		  */
    NULL,				/* initialize_hook	  */
    (XtRealizeProc) Realize,		/* realize		  */
    NULL,				/* actions		  */
    0,					/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* num_resources	  */
    NULLQUARK,				/* xrm_class		  */
    TRUE ,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    Destroy,				/* destroy		  */
    (XtWidgetProc) NULL,		/* resize		  */
    (XtExposeProc) NULL,		/* expose		  */
    (XtSetValuesFunc) SetValues,	/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    XtInheritAcceptFocus,		/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    XtInheritTranslations,		/* tm_table		  */
    (XtGeometryHandler) QueryGeometry,	/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {
    (XtGeometryHandler) GeometryManager,/* geometry_manager	*/
    (XtWidgetProc) ChangedManaged,	/* changed_managed	*/
    (XtWidgetProc) InsertChild,		/* insert_child		*/
    XtInheritDeleteChild,		/* delete_child		*/
    NULL,				/* extension		*/
  },  /* CompositeClass fields initialization */
  {
    /* resources	  */	(XtResourceList)NULL,
    /* num_resources	  */	0,
    /* constraint_size	  */	0,
    /* initialize	  */	(XtInitProc)NULL,
    /* destroy		  */	(XtWidgetProc)NULL,
    /* set_values	  */	(XtSetValuesFunc)NULL
  },	/* constraint_class fields */
  {
    /* highlight_handler  */	NULL,
    /* focus_on_select	*/	True,
    /* traversal_handler  */    (OlTraversalFunc)TraversalHandler,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* register_focus	  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ NULL, 0 },
    /* transparent_proc   */	XtInheritTransparentProc,
  },	/* manager_class fields   */
  {
    0,					/* not used now */
  }  /* ExclusivesClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass exclusivesWidgetClass = (WidgetClass) &exclusivesClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 ************************************************************
 *
 *  CalcColsRows - According to the layout type, this function
 *	calculates the number of columns and rows needed for
 *	the Exclusives widget.
 *
 *********************function*header************************
 */

static void CalcColsRows(ew,nbuttons,columnsptr,rowsptr) 
	int nbuttons,*columnsptr,*rowsptr;
	ExclusivesWidget ew;
{
	int columns,rows;
	OlDefine layout;

	if(nbuttons==0) {
		*columnsptr=0;
		*rowsptr=0;
		return;
	}

	layout= (int) ew->exclusives.layout;

	if(layout==(OlDefine)OL_FIXEDCOLS) {	/* calculate number rows */
		columns= ew->exclusives.measure;
		rows= nbuttons/columns;
		if(rows*columns<nbuttons) rows+=1;
	}
		
	if(layout==(OlDefine)OL_FIXEDROWS) {	/* calculate number columns */
		rows= ew->exclusives.measure;
		columns= nbuttons/rows;
		if(rows*columns<nbuttons) columns+=1;
	}
		
	*columnsptr=columns;
	*rowsptr=rows;

} /* CalcColsRows */

/*
 ************************************************************
 *
 *  CalcNewMax - This function recalculates the maximum 
 *	size button needed for all children.
 *
 *********************function*header************************
 */

static void CalcNewMax(ew,nbuttons) 
	ExclusivesWidget ew;
	int nbuttons;
{
	int i;
	RectButtonWidget bw;
	Widget child;
	Dimension max_height,max_width;

	if(nbuttons==0) {
		ew->exclusives.max_height=(Dimension)1;
		ew->exclusives.max_width=(Dimension)1;
		return;
	}

	max_height=(Dimension) 1;
	max_width=(Dimension) 1;

	for(i=0;i<nbuttons;i++) {
		child=ew->composite.children[i];
		bw= (RectButtonWidget) child;
		if(bw->button.normal_height>max_height) {
			max_height=bw->button.normal_height;
		}
		if(bw->button.normal_width>max_width) {
			max_width=bw->button.normal_width;
		}
	}

	ew->exclusives.max_height=max_height;/* store new values */
	ew->exclusives.max_width=max_width;

} /* CalcNewMax */

/*
 ************************************************************
 *
 *  GetDefaultButtons - This function finds the new (and
 *	possibly) old default buttons set.
 *
 *********************function*header************************
 */

static void GetDefaultButtons(ew,default1,default2) 
	ExclusivesWidget ew;
	Widget *default1,*default2;
{
	Widget button;
	RectButtonWidget bw;
	int i,nchildren;

	*default1=(Widget)0;
	*default2=(Widget)0;

	nchildren=ew->composite.num_children;
	if(nchildren==0) return;

	for(i=0;i<nchildren;i++) {
		button=ew->composite.children[i];
		bw= (RectButtonWidget) button;
		if(bw->button.is_default!=FALSE) {	
			if(*default1==(Widget)0) {
				*default1=button;
			}
			else {
				*default2=button;
			}
		}
	}
} /* GetDefaultButtons */

/*
 ************************************************************
 *
 *  GetSetButtons - This function finds the new and
 *	old set buttons set.
 *
 *********************function*header************************
 */

static void GetSetButtons(ew,set1,set2) 
	ExclusivesWidget ew;
	Widget *set1,*set2;
{
	Widget button;
	RectButtonWidget bw;
	int i,nchildren;

	*set1=(Widget)0;
	*set2=(Widget)0;

	nchildren=ew->composite.num_children;
	if(nchildren==0) return;

	for(i=0;i<nchildren;i++) {
		button=ew->composite.children[i];
		bw= (RectButtonWidget) button;
		if(bw->button.set!=FALSE) {	
			if(*set1==(Widget)0) {
				*set1=button;
			}
			else {
				*set2=button;
			}
		}
	}
} /* GetSetButtons */

/*
 ************************************************************
 *
 *  Layout - This function configures an Exclusives
 *	widget with either row or column layout.
 *
 *********************function*header************************
 */

static void Layout(ew)
	ExclusivesWidget ew;
{
	int i,j,nbuttons,nbuttons1,rows,columns,idx,iheight;
	OlDefine layout = ew->exclusives.layout;
	Widget child;
	RectButtonWidget bw;
	Position x,y,xstart,ystart,lastx;
	Dimension max_height,max_width,last_width;
	int	overlap;

	if(!(nbuttons=ew->composite.num_children)) {
		ew->exclusives.normal_height=(Dimension)1;
		ew->exclusives.normal_width=(Dimension)1;
		ew->exclusives.max_height=(Dimension)1;
		ew->exclusives.max_width=(Dimension)1;
		return;
	}
	
	CalcColsRows(ew,nbuttons,&columns,&rows);
	CalcNewMax(ew,nbuttons); 	/* used to get maximum height */
	max_height= ew->exclusives.max_height;
	iheight = (int) max_height;
	ew->exclusives.normal_width=(Dimension)0;/* we add to this per column */

	nbuttons1 = nbuttons -1;
	xstart=(Position)0;
	ystart=(Position)0;

	overlap = OlgIs3d () ? 0 : 1;	/* should be resolution dependent */
	for(i=0;i<columns;i++) {
		max_width=(Dimension)0;
		for(j=0;j<rows;j++) {	/* get maximum width */
			if(layout==(OlDefine) OL_FIXEDCOLS) {
				idx=j*columns+i;
			}
			else if(layout== (OlDefine) OL_FIXEDROWS) {
				idx=i*rows+j;
			}
			if(idx>nbuttons1) break;
			child=ew->composite.children[idx];
			bw= (RectButtonWidget) child;
			if(bw->button.normal_width>max_width) {
				max_width=bw->button.normal_width;
			}
		}

		if(i==0) {
			lastx = (Position)0;
			last_width=(Dimension)0;
			x = xstart;
		}
		else {
			x = lastx + (Position)last_width - overlap;
		}
			/* set maximum width for a column of buttons */

		for(j=0;j<rows;j++) {
			if(layout==(OlDefine) OL_FIXEDCOLS) {
				idx=j*columns+i;
			}
			else if(layout==(OlDefine) OL_FIXEDROWS) {
				idx=i*rows+j;
			}
			if(idx>nbuttons1) break;
			child=ew->composite.children[idx];
			y=(Position) ((int) ystart + (j*(iheight-overlap)));
			XtConfigureWidget(child,x,y,
				max_width,max_height,(Dimension)0);
		}

	ew->exclusives.normal_width+=max_width;
	last_width=max_width;
	lastx = x;

	} /* i loop */

	if(ew->exclusives.layout==(OlDefine)OL_FIXEDCOLS && nbuttons<columns) {
		columns=nbuttons;
	}
	ew->exclusives.normal_width-=((Dimension)(columns-1) * overlap); 

	if(ew->exclusives.layout==(OlDefine)OL_FIXEDROWS && nbuttons<rows) {
		rows=nbuttons;
	}
	ew->exclusives.normal_height=
			(Dimension)((int)(ew->exclusives.max_height) * rows 
					- (rows - 1) * overlap );

} /* Layout */

/*
 ************************************************************
 *
 *  ResizeSelf - This function calculates the size needed
 *	for the Exclusives widget and calls functions to 
 *	reconfigure the button children accordingly.
 *
 *********************function*header************************
 */

static void ResizeSelf(ew,greq) 
	ExclusivesWidget ew;
	Boolean greq;
{
	XtWidgetGeometry request,reply;
	Widget ewidget;

	Layout(ew);	/* layout columns or rows and calculate widget size */
	
	if(greq) {

	if(ew->exclusives.recompute_size!=FALSE) {
		request.height = ew->exclusives.normal_height;
		request.width = ew->exclusives.normal_width;
	}
	else 
		return;

	if(request.height==(Dimension)0) request.height=(Dimension)1;
	if(request.width==(Dimension)0) request.width=(Dimension)1;/* no 0x0 */

	request.request_mode = CWHeight | CWWidth ;

	ewidget = (Widget) ew;

	switch(XtMakeGeometryRequest(ewidget,&request,&reply)) {

		case XtGeometryDone:
			break;
		case XtGeometryYes:
			break;
		case XtGeometryAlmost:
			XtMakeGeometryRequest(ewidget,&reply,&reply);
			break;
		case XtGeometryNo:
			break;
	}
	}

} /* ResizeSelf */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 ************************************************************
 *
 *  GetFocusItems - utility function
 *
 *********************function*header************************
 */

static int
GetFocusItems(idx,nbuttons1,ncolumns,nrows,layout,
		row_wanted,focus_array,return_array,nitems)
int idx,nbuttons1,ncolumns,nrows,*focus_array,**return_array,*nitems;
Boolean row_wanted;
OlDefine layout;
{

int cs,ce,dim1,dim2,i,n,ni,current_index,*tmp_array;

if((layout==(OlDefine)OL_FIXEDROWS && !row_wanted) 
	|| (layout==(OlDefine)OL_FIXEDCOLS && row_wanted)) {
	if(layout==(OlDefine)OL_FIXEDROWS)
		dim1 = nrows;
	else
		dim1 = ncolumns;

	cs = idx - (idx % dim1);
	ce = cs + dim1 - 1;
	if(ce > nbuttons1)
		ce = nbuttons1;
	n = ce - cs + 1;

	if (n > ARRAY_SIZE) {
		tmp_array = (int *)XtMalloc(n * sizeof(int));
	} else {
		tmp_array = focus_array;
	}

	for(i=0;i<n;i++) {
		tmp_array[i] = cs + i;
		if(tmp_array[i]==idx)
			current_index = i;
	}
}
else if((layout==(OlDefine)OL_FIXEDROWS && row_wanted) 
	|| (layout==(OlDefine)OL_FIXEDCOLS && !row_wanted)) {

	if(layout==(OlDefine)OL_FIXEDROWS) {
		dim1 = nrows;
		dim2 = ncolumns;
	}
	else {
		dim1 = ncolumns;
		dim2 = nrows;
	}
	cs = idx % dim1;
	ce = cs + (dim2-1)*dim1;
	if(ce > nbuttons1)
		ce = ce - dim1;

	n = (ce - cs)/dim1 + 1;

	if (n > ARRAY_SIZE) {
		tmp_array = (int *)XtMalloc(n * sizeof(int));
	} else {
		tmp_array = focus_array;
	}

	for(i=0;i<n;i++) {
		tmp_array[i] = cs + i*dim1;
		if(tmp_array[i]==idx)
			current_index = i;
	}
}
	*nitems = n;
	*return_array = tmp_array;
	return current_index;

} /* GetFocusItems */

/*
 ************************************************************
 *
 *  NewIndex - utility function
 *
 *********************function*header************************
 */

static int
NewIndex(nitems,ofocus_index,n,plus)
	int nitems,ofocus_index,n;
	Boolean plus;
{
	int tmp_idx;

	if(plus) {
		tmp_idx = ofocus_index + n;
		if(tmp_idx > (nitems-1))
			tmp_idx = 0;
	}
	else	{	/* if !plus */
		tmp_idx = ofocus_index - n;
		if(tmp_idx < 0)
			tmp_idx = nitems - 1;
	}
	
	return tmp_idx;

} /* NewIndex */

/*
 ************************************************************
 *
 *  TraversalHandler - This function manages the traversal
 *	among the exclusive's button children.
 *
 *********************function*header************************
 */
static Widget 
TraversalHandler(traversal_manager,w,direction,time)
	Widget traversal_manager;
	Widget w;
	OlVirtualName direction;
	Time time;
{
	ExclusivesWidget ew = (ExclusivesWidget) traversal_manager;

	int k,nbuttons,nbuttons1,nrows,ncolumns,focus_index,idx,tmp_idx,
		focus_array[ARRAY_SIZE],*return_array,multi_count,nitems;
	Widget child, *children = ew->composite.children;
	Time	timestamp = time;

	if(!(nbuttons=ew->composite.num_children)) 
		return (Widget) NULL;

	if(w==(Widget)NULL) 
		return ew->composite.children[0];

	CalcColsRows(ew,nbuttons,&ncolumns,&nrows);

	nbuttons1 = nbuttons -1;

	for(k=0;k<nbuttons;k++) {
		child=children[k];
		idx=k;
		if(child==w)
			break;
	}

	if(direction==(OlVirtualName)OL_MULTIRIGHT 
		|| direction==(OlVirtualName)OL_MOVERIGHT
		|| direction==(OlVirtualName)OL_MULTILEFT
		|| direction==(OlVirtualName)OL_MOVELEFT)

	focus_index =GetFocusItems(idx,nbuttons1,ncolumns,nrows,
		ew->exclusives.layout,TRUE,focus_array,&return_array,&nitems);

	else if(direction==(OlVirtualName)OL_MULTIUP 
		|| direction==(OlVirtualName)OL_MOVEUP
		|| direction==(OlVirtualName)OL_MULTIDOWN
		|| direction==(OlVirtualName)OL_MOVEDOWN)

	focus_index = GetFocusItems(idx,nbuttons1,ncolumns,nrows,
		ew->exclusives.layout,FALSE,focus_array,&return_array,&nitems);

	switch(direction) {

		case OL_IMMEDIATE:
			for(k=0;k<nbuttons;k++) {
				tmp_idx = idx + k+1;
				if(tmp_idx>nbuttons1)
					tmp_idx = tmp_idx % nbuttons;
				child=children[idx];
				if(XtCallAcceptFocus(child, &timestamp)) 
					break;
			}
			break;

		case OL_MULTILEFT:
		case OL_MULTIUP:

		multi_count = (int)
			_OlGetMultiObjectCount(traversal_manager);

			focus_index = NewIndex(nitems,focus_index,
						multi_count,FALSE);

			child = children[return_array[focus_index]];
			if(XtCallAcceptFocus(child, &timestamp)) 
				break;

		case OL_MOVELEFT:
		case OL_MOVEUP:

			for(k=0;k<nitems;k++) {

			focus_index = NewIndex(nitems,focus_index,
						1,FALSE);

			child = children[return_array[focus_index]];
			if(XtCallAcceptFocus(child, &timestamp)) 
				break;
			}

			break;

		case OL_MULTIRIGHT:
		case OL_MULTIDOWN:

		multi_count = (int)
			_OlGetMultiObjectCount(traversal_manager);

			focus_index = NewIndex(nitems,focus_index,
						multi_count,TRUE);

			child = children[return_array[focus_index]];
			if(XtCallAcceptFocus(child, &timestamp)) 
				break;

		case OL_MOVERIGHT:
		case OL_MOVEDOWN:

			for(k=0;k<nitems;k++) {

			focus_index = NewIndex(nitems,focus_index,
						1,TRUE);

			child = children[return_array[focus_index]];
			if(XtCallAcceptFocus(child, &timestamp)) 
				break;
			}

			break;

		default:
			return NULL;
	}

	if (return_array != focus_array) {
		XtFree((char *)return_array);
	}

	return child;
}

/*
 ************************************************************
 *
 *  ChangedManaged - This function keeps all rectangular
 *	button children managed, to prevent chaos.
 *
 *********************function*header************************
 */

static void ChangedManaged(widget)
	Widget widget;
{
	ExclusivesWidget ew;
	RectButtonWidget bw;
	Widget child;
	int i,nchildren,breaktime=0;

	ew= (ExclusivesWidget) widget;
	nchildren=ew->composite.num_children;

	for(i=0;i<nchildren;i++) {
		child=ew->composite.children[i];
		bw= (RectButtonWidget) child;
		if(bw->core.managed==FALSE) {	/* only user would do this */
		XtManageChild(child);
		if(XtIsRealized(child)) XtMapWidget(child);
		breaktime=1;
		break;
		}
		if(breaktime) break;
	}

    ResizeSelf(ew,TRUE);
	
} /* ChangedManaged */

/*
 ************************************************************
 *
 *  ClassInitialize - Register OlDefine string values
 *
 *********************function*header************************
 */

static void ClassInitialize()
{
	_OlAddOlDefineType ("fixedcols", OL_FIXEDCOLS);
	_OlAddOlDefineType ("fixedrows", OL_FIXEDROWS);
}

/*
 ************************************************************
 *
 *  DeleteChild - This function  sees to it that the Exclusives
 *	maintains a correct configuration even if a set or 
 *	default child is destroyed.
 *
 *********************function*header************************
 */

static void
DeleteChild(widget, client_data, call_data)
	Widget		widget;
	XtPointer	client_data;
	XtPointer	call_data;
{
	ExclusivesWidget ew;
	Widget first_child,next_child;
	Arg arg[2];

	ew= (ExclusivesWidget) (XtParent(widget));

	if(ew->core.being_destroyed!=FALSE) {
		return;
	}

	if(ew->composite.num_children==0) {
		ew->exclusives.default_child= (Widget)0;
		ew->exclusives.is_default=FALSE;
		ew->exclusives.set_child= (Widget)0;
		return; 
	}
				/* maintain bookkeeping on default button */

	if(widget==ew->exclusives.default_child) {
		ew->exclusives.default_child=(Widget)0;
		ew->exclusives.is_default=FALSE;
	}
				/* maintain bookkeeping on set button */

	first_child=ew->composite.children[0];

			/* if XtNnoneSet==FALSE try to set another */

	if(widget==ew->exclusives.set_child && ew->exclusives.noneset==FALSE) {
		if(widget!=first_child) {
			XtSetArg(arg[0],XtNset,(XtArgVal)TRUE);
			XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
			XtSetValues(first_child,arg,2);
			ew->exclusives.set_child=first_child;
			ew->exclusives.looks_set=first_child;
		}
		else {				/* widget is first child */
			if(ew->composite.num_children>=2) {
				next_child=ew->composite.children[1];
				XtSetArg(arg[0],XtNset,(XtArgVal)TRUE);
				XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
				XtSetValues(next_child,arg,2);
				ew->exclusives.set_child=next_child;
				ew->exclusives.looks_set=next_child;
			}
		 	else {			/* no other child to set */
				ew->exclusives.set_child=(Widget)0;
				ew->exclusives.looks_set=(Widget)0;
			}
		}
	}
				/* XtNnoneSet==TRUE */
	else if(widget==ew->exclusives.set_child) {
				ew->exclusives.set_child=(Widget)0;
				ew->exclusives.looks_set=(Widget)0;
	}

	ResizeSelf(ew,TRUE);

} /* DeleteChild */

/*
 ************************************************************
 *
 *  Destroy - clean up Eventhandler attached
 *
 *********************function*header************************
 */

static void Destroy(widget)
    Widget widget;
{
	XtRemoveEventHandler(	widget,
				LeaveWindowMask,FALSE,LeaveHandler,
				(XtPointer)NULL);

}	/* Destroy */

/*
 ************************************************************
 *
 *  GeometryManager - This function is called when a button child
 *	wants to resize itself; the current policy is to allow
 *	the requested resizing.
 *
 *********************function*header************************
 */

static XtGeometryResult GeometryManager(widget,request,reply)
	Widget widget;			
	XtWidgetGeometry *request;
	XtWidgetGeometry *reply;
{
	ExclusivesWidget ew;
	RectButtonWidget bw;

	bw = (RectButtonWidget) widget;
	ew = (ExclusivesWidget) XtParent(widget);


	if(((request->request_mode & CWHeight) 
		&& request->height==(Dimension)0) 

		|| ((request->request_mode & CWWidth)
			&& request->width==(Dimension)0)) {

		OlVaDisplayWarningMsg(	XtDisplay(widget),
					OleNinvalidDimension,
					OleTbadGeometry,
					OleCOlToolkitWarning,
					OleMinvalidDimension_badGeometry,
					XtName(widget),
					OlWidgetToClassName(widget));
		return XtGeometryNo;
	}

	ResizeSelf(ew,TRUE);			/* takes care of children too */

	reply->request_mode=CWWidth | CWHeight;
	reply->height=bw->core.height;
	reply->width=bw->core.width;

	if(reply->height==request->height && reply->width==request->width) {
		return XtGeometryYes;
	}
	else return XtGeometryAlmost;
}

/*
 ************************************************************
 *
 *  Initialize - This function checks that Exclusives variable
 *	values are within range and initializes private fields
 *
 *********************function*header************************
 */

/* ARGSUSED */
static void
Initialize(req_w, new_w, args, num_args)
	ExclusivesWidget	req_w;
	ExclusivesWidget	new_w;
	ArgList			args;
	Cardinal *		num_args;
{
	ExclusivesWidget	request = (ExclusivesWidget)req_w;
	ExclusivesWidget	new = (ExclusivesWidget)new_w;
	Widget parent,w;
	CorePart *cp;
	ExclusivesPart *ep;
	ManagerPart	*mp;

	w = (Widget) new;
	cp = &(new->core);
	ep = &(new->exclusives);
	mp = &(new->manager);
	parent= XtParent(w);

/* Set relevant core fields */

	if(cp->height==(Dimension)0) cp->height=(Dimension) 1;		
	if(cp->width==(Dimension)0) cp->width=(Dimension) 1;		

	if(cp->border_width!=(Dimension)0) cp->border_width= (Dimension) 0;

	if (cp->background_pixel!=parent->core.background_pixel) {
		cp->background_pixel=parent->core.background_pixel;
	}

	if (cp->background_pixmap!=parent->core.background_pixmap) {
		cp->background_pixmap=parent->core.background_pixmap;
	}

/* Check/Set relevant exclusives fields */

	/* public resources */

	if(ep->layout!=(OlDefine)OL_FIXEDCOLS 
			&& ep->layout!=(OlDefine)OL_FIXEDROWS) {

		ep->layout=(OlDefine)OL_FIXEDROWS;
		OlVaDisplayWarningMsg(	XtDisplay((Widget)new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName((Widget)new),
					OlWidgetToClassName((Widget)new),
					XtNlayout,
					"OL_FIXEDROWS");
	}

	if(ep->measure<1) {
		ep->measure=1;
		OlVaDisplayWarningMsg(	XtDisplay((Widget)new),
					OleNinvalidResource,
					OleTinitializeDefault,
					OleCOlToolkitWarning,
					OleMinvalidResource_initializeDefault,
					XtName((Widget)new),
					OlWidgetToClassName((Widget)new),
					XtNlabelJustify);
	}

	if(ep->noneset!=FALSE) {
		ep->noneset=TRUE;
	}

	/* private resources */

	if(ep->is_default!=FALSE) {

		ep->is_default=FALSE;
		OlVaDisplayWarningMsg(	XtDisplay((Widget)new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName((Widget)new),
					OlWidgetToClassName((Widget)new),
					XtNdefault,
					"FALSE");
	}

	if(ep->reset_set!=defNEITHER)
		ep->reset_set=defNEITHER;

	if(ep->reset_default!=defNEITHER)
		ep->reset_default=defNEITHER;

	if(		ep->shell_behavior!=OtherBehavior
		&&	ep->shell_behavior!=BaseWindow
		&&	ep->shell_behavior!=PopupWindow
		&&	ep->shell_behavior!=PinnedWindow
		&&	ep->shell_behavior!=PinnedMenu
		&&	ep->shell_behavior!=PressDragReleaseMenu
		&&	ep->shell_behavior!=StayUpMenu
		&&	ep->shell_behavior!=UnpinnedMenu ) {

		new->exclusives.shell_behavior=OtherBehavior;
		OlVaDisplayWarningMsg(	XtDisplay((Widget)new),
					OleNinvalidResource,
					OleTinitialize,
					OleCOlToolkitWarning,
					OleMinvalidResource_initialize,
					XtName((Widget)new),
					OlWidgetToClassName((Widget)new),
					XtNshellBehavior,
					"OtherBehavior");
	}

	/* private fields */

    	ep->default_child =		(Widget) 0;
    	ep->set_child =			(Widget) 0;
    	ep->looks_set =			(Widget) 0;
    	ep->usr_set =			0;
    	ep->max_height =		(Dimension)1;
    	ep->max_width =			(Dimension)1;
    	ep->normal_height =		(Dimension)1;
    	ep->normal_width =		(Dimension)1;

		/* add me to the traversal list */
	_OlUpdateTraversalWidget(w, mp->reference_name,
				 mp->reference_widget, True);
} 	/* Initialize */

/*
 ************************************************************
 *
 *  InsertChild - This function checks that all children are
 *	rectangular buttons, that there is no more than one
 *	set or default button, adds the event handler for
 *	the exclusives, and sees that the Exclusives widget
 *	is reconfigured accordingly. 
 *
 *********************function*header************************
 */

static void
InsertChild(widget)
	Widget widget;
{
    Widget		parent = XtParent(widget);
    ExclusivesWidget	ew = (ExclusivesWidget) parent;
    RectButtonWidget	bw = (RectButtonWidget) widget;
    XtWidgetProc	insert_child = ((CompositeWidgetClass)
      (exclusivesClassRec.core_class.superclass))->composite_class.insert_child;
    Widget			first_child;
    Arg arg[2];

    /* check that a rectangular button */

    if(XtIsSubclass(widget, rectButtonWidgetClass)==FALSE) {

	OlVaDisplayWarningMsg(	XtDisplay(widget),
				OleNgoodParent,
				OleTbadChild,
				OleCOlToolkitWarning,
				OleMgoodParent_badChild,
				XtName(widget),
				OlWidgetToClassName(widget),
				OlWidgetToClassName(parent));

        XtDestroyWidget(widget);
	return;
    }
	
	if (!insert_child)

		OlVaDisplayErrorMsg(	XtDisplay(widget),
					OleNbadFunction,
					OleTinsertChild,
					OleCOlToolkitError,
					OleMbadFunction_insertChild,
					XtName(widget),
					OlWidgetToClassName(widget));
	(*insert_child)(widget);

	XtManageChild(widget);
	XtSetMappedWhenManaged(widget,TRUE); /* default but do it anyhow */

	first_child=ew->composite.children[0];
					
	if(widget==first_child) {	/* set if user desires */
	if(bw->button.set!=FALSE) {
	    ew->exclusives.usr_set=1;
	    ew->exclusives.set_child=widget;
	    ew->exclusives.looks_set=widget;
	}
	else if(bw->button.set==FALSE /* make set as default */
		&& ew->exclusives.noneset==FALSE) {
	    XtSetArg(arg[0],XtNset,(XtArgVal)TRUE);
	    XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
	    XtSetValues(widget,arg,2);
	    ew->exclusives.set_child=widget;
	    ew->exclusives.looks_set=widget;
	}
    }
    else {			/* widget !=first child */
	if(bw->button.set!=FALSE) {
	    if(ew->exclusives.usr_set==0) {
		/* unset first set as default */
		if(ew->exclusives.noneset==FALSE) {
		    XtSetArg(arg[0],XtNset,(XtArgVal)FALSE);
		    XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
		    XtSetValues(first_child,arg,2);
		}
		ew->exclusives.usr_set=1;
		ew->exclusives.set_child=widget;
		ew->exclusives.looks_set=widget;
	    }
	    else {
		XtSetArg(arg[0],XtNset,(XtArgVal)FALSE);
		XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
		XtSetValues(widget,arg,2);
		OlVaDisplayWarningMsg(	XtDisplay(widget),
					OleNfileExclusives,
					OleTmsg1,
					OleCOlToolkitWarning,
					OleMfileExclusives_msg1,
					XtName(widget),
					OlWidgetToClassName(widget));
	    }
	}
    }

    if(bw->button.is_default!=FALSE) { 
	if(ew->exclusives.default_child==(Widget)0) {
	    ew->exclusives.default_child=widget; /* record it */
	    ew->exclusives.is_default=TRUE;
	}
    }

    XtAddCallback(widget,XtNdestroyCallback,DeleteChild,parent);

    XtSetSensitive(widget,ew->core.sensitive);
    XtSetArg(arg[0],XtNlabelJustify,(XtArgVal) OL_LEFT);
    XtSetValues(widget,arg,1);

    _OlDeleteDescendant(widget);	/* remove button from traversal list */


}				/* InsertChild */

/*
 ************************************************************
 *
 *  QueryGeometry - This function is a rather rigid prototype
 *	since the widget insists upon its core height and width.
 *
 ************************************************************
*/

static XtGeometryResult QueryGeometry(widget,request,preferred) 
	Widget widget;
	XtWidgetGeometry *request;
	XtWidgetGeometry *preferred;
{
	ExclusivesWidget ew;

	ew= (ExclusivesWidget) widget;

		/* if not height or width it is okay with widget */

	if(!(request->request_mode & CWHeight 
			|| request->request_mode &CWWidth)) {

		return XtGeometryYes;
	}

		/* if still here look at requested height and/or width */

	if((request->request_mode & CWHeight)
		&& (request->height!=ew->exclusives.normal_height)) {

		preferred->request_mode |=CWHeight;
		preferred->height=ew->exclusives.normal_height;
	};
		
	if((request->request_mode & CWWidth)
		&& (request->width!=ew->exclusives.normal_width)) {

		preferred->request_mode |=CWWidth;
		preferred->width=ew->exclusives.normal_width;
	};

	if((preferred->request_mode & CWHeight)
		|| (preferred->request_mode & CWWidth)) {

		return XtGeometryAlmost;
	}

	else return XtGeometryNo;

} /* QueryGeometry */

/*
 ************************************************************
 *
 *  Realize - This function realizes the Exclusives widget in
 *	order to be able to set window properties
 *
 *********************function*header************************
 */

static void Realize(widget,ValueMask,attributes) 
	Widget widget;
	XtValueMask *ValueMask;
	XSetWindowAttributes *attributes;
{
	ExclusivesWidget ew = (ExclusivesWidget) widget;

	*ValueMask|=CWBitGravity;
	attributes->bit_gravity=NorthWestGravity;

	attributes->do_not_propagate_mask|= LeaveWindowMask;

	XtCreateWindow(	widget,(unsigned int)InputOutput,
			(Visual *)CopyFromParent, *ValueMask, attributes);
	
	XtAddEventHandler(widget,LeaveWindowMask,FALSE,LeaveHandler,NULL);

} /* Realize */

/*
 ************************************************************
 *
 *  SetValues - This function checks and allows setting and
 *	resetting of Exclusives resources.
 *
 *********************function*header************************
 */

/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
	Widget		current;
	Widget		request;
	Widget		new;
	ArgList		args;
	Cardinal *	num_args;
{
	int newmeasure;
	OlDefine newlayout;
	Boolean needs_redisplay=FALSE,Grequest=FALSE;
	Widget button,widget,default1,default2,set1,set2,parent;
	Arg arg[2];
	XSetWindowAttributes attributes;

	ExclusivesWidget currew =	(ExclusivesWidget) current;
	ExclusivesWidget newew =	(ExclusivesWidget) new;

	widget = (Widget) current;
	parent=XtParent(widget);

	/* ******************************************* */
	/* check that core values correct or not reset */
	/* ******************************************* */

	if (newew->core.height==(Dimension)0) {
		newew->core.height=(Dimension)1;
	}

	if (newew->core.width==(Dimension)0) {
		newew->core.width=(Dimension)1;
	}

	if (newew->core.border_width!=(Dimension)0) {
		newew->core.border_width=(Dimension)0;
	}

	/* XtNsensitive resource */

	if(XtIsSensitive(new)!=XtIsSensitive(current)) {
			needs_redisplay=TRUE;
	}

	if (newew->core.background_pixel!=parent->core.background_pixel) {
		newew->core.background_pixel=parent->core.background_pixel;
		if (XtIsRealized((Widget)newew)) {
		attributes.background_pixel=parent->core.background_pixel;
		XChangeWindowAttributes(XtDisplay(new),XtWindow(new),
			CWBackPixel,&attributes);
		needs_redisplay=TRUE;
		}
	}

	if (newew->core.background_pixmap!=parent->core.background_pixmap) {
		newew->core.background_pixmap=parent->core.background_pixmap;
		if((newew->core.background_pixmap!=XtUnspecifiedPixmap) &&
		   XtIsRealized(new)) {

		attributes.background_pixmap=newew->core.background_pixmap;
		XChangeWindowAttributes(XtDisplay(new),XtWindow(new),
			CWBackPixmap,&attributes);
		}
		needs_redisplay=TRUE;
	}

	/* ********************************** */
	/* check exclusives private resources */
	/* ********************************** */

	/* XtNresetSet : used to signal user resetting of set button */

	if(newew->exclusives.reset_set==TRUE) {


	newew->exclusives.reset_set=defNEITHER;		/* reset */

	GetSetButtons(newew,&set1,&set2); 

					/* error --- should not be possible */
	if(set1==(Widget)0 && set2==(Widget)0 
			&& newew->exclusives.noneset==FALSE ) {		
		if(newew->composite.num_children!=0) {
			XtSetArg(arg[0],XtNset,(XtArgVal) TRUE);
			XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
			XtSetValues(newew->composite.children[0],arg,2);	
			newew->exclusives.looks_set=
			newew->exclusives.set_child=
					newew->composite.children[0];
		}
		OlVaDisplayWarningMsg(	XtDisplay(widget),
					OleNfileExclusives,
					OleTmsg3,
					OleCOlToolkitWarning,
					OleMfileExclusives_msg3,
					XtName(new),
					OlWidgetToClassName(new));
	} 					/* user reset same one */
	else if(set1!=(Widget)0 && set2==(Widget)0) {
		newew->exclusives.looks_set=
		newew->exclusives.set_child=set1;
	}					/* one new & one old */
	else if(set1!=(Widget)0 && set2!=(Widget)0) {
					/* set1 old && set2 new */
		if(set1==newew->exclusives.set_child) {
			XtSetArg(arg[0],XtNset,(XtArgVal) FALSE);
			XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
			XtSetValues(set1,arg,2);	/* unset old */
			newew->exclusives.looks_set=
			newew->exclusives.set_child=set2;
		}
		else {			/* set2 old && set1 new */
			XtSetArg(arg[0],XtNset,(XtArgVal) FALSE);
			XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
			XtSetValues(set2,arg,2);	/* unset old */
			newew->exclusives.looks_set=
			newew->exclusives.set_child=set1;
		}
	}
	} /* TRUE */

	else if(newew->exclusives.reset_set==FALSE) {

	newew->exclusives.reset_set=defNEITHER;	/* reset */

	GetSetButtons(currew,&set1,&set2); 
						/* none set */
	if(set1==(Widget)0 && set2==(Widget)0 
				&& newew->exclusives.noneset==FALSE) {		
		if(newew->composite.num_children!=0) {
			XtSetArg(arg[0],XtNset,(XtArgVal) TRUE);
			XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
			XtSetValues(newew->composite.children[0],arg,2);	
			newew->exclusives.looks_set=
			newew->exclusives.set_child=
					newew->composite.children[0];
		}
		OlVaDisplayWarningMsg(	XtDisplay(widget),
					OleNfileExclusives,
					OleTmsg3,
					OleCOlToolkitWarning,
					OleMfileExclusives_msg3,
					XtName(new),
					OlWidgetToClassName(new));
	}
	else if(set1==(Widget)0 && set2==(Widget)0 
				&& newew->exclusives.noneset==TRUE) {		
		newew->exclusives.looks_set=
		newew->exclusives.set_child=
					(Widget)0;

	}

	} /* FALSE */

	/* XtNdefault: used by menu on exclusives as follows */

	if (newew->exclusives.is_default!=currew->exclusives.is_default) {

		if(currew->exclusives.is_default==FALSE 
				&& newew->exclusives.is_default!=FALSE) {

			if(newew->exclusives.default_child==(Widget)0) {
				if(newew->composite.num_children!=0) {

		XtSetArg(arg[0],XtNdefault,(XtArgVal) TRUE);
		XtSetValues(newew->composite.children[0],arg,1);
		newew->exclusives.default_child=newew->composite.children[0];

				}
				else {

				newew->exclusives.is_default=FALSE;

				OlVaDisplayWarningMsg(	XtDisplay(widget),
							OleNfileExclusives,
							OleTmsg4,
							OleCOlToolkitWarning,
							OleMfileExclusives_msg4,
							XtName(new),
							OlWidgetToClassName(new));
				}
			}
		}
		else if(currew->exclusives.is_default!=FALSE 
				&& newew->exclusives.is_default==FALSE) {
					
			GetDefaultButtons(newew,&default1,&default2); 
			if(default1!=(Widget)0) {

		XtSetArg(arg[0],XtNdefault,(XtArgVal) FALSE);
		XtSetValues(default1,arg,1);
		newew->exclusives.default_child=(Widget)0;

			}
		}
	} /* XtNdefault */	

	/* XtNresetDefault : used to signal setting or unsetting of default */

	if(newew->exclusives.reset_default==TRUE) {

	newew->exclusives.reset_default=defNEITHER;	/* reset */
/*
	needs_redisplay=TRUE;
*/
	GetDefaultButtons(newew,&default1,&default2); 
						/* error condition only */
	if(default1==(Widget)0 && default2==(Widget)0) {		
		newew->exclusives.is_default=FALSE; /* signal that no default*/
	} 					/* reset same one */
	else if(default1!=(Widget)0 && default2==(Widget)0) {
		newew->exclusives.is_default=TRUE; /* signal that a default*/
		newew->exclusives.default_child=default1;
	}					/* one new & one old */
	else if(default1!=(Widget)0 && default2!=(Widget)0) {
					/* default1 old && default2 new */
		if(default1==newew->exclusives.default_child) {
			XtSetArg(arg[0],XtNdefault,(XtArgVal) FALSE);
			XtSetValues(default1,arg,1);	/* unset old */
			newew->exclusives.default_child=default2;
		}
		else {			/* default2 old && default1 new */
			XtSetArg(arg[0],XtNdefault,(XtArgVal) FALSE);
			XtSetValues(default2,arg,1);	/* unset old */
			newew->exclusives.default_child=default1;
		}
	}
	} /* TRUE */

	else if(newew->exclusives.reset_default==FALSE) {

	newew->exclusives.reset_default=defNEITHER;	/* reset */
/*
	needs_redisplay=TRUE;
*/
	GetDefaultButtons(newew,&default1,&default2); 
						/* no more default */
	if(default1==(Widget)0 && default2==(Widget)0) {		
		newew->exclusives.default_child=(Widget)0;
		newew->exclusives.is_default=FALSE;	/* signal no default */
	}
	} /* FALSE */

	/* XtNdefaultData resource */

		/* let widget writer set and get any pointer needed 
		   for updating "cloned" menus - intrinsics does work here */

	/* XtNpreview : pass to Button for display */

	widget=newew->exclusives.preview;
	if(widget!=(Widget)0) {
		GetDefaultButtons(newew,&default1,&default2);
		if(default1!=(Widget)0) {
			XtSetArg(arg[0],XtNpreview,(XtArgVal) widget);
			XtSetValues(default1,arg,1);
		}
		else {
			OlVaDisplayWarningMsg(	XtDisplay(widget),
						OleNfileExclusives,
						OleTmsg5,
						OleCOlToolkitWarning,
						OleMfileExclusives_msg5,
						XtName(new),
						OlWidgetToClassName(new));
		}
		newew->exclusives.preview=(Widget)0;   /* reset */
	}
						
	/* XtNshellBehavior */

	if(newew->exclusives.shell_behavior
			!=currew->exclusives.shell_behavior) {

	if(		newew->exclusives.shell_behavior!=OtherBehavior
		&&	newew->exclusives.shell_behavior!=BaseWindow
		&&	newew->exclusives.shell_behavior!=PopupWindow
		&&	newew->exclusives.shell_behavior!=PinnedWindow
		&&	newew->exclusives.shell_behavior!=PinnedMenu
		&&	newew->exclusives.shell_behavior!=PressDragReleaseMenu
		&&	newew->exclusives.shell_behavior!=StayUpMenu
		&&	newew->exclusives.shell_behavior!=UnpinnedMenu ) {

		newew->exclusives.shell_behavior=
			currew->exclusives.shell_behavior;

		OlVaDisplayWarningMsg(	XtDisplay(new),
					OleNinvalidResource,
					OleTsetValuesNC,
					OleCOlToolkitWarning,
					OleMinvalidResource_setValuesNC,
					XtName(new),
					OlWidgetToClassName(new),
					XtNshellBehavior);
	}
	}

	/* ********************************** */
	/* check exclusives public resources */
	/* ********************************** */

	/* XtNnoneSet : set from FALSE->TRUE or TRUE->FALSE */

	if(newew->exclusives.noneset!=currew->exclusives.noneset) {
		if(newew->exclusives.noneset==FALSE
			&& currew->exclusives.noneset!=FALSE) {

		if(newew->exclusives.set_child==(Widget)0) { /* set one */
			if(newew->composite.num_children!=0) {
			XtSetArg(arg[0],XtNset,(XtArgVal)TRUE);
			XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
			XtSetValues(newew->composite.children[0],arg,2);
			newew->exclusives.looks_set=
			newew->exclusives.set_child=
					newew->composite.children[0];
			}
		}
		}
		
	} /* XtNnoneSet */

	/* XtNlayoutType : change layout configuration of buttons */

	newlayout=newew->exclusives.layout;

	if(currew->exclusives.layout!=newlayout) {
	  					/* bad request */
	if(newlayout!=(OlDefine)OL_FIXEDCOLS 
		&& newlayout!=(OlDefine)OL_FIXEDROWS) {
		newew->exclusives.layout=currew->exclusives.layout;

		OlVaDisplayWarningMsg(	XtDisplay(new),
					OleNinvalidResource,
					OleTsetValuesNC,
					OleCOlToolkitWarning,
					OleMinvalidResource_setValuesNC,
					XtName(new),
					OlWidgetToClassName(new),
					XtNlayout);
	}
					/* change in layout */
	else {
		Grequest=TRUE;
	}
	} /* XtNlayoutType */

	/* XtNmeasure : change measure configuration of buttons */

	newmeasure=newew->exclusives.measure;

	if(currew->exclusives.measure!=newmeasure) {
	  					/* bad request */
	if(newmeasure<1) {
		newew->exclusives.measure=currew->exclusives.measure;
		OlVaDisplayWarningMsg(	XtDisplay(new),
					OleNinvalidResource,
					OleTsetValuesNC,
					OleCOlToolkitWarning,
					OleMinvalidResource_setValuesNC,
					XtName(new),
					OlWidgetToClassName(new),
					XtNmeasure);
	}
					/* change in measure */
	else {
		Grequest=TRUE;
	}
	} /* XtNmeasure */

	/* XtNrecomputeSize */

	if(newew->exclusives.recompute_size
				!=currew->exclusives.recompute_size) { 
		Grequest=TRUE;
		needs_redisplay=TRUE;
	}

	/* do not make geometry request - intrinsics will request */

	if(Grequest) {
		ResizeSelf(newew,FALSE); 
		newew->core.width=newew->exclusives.normal_width;
		newew->core.height=newew->exclusives.normal_height;
	}

	return needs_redisplay;

}	/* SetValues */

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
