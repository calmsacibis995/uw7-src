#ifndef __DtDTMMsg_h__
#define __DtDTMMsg_h__

#ifndef NOIDENT
#ident	"@(#)Dt:DtDTMMsg.h	1.18"
#endif

/*
 **************************************************************************
 *
 * Description:
 *              This file contains all the things related to DTM request
 *       and reply messages.
 *
 **************************************************************************
 */

/* message function */
#define DTM_MSG		1
#define DTM_FUNC	(DTM_MSG << MSGFUNC_SHIFT)

/* functional specific request types */
#define DT_OPEN_FOLDER		(DTM_FUNC | 1)
#define DT_SYNC_FOLDER		(DTM_FUNC | 2)
#define DT_CREATE_FILE_CLASS	(DTM_FUNC | 3)
#define DT_DELETE_FILE_CLASS	(DTM_FUNC | 4)
#define DT_QUERY_FILE_CLASS	(DTM_FUNC | 5)
#define DT_GET_DESKTOP_PROPERTY	(DTM_FUNC | 6)
#define DT_SET_DESKTOP_PROPERTY	(DTM_FUNC | 7)
#define DT_DISPLAY_PROP_SHEET  	(DTM_FUNC | 8)
#define DT_DISPLAY_BINDER      	(DTM_FUNC | 9)
#define DT_OPEN_FMAP		(DTM_FUNC | 10)
#define DT_SHUTDOWN		(DTM_FUNC | 11)
#define DT_GET_FILE_CLASS	(DTM_FUNC | 12)
#define DT_GET_FILE_NAME	(DTM_FUNC | 13)
#define DT_MERGE_RES		(DTM_FUNC | 14)
#define DT_CLOSE_FOLDER		(DTM_FUNC | 15)
#define DT_SET_FILE_PROPERTY	(DTM_FUNC | 16)
#define DT_SYNC			(DTM_FUNC | 17)

#define DT_DTM_NUM_REQUESTS	17
#define DT_DTM_NUM_REPLIES	9

#define DT_NO_FILETYPE 0
#define DT_DIR_TYPE 1
#define DT_EXEC_TYPE 2
#define DT_DATA_TYPE 3
#define DT_FIFO_TYPE 4
#define DT_CHRDEV_TYPE 5
#define DT_BLKDEV_TYPE 6
#define DT_SEM_TYPE 7
#define DT_SHDATA_TYPE 8
#define DT_IGNORE_FILETYPE 9

typedef struct {
	REQUEST_HDR
	char		*title;		/* title of folder window */
	char		*path;		/* full path of directory */
	DtAttrs		options;	/* options */
	char		*pattern;	/* filename pattern */
	char		*class_name;	/* file class name */
} DtOpenFolderRequest;

typedef struct {
	REQUEST_HDR
	char		*path;		/* full path of directory */
	DtAttrs		options;	/* options */
} DtCloseFolderRequest;

typedef struct {
	REQUEST_HDR
	char		*path;		/* full path of container */
	char		*name;		/* object name */
	DtPropList 	plist;		/* property list type */
} DtSetFilePropRequest;

typedef struct {
	REQUEST_HDR
	char		*path;		/* full path of directory */
	DtAttrs		options;	/* options */
} DtSyncFolderRequest;

typedef struct {
	REQUEST_HDR
	char		*prop_name;	/* name of property sheet */
} DtDisplayPropSheetRequest;

typedef struct {
	REQUEST_HDR
} DtDisplayBinderRequest;

typedef struct {
	REQUEST_HDR
} DtOpenFMapRequest;

typedef struct {
	REQUEST_HDR
} DtShutdownRequest;

typedef struct {
	REQUEST_HDR
} DtSyncRequest;

typedef struct {
	REQUEST_HDR
	char		*class_name;	/* file class name */
	DtAttrs		options;	/* options */
} DtQueryFclassRequest;

typedef struct {
	REQUEST_HDR
	char		*file_name;	/* class database file name */
} DtDeleteFclassRequest;

typedef struct {
	REQUEST_HDR
	char		*file_name;	/* class database file name */
	DtAttrs		options;	/* options */
} DtCreateFclassRequest;

typedef struct {
	REQUEST_HDR
	char		*name;		/* property name */
} DtGetDesktopPropertyRequest;

typedef struct {
	REQUEST_HDR
	char		*name;		/* property name */
	char		*value;		/* property value */
	DtAttrs	attrs;		/* attributes */
} DtSetDesktopPropertyRequest;

typedef struct {
	REQUEST_HDR
	char			*file_name;	/* file name */
	unsigned short	file_type;	/* file type */
	DtPropList 	plist;		/* property list type */
	DtAttrs		options;		/* options */
} DtGetClassOfFileRequest;

typedef struct {
	REQUEST_HDR
	Window	win_id;	/* window id */
	int	icon_x;	/* x coordinate */
	int	icon_y;	/* y coordinate */
} DtGetFileNameRequest;

typedef struct {
	REQUEST_HDR
	char	*resp;		/*  add resource string */
	uchar_t	 flag;		/*  to merage resources or not */
} DtMergeResRequest;

typedef struct {
	REPLY_HDR
} DtOpenFolderReply;

typedef struct {
	REPLY_HDR
	char		*class_name;	/* file class name */
	DtPropList	plist;		/* property list */
} DtQueryFclassReply;

typedef struct {
	REPLY_HDR
} DtCreateFclassReply;

typedef struct {
	REPLY_HDR
} DtDeleteFclassReply;

typedef struct {
	REPLY_HDR
	char		*value;		/* property value */
	DtAttrs		attrs;		/* attributes */
} DtGetDesktopPropertyReply;

typedef struct {
	REPLY_HDR
	char		*file_name;	/* file name */
	char		*class_name;	/* class name */
	DtPropList	plist;		/* property list */
} DtGetClassOfFileReply;

typedef struct {
	REPLY_HDR
	char	*file_name;	/* file name */
} DtGetFileNameReply;

typedef struct {
	REPLY_HDR
} DtMergeResReply;

typedef struct {
	REPLY_HDR
} DtSyncReply;


extern DtMsgInfo const Dt__dtm_msgtypes[DT_DTM_NUM_REQUESTS];
extern DtMsgInfo const Dt__dtm_replytypes[DT_DTM_NUM_REPLIES];
#endif /* __DtDTMMsg_h__ */
