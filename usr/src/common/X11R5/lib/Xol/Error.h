#ifndef	NOIDENT
#ident	"@(#)olmisc:Error.h	1.84"
#endif

#ifndef __Ol_Error_h__
#define __Ol_Error_h__

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
 *		1. Error classes begin with OlC, e.g.,
 *			#define OleCOlToolkitWarning	"OlToolkitWarning"
 *
 *		2. Error names begin with OleN, e.g.,
 *			#define	OleNinvalidResource	"invalidResource"
 *
 *		3. Error types begin with OleT, e.g.,
 *			#define	OleTsetValues		"setValues"
 *
 *		4. Error message strings begin with OleM and is followed
 *		   by the name string, and underbar '_', and concatenated
 *		   with the error type.  For the above error name and type.
 *
 *			#define OleMinvalidResource_setValues \
 *			   "SetValues: widget \"%s\" (class \"%s\"): invalid\
 *			    resource \"%s\", setting to %s"
 *
 *	Using these conventions, an example use of OlVaDisplayWarningMsg() 
 *	for a bad resource in FooWidget's SetValues procedure would be:
 *
 *	OlVaDisplayWarningMsg(display, OleNinvalidResource, OleTsetValues,
 *		OleCOlToolkitWarning, OleMinvalidResource_setValues,
 *		XtName(w), XtClass(w)->core_class.class_name,
 *		XtNwidth, (OLconst char *)"23");
 *
 *******************************file*header*******************************
 */

/*
 *************************************************************************
 * Define the constant String type
 *************************************************************************
 */
  
#define CString OLconst char * OLconst

/*
 *************************************************************************
 * Define the error classes here:  Use prefix of 'OleC'
 *************************************************************************
 */

#define OleCOlToolkitError		(OLconst char *)"xol_errs"
#define OleCOlToolkitWarning		(OLconst char *)"xol_errs"
#define OleCOlToolkitMessage		(OLconst char *)"xol_msgs"

/*
 *************************************************************************
 * Define the error names here:  Use prefix of 'OleN'
 *************************************************************************
 */

#define OleNmessage			(OLconst char *)"message"

#ifdef sun

#define OleNbadVisual			(OLconst char *)"badVisual"

#endif /* sun */

