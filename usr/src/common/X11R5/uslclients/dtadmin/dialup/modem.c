/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/modem.c	1.23.1.1"
#endif

/* ########################  Modem Family ###################### */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/AbbrevButt.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>
#include <Xol/ControlAre.h>
#include <Xol/StaticText.h>
#include <Gizmos.h>
#include "uucp.h"
#include "error.h"

extern Widget		CreateCaption();
extern void		CBselect();
extern void		CBunselect();
extern void		SetValue();
extern void		Warning();

DeviceItems families[] = {
	{ label_hayes, "hayes" , True},
	{ label_hayesSM1200, "HayesSmartm1200", True },
	{ label_hayesSM2400, "HayesSmartm2400", True },
	{ label_att4000, "att4000", True },
	{ label_att4024, "att4024", True },
	{ label_att2224, "att2224", True },
	{ label_att2248, "att2248a" , True},
	{ label_att2296, "att2296a" , True},
	{ label_tb, "tb", True },
	{ label_multitech, "Multinormalv29", True },
	{ label_datakit, "datakit" , True},
	{ label_direct, "uudirect", True },
};

DeviceItems	*FamilyItemPtr;
/*
 * itemFields[] -  array of choice item resource names
 */
static String  itemFields[] = {
        XtNlabel,
	XtNclientData,
	XtNsensitive,
};

#define NR_ITEM_FIELDS  (sizeof(itemFields) / sizeof(itemFields[0]))
#define NR_FAMILIES	(sizeof(families) / sizeof(DeviceItems))

/* ReadModemTable
 *
 * Read the defined modem type from the modem table
 * ie. $XWINHOME/desktop/dtadmin/Modems
 */
static void
ReadModemTable (DeviceItems **itemp, unsigned *nump)
{
	FILE		*file;
	DeviceItems	*item;
	int		i;
	int		allocated = 0;
	char		*label;
	char		*caret;
	char		buf [BUFSIZ];
	char 		*x;
	static char	modem_tab[BUFSIZ];
	static Boolean	first_time = True;

	if (first_time) {
		first_time = False;

		for (i = 0; i < NR_FAMILIES; i++)
			families[i].label = GGT(families[i].label);

		x = getenv("XWINHOME");
		if (!x)
			x = "/usr/X";
		(void)sprintf( modem_tab, "%s/desktop/DialupMgr/Modems", x); 
	}

#ifdef DEBUG
	fprintf(stderr,"modem_tab=%s\n", modem_tab);
#endif
	if (!(file = fopen (modem_tab, "r"))) {
		*itemp = families;
		*nump = NR_FAMILIES;
#ifdef DEBUG
	fprintf(stderr,"using default table\n");
#endif
		return;
	}

	*itemp = (DeviceItems *) NULL;
	*nump = 0;
	while (fgets (buf, 1024, file)) {
		if (*nump >= allocated) {
			allocated += INCREMENT;
			*itemp = (DeviceItems *) XtRealloc ((char *) *itemp,
				allocated * sizeof (DeviceItems));
		}

		item = *itemp + (*nump)++;

		label = strtok (buf, "\t\n");
		if (!label) {
			(*nump)--;
			continue;
		}

	/* First field is a tag; ignore it.  Second field is an optionally
	 * internationalized label.  A '^' separates the default string from
	 * the file:index.
	 */
		label = strtok (NULL, "\t\n");
		if (!label) {
			(*nump)--;
			continue;
		}

		caret = strchr (label, '^');
		if (caret) {
			*caret = 0;
			item->label = (String)gettxt (caret+1, label);
			if ((char *) item->label == label)
				item->label = (String)strdup (label);
		} else
			item->label = (String)strdup (label);

		label = strtok (NULL, "\t\n");
		if (!label) {
			(*nump)--;
			XtFree ((char *) item->label);
			continue;
		}
		item->value = (String)strdup (label);
#ifdef DEBUG
	fprintf(stderr,"End of ReadModem: item->value=%s\n",item->label);
#endif
		item->sensitive = True;
	}

	fclose (file);

#ifdef DEBUG
	fprintf(stderr,"End of ReadModem: nump=%d\n",*nump);
#endif

} /* ReadModemTable() */

