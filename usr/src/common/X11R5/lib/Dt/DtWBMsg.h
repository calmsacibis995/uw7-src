#ifndef __DtWBMsg_h__
#define __DtWBMsg_h__

#ifndef NOIDENT
#ident	"@(#)Dt:DtWBMsg.h	1.4"
#endif

/*
 **************************************************************************
 *
 * Description:
 *              This file contains all the things related to WB request
 *       and reply messages.
 *
 **************************************************************************
 */

/* message function */
#define WB_FUNC		(WB_MSG << MSGFUNC_SHIFT)

/* functional specific request types */
#define DT_MOVE_TO_WB		(WB_FUNC | 1)
#define DT_MOVE_FROM_WB		(WB_FUNC | 2)
#define DT_DISPLAY_WB		(WB_FUNC | 3)

#define DT_WB_NUM_REQUESTS	3
#define DT_WB_NUM_REPLIES	2

typedef struct {
	REQUEST_HDR
	char		*pathname;
} DtMoveToWBRequest;

typedef struct {
	REQUEST_HDR
	char		*pathname;
} DtMoveFromWBRequest;

typedef struct {
	REQUEST_HDR
} DtDisplayWBRequest;

/* functional specific reply types */
#define DT_MOVE_TO_WB		(WB_FUNC | 1)
#define DT_MOVE_FROM_WB		(WB_FUNC | 2)

typedef struct {
	REPLY_HDR
} DtMoveToWBReply;

typedef struct {
	REPLY_HDR
} DtMoveFromWBReply;

extern DtMsgInfo const Dt__wb_msgtypes[DT_WB_NUM_REQUESTS];
extern DtMsgInfo const Dt__wb_replytypes[DT_WB_NUM_REPLIES];

#endif /* __DtWBMsg_h__ */
