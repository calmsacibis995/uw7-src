#ident	"@(#)r5extensions:lib/xtrap/XERqsts.c	1.2"

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
#include "Xlib.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"

/* the following's a hack to support V3.1 protocol */
#if defined(__STDC__) && !defined(UNIXCPP)
#define GetOldReq(name, req, old_length) \
        WORD64ALIGN\
        if ((dpy->bufptr + SIZEOF(x##name##Req)) > dpy->bufmax)\
                _XFlush(dpy);\
        req = (x##name##Req *)(dpy->last_req = dpy->bufptr);\
        req->reqType = X_##name;\
        req->length = old_length>>2;\
        dpy->bufptr += old_length;\
        dpy->request++

#else  /* non-ANSI C uses empty comment instead of "##" for token concat */
#define GetOldReq(name, req, old_length) \
        WORD64ALIGN\
        if ((dpy->bufptr + SIZEOF(x/**/name/**/Req)) > dpy->bufmax)\
                _XFlush(dpy);\
        req = (x/**/name/**/Req *)(dpy->last_req = dpy->bufptr);\
        req->reqType = X_/**/name;\
        req->length = old_length>>2;\
        dpy->bufptr += old_length;\
        dpy->request++
#endif

#ifndef vms
#include "Xlibint.h"
#else   /* vms */
#define SyncHandle() \
    if (dpy->synchandler) (*dpy->synchandler)(dpy)
/*
 * LockDisplay uses an undocumented feature in V5 of VMS that allows
 * disabling ASTs without calling $SETAST.  A bit is set in P1 space
 * that disables a user mode AST from being delivered to this process.
 * 
 */
#define LockDisplay(dis)             \
{   globalref char ctl$gb_soft_ast_disable;    \
    globalref char ctl$gb_lib_lock;        \
    globalref short ctl$gw_soft_ast_lock_depth;    \
    if ( ctl$gb_soft_ast_disable == 0 ) {    \
    ctl$gb_soft_ast_disable = 1;        \
    ctl$gb_lib_lock = 1;            \
    ctl$gw_soft_ast_lock_depth = 1;        \
    }                        \
    else ctl$gw_soft_ast_lock_depth++;        \
}

/*
 * UnlockDisplay clears the AST disable bit, then checks to see if an
 * AST delivery attempt was made during the critical section.  If so,
 * reenable_ASTs is set, and $SETAST must be called to turn AST delivery
 * back on.
 * 
 * Note that it assumed that LockDisplay and UnlockDisplay appear in
 * matched sets within a single routine.
 */
#define UnlockDisplay(dis)             \
{   globalref char ctl$gb_reenable_asts;    \
    globalref char ctl$gb_soft_ast_disable;    \
    globalref char ctl$gb_lib_lock;        \
    globalref short ctl$gw_soft_ast_lock_depth;    \
    if (!--ctl$gw_soft_ast_lock_depth)         \
    if ( ctl$gb_lib_lock ) {        \
        ctl$gb_lib_lock = 0;        \
            ctl$gb_soft_ast_disable = 0;    \
        if (ctl$gb_reenable_asts != 0)    \
        sys$setast(1);            \
        }                    \
}

#define WORD64ALIGN
#if defined(__STDC__) && !defined(UNIXCPP)
#define GetReq(name, req) \
        WORD64ALIGN\
        if ((dpy->bufptr + SIZEOF(x##name##Req)) > dpy->bufmax)\
                _XFlush(dpy);\
        req = (x##name##Req *)(dpy->last_req = dpy->bufptr);\
        req->reqType = X_##name;\
        req->length = (SIZEOF(x##name##Req))>>2;\
        dpy->bufptr += SIZEOF(x##name##Req);\
        dpy->request++

#else  /* non-ANSI C uses empty comment instead of "##" for token concat */
#define GetReq(name, req) \
        WORD64ALIGN\
        if ((dpy->bufptr + SIZEOF(x/**/name/**/Req)) > dpy->bufmax)\
                _XFlush(dpy);\
        req = (x/**/name/**/Req *)(dpy->last_req = dpy->bufptr);\
        req->reqType = X_/**/name;\
        req->length = (SIZEOF(x/**/name/**/Req))>>2;\
        dpy->bufptr += SIZEOF(x/**/name/**/Req);\
        dpy->request++
#endif
/* The following is from Xlibint.h from the SHRLIB component on DECWIN */
#define _XRead32(dpy, data, len) _XRead((dpy), (char *)(data), (len))
#endif /* vms */

#include "xtraplib.h"
#include "xtraplibp.h"

/* Returns the all important protocol number to be used.
 * The only request guaranteed to be of the same request/reply
 * size is XEGetVersionRequest.  All others need the protocol
 * number to determine how to communicate.
 * Unfortunately, this was broken for V3.1 so GetAvailable will
 * have to be used to determine the protocol version.
 */
#ifdef FUNCTION_PROTOS
int XEGetVersionRequest(XETC *tc, XETrapGetVersRep *ret) 
#else
int XEGetVersionRequest(tc,ret) 
    XETC *tc;
    XETrapGetVersRep *ret;
#endif
{ 
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrapGet = tc->extOpcode;
    xXTrapGetReq *reqptr;
    xXTrapGetVersReply rep; 
    int numlongs = (SIZEOF(xXTrapGetVersReply) -
        SIZEOF(xReply) + SIZEOF(CARD32) -1 ) / SIZEOF(CARD32);
    LockDisplay(dpy); 
    GetReq(XTrapGet,reqptr);
    reqptr->minor_opcode = XETrap_GetVersion;
    reqptr->protocol = XETrapProtocol;
    status = _XReply(dpy,(xReply *)&rep,numlongs,xTrue); 
    SyncHandle(); 
    UnlockDisplay(dpy); 
    memcpy((char *)ret,&(rep.data),sizeof(XETrapGetVersRep)); 
    return(status);
}

#ifdef FUNCTION_PROTOS
int XEGetAvailableRequest(XETC *tc, XETrapGetAvailRep *ret) 
#else
int XEGetAvailableRequest(tc,ret) 
    XETC *tc;
    XETrapGetAvailRep *ret;
#endif
{ 
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrapGet = tc->extOpcode;
    xXTrapGetReq *reqptr;
    xXTrapGetAvailReply rep; 
    int numlongs = (SIZEOF(xXTrapGetAvailReply) -
        SIZEOF(xReply) + SIZEOF(CARD32) -1 ) / SIZEOF(CARD32);
    LockDisplay(dpy); 
    GetReq(XTrapGet,reqptr);
    reqptr->minor_opcode = XETrap_GetAvailable;
    reqptr->protocol = XETrapProtocol;
    status = _XReply(dpy,(xReply *)&rep,numlongs,xTrue); 
    SyncHandle(); 
    UnlockDisplay(dpy); 
    memcpy((char *)ret,&(rep.data),sizeof(XETrapGetAvailRep)); 
    return(status);
}

/* should not be called directly by clients */
#ifdef FUNCTION_PROTOS
static int XEConfigRequest(XETC *tc)
#else
static int XEConfigRequest(tc)
    XETC *tc;
#endif
{   /* protocol changed between V3.1 and V3.2! */
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrapConfig = tc->extOpcode;
    xXTrapConfigReq *reqptr;
    if (tc->protocol == 31)
    {   /* hack to allocate the old request length */
        GetOldReq(XTrapConfig,reqptr,276);
    }
    else
    {
        GetReq(XTrapConfig,reqptr);
    }
    reqptr->minor_opcode = XETrap_Config;

    memcpy((char *)reqptr->config_flags_valid,
        (char *)tc->values.v.flags.valid,4);
    memcpy((char *)reqptr->config_flags_data,
        (char *)tc->values.v.flags.data,4);
    memcpy((char *)reqptr->config_flags_req,
        (char *)tc->values.v.flags.req,XETrapMaxRequest);
    memcpy((char *)reqptr->config_flags_event,
        (char *)tc->values.v.flags.event,XETrapMaxEvent);
    reqptr->config_max_pkt_size=tc->values.v.max_pkt_size;
    reqptr->config_cmd_key=tc->values.v.cmd_key;

    XFlush(dpy);
    SyncHandle();
    tc->dirty = 0L; /* Configuration is no longer dirty */
    return(status);
}

/* Flush out any pending configuration */
#ifdef FUNCTION_PROTOS
int XEFlushConfig(XETC *tc)
#else
int XEFlushConfig(tc)
    XETC *tc;
#endif
{
    return((tc->dirty) ? XEConfigRequest(tc) : True);
}
#ifdef FUNCTION_PROTOS
int XEResetRequest(XETC *tc)
#else
int XEResetRequest(tc)
    XETC *tc;
#endif
{
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrap = tc->extOpcode;
    xXTrapReq *reqptr;
    status = XEFlushConfig(tc); /* Flushout any pending configuration first */
    if (status == True)
    {
        GetReq(XTrap,reqptr);
        reqptr->minor_opcode = XETrap_Reset;
        XFlush(dpy);
        SyncHandle();
    }
    return(status);
}


#ifdef FUNCTION_PROTOS
int XEGetLastInpTimeRequest(XETC *tc, XETrapGetLastInpTimeRep *ret) 
#else
int XEGetLastInpTimeRequest(tc,ret) 
    XETC *tc;
    XETrapGetLastInpTimeRep *ret;
#endif
{   /* this was broken in V3.1! */
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrap = tc->extOpcode;
    xXTrapReq *reqptr;
    xXTrapGetLITimReply rep; 
    int numlongs = (SIZEOF(xXTrapGetLITimReply) -
        SIZEOF(xReply) + SIZEOF(CARD32) - 1) / SIZEOF(CARD32);
    LockDisplay(dpy); 
    GetReq(XTrap,reqptr);
    reqptr->minor_opcode = XETrap_GetLastInpTime;
    status = _XReply(dpy,(xReply *)&rep,numlongs,xTrue); 
    SyncHandle(); 
    UnlockDisplay(dpy); 

    ret->last_time=rep.data_last_time;

    return(status);
}

#ifdef FUNCTION_PROTOS
int XEStartTrapRequest(XETC *tc)
#else
int XEStartTrapRequest(tc)
    XETC *tc;
#endif
{ 
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrap = tc->extOpcode;
    xXTrapReq *reqptr;
    status = XEFlushConfig(tc); /* Flushout any pending configuration first */
    if (status == True)
    {
        /* Add our event handler for the XLib transport */
        status = XETrapSetEventHandler(tc, XETrapData, XETrapDispatchXLib);
        GetReq(XTrap,reqptr);
        reqptr->minor_opcode = XETrap_StartTrap;
        XFlush(dpy);
        SyncHandle(); 
        BitTrue(tc->values.tc_flags, XETCTrapActive);
    }
    return(status);
}
#ifdef FUNCTION_PROTOS
int XEStopTrapRequest(XETC *tc) 
#else
int XEStopTrapRequest(tc) 
    XETC *tc;
#endif
{ 
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrap = tc->extOpcode;
    xXTrapReq *reqptr;
    status = XEFlushConfig(tc); /* Flushout any pending configuration first */
    if (status == True)
    {
        GetReq(XTrap,reqptr);
        reqptr->minor_opcode = XETrap_StopTrap;
        XFlush(dpy);
        SyncHandle(); 
        BitFalse(tc->values.tc_flags, XETCTrapActive);
        /* Remove our event handler for the XLib transport */
        status = XETrapSetEventHandler(tc, XETrapData, NULL);
    }

    return(status);
}

#ifndef _XINPUT
#ifdef FUNCTION_PROTOS
int XESimulateXEventRequest(XETC *tc, CARD8 type, CARD8 detail,
    CARD16 x, CARD16 y, CARD8 screen) 
#else
int XESimulateXEventRequest(tc,type,detail,x,y,screen) 
    XETC  *tc;
    CARD8 type;
    CARD8 detail;
    CARD16 x,y;
    CARD8 screen;
#endif
{
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrapInput = tc->extOpcode;
    xXTrapInputReq *reqptr;
    status = XEFlushConfig(tc); /* Flushout any pending configuration first */
    if (status == True)
    {   /* write out the input event */
        GetReq(XTrapInput,reqptr);
        reqptr->minor_opcode = XETrap_SimulateXEvent;
        reqptr->input.type   = type;
        reqptr->input.detail = detail;
        reqptr->input.x      = x;
        reqptr->input.y      = y;
        reqptr->input.screen = screen;
        XFlush(dpy);
    }
    return(status);
}
#endif
#ifdef FUNCTION_PROTOS
int XEGetCurrentRequest(XETC *tc, XETrapGetCurRep *ret) 
#else
int XEGetCurrentRequest(tc, ret) 
    XETC *tc;
    XETrapGetCurRep *ret;
#endif
{ 
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrap = tc->extOpcode;
    xXTrapReq *reqptr;
    xXTrapGetCurReply rep; 
    int numlongs = (SIZEOF(xXTrapGetCurReply) - 
        SIZEOF(xReply) + SIZEOF(CARD32) -1 ) / SIZEOF(CARD32);
    status = XEFlushConfig(tc); /* Flushout any pending configuration first */
    if (status == True)
    {
        LockDisplay(dpy); 
        GetReq(XTrap,reqptr);
        reqptr->minor_opcode = XETrap_GetCurrent;
        /* to support comm. w/ V3.1 extensions */
        if (tc->protocol == 31)
        {
            char tmp[284];  /* need space for the big *old* reply */
            numlongs = (284-sizeof(xReply)+sizeof(long)-1)/sizeof(long);
            status = _XReply(dpy,(xReply *)tmp,numlongs,xTrue);
            memcpy(&rep,tmp,sizeof(rep));   /* move just what's needed */
        }
        else
        {
            status = _XReply(dpy,(xReply *)&rep,numlongs,xTrue); 
        }
        SyncHandle();
        UnlockDisplay(dpy); 

    memcpy((char *)ret->state_flags,rep.data_state_flags,2);
    memcpy((char *)ret->config.flags.valid,rep.data_config_flags_valid,4);
    memcpy((char *)ret->config.flags.data,rep.data_config_flags_data,4);
    memcpy((char *)ret->config.flags.req,rep.data_config_flags_req,
        XETrapMaxRequest);
    memcpy((char *)ret->config.flags.event,rep.data_config_flags_event,
        XETrapMaxEvent);
    ret->config.max_pkt_size=rep.data_config_max_pkt_size;
    ret->config.cmd_key=rep.data_config_cmd_key;

    }
    return(status);
}

#ifdef FUNCTION_PROTOS
int XEGetStatisticsRequest(XETC *tc, XETrapGetStatsRep *ret) 
#else
int XEGetStatisticsRequest(tc,ret) 
    XETC *tc;
    XETrapGetStatsRep *ret;
#endif
{ 
    int status = True;
    Display *dpy = tc->dpy;
    CARD32 X_XTrap = tc->extOpcode;
    xXTrapReq *reqptr;
    xXTrapGetStatsReply rep; 
    status = XEFlushConfig(tc); /* Flushout any pending configuration first */
    if (status == True)
    {
        LockDisplay(dpy); 
        GetReq(XTrap,reqptr);
        reqptr->minor_opcode = XETrap_GetStatistics;
        /* to support comm. w/ V3.1 extensions */
#ifndef CRAY
        if (tc->protocol == 31)
        {   /* this is the way we used to do it which breaks Cray's */
#ifndef VECTORED_EVENTS
            int numlongs = (1060-sizeof(xReply)+sizeof(long)-1)/sizeof(long);
#else
            int numlongs = (1544-sizeof(xReply)+sizeof(long)-1)/sizeof(long);
#endif
            status = _XReply(dpy,(xReply *)&rep,numlongs,xTrue); 
            if (status == True)
            {   /* need to shift it back into the data struct */
                xXTrapGetStatsReply tmp; 
                tmp = rep; 
                memcpy(&(rep.data),&(tmp.pad0), sizeof(rep.data));
            }
        }
        else
#endif /* CRAY */
        {   /* this is the way we do it for V3.2 */
            int numbytes = SIZEOF(xXTrapGetStatsReply) - SIZEOF(xReply);
            status = _XReply(dpy, (xReply *)&rep, 0, xFalse); 
            if (status == True)
            {
                /*status = _XRead32(dpy, (long *)&rep.data, numbytes); */
                _XRead32(dpy, (long *)&rep.data, numbytes);
            }
        }
        SyncHandle(); 
        UnlockDisplay(dpy); 
        memcpy(ret,&(rep.data),sizeof(XETrapGetStatsRep)); 
    }
    return(status);
}
