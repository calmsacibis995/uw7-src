
#ifndef __DtMsg_h__
#define __DtMsg_h__

#pragma ident	"@(#)Dt:DtMsg.h	1.34"

#include <X11/Xlib.h>

/*
 **************************************************************************
 *
 * Description:
 *              This file contains all the generic things related to request
 *       and reply messages.
 *
 **************************************************************************
 */

/*
 * Request and reply type:
 * 	There are two pieces of information encoded in the value:
 *
 *	X X X X   X X X X X X X X X X X X
 *      -------   -----------------------
 *      function        request type id
 *
 *	function: DTM, WB, HELP.
 *	request type id: a unique value for requests within the same function.
 */
typedef unsigned short DtRequestType;
typedef unsigned short DtReplyType;

#define MSGFUNC_SHIFT	12
#define MSGID_MASK	((1 << (MSGFUNC_SHIFT)) - 1)

#define REQUEST_HDR	\
     DtRequestType  rqtype;    /* request type                   */\
     long           serial;    /* serial number                  */\
     unsigned short version;   /* version number                 */\
     Window         client;    /* sender's window id             */\
     char           *nodename; /* nodename of originating system */

#define REPLY_HDR	\
     DtReplyType    rptype;    /* reply type                     */\
     long           serial;    /* serial number                  */\
     unsigned short version;   /* version number                 */\
     long           status;    /* status                         */

#define REQUEST_HDR_MAPPING	\
     { "SERIAL",    OFFSET(serial),   DT_MTYPE_ULONG  }, \
     { "VERSION",   OFFSET(version),  DT_MTYPE_USHORT }, \
     { "CLIENT",    OFFSET(client),   DT_MTYPE_ULONG  }, \
     { "NODENAME",  OFFSET(nodename), DT_MTYPE_STRING },

#define REPLY_HDR_MAPPING	\
     { "SERIAL",    OFFSET(serial),   DT_MTYPE_ULONG  }, \
     { "VERSION",   OFFSET(version),  DT_MTYPE_USHORT }, \
     { "STATUS",    OFFSET(status),   DT_MTYPE_LONG   },

#define DTM_MSG   1
#define WB_MSG	   2
#define HELP_MSG  3

#ifdef TOOLBOX
#define TOOLBOX_MSG	4
#endif

typedef struct {
	REQUEST_HDR
} DtRequestHeader;

typedef struct {
	REPLY_HDR
} DtReplyHeader;

typedef struct {
     const char *name;
     int        offset;
     int        type;
} DtStrToStructMapping;

typedef struct {
     const char                 *name;    /* name of request type    */
     unsigned short             type;     /* msg type                */
     DtStrToStructMapping const *mapping; /* mapping description     */
     int                        count;    /* # of entries in mapping */
} DtMsgInfo;

#include "DtDTMMsg.h"
#include "DtWBMsg.h"
#include "DtHMMsg.h"

typedef union {
     DtRequestHeader               header;

     /* DTM */
     DtOpenFolderRequest           open_folder;
     DtSyncFolderRequest           sync_folder;
     DtCreateFclassRequest         create_fclass;
     DtDeleteFclassRequest         delete_fclass;
     DtQueryFclassRequest          query_fclass;
     DtGetDesktopPropertyRequest   get_property;
     DtSetDesktopPropertyRequest   set_property;
     DtDisplayPropSheetRequest     display_prop_sheet;
     DtDisplayBinderRequest        display_binder;
     DtOpenFMapRequest             open_fmap;
     DtShutdownRequest             dt_shutdown;
     DtGetClassOfFileRequest       get_fclass;
     DtGetFileNameRequest          get_fname;
     DtMergeResRequest       	   merge_res;
     DtCloseFolderRequest	   close_folder;
     DtSetFilePropRequest	   set_file_prop;
     DtSyncRequest		   sync;

     /* HELP */
     DtDisplayHelpRequest          display_help;
     DtOLDisplayHelpRequest        ol_display_help;
     DtAddToHelpDeskRequest        add_to_helpdesk;
     DtDelFromHelpDeskRequest      del_from_helpdesk;

     /* WB */
     DtMoveToWBRequest             move_to_wb;
     DtMoveFromWBRequest           move_from_wb;
     DtDisplayWBRequest            display_wastebasket;

     long pad[24];
} DtRequest;

