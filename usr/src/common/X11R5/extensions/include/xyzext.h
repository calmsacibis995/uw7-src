#ident	"@(#)r5extensions:include/xyzext.h	1.1"


#ifndef _XYZEXT_H_
#define _XYZEXT_H_

#define X_XYZ_Trace 0
#define X_XYZ_Instrument 1
#define X_XYZ_SetCurTraceLevel 2
#define X_XYZ_QueryState 3
#define X_XYZ_GetTag 4
#define X_XYZ_SetValue 5
#define X_XYZ_SetTraceLevel 6
#define X_XYZ_ListValues 7
#define X_XYZ_ResetValues 8
#define X_XYZ_ResetTraceLevels 9

#define XYZ_NumberEvents 0
#define XYZ_NumberErrors 0

#define XYZ_NO_ERROR 0
#define XYZ_ERROR 1

#define XYZ_DEFAULT_TRACE_LEVEL 255

#ifndef _XAMINE_YOUR_ZERVER_SERVER_

extern Bool XYZ_QueryExtension(
#if NeedFunctionPrototypes
   Display * /* dpy */
#endif
);
extern Status XYZ_Instrument(
#if NeedFunctionPrototypes
   Display * /* dpy */,
   int /* on or off */
#endif
);
extern Status XYZ_Trace(
#if NeedFunctionPrototypes
   Display * /* dpy */,
   int /* on or off */
#endif
);
extern Status XYZ_SetCurTraceLevel(
#if NeedFunctionPrototypes
   Display * /* dpy */,
   int /* tracelevel */
#endif
);
extern Status XYZ_QueryState(
#if NeedFunctionPrototypes
   Display * /* dpy */,
   int * /* RETVAL instrument */,
   int * /* RETVAL trace */,
   int * /* RETVAL tracelevel */,
   int * /* RETVAL status */
#endif
);
extern Status XYZ_GetTag(
#if NeedFunctionPrototypes
   Display * /* dpy */,
   char * /* tagname */,
   int * /* RETVAL value */,
   int * /* RETVAL tracelevel */
#endif
);
extern Status XYZ_SetValue(
#if NeedFunctionPrototypes
   Display * /* dpy */,
   char * /* tagname */,
   int /* value */
#endif
);
extern Status XYZ_SetTraceLevel(
#if NeedFunctionPrototypes
   Display * /* dpy */,
   char * /* tagname */,
   int /* tracelevel */
#endif
);
typedef struct _XYZ_value {
   char *tagname;
   int value;
} XYZ_value;
extern XYZ_value * XYZ_ListValues(
#if NeedFunctionPrototypes
   Display * /* dpy */,
   int /* npats */,
   char ** /* pats */,
   int * /* patlens */,
   int /* maxtags */,
   int * /* RETVAL total */,
   int * /* RETVAL returned */
#endif
);
extern void XYZ_FreeValueList(
#if NeedFunctionPrototypes
   XYZ_value * /* value list */
#endif
);
extern Status XYZ_ResetValues(
#if NeedFunctionPrototypes
   Display * /* dpy */
#endif
);
extern Status XYZ_ResetTraceLevels(
#if NeedFunctionPrototypes
   Display * /* dpy */
#endif
);

#endif /* _XAMINE_YOUR_SERVER_SERVER_ */

#endif /* _XYZEXT_H_ */