#define OleNbadConversion		(OLconst char *)"badConversion"
#define OleNbadDestruction		(OLconst char *)"badDestruction"
#define OleNbadFlatSubclass		(OLconst char *)"badFlatSubclass"
#define OleNbadFile			(OLconst char *)"badFile"
#define OleNbadFont			(OLconst char *)"badFont"
#define OleNbadFormat			(OLconst char *)"badFormat"
#define OleNbadFunction			(OLconst char *)"badFunction"
#define OleNbadGui			(OLconst char *)"badGui"
#define OleNbadItemAddress		(OLconst char *)"badItemAddress"
#define OleNbadItemIndex		(OLconst char *)"badItemIndex"
#define OleNbadItemResource		(OLconst char *)"badItemResource"
#define OleNbadMax			(OLconst char *)"badMax"
#define OleNbadNumItems			(OLconst char *)"badNumItems"
#define OleNbadParent			(OLconst char *)"badParent"
#define OleNbadReference		(OLconst char *)"badReference"
#define OleNbadScreen			(OLconst char *)"badScreen"
#define OleNbadWidth			(OLconst char *)"badWidth"
#define OleNcantUseMax			(OLconst char *)"cantUseMax"
#define OleNcantWrap			(OLconst char *)"cantWarp"
#define OleNgoodParent			(OLconst char *)"goodParent"
#define OleNfileAbbrevFMen		(OLconst char *)"fileAbbrevFMen"
#define OleNfileAction			(OLconst char *)"fileAction"
#define OleNfileApplic			(OLconst char *)"fileApplic"
#define OleNfileBuffutil		(OLconst char *)"fileBuffutil"
#define OleNfileButton			(OLconst char *)"fileButton"
#define OleNfileCategory		(OLconst char *)"fileCategory"
#define OleNfileColorChip		(OLconst char *)"fileColorChip"
#define OleNfileControlArea		(OLconst char *)"fileControlArea"
#define OleNfileConverters		(OLconst char *)"fileConverters"
#define OleNfileDisplay			(OLconst char *)"fileDisplay"
#define OleNfileDynResProc		(OLconst char *)"fileDynResProc"
#define OleNfileDynamic			(OLconst char *)"fileDynamic"
#define OleNfileError			(OLconst char *)"fileError"
#define OleNfileEventObj		(OLconst char *)"fileEventObj"
#define OleNfileExclusives		(OLconst char *)"fileExclusives"
#define OleNfileFButton			(OLconst char *)"fileFButton"
#define OleNfileFExclusive		(OLconst char *)"fileFExclusive"
#define OleNfileFlat			(OLconst char *)"fileFlat"
#define OleNfileFlatCvt			(OLconst char *)"fileFlatCvt"
#define OleNfileFlatExpand		(OLconst char *)"fileFlatExpand"
#define OleNfileForm			(OLconst char *)"fileForm"
#define OleNfileHelp			(OLconst char *)"fileHelp"
#define OleNfileListPane		(OLconst char *)"fileListPane"
#define OleNfileMenu			(OLconst char *)"fileMenu"
#define OleNfileMenuShell		(OLconst char *)"fileMenuShell"
#define OleNfileMenuButton		(OLconst char *)"fileMenuButton"
#define OleNfileOblongButton		(OLconst char *)"fileOblongButton"
#define OleNfileOlCursors		(OLconst char *)"fileOlCursors"
#define OleNfileOlDraw			(OLconst char *)"fileOlDraw"
#define OleNfileOlGetFont		(OLconst char *)"fileOlGetFont"
#define OleNfileOlGetRes		(OLconst char *)"fileOlGetRes"
#define OleNfileOlgAttr			(OLconst char *)"fileOlgAttr"
#define OleNfileOlgInit			(OLconst char *)"fileOlgInit"
#define OleNfileOlgLines		(OLconst char *)"fileOlgLines"
#define OleNfilePacked			(OLconst char *)"filePacked"
#define OleNfilePopupWindo		(OLconst char *)"filePopupWindo"
#define OleNfilePushpin			(OLconst char *)"filePushpin"
#define OleNfileRubberTile		(OLconst char *)"fileRubberTile"
#define OleNfileScrollbar		(OLconst char *)"fileScrollbar"
#define OleNfileSlider			(OLconst char *)"fileSlider"
#define OleNfileSourceDsk		(OLconst char *)"fileSourceDsk"
#define OleNfileSourceStr		(OLconst char *)"fileSourceStr"
#define OleNfileStaticText		(OLconst char *)"fileStaticText"
#define OleNfileStub			(OLconst char *)"fileStub"
#define OleNfileText		        (OLconst char *)"fileText"
#define OleNfileTextEPos		(OLconst char *)"fileTextEPos"
#define OleNfileTextEdit		(OLconst char *)"fileTextEdit"
#define OleNfileTextPane		(OLconst char *)"fileTextPane"
#define OleNfileTextbuff		(OLconst char *)"fileTextbuff"
#define OleNfileTraversal		(OLconst char *)"fileTraversal"
#define OleNfileVendor			(OLconst char *)"fileVendor"
#define OleNfileVirtual			(OLconst char *)"fileVirtual"
#define OleNOlInitialize		(OLconst char *)"OlInitialize"
#define OleNOlResolveGUISymbol		(OLconst char *)"_OlResolveGUISymbol"
#define OleNflatIllegalMnemonic		(OLconst char *)"flatIllegalMnemonic"
#define OleNflatIllegalAccelerator	(OLconst char *)"flatIllegalAccelerator"
#define OleNillegalAccOrMne		(OLconst char *)"illegalAccOrMne"
#define OleNinconsistentNumItemFields	(OLconst char *)"inconsistentNumItemFields"
#define OleNinternal			(OLconst char *)"internal"
#define OleNinvalidArgCount		(OLconst char *)"invalidArgCount"
#define OleNinvalidData			(OLconst char *)"invalidData"
#define OleNinvalidDimension		(OLconst char *)"invalidDimension"
#define OleNinvalidItemRecord		(OLconst char *)"invalidItemRecord"
#define	OleNinvalidRequiredResource	(OLconst char *)"invalidRequiredResource"
#define	OleNinvalidResource		(OLconst char *)"invalidResource"
#define OleNinvalidParameters		(OLconst char *)"invalidParameters"
#define OleNinvalidProcedure		(OLconst char *)"invalidProcedure"
#define OleNitemsAndNumItemFieldsConflict (OLconst char *)"itemsAndNumItemFieldsConflict"
#define OleNolComposeArgList		(OLconst char *)"olComposeArgList"
#define OleNnoConvChar			(OLconst char *)"noConvChar"
#define OleNnoDelimiter			(OLconst char *)"noDelimiter"
#define OleNnoInputContext		(OLconst char *)"noInputContext"
#define OleNnoMemory			(OLconst char *)"noMemory"
#define OleNnotRealized			(OLconst char *)"notRealized"
#define OleNnullFormat			(OLconst char *)"nullFormat"
#define	OleNnullItemField		(OLconst char *)"nullItemField"
#define	OleNnullWidget			(OLconst char *)"nullWidget"
#define OleNsetLocale			(OLconst char *)"setLocale"
#define OleNtooManyDefaults		(OLconst char *)"tooManyDefaults"
#define OleNtooManyRecords		(OLconst char *)"tooManyRecords"
#define OleNtooManySet			(OLconst char *)"tooManySet"

