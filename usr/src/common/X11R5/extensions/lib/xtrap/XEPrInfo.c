#ident	"@(#)r5extensions:lib/xtrap/XEPrInfo.c	1.1"

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
#include "Xos.h"
#include "Xlib.h"
#include "xtraplib.h"
#include "xtraplibp.h"

#ifndef lint
static char SCCSID[] = "@(#)XEPrintInfo.c	1.15 - 90/09/18  ";
static char RCSID[] = "$Header$";
#endif

#ifndef TRUE
# define TRUE 1L
#endif
#ifndef FALSE
# define FALSE 0L
#endif

#ifdef FUNCTION_PROTOS
void XEPrintRelease( FILE *ofp, XETrapGetAvailRep *pavail)
#else
void XEPrintRelease(ofp, pavail)
    FILE *ofp;
    XETrapGetAvailRep *pavail;
#endif
{

    fprintf(ofp,"\tRelease:   %d.%d-%d\n", XETrapGetAvailRelease(pavail),
        XETrapGetAvailVersion(pavail), XETrapGetAvailRevision(pavail));
}
#ifdef FUNCTION_PROTOS
void XEPrintTkRelease( FILE *ofp, XETC *tc)
#else
void XEPrintTkRelease(ofp, tc)
    FILE *ofp;
    XETC *tc;
#endif
{
    fprintf(ofp,"\tRelease:   %d.%d-%d\n", XEGetRelease(tc), XEGetVersion(tc),
        XEGetRevision(tc));
}

#ifdef FUNCTION_PROTOS
void XEPrintPlatform( FILE *ofp, XETrapGetAvailRep *pavail)
#else
void XEPrintPlatform(ofp, pavail)
    FILE *ofp;
    XETrapGetAvailRep *pavail;
#endif
{
    fprintf(ofp,"\tPlatform:  %s (0x%02x)\n",
        XEPlatformIDToString(XETrapGetAvailPFIdent(pavail)), 
        XETrapGetAvailPFIdent(pavail));
}

#ifdef FUNCTION_PROTOS
void XEPrintAvailFlags( FILE *ofp, XETrapGetAvailRep *pavail)
#else
void XEPrintAvailFlags(ofp, pavail)
    FILE *ofp;
    XETrapGetAvailRep *pavail;
#endif
{

    CARD8 f[4L];

    XETrapGetAvailFlags(pavail,f);
    fprintf(ofp,"\tFlags: ");
    XETrapGetAvailFlagTimestamp(pavail) && fputs("Timestamps ", ofp);
    XETrapGetAvailFlagCmd(pavail) && fputs("CmdKey ", ofp);
    XETrapGetAvailFlagCmdKeyMod(pavail) && fputs("CmdKeyMod ", ofp);
    XETrapGetAvailFlagRequest(pavail) && fputs("Requests ", ofp);
    XETrapGetAvailFlagEvent(pavail) && fputs("Events ", ofp);
    XETrapGetAvailFlagMaxPacket(pavail) && fputs("MaxPkt ", ofp);
    XETrapGetAvailFlagStatistics(pavail) && fputs("Statistics ", ofp);
    XETrapGetAvailFlagWinXY(pavail) && fputs("WinXY ", ofp);
    XETrapGetAvailFlagCursor(pavail) && fputs("Cursor ", ofp);
    XETrapGetAvailFlagXInput(pavail) && fputs("XInput ", ofp);
    XETrapGetAvailFlagVecEvt(pavail) && fputs("Vect_Evnts ", ofp);
    XETrapGetAvailFlagColorReplies(pavail) && fputs("ColorRep ", ofp);
    XETrapGetAvailFlagGrabServer(pavail) && fputs("GrabServer ", ofp);
    fprintf(ofp," (0x%02x%02x%02x%02x)\n", f[0], f[1], f[2], f[3]);
}

#ifdef FUNCTION_PROTOS
void XEPrintAvailPktSz( FILE *ofp, XETrapGetAvailRep *pavail)
#else
void XEPrintAvailPktSz(ofp, pavail)
    FILE *ofp;
    XETrapGetAvailRep *pavail;
#endif
{

    fprintf(ofp,"\tMax Packet Size: %d\n", XETrapGetAvailMaxPktSize(pavail));
}
#ifdef FUNCTION_PROTOS
void XEPrintStateFlags( FILE *ofp, XETrapGetCurRep *pcur)
#else
void XEPrintStateFlags(ofp, pcur)
    FILE *ofp;
    XETrapGetCurRep *pcur;
#endif
{

    CARD8   f[2];
    XETrapGetCurSFlags(pcur, f);
    fputs("\tFlags: ",ofp); 
    (BitIsTrue(f,XETrapTrapActive)) && fputs("I/O Active ", ofp);
    fprintf(ofp," (0x%02x%02x)\n", f[0], f[1]);    
}

