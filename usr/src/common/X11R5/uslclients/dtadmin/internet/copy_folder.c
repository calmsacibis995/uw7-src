#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/copy_folder.c	1.10.2.1"
#endif


#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#include <Xm/Label.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/FileGizmo.h>
#include <MGizmo/LabelGizmo.h>

#include <Dt/Desktop.h>
#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"
/*
#define DIRECTORY(f, s)  ((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFDIR))
#define  DMODE           (S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

#define MODE            (S_IRWXU | S_IRGRP | S_IROTH) 
*/

extern void	createMsg(Widget, msgType, char *, char *);
extern char	*mygettxt(char *);

void	copyCB();
void	helpCB();
void	cancel();
void 	InstallRemoteAccess();
void 	MakeSoftLink(Widget, char *, char *);

typedef struct _copyFolder {
	Widget	popup;
	Gizmo	handle1;
	char	*selectHost;		
	char	*link;
} copyFolder;

copyFolder	cf;

static char *title;
/*
 *
 * Install Network use icons (Remote Access) in the control room
 */
static void
InstallRemoteAccess (widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	FILE 			*attrp;
	static char 		node_directory[MEDIUMBUFSIZE];
	char 			target[MEDIUMBUFSIZE];
	static Boolean		first_time = True;
	struct stat		stat_buf;
	int			index, num;
	char			*getenv();
	struct utsname		sname;


	/* check to see if the local machine has been selected */
	uname(&sname);
	if (strcmp(cf.selectHost, sname.nodename) == 0) {
                createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_sameHost), title);
		return;
	}
	if (first_time) {
		first_time = False;
		sprintf (node_directory, "%s/.node", getenv("HOME"));
	}
	/* check the node directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(node_directory, stat_buf) ) {
		if (mkdir(node_directory, DMODE) == -1) {
                        createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_noNodeDir), title);
			return;
		}
		if (chown(node_directory, getuid(), getgid()) == -1) {
                        createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_noAccessNodeDir), title);
			return;
		}
	} else
	    if (access(node_directory, W_OK) < 0) {
                    createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_noAccessNodeDir), title);
		    return;
	    }

	/* Get the select host */
	sprintf (target, "%s/%s", node_directory, cf.selectHost);

	attrp = fopen( target, "w");
	if (attrp == (FILE *) 0) {
                createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_noAccessNodeDir), title);
		return;
	}

	/* put the node's properties here */
	fprintf( attrp, "SYSTEM_NAME=%s\n",
		cf.selectHost);

	(void) fflush(attrp);
	fclose( attrp );

	if (chmod( target, MODE) == -1) {
                createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_noAccessNodeDir), title);
		return;
	}
	if (chown( target, getuid(), getgid()) == -1) {
                createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_noAccessNodeDir), title);
		return;
	}

	MakeSoftLink(widget, cf.link, target); 
	XtPopdown(cf.popup); 

} /* InstallRemoteAccess() */


static void
MakeSoftLink(Widget wid, char *link, char *target)
{
	DtRequest	request;
	char	 	msg[MEDIUMBUFSIZE], buf[MEDIUMBUFSIZE];
	char		*toolbox, *tp;
	int		len;

	errno = 0;
	if(symlink( target, link) == -1) {
/*
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
*/
		sprintf(buf, mygettxt(ERR_installFailed), link);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		return;
	} else {
		if (lchown( link, getuid(), getgid()) == -1) {
			createMsg(hi.net.common.toplevel, ERROR, mygettxt(ERR_noAccessNodeDir), title);
			return;
		}
		sprintf(buf, mygettxt(INFO_installDone), link);
		createMsg(hi.net.common.toplevel, INFO, buf, title);
	}

	memset (&request, 0, sizeof (request));
	request.sync_folder.rqtype= DT_SYNC_FOLDER;
	request.sync_folder.path = link;
	(void) DtEnqueueRequest (XtScreen (hi.net.common.toplevel),
			_DT_QUEUE(XtDisplay(hi.net.common.toplevel)),
			_DT_QUEUE(XtDisplay(hi.net.common.toplevel)),
			XtWindow (hi.net.common.toplevel), &request);

} /* MakeSoftLink */

