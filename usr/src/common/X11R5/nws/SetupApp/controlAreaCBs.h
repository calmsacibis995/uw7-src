#ident	"@(#)controlAreaCBs.h	1.2"
#ifndef CONTROL_AREA_CBS_H
#define CONTROL_AREA_CBS_H


extern void  categoryCB (Widget, XtPointer callData, XmAnyCallbackStruct *);
extern void  optionCB (Widget w, XtPointer callData, XmAnyCallbackStruct *);
extern void  setOptionMenuValue (Widget optionMenu, setupObject_t *curObj);
extern void  popupDialogCB (Widget w, setupObject_t *obj,
					    XmToggleButtonCallbackStruct *cbs);
extern void  varListChgCB (Widget w, SetupWin *win, XConfigureEvent *event);
extern void  labelCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *);
extern void  focusCB (Widget w, setupObject_t *curObj, XmAnyCallbackStruct *);
extern void  losingFocusCB (Widget, setupObject_t *curObj,
					    XmTextVerifyCallbackStruct *cbs);
extern void  passwdTextCB (Widget, char **password,
					    XmTextVerifyCallbackStruct *cbs);
extern void  toggleCB (Widget w, setupObject_t *curObj,
					    XmToggleButtonCallbackStruct *cbs);
extern void  cbFunc (setupObject_t *curObj);
extern Boolean inputIsSetToThisApp (Widget widget, Widget *widList);


#endif	// CONTROL_AREA_CBS_H
