#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:internet/config_local.c	1.16.1.1"
#endif

#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/Form.h>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/secsys.h>

#include "DesktopP.h"

#include "lookup.h"
#include "lookupG.h"
#include "util.h"
#include "inet.h"
#include "inetMsg.h"

extern void 	createMainWin();
extern Boolean	getAddrToDotStr(inetAddr, char*);
extern void	createMsg(Widget, msgType, char *, char *);
extern void	createCommonMsg(Widget, Widget, msgType, char *, char *);
extern void	createExitMsg(Widget, msgType, char *, char *);
extern XmString	createString(char *);
extern void	freeString(XmString);
extern void	busyCursor(Widget, int);
extern void	helpkeyCB(Widget, XtPointer, XtPointer);
extern char	*mygettxt(char *);
extern int	r_decor(Widget);
extern void	noSpacesCB(Widget, XtPointer, XtPointer);

static char *title;
static void okCB();
static void resetCB();
static void cancelCB();
static void helpCB();
int	initNetwork();

HelpText	CfgHelp = { NULL, NULL, "20" };

static ActionAreaItems actions[] = {
                { TXT_OK,       MNE_OK, okCB,   NULL    },
                { TXT_Reset,    MNE_Reset, resetCB, NULL   },
                { TXT_Cancel,   MNE_Cancel, cancelCB, NULL  },
                { TXT_Help,     MNE_Help, helpkeyCB, &CfgHelp },
};

static char    *config_form[]={
        TXT_system_name,
        TXT_inet_addr,
        TXT_comment,
        "",
};
static char    *options_form[]={
        TXT_LDN,
        TXT_name_server,
        TXT_inet_addr,
        "",
};
typedef struct _config{
        Widget  popup;          /* popup shell widget id */
        Widget  sysName;        /* system name input label widget id */
        inetAddr       addr;   /* address for the system */
	Widget	comment;	/* text widget for the comment */
	Widget  showOptions;	/* toggle button to show other options */
	Widget  locDomain;	/* text widget for the local domain */
	Widget  nameServer;	/* text widget for the name server */
        inetAddr       domAddr;   /* address for the system */
        ActionAreaItems *actions; /* control area handle */
	Widget status;
	/* non-gui structure */
	char	*systemName_P;
	char	*systemName_C;
	char	*sysAddr_P;
	char	*sysAddr_C;
	char	*cmt_P;
	char	*cmt_C;
	char	*domainName_P;
	char	*domainName_C;
	char	*server_P;
	char	*server_C;
	char	*serAddr_P;
	char	*serAddr_C;
	char	*statusLine_P;
	char	*statusLine_C;
	ADDR	dotAddr;
	char	*haddr;
}config;

config clWin;			/* config local window data struct */

/* This routine returns TRUE when the network is available */
Boolean
IsTCP_OK()
{
	extern	errno;
	int	fd, old_errno;

	old_errno = errno;
	if ((fd = open("/dev/tcp", O_RDONLY, 0)) >= 0);
		(void) close(fd);
	errno = old_errno;
	return (fd >= 0);
} /* IsTCP_OK */

initNetMsg()
{
	createCommonMsg(
		hi.net.common.toplevel,
		&hi.net.common.mb,
		INFO,
		mygettxt(INFO_setupNet),
		title
	);
	XtAppAddTimeOut(
		hi.net.common.app,
		2000,
		(void(*)())initNetwork,
		clWin.sysName
	);
}

