#pragma ident	"@(#)Dt:helpReq.c	1.25"

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include "DesktopI.h"

#define OFFSET(P)	XtOffset(DtDisplayHelpRequest *, P)
static DtStrToStructMapping const display_help[] = {
	REQUEST_HDR_MAPPING
	{ "HELPTYPE",	OFFSET(source_type),	DT_MTYPE_ULONG },
	{ "APPTITLE",	OFFSET(app_title),		DT_MTYPE_STRING },
	{ "APPNAME",	OFFSET(app_name),		DT_MTYPE_STRING },
	{ "TITLE",	OFFSET(title),			DT_MTYPE_STRING },
	{ "HELPDIR",	OFFSET(help_dir),		DT_MTYPE_STRING },
	{ "ICONFILE",	OFFSET(icon_file),		DT_MTYPE_STRING },
	{ "STRING",	OFFSET(string),		DT_MTYPE_STRING },
	{ "FILENAME",	OFFSET(file_name),		DT_MTYPE_STRING },
	{ "SECTNAME",	OFFSET(sect_tag),		DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtOLDisplayHelpRequest *, P)
static DtStrToStructMapping const ol_display_help[] = {
	REQUEST_HDR_MAPPING
	{ "HELPTYPE",	OFFSET(source_type),	DT_MTYPE_ULONG },
	{ "APPNAME",	OFFSET(app_name),		DT_MTYPE_STRING },
	{ "APPTITLE",	OFFSET(app_title),		DT_MTYPE_STRING },
	{ "TITLE",	OFFSET(title),			DT_MTYPE_STRING },
	{ "STRING",	OFFSET(string),		DT_MTYPE_STRING },
	{ "FILENAME",	OFFSET(file_name),		DT_MTYPE_STRING },
	{ "HELPDIR",	OFFSET(help_dir),		DT_MTYPE_STRING },
	{ "SECTTAG",	OFFSET(sect_tag),		DT_MTYPE_STRING },
	{ "XPOS",		OFFSET(x),			DT_MTYPE_SHORT  },
	{ "YPOS",		OFFSET(y),			DT_MTYPE_SHORT  },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtAddToHelpDeskRequest *, P)
static DtStrToStructMapping const add_to_helpdesk[] = {
	REQUEST_HDR_MAPPING
	{ "APPNAME",	OFFSET(app_name),		DT_MTYPE_STRING },
	{ "ICONLABEL",	OFFSET(icon_label),		DT_MTYPE_STRING },
	{ "ICONFILE",	OFFSET(icon_file),		DT_MTYPE_STRING },
	{ "FILENAME",	OFFSET(help_file),		DT_MTYPE_STRING },
	{ "HELPDIR",	OFFSET(help_dir),		DT_MTYPE_STRING },
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDelFromHelpDeskRequest *, P)
static DtStrToStructMapping const del_from_helpdesk[] = {
	REQUEST_HDR_MAPPING
	{ "APPNAME",	OFFSET(app_name),		DT_MTYPE_STRING },
	{ "FILENAME",	OFFSET(help_file),		DT_MTYPE_STRING },
};
#undef OFFSET

DtMsgInfo const Dt__help_msgtypes[] = {
	{ "DISPLAY_HELP", DT_DISPLAY_HELP,  display_help,
		XtNumber(display_help) },
	{ "ADD_TO_HELPDESK", DT_ADD_TO_HELPDESK, add_to_helpdesk,
		XtNumber(add_to_helpdesk) },
	{ "DEL_FROM_HELPDESK", DT_DEL_FROM_HELPDESK, del_from_helpdesk,
		XtNumber(del_from_helpdesk) },
	{ "OL_DISPLAY_HELP", DT_OL_DISPLAY_HELP,  ol_display_help,
		XtNumber(ol_display_help) },
};

/* reply structures */

#define OFFSET(P)	XtOffset(DtAddToHelpDeskReply *, P)
static DtStrToStructMapping const radd_to_helpdesk[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

#define OFFSET(P)	XtOffset(DtDelFromHelpDeskReply *, P)
static DtStrToStructMapping const rdel_from_helpdesk[] = {
	REPLY_HDR_MAPPING
};
#undef OFFSET

DtMsgInfo const Dt__help_replytypes[] = {
	{ "ADD_TO_HELPDESK", DT_ADD_TO_HELPDESK, radd_to_helpdesk,
		XtNumber(radd_to_helpdesk) },
	{ "DEL_FROM_HELPDESK", DT_DEL_FROM_HELPDESK, rdel_from_helpdesk,
		XtNumber(rdel_from_helpdesk) },
};
