/*		copyright	"%c%" 	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/iproperty.c	1.16.1.27"
#endif

/*		This module handles the device property sheet */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/PopupWindo.h>
#include <libDtI/DtI.h>
#include <Gizmos.h>
#include <Xol/Footer.h>
#include "error.h"
#include "uucp.h"
#include <sys/stat.h>

extern void callRegisterHelp(Widget, char * , char *);
Boolean Check4DuplicatePorts(char *, char *, int);
Boolean Check4PortAlias(char *, int);
char * TranslateSpecialNames(char *);
static Boolean Check4SpecialSpeeds(char *);
static Boolean VerifyOtherPort();
extern Boolean DeviceExists(char *file);
extern void Remove_ttyService(char *port);

DeviceData	deviceDefaults = {
	"com1",		/* portNumber */
	"hayes",	/* modemFamily */
	"Any",		/* portSpeed */
	"outgoing", /* port Direction */
	"enabled",		/* port enabled */
	"",		/* DTP */
	
};

extern char *	ApplicationName;

extern Boolean CheckMousePort(char *port);
extern XtArgVal	GetValue();	/* get a single resource value */
extern void	SetValue();	/* set a single resource value */
extern void	SaveDeviceFile();
extern void	AlignIcons();
extern void	Warning();
extern caddr_t	ModemFamily();
extern caddr_t	PortEnabled();
extern caddr_t	PortNumber();

extern DeviceItems     enabled[];
extern DeviceItems     ports[];
extern DeviceItems     *FamilyItemPtr;
extern DeviceItems     directions[];
extern DeviceItems     speeds[];
void FreeAndSaveHoldData(int type);

void HandleButtonAction();

void
CBPopupPropertyWindow(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	ClearLeftFooter(sf->dprop_footer);
	XtPopup(df->propPopup, XtGrabNone);	/* popup the window */
	callRegisterHelp(df->propPopup, title_property, help_dproperty);
	XRaiseWindow(DISPLAY, XtWindow(df->propPopup));
} /* CBPopupPropertyWindow */

void
CBUpdateContainer(wid, client_data, call_data)
Widget	wid;
XtPointer	client_data;
XtPointer	call_data;
{
        extern XFontStruct	*font;
	extern DmFclassPtr	acu_fcp, dir_fcp;
	extern DmObjectPtr	new_object();
	DmObjectPtr		op;
	DmItemPtr		ip;
	DeviceData		*tmp;
	Dimension		width;
	Cardinal		nitems;
	register		i;
	char			name[20];
	
	XtVaGetValues(df->iconbox,
		XtNnumItems, &nitems,
		0
	);
	width = GetValue(df->iconbox, XtNwidth);
#ifdef NITEMS
		fprintf(stderr, "In Creation(B4), nitems = %d\n", nitems);
		fprintf(stderr, "In Creation(B4), width = %d\n", width);
#endif
	tmp = (DeviceData *) XtMalloc(sizeof(DeviceData));
	if (tmp == NULL) {
		fprintf(stderr,
		"CBUpdateConatainer: couldn't XtMalloc an DeviceData\n");
		exit(1);
	}

	tmp->DTP = strdup("");
	tmp->holdPortEnabled = tmp->portEnabled = strdup(holdData.portEnabled);
	tmp->holdModemFamily = tmp->modemFamily = strdup(holdData.modemFamily);
	tmp->holdPortSpeed = tmp->portSpeed = strdup(holdData.portSpeed);
	tmp->holdPortNumber = tmp->portNumber = strdup(holdData.portNumber);
	tmp->holdPortDirection = tmp->portDirection = strdup(holdData.portDirection);
	strcpy(name, tmp->portNumber);

	if (df->request_type == B_MOD) {
		for (i=0, ip = df->itp; i < nitems; i++, ip++) {
		    if(ITEM_MANAGED(ip) == False)
			    continue;
		    if( (op = (DmObjectPtr)OBJECT_PTR(ip)) == df->select_op) {
			/* found it */
			FreeDeviceData(op->objectdata);
			op->objectdata = tmp;
			free((char *)ip->label);
			ip->label = (XA)strdup(name);
			free(op->name);
			op->name = strdup(name);
			break;
		    }
		}
	} else {
		if ((op=new_object(name, tmp)) == (DmObjectPtr) OL_NO_ITEM) {
				return;
		}
	}
	if ((strcmp(tmp->modemFamily, "uudirect") == 0)
	    || (strcmp(tmp->modemFamily, "datakit") == 0))
		op->fcp = dir_fcp;
	else
		op->fcp = acu_fcp;

	op->x = op->y = UNSPECIFIED_POS;
	if (df->request_type == B_MOD) {
		OlFlatRefreshItem(
			df->iconbox,
			i,
			True
		);
	} else {
		Dm__AddObjToIcontainer(df->iconbox, &(df->itp),
				(Cardinal *)&(nitems),
				df->cp, op,
				op->x, op->y, DM_B_NO_INIT | DM_B_CALC_SIZE,
				NULL,
				font,
				width,
				(Dimension)INC_X,
				(Dimension)INC_Y
				);
#ifdef NITEMS
		fprintf(stderr, "In Creation, nitems = %d\n", nitems);
		fprintf(stderr, "In Creation, width = %d\n", width);
#endif
		AlignIcons();
	}
} /* CBUpdateContainer */