/*
 *************************************************************************
 * Define the error types here:  Use prefix of 'OleT'
 *************************************************************************
 */

#define OleTprefix			(OLconst char *)"prefix"

#define OleTbadBitmap			(OLconst char *)"badBitmap"
#define OleTbadChild			(OLconst char *)"badChild"
#define OleTbadDefaultType		(OLconst char *)"badDefaultType"
#define OleTbadDisplay			(OLconst char *)"badDisplay"
#define OleTfileOpen			(OLconst char *)"fileOpen"
#define OleTbadGeometry			(OLconst char *)"badGeometry"
#define OleTbadHeight			(OLconst char *)"badHeight"
#define OleTbadInitialize		(OLconst char *)"badInitialize"
#define OleTbadItemResource		(OLconst char *)"badItemResource"
#define OleTbadKey			(OLconst char *)"badKey"
#define OleTbadMatch			(OLconst char *)"badMatch"
#define OleTbadMenuChildHeight		(OLconst char *)"badMenuChildHeight"
#define OleTbadMenuChildWidth		(OLconst char *)"badMenuChildWidth"
#define OleTbadNodeReference		(OLconst char *)"badNodeReference"
#define OleTbadQueryFont		(OLconst char *)"badQueryFont"
#define OleTbadVersion			(OLconst char *)"badVersion"
#define OleTbadValue			(OLconst char *)"badValue"
#define OleTbadWidth			(OLconst char *)"badWidth"
#define OleTdefaultOLFont		(OLconst char *)"defaultOLFont"
#define OleTdiffChild			(OLconst char *)"diffChild"
#define OleTdlopen	  		(OLconst char *)"dlopen"
#define OleTdlsym	  		(OLconst char *)"dlsym"
#define OleTduplicateDefault		(OLconst char *)"duplicateDefault"
#define OleTduplicateKey		(OLconst char *)"duplicateKey"
#define OleTcalloc			(OLconst char *)"calloc"
#define OleTcategory			(OLconst char *)"category"
#define OleTclassPointer		(OLconst char *)"classPointer"
#define OleTcommandLine			(OLconst char *)"commandLine"
#define OleTConvert		        (OLconst char *)"Convert"
#define OleTcopy			(OLconst char *)"copy"
#define OleTcopyMne			(OLconst char *)"copyMne"
#define OleTcorruptedList		(OLconst char *)"corruptedList"
#define OleTcut				(OLconst char *)"cut"
#define OleTcutMne			(OLconst char *)"cutMne"
#define	OleTdataType			(OLconst char *)"dataType"
#define OleTdelete			(OLconst char *)"delete"
#define OleTdeleteMne			(OLconst char *)"deleteMne"
#define	OleTolDoGravity			(OLconst char *)"olDoGravity"
#define OleTedit			(OLconst char *)"edit"
#define OleTflatState			(OLconst char *)"flatState"
#define OleTfree			(OLconst char *)"free"
#define OleTgeneral			(OLconst char *)"general"
#define OleTguiVar			(OLconst char *)"guiVar"
#define OleTillegalString		(OLconst char *)"illegalString"
#define OleTillegalSyntax		(OLconst char *)"illegalSyntax"
#define OleTinputMethod			(OLconst char *)"inputMethod"
#define	OleTinitialize			(OLconst char *)"initialize"
#define	OleTinitializeDefault		(OLconst char *)"initializeDefault"
#define	OleTinitializeNC		(OLconst char *)"initializeNC"
#define OleTinheritanceProc		(OLconst char *)"inheritanceProc"
#define OleTinsertChild			(OLconst char *)"insertChild"
#define OleTinvalidFont			(OLconst char *)"invalidFont"
#define OleTinvalidFormat		(OLconst char *)"invalidFormat"
#define OleTinvalidParameters		(OLconst char *)"invalidParameters"
#define OleTmsg1			(OLconst char *)"msg1"
#define OleTmsg2			(OLconst char *)"msg2"
#define OleTmsg3			(OLconst char *)"msg3"
#define OleTmsg4			(OLconst char *)"msg4"
#define OleTmsg5			(OLconst char *)"msg5"
#define OleTmsg6			(OLconst char *)"msg6"
#define OleTmsg7			(OLconst char *)"msg7"
#define OleTmsg8			(OLconst char *)"msg8"
#define OleTmsg9			(OLconst char *)"msg9"
#define OleTmsg10			(OLconst char *)"msg10"
#define OleTmsg11			(OLconst char *)"msg11"
#define OleTmsg12			(OLconst char *)"msg12"
#define OleTmsg13			(OLconst char *)"msg13"
#define OleTmsg14			(OLconst char *)"msg14"
#define OleTmsg15			(OLconst char *)"msg15"
#define OleTmsg16			(OLconst char *)"msg16"
#define OleTmsg17			(OLconst char *)"msg17"
#define OleTmsg18			(OLconst char *)"msg18"
#define OleTmsg19			(OLconst char *)"msg19"
#define OleTmsg20			(OLconst char *)"msg20"
#define OleTmsg21			(OLconst char *)"msg21"
#define OleTmsg22			(OLconst char *)"msg22"
#define OleTmsg23			(OLconst char *)"msg23"
#define OleTmsg24			(OLconst char *)"msg24"
#define OleTnoChild			(OLconst char *)"noChild"
#define OleTnoColormap			(OLconst char *)"noColormap"
#define OleTnoFont			(OLconst char *)"noFont"
#define OleTnoMaskList			(OLconst char *)"noMaskList"
#define	OleTnoMenuParent		(OLconst char *)"noMenuParent"
#define OleTnoSourceList		(OLconst char *)"noSourceList"
#define OleTnullDestination		(OLconst char *)"nullDestination"
#define OleTnullListPtr			(OLconst char *)"nullListPtr"
#define OleTnullParent			(OLconst char *)"nullParent"
#define OleToneChild			(OLconst char *)"oneChild"
#define OleToverflow			(OLconst char *)"overflow"
#define OleTpointerGrab			(OLconst char *)"pointerGrab"
#define OleTnullList			(OLconst char *)"nullList"
#define OleTnullScreen			(OLconst char *)"nullScreen"
#define OleTnullString			(OLconst char *)"nullString"
#define OleTmalloc			(OLconst char *)"malloc"
#define OleTmalloc2			(OLconst char *)"malloc2"
#define OleTmissingParameter		(OLconst char *)"missingParameter"
#define OleTmissing2Parms		(OLconst char *)"missing2Parms"
#define OleTpaste			(OLconst char *)"paste"
#define OleTpasteMne			(OLconst char *)"pasteMne"
#define OleTreadFile			(OLconst char *)"readFile"
#define OleTrealloc			(OLconst char *)"realloc"
#define OleTxtRealloc			(OLconst char *)"xtRealloc"
#define OleTsetGui			(OLconst char *)"setGui"
#define OleTsetToDefault		(OLconst char *)"setToDefault"
#define OleTsetToSomething		(OLconst char *)"setToSomething"
#define	OleTsetValues			(OLconst char *)"setValues"
#define	OleTsetValuesNC			(OLconst char *)"setValuesNC"
#define	OleTsetValuesRO			(OLconst char *)"setValuesRO"
#define	OleTsetValuesAlmost		(OLconst char *)"setValuesAlmost"
#define OleTtextEdit			(OLconst char *)"textEdit"
#define OleTtooManyParams		(OLconst char *)"tooManyParams"
#define OleTundo			(OLconst char *)"undo"
#define OleTundoMne			(OLconst char *)"undoMne"
#define OleTunknownRule			(OLconst char *)"unknownRule"
#define OleTunknownWidget		(OLconst char *)"unknownWidget"
#define OleTviewItem			(OLconst char *)"viewItem"
#define OleTwidgetSize			(OLconst char *)"widgetSize"
#define OleTwrongParameters		(OLconst char *)"wrongParameters"

