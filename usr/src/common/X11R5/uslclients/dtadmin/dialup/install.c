/*		copyright	"%c%" 	*/

#ident	"@(#)dtadmin:dialup/install.c	1.14.1.13"

/* Popup Controls for installing network use icons */

#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <PopupWindo.h>
#include <Caption.h>
#include <TextField.h>
#include <FButtons.h>
#include <ControlAre.h>
#include <StaticText.h>
#include <ScrolledWi.h>
#include <FList.h>
#include <FButtons.h>
#include <MenuShell.h>
#include "fileinfo.h"
#include "uucp.h"
#include "dtcopy.h"
#include "error.h"

#define	FN	1
#define	QD	2
#define VIEW_HEIGHT	6

extern void callRegisterHelp(Widget, char * , char *);
extern char *		ApplicationName;
extern Filelist * ReadDirectory();
extern void       ListSelect();
extern void		NotifyUser ();
extern void		DeviceNotifyUser ();
extern void             DisallowPopdown();
extern void             HelpCB();
extern Boolean		IsSystemFile();
void			InstallCB();

static void		MakeSoftLink(Widget, char *, char *, char *);
static void		Dispatch();
static void		InstallPort();
static void		InstallFriendNode();
static void		Cancel();

static Widget           PopupWindow;


static HelpText AppHelp = {
    title_hinstall, HELP_FILE, help_install,
};

static Items installItems [] = {
    { Dispatch, NULL, (XA)TRUE}, /* Install */
    { Cancel, NULL, (XA)TRUE},   /* Cancel */
    { HelpCB, NULL, (XA)TRUE, NULL, NULL, (XA)&AppHelp},
};

static Menus installMenu = {
	"install",
	installItems,
	XtNumber (installItems),
	False,
	OL_FIXEDROWS,
	OL_NONE,
	NULL
};


static FileInfo *file_info;

static char * listFields[] = {XtNlabel};



static void
Dispatch(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	int *which;

	XtVaGetValues(PopupWindow, XtNuserData, &which, 0);

	switch (*which) {
		case FN:

			InstallFriendNode (wid, client_data, call_data);
			callRegisterHelp(sf->toplevel, title_setup, help_setup);
			break;
		case QD:
			InstallPort (wid, client_data, call_data);
			callRegisterHelp(df->toplevel, title_setup, help_device);
			break;
		default:
			break;
	}
	
	XtPopdown(PopupWindow);
}

static void
Cancel(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	callRegisterHelp(sf->toplevel, title_setup, help_setup);
	XtPopdown(PopupWindow);
}

