#ident	"@(#)r5extensions:include/xtrap/xtraplibp.h	1.1.1.1"


#ifndef __XTRAPLIBPROTO__
#define __XTRAPLIBPROTO__


/*****************************************************************************
Copyright 1987, 1988, 1989, 1990, 1991 by Digital Equipment Corp., Maynard, MA

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*****************************************************************************/
/*
 *
 *  CONTRIBUTORS:
 *
 *      Dick Annicchiarico
 *      Robert Chesler
 *      Dan Coutu
 *      Gene Durso
 *      Marc Evans
 *      Alan Jamison
 *      Mark Henry
 *      Ken Miller
 *
 *  DESCRIPTION:
 *      This header file contains the function prototypes for client/toolkit
 *      routines sorted by module (globally defined routines *only*).
 */
#if (defined(__STDC__) || defined (vaxc))
#define FUNCTION_PROTOS
#endif
#ifdef FUNCTION_PROTOS
# define	P(s) s
#else
# define P(s) ()
#endif

/* XETrapContext.c */
XETC *XECreateTC P((Display *dpy , CARD32 valuemask , XETCValues *value ));
int XEChangeTC P((XETC *tc , CARD32 mask , XETCValues *values ));
void XEFreeTC P((XETC *tc ));
int XETrapSetMaxPacket P((XETC *tc , Bool set_flag , CARD16 size ));
int XETrapSetCommandKey P((XETC *tc , Bool set_flag , KeySym cmd_key , 
    Bool mod_flag ));
int XETrapSetTimestamps P((XETC *tc , Bool set_flag , Bool delta_flag ));
int XETrapSetWinXY P((XETC *tc , Bool set_flag ));
int XETrapSetCursor P((XETC *tc , Bool set_flag ));
int XETrapSetXInput P((XETC *tc , Bool set_flag ));
int XETrapSetColorReplies P((XETC *tc , Bool set_flag ));
int XETrapSetGrabServer P((XETC *tc , Bool set_flag ));
int XETrapSetStatistics P((XETC *tc , Bool set_flag ));
int XETrapSetRequests P((XETC *tc , Bool set_flag , ReqFlags requests ));
int XETrapSetEvents P((XETC *tc , Bool set_flag , EventFlags events ));
Bool XESetCmdGateState P((XETC *tc , CARD8 type, Bool *gate_closed , 
    CARD8 *next_key , Bool *key_ignore ));

/* XETrapRequests.c */
int XEFlushConfig P((XETC *tc ));
int XEResetRequest P((XETC *tc ));
int XEGetVersionRequest P((XETC *tc , XETrapGetVersRep *ret ));
int XEGetLastInpTimeRequest P((XETC *tc , XETrapGetLastInpTimeRep *ret ));
int XEGetAvailableRequest P((XETC *tc , XETrapGetAvailRep *ret ));
int XEStartTrapRequest P((XETC *tc ));
int XEStopTrapRequest P((XETC *tc ));
int XESimulateXEventRequest P((XETC *tc , CARD8 type , CARD8 detail , 
    CARD16 x , CARD16 y , CARD8 screen ));
int XEGetCurrentRequest P((XETC *tc , XETrapGetCurRep *ret ));
int XEGetStatisticsRequest P((XETC *tc , XETrapGetStatsRep *ret ));

/* XECallbacks.c */
int XEAddRequestCB P((XETC *tc , CARD8 req , void_function func , BYTE *data ));
int XEAddRequestCBs P((XETC *tc , ReqFlags req_flags , void_function func , 
    BYTE *data ));
int XEAddEventCB P((XETC *tc , CARD8 evt , void_function func , BYTE *data ));
int XEAddEventCBs P((XETC *tc , EventFlags evt_flags , void_function func , 
    BYTE *data ));

/* XETrapDispatch.c */
void XETrapDispatchXLib P((XETrapDataEvent *event , XETC *tc));

/* XEWrappers.c */
Boolean XETrapDispatchEvent P((XEvent *pevent , XETC *tc ));
XtInputMask XETrapAppPending P((XtAppContext app));
void XETrapAppMainLoop P((XtAppContext app , XETC *tc ));
int XETrapAppWhileLoop P((XtAppContext app , XETC *tc , Bool *done ));
#ifdef X11R3
Widget XtAppInitialize P((XtAppContext *app , String application_class ,
    XrmOptionDescRec *options , Cardinal num_options , Cardinal *argc_in_out ,
    String *argv_in_out , String *fallback_resources ,
    ArgList args_in , Cardinal num_args_in));
#endif
int XETrapWaitForSomething P((XtAppContext app ));

/* XEPrintInfo.c */
void XEPrintRelease P((FILE *ofp , XETrapGetAvailRep *pavail ));
void XEPrintPlatform P((FILE *ofp , XETrapGetAvailRep *pavail ));
void XEPrintAvailFlags P((FILE *ofp , XETrapGetAvailRep *pavail ));
void XEPrintAvailPktSz P((FILE *ofp , XETrapGetAvailRep *pavail ));
void XEPrintStateFlags P((FILE *ofp , XETrapGetCurRep *pcur ));
void XEPrintMajOpcode P((FILE *ofp , XETrapGetAvailRep *pavail ));
void XEPrintCurXY P((FILE *ofp , XETrapGetAvailRep *pavail ));
void XEPrintTkFlags P((FILE *ofp , XETC *tc ));
void XEPrintLastTime P((FILE *ofp , XETC *tc ));
void XEPrintCfgFlags P((FILE *ofp , XETrapGetCurRep *pcur ));
void XEPrintRequests P((FILE *ofp , XETrapGetCurRep *pcur ));
void XEPrintEvents P((FILE *ofp , XETrapGetCurRep *pcur ));
void XEPrintCurPktSz P((FILE *ofp , XETrapGetCurRep *pcur ));
void XEPrintCmdKey P((FILE *ofp , XETrapGetCurRep *pcur ));
void XEPrintEvtStats P((FILE *ofp , XETrapGetStatsRep *pstats ));
void XEPrintReqStats P((FILE *ofp , XETrapGetStatsRep *pstats ));
void XEPrintAvail P((FILE *ofp , XETrapGetAvailRep *pavail ));
void XEPrintTkState P((FILE *ofp , XETC *tc ));
void XEPrintCurrent P((FILE *ofp , XETrapGetCurRep *pcur ));
void XEPrintStatistics P((FILE *ofp , XETrapGetStatsRep *pstats ));

/* XEStringMapping.c */
INT16 XEEventStringToID P((char *string ));
INT16 XERequestStringToID P((char *string ));
CARD32 XEPlatformStringToID P((char *string ));
char *XEEventIDToString P((CARD8 id ));
char *XERequestIDToExtString P((register CARD8 id ));
char *XERequestIDToString P((CARD8 id ));
char *XEPlatformIDToString P((CARD32 id ));

/* getopt.c */
int *XEgetopterr();
int *XEgetoptind();
int *XEgetoptopt();
char **XEgetoptarg();
#ifdef SVR4 /* PPc:md */
int getopt P((int argc , char *const *argv , const char *optstring ));
#else
int getopt P((int argc , char **argv , char *optstring ));
#endif

/* sleep.c */
int usleep P((unsigned long usecs ));

/* XEKeybCtrl.c */
int XEEnableCtrlKeys P((void (*rtn)() ));
int XEClearCtrlKeys P((void ));
int XEEnableCtrlC P((void (*rtn)() ));
int XEEnableCtrlY P((void (*rtn)() ));
int XEDeclExitHndlr P((void (*rtn)() ));

#undef P

#endif /* __XTRAPLIBPROTO__ */