/*
 *************************************************************************
 * Define the default error messages here:  Use prefix of 'OleM'
 * followed by the error name, an underbar <_>, and the error type.
 *************************************************************************
 */

extern char error_buffer[] ;		/* see Error.c */

#ifndef IN_ERROR_C
#ifdef sun

extern CString OleMbadVisual_noVisual ;

extern CString OleMbadVisual_copyFromParent ;

extern CString OleMbadConversion_invalidVisual ;

#endif /* sun */

extern CString OleMbadConversion_badQueryFont ;

extern CString OleMbadConversion_illegalString ;

extern CString OleMbadConversion_illegalSyntax ;

extern CString OleMbadConversion_invalidFont;

extern CString OleMbadConversion_invalidFormat ;

extern CString OleMbadConversion_missingParameter ;

extern CString OleMbadConversion_missing2Parms ;

extern CString OleMbadConversion_noColormap ;

extern CString OleMbadConversion_noFont ;

extern CString OleMbadConversion_nullString ;

extern CString OleMbadConversion_tooManyParams ;

extern CString OleMbadDestruction_missingParameter ;

extern CString OleMbadDestruction_missing2Parms ;

extern CString OleMbadFlatSubclass_flatState ;

extern CString OleMbadFile_badBitmap ;

extern CString OleMbadFile_fileOpen ;

