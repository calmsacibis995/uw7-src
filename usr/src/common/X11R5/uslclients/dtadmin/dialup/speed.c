
/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/speed.c	1.18"
#endif

/* ############################  Port Speed ############################ */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FButtons.h>
#include <Xol/Form.h>
#include <Xol/ControlAre.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

static DeviceItems *	FindSpeed();
extern Boolean	ChangeCB();
extern void		FocusChangeCB();
extern void		Warning();
extern Widget		CreateCaption();
extern void		CBselect();
extern void		CBunselect();
static void 		SetManaged();

typedef enum {
	Speed300, Speed1200, Speed2400, Speed4800, Speed9600,
	Speed14400, Speed19200, Speed28800, Speed38400, SpeedOther, SpeedAny
} SpeedMenuIndex;

DeviceItems	speeds[] = {
	{ label_b300, "300" , True},
	{ label_b1200, "1200", True },
	{ label_b2400, "2400" , True},
	{ label_b4800, "4800", True },
	{ label_b9600, "9600" , True},
	{ label_b14400, "14400", True },
	{ label_b19200, "19200" , True},
	{ label_b28800, "28800" , True},
	{ label_b38400, "38400" , True},
	{ label_other, "Other" , True},
	{ label_any, "Any" , False},
};
/*
 * itemFields[] -  array of choice item resource names
 */
static String  itemFields[] = {
        XtNlabel,
	XtNclientData,
	XtNsensitive
};

#define NR_ITEM_FIELDS  (sizeof(itemFields) / sizeof(itemFields[0]))
#define NR_SPEEDS	(sizeof(speeds) / sizeof(DeviceItems))

static Widget		w_control[2];
static void	SetSpeedSensitivity();
static DeviceItems	*current[2];

