/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/port.c	1.14.1.10"
#endif

/* ############################  Port Number  ############################ */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"


extern void 		DeviceFocusChangeCB();
static DeviceItems *	FindItem();
extern Widget		CreateCaption();
extern void		Warning();


#define NR_ENABLED (sizeof(enabled) / sizeof(DeviceItems))
DeviceItems	enabled[] = {
	{ label_Enabled, "enabled", True },
	{ label_Disabled, "disabled", True },
};

DeviceItems	directions[] = {
	{ label_biDirectional, "bidirectional", True },
	{ label_outgoingOnly, "outgoing", True },
	{ label_incomingOnly, "incoming" , True},
};


DeviceItems	ports[] = {
	{ label_com1, "com1" , True},
	{ label_com2, "com2" , True},
	{ label_other, "Other", True },
};
/*
 * itemFields[] -  array of choice item resource names
 */
static String  itemFields[] = {
        XtNlabel,
	XtNclientData,
	XtNsensitive,
};

#define NR_ITEM_FIELDS  (sizeof(itemFields) / sizeof(itemFields[0]))
#define NR_DIRECTIONS	(sizeof(directions) / sizeof(DeviceItems))
#define NR_PORTS	(sizeof(ports) / sizeof(DeviceItems))

caddr_t
PortNumber(message, param)
int	message;
caddr_t	param;
{
	int			i;
	static DeviceItems	*current = &ports[0];
	static Boolean 		manage_extra = True;
	static Boolean		first_time = True;

	if (first_time) {
		first_time = False;

		for (i = 0; i < NR_PORTS; i++)
			ports[i].label = GGT(ports[i].label);
	}


	switch (message) {
	case NEW: {
		/*
		 * NEW: create the object from scratch
		 *
		 *	INPUT: param == parent widget
		 *	RETURN: parent's child (highest level created here)
		 */
		 
		Widget			w_parent = (Widget) param;
		Widget			w_control;
		Widget			CreateCaption();
		void			CBselect(), CBunselect();

		w_control = XtVaCreateManagedWidget(
				"control",
				controlAreaWidgetClass,
				w_parent,
				XtNlayoutType,	(XtArgVal) OL_FIXEDROWS,
				XtNcenter,	(XtArgVal) FALSE,
				XtNalignCaptions,	(XtArgVal) TRUE,
				XtNshadowThickness,	(XtArgVal) 0,
				(String)0
			);
		/*
		 * Add the exclusives area as the caption's child
		 */
		df->w_port = 
			XtVaCreateManagedWidget(
				"ports",
				flatButtonsWidgetClass,
				CreateCaption(w_control, GGT (label_port)),
				XtNlayoutType,	        OL_FIXEDROWS,
				XtNmeasure,	        1,
				XtNitems,               ports,
				XtNnumItems,            NR_PORTS,
				XtNitemFields,          itemFields,
				XtNnumItemFields,       NR_ITEM_FIELDS,
				XtNlabelJustify,        OL_LEFT,
				XtNselectProc,          CBselect,
				XtNunselectProc,        CBunselect,
				XtNbuttonType,		OL_RECT_BTN,
				XtNexclusives,		TRUE,
				0
			);

		df->w_otherPort = (Widget)XtVaCreateWidget(
				"extra",
				textFieldWidgetClass,
				w_control,
				XtNcharsVisible, DEVICE_SIZE,
				XtNmaximumSize, DEVICE_SIZE,
				XtNtraversalOn,	True,
				XtNstring, holdData.portNumber,

				(String)0
			);
		XtVaSetValues((Widget) df->w_otherPort, 
				XtNregisterFocusFunc,
				DeviceFocusChangeCB,
				0);
		return((caddr_t) df->w_port);
	}

	case SET: {
		/*
		 * SET: set the value of the object.  NOTE that this is
		 * a forced set by our parent, the popup window object,
		 * as opposed to a direct use selection.
		 *
		 *	INPUT: param == value to set
		 *	RETURN: value actually set
		 */
		String	value = (String) param;
		DeviceItems	*pItem;
		int		index;

#ifdef xxx
		if (strcmp(current->value, value) == 0) {
			return((caddr_t) value);	/* already set */
		}
#endif
		pItem = FindItem(value, ports, NR_PORTS, &index);
		if (pItem == NULL) {
			/* set to other if not match in list */
			SetValue(df->w_otherPort, XtNstring, value, NULL);
			pItem = FindItem("Other", ports, NR_PORTS, &index);
		}
		if (pItem != NULL) {
			OlVaFlatSetValues(
				df->w_port,
                		index,   /* sub-item to modify */
				XtNset,     TRUE,
				0
        		);

			current = pItem;		/* remember new */
		}
		if ((current->value != NULL)&& 
		 (strcmp(current->value, "Other") == 0)) {
			if (manage_extra) {
				manage_extra = False;
				OlCallAcceptFocus( df->w_otherPort,  0);
				XtManageChild(df->w_otherPort);
			}
			SetValue( df->w_otherPort, XtNmappedWhenManaged, True, NULL);
			SetValue( df->w_otherPort, XtNstring, value, NULL);
			OlCallAcceptFocus( df->w_otherPort,  0);


		} else
			SetValue( df->w_otherPort, XtNmappedWhenManaged, False, NULL);
		return((caddr_t) current->value);    /* return current value */
	}

	case GET: {
		/*
		 * GET: return the current value of the object
		 *
		 *	INPUT: param N/A
		 *	RETURN: current value
		 */
		return((caddr_t) current->value);
	}

	case SELECT: {
		/*
		 * SELECT: One of our widgets become selected, so remember
		 * which one it was so we can return the value on GET.
		 * NOTE that this message is for use by the XtNselect callback
		 * associated with the button widgets.
		 *
		 *	INPUT: param == pSizeItem of object
		 *	RETURN: pSizeItem of object actually set.
		 */
		current = (DeviceItems *) param;
		if ((current->value) && 
		 (strcmp(current->value, "Other") == 0)) {
			if (manage_extra) {
				manage_extra = False;
				XtManageChild(df->w_otherPort);
			}
	
			SetValue( df->w_otherPort, XtNmappedWhenManaged, True, NULL);
			SetValue( df->w_otherPort, XtNstring, "", NULL);
			OlCallAcceptFocus( df->w_otherPort,  0);
		} else
			SetValue( df->w_otherPort, XtNmappedWhenManaged, False, NULL);
		return((caddr_t) current);
	}

	default:
		Warning("ObjPortNumber: Unknown message %d\n", message);
		return((caddr_t) NULL);
	}
} /* PortNumber */