static void
cancelCB() {
	XtPopdown(cf.popup);;	
}

static void
helpCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	HelpInfo *	hInfo = (HelpInfo *)client_data;

	PostGizmoHelp(w, hInfo);
}

static void
verifyCB(Widget w, XtPointer clientData, XtPointer callData)
{
	int *	handle = clientData;

	ExpandFileGizmoFilename(*handle);
	cf.link = strdup(GetFilePath(*handle));
	printf("file = %s\n", cf.link);
	/* ResetFileGizmoPath(*handle); */
	InstallRemoteAccess(w, clientData, callData);
}

static HelpInfo	ctfHelp = {
	TXT_appName,
	TXT_appName,
	"dtadmin/inet.hlp",
	"220"
};

static MenuItems	saveItems[] = {
	{True, TXT_Copy,   MNE_Copy,  I_PUSH_BUTTON, NULL, verifyCB},
	{True, TXT_Cancel, MNE_Cancel,I_PUSH_BUTTON, NULL, cancelCB},
	{True, TXT_Help,   MNE_Help,  I_PUSH_BUTTON, NULL, helpCB, (XtPointer)&ctfHelp },
	{0, NULL}
};

static MenuGizmo	saveMenu = {
	&ctfHelp, "saveMenu", "Save", saveItems, NULL, NULL, XmHORIZONTAL, 1,
	0, 1
};

LabelGizmo	text1 = {
	NULL, "text1", NULL, True
};

GizmoRec textWin1[] = {
	{LabelGizmoClass,	&text1}
};
LabelGizmo	label1 = {
	NULL, "label1", TXT_copy, False, textWin1, XtNumber(textWin1)
};

GizmoRec upper[] = {
	{LabelGizmoClass,	&label1}
};

FileGizmo file = {
	NULL, "file", TXT_copyView, &saveMenu, upper, XtNumber(upper),
	NULL, NULL, TXT_to, TXT_as, FOLDERS_ONLY, NULL, ABOVE_LIST
};

void
createCopyFolder(w, client_data, cbs)
Widget			w;
int			client_data;
XmAnyCallbackStruct	*cbs;
{
	XmString	string;
	char		*buf, *getenv();
	Widget		copy_label;
	Arg		args[10];
	int		listIndex, wid_pos, i;
	dnsList		*cur_pos;
	static int	first_time = 1;

	if (first_time) {
		InitializeGizmos(TXT_appName);
		first_time = 0;
	}

	if (cf.handle1 == NULL) {
		buf = getenv("HOME");
		file.directory = strdup(buf);
		file.dialogType = FOLDERS_ONLY;
		saveItems[0].clientData = (XtPointer)&cf.handle1;
		cf.handle1 = CreateGizmo(XtParent(w), FileGizmoClass, &file, NULL, 0);
		cf.popup = GetFileGizmoShell(cf.handle1);
		title = mygettxt(TXT_copyView);
		XtVaSetValues(cf.popup, XmNtitle, title, NULL);
	}
	copy_label = QueryGizmo(FileGizmoClass, cf.handle1, 
			GetGizmoWidget, "text1");
	if (hi.net.common.cur_view == etcHost) {
		listIndex = ((hi.net.etc.etcHostIndex+(ETCCOLS-1))/ETCCOLS)-2;
		buf = strdup(hi.net.etc.etcHosts->list[listIndex].etcHost.name);
	}
	else {
		wid_pos = hi.net.dns.cur_wid_pos;
		cur_pos = hi.net.dns.cur_pos;
		for (i=0; i < wid_pos; i++)
			cur_pos = cur_pos->next;
		buf = strdup(hi.net.dns.dnsSelection[wid_pos]);
	}
	if (cf.selectHost)
		free(cf.selectHost);
	cf.selectHost = strdup(buf);
	string = XmStringCreateSimple(buf);	
	
	XtSetArg(args[0], XmNlabelString, string);
	XtSetArg(args[1], XmNalignment, XmALIGNMENT_BEGINNING);
	XtSetValues(copy_label, args, 2);
	XmStringFree(string);
	SetFileGizmoInputField(cf.handle1, buf);
	SelectFileGizmoInputField(cf.handle1);
	MapGizmo(FileGizmoClass, cf.handle1);
}
