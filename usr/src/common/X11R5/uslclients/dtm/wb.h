#ifndef _wb_h
#define _wb_h

#pragma ident	"@(#)dtm:wb.h	1.81"

#include <time.h>
#include <DtI.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/NumericGiz.h>

/* wastebasket properties */
#define VERSION     	"_VERSION"
#define TIME_STAMP  	"_TIMESTAMP"
#define TIMER_STATE  	"_TIMER_STATE"
#define CLEANUP_METHOD	"_CLEANUP_METHOD"
#define TIMER_INTERVAL	"_TIMER_INTERVAL"
#define UNIT_INDEX		"_UNIT_INDEX"
#define RESTART_TIMER	"_RESTART_TIMER"

/* default values for wastebasket timer settings */
#define DEFAULT_WB_TIMER_INTERVAL	7
#define DEFAULT_WB_TIMER_UNIT		2
#define DEFAULT_WB_CLEANUP		0
#define DEFAULT_WB_SUSPEND_TIMER	False
#define DEFAULT_WB_RESTART_TIMER	False

#define FTIMEUNIT (86400000)
#define DAYUNIT   (86400000)
#define HOURUNIT  (3600000)
#define MINUNIT   (60000)
#define NUM_ALLOC 10

/* macros */
#define WB_BY_TIMER(w)    (w.cleanUpMethod == 0)
#define WB_ON_EXIT(w)     (w.cleanUpMethod == 1)
#define WB_IMMEDIATELY(w) (w.cleanUpMethod == 2)
#define WB_NEVER(w)       (w.cleanUpMethod == 3)

enum WB_OPS {
	DM_PUTBACK,
	DM_WBDELETE,
	DM_EMPTY,
	DM_DROP,
	DM_TIMER,
	DM_MOVETOWB,
	DM_MOVEFRWB,
	DM_IMMEDDELETE,
	DM_TIMERCHG
};

enum CleanUp_Methods {
	WBByTimer,
	WBOnExit,
	WBImmediately,
	WBNever
};

typedef struct {
	DmGlyphPtr	fgp;
	DmGlyphPtr	egp;
	XtIntervalId	timer_id;
	char		*time_str;
	char      	*dtinfo;
	char	     	*wbdir;
	Boolean		suspend;
	Boolean		restart;
	int		cleanUpMethod;
	int		interval;
	time_t		tm_start;
	unsigned int	unit_idx;
	unsigned long	time_unit;
	unsigned long	tm_remain;
	unsigned long	tm_interval;
	Widget		iconShell;
	Widget		rowCol;
	Gizmo		propGizmo;
	Gizmo		delFileGizmo;
	Gizmo		numericGizmo;
	DmFclassPtr	fcp;
	DmFnameKeyRec	key;
} DmWbDataRec, *DmWbDataPtr;

typedef struct {
	Screen         *screen;
	DmItemPtr      itp;
	long           serial;
	int            op_type;
	Window         client;
	Atom           replyq;
	unsigned short version;
} DmWBMoveFileReqRec, *DmWBMoveFileReqPtr;

typedef struct {
	int       op_type;
	DmItemPtr itp;
	char      *target;
} DmWBCPDataRec, *DmWBCPDataPtr;

/* external variables */
extern DmWbDataRec wbinfo;

/* external functions */
extern void DmWBTimerProc(XtPointer client_data, XtIntervalId timer_id);
extern void DmWBEMPutBackCB(Widget, XtPointer, XtPointer);
extern void DmWBEMFilePropCB(Widget, XtPointer, XtPointer);
extern void DmWBEMDeleteCB(Widget, XtPointer, XtPointer);
extern void DmWBIconMenuCB(Widget, XtPointer, XtPointer);
extern void DmWBPropCB(Widget, XtPointer, XtPointer);
extern void DmWBHelpCB(Widget, XtPointer, XtPointer);
extern void DmWBPropHelpCB(Widget, XtPointer, XtPointer);
extern void DmWBHelpTOCCB(Widget, XtPointer, XtPointer);
extern void DmWBHelpDeskCB(Widget, XtPointer, XtPointer);
extern void DmConfirmEmptyCB(Widget, XtPointer, XtPointer);
extern void DmWBDelete(char ** src_list, int src_cnt, DmWBCPDataPtr cpdp);
extern int DmWBGetVersion(DmItemPtr itp, char *fname);
extern void DmSwitchWBIcon();
extern void DmWBRestartTimer();
extern void DmLabelWBFiles();
extern void DmEmptyYesWB(Widget, XtPointer, XtPointer);
extern void DmEmptyNoWB(Widget, XtPointer, XtPointer);
extern void DmEmptyHelpWB(Widget, XtPointer, XtPointer);
extern void DmWBTimerCB(Widget, XtPointer, XtPointer);
extern void DmWBClientProc(DmProcReason, XtPointer, XtPointer,
		char *, char *);
extern void DmGetWBPixmaps();
extern void DmCreateWBIconShell(Widget toplevel, Boolean iconic);
extern void DmWBToggleTimerBtn(Boolean sensitive);
extern void DmWBResumeTimer();
extern void DmWBSuspendTimer();
extern DmFileInfoPtr WBGetFileInfo(char *path);
extern void DmSaveWBProps();

#endif /* _wb.h */
