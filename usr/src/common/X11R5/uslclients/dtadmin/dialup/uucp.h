#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/uucp.h	1.51.1.1"
#endif

#ifndef _UUCP_H
#define _UUCP_H

#include <stdio.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <Xol/TextField.h>
#include <libDtI/DtI.h>


/* Some macros */

#define GOOD_SPEED  0
#define BAD_SPEED 1
#define DEVICES_OUTGOING	0
#define DEVICES_DISABLED	1
#define DEVICES_INCOMING	2
#define SYSTEM_FILE		2
#define MAXPAGES 	2
#define DISPLAY		XtDisplay(sf->toplevel)
#define WINDOW		XtWindow(sf->toplevel)
#define SCREEN		XtScreen(sf->toplevel)
#define ROOT		RootWindowOfScreen(SCREEN)
#define STRUCTASSIGN(d, s)	memcpy(&(d), &(s), sizeof(d))
#define DIRECTORY(f, s)	((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFDIR))
#define REGFILE(f, s)	((stat((f), &s)==0) && ((s.st_mode&(S_IFMT))==S_IFREG))
#define	DMODE		(S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#define	MODE		(S_IRWXU | S_IRGRP | S_IROTH) /* 0744 */
#define UUCPUID		(uid_t) 5
#define UUCPGID		(gid_t) 5
#define NIL		(XtPointer)0
#define BLANKLINE	""
#define CLEARMSG()	ClearFooter(sf->footer)
#define PUTMSG(msg)     FooterMsg(sf->footer, "%s", msg)

#define PORT		1
#define SPEED		2
/*
** Bit manipulating macros
*/
#define BIT_IS_SET(field, bit)  (((field) & (bit)) != (unsigned)0)
#define SET_BIT(field, bit)     ((field) |= (bit))
#define UNSET_BIT(field, bit)   ((field) &= ~(bit))
/*
** Masks for `stat_flags'
*/
#define	FILE_EXISTS	1
#define FILE_READABLE	2
#define FILE_WRITEABLE	4

#define	COM1 "com1"
#define COM2 "com2"
#define LONG_COM1 "/dev/tty00h"
#define LONG_COM2 "/dev/tty01h"
#define COM1_SHORT_ALIAS "tty00h"
#define COM2_SHORT_ALIAS "tty01h"

#define BIDIRECTIONAL   0
#define OUTGOING        1
#define INCOMING        2
#define UNAMESIZE	20	/* max chars in a user name */
#define	INCREMENT	20
#define	LOGIN_INCREMENT	5
#define BUF_SIZE	1024
#define MAXLINE		128

#define	HDR_FORMAT		"%14s%8s%20s"
#define LOGIN_FORMAT	"%20s %20s"
#define	FORMAT		"%14s%0s%0s%8s%20s"
#define	PROMPT_SIZE		25
#define	RESPONSE_SIZE		25
#define MAX_VISIBLE	20
#define	MAXSIZE		20	/* is 13 enough? */
#ifdef old
#define MAXNAMESIZE	SYS_NMLN - 1
#endif
#define MAXNAMESIZE	14
#define	MAXPHONESIZE	20
#define DEVICE_SIZE	25
#define SPEED_SIZE	10

/* Possible return values for field validation */
#define VALID		False
#define INVALID		True

#define SYSTEMS_TYPE	0
#define DEVICE_TYPE	1

/* request type for the device popup window */
#define	B_ADD		0
#define	B_MOD		1

/*
 * Popup Window actions.  These messages are sent to the popup callback when
 * a user has selected one of the popup window's buttons.
 */
#define APPLY		1
#define RESET		2
#define RESETFACTORY	3
#define SETDEFAULTS	4
#define CANCEL		5

/*
 * Object messages.  These are arbitrary actions we have defined for the
 * popup widget's instance methods (i.e. routines we have defined) to use
 * to control the items in the property window.
 */
#define NEW		1
#define SET		2
#define GET		3
#define UNGREY		4
#define GREY		5
#define DEFAULT		6
#define SELECT		7
#define UNSELECT	8
#define VERIFY		9

#define ADD 6
#define MODIFY  7 
#define DELETE 8

/* Fields in Systems file */
#define F_NAME 0
#define F_TIME 1
#define F_TYPE 2
#define F_CLASS 3
#define F_PHONE 4
#define F_EXPECT1 5
#define F_LOGIN 6
#define F_EXPECT2 7
#define F_PASSWD 8
#define F_SPEED 9
#define F_MAX 40

/* Fields in Devices file */
#define D_TYPE 0
#define D_LINE 1
#define D_CALLDEV 2
#define D_CLASS 3
#define D_CALLER 4
#define D_ARG 5
#define D_MAX 40

/* bnu object type */
typedef struct {
	char	*path;
	DmObjectPtr	op;
	int		num_objs;
} ContainerRec, *ContainerPtr;

#define	INIT_X	45
#define	INIT_Y	0
#define	INC_X	70
#define	INC_Y	30
#define DIALUP_WIDTH	OlMMToPixel(OL_HORIZONTAL, 140)
#define DIALUP_HEIGHT	OlMMToPixel(OL_VERTICAL, 60)
#define BNU_WIDTH	OlMMToPixel(OL_HORIZONTAL, 110)
#define BNU_HEIGHT	OlMMToPixel(OL_VERTICAL, 60)
#define OFFSET	29

