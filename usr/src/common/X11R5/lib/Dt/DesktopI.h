#ifndef __DesktopI_h__
#define __DesktopI_h__

#pragma ident	"@(#)Dt:DesktopI.h	1.13.1.1"

#include <Dt/Desktop.h>

/* application type id must be >= this */
#define DT_CACHE_BASE	10000

/* system type id for cached names in property list */
#define DT_CACHE_NAME	9999
#define DT_CACHE_QUEUE	9998
#define DT_CACHE_WIDGET	9997
#define DT_CACHE_DND	9996

/*
 * The following assumes that DtAttrs is a 32 bit integer.
 */
#define ATTR_LEN	10
#define ATTR_OVERHEAD	2

/* Drag&Drop internal structures */
typedef struct {
	/*
	 * These fields must match the fields in DtDnDInfo structure.
	 */
	char		**files;	/* accumulated list of file names */
	DtAttrs		attrs;		/* attributes */
	Boolean		error;		/* error */
	int		nitems;		/* # of source items */
	Time		timestamp;	/* timestamp of trigger message */
	Boolean		send_done;	/* send_done from trigger message */

	void		(*proc)();	/* application callback */
	int		nreceived;	/* # of items received */
	int		nreplies;	/* # of replies */
	XtPointer	client_data;	/* client_data for callback */
	Atom		*targets;
	XtPointer	*cd_list;
	Boolean		send_error;	/* Ok to Terminate DnD */
} Dt__DnDInfo, *Dt__DnDInfoPtr;

typedef struct {
	/*
	 * These fields must match the fields in DtDnDSend structure.
	 */
	char		**files;	/* accumulated list of file names */
	DtAttrs		attrs;		/* attributes */
	Widget		widget;		/* owner widget */
	Atom		selection;	/* DnD transaction ID */
	char		**fp;		/* current file name */
	Time		drop_timestamp;	/* timestamp of drop action */
	int		hint;		/* DnD hint */
	int		state;		/* transaction state */

	Boolean		(*del_proc)();	/* callback for deletion of item */
	void		(*state_proc)();/* callback for transaction states */
	XtPointer	client_data;	/* client_data for callback */
} Dt__DnDSend, *Dt__DnDSendPtr;

/* request protocol functions */
extern int Dt__DecodeFromString(
				char	*base,
				DtMsgInfo const *info,
				char	*str,
				char	**endptr);
extern char * Dt__StructToString(
				DtRequest *request,
				int *len,
				DtMsgInfo const *mp,
				int mlen);
extern char *Dt__GetCharProperty(Display * dpy, Window w, Atom property,
			         unsigned long *length);
extern void Dt__EnqueueCharProperty(Display * dpy, Window w, Atom atom,
				    char *data, int length);

/* miscellaneous functions */
extern char *Dt__strndup(char *str, int len);

#endif __DesktopI_h__
