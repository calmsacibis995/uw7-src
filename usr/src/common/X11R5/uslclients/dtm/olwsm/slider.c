#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/slider.c	1.8"
#endif

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>
#include <Xm/Scale.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>

#include <misc.h>
#include <slider.h>

static void	SliderMovedCB (
	Widget w, XtPointer client_data, XtPointer call_data
);

/**
 ** CreateSlider()
 **/

Widget
CreateSlider (
	Widget			parent,
	Slider *		slider,
	Boolean			track_changes,
	void			(*change)()
	)
{
	Screen *		screen	= XtScreenOfObject(parent);
	int			dist;
	Widget			label = NULL;

	if (slider->caption) 
	  {
		label = (Widget) CreateCaption(
			(String) slider->name,
			(String) slider->string,
			parent
		);
		parent = XtParent(label);
	  }

	/* 2 horz inches */

	dist = ScreenMMToPixel(XmHORIZONTAL, 75, screen);
	
	slider->w = XtVaCreateManagedWidget(
		slider->name,
		xmScaleWidgetClass,
		parent,
		XmNorientation,		XmHORIZONTAL,
		XmNwidth,		dist,
		XmNvalue,		slider->slider_value,
		XmNminimum,		slider->slider_min,
		XmNmaximum,		slider->slider_max,
		XmNscaleMultiple,	slider->granularity,
		XmNshowValue,		True,
		NULL);
	
	/* Mouse only sliders with changebars */
	slider->changebar = (ChangeBar *) calloc(1, sizeof(ChangeBar));
	CreateChangeBar(label, slider->changebar);
	slider->change = change;
	if(slider->caption)
	  {
	  	AddToCaption(slider->w, label);
	 }
	
	/*
	 * (1) Make the internal sensitivity-flag agree with the actual
	 * sensitivity. This covers the case where the sensitivity is
	 * set from a resource file.
	 * (2) Make sure the widget sensitivity agrees with the internal
	 * flag. This covers the case where the client has programmatic-
	 * ally determined that the slider can't work.
	 * (3) If an insensitive slider is captioned, make the caption
	 * insensitive, too.
	 */
	if (!XtIsSensitive(slider->w))
	  {
		slider->sensitive = False;
	  }
	else if (!slider->sensitive)
	  {
		XtSetSensitive (slider->w, False);
	  }
	if (!slider->sensitive && slider->caption)
	  {
		XtSetSensitive (parent, False);
	  }

	/*
	 * Ask the slider widget for the min/max values, as we set
	 * them from the resource file.
	 */
	
	
	XtVaGetValues (
		slider->w,
		XmNminimum, &(slider->slider_min),
		XmNmaximum, &(slider->slider_max),
		NULL);
	

	/*
	 * Now set the value.
	 */
	if (slider->slider_value < slider->slider_min)
	  {
		slider->slider_value = slider->slider_min;
	  }
	if (slider->slider_value > slider->slider_max)
          {
		slider->slider_value = slider->slider_max;
	  }
	XtVaSetValues (
		slider->w,
		XmNvalue, slider->slider_value,
		NULL);

	XtAddCallback (slider->w, XmNvalueChangedCallback, SliderMovedCB, (XtPointer)slider);
	XtAddCallback (slider->w, XmNdragCallback, SliderMovedCB, (XtPointer)slider);
	slider->track_changes = track_changes;

	return label;
} /* CreateSlider */

/**
 ** SetSlider()
 **/

void
SetSlider (
	Slider *		slider,
	int			value,
	int			change_state
	)
{
	if (slider->slider_value < slider->slider_min)
	  {
		slider->slider_value = slider->slider_min;
	  }
	if (slider->slider_value > slider->slider_max)
	  {
		slider->slider_value = slider->slider_max;
	  }
	if (slider->slider_value != value) 
	  {
		XtVaSetValues (
			slider->w,
			XmNvalue,	value,
			NULL
		);
		if (slider->track_changes && slider->changebar)
		  {
			SetChangeBarState (slider->changebar, 0,
				change_state, True,  slider->change);
		  }
	  } 
	else
	  {
		if (slider->track_changes && slider->changebar && change_state == WSM_NONE)
	          {
			SetChangeBarState (slider->changebar, 0,
				WSM_NONE, True, slider->change);
		  }
	  }

	slider->slider_value = value;
	return;
} /* SetSlider */

/**
 ** SliderMovedCB()
 **/

static void
SliderMovedCB (Widget w, XtPointer client_data, XtPointer call_data)
{
	Slider *			slider	= (Slider *)client_data;
	XmScrollBarCallbackStruct *	scrollbar = (XmScrollBarCallbackStruct *)call_data;


	slider->slider_value = scrollbar->value;
	if (slider->track_changes && slider->changebar )
	  {
		SetChangeBarState (slider->changebar, 0,
				WSM_NORMAL, True, slider->change);
	  }
	if (slider->f)
	  {
		(*slider->f) (slider, slider->closure, False); /* cd->more_cb_pending to False */
	  }

	return;
} /* SliderMovedCB */