void
CBUnbusyButton(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	SetValue((Widget) client_data, XtNbusy, FALSE);	/* unbusy button */
	return;
} /* CBUnbusyButton */

extern void		DisallowPopdown();
extern void             HelpCB();

static HelpText AppHelp = {
    title_property, HELP_FILE, help_dproperty,
};

static Items propertyItems [] = {
    { HandleButtonAction, NULL, (XA)TRUE, NULL, NULL, (XA)APPLY}, /* apply */
    { HandleButtonAction, NULL, (XA)TRUE, NULL, NULL, (XA)RESET}, /* reset */
    { HandleButtonAction, NULL, (XA)TRUE, NULL, NULL, (XA)CANCEL}, /* cancel */
    { HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&AppHelp},
};

static Menus propertyMenu = {
	"property",
	propertyItems,
	XtNumber (propertyItems),
	False,
	OL_FIXEDROWS,
	OL_NONE,
	NULL
};

/*
 * Device Property Window routines: creating a Property Window and managing 
 * its fields.
 */
void
CreatePropertyWindow(w_parent)
Widget	w_parent;
{
	char 	buf[128];
	Widget	w_controlTop, w_controlBottom, w_footerPanel;

	static XtCallbackRec	reset[] = {
		{ (XtCallbackProc) HandleButtonAction, (caddr_t) RESET },
		{ NULL, NULL },
	};

#ifdef DEBUG
	fprintf(stderr,"CreatePropertyWindow\n");
#endif
	SET_HELP(AppHelp);
	sprintf(buf, "%s: %s", ApplicationName, GGT(title_dproperties));
	/*
	 * create the popup window widget
	 */
	df->propPopup = 
		XtVaCreatePopupShell(
			"deviceProperties",		/* instance name */
			popupWindowShellWidgetClass,	/* widget class */
			w_parent,			/* parent widget */
			XtNtitle,	buf,
			XtNpopdownCallback,	reset,
			0
		);
	XtAddCallback (
                df->propPopup,
                XtNverify,
                DisallowPopdown,
                (XtPointer)0
        );

	/*
	 * retrieve the widget IDs of the control areas and the footer panel
	 */
	XtVaGetValues(
		df->propPopup,
		XtNupperControlArea,	&w_controlTop,
		XtNlowerControlArea,	&w_controlBottom,
		XtNfooterPanel,			&w_footerPanel,
		0
	);

	sf->dprop_footer = 
		XtVaCreateManagedWidget("dprop_footer",
			footerWidgetClass,
			w_footerPanel,
			0);

	/*
	 * Add property items to the popup.  Each object has its own 
	 * unique entry point where you send action messages such as 
	 * NEW, SET, and GET.  The Apply, Reset, ResetToFactory, and 
	 * Set Defaults actions are handled at this level, above 
	 * those objects.
	 */
	PortNumber(NEW, w_controlTop);
	ModemFamily(NEW, w_controlTop);
#ifdef DEBUG
	fprintf(stderr,"Calling PortDirection NEW from CreateWindow\n");
#endif
	PortDirection(NEW, w_controlTop);
	PortSpeed(DEVICE_TYPE,NEW, w_controlTop);
	PortEnabled(NEW, w_controlTop);
	SET_LABEL (propertyItems,0,add);
	SET_LABEL (propertyItems,1,reset);
	SET_LABEL (propertyItems,2,cancel);
	SET_LABEL (propertyItems,3,help);

	if (!sf->update) propertyItems[0].sensitive = sf->update;

	AddMenu (w_controlBottom, &propertyMenu, False);
	callRegisterHelp(df->propPopup, title_property, help_dproperty);
#ifdef TRACE
	fprintf(stderr,"End CreatePropertyWindow\n");
#endif
} /* CreatePropertyWindow */

