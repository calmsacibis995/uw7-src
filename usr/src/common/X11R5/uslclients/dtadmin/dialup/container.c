/*		copyright	"%c%"	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/container.c	1.26.1.14"
#endif

/******************************file*header********************************

	Description:
	    This routine uses the flattened icon container widget
	(aka. FlatList)
	    to display the contents of the /etc/uucp/Devices file.
*/

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ScrolledWi.h>
#include <X11/Shell.h>		/* for XtNminWidth and XtNminHeight */

#include <Dt/Desktop.h>
#include <libDtI/DtI.h>
#include <libDtI/FIconBox.h>
#include <sys/stat.h>
#include <unistd.h>
#include "uucp.h"
#include "error.h"

/* 40755 */
extern void	GetContainerItems();
extern void	AlignIcons();
extern XtArgVal	GetValue();
extern caddr_t	PortNumber();
extern caddr_t	ModemFamily();
extern char *	_path;
extern char *	device_path;
extern void     CBProperty();
extern void     CreateIconCursor();
extern void     NotifyUser();

extern		char *incoming_device_path;
extern		char *disabled_device_path;
void		SelectProc(Widget, XtPointer, XtPointer);

XFontStruct	*font;

Arg arg[50];
static char **target;


static void
CleanUpCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	char msg[BUFSIZ];
	sprintf(msg, GGT(string_installDone),
		*target);
    ClearFooter(df->footer); /* clear mesaage area */
    ClearFooter(sf->footer); /* clear mesaage area */

	FooterMsg(df->footer, "%s", msg);
	if (*target) XtFree (*target);
} /* CleanUpCB */

static void
DropProcCB(w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
	FILE			*attrp;
	OlFlatDropCallData	*drop = (OlFlatDropCallData *)call_data;
	static char 		port_directory[PATH_MAX];
	char 			buf[PATH_MAX];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;
	DmItemPtr		itemp;
	DmObjectPtr		op;
	char			*port, *type;
	char 			*dev;

	if (first_time) {
		first_time = False;
		sprintf (port_directory, "%s/.port", sf->userHome);
	}
	ClearFooter(df->footer);
    ClearFooter(sf->footer); /* clear mesaage area */
	/* check the port directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(port_directory, stat_buf) )
		mkdir(port_directory, DMODE);
	else
		if ( stat_buf.st_mode != DMODE ) {
			NotifyUser(sf->toplevel, GGT(string_noAccessPortDir));
			return;
		}

#ifdef debug
	fprintf(stderr,"the DMODE is: %o\n", DMODE);
	fprintf(stderr,"the mode for %s is: %o\n", port_directory,
						stat_buf.st_mode);
#endif

	itemp = (DmItemPtr ) drop->item_data.items;
	index = drop->item_data.item_index;
	op = (DmObjectPtr)OBJECT_PTR(itemp + index);
	num = drop->item_data.num_items;
	if ((char *)itemp[index].label != NULL) {
		dev = malloc(strlen((char *)itemp[index].label)+3);
		strcpy(dev, (char *)itemp[index].label);
		if (strncmp(dev, "/dev/", 5) == 0)
			/* strip off leading "/dev/" */
			strcpy(dev, &(dev[5]));
		if (strncmp(dev, "term/", 5) == 0)
			/* strip off leading "term/" */
			strcpy(dev, &(dev[5]));
		if ((!strcmp(dev, "tty00h")) ||
			(!strcmp(dev, "00h")))
                                 strcpy(dev, COM1);
		else if ((!strcmp(dev, "tty01h")) ||
			(!strcmp(dev, "01h")))
                                 strcpy(dev, COM2);
	} else
		strcpy(dev, "");
	if (num = 1) {
		sprintf (buf, "%s/%s", port_directory, dev);
		port = strdup(((DeviceData *)op->objectdata)->portNumber);
		type = strdup(((DeviceData *)op->objectdata)->modemFamily);
		target = (char **)malloc(sizeof(char *) * (1 + 1));
		*(target + 1) = NULL;
		*target = strdup(buf);
		attrp = fopen( *target, "w");
		if (attrp == (FILE *) 0) {
                        NotifyUser(sf->toplevel, GGT(string_noAccessPortDir));
			return;
		}

		if (chmod( *target, MODE) == -1) {
                        NotifyUser(sf->toplevel, GGT(string_noAccessPortDir));
			return;
		}


		/* put the node's properties here */

		fprintf( attrp, "PORT=%s\n", port);
		fprintf( attrp, "TYPE=%s\n", type);
		(void) fflush(attrp);
		fclose( attrp );
		XtFree(port);
		XtFree(type);
	} else {
		fprintf (stderr, "Multiple installation not supported");
		return;
	}

	DtNewDnDTransaction(
		w,				/* owner widget */
		target,				/* file list */
		DT_B_STATIC_LIST,		/* options */
		drop->root_info->root_x,	/* root x */
		drop->root_info->root_y,	/* root y */
		drop->ve->xevent->xbutton.time,	/* timestamp */
		drop->dst_info->window,		/* dst window */
		DT_LINK_OP,			/* dnd hint */
		NULL,				/* del proc */
		CleanUpCB,			/* state proc */
		(XtPointer) *target		/* client data */
		);
} /* DropProcCB */

