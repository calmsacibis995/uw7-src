#pragma ident	"@(#)dtm:extern.h	1.224.1.1"

#ifndef _extern_h
#define _extern_h

/************************************************************************

	FUNCTION PROTOTYPES
*/
/* "exit" points for Desktop */
extern void	DmPromptExit(Boolean);
extern void	DmShutdownPrompt();
extern void	DmExit();
extern void     DmPrepareForRestart();

/*	Folder Routines							*/

/* Callbacks for Folder "File" menu items */
extern void     DmFileCopyCB(Widget, XtPointer, XtPointer);
extern void     DmFileDeleteCB(Widget, XtPointer, XtPointer);
extern void     DmFileLinkCB(Widget, XtPointer, XtPointer);
extern void     DmFileMoveCB(Widget, XtPointer, XtPointer);
extern void     DmFileNewCB(Widget, XtPointer, XtPointer);
extern void     DmFileOpenCB(Widget, XtPointer, XtPointer);
extern void 	DmFileOpenNewCB(Widget, XtPointer, XtPointer);
extern void     DmFilePrintCB(Widget, XtPointer, XtPointer);
extern void     DmFilePropCB(Widget, XtPointer, XtPointer);
extern void     DmFileRenameCB(Widget, XtPointer, XtPointer);
extern void     DmFileConvertD2UCB(Widget, XtPointer, XtPointer);
extern void     DmFileConvertU2DCB(Widget, XtPointer, XtPointer);
extern void     DmCloseNewWindow(CreateInfo *cip);
extern void     DmDestroyNewWindow(DmFolderWindow folder);

/* Callback for New Button*/
extern void    DmPromptCreateFile(DmFolderWinPtr fwp);

/* Callback for Find Button*/
extern void	DmFindCB(Widget, XtPointer, XtPointer);

/* Callbacks for Folder "View" menu items */
extern void	DmViewAlignCB(Widget, XtPointer, XtPointer);
extern void	DmViewCustomizedCB(Widget, XtPointer, XtPointer);
extern void	DmViewFormatCB(Widget, XtPointer, XtPointer);
extern void	DmViewSortCB(Widget, XtPointer, XtPointer);
extern void	DmFormatView(DmFolderWinPtr, DmViewFormatType);
extern void	DmSortItems(DmFolderWindow, DmViewSortType,
			    DtAttrs calc_geom_otions,
			    DtAttrs layout_options, Dimension wrap_width);

/* Callbacks for Folder "Edit" menu items */
extern void	DmEditSelectAllCB(Widget, XtPointer, XtPointer);
extern void	DmEditUnselectAllCB(Widget, XtPointer, XtPointer);
extern void	DmEditUndoCB(Widget, XtPointer, XtPointer);

/* Callbacks for Folder "Folder" menu items */
extern void	DmFolderOpenDirCB(Widget, XtPointer, XtPointer);
extern void	DmFolderOpenOtherCB(Widget, XtPointer, XtPointer);
extern void	DmFolderOpenParentCB(Widget, XtPointer, XtPointer);
extern void	DmFolderOpenTreeCB(Widget, XtPointer, XtPointer);
extern void	DmGotoDesktopCB(Widget, XtPointer, XtPointer);


/* Callbacks for Folder/Wastebasket "Help" menu items */
extern void	DmBaseWinHelpKeyCB(Widget, XtPointer, XtPointer);
extern void	DmPopupWinHelpKeyCB(Widget, XtPointer, XtPointer);
extern void	DmHelpSpecificCB(Widget, XtPointer, XtPointer);
extern void     DmRegContextSensitiveHelp(Widget widget, int app_id,
			char *file, char *section);
extern void	DmHelpTOCCB(Widget, XtPointer, XtPointer);
extern void	DmHelpMAndKCB(Widget, XtPointer, XtPointer);
extern void	DmHelpDeskCB(Widget, XtPointer, XtPointer);

/* External routines for Tree View */
extern void	Dm__UpdateTreeView(char * dir1, char * dir2);
extern void	TreeIconMenuCB(Widget, XtPointer, XtPointer);

/* Routines for drawing non-standard file glyphs (icons) */
extern void	DmDrawLongIcon(Widget, XtPointer, XtPointer);
extern void	DmDrawNameIcon(Widget, XtPointer, XtPointer);
extern void	DmDrawTreeIcon(Widget, XtPointer, XtPointer);
extern void	DmDrawLinkIcon(Widget, XtPointer, XtPointer);

/*	Folder: to popup file property sheet(s)			*/
extern void	DmBusyPropSheets(DmFolderWindow folder, Boolean busy);
extern void	Dm__PopupFilePropSheet(DmFolderWinPtr, DmItemPtr);
extern void	Dm__PopupFilePropSheets(DmFolderWinPtr);
extern DmFPropSheetPtr         GetNewSheet(DmFolderWinPtr window);
extern void	DmPopdownAllPropSheets(DmFolderWinPtr);