void
InstallCB (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	HostData		*dp = NULL;
	String port;
	String port_type;
	static char *dev=NULL;
	int *type;
	Widget      button_area;
	Widget      prompt_area;
	Widget      footer_area;
	static Widget label1,label3, label5, subdir;
	static char buf[128];

	type = (int *) client_data;

    ClearFooter(df->footer); /* clear mesaage area */
    ClearFooter(sf->footer); /* clear mesaage area */

	/* If first time, create the popup and its controls */
	if (!PopupWindow) {
		file_info = malloc(sizeof(FileInfo));
		file_info->name = NULL;
		file_info->list = NULL;
		file_info->directory = strdup(sf->userHome);
		SET_HELP(AppHelp);
		sprintf(buf, "%s: %s", ApplicationName, GGT(title_copyToFolder));
		PopupWindow = XtVaCreatePopupShell ("install",
			popupWindowShellWidgetClass, widget,
			XtNpushpin,             (XtArgVal) OL_NONE,
			XtNborderWidth, 0,
			XtNtitle,		buf,
			0);

		XtAddCallback (
			PopupWindow,
			XtNverify,
			DisallowPopdown,
			(XtPointer)0
		);

		XtVaGetValues (PopupWindow,
			XtNlowerControlArea,    (XtArgVal) &button_area,
			XtNupperControlArea,    (XtArgVal) &prompt_area,
			XtNfooterPanel,		(XtArgVal) &footer_area,
			0);
		file_info->controlWidget =  XtVaCreateManagedWidget(
			"controlarea",
			controlAreaWidgetClass,
			prompt_area,
			XtNmeasure, 1,
			XtNlayoutType, OL_FIXEDCOLS,
			XtNborderWidth, 0,
			(String) 0);

			
		label1 = XtVaCreateManagedWidget(
			"label1",
			captionWidgetClass,
			file_info->controlWidget,
			XtNlabel, GGT(label_copyto),
			(String) 0
		);
		file_info->curItemWidget = XtVaCreateManagedWidget(
			"label2",
			staticTextWidgetClass,
			label1,
			(String) 0
		);
		label3 = XtVaCreateManagedWidget(
			"label3",
			captionWidgetClass,
			file_info->controlWidget,
			XtNlabel, GGT(label_to),
			(String) 0
		);
		file_info->curPathWidget = XtVaCreateManagedWidget(
			"label4",
			staticTextWidgetClass,
			label3,
			(String) 0
		);

		file_info->textFieldCaption = XtVaCreateManagedWidget(
			"caption",
			captionWidgetClass,
			file_info->controlWidget,
			XtNlabel, GGT(label_as),
			XtNborderWidth, 0,
			(String) 0
		);

		sprintf(buf, "%s", sf->userHome);
		file_info->textFieldWidget = XtVaCreateManagedWidget(
			"input",
			textFieldWidgetClass,
			file_info->textFieldCaption,
			XtNstring, buf,
			XtNborderWidth, 0,
			XtNcharsVisible, 20,
			(String) 0
		);

		label5 = XtVaCreateManagedWidget(
			"label5",
			captionWidgetClass,
			file_info->controlWidget,
			XtNlabel, GGT(label_folders),
			(String) 0
		);

		file_info->curFolderWidget = XtVaCreateManagedWidget(
			"label6",
			staticTextWidgetClass,
			label5,
			(String) 0
		);	
		subdir = XtVaCreateManagedWidget("iscrollwin",
			scrolledWindowWidgetClass,
			file_info->controlWidget,
			0);

		FixDirectory(file_info);
		file_info->list = 
			ReadDirectory(file_info, file_info->directory);

		file_info->subdirListWidget = XtVaCreateManagedWidget("flist",
			flatListWidgetClass,
			subdir,
			XtNitems, file_info->list->dirs,
			XtNnumItems, file_info->list->dir_used,
			XtNexclusives, True,
			XtNnoneSet, True,
			XtNitemFields, listFields,
			XtNnumItemFields, XtNumber(listFields),
			XtNselectProc, ListSelect,
			XtNviewHeight, VIEW_HEIGHT,
			XtNclientData, file_info,
			0);


		SET_LABEL (installItems,0,copy);
		SET_LABEL (installItems,1,cancel);
		SET_LABEL (installItems,2,help);
		AddMenu (button_area, &installMenu, False);

		callRegisterHelp(PopupWindow, title_hinstall, help_install);
	}

	XtVaSetValues(file_info->textFieldWidget, XtNstring, sf->userHome, 0);
	SetFileCriteria(file_info, sf->userHome);
#ifdef DEBUG
	fprintf(stderr,"type = %d\n", *type);
#endif
	if (*type == 1) {
			/* install systems file */
		/* Nothing to operate on, just return */
		if (sf->numFlatItems == 0) { /* nothing to operate */
			PUTMSG(GGT(string_noItem));
			return;
		}
		/* Get the select host */
		dp = sf->flatItems[sf->currentItem].pField;
	
			/* label for current system being installed */
		file_info->name = dp->f_name;
		XtVaSetValues(file_info->textFieldWidget, XtNstring, dp->f_name, 0);
		XtVaSetValues(file_info->curItemWidget, XtNstring, dp->f_name, 0);
	

	} else  {

		/* installing device file */

        	port = ((DeviceData*)(df->select_op->objectdata))->portNumber;
        	port_type = ((DeviceData*)(df->select_op->objectdata))->modemFamily;
	
        	if ((port == NULL) || (port_type == NULL)) {
                	DeviceNotifyUser(df->toplevel, GGT(string_noSelect));
                	return;
        	}

	
		if (df->select_op->name) {
			/* malloc extra space in case e.g. 00 needs to be
				overwritten with com1 */
#ifdef DEBUG_MALLOC
		fprintf(stderr,"InstallCB before malloc\n");
#endif
			if (dev)free(dev);
			dev = malloc(strlen(df->select_op->name) +3 );
			strcpy (dev, df->select_op->name);
			if (strncmp(dev, "/dev/", 5) == 0)
				/* strips off leading "/dev/" */
				strcpy(dev, &(dev[5]));
			if (strncmp(dev,"term/",5) == 0)
				/* strips off leading "term/" */
				strcpy(dev, &(dev[5]) );
			if ((!strcmp(dev, "tty00h")) ||
				(!strcmp(dev, "00h")))
					strcpy(dev,COM1);
			else if ((!strcmp(dev, "tty01h")) ||
				(!strcmp(dev, "01h")))
					strcpy(dev, COM2);
		} else {
		 	strcpy(dev, "");
		}
		file_info->name = dev;
			/* label for current system being installed */
		XtVaSetValues(file_info->curItemWidget, XtNstring, dev, 0);
			/* set same value in text field */
		XtVaSetValues(file_info->textFieldWidget, XtNstring, dev, 0);
 
	}
	SetValue(PopupWindow, XtNuserData, client_data);
	XtPopup (PopupWindow, XtGrabExclusive);
} /* InstallCB */

