#pragma ident	"@(#)m1.2libs:Xm/CutPaste.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
#ifndef _XmCutPaste_h
#define _XmCutPaste_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* XmClipboard return status definitions */

#define XmClipboardFail         0
#define XmClipboardSuccess      1
#define XmClipboardTruncate     2
#define XmClipboardLocked       4
#define XmClipboardBadFormat    5
#define XmClipboardNoData       6

/* XmClipboard pre-1.2 definitions */

#define ClipboardFail     	0
#define ClipboardSuccess  	1 
#define ClipboardTruncate 	2
#define ClipboardLocked   	4
#define ClipboardBadFormat   	5
#define ClipboardNoData   	6

typedef struct {
    long DataId;
    long PrivateId;
} XmClipboardPendingRec, *XmClipboardPendingList;

#ifdef _NO_PROTO
typedef void (*XmCutPasteProc)() ;
typedef void (*VoidProc)() ;
#else
typedef void (*XmCutPasteProc)( Widget w, long * data_id, long * private_id,
							        int * reason) ;
typedef void (*VoidProc)( Widget w, int * data_id, int * private_id,
							        int * reason) ;
#endif


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern int XmClipboardBeginCopy() ;
extern int XmClipboardStartCopy() ;
extern int XmClipboardCopy() ;
extern int XmClipboardEndCopy() ;
extern int XmClipboardCancelCopy() ;
extern int XmClipboardWithdrawFormat() ;
extern int XmClipboardCopyByName() ;
extern int XmClipboardUndoCopy() ;
extern int XmClipboardLock() ;
extern int XmClipboardUnlock() ;
extern int XmClipboardStartRetrieve() ;
extern int XmClipboardEndRetrieve() ;
extern int XmClipboardRetrieve() ;
extern int XmClipboardInquireCount() ;
extern int XmClipboardInquireFormat() ;
extern int XmClipboardInquireLength() ;
extern int XmClipboardInquirePendingItems() ;
extern int XmClipboardRegisterFormat() ;

#else

extern int XmClipboardBeginCopy( 
                        Display *display,
                        Window window,
                        XmString label,
                        Widget widget,
                        VoidProc callback,
                        long *itemid) ;
extern int XmClipboardStartCopy( 
                        Display *display,
                        Window window,
                        XmString label,
                        Time timestamp,
                        Widget widget,
                        XmCutPasteProc callback,
                        long *itemid) ;
extern int XmClipboardCopy( 
                        Display *display,
                        Window window,
                        long itemid,
                        char *format,
                        XtPointer buffer,
                        unsigned long length,
                        long private_id,
                        long *dataid) ;
extern int XmClipboardEndCopy( 
                        Display *display,
                        Window window,
                        long itemid) ;
extern int XmClipboardCancelCopy( 
                        Display *display,
                        Window window,
                        long itemid) ;
extern int XmClipboardWithdrawFormat( 
                        Display *display,
                        Window window,
                        long data) ;
extern int XmClipboardCopyByName( 
                        Display *display,
                        Window window,
                        long data,
                        XtPointer buffer,
                        unsigned long length,
                        long private_id) ;
extern int XmClipboardUndoCopy( 
                        Display *display,
                        Window window) ;
extern int XmClipboardLock( 
                        Display *display,
                        Window window) ;
extern int XmClipboardUnlock( 
                        Display *display,
                        Window window,
#if NeedWidePrototypes
                        int all_levels) ;
#else
                        Boolean all_levels) ;
#endif /* NeedWidePrototypes */
extern int XmClipboardStartRetrieve( 
                        Display *display,
                        Window window,
                        Time timestamp) ;
extern int XmClipboardEndRetrieve( 
                        Display *display,
                        Window window) ;
extern int XmClipboardRetrieve( 
                        Display *display,
                        Window window,
                        char *format,
                        XtPointer buffer,
                        unsigned long length,
                        unsigned long *outlength,
                        long *private_id) ;
extern int XmClipboardInquireCount( 
                        Display *display,
                        Window window,
                        int *count,
                        unsigned long *maxlength) ;
extern int XmClipboardInquireFormat( 
                        Display *display,
                        Window window,
                        int n,
                        XtPointer buffer,
                        unsigned long bufferlength,
                        unsigned long *outlength) ;
extern int XmClipboardInquireLength( 
                        Display *display,
                        Window window,
                        char *format,
                        unsigned long *length) ;
extern int XmClipboardInquirePendingItems( 
                        Display *display,
                        Window window,
                        char *format,
                        XmClipboardPendingList *list,
                        unsigned long *count) ;
extern int XmClipboardRegisterFormat( 
                        Display *display,
                        char *format_name,
                        int format_length) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#if defined(__cplusplus) || defined(c_plusplus)
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmCutPaste_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
