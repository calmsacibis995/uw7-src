#pragma ident	"@(#)builtinext:include/builtinstr.h	1.1"

#ifndef BUILTINSTR_H_
#define BUILTINSTR_H_

#include "builtin.h"

#define BUILTIN_EXT_NAME	"Builtin"
#define BUILTIN_MAJOR_VERSION	1		/* current version numbers */
#define BUILTIN_MINOR_VERSION	0

typedef struct _BuiltinQueryVersionReq {
    CARD8	reqType;		/* major opcode for this ext */
    CARD8	builtinReqType;		/* always X_BuiltinQueryVersion */
    CARD16	length B16;
} xBuiltinQueryVersionReq;
#define sz_xBuiltinQueryVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	valid;			/* if valid to run as builtin client */
    CARD16	sequenceNumber B16;	/* last sequence number */
    CARD32	length B32;
    CARD16	majorVersion B16;	/* BUILTIN extension major version */
    CARD16	minorVersion B16;	/* BUILTIN extension minor version */
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
} xBuiltinQueryVersionReply;
#define sz_xBuiltinQueryVersionReply	32

typedef struct _BuiltinRunClientReq {
    CARD8	reqType;		/* major opcode for this ext */
    CARD8	builtinReqType;		/* always X_BuiltinRunClient */
    CARD16	length B16;
} xBuiltinRunClientReq;
#define sz_xBuiltinRunClientReq	4

typedef struct _BuiltinExecNotify {
    BYTE	type;			/* eventBase + BuiltinExecNotify */
    BYTE	detail;			/* unused */
    CARD16	sequenceNumber B16;
    CARD32	pad1 B32;		/* unused */
    CARD32	pad2 B32;		/* unused */
    CARD32	pad3 B32;		/* unused */
    CARD32	pad4 B32;		/* unused */
    CARD32	pad5 B32;		/* unused */
    CARD32	pad6 B32;		/* unused */
    CARD32	pad7 B32;		/* unused */
} xBuiltinExecNotifyEvent;
#define sz_xBuiltinExecNotifyEvent	32

typedef struct _BuiltinExitNotify {
    BYTE	type;			/* eventBase + BuiltinExitNotify */
    BYTE	detail;			/* unused */
    CARD16	sequenceNumber B16;
    CARD32	exit_code B32;
    CARD32	pad1 B32;		/* unused */
    CARD32	pad2 B32;		/* unused */
    CARD32	pad3 B32;		/* unused */
    CARD32	pad4 B32;		/* unused */
    CARD32	pad5 B32;		/* unused */
    CARD32	pad6 B32;		/* unused */
} xBuiltinExitNotifyEvent;
#define sz_xBuiltinExitNotifyEvent	32

#endif /* BUILTINSTR_H_ */
