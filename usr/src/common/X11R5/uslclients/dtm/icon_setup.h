#pragma ident	"@(#)dtm:icon_setup.h	1.13"

/******************************file*header********************************

    Description:
     This is a header file for Icon Setup.
*/
			/* includes go here */
#include <signal.h>
#include <errno.h>
#include <libgen.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/Protocols.h>	/* for XmAddWMProtocolCallback */
#include <Xm/DragDrop.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>

#include <MGizmo/Gizmo.h>
#include <MGizmo/MenuGizmo.h>
#include <MGizmo/MsgGizmo.h>
#include <MGizmo/BaseWGizmo.h>
#include <MGizmo/ContGizmo.h>
#include <MGizmo/FileGizmo.h>
#include <MGizmo/ChoiceGizm.h>
#include <MGizmo/LabelGizmo.h>
#include <MGizmo/PopupGizmo.h>
#include <MGizmo/SeparatorG.h>
#include <MGizmo/ModalGizmo.h>
#include <MGizmo/TextGizmo.h>

#include <Dt/Desktop.h>

#include "dm_strings.h"
#include "Dtm.h"
#include "extern.h"

/*****************************file*variables******************************

    Define global variables and #defines, and
    Declare externally referenced variables
*/

#define SET_INPUT_GIZMO_TEXT(w,s) SetInputGizmoText(w, (s)?(s):"")
#define ITEM_CLASS(IP)	((DmFnameKeyPtr)(ITEM_OBJ(IP)->fcp->key))

#define DFLT_PIXMAP_LIBRARY_PATH	"/usr/X/lib/pixmaps"
#define DFLT_BITMAP_LIBRARY_PATH	"/usr/X/lib/bitmaps"

#define VALUE(s)		real_string(s)
#define OLD_VAL(FIELD)		wip->old.FIELD
#define NEW_VAL(FIELD)		wip->new.FIELD

#define VAR_ICON_FILE(s)	(s[0] == '%' && (s[1] == 'f' || s[1] == 'F'))

#define PRIVILEGED_USER()	(system("/sbin/tfadmin -t tfadmin 2>/dev/null")\
					== 0)

#define CB_PROTO	Widget w, XtPointer client_data, XtPointer call_data

typedef struct _settings {
	char	*class_name;
	char	*pattern;
	char	*prog_to_run;
	char	*label;
	char	*open_cmd;
	char	*drop_cmd;
	char	*print_cmd;
	char	*icon_file;
	char	*dflt_icon_file;
	char	*filepath;
	char	*lpattern;
	char	*lfilepath;
	char	*tmpl_name;
	char	**tmpl_list;
	int	num_tmpl;
	int	prog_type;
	int	in_new;
	int	to_wb;
	int	load_multi;
} Settings;

typedef struct winInfo *WinInfoPtr;

typedef struct _menuInfo {
	WinInfoPtr	wip;
	DmFnameKeyPtr	fnkp;
} MenuInfo, *MenuInfoPtr;

typedef struct winInfo {
	int		class_type;
	Boolean		new_class;
	Boolean		no_ftype;
	Settings	old;
	Settings	new;
	Pixmap		p[2];
	Widget		w[25];
	Gizmo		g[15];
	Gizmo		popupGizmo;
	Gizmo		iconLibraryGizmo;
	Gizmo		tmplFileGizmo;
	Gizmo		iconFileGizmo;
	DmFolderWinPtr	lib_fwp;
	DmHelpInfoPtr	hip;
	MenuInfoPtr	mip;
} WinInfo;

typedef enum {
	BasicOptions, FileTyping, IconActions, Templates
} Page;

typedef enum {
	Old, New
} Which;

typedef enum {
	OldToNew, NewToOld
} CopyMode;

typedef enum {
	TemplateAdd, TemplateModify, TemplateDelete
} TemplateActions;

typedef enum {
	FileType, DirType, AppType
} ClassType;

typedef enum {
	Add, Apply, Reset, Cancel, Help
} ActionType;

typedef enum {
	P_IconStub,
	P_IconPixmap
} PixmapFields;

typedef enum {
/* all types */
	W_ClassName,
	W_Pattern,
	W_LPattern,
	W_FilePath,
	W_LFilePath,
	W_IconFile,
	W_IconStub,
	W_IconMenu,
	W_AddMenu,
	W_PropMenu,
	W_Shell,
/* File and App Type only */
	W_Category,
	W_ProgType,
	W_Open,
/* File Type only */
	W_ProgToRun,
	W_InNew,
	W_Print,
	W_List,
	W_TmplName,
	W_TmplMenu,
	W_TmplFind,
/* Application Type only */
	W_ToWB,
	W_Drop,
	W_LoadMulti,
} WidgetFields;

