#ident	"@(#)r5extensions:include/xyzstr.h	1.1"


#ifndef _XYZSTR_H_
#define _XYZSTR_H_

#include "xyzext.h"

#define XAMINE_YOUR_ZERVER_NAME "XamineYourZerver"

typedef CARD8 XYZSTATUS;

typedef struct _XYZ_Instrument {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_Instrument */
   CARD16 length B16;
   BOOL instrument;
   BYTE pad1;
   BYTE pad2;
   BYTE pad3;
} xXYZ_InstrumentReq;
#define sz_xXYZ_InstrumentReq 8

typedef struct _XYZ_Trace {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_Trace */
   CARD16 length B16;
   BOOL trace;
   BYTE pad1;
   BYTE pad2;
   BYTE pad3;
} xXYZ_TraceReq;
#define sz_xXYZ_TraceReq 8

typedef struct _XYZ_SetCurTraceLevel {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_SetCurTraceLevel */
   CARD16 length B16;
   CARD8 tracelevel;
   BYTE pad1;
   BYTE pad2;
   BYTE pad3;
} xXYZ_SetCurTraceLevelReq;
#define sz_xXYZ_SetCurTraceLevelReq 8

typedef struct _XYZ_QueryState {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_QueryState */
   CARD16 length B16;
} xXYZ_QueryStateReq;
#define sz_xXYZ_QueryStateReq 4

typedef struct {
   BYTE type;
   BYTE pad;
   CARD16 sequenceNumber B16;
   CARD32 length B32;
   CARD8 tracelevel;
   XYZSTATUS status;
   BOOL instrument;
   BOOL trace;
   CARD32 pad1 B32;
   CARD32 pad2 B32;
   CARD32 pad3 B32;
   CARD32 pad4 B32;
   CARD32 pad5 B32;
} xXYZ_QueryStateReply;
#define sz_xXYZ_QueryStateReply 32

typedef struct _XYZ_GetTag {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_GetTag */
   CARD16 length B16;
   CARD16 nChars B16;
   CARD16 pad B16;
} xXYZ_GetTagReq;
#define sz_xXYZ_GetTagReq 8

typedef struct {
   BYTE type;
   CARD8 tracelevel;
   CARD16 sequenceNumber B16;
   CARD32 length B32;
   INT32 value B32;
   CARD32 pad0 B16;
   CARD32 pad1 B32;
   CARD32 pad2 B32;
   CARD32 pad3 B32;
   CARD32 pad4 B32;
} xXYZ_GetTagReply;
#define sz_xXYZ_GetTagReply 32

typedef struct _XYZ_SetValue {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_SetValue */
   CARD16 length B16;
   INT32 value B32;
   CARD16 nChars B16;
   CARD16 pad B16;
} xXYZ_SetValueReq;
#define sz_xXYZ_SetValueReq 12 

typedef struct _XYZ_SetTraceLevel {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_SetTraceLevel */
   CARD16 length B16;
   CARD8 tracelevel;
   CARD8 pad;
   CARD16 nChars B16;
} xXYZ_SetTraceLevelReq;
#define sz_xXYZ_SetTraceLevelReq 8 

typedef struct _XYZ_ListValuesReq {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_ListValues */
   CARD16 length B16;
   CARD16 npats B16;
   CARD16 maxtags B16;
} xXYZ_ListValuesReq;
#define sz_xXYZ_ListValuesReq 8

typedef struct _XYZ_ListValuesReply {
   BYTE type;
   BYTE pad;
   CARD16 sequenceNumber B16;
   CARD32 length B32;
   CARD16 returned B32;
   CARD16 total B32;
   CARD32 pad0 B32;
   CARD32 pad1 B32;
   CARD32 pad2 B32;
   CARD32 pad3 B32;
   CARD32 pad4 B32;
} xXYZ_ListValuesReply;
#define sz_xXYZ_ListValuesReply 32

typedef struct _XYZ_ResetValuesReq {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_ResetValues */
   CARD16 length B16;
} xXYZ_ResetValuesReq;
#define sz_xXYZ_ResetValuesReq 4

typedef struct _XYZ_ResetTraceLevelsReq {
   CARD8 reqType; /* XYZReqCode */
   CARD8 xyzReqType; /* always X_XYZ_ResetTraceLevels */
   CARD16 length B16;
} xXYZ_ResetTraceLevelsReq;
#define sz_xXYZ_ResetTraceLevelsReq 4

#endif /* _XYZSTR_H_ */

