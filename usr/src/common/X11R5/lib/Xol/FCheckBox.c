#ifndef	NOIDENT
#ident	"@(#)flat:FCheckBox.c	1.14.1.26"
#endif

/******************************file*header********************************

    Description:
	This file contains the source code for the flat checkbox container.
*/
						/* #includes go here	*/
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/FCheckBoxP.h>

/**************************forward*declarations***************************

	Forward Procedure definitions listed by category:
		1. Private Procedures
		2. Class   Procedures
		3. Action  Procedures
		4. Public  Procedures 
*/
					/* private procedures		*/
/* There are no private procedures */

					/* class procedures		*/
static void	ClassInitialize OL_NO_ARGS();

					/* action procedures		*/
/* There are no action procedures */

					/* public procedures		*/
/* There are no public procedures */

/*****************************file*variables******************************

    Define global/static variables and #defines, and
    Declare externally referenced variables

*/

/***********************widget*translations*actions***********************

    Define Translations and Actions

*/

/****************************widget*resources*****************************

    Define Resource list associated with the Widget Instance

*/

#define OFFSET(f)	XtOffsetOf(FlatCheckBoxRec, f)

static XtResource resources[] = {
	/* Declare resources to override superclasses' values		*/
	/* Note that XtNexclusives and XtNnoneSet are False by default	*/
	/*	in flatButtons...					*/
  {
	XtNbuttonType, XtCButtonType, XtROlDefine, sizeof(OlDefine),
	OFFSET(buttons.button_type), XtRImmediate, (XtPointer)OL_CHECKBOX
  },
};
#undef OFFSET

/***************************widget*class*record***************************

    Define Class Record structure to be initialized at Compile time

*/

FlatCheckBoxClassRec
flatCheckBoxClassRec = {
    {
	(WidgetClass)&flatButtonsClassRec,	/* superclass		*/
	"FlatCheckBox",				/* class_name		*/
	sizeof(FlatCheckBoxRec),		/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	0,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	NULL,					/* destroy		*/
	XtInheritResize,			/* resize		*/
	XtInheritExpose,			/* expose		*/
	NULL,					/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_offsets	*/
	XtInheritTranslations,			/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
        False,					/* focus_on_select	*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	XtInheritTraversalHandler,		/* traversal_handler	*/
	NULL,					/* register_focus	*/
	XtInheritActivateFunc,			/* activate		*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	OlVersion,				/* version		*/
	(XtPointer)NULL,			/* extension		*/
	{
		(_OlDynResourceList)NULL,	/* resources		*/
		(Cardinal)0			/* num_resources	*/
	},					/* dyn_data		*/
	XtInheritTransparentProc		/* transparent_proc	*/
    },	/* End of Primitive Class Part Initialization */
    {
	(XtPointer)NULL,			/* extension		*/
	True,					/* transparent_bg	*/
	XtOffsetOf(FlatCheckBoxRec, default_item),/* default_offset*/
	sizeof(FlatCheckBoxItemRec)		/* rec_size		*/

		/*
		 * See ClassInitialize for procedures
		 */

    }, /* End of Flat Class Part Initialization */
    {
	NULL					/* no fields		*/
    }, /* End of FlatRowColumn Class Part Initialization */
    {
	True					/* allow_class_default	*/
    }, /* End of FlatButtons Class Part Initialization */
};

/*************************public*class*definition*************************

    Public Widget Class Definition of the Widget Class Record
*/

WidgetClass flatCheckBoxWidgetClass = (WidgetClass) &flatCheckBoxClassRec;

/****************************procedure*header*****************************

    ClassInitialize - this procedure inherits all superclass procedures.
*/
static void
ClassInitialize OL_NO_ARGS()
{
    /* Inherit all superclass procedures.  This scheme saves us from
       worrying about putting function pointers in the wrong class slot if
       they were statically declared.  It also allows us to inherit new
       functions simply be recompiling, i.e., we don't have to stick
       XtInheritBlah into the class slot.
    */
	OlFlatInheritAll(flatCheckBoxWidgetClass);

} /* END OF ClassInitialize() */
