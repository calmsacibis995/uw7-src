#ifndef __dndutil_h__
#define __dndutil_h__

#pragma ident	"@(#)libMDtI:DnDUtil.h	1.3"

/* Atoms used in DnD transactions.  Note that XA_STRING is in Xatom.h and
 * that "MULTIPLE" and "TIMESTAMP" are handled by the Xt.
 */
#define OL_XA_TARGETS(d)        XInternAtom(d, "TARGETS", False)

#define OL_XA_TEXT(d)           XInternAtom(d, "TEXT", False)
#define OL_XA_COMPOUND_TEXT(d)  XInternAtom(d, "COMPOUND_TEXT", False)
#define OL_XA_FILE_NAME(d)      XInternAtom(d, "FILE_NAME", False)
#define OL_XA_HOST_NAME(d)      XInternAtom(d, "HOST_NAME", False)
#define OL_XA_DELETE(d)         XInternAtom(d, "DELETE", False)

        /* The following atoms are for Desktop DnD              */
        /* Subject to change                                    */
#define OL_USL_ITEM(d)          XInternAtom(d, "_USL_ITEM", False)
#define OL_USL_NUM_ITEMS(d)     XInternAtom(d, "_USL_NUM_ITEMS", False)

/*
 * This structure is used as for the DnD destination. 
 */
typedef struct {
	Widget		widget;		/* widget */
	char		**files;	/* accumulated list of file names */
	DtAttrs		attrs;		/* attributes */
	Boolean		error;		/* error */
	int		nitems;		/* # of source items */
	Time		timestamp;	/* timestamp */
} DmDnDDstInfo, *DmDnDDstInfoPtr;

/*
 * This structure is used as the DnD source.
 */
typedef struct {
	Widget		widget;		/* owner widget */
	char		**files;	/* accumulated list of file names */
	DtAttrs		attrs;		/* attributes */
	char		**fp;		/* current file name */
	Atom		selection;	/* DnD transaction ID */
	unsigned char	opcode;		/* operation type */
} DmDnDSrcInfo, *DmDnDSrcInfoPtr;

#ifdef __cplusplus
extern "C" {
#endif

extern void DmDnDGetOneName(Widget w, XtPointer client_data, Atom *selection,
	Atom *type, XtPointer value, unsigned long *length, int *format);
extern void DmDnDGetNumItems(Widget w, XtPointer client_data, Atom *selection,
	Atom *type, XtPointer value, unsigned long *length, int *format);
extern DmDnDDstInfoPtr DmDnDGetFileNames(Widget w, DtAttrs attrs,
	void (*transferProc)(), XtPointer client_data, XtPointer call_data);
extern Boolean DmDnDConvertSelectionProc(Widget w, Atom *selection,
	Atom *target, Atom *type_rtn, XtPointer *val_rtn,
	unsigned long *length_rtn, int *format_rtn);
extern void DmDnDFinishProc(Widget w, XtPointer client_data,
	XtPointer call_data);
extern DmDnDSrcInfoPtr DmDnDNewTransaction(Widget w, char **files,
	DtAttrs attrs, Atom selection, unsigned char operation,
	XtCallbackProc deleteProc, XtCallbackProc finishProc,
	XtPointer client_data);

#ifdef __cplusplus
}
#endif

#endif /* __dndutil_h */