extern CString OleMbadFont_defaultOLFont ;

extern CString OleMbadFormat_invalidFormat ;

extern CString OleMbadFunction_insertChild ;

extern CString OleMbadGui_commandLine ;

extern CString OleMbadGui_guiVar ;

extern CString OleMbadGui_setGui ;

extern CString OleMbadItemAddress_flatState ;

extern CString OleMbadItemIndex_flatState ;

extern CString OleMbadItemResource_Convert ;

extern CString OleMbadMax_invalidFormat ;

extern CString OleMbadParent_noMenuParent ;

extern CString OleMbadParent_nullParent ;

extern CString OleMbadReference_badMatch ;

extern CString OleMbadReference_unknownWidget ;

extern CString OleMbadScreen_nullScreen ;

extern CString OleMbadWidth_invalidFormat ;

extern CString OleMcantUseMax_invalidFormat ;

extern CString OleMcantWrap_invalidFormat ;

extern CString OleMolComposeArgList_noSourceList ;

extern CString OleMolComposeArgList_noMaskList ;

extern CString OleMolComposeArgList_nullListPtr ;

extern CString OleMolComposeArgList_invalidParameters ;

extern CString OleMolComposeArgList_wrongParameters ;

extern CString OleMolComposeArgList_nullDestination ;

extern CString OleMolComposeArgList_unknownRule ;

extern CString OleMgoodParent_badChild ;

extern CString OleMgoodParent_diffChild ;

extern CString OleMgoodParent_noChild;	

extern CString OleMgoodParent_oneChild ;

extern CString OleMinconsistentNumItemFields_Convert ;

extern CString OleMinternal_badNodeReference ;

extern CString OleMinternal_badVersion ;

extern CString OleMinternal_corruptedList ;

extern CString OleMinvalidArgCount_flatState ;

extern CString OleMinvalidData_dataType ;

extern CString OleMinvalidParameters_olDoGravity ;

extern CString OleMinvalidProcedure_inheritanceProc ;

extern CString OleMinvalidProcedure_setValuesAlmost ;

extern CString OleMinvalidItemRecord_flatState ;

extern CString OleMinvalidRequiredResource_badDefaultType ;

extern CString OleMinvalidRequiredResource_badValue ;

extern CString OleMinvalidRequiredResource_nullList ;

extern CString OleMinvalidResource_duplicateDefault ;

extern CString OleMinvalidResource_flatState ;

extern CString OleMinvalidResource_initialize ;

extern CString OleMinvalidResource_initializeDefault ;

extern CString OleMinvalidResource_setToDefault ;
  
