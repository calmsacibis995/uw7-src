#pragma ident	"@(#)Dt:DtHMMsg.h	1.25"

#ifndef __DtHMMsg_h__
#define __DtHMMsg_h__

/*
 **************************************************************************
 *
 * Description:
 *              This file contains all the things related to HM request
 *       and reply messages.
 *
 **************************************************************************
 */

/* message function */
#define HM_FUNC		(HELP_MSG << MSGFUNC_SHIFT)

/* functional specific request types */
#define DT_DISPLAY_HELP		(HM_FUNC | 1)
#define DT_ADD_TO_HELPDESK	(HM_FUNC | 2)
#define DT_DEL_FROM_HELPDESK	(HM_FUNC | 3)
#define DT_OL_DISPLAY_HELP	(HM_FUNC | 4)

#define DT_HELP_NUM_REQUESTS	4
#define DT_HELP_NUM_REPLIES	2

/* For all help requests to the Help Manager, "app_name" should be
 * the name of an application's executable file.
 */

/* Used for DT_DISPLAY_HELP request.
 * The following must be specified for all source types:
 * - app_name
 * - source_type
 *
 * The following must be specified for all DT_SECTION_HELP and DT_TOC
 * source types:
 * - file_name
 *
 * The following must be specified for all DT_STRING_HELP source type:
 * - string
 * 
 * If app_title is not specified, it defaults to app_name.
 *
 * The rest is optional.
 */
typedef struct {
	REQUEST_HDR
	unsigned long	source_type;	/* source type */
	char		*app_title;		/* application title for window title */
	char		*app_name;		/* application name */
	char		*title;			/* title in help window */
	char		*help_dir;		/* help directory */
	char		*icon_file;		/* icon pixmap file */
	char		*string;			/* help source */
	char		*file_name;		/* help filename */
	char		*sect_tag;		/* section tag/name */
} DtDisplayHelpRequest;

/* Used for DT_OL_DISPLAY_HELP; i.e., context-sensitive help.
 * file_name must be specified for OL_DISK_SOURCE and OL_DESKTOP_SOURCE
 * source_types.
 * string must be specified for OL_STRING_SOURCE.
 */
typedef struct {
	REQUEST_HDR
	unsigned long	source_type;	/* source type */
	char		*app_name;		/* application name */
	char		*app_title;		/* application title for help win. title*/
	char		*title;			/* title in help window */
	char		*string;			/* help source */
	char		*file_name;		/* help filename */
	char		*help_dir;		/* help directory */
	char		*sect_tag;		/* section tag */
	int		x;				/* x relative to root */
	int		y;				/* y relative to root */
} DtOLDisplayHelpRequest;

/* Used in a DT_ADD_TO_HELPDESK request.
 * app_name and help file are required; the rest is optional.
 * If icon_label is not specified, it defaults to app_name.
 * icon_label is also used as app_title.
 */
typedef struct {
	REQUEST_HDR
	char		*app_name;		/* name of executable */
	char		*icon_label;		/* icon label */
	char		*icon_file;		/* icon pixmap file */
	char		*help_file;		/* help file name */
	char		*help_dir;		/* help directory */
} DtAddToHelpDeskRequest;

/* Used in a DT_DEL_FROM_HELPDESK request.
 * Only app_name is required. However, if more than one icon
 * is in the Help Desk for the same application, icon_label
 * should be specified to indicate which icon to remove.
 */
typedef struct {
	REQUEST_HDR
	char		*app_name;		/* application name */
	char		*help_file;		/* help file */
} DtDelFromHelpDeskRequest;

typedef struct {
	REPLY_HDR
} DtAddToHelpDeskReply;

typedef struct {
	REPLY_HDR
} DtDelFromHelpDeskReply;

extern DtMsgInfo const Dt__help_msgtypes[DT_HELP_NUM_REQUESTS];
extern DtMsgInfo const Dt__help_replytypes[DT_HELP_NUM_REPLIES];

#endif /* __DtHMMsg_h__ */
