#ident	"@(#)r5extensions:lib/Xxyz.c	1.1"


/*
 * XamineYourZerver extension library routines
 *
 * $Revision$
 */

#include <X11/Xlibint.h>
#include "xyzstr.h"
#include <X11/extensions/Xext.h>
#include "extutil.h"

static XExtensionInfo *xyz_info;
static char *xyz_extension_name = XAMINE_YOUR_ZERVER_NAME;

#define XYZ_CheckExtension(dpy,i,val) \
   XextCheckExtension(dpy, i, xyz_extension_name, val)

/*****************************************************************************
 *                                                                           *
 *                         private utility routines                          *
 *                                                                           *
 *****************************************************************************/

static int close_display();
static /* const */ XExtensionHooks xyz_extension_hooks = {
    NULL,                               /* create_gc */
    NULL,                               /* copy_gc */
    NULL,                               /* flush_gc */
    NULL,                               /* free_gc */
    NULL,                               /* create_font */
    NULL,                               /* free_font */
    close_display,                      /* close_display */
    NULL,                               /* wire_to_event */
    NULL,                               /* event_to_wire */
    NULL,                               /* error */
    NULL                                /* error_string */
};

static XEXT_GENERATE_FIND_DISPLAY(find_display, xyz_info, xyz_extension_name,
   &xyz_extension_hooks, XYZ_NumberEvents, NULL)

static XEXT_GENERATE_CLOSE_DISPLAY(close_display, xyz_info)

/*****************************************************************************
 *                                                                           *
 *                  public Xamine Your Zerver routines                       *
 *                                                                           *
 *****************************************************************************/

Bool
XYZ_QueryExtension(dpy)
Display *dpy;
{
   XExtDisplayInfo *info = find_display(dpy);

   if(XextHasExtension(info)) {
      return True;
   } else {
      return False;
   }
}

