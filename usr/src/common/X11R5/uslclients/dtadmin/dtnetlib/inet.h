#ifndef NOIDENT
#ident	"@(#)dtnetlib:inet.h	1.8"
#endif

/* These two structures are for uucp/Systems.tcp entry - non etcHost ent */

typedef struct _systems_entry {
	char *	name;
	char *	haddr;
	char *	cmt;
	char *	preserve;
} systemsEntry;

typedef struct _systems_list {
	systemsEntry *	list;
	int		count;
} systemsList;

typedef struct _permEntry {
	char *	hostName;
	char *	id;
	char *	preserve;
} permEntry;

typedef struct _securityList {
	permEntry	*list;
	int		count;
} securityList;

typedef struct _idString {
	char	*id;
} idString;

typedef struct _idList {
	idString	*list;
	int		count;
} idList;

typedef enum {
	ETC,
	NONETC
} uucpType;

typedef enum {
	List,
	PushButton,
	TextField
} widType;

typedef enum {
	new,
	prop
} focusType;

typedef struct _newPropInfo {
	Widget		popup;
	Widget		sysName;
	Widget		sysLabel;
	Widget		getaddr;
	inetAddr	addr;
	Widget		naLabel;
	Widget		comment;
	ActionAreaItems *actions;
	Widget		aliases;
	Widget		add;
	Widget		mod;
	Widget		delete;
	Widget		list;
	Widget		status;
        focusType       cur_focus;      /* current focus on New/Properties */
        Boolean         isNew;          /* Is New is up */
        Boolean         isProp;         /* Is Prop is up */
        Boolean         isPropChg;      /* Is Prop changed */ 
} newPropInfo;

typedef struct _inetInfo {
	systemsList	*uucp;
	widType		cur_wid; /* widget that has current focus on with mutilple popups */
	newPropInfo	*newInfo;
	newPropInfo	*propInfo;
} inetInfo;

typedef struct _hostInfo {
	netInfo		net;
	inetInfo	inet;
} hostInfo;

hostInfo	hi; /* global structure for accessing host data in all files */


static char	*hostEquivPath = "/etc/hosts.equiv";
static char	*uucpPath = "/etc/uucp/Systems.tcp";
static char	*etcHostPath = "/etc/hosts";