void
SetDevPropertyLabel(index, value, mnemonic)
int index;
char * value;
char * mnemonic;
{


#ifdef TRACE
	fprintf(stderr,"SetDevPropetyLabel label=%s mnemonic=%c\n",GGT(value),*GGT(mnemonic));
#endif
	OlVaFlatSetValues(propertyMenu.widget, index,
			XtNlabel, (String)GGT(value),
			XtNmnemonic, (XtArgVal) *GGT(mnemonic),
			0);

}

/*
 * HandleButtonAction() - manages events that effect the state of the 
 * entire popup window.
 */
void HandleButtonAction(wid, client_data, call_data)
Widget	wid;
caddr_t	client_data;
caddr_t	call_data;
{
	extern	void CBUpdateContainer();
	Boolean result;
	int devAction;
	String tmpPort;
	int	action = (int) client_data;

	ClearFooter(df->footer);
	switch (action) {

	case APPLY:
		ClearFooter(df->footer);
		FreeAndSaveHoldData(1);
			/* save PortNumber */
		holdData.portNumber = strdup((String) PortNumber(GET, 0));
		
		if (strcmp(holdData.portNumber, "Other") == 0) {
		/* not com1, not com2; it is other */
			XtFree(holdData.portNumber);
			holdData.portNumber = OlTextFieldGetString(df->w_otherPort, NULL);
			if (holdData.portNumber != NULL) {
				/* strip off leading spaces if any */
				if ((VerifyOtherPort() == INVALID)) {
					ResetPortInformation(PORT);
					DeviceNotifyUser(df->toplevel, GGT(string_otherPort));
					/* error popup that port is required */
					return;
				}
		    	}
		}
			/* check that neither the printer nor the 
				mouse is assigned to the port */
		if ((result = CheckMousePort(holdData.portNumber)) == True) {
			ResetPortInformation(PORT);
			DeviceNotifyUser(df->toplevel, GGT(string_badMousePort));
			return;
		}
		else
		if ((result = CheckPrinterPort(holdData.portNumber)) == True) {
			ResetPortInformation(PORT);
			DeviceNotifyUser(df->toplevel, GGT(string_badPrinterPort));
			return;
		}
			
		/* need to check for duplicate ports on an ADD
			and on a change of portNumber */
		/* check for duplicate ports on an add */
		if (df->request_type == B_ADD) {
			if ((result = Check4DuplicatePorts(holdData.portNumber,
				holdData.holdPortNumber,
				df->request_type)) == INVALID) {
					ResetPortInformation(PORT);
					DeviceNotifyUser(df->toplevel, GGT(string_badPortAdd));
					return;
				} 
						
		} else
		/* on a change check for duplicates if the
			port number changed */
		if (( holdData.portNumber != NULL) &&
			(df->request_type != B_ADD) &&
			( holdData.holdPortNumber != NULL) &&
			(strcmp(holdData.portNumber, holdData.holdPortNumber) != 0)) {
			/* port changed so need to check for duplicates */
			if ((result = Check4DuplicatePorts(holdData.portNumber,
				holdData.holdPortNumber,
				df->request_type)) == INVALID) {
					ResetPortInformation(PORT);
					DeviceNotifyUser(df->toplevel, GGT(string_badPortChg));
					return;
			} else 


			/* if port number changed then remove ttymon on old port */
			/* if the old port was not outgoing */
			 if ((holdData.holdPortDirection != NULL) &&
			((strcmp(holdData.holdPortDirection, "outgoing"))!=0)) { 
				/* portnumber changed */

#ifdef TTYMON_DEBUG	
		fprintf(stderr,"port changed so we need to getrid of the oldttymon for port=%s oldport=%s\n",holdData.portNumber,
		holdData.holdPortNumber);
#endif
				Remove_ttyService(holdData.holdPortNumber);
				}
	
		}
		FreeAndSaveHoldData(2);
		holdData.modemFamily = strdup((String) ModemFamily(GET, 0));
		FreeAndSaveHoldData(4);
		holdData.portDirection = strdup((String) PortDirection(GET, 0));
#ifdef DEBUG
		fprintf(stderr,"Apply portDirection=%s\n", holdData.portDirection);
#endif
		/* add call to create/delete ttymon */
#ifdef DEBUG
	fprintf(stderr,"call Update_ttymon port=%s direction=%s\n",holdData.portNumber,holdData.portDirection);
#endif
		FreeAndSaveHoldData(0);
			/* save PortSpeed */
		holdData.portSpeed = strdup((String) PortSpeed(DEVICE_TYPE, GET, 0));
		if (strcmp(holdData.portSpeed, "Other") == 0) {
		/* not listed speed; it is other */
			XtFree(holdData.portSpeed);
			holdData.portSpeed = OlTextFieldGetString(df->w_otherSpeed, NULL);
			if (holdData.portSpeed != NULL) {
				/* strip off leading spaces if any */
				if ((VerifySpeedField(DEVICE_TYPE,
					holdData.portSpeed) == -1)) {
					/* error popup that port is required */
					ResetPortInformation(SPEED);
					return;
				}
			}

		}
		if (strcmp(holdData.portSpeed, "14400") == 0) {
			if (holdData.portSpeed != NULL) XtFree(holdData.portSpeed);
			holdData.portSpeed = strdup("19200");
			DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
			PortSpeed(DEVICE_TYPE, SET, "19200");
			return;
		} else
		if (strcmp(holdData.portSpeed, "28800") == 0) {
			if (holdData.portSpeed != NULL) XtFree(holdData.portSpeed);
			holdData.portSpeed = strdup("38400");
			DeviceNotifyUser(df->toplevel, GGT(string_resetSpeed));
			PortSpeed(DEVICE_TYPE, SET, "38400");
			return;
		} else 
		if (Check4SpecialSpeeds(holdData.portSpeed) == INVALID) {
			ResetPortInformation(SPEED);
			 return;
		}
		FreeAndSaveHoldData(3);
		holdData.portEnabled= strdup((String) PortEnabled(GET, 0));
		CBUpdateContainer(wid, (caddr_t)0, (caddr_t)0);
		if (df->request_type == B_ADD) 
			devAction = ADD;
		else
			devAction = MODIFY;

		SaveDeviceFile(holdData.portNumber, holdData.holdPortNumber, devAction);
			/*do the ttymon updates after the /etc/uucp
			Devices file has been written so the Reset
			lines will be up to date when ttymon -o options
			are given */
		Update_ttymon(&holdData);
		SetDialSensitivity();
		/* now save the new values in hold data and current data */
		FreeAndSaveHoldData(0);	
		FreeAndSaveHoldData(1);
		FreeAndSaveHoldData(2);
		FreeAndSaveHoldData(3);
		FreeAndSaveHoldData(4);
		XtPopdown(df->propPopup);
		callRegisterHelp(df->toplevel, title_setup, help_setup);
		break;

	case RESET:
		/* if the request is B_ADD, then, it should fall through */
		if(df->request_type == B_MOD) {
			PortNumber(SET, ((DeviceData*)
			(df->select_op->objectdata))->holdPortNumber);
			ModemFamily(SET, ((DeviceData*)
			(df->select_op->objectdata))->holdModemFamily);
			PortEnabled(SET, ((DeviceData *)
			(df->select_op->objectdata))->holdPortEnabled);
			PortDirection(SET, ((DeviceData *)
			(df->select_op->objectdata))->holdPortDirection);
			PortSpeed(DEVICE_TYPE, SET, ((DeviceData *)
			(df->select_op->objectdata))->holdPortSpeed);
		    return;
		}


	case RESETFACTORY:
		PortNumber(SET,	deviceDefaults.portNumber);
		ModemFamily(SET, deviceDefaults.modemFamily);
		PortDirection(SET, deviceDefaults.portDirection);
		PortSpeed(DEVICE_TYPE, SET, deviceDefaults.portSpeed);
		PortEnabled(SET, deviceDefaults.portEnabled);
		FreeAndSaveHoldData(0);	
		FreeAndSaveHoldData(1);	
		FreeAndSaveHoldData(2);	
		FreeAndSaveHoldData(3);	
		FreeAndSaveHoldData(4);	
		if (holdData.portNumber != NULL) 
			XtFree(holdData.portNumber);
		holdData.portNumber = strdup(deviceDefaults.portNumber);
		if (holdData.modemFamily != NULL) 
			XtFree(holdData.modemFamily);
		holdData.modemFamily = strdup(deviceDefaults.modemFamily);
		if (holdData.portDirection != NULL) 
			XtFree(holdData.portDirection);
		holdData.portDirection = strdup(deviceDefaults.portDirection);
		if (holdData.portEnabled != NULL) 
			XtFree(holdData.portEnabled);
		holdData.portEnabled = strdup(deviceDefaults.portEnabled);
		if (holdData.portSpeed != NULL) 
			XtFree(holdData.portSpeed);
		holdData.portSpeed = strdup(deviceDefaults.portSpeed);
		break;

	case CANCEL:
		XtPopdown(df->propPopup);
		callRegisterHelp(df->toplevel, title_setup, help_setup);
		break;

	default:
		Warning("HandleButtonAction: Unknown action %d\n", action);
		break;
	}

	return;
} /* HandleButtonAction */