Status
XYZ_Instrument(dpy, instrument)
Display *dpy;
int instrument;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_InstrumentReq *req;

   XYZ_CheckExtension(dpy, info, 0);

   LockDisplay(dpy);
   GetReq(XYZ_Instrument, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_Instrument;
   req->instrument = instrument;
   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

Status
XYZ_Trace(dpy, trace)
Display *dpy;
int trace;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_TraceReq *req;

   XYZ_CheckExtension(dpy, info, 0);

   LockDisplay(dpy);
   GetReq(XYZ_Trace, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_Trace;
   req->trace = trace;
   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

Status
XYZ_SetCurTraceLevel(dpy, tracelevel)
Display *dpy;
int tracelevel;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_SetCurTraceLevelReq *req;

   XYZ_CheckExtension(dpy, info, 0);

   LockDisplay(dpy);
   GetReq(XYZ_SetCurTraceLevel, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_SetCurTraceLevel;
   req->tracelevel = tracelevel;
   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

Status
XYZ_QueryState(dpy, instrument, trace, tracelevel, status)
Display *dpy;
int *instrument;
int *trace;
int *tracelevel;
int *status;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_QueryStateReq *req;
   xXYZ_QueryStateReply rep;

   XYZ_CheckExtension(dpy, info, 0);

   LockDisplay(dpy);
   GetReq(XYZ_QueryState, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_QueryState;
   if(!_XReply(dpy, &rep, 0, xFalse)) {
      UnlockDisplay(dpy);
      SyncHandle();
      return 0;
   }
   *instrument = rep.instrument;
   *trace = rep.trace;
   *tracelevel = rep.tracelevel;
   *status = rep.status;
   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

Status
XYZ_GetTag(dpy, tagname, value, tracelevel)
Display *dpy;
char *tagname;
int *value;
int *tracelevel;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_GetTagReq *req;
   xXYZ_GetTagReply rep;
   int nChars;

   XYZ_CheckExtension(dpy, info, 0);

   nChars = strlen(tagname);
   LockDisplay(dpy);
   GetReq(XYZ_GetTag, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_GetTag;
   req->length += (nChars + 3) >> 2;
   req->nChars = nChars;
   _XSend(dpy, tagname, nChars);
   if(!_XReply(dpy, &rep, 0, xFalse)) {
      UnlockDisplay(dpy);
      SyncHandle();
      return 0;
   }

   *value = rep.value;
   *tracelevel = rep.tracelevel;

   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

Status
XYZ_SetValue(dpy, tagname, value)
Display *dpy;
char *tagname;
int value;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_SetValueReq *req;
   int nChars;

   XYZ_CheckExtension(dpy, info, 0);

   nChars = strlen(tagname);
   LockDisplay(dpy);
   GetReq(XYZ_SetValue, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_SetValue;
   req->length += (nChars + 3) >> 2;
   req->value = value;
   req->nChars = nChars;
   Data(dpy, tagname, nChars);

   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

Status
XYZ_SetTraceLevel(dpy, tagname, tracelevel)
Display *dpy;
char *tagname;
int tracelevel;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_SetTraceLevelReq *req;
   int nChars;

   XYZ_CheckExtension(dpy, info, 0);

   nChars = strlen(tagname);
   LockDisplay(dpy);
   GetReq(XYZ_SetTraceLevel, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_SetTraceLevel;
   req->length += (nChars + 3) >> 2;
   req->tracelevel = tracelevel;
   req->nChars = nChars;
   Data(dpy, tagname, nChars);

   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

XYZ_value *
XYZ_ListValues(dpy, npats, pats, patlens, maxtags, total, returned)
Display *dpy;
int npats;
char **pats;
int *patlens;
int maxtags;
int *total;
int *returned;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_ListValuesReq *req;
   xXYZ_ListValuesReply rep;
   CARD16 nChars;
   char *data;
   char *valptr;
   int rlen;
   int length;
   XYZ_value *mem;
   XYZ_value *vals;
   int i;
   int space;

   XYZ_CheckExtension(dpy, info, 0);

   LockDisplay(dpy);
   GetReq(XYZ_ListValues, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_ListValues;
   for(i=0;i<npats;i++) {
      req->length += (patlens[i] + 2 + 3) >> 2;
   }
   req->npats = npats;
   req->maxtags = maxtags;

   for(i=0;i<npats;i++) {
      nChars = patlens[i];
      /* the size for BufAlloc is careful to be 4-byte aligned */
      space = (2 + nChars + 3) & ~3;
      BufAlloc(char *, data, space);
      *((CARD16 *) data) = nChars;
      bcopy(pats[i], data + 2, patlens[i]);
   }

   if(!_XReply(dpy, &rep, 0, xFalse)) {
      /* error receiving reply */
      UnlockDisplay(dpy);
      SyncHandle();
      return NULL;
   }
  
   *total = -1;
   *returned = -1;

   rlen = rep.length << 2;
   if(rep.returned > 0) {
      data = (char *) Xmalloc((unsigned) rlen);
      mem = (XYZ_value *) Xmalloc((rep.returned + 1) * sizeof(XYZ_value));
      mem[0].tagname = data;
      vals = &mem[1];

      if((!data) || (!vals)) {
	 /* memory allocation failure */
	 if(data) Xfree(data);
	 if(vals) Xfree((char *) mem);
	 _XEatData(dpy, (unsigned long) rlen);
	 UnlockDisplay(dpy);
	 SyncHandle();
	 return NULL;
      }

      _XReadPad(dpy, data, rlen);
      for(i=0; i < (int)rep.returned; i++) {
	 /* grab length of tag name */
	 length = *((unsigned short *) data);
	 /* advance past tag name length */
	 data += sizeof(unsigned short);
	 /* save pointer to tag name */
	 vals[i].tagname = data;
	 /* make sure read value from aligned boundary */
	 valptr = (char *)((int)(data + length + 3) & ~3);
	 /* save tag value */
	 vals[i].value = *((int *)valptr);
	 /* null terminate tag name */
	 data[length] = '\0';
	 /* advance to start of next name/value pair*/
	 data= valptr + sizeof(int);
      }
   } else {
      vals = NULL;
   }
   *total = rep.total;
   *returned = rep.returned;

   UnlockDisplay(dpy);
   SyncHandle();
   return vals;
}

void
XYZ_FreeValueList(values_list)
XYZ_value *values_list;
{
   values_list--;
   Xfree(values_list[0].tagname);
   Xfree(values_list);
}

Status
XYZ_ResetValues(dpy)
Display *dpy;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_ResetValuesReq *req;

   XYZ_CheckExtension(dpy, info, 0);

   LockDisplay(dpy);
   GetReq(XYZ_ResetValues, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_ResetValues;
   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

Status
XYZ_ResetTraceLevels(dpy)
Display *dpy;
{
   XExtDisplayInfo *info = find_display(dpy);
   xXYZ_ResetTraceLevelsReq *req;

   XYZ_CheckExtension(dpy, info, 0);

   LockDisplay(dpy);
   GetReq(XYZ_ResetTraceLevels, req);
   req->reqType = info->codes->major_opcode;
   req->xyzReqType = X_XYZ_ResetTraceLevels;
   UnlockDisplay(dpy);
   SyncHandle();
   return 1;
}