caddr_t
PortSpeed(popup_type,message, param)
int 	popup_type;		/* 0 = Systems file , 1 = Devices file */
int	message;
caddr_t	param;
{
	int			i;
	static Boolean		first_time = True;
	static Widget		w_otherSpeed[2];
	static DeviceItems	*current[2];
	static Widget		w_speed[2];


#ifdef DEBUG
	fprintf(stderr,"PortSpeed: popup_type=%d message=%d\n",popup_type,message);
#endif
	if (first_time) {
		current[0] = &speeds[0];
		current[1] = &speeds[0];
		first_time = False;

		for (i = 0; i < NR_SPEEDS; i++)
			speeds[i].label = GGT(speeds[i].label);
	}


	switch (message) {
	case UNGREY:
#ifdef DEBUG
	fprintf(stderr,"PortSPeed UNGREY holdData.portDirection=%s\n",holdData.portDirection);
#endif
		/* need to ungrey the speeds */
		SetSpeedSensitivity(w_speed[popup_type], True);

		/* if speed is already set, then use the 
			previously chosen speed, else
			default to 9600 */
		if ((holdData.portSpeed != NULL) &&
		(strcmp(holdData.portSpeed, speeds[SpeedAny].value) != 0)) {
			PortSpeed(popup_type, SET, holdData.portSpeed);
		}  else  {
			PortSpeed(popup_type, SET, speeds[Speed9600].value);
		}
		

		break;
	case GREY:
#ifdef DEBUG
	fprintf(stderr,"PortSpeed: GREY set to outgoing \n");
#endif
		/* need to grey the speed controls */
		/* for outgoing we force  */
		/*  the speed to be set to autoselect (any) */
#ifdef DEBUG
	fprintf(stderr,"Port Speed GREY before call to SET SpeedAny\n");
#endif
			/* set them all to sensitive to make selection */
		SetSpeedSensitivity(w_speed[popup_type], True);
			/* select auto select */
		PortSpeed(popup_type, SET, speeds[SpeedAny].value);
		OlVaFlatSetValues(w_speed[popup_type], 
				SpeedAny, 
				XtNset,
				True,
				0);
			/* set them all to insensitive */
		SetSpeedSensitivity(w_speed[popup_type], False);
			/* unamanage other textfield */
		XtVaSetValues(w_otherSpeed[popup_type],
				XtNmappedWhenManaged,
				False,
				0);
			/* set speed any back to being sensitive */
		OlVaFlatSetValues(w_speed[popup_type], 
				SpeedAny, 
				XtNsensitive,
				True,
				0);
		
		
		break;
			
	case NEW: {
		/*
		 * NEW: create the object from scratch
		 *
		 *	INPUT: param == parent widget
		 *	RETURN: parent's child (highest level created here)
		 */
		 
		Widget			w_parent = (Widget) param;


#ifdef DEBUG
	fprintf(stderr,"PortSpeed NEW\n");
#endif
        w_control[popup_type] = XtVaCreateManagedWidget(
                "control",
                controlAreaWidgetClass,
                w_parent,
                XtNlayoutType,  (XtArgVal) OL_FIXEDCOLS,
				XtNmeasure,	        2,
                XtNcenter,  (XtArgVal) FALSE,
                XtNalignCaptions,   (XtArgVal) TRUE,
                XtNshadowThickness, (XtArgVal) 0,
                (String)0
            );

		/*
		 * Add the exclusives area as the caption's child
		 */
		w_speed[popup_type] =
			XtVaCreateManagedWidget(
				"speeds",
				flatButtonsWidgetClass,
				CreateCaption(w_control[popup_type] , 
						GGT(label_class)),
				XtNlayoutType,	        OL_FIXEDROWS,
				XtNmeasure,	        3,
				XtNitems,               speeds,
				XtNnumItems,            NR_SPEEDS,
				XtNitemFields,          itemFields,
				XtNnumItemFields,       NR_ITEM_FIELDS,
				XtNlabelJustify,        OL_CENTER,
				XtNselectProc,          CBselect,
				XtNunselectProc,        CBunselect,
				XtNbuttonType,		OL_RECT_BTN,
				XtNexclusives,		TRUE,
				0
			);

		/* set mapped to False initially */

		w_otherSpeed[popup_type] = (Widget) XtVaCreateWidget(
				"other_speed",
				textFieldWidgetClass,
				w_control[popup_type],
				XtNcharsVisible, SPEED_SIZE,
				XtNmaximumSize,	SPEED_SIZE,
				XtNtraversalOn, True,
				(String) 0
			);
			
		XtVaSetValues((Widget) w_otherSpeed[popup_type],
				XtNregisterFocusFunc, 
				FocusChangeCB,
				0);
		XtAddCallback((Widget) w_otherSpeed[popup_type],
			XtNverification,
			(XtCallbackProc) ChangeCB,
			(caddr_t) F_SPEED
		);
		/*
		 * Manage the choice widget and return
		 */
		if (popup_type == 0) {
			sf->w_speed = w_speed[popup_type];
			sf->w_otherSpeed = w_otherSpeed[popup_type];
		} else {
			df->w_speed = w_speed[popup_type];
			df->w_otherSpeed = w_otherSpeed[popup_type];
		}
		return((caddr_t) w_speed[popup_type]);
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
		String		value = (String) param;
		DeviceItems	*pItem;
		int		index;
#ifdef DEBUG
	fprintf(stderr,"PortSpeed SET value=%s\n", value);
#endif

		if (popup_type == SYSTEMS_TYPE) {
			SetSpeedSensitivity(w_speed[popup_type], True);
		}
		pItem = FindSpeed(value, speeds, NR_SPEEDS, &index);
		if (pItem == NULL) {
			/* set to other if no match found in list */
            		XtVaSetValues(w_otherSpeed[popup_type], XtNstring, "", NULL);
			pItem = FindSpeed("Other", speeds, NR_SPEEDS, &index);
		}
		if (pItem != NULL) {
			OlVaFlatSetValues(
				w_speed[popup_type],
                		index,   /* sub-item to modify */
				XtNset,     TRUE,
				0
        		);

		current[popup_type] = pItem;		/* remember new */
	}
	if ((current[popup_type]->value != NULL) &&
	 (strcmp(current[popup_type]->value, "Other") == 0)) {
		SetManaged(popup_type, w_otherSpeed[popup_type],
			value);
	} else {
		XtVaSetValues(w_otherSpeed[popup_type],
			XtNmappedWhenManaged,
			False,
			0);
	}	
        return((caddr_t) current[popup_type]->value);    /* return current value */
    }



	case GET: {
		/*
		 * GET: return the current value of the object
		 *
		 *	INPUT: param N/A
		 *	RETURN: current value
		 */
#ifdef DEBUG
	fprintf(stderr,"PortSpeed GET value=%s\n", current[popup_type]->value);
#endif
		return((caddr_t) current[popup_type]->value);
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
	current[popup_type] = (DeviceItems *) param;
#ifdef DEBUG
	fprintf(stderr,"PortSpeed SELECT value=%s\n", current[popup_type]->value);
#endif
	if (strcmp(current[popup_type]->value, "14400") == 0) {
		if (popup_type == SYSTEMS_TYPE) {
			NotifyUser(sf->toplevel, GGT(string_resetSpeed));
		} else {
			DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
		}
		PortSpeed(popup_type, SET, "19200");
        	return((caddr_t) current[popup_type]);
	} else
	if (strcmp(current[popup_type]->value, "28800") == 0) {
		if (popup_type == SYSTEMS_TYPE) {
			NotifyUser(sf->toplevel, GGT(string_resetSpeed));
		} else {
			DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
		}
		PortSpeed(popup_type, SET, "38400");
        	return((caddr_t) current[popup_type]);
	}
	if ((current[popup_type]->value != NULL) &&
	(strcmp(current[popup_type]->value, "Other") == 0)) {
		SetManaged(popup_type, w_otherSpeed[popup_type],
			"");
	} else {
		XtVaSetValues(w_otherSpeed[popup_type],
			XtNmappedWhenManaged,
			False,
			0);
	}	
   
        return((caddr_t) current[popup_type]);
    }


	default:
#ifdef DEBUG
	fprintf(stderr,"PortSpeed default\n");
#endif
		Warning("SpeedSpeed: Unknown message %d\n", message);
		return((caddr_t) NULL);
	}
} /* PortSpeed */

static DeviceItems *
FindSpeed(value, list, count, index)
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
} /* FindSpeed */


static void
SetSpeedSensitivity(w, value)
Widget w;
Boolean value;
{

		int i;
		/* set all the speeds to sensitive or insensitive */
		for (i = 0; i < NR_SPEEDS; i++) {
			OlVaFlatSetValues(w, 
				i,
				XtNsensitive,
				(Boolean) value, 
				0);
		}
}


static void
SetManaged(popup_type,speedWidget, value)
int popup_type;
Widget speedWidget;
char *value;
{
	static Boolean manage_extra[] = { True, True} ;

		if (manage_extra[popup_type]) {
                	manage_extra[popup_type] = False;
                	XtManageChild(speedWidget);
            	}
            	XtVaSetValues( speedWidget, 
			XtNmappedWhenManaged, 
			True, 
			NULL);
            	XtVaSetValues( speedWidget, 
				XtNstring, 
				value, 
				NULL);

		/* move focus to the textfield */
		OlCallAcceptFocus(speedWidget, 0);
		
}