caddr_t
ModemFamily(message, param)
int	message;
caddr_t	param;
{
	static Widget		w_display;	/* displays current modem */
	static DeviceItems	*current;
	static unsigned		numberOfItem;
	static Boolean		first_time = True;

	if (first_time) {
		first_time = False;
		ReadModemTable(&FamilyItemPtr, &numberOfItem);
		current = FamilyItemPtr;
	}
		
	switch (message) {
	case NEW: {
		/*
		 * NEW: create the object from scratch
		 *
		 *	INPUT: param == parent widget
		 *	RETURN: parent's child (highest level created here)
		 */
		Widget		w_control, w_stack, w_menuPane;
		Widget		w_parent = (Widget) param;

		/*
		 * Add the control area to hold the AbbrevFlatMenuButton
		 * and its display-current-value widget.
		 */
		w_control = 
			XtVaCreateWidget(
				"control",
				controlAreaWidgetClass,
				/*CreateCaption(w_parent, GGT(label_modemType)),*/
				CreateCaption(w_parent, GGT(label_connectTo)),
				XtNlayoutType,	OL_FIXEDROWS,
				XtNalignCaptions,	(XtArgVal) TRUE,
				XtNshadowThickness,	(XtArgVal) 0,
				XtNmeasure,	1,
				0
			);

		/*
		 * Create the AbbrevFlatMenuButton widget as a
		 * child of control
		 */

		w_menuPane = XtVaCreatePopupShell(
					"pane",
					popupMenuShellWidgetClass,
					w_control,
					XtNpushpin,     OL_NONE,
					XtNhasTitle,	TRUE,
					XtNtitle,       GGT(title_supportDev),
					0
				);
		w_stack = 
			XtVaCreateWidget(
				"buttonStack",
				abbreviatedButtonWidgetClass,
				w_control,	
				XtNborderWidth,	0,
				XtNpopupWidget,	(XtArgVal)w_menuPane,
				0
			);

		/*
		 * Create the static text widget to display the 
		 * current setting of the button stack.  Also a
		 * child of control.
		 */
		w_display = 
			XtVaCreateManagedWidget(
				"selection",		/* instance name */
				staticTextWidgetClass,	/* widget class */
				w_control,		/* parent widget */
				XtNgravity,	NorthWestGravity,
				XtNstring,	FamilyItemPtr->label,
				XtNborderWidth,	1,
				0
			);

		/*
		 * Tell the button stack widget about w_display so it
		 * can use it as a preview area for the default selection.
		 * (i.e. when the user holds down the SELECT button on the
		 * abbrev widget, it should display the default in w_display
		 */
		SetValue(w_stack, XtNpreviewWidget, w_display);

		/*
		 * Add the exclusives container to the abbrev stack by
		 * making it a child of the abbrev stack menuPane
		 */
		df->w_modem = 
			XtVaCreateWidget(
				"modem",
				flatButtonsWidgetClass,
				w_menuPane,
				XtNlayoutType,	        OL_FIXEDCOLS,
				XtNmeasure,	        3,
				XtNitems,               FamilyItemPtr,
				XtNnumItems,            numberOfItem,
				XtNitemFields,          itemFields,
				XtNnumItemFields,       NR_ITEM_FIELDS,
				XtNlabelJustify,        OL_LEFT,
				XtNselectProc,          CBselect,
				XtNunselectProc,        CBunselect,
				XtNbuttonType,		OL_RECT_BTN,
				XtNexclusives,		TRUE,
				0
			);

		XtManageChild(df->w_modem);
		XtManageChild(w_stack);
		XtManageChild(w_control);
		return((caddr_t)w_control);
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
		int		index;
		DeviceItems	*pItem, *FindModem();

		if (strcmp (current->value, value) == 0) {
			return((caddr_t) value);	/* already set */
		} 

		pItem = FindModem(value, FamilyItemPtr, numberOfItem, &index);
#ifdef DEBUG
	fprintf(stderr,"pItem found=%s index=%d\n",pItem,index);
#endif
		if (pItem != NULL) {
			OlVaFlatSetValues(
				df->w_modem,
          		index,   /* sub-item to modify */
				XtNset,     TRUE,
				0
        		);

			current = pItem;		/* remember new */

			/*
			 * display in Current Selection widget
			 */
#ifdef DEBUG
	fprintf(stderr,"label to show =%s\n",pItem->label);
#endif

			SetValue(w_display, XtNstring, current->label);
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

		/*
		 * display in Current Selection widget
		 */
		SetValue(w_display, XtNstring, current->label);

		return((caddr_t) current);
	}

	default:
		Warning("ModemFamily: Unknown message %d\n", message);
		return((caddr_t) NULL);
	}
} /* ModemFamily */

static DeviceItems *
FindModem(value, list, count, index)
	String		value;
	DeviceItems	*list;
	register	count;
	int	*index;
{
	register DeviceItems	*pItem = list;
	register i;

	for (i=0 ; i < count; i++, pItem++) {
#ifdef DEBUG
	fprintf(stderr,"FindModem: list_value=%s  value= %s\n",pItem->value, value);
#endif
		if (!strcmp(value, pItem->value)) {
			*index = i;
			return(pItem);
		}
	}
	Warning("FindModem: couldn't find value %s\n", value);
	return(NULL);
} /* FindModem */