extern CString OleMinvalidResource_setToSomething ;

extern CString OleMinvalidResource_badItemResource ;

extern CString OleMinvalidResource_nullList ;

extern CString OleMinvalidResource_setValues ;

extern CString OleMinvalidResource_setValuesNC ;

extern CString OleMinvalidResource_setValuesRO ;

extern CString OleMinvalidResource_textEdit ;

extern CString OleMitemsAndNumItemFieldsConflict_Convert ;


extern CString OleMinvalidDimension_badHeight ;

extern CString OleMinvalidDimension_badMenuChildHeight ;

extern CString OleMinvalidDimension_badMenuChildWidth ;

extern CString OleMinvalidDimension_badWidth ;

extern CString OleMinvalidDimension_widgetSize ;

extern CString OleMinvalidDimension_badGeometry ;

extern CString OleMnoConvChar_invalidFormat ;

extern CString OleMnoDelimiter_invalidFormat ;

extern CString OleMnoInputContext_textEdit ;

extern CString OleMnoMemory_calloc ;

extern CString OleMnoMemory_free ;

extern CString OleMnoMemory_general ;

extern CString OleMnoMemory_malloc ;

extern CString OleMnoMemory_malloc2 ;

extern CString OleMnoMemory_readFile ;

extern CString OleMnoMemory_realloc ;

extern CString OleMnoMemory_xtRealloc ;

extern CString OleMnotFlatList_viewItem ;

extern CString OleMnotRealized_pointerGrab ;

extern CString OleMnullFormat_invalidFormat ;

extern CString OleMnullItemField_Convert ;

extern CString OleMnullWidget_classPointer;

extern CString OleMnullWidget_flatState ;

extern CString OleMOlInitialize_badDisplay ;

extern CString OleMOlInitialize_badInitialize ;

extern CString OleMOlInitialize_badLinkOrder ;

extern CString OleMOlInitialize_inputMethod ;

extern CString OleMOlResolveGUISymbol_dlopen;

extern CString OleMOlResolveGUISymbol_dlsym;

extern CString OleMsetLocale_category ;

extern CString OleMtooManyDefaults_flatState ;

extern CString OleMtooManyRecords_Convert ;

extern CString OleMtooManySet_flatState ;

/*
 * Messages that defied classification.
 */

extern CString OleMfileAbbrevFMen_msg1 ;

extern CString OleMfileAction_msg1 ;
extern CString OleMfileAction_msg2 ;
extern CString OleMfileAction_msg3 ;

extern CString OleMfileApplic_msg1 ;
extern CString OleMfileBuffutil_msg1;

extern CString OleMfileButton_msg1 ;
extern CString OleMfileButton_msg2 ;

extern CString OleMfileCategory_msg1 ;
extern CString OleMfileCategory_msg2 ;

extern CString OleMfileColorChip_msg1 ;

extern CString OleMfileControlArea_msg1 ;

extern CString OleMfileConverters_msg1 ;
extern CString OleMfileConverters_msg2 ;
extern CString OleMfileConverters_msg3 ;
extern CString OleMfileConverters_msg4 ;
extern CString OleMfileConverters_msg5 ;
extern CString OleMfileConverters_msg6 ;
extern CString OleMfileConverters_msg7 ;
extern CString OleMfileConverters_msg8 ;
extern CString OleMfileConverters_msg9 ;
extern CString OleMfileConverters_msg10 ;
extern CString OleMfileConverters_msg11 ;
extern CString OleMfileConverters_msg12 ;
extern CString OleMfileConverters_msg13 ;
extern CString OleMfileConverters_msg14 ;

extern CString OleMfileDisplay_msg1 ;

extern CString OleMfileDynResProc_msg1 ;

extern CString OleMfileDynamic_msg1 ;
extern CString OleMfileDynamic_msg2 ;
extern CString OleMfileDynamic_msg3 ;
extern CString OleMfileDynamic_msg4 ;

extern CString OleMfileError_msg1 ;
extern CString OleMfileError_msg2 ;
extern CString OleMfileError_msg3 ;
extern CString OleMfileError_msg4 ;
extern CString OleMfileError_msg5 ;

extern CString OleMfileEventObj_msg1 ;
extern CString OleMfileEventObj_msg2 ;

extern CString OleMfileFButton_msg1 ;