caddr_t
PortDirection(message, param, index)
int	message;
caddr_t	param;
int index;
{
	int			i;
	static DeviceItems	*current = &directions[0];
	static Boolean 		manage_extra = True;
	static Boolean		first_time = True;

#ifdef TRACE
	fprintf(stderr,"PortDirection \n");
#endif
	if (first_time) {
		first_time = False;

		for (i = 0; i < NR_DIRECTIONS; i++)
			directions[i].label = GGT(directions[i].label);
	}


	switch (message) {
	case NEW: {
		/*
		 * NEW: create the object from scratch
		 *
		 *	INPUT: param == parent widget
		 *	RETURN: parent's child (highest level created here)
		 */
		 
		Widget			w_parent = (Widget) param;
		Widget			w_control;
		Widget			CreateCaption();
		void			CBselect(), CBunselect();

		w_control = XtVaCreateManagedWidget(
				"control",
				controlAreaWidgetClass,
				w_parent,
				XtNlayoutType,	(XtArgVal) OL_FIXEDROWS,
				XtNcenter,	(XtArgVal) FALSE,
				XtNalignCaptions,	(XtArgVal) TRUE,
				XtNshadowThickness,	(XtArgVal) 0,
				(String)0
			);
		/*
		 * Add the exclusives area as the caption's child
		 */
		df->w_direction = 
			XtVaCreateManagedWidget(
				"directions",
				flatButtonsWidgetClass,
				CreateCaption(w_control, GGT (label_ConfigurePort)),
				XtNlayoutType,	        OL_FIXEDROWS,
				XtNmeasure,	        1,
				XtNitems,               directions,
				XtNnumItems,            NR_DIRECTIONS,
				XtNitemFields,          itemFields,
				XtNnumItemFields,       NR_ITEM_FIELDS,
				XtNlabelJustify,        OL_LEFT,
				XtNselectProc,          CBselect,
				XtNunselectProc,        CBunselect,
				XtNbuttonType,		OL_RECT_BTN,
				XtNexclusives,		TRUE,
				0
			);

#ifdef TRACE
	fprintf(stderr,"PortDirection end NEW\n");
#endif
		return((caddr_t) df->w_direction);
	}

	case SET: {
		/*
		 * SET: set the value of the object.  NOTE that this is
		 * a forced set by our parent, the popup window object,
		 * as opposed to a direct use selection.
		 *
		 *	INPUT: param == value to set
		 *	RETURN: value actually set
		 */
		String	value = (String) param;
		DeviceItems	*pItem;
		int		index;

#ifdef DEBUG
	fprintf(stderr,"PortDirection SET value=%s current->value=%s\n",value, current->value);
#endif
		pItem = FindItem(value, directions, NR_DIRECTIONS, &index);
		if (pItem != NULL) {
			OlVaFlatSetValues(
				df->w_direction,
           		index,   /* sub-item to modify */
				XtNset,     TRUE,
				0
        		);

			current = pItem;		/* remember new */
		}
#ifdef DEBUG
fprintf(stderr, "PortDirection Set index=%d\n",index);
#endif

		if (index == 1) {
			/* direction is outgoing */
#ifdef DEBUG
	fprintf(stderr,"calling Port Speed GREY  since selected direction is outgoing\n");
#endif
				PortSpeed(DEVICE_TYPE,GREY, 0);
		} else {
#ifdef DEBUG
	fprintf(stderr,"calling Port Speed UNGREY  since selected direction is NOT outgoing\n");
#endif
				PortSpeed(DEVICE_TYPE, UNGREY, 0);
		}
		
 #ifdef TRACE
	fprintf(stderr,"PortDirection end SET\n");
#endif
		return((caddr_t) current->value);    /* return current value */
	}

	case GET: {
		/*
		 * GET: return the current value of the object
		 *
		 *	INPUT: param N/A
		 *	RETURN: current value
		 */
#ifdef TRACE
	fprintf(stderr,"PortDirection end GET\n");
#endif
		return((caddr_t) current->value);
	}

	case SELECT: {
		/*
		 * SELECT: One of our widgets become selected, so remember
		 * which one it was so we can return the value on GET.
		 * NOTE that this message is for use by the XtNselect callback
		 * associated with the button widgets.
		 *
		 *	INPUT: param == pSizeItem of object
		 *	RETURN: pSizeItem of object actually set.
		 */
		current = (DeviceItems *) param;
		switch (index) {

			case 0:

				/* bi-directional */
				/* map the speed selections */
#ifdef DEBUG
	fprintf(stderr, "PortDirection SELECT bi-directional so call UNGREY speed\n");
#endif
				PortSpeed(DEVICE_TYPE, UNGREY, 0);
				break;

			case 1:


				/* outgoing only */
				/* unmap the speed selections */
#ifdef DEBUG
	fprintf(stderr, "PortDirection SELECT outgoing so call GREY speed\n");
#endif
				PortSpeed(DEVICE_TYPE, GREY, 0);

				break;


			case 2:

				/* incoming only */

				/* map the speed selections */
#ifdef DEBUG
	fprintf(stderr, "PortDirection SELECT incoming so call UNGREY speed\n");
#endif
				PortSpeed(DEVICE_TYPE, UNGREY, 0);

				break;


			default:

				break;

			}
#ifdef TRACE
	fprintf(stderr,"PortDirection end SELECT\n");
#endif
		return((caddr_t) current);
	}

	default:
		Warning("ObjPortNumber: Unknown message %d\n", message);
		return((caddr_t) NULL);
	}
} /* PortNumber */