void
CBselect(wid, client_data, call_data)
Widget  wid;
caddr_t client_data;
caddr_t call_data;
{
        OlFlatCallData *fcd = (OlFlatCallData *) call_data;
        String			label;

#ifdef DEBUG
	fprintf(stderr,"CBselect\n");
#endif
#ifdef debug
        /*
         * Get the resource value from the sub-object
         */
        OlVaFlatGetValues(
            wid,                            /* widget */
            fcd->item_index,  /* item_index */
			XtNlabel,       &label,
			0
        );

        fprintf(stderr,"set %s\n", label);
#endif
	if (wid == df->w_modem)
	    ModemFamily(SELECT,FamilyItemPtr+fcd->item_index);
	else if (wid == df->w_port)
	    PortNumber(SELECT,&ports[fcd->item_index]);
	else if (wid == sf->w_speed)
	    PortSpeed(SYSTEMS_TYPE, SELECT,&speeds[fcd->item_index]);
	else if (wid == df->w_speed)
	    PortSpeed(DEVICE_TYPE, SELECT,&speeds[fcd->item_index]);
	else if (wid == df->w_enabled)
	    PortEnabled(SELECT,&enabled[fcd->item_index], fcd->item_index);
	else if (wid == df->w_direction)
	    PortDirection(SELECT,&directions[fcd->item_index], fcd->item_index);
        return;
} /* CBselect */