extern CString OleMfileFlat_msg1 ;
extern CString OleMfileFlat_msg2 ;
extern CString OleMfileFlat_msg3 ;
extern CString OleMfileFlat_msg4 ;
extern CString OleMfileFlat_msg5 ;

extern CString OleMfileFlatCvt_msg1 ;
extern CString OleMfileFlatCvt_msg2 ;
extern CString OleMfileFlatCvt_msg3 ;
extern CString OleMfileFlatCvt_msg4 ;
extern CString OleMfileFlatCvt_msg5 ;
extern CString OleMfileFlatCvt_msg6 ;

extern CString OleMfileFlatExpand_msg1 ;

extern CString OleMfileFList_msg1;
extern CString OleMfileFList_msg2;

extern CString OleMfileForm_msg1 ;
extern CString OleMfileForm_msg2 ;
extern CString OleMfileForm_msg3 ;

extern CString OleMfileHelp_msg1 ;
extern CString OleMfileHelp_msg2 ;
extern CString OleMfileHelp_msg3 ;
extern CString OleMfileHelp_msg4 ;
extern CString OleMfileHelp_msg5 ;
extern CString OleMfileHelp_msg6 ;
extern CString OleMfileHelp_msg8 ;
extern CString OleMfileHelp_msg9 ;
extern CString OleMfileHelp_msg10 ;

extern CString OleMfileExclusives_msg1 ;
extern CString OleMfileExclusives_msg2 ;
extern CString OleMfileExclusives_msg3 ;
extern CString OleMfileExclusives_msg4 ;
extern CString OleMfileExclusives_msg5 ;

extern CString OleMfileButton_msg1 ;

extern CString OleMfileFExclusive_msg1 ;

extern CString OleMfileListPane_msg1 ;
extern CString OleMfileListPane_msg2 ;
extern CString OleMfileListPane_msg3 ;
extern CString OleMfileListPane_msg4 ;
extern CString OleMfileListPane_msg5 ;
extern CString OleMfileListPane_msg6 ;
extern CString OleMfileListPane_msg7 ;
extern CString OleMfileListPane_msg8 ;
extern CString OleMfileListPane_msg9 ;
extern CString OleMfileListPane_msg10 ;
extern CString OleMfileListPane_msg11 ;
extern CString OleMfileListPane_msg12 ;
extern CString OleMfileListPane_msg13 ;
extern CString OleMfileListPane_msg14 ;

extern CString OleMfileMenu_msg1 ;
extern CString OleMfileMenu_msg2 ;
extern CString OleMfileMenu_msg3 ;
extern CString OleMfileMenu_msg4 ;
extern CString OleMfileMenu_msg5 ;
extern CString OleMfileMenu_msg6 ;
extern CString OleMfileMenu_msg7 ;
extern CString OleMfileMenu_msg8 ;
extern CString OleMfileMenu_msg9 ;
extern CString OleMfileMenu_msg10 ;
extern CString OleMfileMenu_msg11 ;

extern CString OleMfileMenuButton_msg1 ;
extern CString OleMfileMenuButton_msg2 ;
extern CString OleMfileMenuButton_msg3 ;

extern CString OleMfileOblongButton_msg1 ;

extern CString OleMfileOlCursors_msg1 ;

extern CString OleMfileOlDraw_msg1 ;
extern CString OleMfileOlDraw_msg2 ;
extern CString OleMfileOlDraw_msg3 ;

extern CString OleMfileOlGetFont_msg1 ;

extern CString OleMfileOlGetRes_msg1 ;
extern CString OleMfileOlGetRes_msg2 ;
extern CString OleMfileOlGetRes_msg3 ;
extern CString OleMfileOlGetRes_msg4 ;
extern CString OleMfileOlGetRes_msg5 ;
extern CString OleMfileOlGetRes_msg6 ;
extern CString OleMfileOlGetRes_msg7 ;
extern CString OleMfileOlGetRes_msg8 ;
extern CString OleMfileOlGetRes_msg9 ;

extern CString OleMfileOlgAttr_msg1 ;
extern CString OleMfileOlgAttr_msg2 ;
extern CString OleMfileOlgAttr_msg3 ;
extern CString OleMfileOlgAttr_msg4 ;