initNetwork(wid)
Widget wid;
{

	Widget		w;
	char 	cmdline[MEDIUMBUFSIZE], *bp;
	char	exitp[128];
	char	stderr_file[MEDIUMBUFSIZE] = "/var/tmp/network";
	char	buf[MEDIUMBUFSIZE];
	int  	x, i;
	int	sfd;
	FILE	*stderr_p;
	struct stat	statbuf;
	Boolean	did_setuid;
	uid_t	orig_uid, _loc_id_priv;
	XmString	status_line;

	/* Since some network initialization still requires
	 * using the root adminstrator uid, we are going to
	 * setuid() for this portion of code.
	 * If we luck out to be root from the start,
	 * make note not to do the restore by leaving
	 * did_setuid = FALSE.
	 * In an SUM (Super-User-Mode) system, we will
	 * be able to to setuid(non-root) without losing
	 * privilege since we assume we are aquiring
	 * privilege for this process via tfadmin, and
	 * therefore the privs are aquired via fixed
	 * privilege.
	 *
	 * Since privilege is required by the init scripts,
	 * we'll have to handle the error case of not being
	 * able to do the setuid().
	 */

	/* get root administrator user & current user id,
	 * censure -1's
	 */
	orig_uid = getuid();
	_loc_id_priv = secsys(ES_PRVID, 0);

	if ((-1 == orig_uid) || (-1 == _loc_id_priv)) {
		/* what besides?! ********/
		createExitMsg(wid, ERROR, mygettxt(ERR_noPriv), title);
		return;
	}
	if ((_loc_id_priv >= 0) &&
	    (orig_uid == _loc_id_priv))
		did_setuid = FALSE;
	else {
		if ((setuid(_loc_id_priv)) < 0) {
			/* what besides?! ********/
		createExitMsg(wid, ERROR, mygettxt(ERR_noPriv), title); 
			return;
		}
		did_setuid = TRUE;
	}

	(void)umask(0022);

	/* create path name to the stderr file */

#ifdef later
	stderr_p = tmpfile();
#else
	stderr_p = fopen(stderr_file, "a+");
#endif

	/* replace stderr with stderr_file */
	if ( stderr_p  == (FILE *)NULL ) {
		fprintf(stderr, "cannot open stderr_p file");
	} else {
		sfd = dup(2);
		(void)close(2);
		(void)dup(fileno(stderr_p));
		(void)fclose(stderr_p);
	}

	/* form the listener command line */

	sprintf( cmdline,
		"sh /etc/inet/listen.setup 1>&2"
		);
	/* set busy cursor */
	busyCursor(hi.net.common.mb, TRUE);
	i = system(cmdline);
	switch((i >> 8) && 0xff) {

	case 0:
		if (IsTCP_OK() == False) {
			busyCursor(hi.net.common.mb, False);
			createExitMsg(wid, ERROR, mygettxt(ERR_noTCP), title);
			return;
		}
		x = sprintf( cmdline,
			"sh /etc/inet/rc.restart 1>&2"
			);
		i = system(cmdline);
		switch((i >> 8) && 0xff) {
		case 0:
			createCommonMsg(
				hi.net.common.toplevel,
				&hi.net.common.mb,
				INFO,
				mygettxt(INFO_setupOK),
				title
			);
			break;
		case 1:
		case 2:
			busyCursor(wid, False);
			createExitMsg(wid, ERROR, mygettxt(ERR_listenerFail), title);
			return;
		case 3:
		case 4:
			sprintf(buf, mygettxt(ERR_uucpFail), stderr_file);
			createMsg(wid, ERROR, buf, title);
			break;
		default:
			(void)fprintf(stderr,"default in InitNetwork taken!!!\n");
			break;
		}
		
		break;
	case 1:
	case 2:
		busyCursor(wid, False);
		createExitMsg(wid, ERROR, mygettxt(ERR_listenerFail), title);
		return;
	case 3:
	case 4:
		sprintf(buf, mygettxt(ERR_uucpFail), stderr_file);
		createMsg(wid, ERROR, buf, title);
		break;
	default:
		(void)fprintf(stderr,"default in InitNetwork taken!!!\n");
		break;
	}
	busyCursor(wid, False);
	/* put back the stderr as before */
	(void)close(2);
	(void)dup(sfd);
	(void)close(sfd);

	if (did_setuid)
		if ((setuid(orig_uid)) < 0) {
			/* what besides?! ********/
			createMsg(wid, ERROR, mygettxt(ERR_noPriv), title);
			return;
		}

	/* popdown local win and call create mainwin() */
	XtUnmanageChild(clWin.popup);
	XtAppAddTimeOut(hi.net.common.app, 0, (void(*)())createMainWin, NULL);
} /* InitNetwork */

Boolean
appendEtcFile(char *sys, char *addr, char *cmt)
{
	FILE	*fd;
	char	buf[MEDIUMBUFSIZE], errbuf[MEDIUMBUFSIZE];
	struct stat	statBuffer;

	if ((fd = fopen(etcHostPath, "a")) == NULL) {
		createMsg(hi.net.common.toplevel, ERROR, 
			mygettxt(ERR_cantOpenEtcFile), title);
		return(False);
	}
	while (fstat(fileno(fd), &statBuffer) < 0 && errno == EINTR)
		/* try again */;
	sprintf(buf, "%s\t%s\t%s%s\n",
		addr, sys, "#", cmt);
	if (fputs(buf, fd) == EOF) {
		sprintf(errbuf, mygettxt(ERR_cantWriteFile), etcHostPath);
		createMsg(hi.net.common.toplevel, ERROR, errbuf, title);
		fclose(fd);
		return(False);
	}
	fclose(fd);
	return(True);
}

