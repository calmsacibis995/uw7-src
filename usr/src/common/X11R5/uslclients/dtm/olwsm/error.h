#ifndef __olwsm_error_h__
#define __olwsm_error_h__

#ifndef NOIDENT
#pragma ident	"@(#)dtm:olwsm/error.h	1.25"
#endif

#include "dm_strings.h"
#include <MGizmo/Gizmo.h>

extern char *	GetText(char *);
#define pGGT	GetText

/*
 *************************************************************************
 *
 * Description:
 *		This file contains standard error message strings for
 *	use in the routines OlVaDisplayErrorMsg() and
 *	OlVaDisplayWarningMsg().
 *
 *	When adding strings, the following conventions should be used:
 *
 *		1. Classes begin with OlC, e.g.,
 *			#define COlClientOlpixmapMsg	"olpixmap_msgs"
 *
 *		2. Names begin with N, e.g.,
 *			#define	NinvalidResource	"invalidResource"
 *
 *		3. Types begin with T, e.g.,
 *			#define	TsetValues		"setValues"
 *
 *		4. Error message strings begin with M and is followed
 *		   by the name string, and underbar '_', and concatenated
 *		   with the error type.  For the above error name and type.
 *
 *			#define MinvalidResource_setValues \
 *			   "SetValues: widget \"%s\" (class \"%s\"): invalid\
 *			    resource \"%s\", setting to %s"
 *
 *	Using these conventions, an example use of OlVaDisplayWarningMsg() 
 *	for a bad resource in FooWidget's SetValues procedure would be:
 *
 *	OlVaDisplayWarningMsg(display, NinvalidResource, TsetValues,
 *		COlToolkitWarning, MinvalidResource_setValues,
 *		XtName(w), XtClass(w)->core_class.class_name,
 *		XtNwidth, "23");
 *
 *******************************file*header*******************************
 */

#if	defined(__STDC__)
# define concat(a,b) a ## b
# define concat4(a,b,c,d) a ## b ## c ## d
#else
# define concat(a,b) a/**/b
# define concat4(a,b,c,d) a/**/b/**/c/**/d
#endif

/* #define OLGM(y,x)	OlGetMessage(dpy, Labels[n++], BUFSIZ, \
				     concat(N,x), \
				     concat(T,y), \
				     COlClientOlwsmMsgs, \
				     concat4(M,x,_,y), \
				     (XrmDatabase)NULL) */

/* Changed from Dm__gettxt to GetText */

#define COlClientOlwsmMsgs	"olwsm_msgs"

#ifdef USE_TOOLKIT_I18N
/* #define OLG(y,x)	(char *)OlGetMessage(dpy, NULL, BUFSIZ, \
				     concat(N,x), \
				     concat(T,y), \
				     COlClientOlwsmMsgs, \
				     concat4(M,x,_,y), \
				     (XrmDatabase)NULL) */


/*
 *************************************************************************
 * Define the error names here:  Use prefix of 'N'
 *************************************************************************
 */

#define NbadCommandLine		"badCommandLine"
#define NdupMsg			"dupMsg"
#define NerrorMsg			"errorMsg"
#define NfixedString			"fixedString"
#define NfooterMsg			"footerMsg"
#define NhelpTag			"helpTag"
#define NmaxLabel			"maxLabel"
#define NminLabel			"minLabel"
#define Nmnemonic			"mnemonic"
#define NpageLabel			"pageLabel"
#define NwarningMsg			"warningMsg"

/*
 *************************************************************************
 * Define the error types here:  Use prefix of 'T'
 *************************************************************************
 */