extern CString OleMfileOlgInit_msg1 ;
extern CString OleMfileOlgInit_msg2 ;
extern CString OleMfileOlgInit_msg3 ;
extern CString OleMfileOlgInit_msg4 ;
extern CString OleMfileOlgInit_msg5 ;
extern CString OleMfileOlgInit_msg6 ;
extern CString OleMfileOlgInit_msg7 ;

extern CString OleMfileOlgLines_msg1 ;

extern CString OleMfilePacked_msg1 ;
extern CString OleMfilePacked_msg2 ;

extern CString OleMfilePopupWindo_msg1 ;

extern CString OleMfilePushpin_msg1 ;
extern CString OleMfilePushpin_msg2 ;

extern CString OleMfileRubberTile_msg1 ;

extern CString OleMfileScrollbar_msg1 ;
extern CString OleMfileScrollbar_msg2 ;

extern CString OleMfileSlider_msg1 ;
extern CString OleMfileSlider_msg2 ;

extern CString OleMfileSourceDsk_msg1 ;
extern CString OleMfileSourceDsk_msg2 ;

extern CString OleMfileSourceStr_msg1 ;
extern CString OleMfileSourceStr_msg2 ;

extern CString OleMfileStaticText_msg1 ;
extern CString OleMfileStaticText_msg2 ;
extern CString OleMfileStaticText_msg3 ;
extern CString OleMfileStaticText_msg4 ;
extern CString OleMfileStaticText_msg5 ;

extern CString OleMfileStub_msg1 ;
extern CString OleMfileStub_msg2 ;

extern CString OleMfileText_msg1 ;
extern CString OleMfileText_msg2 ;

extern CString OleMfileTextEPos_msg1 ;
extern CString OleMfileTextEPos_msg2 ;

extern CString OleMfileTextEdit_msg1 ;
extern CString OleMfileTextEdit_msg2 ;
extern CString OleMfileTextEdit_msg3 ;

extern CString OleMfileTextPane_msg1 ;
extern CString OleMfileTextPane_msg2 ;
extern CString OleMfileTextPane_msg3 ;
extern CString OleMfileTextPane_msg4 ;
extern CString OleMfileTextPane_msg5 ;
extern CString OleMfileTextPane_msg6 ;
extern CString OleMfileTextPane_msg7 ;

extern CString OleMfileTraversal_msg1 ;
extern CString OleMfileTraversal_msg2 ;
extern CString OleMfileTraversal_msg3 ;
extern CString OleMfileTraversal_msg4 ;

extern CString OleMfileVendor_msg1 ;
extern CString OleMfileVendor_msg2 ;
extern CString OleMfileVendor_msg3 ;
extern CString OleMfileVendor_msg4 ;
extern CString OleMfileVendor_msg5 ;
extern CString OleMfileVendor_msg6 ;

extern CString OleMfileVirtual_msg1;
extern CString OleMfileVirtual_msg2;
extern CString OleMfileVirtual_msg3;
extern CString OleMfileVirtual_msg4;
extern CString OleMfileVirtual_msg5;
extern CString OleMfileVirtual_msg6;
extern CString OleMfileVirtual_msg7;
extern CString OleMfileVirtual_msg8;
extern CString OleMfileVirtual_msg9;
extern CString OleMfileVirtual_msg10;
extern CString OleMfileVirtual_msg11;
extern CString OleMfileVirtual_msg12;
extern CString OleMfileVirtual_msg13;
extern CString OleMfileVirtual_msg14;
extern CString OleMfileVirtual_msg15;
extern CString OleMfileVirtual_msg16;
extern CString OleMfileVirtual_msg17;
extern CString OleMfileVirtual_msg18;
extern CString OleMfileVirtual_msg19;
extern CString OleMfileVirtual_msg20;
extern CString OleMfileVirtual_msg21;
extern CString OleMfileVirtual_msg22;
extern CString OleMfileVirtual_msg23;
extern CString OleMfileVirtual_msg24;

extern CString OleMfileTextbuff_msg1 ;

extern CString OleMmessage_prefix ;

extern CString OleMflatIllegalMnemonic_duplicateKey ;
extern CString OleMflatIllegalMnemonic_badKey ;
extern CString OleMflatIllegalAccelerator_duplicateKey ;
extern CString OleMflatIllegalAccelerator_badKey ;

extern CString OleMillegalAccOrMne_duplicateKey ;

#endif

#undef CString

#endif /* __Ol_Error_h__ */