Boolean
appendResolvFile(char *domain, char *server, char *addr)
{
	FILE	*fd;
	char	buf[MEDIUMBUFSIZE];
	struct stat	statBuffer;

	if ((fd = fopen(ETCCONF_PATH, "w")) == NULL) {
		sprintf(buf, mygettxt(ERR_cantOpenFile), ETCCONF_PATH);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		return(False);
	}
	while (fstat(fileno(fd), &statBuffer) < 0 && errno == EINTR)
		/* try again */;
	sprintf(buf, "domain\t%s\n", domain);
	if (fputs(buf, fd) == EOF) {
		sprintf(buf, mygettxt(ERR_cantWriteFile), ETCCONF_PATH);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		fclose(fd); 
		return(False);
	}
	sprintf(buf, "nameserver\t%s\t#%s\n", addr, server);
	if (fputs(buf, fd) == EOF) {
		sprintf(buf, mygettxt(ERR_cantWriteFile), ETCCONF_PATH);
		createMsg(hi.net.common.toplevel, ERROR, buf, title);
		fclose(fd); 
		return(False);
	}
	fclose(fd);
	return(True);
}

static void 
okCB()
{
	char	buf[MEDIUMBUFSIZE];
	Boolean	etcOk, resolvOk;	

	/* get the system address */
	if (getAddrToDotStr(clWin.addr, buf) == False) {
		createMsg(
			hi.net.common.toplevel,
			ERROR, 
			mygettxt(ERR_clsFillOutForm1),
			title
		);
		return;
	} else {
		clWin.sysAddr_C = strdup(buf);
	}
	/* get the comment line */
	XtVaGetValues(clWin.comment, XmNvalue, &clWin.cmt_C, NULL);
	if (*clWin.cmt_C == 0) {
		free(clWin.cmt_C);
		clWin.cmt_C = NULL;
	}
	/* get the domain name */
	XtVaGetValues(clWin.locDomain, XmNvalue, &clWin.domainName_C, NULL);
	if (*clWin.domainName_C == 0) {
		free(clWin.domainName_C);
		clWin.domainName_C = NULL;
	}
	/* get the name server */
	XtVaGetValues(clWin.nameServer, XmNvalue, &clWin.server_C, NULL);
	if (*clWin.server_C == 0) {
		free(clWin.server_C);
		clWin.server_C = NULL;
	}
	/* get the server address */
	if (clWin.domainName_C && clWin.server_C && (getAddrToDotStr(clWin.domAddr, buf) == False)) {
		createMsg(
			hi.net.common.toplevel,
			ERROR, 
			mygettxt(ERR_clsFillOutForm1),
			title
		);
		return;
	} else {
		clWin.serAddr_C = strdup(buf);
	}

	/* append the system name, address and comment into the /etc/hosts */
	etcOk = appendEtcFile(clWin.systemName_C, clWin.sysAddr_C, clWin.cmt_C);

	/* append the domain name, name server and server address into
	 * /etc/resolv.conf file
	 */
	if (clWin.domainName_C && clWin.server_C && clWin.serAddr_C) {
		resolvOk = appendResolvFile(
			clWin.domainName_C,
			clWin.server_C,
			clWin.serAddr_C
		);
	} else {
		resolvOk = True;
	}
	if ((etcOk == True) && (resolvOk == True)) {
		hi.net.common.isDnsConfigure = True;
		XtAppAddTimeOut(
			hi.net.common.app,
			0,
			(void(*)())initNetMsg,
			NULL
		);
	} else {
		createMsg(
			hi.net.common.toplevel,
			ERROR,
			mygettxt(ERR_cantConfLocalSys),
			title
		);
	}
}