#define T2D				"2D"
#define T3D				"3D"
#define Tabort			"abort"
#define Taccel			"accel"
#define Taccepted			"accepted"
#define Tadjust			"adjust"
#define Tafter			"after"
#define Talternate			"alternate"
#define Talways			"always"
#define Tapply			"apply"
#define Tapplyall			"applyall"
#define TapplyEdits			"applyEdits"
#define TasAGroup			"asAGroup"
#define TbadAssert			"badAssert"
#define TbadKeyMatch			"badKeyMatch"
#define TbadModMatch			"badModMatch"
#define TbasicSet			"basicSet"
#define Tbeep			"beep"
#define Tbefore			"before"
#define TblackOnWhite		"blackOnWhite"
#define Tblue			"blue"
#define Tbottom			"bottom"
#define Tborder			"border"
#define TcacheExceed			"cacheExceed"
#define Tcalculator			"calculator"
#define TcannotWrite			"cannotWrite"
#define TchangeGUI			"changeGUI"
#define TchooseColor			"chooseColor"
#define TclickSelect			"clickSelect"
#define Tclock			"clock"
#define Tcolor			"color"
#define TcolorChoices		"colorChoices"  
#define TcolorCombo			"colorCombo"  
#define TcolorSample			"colorSample"  
#define TcolorStart			"colorStart"
#define TcolorError			"colorError"
#define Tconstrain			"constrain"
#define Tcontinue			"continue"
#define Tcustom			"custom"
#define Tdamping			"damping"
#define TdateTime			"dateTime"
#define TdeletePrime			"deletePrime"
#define Tdesktop			"desktop"
#define TdispDefault			"dispDefault"
#define TdispLang			"dispLang"
#define TdispMenu			"dispMenu"
#define Tdistinct			"distinct"
#define TdragRight			"dragRight"
#define Tduplicate			"duplicate"
#define Texit			"exit"
#define	Tfactory			"factory"
#define	Tfile			"file"
#define TflatKeys			"flatKeys"
#define Tfollow			"follow"
#define TfontGroup			"fontGroup"
#define TforkFailed			"forkFailed"
#define Tfunction			"function"
#define Tgray			"gray"
#define Tgreen			"green"
#define Thelp			"help"
#define ThelpKeyClr			"helpKeyColor"
#define ThelpModel			"helpModel"
#define Ticons			"icons"
#define Tindividually		"individually"
#define TinitKeyHelp			"initKeyHelp"
#define TinputArea			"inputArea"
#define TinputFocus			"inputFocus"
#define TinputLang			"inputLang"
#define TinputWindow			"inputWindow"
#define Tinsert			"insert"
#define Tinterface			"interface"
#define TdupModifier			"dupModifier"
#define Tinvocation			"invocation"
#define TkbdProps			"kbdProps"
#define Tlayering			"layering"
#define Tleft			"left"
#define Tlist			"list"
#define Tlmr				"lmr"
#define Tlocation			"location"
#define Tlogin			"login"
#define Tmenu			"menu"
#define TmenuLabels			"menuLabels"
#define TmenuMarkR			"menuMarkR"
#define Tmiddle			"middle"
#define Tmisc			"misc"
#define TmneFollow			"mneFollow"
#define TmnePreface			"mnePreface"
#define TmneSetting			"mneSetting"
#define TmneUnspec			"mneUnspec"
#define Tmnemonic			"mnemonic"
#define Tmodifier			"modifier"
#define TmouseAcc			"mouseAcc"
#define TmouseBtn			"mouseBtn"
#define TmouseEq			"mouseEq"
#define TmouseM			"mouseM"
#define TmouseMod			"mouseMod"
#define TmouseS			"mouseS"
#define TmouseSelect			"mouseSelect"
#define TmovePointer			"movePointer"
#define TmultiClick			"multiClick"
#define TneedOladduser		"needOladduser"
#define Tname			"name"
#define Tnever			"never"
#define TnextChoice			"nextChoice"
#define Tno				"no"
#define TnoDefaults			"noDefaults"
#define TnoFile			"noFile"
#define TnoShow			"noShow"
#define TnoStepParent		"noStepParent"
#define TnoWidget			"noWidget"
#define Tnone			"none"
#define Tnotices			"notices"
#define TnowPress			"nowPress"
#define Tnum				"num"
#define TnumFormat			"numFormat"
#define Toff				"off"
#define Tolam			"olam"
#define Tolfm			"olfm"
#define Tolps			"olps"
#define TonHighlight			"onHighlight"
#define TonNoShow			"onNoShow"
#define TonShow			"onShow"
#define TonUnderline			"onUnderline"
#define TotherControl		"otherControl"
#define Toutgoing			"outgoing"
#define Tpixeditor			"pixeditor"
#define Tpointer			"pointer"
#define TpoorColors			"poorColors"
#define Tpreface			"preface"
#define Tprimary			"primary"
#define TprogMenu			"progMenu"
#define Tprograms			"programs"
#define Tproperties			"properties"
#define TpropsTitle			"propsT"
#define Tpushpin			"pushpin"
#define Tquoting			"quoting"
#define Tred				"red"
#define Trefresh			"refresh"
#define Treset			"reset"
#define Tright			"right"
#define TsameColors			"sameColors"
#define TscrollPan			"scrollPan"
#define Tselect			"select"
#define TsetLocale			"setLocale"
#define TsetMenuDef			"setMenuDef"
#define Tshow			"show"
#define Tsettings			"settings"
#define TspecSetting			"specSetting"
#define TsuppSetting			"suppSetting"
#define TstepParentNotComposite	"stepParentNotComposite"
#define Tterm			"term"
#define TtextBG			"textBG"
#define TtextFG			"textFG"
#define Ttop				"top"
#define Ttotal			"total"
#define TuseThisArea			"useThisArea"
#define Tutils			"utils"
#define Tvideo			"video"
#define Tview			"view"
#define TwMenu			"wMenu"
#define TwProps			"wProps"
#define TwantExit			"wantExit"
#define TwhiteOnBlack		"whiteOnBlack"
#define TwindowBG			"windowBG"
#define Tworkspace			"workspace"
#define Twsm				"wsm"
#define Tyes				"yes"