#ifdef FUNCTION_PROTOS
void XEPrintMajOpcode( FILE *ofp, XETrapGetAvailRep *pavail)
#else
void XEPrintMajOpcode(ofp, pavail)
    FILE *ofp;
    XETrapGetAvailRep *pavail;
#endif
{

    fprintf(ofp,"\tMajor Opcode:  %d\n", XETrapGetAvailOpCode(pavail));
}
#ifdef FUNCTION_PROTOS
void XEPrintCurXY( FILE *ofp, XETrapGetAvailRep *pavail)
#else
void XEPrintCurXY(ofp, pavail)
    FILE *ofp;
    XETrapGetAvailRep *pavail;
#endif
{

    fprintf(ofp,"\tCurrent (x,y):  (%d,%d)\n", XETrapGetCurX(pavail), 
        XETrapGetCurY(pavail));
}

#ifdef FUNCTION_PROTOS
void XEPrintTkFlags( FILE *ofp, XETC *tc)
#else
void XEPrintTkFlags(ofp, tc)
    FILE *ofp;
    XETC *tc;
#endif
{

    CARD8   f[2];
    XETrapGetTCLFlags(tc, f);
    fputs("\tFlags: ",ofp); 
    (XETrapGetTCFlagDeltaTimes(tc)) && fputs("Delta Times ", ofp);
    (XETrapGetTCFlagTrapActive(tc)) && fputs("Trap Active ", ofp);
    fprintf(ofp," (0x%02x%02x)\n", f[0], f[1]);    
}

#ifdef FUNCTION_PROTOS
void XEPrintLastTime( FILE *ofp, XETC *tc)
#else
void XEPrintLastTime(ofp, tc)
    FILE *ofp;
    XETC *tc;
#endif
{

    fprintf(ofp,"\tLast Relative Time:  %d\n", XETrapGetTCTime(tc));
}

#ifdef FUNCTION_PROTOS
void XEPrintCfgFlags( FILE *ofp, XETrapGetCurRep *pcur)
#else
void XEPrintCfgFlags(ofp, pcur)
    FILE *ofp;
    XETrapGetCurRep *pcur;
#endif
{

    CARD8 f[4L];

    XETrapGetCurCFlags(pcur,data,f);
    fprintf(ofp,"\tFlags: ");
    XETrapGetCurFlagTimestamp(pcur,data)
        && fputs("Timestamps ", ofp);
    XETrapGetCurFlagCmd(pcur,data) && fputs("CmdKey ", ofp);
    XETrapGetCurFlagCmdKeyMod(pcur,data)
        && fputs("CmdKeyMod ", ofp);
    XETrapGetCurFlagRequest(pcur,data)
        && fputs("Requests ", ofp);
    XETrapGetCurFlagEvent(pcur,data) && fputs("Events ", ofp);
    XETrapGetCurFlagMaxPacket(pcur,data)
        && fputs("MaxPkt ", ofp);
    XETrapGetCurFlagStatistics(pcur,data)
        && fputs("Statistics ", ofp);
    XETrapGetCurFlagWinXY(pcur,data) && fputs("WinXY ", ofp);
    XETrapGetCurFlagCursor(pcur,data) && fputs("Cursor ", ofp);
    XETrapGetCurFlagXInput(pcur,data) && fputs("XInput ", ofp);
    XETrapGetCurFlagColorReplies(pcur,data) && fputs("ColorReplies ", ofp);
    XETrapGetCurFlagGrabServer(pcur,data) && fputs("GrabServer ", ofp);
    fprintf(ofp," (0x%02x%02x%02x%02x)\n", f[0], f[1], f[2], f[3]);
}

#ifdef FUNCTION_PROTOS
void XEPrintRequests( FILE *ofp, XETrapGetCurRep *pcur)
#else
void XEPrintRequests(ofp, pcur)
    FILE *ofp;
    XETrapGetCurRep *pcur;
#endif
{

    long i;
    fprintf(ofp,"\tX Requests:  ");
    for (i=0L; i<=XETrapMaxRequest-1; i++)
    {   /* Not using the macro cause we're doing things
         * a byte at a time rather than a bit.
         */
        fprintf(ofp,"%02x ", pcur->config.flags.req[i]);
        if ((i+1L)%4L == 0L)
        { 
            fprintf(ofp,"  "); 
        }
        if ((i+1L)%16L == 0L)
        { 
            fprintf(ofp,"\n\t\t     "); 
        }
    }
    fprintf(ofp,"\n");
}

#ifdef FUNCTION_PROTOS
void XEPrintEvents( FILE *ofp, XETrapGetCurRep *pcur)
#else
void XEPrintEvents(ofp, pcur)
    FILE *ofp;
    XETrapGetCurRep *pcur;