/*
 * CBunselect() - use direct access to sub-object resources.  (The sub-object
 * data is actually maintained here in the application.)
 */
void
CBunselect(wid, client_data, call_data)
Widget  wid;
caddr_t client_data;
caddr_t call_data;
{
        OlFlatCallData  *fcd = (OlFlatCallData *) call_data;

#ifdef debug
        fprintf(stderr,
                "unset = %s\n",
		(wid == df->w_modem)
                ?FamilyItemPtr+fcd->item_index->label
		:ports[fcd->item_index].label
        );
#endif
        return;
} /*CBunselect */

void
FreeAndSaveHoldData(type)
int type;
{
	switch (type) {
	
	case 0:

		if (holdData.holdPortSpeed != NULL) {
			XtFree(holdData.holdPortSpeed);
			holdData.holdPortSpeed = NULL;
		}
		if (holdData.portSpeed != NULL)
			holdData.holdPortSpeed = strdup(holdData.portSpeed);
		break;
	case 1:

		if (holdData.holdPortNumber != NULL) {
			XtFree(holdData.holdPortNumber);
			holdData.holdPortNumber = NULL;
		}
		if (holdData.portNumber != NULL)
			holdData.holdPortNumber = strdup(holdData.portNumber);
			
		break;

	case 2:

		if (holdData.holdModemFamily != NULL) {
			XtFree(holdData.holdModemFamily);
			holdData.holdModemFamily = NULL;
		}
		if (holdData.modemFamily != NULL)
			holdData.holdModemFamily = strdup(holdData.modemFamily);
				break;

	case 3:

		if (holdData.holdPortEnabled != NULL) {
			XtFree(holdData.holdPortEnabled);
			holdData.holdPortEnabled = NULL;
		}
		if (holdData.portEnabled != NULL)
			holdData.holdPortEnabled = strdup(holdData.portEnabled);
		break;

	case 4:

	
		if (holdData.holdPortDirection != NULL) {
			XtFree(holdData.holdPortDirection);
			holdData.holdPortDirection = NULL;
		}
		if (holdData.portDirection != NULL)
			holdData.holdPortDirection = strdup(holdData.portDirection);
		break;

    }

}