static void
resetCB() {
        
	if (clWin.sysAddr_C) free(clWin.sysAddr_C);
	clWin.sysAddr_C = strdup(clWin.sysAddr_P);
	if (clWin.cmt_C) free(clWin.cmt_C);
	clWin.cmt_C = strdup(clWin.cmt_P);
	if (clWin.domainName_C) free(clWin.domainName_C);
	clWin.domainName_C = strdup(clWin.domainName_P);
	if (clWin.server_C) free(clWin.server_C);
	clWin.server_C = strdup(clWin.server_P);
	if (clWin.serAddr_C) free(clWin.serAddr_C);
	clWin.serAddr_C = strdup(clWin.serAddr_P);

	/* reset the Gui */
	clearAddr(clWin.addr);
	XtVaSetValues(clWin.comment, XmNvalue, clWin.cmt_C, NULL);
	XtVaSetValues(clWin.locDomain, XmNvalue, clWin.domainName_C, NULL);
	XtVaSetValues(clWin.nameServer, XmNvalue, clWin.server_C, NULL);
	clearAddr(clWin.domAddr);
}

static void
cancelCB() {
        exit(0);
}

static void
helpCB() {
        printf("help\n");
}

static void
decideScreenLayout()
{
	struct utsname	name;
	XmString	str;

	uname(&name);
	str = createString(name.nodename);
	XtVaSetValues(clWin.sysName, XmNlabelString, str, NULL);
	freeString(str);
	clWin.systemName_C = strdup(name.nodename);
}

static void 
setupGui(Widget w, int client_data, XmAnyCallbackStruct *cbs)
{
	int maxWidth;
	static Widget topRC, topForm; 
	Widget optionsRC;
	Widget sep1, sep2;
	DmMnemonicInfoRec	mneInfo[MI_TOTAL];
	int			mm;

	if(!clWin.popup){
                maxWidth = GetMaxWidth(w, config_form);
                clWin.popup = XtVaCreatePopupShell("configLocal",
                        xmDialogShellWidgetClass, w,
                        NULL);
		title = mygettxt(TXT_configView);
		XtVaSetValues(clWin.popup,
                        XmNtitle, title,
                        XmNallowShellResize, True,
                        XmNdeleteResponse, XmUNMAP,
                        NULL);
		
		r_decor(clWin.popup);

		topForm = XtVaCreateWidget("topForm",
			xmFormWidgetClass, clWin.popup,
			NULL);

                topRC = XtVaCreateManagedWidget("topRC",
                        xmRowColumnWidgetClass, topForm,
                        XmNorientation, XmVERTICAL,
                        NULL);
		XtAddCallback(topRC, XmNhelpCallback, helpkeyCB, &CfgHelp);
		clWin.sysName = (Widget) crt_inputlabellabel(topRC, TXT_system_name, maxWidth, NULL);
		crt_inet_addr(topRC, TXT_inet_addr, maxWidth, &(clWin.addr));
		clWin.comment = (Widget) crt_inputlabel(topRC, TXT_comment, maxWidth);
		XtAddCallback(
			clWin.comment,
			XmNmodifyVerifyCallback,
			noSpacesCB,
			NULL
		);
                optionsRC = XtVaCreateManagedWidget("topRC",
                        xmRowColumnWidgetClass, topRC,
                        NULL);
		clWin.locDomain = (Widget) crt_inputlabel(optionsRC, TXT_LDN, maxWidth);
		XtAddCallback(
			clWin.locDomain,
			XmNmodifyVerifyCallback,
			noSpacesCB,
			NULL
		);
		clWin.nameServer = (Widget) crt_inputlabel(optionsRC, TXT_name_server, maxWidth);
		XtAddCallback(
			clWin.nameServer,
			XmNmodifyVerifyCallback,
			noSpacesCB,
			NULL
		);
		crt_inet_addr(optionsRC, TXT_inet_addr, maxWidth, &(clWin.domAddr));
		sep1 = XtVaCreateManagedWidget("spe1",
                        xmSeparatorGadgetClass, topRC, NULL);
                createActionArea(
			topRC,
			actions,
                        XtNumber(actions),
			clWin.comment,
			mneInfo,
			0
		);
                sep2 = XtVaCreateManagedWidget("spe2",
                        xmSeparatorGadgetClass, topRC, NULL);
		clWin.status = XtVaCreateManagedWidget("label",
			xmLabelGadgetClass,topRC, NULL);
		setLabel(clWin.status, "");
		XtVaSetValues(topForm,
			XmNdefaultButton, actions[0].widget,
			XmNcancelButton, actions[2].widget,
			NULL);

		REG_MNE(clWin.popup, mneInfo, XtNumber(actions));
	}
	decideScreenLayout();
        XtManageChild(topForm);
        XtManageChild(clWin.popup);
}

void
createConfigLocal(Widget w, int client_data, XmAnyCallbackStruct *cbs)
{
	setupGui(w, client_data, cbs);
}