caddr_t
PortEnabled(message, param)
int	message;
caddr_t	param;
{
	int			i;
	static DeviceItems	*current = &enabled[0];
	static Boolean 		manage_extra = True;
	static Boolean		first_time = True;

	if (first_time) {
		first_time = False;

		for (i = 0; i < NR_ENABLED; i++)
			enabled[i].label = GGT(enabled[i].label);
	}


	switch (message) {
	case NEW: {
		/*
		 * NEW: create the object from scratch
		 *
		 *	INPUT: param == parent widget
		 *	RETURN: parent's child (highest level created here)
		 */
		 
		Widget			w_parent = (Widget) param;
		Widget			w_control;
		Widget			CreateCaption();
		void			CBselect(), CBunselect();

		w_control = XtVaCreateManagedWidget(
				"control",
				controlAreaWidgetClass,
				w_parent,
				XtNlayoutType,	(XtArgVal) OL_FIXEDROWS,
				XtNcenter,	(XtArgVal) FALSE,
				XtNalignCaptions,	(XtArgVal) TRUE,
				XtNshadowThickness,	(XtArgVal) 0,
				(String)0
			);
		/*
		 * Add the exclusives area as the caption's child
		 */
		df->w_enabled = 
			XtVaCreateManagedWidget(
				"ports",
				flatButtonsWidgetClass,
				CreateCaption(w_control, GGT (label_PortIs)),
				XtNlayoutType,	        OL_FIXEDROWS,
				XtNmeasure,	        1,
				XtNitems,               enabled,
				XtNnumItems,            NR_ENABLED,
				XtNitemFields,          itemFields,
				XtNnumItemFields,       NR_ITEM_FIELDS,
				XtNlabelJustify,        OL_LEFT,
				XtNselectProc,          CBselect,
				XtNunselectProc,        CBunselect,
				XtNbuttonType,		OL_RECT_BTN,
				XtNexclusives,		TRUE,
				0
			);

		return((caddr_t) df->w_enabled);
	}

	case SET: {
		/*
		 * SET: set the value of the object.  NOTE that this is
		 * a forced set by our parent, the popup window object,
		 * as opposed to a direct use selection.
		 *
		 *	INPUT: param == value to set
		 *	RETURN: value actually set
		 */
		String	value = (String) param;
		DeviceItems	*pItem;
		int		index;

		if ((current->value != NULL) &&
		( value != NULL) &&
		 (strcmp(current->value, value) == 0)) {
			return((caddr_t) value);	/* already set */
		}
		pItem = FindItem(value, enabled, NR_ENABLED, &index);
		if (pItem != NULL) {
			OlVaFlatSetValues(
				df->w_enabled,
                		index,   /* sub-item to modify */
				XtNset,     TRUE,
				0
        		);

			current = pItem;		/* remember new */
		}


		return((caddr_t) current->value);    /* return current value */
	}

	case GET: {
		/*
		 * GET: return the current value of the object
		 *
		 *	INPUT: param N/A
		 *	RETURN: current value
		 */
		return((caddr_t) current->value);
	}

	case SELECT: {
		/*
		 * SELECT: One of our widgets become selected, so remember
		 * which one it was so we can return the value on GET.
		 * NOTE that this message is for use by the XtNselect callback
		 * associated with the button widgets.
		 *
		 *	INPUT: param == pSizeItem of object
		 *	RETURN: pSizeItem of object actually set.
		 */
		current = (DeviceItems *) param;
		return((caddr_t) current);
	}

	default:
		Warning("PortEnable: Unknown message %d\n", message);
		return((caddr_t) NULL);
	}
} /* PortEnable */

static DeviceItems *
FindItem(value, list, count, index)
String		value;
DeviceItems	*list;
register	count;
int*		index;
{
	register DeviceItems	*pItem = list;
	register i;

	for (i=0; i < count; i++, pItem++) {
		if (!strcmp(pItem->value, value)) {
			*index = i;
			return(pItem);
		}
	}
	return(NULL);
} /* FindItem */