typedef union {
     DtReplyHeader                 header;

     /* DTM */
     DtOpenFolderReply             open_folder;
     DtCreateFclassReply           create_fclass;
     DtQueryFclassReply            query_fclass;
     DtDeleteFclassReply           delete_fclass;
     DtGetDesktopPropertyReply     get_property;
     DtGetClassOfFileReply         get_fclass;
     DtGetFileNameReply            get_fname;
     DtMergeResReply         	   merge_res;
     DtSyncReply         	   sync;

     /* HELP */
     DtAddToHelpDeskReply          add_to_helpdesk;
     DtDelFromHelpDeskReply        del_from_helpdesk;

     /* WB */
     DtMoveToWBReply              move_to_wb;
     DtMoveFromWBReply            move_from_wb;

     long pad[12];
} DtReply;

/*
 * Description of string-to-struct mapping
 */

/* member type */
#define DT_MTYPE_SHORT     1
#define DT_MTYPE_LONG      2
#define DT_MTYPE_USHORT    3
#define DT_MTYPE_ULONG     4
#define DT_MTYPE_STRING    5
#define DT_MTYPE_BOOLEAN   6
#define DT_MTYPE_PLIST     7
#define DT_MTYPE_CHAR      8
#define DT_MTYPE_FLOAT     9
#define DT_MTYPE_DOUBLE   10

/* standard reply status */
#define DT_OK              0
#define DT_FAILED         -1
#define DT_BAD_INPUT      -2
#define DT_DUP            -3
#define DT_NOENT          -4
#define DT_BUSY           -5

/* options */
#define DT_GET_PROPERTIES	(1 << 0)
#define DT_UPPER_CASE		(1 << 1)
#define DT_EXPAND_PROPERTIES	(1 << 2)

/* options for DT_OPEN_FOLDER */
#define DT_SHOW_HIDDEN_FILES  (1 << 0)
#define DT_NOTIFY_ON_CLOSE    (1 << 1)
#define DT_ICONIC_VIEW        (1 << 2)
#define DT_NAME_VIEW          (1 << 3)
#define DT_LONG_VIEW          (1 << 4)
#define DT_TREE_VIEW          (1 << 5)

/* options for DT_SYNC_FOLDER */
#define DT_SYNC_ALL_FOLDERS       (1 << 0)
#define DT_SYNC_ALL_SUB_FOLDERS   (1 << 1)

/* options for DT_CREATE_FILE_CLASS */
#define DT_APPEND             (1 << 0)
#define DT_PRIVATE            (1 << 1)

#ifdef __cplusplus
extern "C" {
#endif

/* function prototypes */
extern void DtClearQueue(Screen *scrn, Atom queue);
extern int DtEnqueueRequest(Screen *scrn, Atom queue, Atom my_queue,
			Window client, DtRequest *request);
extern int DtEnqueueStrRequest(Screen *scrn, Atom queue, Atom my_queue,
			Window client, char *str, int len);
extern int DtDequeueRequest(Screen *scrn, Atom queue, Window *client,
			 DtRequest *request);
extern char *DtDequeueMsg(Screen *scrn, Atom queue, Window window);
extern int DtSendReply  (Screen *scrn, Atom queue, Window client,
			 DtReply *reply);
extern int DtAcceptReply(Screen *scrn, Atom queue, Window client,
			 DtReply *reply);
extern void DtFreeReply(DtReply *reply);

#ifdef __cplusplus
}
#endif

#endif /* __DtMsg_h__ */

