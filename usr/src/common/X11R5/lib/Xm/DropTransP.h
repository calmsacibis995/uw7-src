#pragma ident	"@(#)m1.2libs:Xm/DropTransP.h	1.2"
/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.3
*/ 
/*   $RCSfile$ $Revision$ $Date$ */
/*
*  (c) Copyright 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */

#ifndef _XmDropTransferP_h
#define _XmDropTransferP_h

#include <Xm/DropTrans.h>
#include <Xm/XmP.h>


#ifdef __cplusplus
extern "C" {
#endif

/*  DropTransfer class structure  */

#ifdef _NO_PROTO
typedef Widget (*XmDropTransferStartTransferProc)();
typedef void (*XmDropTransferAddTransferProc)();
#else
typedef Widget (*XmDropTransferStartTransferProc)(Widget,
	ArgList, Cardinal);
typedef void (*XmDropTransferAddTransferProc)(Widget,
	XmDropTransferEntry, Cardinal);
#endif

typedef struct _XmDropTransferClassPart
{
	XmDropTransferStartTransferProc	start_drop_transfer;
	XmDropTransferAddTransferProc	add_drop_transfer;
	XtPointer extension;
} XmDropTransferClassPart;

/*  Full class record declaration */

typedef struct _XmDropTransferClassRec
{
   ObjectClassPart        object_class;
   XmDropTransferClassPart dropTransfer_class;
} XmDropTransferClassRec;

externalref XmDropTransferClassRec xmDropTransferClassRec;


typedef struct _XmDropTransferListRec {
	XmDropTransferEntry	transfer_list;
	Cardinal		num_transfers;
} XmDropTransferListRec, * XmDropTransferList;


/*  The DropTransfer instance record  */

typedef struct _XmDropTransferPart
{
    XmDropTransferEntry		drop_transfers;
    Cardinal			num_drop_transfers;
    Atom			selection;
    Widget			dragContext;
    Time			timestamp;
    Boolean			incremental;
    Window			source_window;
    unsigned int		tag;
    XtSelectionCallbackProc 	transfer_callback;
    unsigned char		transfer_status;

    Atom 			motif_drop_atom;
    
    XmDropTransferList		drop_transfer_lists;
    Cardinal			num_drop_transfer_lists;
    Cardinal			cur_drop_transfer_list;
    Cardinal			cur_xfer;
    Atom *			cur_targets;
    XtPointer *			cur_client_data;
} XmDropTransferPart;

/*  Full instance record declaration  */

typedef struct _XmDropTransferRec
{
	ObjectPart	object;
	XmDropTransferPart dropTransfer;
} XmDropTransferRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#if defined(__cplusplus) || defined(c_plusplus)
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmDropTransferP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
