#ifndef NOIDENT
#pragma ident	"@(#)dtnetlib:lookupG.h	1.10"
#endif

#include <Xm/Xm.h>

/* NOTE:  Include lookup.h in front of lookupG.h in all the source files */

typedef void	(*funcdef)();


typedef struct _menu_item {
	char		*label;		/* the label for the item */
	WidgetClass	*class; 	/* pushbutton, label, separator... */	
	char *		mnemonic;	/* mnemonic; NULL if none */
	char		*accelerator;	/* accelerator; NULL if none */
	char		*accel_text;	/* to be converted to compound string */
	void		(*callback)();	/* routine to call; NULL if none */
	XtPointer	callback_data;	/* client_data for callback() */
	struct _menu_item *subitems;	/* pullright menu items, if not NULL */
	Boolean		sensitivity;
	Widget		handle;	
} MenuItem;

typedef struct {
    char        *title;
    char        *file;
    char        *section;
} HelpText;

typedef struct _ActionAreaItems{
	char *		label;
	char *		mnemonic;
	void		(*callback)();
	XtPointer	data;
	Widget		widget;
} ActionAreaItems;

typedef enum {
        etcHost,
        DNS
} viewType;

typedef struct _etcInfo {
	etcHostList	*etcHosts;
	Widget	etcRC;
	Widget	etcSwin;
	Widget	etcList;
	funcdef	clickCB; 	/* inet-sepecific function */
	int	etcHostIndex;   /* current index into the etcHostList */
	char	*etcSelection;	/* current selection from etcHostList */
} etcInfo;

typedef enum {
	DNSL1,
	DNSL2,
	DNSL3
} dnsListNum;

typedef enum {
        NONE,
        GET_ADDRESS,
        NAME_SERVERS,
        HOSTS,
} nsQueryType;

typedef enum {
	START,
	SHOWDOMAIN,
	DOUBLECLICK,
} nsQueryFrom;

typedef struct _dnsInfo {
	Widget	dnsRC;
	Widget	dnsArray[3];
	Widget	leftArrow;
	Widget	rightArrow;
	Widget	domainText;	/* text widget */
	Widget	domainLabel;	/* label widget */
	Widget	showDomainBut;	/* push button widget */
	Widget	listLabel[3];
	Widget	errorMsg;	/* popup error box */
	funcdef	clickCB;	/* inet specific function */
	resolvConf	*resolv;
	dnsList		*dnsHosts;	/* for dnsQuery() only */
	dnsListNum	cur_wid_pos;	/* position in the list widget array */
	dnsList		*cur_pos;	/* position in the linked list: always
					   points to the 1st widget */
	dnsList		*work_pos;	/* position in the linked list that is
					   currently working on. */
	int	totalDnsItems;		/* total number of linked list items */
	int	pid;    /* process id for the current ns query process */
	char    *outfile; /* parse output of the nsquery from this file */
	char	*errfile; /* parse error output for the nsquery from this file */
	nsQueryType query;/* which is the current query */
	nsQueryFrom queryFrom; /* from where the query get call: double click,
				show domain button or initialization */	
	Boolean hostQuerySuccess; /* is the last query to dns successful */
	int serverIndex;        /* to query the current server in nameservers
				list, asynchronous querying  */
	int	dnsIndex[3];	/* current index into the List widget */
	char	*dnsSelection[3];	/* current selection from List widget */
} dnsInfo;


/* lookup info (non-inet) */

typedef struct _lookupInfo {
	Widget		viewBtn;
	Widget		clientText; /* text passed by client to update 
					host name */	
	Widget		selectLabel;
	ActionAreaItems	*actions;	
} lookupInfo;

/* common info */

typedef struct _commonInfo {
	XtAppContext	app;
	Widget		toplevel;	/* toplevel shell in inet, NULL in 
					non-inet */
	Widget		topRC;		/* inet: manage child of toplevel shell */
	Widget		menubar;	/* inet: menubar */
	Atom		COMPOUND_TEXT;	/* for drag and drop */
	XtTranslations	parsed_xlations;/* translation */
	Widget		status;		/* non-inet shares one status line */
	Widget		dnsStatus;	/* inet - status line in DNS view */
	Widget		etcStatus;	/* inet - status line in ETC view */
	Widget		topMShell;	/* inet toplevel shell for message box*/
	Widget		mb;		/* inet toplevel message box */
	Boolean		isInet;
	Boolean		isDnsConfigure;	/* Is Dns configured */
	Boolean		isProp;		/* Is Property window up or not */
	Boolean		isNew;		/* Is the new widnow up or not */
	Boolean		isFirst;	/* is it is the first time to call inet */
	MenuItem	*menu;		/* inet-specific: menu bar */
	Boolean		isOwner;	/* inet-specific: Is privelge user or not */
	viewType	cur_view;	
	XtIntervalId	killQuery;	/* timeout id for the killQuery */
} commonInfo;

typedef struct _netInfo {
	etcInfo		etc;
	dnsInfo		dns;
	lookupInfo	lookup;
	commonInfo	common;
} netInfo;