/*	Sync Timer						*/
extern void	Dm__SyncTimer(XtPointer client_data, XtIntervalId * timer_id);
extern void	Dm__SyncContainer(DmContainerPtr cp, Boolean force);
extern void	Dm__RmFromStaleList(DmContainerPtr cp);

/*	Visited Folders						*/
extern void	Dm__UpdateVisitedFolders(char * old_path, char * new_path);

/*	File Operations						*/

extern DmTaskInfoListPtr DmDoFileOp(DmFileOpInfoPtr, DmClientProc, XtPointer);
extern int	DmSetupFileOp(DmFolderWindow, DmFileOpType, char *, void **);
extern int	DmUndoFileOp(DmTaskInfoListPtr tip);
extern void	DmStopFileOp(DmTaskInfoListPtr tip);
extern void	Dm__Overwrite(DmTaskInfoListPtr tip, Boolean overwrite);
extern void	Dm__NameChange(DmTaskInfoListPtr tip, Boolean overwrite);
extern void	DmFolderFMProc(DmProcReason, XtPointer client_data,
			       XtPointer call_data, char * str1, char * str2);

extern void	DmDropProc(Widget, XtPointer, XtPointer);
extern void	DmDblSelectProc(Widget, XtPointer, XtPointer);
extern void	DmIconMenuProc(Widget, XtPointer, XtPointer);
extern void	DmIconMenuCB(Widget, XtPointer, XtPointer);
extern void	DmWBPutBackByDnD(DmFolderWinPtr dst_win,ExmFlatDropCallData *d,
			void **list);

/*	Interface to the Help subsystem				*/

extern DmHelpAppPtr	DmNewHelpAppID(Screen *scrn,
				       Window win_id,
				       char *app_name,
				       char *app_title,
				       char *node,
				       char *help_dir,
				       char *icon_file);
extern DmHelpAppPtr	DmGetHelpApp(int app_id);

extern void          DmDisplayHelpSection(DmHelpAppPtr hap,
                              char *title,
                              char *file_name,
                              char *sect_name);

extern void		DmDisplayHelpTOC(Widget w, DmHelpWinPtr hwp,
					 char *file_name, int app_id);


extern void Dm__RootPropertyEventHandler(Widget w,
					 XtPointer client_data,
					 XEvent *xevent,
					 Boolean cont_to_dispatch);

/* Object utility functions */

extern Boolean	DmObjMatchFilter(DmFolderWindow folder, DmObjectPtr objptr);
extern void	DmDropObject(DmWinPtr dst_win, Cardinal dst_indx,
			DmWinPtr src_wp, void *src, void **src_list,
			char *(*expand_proc)(), int flag);
extern DmObjectPtr DmFileToObject(char *file);
extern void	DmOpenObject(DmWinPtr wp, DmObjectPtr op, DtAttrs attrs);
extern void	DmPrintObject(DmWinPtr wp, DmObjectPtr op);
extern void	DmRenameObject(DmObjectPtr obj, char *new_name);

extern void	DeleteItems(DmFolderWindow, DmItemPtr *, Position, Position);
extern int	DmExecCommand(DmWinPtr wp, DmObjectPtr op,
			      char * name, char * str);
extern int	DmExecuteShellCmd(DmWinPtr wp, DmObjectPtr op, char * cmdstr,
				  Boolean force_chdir);
extern int	DmSameOrDescendant(char * path1, char * path2, int path1_len);
extern void	DmTouchIconBox(DmWinPtr window, ArgList, Cardinal);
extern void	Dm__StampContainer(DmContainerRec *);
extern char *DmGetDTProperty(char *name, DtAttrs *attrs);
extern int DmOpenDesktop();
extern DmFnameKeyPtr DmReadFileClassDB(char *filename);
extern DmFnameKeyPtr DmGetFileClass(char *path, DtPropListPtr pp);
extern DmFnameKeyPtr DmGetNetWareServerClass();
extern DmFnameKeyPtr DmLookUpObjClass(DmContainerPtr cp, char *obj_name,
	DtPropListPtr pp);