void
InitContainer(form, reference)
Widget form;
Widget reference;
{
	extern XFontStruct	*font;
	extern XFontStruct	* _OlGetDefaultFont();
	Widget			swin;

	ClearFooter(df->footer);
	font = _OlGetDefaultFont(form, OlDefaultFont);
	GetContainerItems(device_path, DEVICES_OUTGOING);
	GetContainerItems(incoming_device_path, DEVICES_INCOMING);
	GetContainerItems(disabled_device_path, DEVICES_DISABLED);

	    /* Create icon container        */

	XtSetArg(arg[0], XtNselectProc, (XtArgVal)SelectProc);
	XtSetArg(arg[2], XtNmovableIcons, (XtArgVal)False);
	XtSetArg(arg[3], XtNexclusives, True);
	XtSetArg(arg[4], XtNdropProc, DropProcCB);
	XtSetArg(arg[5], XtNdrawProc, (XtArgVal)DmDrawIcon);
	XtSetArg(arg[6], XtNminWidth, (XtArgVal)1);
	XtSetArg(arg[7], XtNminHeight, (XtArgVal)1);
        XtSetArg(arg[8], XtNdblSelectProc, (XtArgVal)CBProperty);
        XtSetArg(arg[9], XtNdragCursorProc, (XtArgVal)CreateIconCursor);
	df->iconbox = DmCreateIconContainer(form,
		DM_B_NO_INIT | DM_B_CALC_SIZE,
		arg,
		10,
		df->cp->op,
		df->cp->num_objs,
		&df->itp,
		df->cp->num_objs,
		&swin,
		NULL,
		font,
		1);

	AlignIcons();

	XtVaSetValues(swin,
		XtNyResizable,	True,
   		XtNxResizable,	True,
		XtNxAttachRight,  True,
		XtNyAttachBottom, True,
		XtNyAddHeight,	  True,
		XtNyAttachOffset, OFFSET,
		XtNyRefWidget,	  reference,
		(String)0
	);

	if (df->cp->num_objs == 0) 
		FooterMsg(df->footer, "%s", GGT(string_noDevices));
} /* InitContainer */

void
AlignIcons()
{
	Cardinal	i;
	DmItemPtr	item;
	Dimension	width;
	Cardinal	nitems;

	Position	icon_x, icon_y;


	nitems = GetValue(df->iconbox, XtNnumItems);
	width = GetValue(df->iconbox, XtNwidth);
#ifdef NITEMS
		fprintf(stderr, "In AlignIcons, nitems = %d\n", nitems);
		fprintf(stderr, "In AlignIcons, width = %u\n", width);
#endif
	if ( width < 30 || width > 800)
		width = (Dimension) BNU_WIDTH - 30;
	else
		width -= 30; 
#ifdef NITEMS
		fprintf(stderr, "In AlignIcons, nitems = %d\n", nitems);
		fprintf(stderr, "In AlignIcons, width = %u\n", width);
#endif
	for (i = 0, item = df-> itp; i < nitems;i++, item++)
		item-> x = item-> y = UNSPECIFIED_POS;

	for (i = 0, item = df-> itp; i < nitems;i++, item++)
		if (ITEM_MANAGED(item)) {
			DmGetAvailIconPos(df->itp, nitems,
				  ITEM_WIDTH(item), ITEM_HEIGHT(item), width,
				  INC_X, INC_Y,
				  &icon_x, &icon_y );

			item->x = (XtArgVal)icon_x;
			item->y = (XtArgVal)icon_y;
		}

	SetValue(df-> iconbox, XtNitemsTouched, True);
} /* AlignIcons */

void
SelectProc(w, client_data, call_data)
Widget	      w;
XtPointer	   client_data;
XtPointer	   call_data;
{
	String type;

	OlFIconBoxButtonCD *d = (OlFIconBoxButtonCD *)call_data;

	ClearFooter(df->footer);
#ifdef DEBUG
	printf("SelectProc(idx=%d, cnt=%d)\n",d->item_data.item_index,d->count);
#endif
	df->select_op = OBJECT_CD(d->item_data);
	SetDevSensitivity();
	SetSelectionData();
#ifdef DEBUG
	printf("SelectProc(df->select_op=%0x)\n",df->select_op);
#endif
	PortNumber(SET,
		((DeviceData*)(df->select_op->objectdata))->portNumber);
	ModemFamily(SET,
		((DeviceData*)(df->select_op->objectdata))->modemFamily);
#ifdef DEBUG
	fprintf(stderr,"Setting PortSpeed from SelectProc\n");
#endif
	PortSpeed(DEVICE_TYPE, SET,
		((DeviceData*)(df->select_op->objectdata))->portSpeed);
#ifdef DEBUG
	fprintf(stderr,"Setting PortDirection from SelectProc\n");
#endif
	PortDirection(SET,
		((DeviceData*)(df->select_op->objectdata))->portDirection);
	PortEnabled(SET,
		((DeviceData*)(df->select_op->objectdata))->portEnabled);
	if (df->w_acu == NULL) return;
	type = ((DeviceData*)(df->select_op->objectdata))->modemFamily;
	if (strcmp(type, "datakit") != 0 && strcmp(type, "direct") != 0)
		if (XtIsRealized(df->w_acu))
			XtMapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, TRUE, NULL);
	else
		if (XtIsRealized(df->w_acu))
			XtUnmapWidget(df->w_acu);
		else
			SetValue(df->w_acu, XtNmappedWhenManaged, False, NULL);
} /* SelectProc */