typedef struct device_type {
	String code;
	String translation;
} device_type;

static char *device_aliases[4][8] = {
{	"com1", "tty00h", "tty00", "tty00s", "00","00h", "00s", NULL },
{	"com2", "tty01h", "tty01", "tty01s", "01","01h", "01s", NULL },
{	"tty02h", "tty02", "tty02s", "02","02h", "02s", NULL, NULL },
{	"tty03h", "tty03", "tty03s", "03","03h", "03s", NULL, NULL },

};

#define MAX_DEVICES 	4
#define MAX_ALIASES	8	

static char *bad_devices[] = { "mouse", 
			"mousecfg",
			"mousemon",
			"lp",
			"lp0",
			"lp1",
			"lp2",
			"mem",
			"kmem",
			"clock"
			};

			

static Boolean 
VerifyOtherPort()
{

char device[PATH_MAX];
char *str, *ptr;
int result, i;
struct stat stat_buf;

		/* port is not com1 or com2 */

	while (*holdData.portNumber == ' ')
		holdData.portNumber++;
	if (holdData.portNumber == NULL) {
		/* popup error poup that port is required */
		return INVALID;
	}
	/* exclude certain devices that are invalid */

	str = (char *) IsolateName(holdData.portNumber);
	for (i=0; i < XtNumber(bad_devices); i++) { 
	if (strcmp(str, bad_devices[i]) == 0)
		return INVALID;
	}
	/*  device must be in /dev/ or /dev/term/ or /dev/xt
		or /dev/pts */

	if (strncmp(holdData.portNumber, "/", 1) != 0) {
		/* no directory specified so assume /dev */
		strcpy(device, "/dev/");
		strcat(device, holdData.portNumber);
		if (DeviceExists(device)) {
			XtFree(holdData.portNumber);
			holdData.portNumber = strdup((String)device);
			return VALID;
		} 
		/* try other directories  - /dev/term */
		strcpy(device, "/dev/term/");
		strcat(device, holdData.portNumber);
		if (DeviceExists(device)) {
			XtFree(holdData.portNumber);
			holdData.portNumber = strdup((String)device);
			return VALID;
		}
		/* try other directories  - /dev/xt */
		strcpy(device, "/dev/xt/");
		strcat(device, holdData.portNumber);
		if (DeviceExists(device)) {
			XtFree(holdData.portNumber);
			holdData.portNumber = strdup((String)device);
			return VALID;
		}
		strcpy(device, "/dev/pts/");
		strcat(device, holdData.portNumber);
		if (DeviceExists(device)) {
			XtFree(holdData.portNumber);
			holdData.portNumber = strdup((String)device);
			return VALID;
		}
	return INVALID;
	}
	/* contains a path name so use it */
	if (DeviceExists(holdData.portNumber)) return VALID;
	
return INVALID;


}