extern char *DmGetFolderIconFile(char *path);
extern int DmWriteFileClassDBList(DmFnameKeyPtr fnkp);
extern void DmSetFileClass(DmObjectPtr op);
extern char *DmResolveLFILEPATH(DmFnameKeyPtr fnkp, char *p);
extern void DmInitFileClass(DmFnameKeyPtr fnkp);
extern void DmFreeFileClass(DmFnameKeyPtr fnkp);
extern void DmInitSmallIcons(Widget w);
extern DmFnameKeyPtr DmGetClassInfo(DmFnameKeyPtr fnkp_list, char *name);
extern DmObjectPtr	Dm__CreateObj(DmContainerPtr, char *, DtAttrs);
extern DmObjectPtr	Dm__NewObject(DmContainerPtr cp, char * name);
extern void		Dm__FreeObject(DmObjectPtr op);
extern DmContainerPtr	Dm__NewContainer(char *name);
extern int DmReadDtInfo(DmContainerPtr cp, char *filepath, DtAttrs options);
extern void DmWriteDtInfo(DmContainerPtr cp, char *filepath, DtAttrs options);
extern int DmRestartSession(char *path);
extern void DmSaveSession(char *path);
extern void DmSaveDesktopProperties(DmDesktopPtr desktop);
extern int Dm__ReadDir(DmContainerPtr cp, DtAttrs options);
extern DmContainerPtr DmOpenDir(char *path, DtAttrs options);
extern int DmCloseDir(char *path, DtAttrs options);
extern void DmDestroyContainer(DmContainerPtr cp);
extern int DmFlushDir(char *path);
extern int DmFlushContainer(DmContainerPtr cp);
extern DmContainerPtr DmQueryContainer(char *path);
extern DmFmodeKeyPtr DmFtypeToFmodeKey(DmFileType ftype);
extern DmFmodeKeyPtr DmStrToFmodeKey(char *str);
extern DmFolderWinPtr DmOpenFolderWindow(char *path, DtAttrs attrs,
                   			 char *geom_str, Boolean iconic);
extern DmFolderWinPtr DmOpenInPlace(DmFolderWinPtr folder, char *path);

extern void	DmPrintCB(Widget, XtPointer, XtPointer);
extern void DmFolderPathChanged(char * old_path, char * new_path);
extern char * DmMakeFolderTitle(DmFolderWindow window);
extern DmFolderWinPtr DmQueryFolderWindow(char *path, DtAttrs options);
extern DmFolderWinPtr DmFindFolderWindow(Widget widget);
extern DmFolderWinPtr DmIsFolderWin(Window win);
extern void DmCloseFolderWindows(void);
extern void DmCloseFolderWindow(DmFolderWindow);
extern void DmCloseView(DmFolderWindow window, Boolean destroy);
extern void DmWindowSize(DmWinPtr window, char *buf);
extern void DmSaveXandY(DmItemPtr ip, int count);
extern int DmAddObjectToView(DmFolderWindow, Cardinal, DmObjectPtr,
			     WidePosition, WidePosition);
extern void DmRmObjectFromContainer(DmContainerPtr cp, DmObjectPtr op);
DmObjectPtr DmAddObjectToContainer(DmContainerPtr cp, DmObjectPtr obj, 
				   char *name, DtAttrs options);
extern void DmDelObjectFromContainer(DmContainerPtr cp, DmObjectPtr op);
extern DmObjectPtr DmGetObjectInContainer(DmContainerPtr cp, char *name);
extern void DmAddContainer(DmContainerBuf *cp_buf, DmContainerPtr cp);
extern void DmRemoveContainer(DmContainerBuf *cp_buf, DmContainerPtr cp);
extern void DmLockContainerList(Boolean lock);
extern void DmWorkingFeedback(DmFolderWindow folder, unsigned long interval, 
			      Boolean busy);

extern void DmClassNetWareServers(DmContainerPtr cp);
extern char * DmClassName(DmFclassPtr fcp);
extern char * DmObjClassName(DmObjectPtr op);
extern DmItemPtr DmObjNameToItem(DmWinPtr win, register char * name);
extern int DmObjectToIndex(DmWinPtr wp, DmObjectPtr op);
extern DmItemPtr DmObjectToItem(DmWinPtr wp, DmObjectPtr op);
extern void **DmGetItemList(DmWinPtr window, int item_index);
extern void **DmOneItemList(DmItemPtr ip);
extern char **DmItemListToSrcList(void **ilist, int *count);

extern void	DmComputeItemSize(Widget icon_box, DmItemPtr item, 
				  DmViewFormatType view_type, 
				  Dimension * width, Dimension * height);

extern void DmComputeLayout(Widget, DmItemPtr, int count, int type,
			    Dimension width,
			    DtAttrs options,    /* size, icon pos. options */
			    DtAttrs lattrs);    /* layout attributes */

extern void DmFreeTaskInfo(DmTaskInfoListPtr tip);
extern void DmUpdateWindow(DmFileOpInfoPtr, DtAttrs update_options);
extern void DmUpdateFolderTitle(DmFolderWindow);
extern void DmSetSwinSize(Widget swin);
extern void UpdateFolderCB(Widget, XtPointer, XtPointer);

