#ifndef __DnDUtilP_h_
#define __DnDUtilP_h_

#pragma ident	"@(#)libMDtI:DnDUtilP.h	1.4"

#include "WidePosDef.h"
#include "DnDUtil.h"

/* DnD private structures */

typedef struct {
	/*
	 * These fields must match the fields in DmDnDDstInfo structure.
	 */
	Widget		widget;		/* widget */
	char		**files;	/* accumulated list of file names */
	DtAttrs		attrs;		/* attributes */
	Boolean		error;		/* error */
	int		nitems;		/* # of source items */
	Time		timestamp;	/* timestamp */

	void		(*proc)();	/* application callback */
	int		nreceived;	/* # of items received */
	int		nreplies;	/* # of replies */
	XtPointer	client_data;	/* client_data for callback */
	Atom		*targets;
	XtPointer	*cd_list;
	unsigned char   operation;      /* operation type */
	XtPointer       items;
	Cardinal        item_index;
	WidePosition	x;		/* dest x */
	WidePosition	y;		/* dest y */
} _DmDnDDstInfo, *_DmDnDDstInfoPtr;

typedef struct {
	/*
	 * These fields must match the fields in DmDnDSrcInfo structure.
	 */
	Widget		widget;		/* owner widget */
	char		**files;	/* accumulated list of file names */
	DtAttrs		attrs;		/* attributes */
	char		**fp;		/* current file name */
	Atom		selection;	/* DnD transaction ID */
	unsigned char	opcode;		/* operation type */

	Boolean		(*del_proc)();	/* callback for XA_DELETE target */
	void		(*done_proc)();	/* callback for when DnD is done */
	XtPointer	client_data;	/* client_data for callback */
} _DmDnDSrcInfo, *_DmDnDSrcInfoPtr;

#endif /* __DnDUtilP_h_ */