/*
 *************************************************************************
 * Define the default error messages here:  Use prefix of 'M'
 * followed by the error name, an underbar <_>, and the error type.
 *************************************************************************
 */

extern String MdupMsg_follow ;
extern String MdupMsg_mneFollow ;
extern String MdupMsg_preface ;
extern String MdupMsg_mnePreface ;
extern String MdupMsg_total ;
extern String MdumMsg_mneUnspec ;

extern String MerrorMsg_badAssert ;
extern String MerrorMsg_distinct ;
extern String MerrorMsg_dupModifier ;
extern String MerrorMsg_needOladduser ;
extern String MerrorMsg_noWidget ;
extern String MerrorMsg_otherControl ;

extern String MfixedString_2D ;
extern String MfixedString_3D ;
extern String MfixedString_abort;
extern String MfixedString_accel ;
extern String MfixedString_accepted ;
extern String MfixedString_adjust ;
extern String MfixedString_after ;
extern String MfixedString_alternate ;
extern String MfixedString_always ;
extern String MfixedString_apply ;
extern String MfixedString_applyall ;
extern String MfixedString_applyEdits ;
extern String MfixedString_asAGroup ;
extern String MfixedString_basicSet ;
extern String MfixedString_beep ;
extern String MfixedString_before ;
extern String MfixedString_blackOnWhite ;
extern String MfixedString_blue ;
extern String MfixedString_border ;
extern String MfixedString_bottom ;
extern String MfixedString_calculator ;
extern String MfixedString_clickSelect ;
extern String MfixedString_clock ;
extern String MfixedString_color ;
extern String MfixedString_colorChoices ;
extern String MfixedString_colorCombo ;
extern String MfixedString_colorSample ;
extern String MfixedString_constrain ;
extern String MfixedString_continue ;
extern String MfixedString_copy ;
extern String MfixedString_custom ;
extern String MfixedString_cut ;
extern String MfixedString_damping ;
extern String MfixedString_dateTime ;
extern String MfixedString_delete ;
extern String MfixedString_desktop ;
extern String MfixedString_dispDefault ;
extern String MfixedString_dispLang ;
extern String MfixedString_dispMenu ;
extern String MfixedString_dragRight ;
extern String MfixedString_duplicate ;
extern String MfixedString_edit ;
extern String MfixedString_exit ;
extern String MfixedString_factory ;
extern String MfixedString_file ;
extern String MfixedString_fontGroup ;
extern String MfixedString_function ;
extern String MfixedString_gray ;
extern String MfixedString_green ;
extern String MfixedString_help ;
extern String MfixedString_helpKeyClr ;
extern String MfixedString_helpModel ;
extern String MfixedString_icons ;
extern String MfixedString_individually ;
extern String MfixedString_inputArea ;
extern String MfixedString_inputFocus ;
extern String MfixedString_inputLang ;
extern String MfixedString_inputMethod ;
extern String MfixedString_inputWindow ;
extern String MfixedString_insert ;
extern String MfixedString_interface ;
extern String MfixedString_invocation ;
extern String MfixedString_kbdProps ;
extern String MfixedString_layering ;
extern String MfixedString_left ;
extern String MfixedString_lmr ;
extern String MfixedString_location ;
extern String MfixedString_login ;
extern String MfixedString_menu ;
extern String MfixedString_menuLabels ;
extern String MfixedString_menuMarkR ;
extern String MfixedString_misc ;
extern String MfixedString_mneSetting ;
extern String MfixedString_mnemonic ;
extern String MfixedString_modifier ;
extern String MfixedString_mouseAcc ;
extern String MfixedString_mouseBtn ;
extern String MfixedString_mouseEq ;
extern String MfixedString_mouseM ;
extern String MfixedString_mouseMod ;
extern String MfixedString_mouseS ;
extern String MfixedString_mouseSelect ;
extern String MfixedString_movePointer ;
extern String MfixedString_multiClick ;
extern String MfixedString_name ;
extern String MfixedString_never ;
extern String MfixedString_nextChoice ;
extern String MfixedString_no ;
extern String MfixedString_noShow ;
extern String MfixedString_none ;
extern String MfixedString_notices ;
extern String MfixedString_numFormat ;
extern String MfixedString_off ;
extern String MfixedString_olam ;
extern String MfixedString_olfm ;
extern String MfixedString_olps ;
extern String MfixedString_onHighlight ;
extern String MfixedString_onNoShow ;
extern String MfixedString_onShow ;
extern String MfixedString_onUnderline ;
extern String MfixedString_outgoing ;
extern String MfixedString_paste ;
extern String MfixedString_pixeditor ;
extern String MfixedString_pointer ;
extern String MfixedString_primary ;
extern String MfixedString_progMenu ;
extern String MfixedString_programs ;
extern String MfixedString_properties ;
extern String MfixedString_propsTitle;
extern String MfixedString_red ;
extern String MfixedString_refresh ;
extern String MfixedString_reset ;
extern String MfixedString_right ;
extern String MfixedString_scrollPan ;
extern String MfixedString_setLocale ;
extern String MfixedString_setMenuDef ;
extern String MfixedString_select ;
extern String MfixedString_settings ;
extern String MfixedString_show ;
extern String MfixedString_specSetting ;
extern String MfixedString_suppSetting ;
extern String MfixedString_term ;
extern String MfixedString_textBG ;
extern String MfixedString_textFG ;
extern String MfixedString_top ;
extern String MfixedString_utils ;
extern String MfixedString_video ;
extern String MfixedString_view ;
extern String MfixedString_whiteOnBlack ;
extern String MfixedString_windowBG ;
extern String MfixedString_workspace ;
extern String MfixedString_wsm ;
extern String MfixedString_yes ;