extern unsigned int DmInitWasteBasket(char *geom_str, Boolean iconic,
				      Boolean map_window);
extern unsigned int DmInitIconSetup(char *geom_str, Boolean iconic,
				      Boolean map_window);
extern void DmISHandleFontChanges();
extern void DmExitIconSetup();

extern void DmSetISMenuItem(MenuItems **, char *, int, DmFolderWinPtr);
extern void DmSetHDMenuItem(MenuItems **, char *, int, DmFolderWinPtr);
extern void DmSetWBMenuItem(MenuItems **, char *, int, DmFolderWinPtr);
extern char **DmParseList(char *list, int *nitems);

extern char *DmGetLongName(DmItemPtr item, int len, Boolean no_iconlabel);

/* Help Desk external routines */
extern unsigned int DmInitHelpDesk(char *geom_str, Boolean iconic,
                          Boolean map_window);
extern void DmHDExit();
extern void DmHDOpenCB();


/* Wastebasket external routines */
extern int	DmWBCleanUpMethod(DmDesktopPtr);
extern void	DmWBExit(void);
extern void	DmWBFilePropCB(Widget, XtPointer, XtPointer);
extern void	DmMoveFileToWB(DmFileOpInfoPtr, Boolean client_req);
extern void	DmMoveFileFromWB(DmFileOpInfoPtr opr_info);
extern int	DmMoveToWBProc2(char *pathname, Screen*, XEvent*, DtRequest*);
extern DmFolderWinPtr DmIsWB(Window);

extern void	DmHMExit();

/* functions to expand property */
extern char *	DmObjProp(char *str, XtPointer client_data);
extern char *	DmDropObjProp(char *str, XtPointer client_data);
extern void	DmSetSrcObject(DmObjectPtr op);
extern void	DmSetSrcWindow(DmWinPtr wp);
extern char *	DmGetSrcWinPath();
extern void	DmSetExpandFunc(char *(*fp)(), void **list, int flag);
extern char *	DmDTProp(char *str, XtPointer client_data);

extern void	Dm__VaPrintMsg(char * msg, ... );
extern void	DmVaDisplayState(DmWinPtr window, char * msg, ...);
extern void	DmVaDisplayStatus(DmWinPtr window, int type, char * msg, ...);
extern char *	StrError(int err);
extern void	DmBusyWindow(Widget w, Boolean busy);
extern void DmAddWindow(DmWinPtr *list, DmWinPtr newp);
extern char *Dm__gettxt(char *msg);
extern void DmMenuSelectCB(Widget, XtPointer, XtPointer);
extern void DmSetWinPtr(DmWinPtr wp);
extern XtPointer DmGetWinPtr(Widget);
extern XtPointer DmGetLastMenuShell(Widget *);
extern void DmSetToolbarSensitivity(DmWinPtr window, int);
extern void DmDisplayStatus(DmWinPtr window);
extern void DmMapWindow(DmWinPtr window);
extern void DmUnmapWindow(DmWinPtr window);
extern void DmButtonSelectProc(Widget, XtPointer, XtPointer);
extern void DmRetypeObj(DmObjectPtr op, Boolean notify);
extern void DmSyncWindows(DmFnameKeyPtr new_fnkp, DmFnameKeyPtr del_fnkp);
extern DmFclassFilePtr DmCheckFileClassDB(char *filename);
extern Boolean DmMatchFilePath(char *p, char *path);
extern char *DmToUpper(char *str);
extern Dimension DmFontWidth(Widget iconbox);
extern Dimension DmFontHeight(Widget iconbox);

/* Drag-and-Drop functions */
extern Boolean DtmConvertSelectionProc(
	Widget w,
	Atom *selection,
	Atom *target,
	Atom *type_rtn,
	XtPointer *val_rtn,
	unsigned long *length_rtn,
	int *format_rtn,
	unsigned long *ignored_max_len,
	XtPointer *client_data,
	XtRequestId *ignored_req_id);

extern DmDnDInfoPtr
DtmDnDNewTransaction(
	DmWinPtr wp,
	DmItemPtr *ilist,
	DtAttrs attrs,
	Window dst_win,
	Atom selection,
	unsigned char operation);

extern void DmFolderTriggerNotify(Widget w, XtPointer client_data,
	XtPointer call_data);

extern void DtmDnDFinishProc(Widget w, XtPointer client_data,
	XtPointer call_data);

extern void DtmDnDFreeTransaction(DmDnDInfoPtr dip);

extern void DmDnDRegIconShell(Widget w, XtPointer client_data, XEvent *xevent,
		Boolean *cont_to_dispatch);

#endif /* _extern_h */