#define VIEWHEIGHT	6
#define LOGIN_VIEWHEIGHT	4

#define XA		XtArgVal

typedef struct _DeviceData {
	String		portNumber;	/* eg. tty00, tty01 ... */
	String		modemFamily;	/* eg. hayes, telebit ...*/
	String		portSpeed;	/* eg. 1200, 2400 ... */
	String		portDirection;
	String		portEnabled;
	String		DTP;		/* Dialer-Token-Pair */
	String		holdPortNumber;	/* used for Reset */
	String		holdModemFamily;	/* same as above */
	String		holdPortSpeed;	/* same as above */
	String		holdPortDirection; /* bi-directional, outgoing only, or incoming 
								only */
	String		holdPortEnabled;
} DeviceData;

DeviceData holdData;

typedef struct _DeviceItems {
	String		label;
	String		value;
	XtArgVal sensitive;
} DeviceItems;


#define ACU "ACU"
#define DIR "Direct"
#define DK "DK"

#define ACU_ICON	"acu.glyph"
#define DIR_ICON	"dir.glyph"

typedef struct lineRec *LinePtr;
typedef	struct lineRec {
	LinePtr	next;
	char	*text;
} LineRec;

typedef struct {
	XtPointer f_prompt;
	XtPointer f_response;
} loginData;

typedef struct {
	loginData *pExpectSend;
} LoginFlatList;

typedef struct {
	int numAllocated;
	int numExpectSend;
	int currExpectSend;
	LoginFlatList *expectFlatItems;
} LoginData;

LoginData * work;
LoginData * newExpectSend;

typedef struct {
    XtPointer f_name;
    XtPointer f_time;
    XtPointer f_type;
    XtPointer f_class;
    XtPointer f_phone;
    XtPointer f_expectSeq;
    LoginData *loginp;
    LinePtr   lp;
    Boolean   changed;
} HostData;

typedef struct {
    HostData *pField;
} FlatList;

FlatList *	new;

typedef struct _ExecItem {
	void		(*p)();		/* proc to call when timer expires */
	Widget		popup; 		/* wid used when command exit != 0 */
	Widget		button; 	/* wid used to (un)set sensitivity */
	int		pid;		/* child process id */
	int		exit_code;	/* the exit code		   */
	char *		exec_argv[14];	/* arg0 is command, arg1 is opts   */
} ExecItem;

typedef struct _Items {
	void		(*p)();
	Widget	popup;
	XtArgVal	sensitive;
	XtArgVal	label;
	XtArgVal	mnemonic;
	XtArgVal	client;
} Items;

typedef struct Menus {
	String		label;
	Items		*items;
	int			numitems;
	Bool		use_popup;
	int			orientation;	/* OL_FIXEDROWS, OL_FIXEDCOLS */
	int			pushpin;	/* OL_OUT, OL_NONE */
	Widget		widget;		/* Pointer to this menu widget */
} Menus;

typedef struct {
    char        *title;
    char        *file;
    char        *section;
} HelpText;

typedef struct _SystemFile {
	Boolean		readAllow;
	Boolean		update;
	FlatList *	flatItems;
	Items *		popupMenuItems;
	Widget		cancelNotice;
	Widget		findPopup;
	Widget		findTextField;
	Widget		footer;
	Widget		sprop_footer;			/* footer for system property window */
	Widget		dfooter;
	Widget		dprop_footer;		/* footer for device property window */
	Widget		w_otherSpeed;
	Widget 		pages[MAXPAGES];
	TextFieldWidget	w_name;	        /* Main body of the entry */
	Widget		w_type;
	Widget		w_speed;
	TextFieldWidget	w_phone;
	Widget		category;
	TextFieldWidget	w_prompt;
	TextFieldWidget	w_response;
	Widget		logSeqCaption;
	Widget 		loginScrollList;
	Widget		loginFlatButtons;
	Widget		propPopup;
	Widget		devicePopup;
	Widget		quitNotice;
	Widget		scrollingList;
	Widget		toplevel;
	char *		userName;	/* User name of the operator */
	char *		userHome;	/* $HOME of the user */
	char *		nodeName;	/* Node name of the local hosts */
	char *		filename;	/* Used by open and save */
	int		currentItem;	/* Currently selected index */
	int		numFlatItems;
	int		numAllocated;	/* The high water mark of items installed */
	LinePtr 	lp;		/* records holder */
} SystemFile;

SystemFile *sf;

typedef struct _DeviceFile {
	DmItemPtr	itp;
	DmContainerPtr  cp;
	DmObjectPtr	select_op;
	Items *		popupMenuItems;
	Widget		w_enabled;
	Widget		w_modem;
	Widget		w_speed;
	Widget		w_port;
	Widget		w_otherPort;
	Widget		w_otherSpeed;
	Widget		w_acu;
	Widget		iconbox;
	Widget		cancelNotice;
	Widget		footer;
	Widget		propPopup;
	Widget		QDPopup;
	Widget		openNotice;
	Widget		openTextField;
	Widget		quitNotice;
	Widget		saveTextField;
	Widget		toplevel;
	Widget		w_direction;
	int		request_type;
	char *		filename;	/* Used by open and save */
	char *		disabled_filename;	/* Used by open and save */
	char *		incoming_filename;	/* Used by open and save */
	char *		saveFilename;	/* Used by saveas... */
} DeviceFile;

DeviceFile *df;

#endif /* _UUCP_H */