/*
 *
 * Install Network use icons (friend nodes) in the control room
 */
static void
InstallFriendNode (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	FILE 			*attrp;
	HostData		*dp = NULL;
	static char 		node_directory[PATH_MAX];
	char 			target[PATH_MAX];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;
	
	char *fullname=NULL;
	char *name=NULL;



#ifdef DEBUG
	fprintf(stderr,"InstallFriendNode name=%s\n",name);
#endif
	/* Nothing to operate on, just return */
	if (sf->numFlatItems == 0) { /* nothing to operate */
		PUTMSG(GGT(string_noItem));
		return;
	}
	/* Get the select host */
	dp = sf->flatItems[sf->currentItem].pField;

	if (dp == NULL) { 
                NotifyUser(sf->toplevel, GGT(string_noSelect));
		return;
	}

	XtVaGetValues (file_info->textFieldWidget, XtNstring, &fullname, (String)0);

	/* need to see if name is a full path or just a filename */
		/* find last / and get name after it */
	name = strrchr(fullname, '/');
			/* if no / found then name is a simple name so
			so we need to get the path from the scroll list */
	if (name == NULL) {
		name = fullname;	
	} else {
			/* increment past the / character */
		name++;
		if (name == NULL) name = fullname;
	}

	/* check to see if the local machine has been selected */
	if (strcmp(name, sf->nodeName) == 0) {
                NotifyUser(sf->toplevel, GGT(string_sameNode));
		XtFree(fullname);
		return;
	}
	/* need to see if name is a full path or just a filename */
	if (first_time) {
		first_time = False;
		sprintf (node_directory, "%s/.node", sf->userHome);
	}
	/* check the node directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(node_directory, stat_buf) ) {
		if (mkdir(node_directory, DMODE) == -1) {
                        NotifyUser(sf->toplevel, GGT(string_noAccessNodeDir));
			XtFree(fullname);
			return;
		}
		if (chown(node_directory, getuid(), getgid()) == -1) {
                        NotifyUser(sf->toplevel, GGT(string_noAccessNodeDir));
			XtFree(fullname);
			return;
		}
	} else
	    if (access(node_directory, W_OK) < 0) {
                    NotifyUser(sf->toplevel, GGT(string_noAccessNodeDir));
			XtFree(fullname);
		    return;
	    }

#ifdef debug
	fprintf(stderr,"the DMODE is: %o\n", DMODE);
	fprintf(stderr,"the mode for %s is: %o\n", node_directory,
						stat_buf.st_mode);
#endif
	/* Get the select host */
	sprintf (target, "%s/%s", node_directory, name);

	attrp = fopen( target, "w");
	if (attrp == (FILE *) 0) {
                NotifyUser(sf->toplevel, GGT(string_noAccessDir));
		XtFree(fullname);
		return;
	}

	/* put the node's properties here */
	fprintf( attrp, "SYSTEM_NAME=%s\n", name);
	fprintf( attrp, "LOGIN_NAME=%s\n", sf->userName);
	fprintf( attrp, "DISPLAY_CONN=\n");
	fprintf( attrp, "TRANSFER_FILE_TO=\n");
	fprintf( attrp, "TRANSFER_FILE_USING=\n");
	fprintf( attrp, "COPY_FILE_TO=\n");
	fprintf( attrp, "ALWAYS_CONFIRM=\n");
	fprintf( attrp, "CONN_CONFIRM=\n");
	fprintf( attrp, "FTP_CONFIRM=\n");
	fprintf( attrp, "XFER_OPTION=%s\n", "UUCP");

	(void) fflush(attrp);
	fclose( attrp );

	if (chmod( target, MODE) == -1) {
                NotifyUser(sf->toplevel, GGT(string_noAccessDir));
		XtFree(fullname);
		return;
	}
	if (chown( target, getuid(), getgid()) == -1) {
                NotifyUser(sf->toplevel, GGT(string_noAccessDir));
		XtFree(fullname);
		return;
	}


	MakeSoftLink(widget, fullname, name, target);

	XtFree(fullname);
} /* InstallFriendNode() */