extern String MfooterMsg_changeGUI ;
extern String MfooterMsg_chooseColor ;
extern String MfooterMsg_colorStart ;
extern String MfooterMsg_colorError ;
extern String MfooterMsg_deletePrime ;
extern String MfooterMsg_initKeyHelp ;
extern String MfooterMsg_nowPress ;
extern String MfooterMsg_quoting ;
extern String MfooterMsg_sameColors ;
extern String MfooterMsg_useThisArea ;
extern String MfooterMsg_wantExit ;

extern String MhelpTag_exit ;
extern String MhelpTag_progMenu ;
extern String MhelpTag_programs ;
extern String MhelpTag_properties ;
extern String MhelpTag_pushpin ;
extern String MhelpTag_refresh ;
extern String MhelpTag_utils ;
extern String MhelpTag_wMenu ;
extern String MhelpTag_wProps ;
extern String MhelpTag_workspace ;

extern String Minternal_cacheExceed ;

extern String MinvalidResource_flatKeys ;
extern String MinvalidResource_noStepParent ;
extern String MinvalidResource_stepParentNotComposite ;

extern String MmaxLabel_damping ;
extern String MmaxLabel_dragRight ;
extern String MmaxLabel_menuMarkR ;
extern String MmaxLabel_mouseAcc ;
extern String MmaxLabel_multiClick ;
  
extern String MminLabel_damping ;
extern String MminLabel_dragRight ;
extern String MminLabel_menuMarkR ;
extern String MminLabel_mouseAcc ;
extern String MminLabel_multiClick ;

extern String Mmnemonic_abort ;
extern String Mmnemonic_accepted ;
extern String Mmnemonic_after ;
extern String Mmnemonic_apply ;
extern String Mmnemonic_applyEdits ;
extern String Mmnemonic_before ;
extern String Mmnemonic_color ;
extern String Mmnemonic_continue ;
extern String Mmnemonic_delete ;
extern String Mmnemonic_desktop ;
extern String Mmnemonic_exit ;
extern String Mmnemonic_factory ;
extern String Mmnemonic_icons ;
extern String Mmnemonic_insert ;
extern String Mmnemonic_kbdProps ;
extern String Mmnemonic_left ;
extern String Mmnemonic_middle ;
extern String Mmnemonic_misc ;
extern String Mmnemonic_mouseM ;
extern String Mmnemonic_mouseS ;
extern String Mmnemonic_no ;
extern String Mmnemonic_olam ;
extern String Mmnemonic_olfm ;
extern String Mmnemonic_olps ;
extern String Mmnemonic_outgoing ;
extern String Mmnemonic_progMenu ;
extern String Mmnemonic_programs ;
extern String Mmnemonic_properties ;
extern String Mmnemonic_refresh ;
extern String Mmnemonic_reset ;
extern String Mmnemonic_right ;
extern String Mmnemonic_setLocale ;
extern String Mmnemonic_utils ;
extern String Mmnemonic_yes ;

extern String MpageLabel_color ;
extern String MpageLabel_desktop ;
extern String MpageLabel_icons ;
extern String MpageLabel_misc ;
extern String MpageLabel_mouseM ;
extern String MpageLabel_progMenu ;
extern String MpageLabel_setLocale ;
extern String MpageLabel_settings ;

extern String MwarningMsg_badKeyMatch ;
extern String MwarningMsg_badModMatch ;
extern String MwarningMsg_cannotWrite ;
extern String MwarningMsg_forkFailed ;
extern String MwarningMsg_noDefaults ;
extern String MwarningMsg_noFile ;
extern String MwarningMsg_poorColors ;
#endif /* USE_TOOLKIT_I18N */

#endif /* __olwsm_error_h__ */  