void
ClearSelectionData()
{

	if (holdData.portSpeed) {
		XtFree(holdData.portSpeed);
		holdData.portSpeed = NULL;
	}
	if (holdData.holdPortSpeed) {
		XtFree(holdData.holdPortSpeed);
		holdData.holdPortSpeed = NULL;
	}
	if (holdData.portDirection) {
		XtFree(holdData.portDirection);
		holdData.portDirection = NULL;
	}
	if (holdData.holdPortDirection) {
		XtFree(holdData.holdPortDirection);
		holdData.holdPortDirection = NULL;
	}
	if (holdData.modemFamily) {
		XtFree(holdData.modemFamily);
		holdData.modemFamily = NULL;
	}
	if (holdData.holdModemFamily) {
		XtFree(holdData.holdModemFamily);
		holdData.holdModemFamily = NULL;
	}
	if (holdData.portNumber) {
		XtFree(holdData.portNumber);
		holdData.portNumber = NULL;
	}
	if (holdData.holdPortNumber) {
		XtFree(holdData.holdPortNumber);
		holdData.holdPortNumber = NULL;
	}
	if (holdData.holdPortEnabled) {
		XtFree(holdData.holdPortEnabled);
		holdData.holdPortEnabled = NULL;
	}
	if (holdData.portEnabled) {
		XtFree(holdData.portEnabled);
		holdData.portEnabled = NULL;
	}
}

SetSelectionData()
{

ClearSelectionData();
holdData.portNumber = holdData.holdPortNumber =
		strdup(((DeviceData *)df->select_op->objectdata)->portNumber);
	holdData.modemFamily = holdData.holdModemFamily =
		strdup(((DeviceData *) df->select_op->objectdata)->modemFamily);
	holdData.portDirection = holdData.holdPortDirection =
		strdup(((DeviceData *) df->select_op->objectdata)->portDirection);
	holdData.portSpeed = holdData.holdPortSpeed =
		strdup(((DeviceData *) df->select_op->objectdata)->portSpeed);
	holdData.portEnabled = holdData.holdPortEnabled =
		strdup(((DeviceData *) df->select_op->objectdata)->portEnabled);
}

static Boolean
Check4SpecialSpeeds(speed)
char *speed;
{
	if (speed == NULL) return INVALID;
	if ((holdData.portDirection) && 
		((strcmp(holdData.portDirection, "outgoing")) == 0))
		return VALID;
		
	if (Check_ttydefsSpeed(holdData.portSpeed) != GOOD_SPEED) {
		DeviceNotifyUser(df->toplevel, GGT(string_badSpeed2));
		return INVALID;
	}
}