static void
InstallPort (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	FILE 			*attrp;
	static char 		port_directory[PATH_MAX];
	char 			target[PATH_MAX];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;
	char			*dev;
	char			*fullname=NULL;
	char			*name=NULL;



	String  port = ((DeviceData*)(df->select_op->objectdata))->portNumber;
	String  type = ((DeviceData*)(df->select_op->objectdata))->modemFamily;

#ifdef DEBUG
	fprintf(stderr,"InstallPort name=%s\n",name);
#endif
	if ((port == NULL) || (type == NULL)) {
                DeviceNotifyUser(df->toplevel, GGT(string_noSelect));
		return;
	}
	if (first_time) {
		first_time = False;
		sprintf (port_directory, "%s/.port", sf->userHome);
	}
	/* check the port directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(port_directory, stat_buf) )
		mkdir(port_directory, DMODE);
	else
	    if (access(port_directory, W_OK) < 0) {
                    DeviceNotifyUser(df->toplevel, GGT(string_noAccessPortDir));
		    return;
	    }

#ifdef debug
	fprintf(stderr,"the MODE is: %o\n", DMODE);
	fprintf(stderr,"the mode for %s is: %o\n", port_directory,
						stat_buf.st_mode);
#endif
	XtVaGetValues (file_info->textFieldWidget, XtNstring, &fullname, (String)0);
	/* need to see if name is a full path or just a filename */
		/* find last / and get name after it */
	name = strrchr(fullname, '/');
		/* if no / found then name is a simple name so
		set it to fullname */
	if (name == NULL) {
		name = fullname;	
	} else {
			/* increment past the / character */
		name++;
		if (name == NULL) name = fullname;
	}
	sprintf(target, "%s/%s", port_directory, name);

#ifdef DEBUG
	fprintf(stderr,"InstallPort target=%s\n",target);
#endif
	/* Get the select host */
	attrp = fopen( target, "w");
	if (attrp == (FILE *) 0) {
                DeviceNotifyUser(df->toplevel, GGT(string_noAccessPortDir));
		XtFree(fullname);
		return;
	}
	if (chmod( target, MODE) == -1) {
                DeviceNotifyUser(df->toplevel, GGT(string_noAccessPortDir));
		XtFree(fullname);
		return;
	}

	/* put the node's properties here */
	fprintf( attrp, "PORT=%s\n", port);
	fprintf( attrp, "TYPE=%s\n", type);
	(void) fflush(attrp);
	fclose( attrp );

	MakeSoftLink(widget, fullname, name, target);
#ifdef DEBUG_MALLOC
	fprintf(stderr,"InstallPort before XtFree\n");

	XtFree(name);
#endif
}

static void
MakeSoftLink(Widget wid, char *fname,char * name, char *target)
{
	DtRequest	request;
	char	 	msg[BUFSIZ];
	char	 	link[PATH_MAX];
	char		*toolbox, *tp;
	Widget		w;
	int		len;


	/* strip off leading spaces if any */
	if (name == fname) 
		sprintf (link, "%s/%s", file_info->directory, name);
	else
		strcpy (link, fname);
#ifdef DEBUG_MALLOC
	fprintf(stderr,"MakeSoftLInkbefore XtFree \n");
#endif

	errno = 0;
#ifdef DEBUG
	fprintf(stderr,"making symlink from %s to %s\n", target, link);
#endif
	if(symlink( target, link) == -1) {
		switch (errno) {
		case EACCES:
		case ENOENT:
			sprintf(msg, GGT(string_pathDeny),
				link);
			break;
		case EEXIST:
			sprintf(msg, GGT(string_pathExist),
				fname);
			break;
		case ENAMETOOLONG:
			sprintf(msg, GGT(string_longFilename),
				fname);
			break;
		default:
			sprintf(msg, GGT(string_installFailed),
				link);
			break;
		}
		w = (Widget) getShell(wid);
		/* see if shell is for systems file */
		if (w == df->toplevel) {
			DeviceNotifyUser(df->toplevel, msg);
		} else {
                       	NotifyUser(w,msg);
		}
		return;
	} else {
		if (lchown( link, getuid(), getgid()) == -1) {
			w = (Widget) getShell(wid);
			/* see if shell is for systems file */
			if (w == df->toplevel) {
				DeviceNotifyUser(df->toplevel, 
					GGT(string_noAccessNodeDir));
			} else {
                        	NotifyUser(w, GGT(string_noAccessNodeDir));
			}
			return;
		}
		sprintf(msg, GGT(string_installDone),
			fname);
		PUTMSG(msg);
	}

	memset (&request, 0, sizeof (request));
	request.sync_folder.rqtype= DT_SYNC_FOLDER;
	request.sync_folder.path = link;
	(void) DtEnqueueRequest (XtScreen (wid),
			_DT_QUEUE(XtDisplay(wid)),
			_DT_QUEUE(XtDisplay(wid)),
			XtWindow (wid), &request);

} /* MakeSoftLink */

void
FreeInstallList()
{
	if (file_info == NULL) return;
	FreeLists(file_info);
	if (file_info->directory) XtFree(file_info->directory);
}
