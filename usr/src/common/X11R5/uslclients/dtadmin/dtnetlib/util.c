#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:util.c	1.12"
#endif


#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#include <Xm/DialogS.h>
#include <Xm/PushBG.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/PanedW.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleBG.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/SeparatoG.h>
#include <X11/cursorfont.h>

#include <Dt/Desktop.h>

#include "DesktopP.h"

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "lookupMsg.h"

/* fcn prototypes */
/* external */
extern void	createMsg(Widget, msgType, char *, char *);

/* static */
static void	turnOnCursorCB(Widget, XtPointer, XtPointer);
static void	turnOffCursorCB(Widget, XtPointer, XtPointer);
static void	restrictCB(Widget, XtPointer, XtPointer);

char *
mygettxt(char * label)
{
	char	*p;
	char	c;
	
	if (label == NULL)
		return(FALSE);
	for (p = label; *p; p++)
		if (*p == '\001') {
			c = *p;
			*p++ = 0;
			label = (char *)gettxt(label, p);
			*--p = c;
			break;
		}
		
	return(label);
}

XmString
i18nString(char	*str)
{
	XmString tmp;
	return(XmStringCreateLocalized(mygettxt(str)));
}

XmString
createString(char *str)
{
	XmString tmp;
	return(XmStringCreateLocalized(str));
}

void
freeString(XmString	str)
{
	XmStringFree(str);
}

void setLabel(Widget w, char *str)
{
    XmString tmp;

    tmp = XmStringCreateLtoR(mygettxt(str), "charset1");
    XtVaSetValues(w, XmNlabelString, tmp, NULL);
    XmStringFree(tmp);

}

void setSLabel(Widget w, char *str)
{
    XmString tmp;

    tmp = XmStringCreateLocalized(mygettxt(str)); 
    XtVaSetValues(w, XmNlabelString, tmp, NULL);
    XmStringFree(tmp);
}


GetMaxWidth(w, label_arr)
Widget	w;
char	*label_arr[];
{
	int		i=0;
	int		MaxWidth=0, len;
	Widget		label;
	XmString	string;	
	XtWidgetGeometry	size;
	
	
	while (strlen(mygettxt(label_arr[i])) != 0) {
		string = XmStringCreateLocalized(label_arr[i]);
		label = XtVaCreateManagedWidget("label",
			xmLabelWidgetClass, w, 
			XmNlabelString, string,
			NULL);			
		size.request_mode = CWWidth;
		XtQueryGeometry(label, NULL, &size);
		len = size.width;
		if ( len > MaxWidth ) 
			MaxWidth=len;
		i++;
		XmStringFree(string);
		XtDestroyWidget(label);
	}
	return(MaxWidth);
}

Widget
crt_inet_addr(parent, labelstr, maxlen, addr)
Widget		parent;
char		*labelstr;
int		maxlen;
inetAddr	*addr;
{
	Widget	form, label, rc, dot;
	int i;

	form = XtVaCreateManagedWidget("form",
		xmFormWidgetClass, parent,
		NULL);
	label = XtVaCreateManagedWidget("label",
		xmLabelWidgetClass, form,
		NULL);
	setLabel(label, labelstr);		
	XtVaSetValues(label, XmNwidth, maxlen,
		XmNalignment, XmALIGNMENT_END,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	rc = XtVaCreateManagedWidget("rc",
		xmRowColumnWidgetClass, form,
		XmNorientation, XmHORIZONTAL,
		NULL);	
	XtVaSetValues(rc, XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	for(i=0;i < NUMADDR; i ++){
		addr->addr[i] = XtVaCreateManagedWidget("addr",
			xmTextFieldWidgetClass, rc,
		XmNcolumns, 3,
		XmNmaxLength, 3,
		XmNcursorPositionVisible, False,
		NULL);
		if(i  != NUMADDR -1){
			dot = XtVaCreateManagedWidget("dot",
			xmLabelWidgetClass, rc,
				NULL);
			setLabel(dot, TXT_dot);
		}
		XtAddCallback(addr->addr[i], XmNfocusCallback, turnOnCursorCB, NULL);
		XtAddCallback(addr->addr[i], XmNlosingFocusCallback, turnOffCursorCB, NULL);
		XtAddCallback(addr->addr[i], XmNmodifyVerifyCallback, restrictCB, NULL);
        }
	return(form);
}

static void
turnOnCursorCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtVaSetValues(w, XmNcursorPositionVisible, True, NULL);
}

static void
turnOffCursorCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtVaSetValues(w, XmNcursorPositionVisible, False, NULL);
}