#endif
{

    int i;
    fprintf(ofp,"\tX Events:  ");
    for (i=0L; i<XETrapMaxEvent; i++)
    {   /* Not using the macro cause we're doing things
         * a byte at a time rather than a bit.
         */
        fprintf(ofp,"%02x ", pcur->config.flags.event[i]);
        if ((i+1L)%4L == 0L)
        { 
            fprintf(ofp,"  "); 
        }
        if ((i+1L)%16L == 0L)
        { 
            fprintf(ofp,"\n\t\t     "); 
        }
    }
    fprintf(ofp,"\n");
}

#ifdef FUNCTION_PROTOS
void XEPrintCurPktSz( FILE *ofp, XETrapGetCurRep *pcur)
#else
void XEPrintCurPktSz(ofp, pcur)
    FILE *ofp;
    XETrapGetCurRep *pcur;
#endif
{

    fprintf(ofp,"\tMax Packet Size: %d\n", XETrapGetCurMaxPktSize(pcur));
}

#ifdef FUNCTION_PROTOS
void XEPrintCmdKey( FILE *ofp, XETrapGetCurRep *pcur)
#else
void XEPrintCmdKey(ofp, pcur)
    FILE *ofp;
    XETrapGetCurRep *pcur;
#endif
{

    fprintf(ofp,"\tcmd_key: 0x%02x\n", XETrapGetCurCmdKey(pcur));
}

#ifdef FUNCTION_PROTOS
void XEPrintEvtStats( FILE *ofp, XETrapGetStatsRep *pstats)
#else
void XEPrintEvtStats(ofp, pstats)
    FILE *ofp;
    XETrapGetStatsRep *pstats;
#endif
{

    int i;
    fprintf(ofp,"\tX Events:\n");
    for (i=0; i<XETrapCoreEvents; i++)
    {   
        if (XETrapGetStatsEvt(pstats,i))
        {
            fprintf(ofp,"\t   %-20s :  %d\n", XEEventIDToString(i),
                XETrapGetStatsEvt(pstats,i));
        }
    }
    fprintf(ofp,"\n");
}

#ifdef FUNCTION_PROTOS
void XEPrintReqStats( FILE *ofp, XETrapGetStatsRep *pstats)
#else
void XEPrintReqStats(ofp, pstats)
    FILE *ofp;
    XETrapGetStatsRep *pstats;
#endif
{

    int i;
    fprintf(ofp,"\tX Requests:\n");
    for (i=0L; i<256L; i++)
    {   
        if (XETrapGetStatsReq(pstats,i))
        {
            fprintf(ofp,"\t   %-20s :  %d\n", XERequestIDToString(i),
                XETrapGetStatsReq(pstats,i));
        }
    }
    fprintf(ofp,"\n");
}


#ifdef FUNCTION_PROTOS
void XEPrintAvail( FILE *ofp, XETrapGetAvailRep *pavail)
#else
void XEPrintAvail(ofp, pavail)
    FILE *ofp;
    XETrapGetAvailRep *pavail;
#endif
{

    fprintf(ofp,"Available Information:\n");
    XEPrintRelease(ofp, pavail);
    XEPrintPlatform(ofp, pavail);
    XEPrintMajOpcode(ofp, pavail);
    XEPrintAvailFlags(ofp, pavail);
    XEPrintAvailPktSz(ofp, pavail);
    XEPrintCurXY(ofp, pavail);
    return;
}

#ifdef FUNCTION_PROTOS
void XEPrintTkState( FILE *ofp, XETC *tc)
#else
void XEPrintTkState(ofp, tc)
    FILE *ofp;
    XETC *tc;
#endif
{

    fprintf(ofp,"Toolkit State:\n");
    XEPrintTkFlags(ofp, tc);
    XEPrintLastTime(ofp, tc);
    XEPrintTkRelease(ofp, tc);
}

#ifdef FUNCTION_PROTOS
void XEPrintCurrent( FILE *ofp, XETrapGetCurRep *pcur)
#else
void XEPrintCurrent(ofp, pcur)
    FILE *ofp;
    XETrapGetCurRep *pcur;
#endif
{

    fprintf(ofp,"Current State:\n");
    XEPrintStateFlags(ofp, pcur);
    fprintf(ofp,"Current Config:\n");
    XEPrintCfgFlags(ofp, pcur);
    XEPrintRequests(ofp, pcur);
    XEPrintEvents(ofp, pcur);
    XEPrintCurPktSz(ofp, pcur);
    XEPrintCmdKey(ofp, pcur);
}

#ifdef FUNCTION_PROTOS
void XEPrintStatistics( FILE *ofp, XETrapGetStatsRep *pstats)
#else
void XEPrintStatistics(ofp, pstats)
    FILE *ofp;
    XETrapGetStatsRep *pstats;
#endif
{

    fprintf(ofp,"Statistics:\n");
    XEPrintEvtStats(ofp, pstats);
    XEPrintReqStats(ofp, pstats);
}