typedef enum {
/* all Types */
	G_ClassName,
	G_Pattern,
	G_LPattern,
	G_FilePath,
	G_LFilePath,
	G_IconStub,
	G_IconFile,
/* File and App Type only */
	G_ProgType,
	G_WbNew,
	G_Open,
/* File Type only */
	G_ProgToRun,
	G_Print,
	G_List,
	G_TmplName,
/* Application Type only */
	G_Drop,
} GizmoFields;

extern Widget NewFileShell;
extern Widget NewFolderShell;
extern Widget NewAppShell;
extern char *TypeNames[];
extern char **FileProgTypes;
extern char **AppProgTypes;
extern char *dflt_iconfile[];
extern char *dflt_dflticonfile[];
extern char *dflt_open_cmd[3][2];
extern char *dflt_drop_cmd[2][2];
extern char *dflt_print_cmd;
extern char *systemLibraryPath;
extern DmFolderWindow base_fwp; /* Icon Setup folder window */

extern Atom targets[1];
extern DmHelpInfoRec helpInfo;

extern int num_file_ptypes;
extern int num_app_ptypes;

/**************************forward*declarations****************************/

                         /* public procedures         */
extern unsigned int DmInitIconSetup(char *geom_str, Boolean iconic,
		Boolean map_window);
extern void DisplayTemplates(char **template, int nitems, Widget listW);
extern void DmExitIconSetup();
extern void DmISAlignIcons(DmFolderWinPtr fwp);
extern Widget DmISCreateFileWin(DmObjectPtr op, WinInfoPtr wip,
		Boolean *bad_icon_file);
extern Widget DmISCreateAppWin(DmObjectPtr op, WinInfoPtr wip,
		Boolean *bad_icon_file);
extern Widget DmISCreateFolderWin(DmObjectPtr op, WinInfoPtr wip,
		Boolean *bad_icon_file);
extern void DmISExtractClass(DtAttrs attrs);
extern void DmISTmplFindMenuCB(CB_PROTO);
extern void DmISIconFindMenuCB(CB_PROTO);
extern void DmISAddMenuCB(CB_PROTO);
extern void DmISCategoryMenuCB(CB_PROTO);
extern void DmISIconMenuCB(CB_PROTO);
extern void DmISProgTypeMenuCB(CB_PROTO);
extern void DmISTmplListSelectCB(CB_PROTO);
extern void DmISTemplateMenuCB(CB_PROTO);
extern void DmISTmplFindCB(CB_PROTO);
extern void DmISIconLibraryMenuCB(CB_PROTO);
extern void DmISOtherIconMenuCB(CB_PROTO);
extern void DmISIconPathMenuCB(CB_PROTO);
extern void DmISSelectCB(CB_PROTO);
extern void DmISUnSelectCB(CB_PROTO);
extern void DmISDblSelectCB(CB_PROTO);
extern void DmIsGetProgTypes();
extern void DmISIconStubDropCB(CB_PROTO);
extern void DmISDestroyIconLibrary(CB_PROTO);
extern int DmISInsertClass(WinInfoPtr wip);
extern int DmISModifyClass(WinInfoPtr wip);
extern void DmISDeleteClass(DmItemPtr ip);
extern void DmISFreeContainer(DmContainerPtr cp);
extern void DmISSwitchIconLibrary(WinInfoPtr wip, char *path);
extern void DmISUpdateTemplates(WinInfoPtr wip);
extern void DmISShowProperties(DmItemPtr ip);
extern int DmISValidateInputs(WinInfoPtr wip);
extern char *real_string(char *str);
extern void DmISGetIDs(WinInfoPtr wip, int type);
extern void DmISFreeSettings(WinInfoPtr wip, Boolean flag);
extern void DmISCopySettings(WinInfoPtr wip, int flag);
extern void DmISFreeTemplates(WinInfoPtr wip, int which);
extern void DmISResetTemplateList(WinInfoPtr wip);
extern void DmISInitClassWin(DmObjectPtr op, WinInfoPtr wip, int type);
extern void DmISResetValues(WinInfoPtr wip);
extern int DmISUpdateIconStub(char *icon_file, Widget stub, DmFolderWindow fwp,
		Pixmap *p, Boolean display_error);
extern void RegisterFocusEH(WinInfoPtr wip);
extern int DmISShowIcon(WinInfoPtr wip);
extern void DmISResetProgTypeMenu(WinInfoPtr wip, char *menu_name,
		int new_index, int num_btns);
extern void DmISResetYesNoMenu(WinInfoPtr wip, int new_idx);
extern void DmISCreatePixmapFromBitmap(Widget, Pixmap *, DmGlyphPtr);