static void
restrictCB(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmTextVerifyPtr cbs = (XmTextVerifyPtr)call_data;

	if (isspace(*cbs->text->ptr)) {
		cbs->doit = False;
	}

	return;
}

Widget 
crt_inputlabel(parent, labelstr, maxlen)
Widget	parent;
char	*labelstr;
int	maxlen;
{
	Widget		form, label, textf;

	form = XtVaCreateManagedWidget("form",
		xmFormWidgetClass, parent,
		NULL);
	
	label = XtVaCreateManagedWidget("label",
		xmLabelWidgetClass, form,
		NULL);
	setLabel(label, labelstr);
	XtVaSetValues(label, XmNwidth, maxlen,
		XmNalignment, XmALIGNMENT_END,
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);
	textf = XtVaCreateManagedWidget("textf",
		xmTextFieldWidgetClass, form,
		NULL);
	XtVaSetValues(textf, XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, label,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	return(textf);
}

    /* Set the label of the widget with the string passed */

#define SPACE 50

void resizeActionArea(Widget w,  ActionAreaItems *actions,
                        int num_actions, int max_but_wid)
{
    static first = True;
    static Widget  *action_but;
    int num_wid, but_wid;
    XtWidgetGeometry  size;
    int i;
    int max_w;
    int cur_position;
    int tightness;
    Widget	shell = w;

    if( actions){
	action_but = (Widget *)XtMalloc(num_actions * sizeof(Widget));
        for (i = 0; i < num_actions; i++) {
  	     action_but[i] = (Widget) actions[i].widget;
             XtQueryGeometry(action_but[i], NULL, &size);
		if(but_wid < (int)size.width)
			but_wid = (int)size.width;
        }
	but_wid = max_but_wid;
	num_wid = num_actions;
        first = False;	
    }
    size.request_mode = CWWidth;
    XtQueryGeometry(w, NULL, &size);

    max_w = size.width;
    if(max_w > num_wid * but_wid){
	/* REMOVE printf("reater\n"); */
/*
        XtVaSetValues(XtParent(action_but[0]),
	        XmNwidth,	 max_w,
        	XmNfractionBase, 2*num_wid - 1,
                NULL);
*/
    }else
     max_w = num_wid * but_wid + SPACE;
	while(!XtIsShell(shell))
		shell = XtParent(shell);
		
	/*XtVaSetValues(XtParent(action_but[0]), */
	XtVaSetValues(shell,
	        XmNwidth,	 max_w,
        	/*XmNfractionBase, 2*num_wid - 1, */
		XmNfractionBase, max_w,
                NULL);
    cur_position = 0;
    for (i = 0; i < num_wid - 1; i++) {
        XtVaSetValues(action_but[i],
	    XmNwidth,		     but_wid,
            XmNleftAttachment,       XmATTACH_POSITION ,
            XmNleftPosition,         cur_position,
                NULL);
            cur_position += 2; 
    }
    XtVaSetValues(action_but[i],
	XmNwidth,	     	but_wid,
	XmNrightAttachment,	XmATTACH_FORM,
	NULL);
}

Widget
createActionArea(
	Widget			parent,
	ActionAreaItems *	actions,
	int			num_actions,
	Widget			maxWidthWid,
	DmMnemonicInfo		mneInfo,
	int			mneIndex
)
{
	Widget 		action_area, widget,rc;
	int		i, j, max_w, needed_len;
	XtWidgetGeometry  size;
	int 		max_but_len, cur_position=0, left_pos=0, right_pos=0;
	int 		tightness;
	void 		setLabel();
	unsigned char	leftattach, rightattach;
	Boolean		showasdefault;
	char		tmpstr[BUFSIZ];
	unsigned char *		mne;
	KeySym			mneks;
	XmString		mneString;

	size.request_mode = CWWidth;
	XtQueryGeometry(maxWidthWid, NULL, &size);
	max_w = size.width;
	action_area = XtVaCreateWidget("action_area", xmFormWidgetClass, 
			parent, XmNfractionBase, max_w - 1, 
			NULL);

	max_but_len = 0;
	for (i = 0; i < num_actions; i++) {

		actions[i].widget = XtVaCreateManagedWidget("action_label",
			xmPushButtonWidgetClass, action_area,
			NULL);
		setLabel(actions[i].widget, actions[i].label);
		XtQueryGeometry(actions[i].widget, NULL, &size);
		if (max_but_len < (int)size.width)
			max_but_len = (int)size.width;
		XtDestroyWidget(actions[i].widget);
	}

	needed_len = max_but_len * num_actions;
	if (needed_len > max_w) {
		XtVaSetValues(action_area, 
			XmNfractionBase, needed_len, 
			XmNskipAdjust, True,
			NULL);
		tightness = 0;
	}
	else {
		tightness = (max_w - needed_len)/(num_actions - 1);
	}

	for (j = 0; j < num_actions; j++) {
		strcpy(tmpstr, mygettxt(actions[j].label));
		actions[j].widget = XtVaCreateManagedWidget(tmpstr,
			xmPushButtonWidgetClass, action_area,
			NULL);

		XmSTR_N_MNE(
			actions[j].label,
			actions[j].mnemonic,
			mneInfo[j + mneIndex],
			DM_B_MNE_ACTIVATE_CB | DM_B_MNE_GET_FOCUS
		);
		XmStringFree(mneString);
			
		XtVaSetValues(actions[j].widget, XmNmnemonic, mneks, NULL);

		mneInfo[j + mneIndex].w = actions[j].widget;
		if (actions[j].callback) {
			XtAddCallback(actions[j].widget, XmNactivateCallback,
				actions[j].callback, actions[j].data);
			mneInfo[j + mneIndex].cb = (XtCallbackProc)actions[j].callback;
			mneInfo[j + mneIndex].cd = (XtPointer)actions[j].data;
		}
		
		if (j == 0) {
			leftattach = XmATTACH_FORM;
			showasdefault = True;
			left_pos = cur_position;
		}
		else {
			leftattach = XmATTACH_POSITION;
			showasdefault = False;
			left_pos = cur_position + tightness;
		}
		if ( j != num_actions-1) {
			rightattach = XmATTACH_POSITION;
			right_pos = left_pos + max_but_len;
			cur_position = right_pos;	
		}
		else {
			rightattach = XmATTACH_FORM;
		}
		XtVaSetValues(actions[j].widget,
			XmNleftAttachment, leftattach,
			XmNleftPosition, left_pos,
			XmNrightAttachment, rightattach,
			XmNrightPosition, right_pos,
			XmNshowAsDefault, showasdefault,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNdefaultButtonShadowThickness, 1, 
			NULL);
	}

	XtManageChild(action_area);
	return action_area;
}

Widget 
crt_radio(Widget parent, radioList *radio, Widget label)
{
	Widget radio_box;
	int i;

	radio_box = XmCreateRadioBox(parent, "radio_box", NULL, 0);

	XtVaSetValues(radio_box, 
		XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
		XmNorientation, radio->orientation,
		XmNradioBehavior, True,
		XmNisHomogeneous, True,
                NULL);
	for(i = 0; i< radio->count; i++){
	    radio->list[i].widget= XtVaCreateManagedWidget("radioBtn",
        		xmToggleButtonWidgetClass, radio_box, NULL);
	    XtVaSetValues(radio->list[i].widget, 
		XmNvisibleWhenOff, True,
		XmNindicatorType, XmONE_OF_MANY,
		NULL);
            setLabel(radio->list[i].widget,  radio->list[i].label);
	    if(radio->list[i].callback)
	    	XtAddCallback(radio->list[i].widget, XmNvalueChangedCallback, 
			radio->list[i].callback, NULL);
	}
	XtManageChild(radio_box);
	return(radio_box);

}

Widget 
crt_rc_radio(Widget parent, radioList *radio, Widget label)
{
	Widget radio_box;
	int i;

	radio_box = XtVaCreateManagedWidget("radio_box",
		xmRowColumnWidgetClass, parent,
		XmNorientation, XmHORIZONTAL,
		NULL);

	XtVaSetValues(radio_box,
		XmNradioBehavior, TRUE,
		NULL);
/*
	XtVaSetValues(radio_box, 
		XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
		XmNorientation, radio->orientation,
                NULL);
*/
	for(i = 0; i< radio->count; i++){
	    radio->list[i].widget= XtVaCreateManagedWidget("radioBtn",
        		xmToggleButtonWidgetClass, radio_box, NULL);
            setLabel(radio->list[i].widget,  radio->list[i].label);
	    XtVaSetValues(radio->list[i].widget, 
		XmNindicatorType, XmONE_OF_MANY,
		NULL);
	    if(radio->list[i].callback)
	    	XtAddCallback(radio->list[i].widget, XmNvalueChangedCallback, 
			radio->list[i].callback, NULL);
	}
	/*XtManageChild(radio_box); */
	return(radio_box);

}
Widget
crt_inputradio(Widget parent,char *labelStr,int len,radioList *radio,Boolean alignTop)
{
	Widget form, label, rb;


	form = XtVaCreateManagedWidget("form",
                xmFormWidgetClass, parent,
                NULL);

        label = XtVaCreateManagedWidget("label",
                xmLabelWidgetClass, form,
                NULL);
        setLabel(label, labelStr);

	if (alignTop) {
	        XtVaSetValues(label, XmNwidth, len,
		       	XmNalignment, XmALIGNMENT_END,
		       	XmNleftAttachment, XmATTACH_FORM,
		       	XmNtopAttachment, XmATTACH_FORM,
		       	XmNbottomAttachment, XmATTACH_FORM,
		       	NULL);
	} else {
		XtVaSetValues(label, XmNwidth, len,
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			NULL);
	}
	rb = crt_radio(form, radio, label);
	return(form);
}

Widget crt_inputlabellabel(Widget parent,char *labelStr,int len,char *labelStr1)
{
        Widget form, label, label1;


        form = XtVaCreateManagedWidget("form",
                xmFormWidgetClass, parent,
                NULL);

        label = XtVaCreateManagedWidget("label",
                xmLabelWidgetClass, form,
                NULL);
        setLabel(label, labelStr);

        XtVaSetValues(label, XmNwidth, len,
                XmNalignment, XmALIGNMENT_END,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                NULL);
	label1 = XtVaCreateManagedWidget("label",
                xmLabelWidgetClass, form,
                NULL);

        XtVaSetValues(label1, 
		XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL);
        setLabel(label1, labelStr1);
        return(label1);
}

Boolean
chkEtcHostEnt(node_name)
char	*node_name;
{
	char	cmd[256];
	int	ret;
	sprintf(cmd, "grep \"^[^#].*%s\" /etc/hosts > /dev/null 2>&1", node_name);
	ret = system(cmd);
	if ( ret == 0 )
		return(TRUE);
	else
		return(FALSE);
}

void
listadd(list, string)
Widget	list;
char	*string;
{
	XmString	str;	
	
	str = createString(string);
	XmListAddItemUnselected(list, str, 0);
	freeString(str);	
}

makePosVisible(Widget list, int index)
{
        int     top, visible;

        XtVaGetValues(list,
                XmNtopItemPosition, &top,
                XmNvisibleItemCount, &visible,
                NULL);
        if (index < top)
                XmListSetPos(list, index);
        else if (index >= top+visible)
                XmListSetBottomPos(list, index);
}

/* remove leading and trailing whitespace without moving the pointer */
/* so that the pointer may still be free'd later.		     */
/* returns True if the string was modified; False otherwise	     */

Boolean
removeWhitespace(char * string)
{
    register char *ptr = string;
    size_t   len;
    Boolean  changed = False;

    if (string == NULL)
	return False;

    while (isspace(*ptr))
    {
	ptr++;
	changed = True;
    }
    if ((len = strlen(ptr)) == 0)
    {
	*string = EOS;
	return changed;
    }

    if (changed)
	(void)memmove((void *)string, (void *)ptr, len+1); /* +1 to */
                                                           /* move EOS */
    ptr = string + len - 1;    /* last character before EOS */
    while (isspace(*ptr))
    {
	ptr--;
	changed = True;
    }
    *(++ptr) = EOS;
   
    return changed;
}

Boolean
getRemoteAccessPath(Widget toplevel, char *sysName, char **target)
{
	static char	node_directory[MEDIUMBUFSIZE];
	char		buf[MEDIUMBUFSIZE];
	static Boolean	first_time = True;
	struct stat	stat_buf;
	char		*getenv();	
	struct utsname	sname;

	
	/* check to see if the local machine has been selected */
	uname(&sname);
	if (strcmp(sysName, sname.nodename) == 0) {
		createMsg(toplevel, ERROR, ERR_remoteFile, NULL);
		return(FALSE);
	}
	if (first_time) {
		first_time = False;
		sprintf (node_directory, "%s/.node", getenv("HOME"));
	}
	/* check the node directory is there or not. */
	/* if not, then create it			*/

	if (!DIRECTORY(node_directory, stat_buf) ) {
		if (mkdir(node_directory, DMODE) == -1) {
			createMsg(toplevel, ERROR, ERR_remoteFile,NULL);
			return(FALSE);
		}
		if (chown(node_directory, getuid(), getgid()) == -1) {
			createMsg(toplevel, ERROR, ERR_remoteFile, NULL);
			return(FALSE);
		}
	} else
	    if (access(node_directory, W_OK) < 0) {
		    createMsg(toplevel, ERROR, ERR_remoteFile, NULL);
		    return(FALSE);
	    }

	/* create the path name */
	sprintf (buf, "%s/%s", node_directory, sysName);
	*target = strdup(buf);

	return(True);
}

createRemoteAccessFile(Widget toplevel, char *target)
{

	FILE		*attrp;
	char	*sysName, *tmp, *strstr();

	tmp = strstr(target, ".node/");
	sysName = tmp + strlen(".node/"); 

	attrp = fopen( target, "w");
	if (attrp == (FILE *) 0) {
                createMsg(toplevel, ERROR, ERR_remoteFile,NULL);
		return(FALSE);
	}

	/* put the node's properties here */
	fprintf( attrp, "SYSTEM_NAME=%s\n",
		sysName);

	(void) fflush(attrp);
	fclose( attrp );

	if (chmod( target, MODE) == -1) {
                createMsg(toplevel, ERROR, ERR_remoteFile, NULL);
		return(FALSE);
	}
	if (chown( target, getuid(), getgid()) == -1) {
                createMsg(toplevel, ERROR, ERR_remoteFile, NULL);
		return(FALSE);
	}
	return(TRUE);
}