static Boolean
Check4DuplicatePorts(currPort,oldPort, action)
char *currPort;
char *oldPort;
int action;
{

	DmObjectPtr op, objp;

	char *shortPort;
	char *oldShortPort;
	char *shortObjPort;
	Boolean exact_match_needed;
	int i, dev_idx, alias_idx;

	if (currPort == NULL) return INVALID;
	if ((shortPort = (char *) IsolateName(currPort)) == NULL) return INVALID;
	exact_match_needed = False;

#ifdef DEBUG
	fprintf(stderr,"Check4DuplicatePorts: currport=%s oldPort=%s action=%d  shortPort=%s\n",currPort,oldPort,action,shortPort);

#endif
		/* call FindAliasTable to find the index to the
		array that contains the aliases for this port */

	if ((dev_idx = FindAliasTable(shortPort)) == -1) {
		/* no aliases exist for this port */
		/* e.g. ttyp1 */
		exact_match_needed = True;
	}
	oldShortPort = oldPort;
	if (oldPort) oldShortPort = (char *) IsolateName(oldPort);

#ifdef DEBUG
fprintf(stderr,"Check4DuplicatPorts: currPort=%s oldPort=%s oldShortPort=%s\n",currPort,oldPort,oldShortPort);
#endif

	if ((df->request_type != B_ADD) &&
		(oldShortPort != NULL)) {
			/* we are changing the port but to an alias so
			that is ok */
		if (strcmp(shortPort, oldShortPort) == 0)  
			 return VALID;
			/* if exact match is needed because there are no
			aliases for this port then it is an error */
		if (exact_match_needed) return INVALID;	
			/* check for aliases */
		if (Check4PortAlias(oldShortPort, dev_idx) == True)
			return VALID;
		
	}
	
	/* look at all existing objects for a matching name
		or alias */
        for (objp=df->cp->op, i=0; objp; objp=objp->next, i++) {
			/* compare full name of object with the
				translated name */
			/* and compare full name of object with full name */
			/* if a match is found it is a duplicate */
		if (objp->name == NULL) continue;
		if ((strcmp(objp->name,shortPort) == 0)  ||
			/* compare full name of object with full name */
		 	(strcmp(objp->name, currPort) == 0)) {
                        /* duplicate */
			return INVALID;
                 }
		/* need to translate the objectname and compare also */
		/* this handles cases like /dev/tty01s is equivalent
			to com2, and /dev/tty01s is equivalent to
				/dev/tty01 etc. */
		if ((shortObjPort = (char *) IsolateName(objp->name)) 
			== NULL) continue;
		if (Check4PortAlias(shortObjPort, dev_idx) == True)
			/* found an alias so device already exists */
			return INVALID;
        } /* end for */
	return VALID;
}


int 
FindAliasTable(port)
char *port;
{
	int dev_idx, alias_idx = 0;
	int match = -1;

	if (port == NULL) return match;
	for (dev_idx = 0; dev_idx < MAX_DEVICES; dev_idx++) {
		for (alias_idx = 0; alias_idx < MAX_ALIASES; alias_idx++) {
			/* compare each alias in the current table */
			/* if table value is NULL we are at the end
				of that list so break */
			if (device_aliases[dev_idx][alias_idx] == NULL) 
				break; 
			if (strcmp(port, device_aliases[dev_idx][alias_idx]) 
				== 0)  return dev_idx;
				/* found a matching port so return
				the table index */
			} /* end for alias_idx */
	} /* end for dev_idx */
	return match;
}

Boolean
Check4PortAlias(name, dev_idx)
char *name;
int dev_idx;
{
	Boolean match = False;
	int alias_idx;
	if (name == NULL) return match;

	for (alias_idx = 0; alias_idx < MAX_ALIASES; alias_idx++) {
		/* compare each alias in the current table */
			/* if table value is NULL we are at the end
				of that list so break */
		if (device_aliases[dev_idx][alias_idx] == NULL) 
				break; 
		if (strcmp(name, device_aliases[dev_idx][alias_idx]) 
				== 0) {
				/* found a matching port so return
				True that an alias was found */
				return True;
			}
	}
	return match;

}


ResetPortInformation(type)
int type;
{

	/* error adding or changing the port so we need to
		restore the old port information */
	if (holdData.portNumber) XtFree(holdData.portNumber);
	holdData.portNumber = NULL;
	if (holdData.holdPortNumber != NULL)
		/* set back last prior valid value for field */
		holdData.portNumber = strdup(holdData.holdPortNumber);

	if (type == PORT) return;
		/* only port number had been changed so we are
		finished restoring the data */

	if (holdData.modemFamily) XtFree(holdData.modemFamily);
	holdData.modemFamily = NULL;
	if (holdData.holdModemFamily != NULL)
		/* set back last prior valid value for field */
		holdData.modemFamily = strdup(holdData.holdModemFamily);

	if (holdData.portSpeed) XtFree(holdData.portSpeed);
	holdData.portSpeed = NULL;
	if (holdData.holdPortSpeed != NULL)
		/* set back last prior valid value for field */
		holdData.portSpeed = strdup(holdData.holdPortSpeed);

	if (holdData.portDirection) XtFree(holdData.portDirection);
	holdData.portDirection = NULL;
	if (holdData.holdPortDirection != NULL)
		/* set back last prior valid value for field */
		holdData.portDirection = strdup(holdData.holdPortDirection);

}
