#ifndef __DesktopP_h_
#define __DesktopP_h_

#pragma ident	"@(#)libMDtI:DesktopP.h	1.32.1.2"

#include <stdio.h>
#include <stdlib.h>		/* for realpath() */
#include <libgen.h>		/* for basename() and dirname() */
#include <limits.h>
#include <sys/types.h>		/* for time_t in DmContainerRec */
#include <Dt/Desktop.h>
#include <X11/Xlib.h>

#include "WidePosDef.h"

/* file types */
typedef enum {
    DM_FTYPE_DIR = 1,
    DM_FTYPE_EXEC,
    DM_FTYPE_DATA,
    DM_FTYPE_FIFO,
    DM_FTYPE_CHR,
    DM_FTYPE_BLK,
    DM_FTYPE_SEM,
    DM_FTYPE_SHD,
    DM_FTYPE_UNK,
} DmFileType;

/*
 * The DmGlyphRec structure is used to store pixmap information by
 * pixmap caching functions.
 */
typedef struct dmGlyphRec {
	char		*path;		/* filename */
	Pixmap		pix;
	Pixmap		mask;
	Dimension	width;
	Dimension	height;
	short		depth;
	short		count;		/* usage count */
} DmGlyphRec, *DmGlyphPtr;

/*
 * This structure stores info returned from stat(2) about each file, and
 * hangs off of the DmObjectRec.
 */
typedef struct dmFileInfo {
	mode_t		mode;
	nlink_t		nlink;
	uid_t		uid;
	gid_t		gid;
	off_t		size;
	time_t		mtime;
	ushort		fstype;
} DmFileInfo, *DmFileInfoPtr;

/*
 * The DmFclassRec structure stores the information about file class.
 * The information stored in this structure is common to both the internal
 * list of file classes and the list of file classes described in the
 * file database.
 */
typedef struct dmFclassRec {
	DtAttrs		attrs;
	DtPropList	plist;
	DmGlyphPtr	glyph;
	DmGlyphPtr	cursor;		/* This field is no longer used by the
					 * Motif dtm. However, it is still
					 * referenced by Printer_Setup and
					 * MoOLIT clients. Once, we clean up
					 * all the references, this field should
					 * be removed.
					 */
	void *		key;		/* ptr back to FnameKey or FmodeKey */
} DmFclassRec, *DmFclassPtr;

/*
 * Information about each open directory and other containers is stored 
 * in the DmContainerRec structure.
 */
typedef struct dmContainerRec {
    struct dmContainerRec *	next;
    char *			path;
    int				count;		/* usage count */
    struct dmObjectRec *	op;
    int				num_objs;	/* number of objects */
    DtAttrs			attrs;
    DtPropList			plist;		/* folder win properties */
    time_t			time_stamp;	/* "update" time */
    void *			data;		/* user specific data */
    DtCallbackList		cb_list;	/* for updating dependents */
    ushort			fstype;		/* file system type */
} DmContainerRec, *DmContainerPtr;




/*
 * Information about an object in a container is stored in the
 * DmObjectRec structure.
 */
typedef struct dmObjectRec {
				/* CAUTION: Items in Tree view depend on
				   the ordering of the following fields */
    DmContainerPtr	container;
    char *		name;
    DmFileType		ftype;
    DmFclassPtr		fcp;
    DtAttrs		attrs;
    DtPropList		plist;
				/* End of order-dependent fields */
    WidePosition	x;
    WidePosition	y;
    void *		objectdata;
    struct dmObjectRec *next;
} DmObjectRec, *DmObjectPtr;

/* reasons for container updates */
typedef enum{ 
    DM_CONTAINER_CONTENTS_CHANGED,
    DM_CONTAINER_DESTROYED,
    DM_CONTAINER_MOVED,
    DM_CONTAINER_SYNC,		/* sync folder view */
    DM_CONTAINER_OBJ_CHANGED, 
    DM_CONTAINER_ICON_CHANGED
} DmContainerCBReason;


/*
 * bit values for the attrs field of DmContainerFileChangedRec
 */
#define	DM_B_OBJ_BUSY_STATE 	1 << 0	/* busy status changed */
#define	DM_B_OBJ_NAME		1 << 1	/* file name changed */
#define DM_B_OBJ_TYPE		1 << 2	/* file reclassed or class updated */

typedef struct _DmContainerMovedRec{
    char 	*old_path;
    char 	*new_path;
}DmContainerMovedRec;

typedef struct _DmContainerChangedRec{
    int 	num_added;
    int		num_removed;
    DmObjectPtr *objs_added;
    DmObjectPtr *objs_removed;
}DmContainerChangedRec;

typedef struct _DmContainerObjChangedRec{
    DmObjectPtr	obj;
    char *old_name;
    char *new_name;
    DtAttrs attrs;	
}DmContainerObjChangedRec;

typedef union _DmContainerCBDetailRec{
    DmContainerMovedRec	moved;
    DmContainerChangedRec changed;
    DmContainerObjChangedRec obj_changed;
}DmContainerCBDetailRec;

typedef struct _DmContainerCallDataRec{
    DmContainerCBReason	reason;
    DmContainerPtr	cp;
    DmContainerCBDetailRec detail; 
} DmContainerCallDataRec, *DmContainerCallDataPtr;


/*
 * This structure is used by the generic DmPropertyEventHandler()
 */
typedef struct {
	Atom		prop_name;	/* queue name */
	DtMsgInfo const	*msgtypes;	/* list of msg types */
	int		(**proc_list)();/* list of procs */
	int		count;		/* # of msg types */
} DmPropEventData;

/* mapping between an int value and corresponding string */

#if defined(__cplusplus) || defined(c_plusplus)
typedef struct {
	char	* value;
	int	type;
} DmMapping;
#else
typedef struct {
	const char	* const value;
	const int	type;
} DmMapping;
#endif

/*
 * Bit values for the op field of DmMnemonicInfoRec.
 */
#define DM_B_MNE_NOOP			0
#define DM_B_MNE_ACTIVATE_BTN		(1L << 0)
#define DM_B_MNE_DEFAULT_ACTION		DM_B_MNE_ACTIVATE_BTN
#define DM_B_MNE_GET_FOCUS		(1L << 1)	
#define DM_B_MNE_ACTIVATE_CB		(1L << 2)
#define DM_B_MNE_KEEP_LOOKING		(1L << 3)	
#define DM_B_MNE_PASS_KEY		(1L << 4)

/*
 * The DmMnemonicInfoRec is used to store mnemonic information for an
 * object (widget/gadget). See DmRegisterMnemonic() for more info.
 */
typedef struct {
	Widget		w;	/* Widget/gadget id associates with this mne */
	unsigned long	op;	/* operation type when this mne is triggerred.
				 * The value is the bitwise inclusive OR of one
				 * or more of following flag bits:
				 *   DM_B_MNE_ACTIVATE_BTN	or
				 *   DM_B_MNE_DEFAULT_ACTION - invoke
				 *     XtCallCallbacks(w,
				 *	XmNactivateCallback, NULL *call_data*)
				 *   DM_B_MNE_ACTIVATE_CB - invoke
				 *     (*cb)(w, cd, NULL *call_data*)
				 *   DM_B_MNE_GET_FOCUS - invoke
				 *     XmProcessTraversal(w, XmTRAVERSE_CURRENT)
				 *   DM_B_MNE_KEEP_LOOKING - keep search the
				 *     rest of list when the current candidate
				 *     is either un-managed or insensitive.
				 *   DM_B_MNE_PASS_KEY - shall pass this
				 *     key stroke to application after
				 *     processing when the bit is set.
				 *
				 * Note that the implementation will check the
				 * bits in the following order:
				 *   DM_B_MNE_GET_FOCUS
				 *   DM_B_MNE_ACTIVATE_BTN
				 *   DM_B_MNE_ACTIVATE_CB
				 */
	unsigned char *	mne;	/* I18N mnemonic string */
	unsigned short	mne_len;/* strlen(mne) */
	XtCallbackProc	cb;	/* valid if DM_B_MNE_ACTIVATE_CB is set */
	XtPointer	cd;	/* valid if DB_B_MNE_ACTIVATE_CB is set */
} DmMnemonicInfoRec, *DmMnemonicInfo;

/* constants for caching types */
#define DM_CACHE_DND		1
#define DM_CACHE_PIXMAP		2
#define DM_CACHE_FOLDER		3
#define DM_CACHE_HELPFILE	4

/* options used in DmContainerRec */
#define DM_B_NO_INFO		(1 << 0)
#define DM_B_FLUSH_DATA		(1 << 1)
#define DM_B_INITED		(1 << 2)
#define DM_B_NO_LINK		(1 << 3)

/* options used in DmObjectRec */
#define DM_B_HIDDEN		(1 << 0)
#define DM_B_SHORTCUT		(1 << 1)
#define DM_B_SYMLINK		(1 << 2)
#define DM_B_NEWBORN		(1 << 3)

/* options for DmAddObjToIcontainer() and DmCreateIconContainer() */
#define DM_B_CALC_SIZE		(1 << 0)
#define DM_B_CALC_POS		(1 << 1)
#define DM_B_SPECIAL_NAME	(1 << 2)
#define DM_B_NO_INIT		(1 << 3)
#define DM_B_CHECK_DUP		(1 << 4)
#define DM_B_ADD_TO_END		(1 << 5)
#define DM_B_SHOW_DOT_DOT	(1 << 6)
#define DM_B_ALLOW_DUPLICATES	(1 << 7)
#define DM_B_WRAPPED_LABEL	(1 << 8)

/* options in DmFclassRec structure */
#define DM_B_VAR		(1 << 0)
#define DM_B_FREE		(1 << 1)

/* common property names */
#define ICONFILE		"_ICONFILE"
#define FILETYPE		"_FILETYPE"
#define FILEPATH		"_FILEPATH"
#define DFLTICONFILE		"_DFLTICONFILE"
#define OPENCMD			"_Open"
#define PRINTCMD		"_Print"
#define DROPCMD			"_DROP"

#define _DEFAULT_PRINTER	"_DEFAULT_PRINTER"

/* instance property names */
#define ICONLABEL		"_ICONLABEL"

/* size of the global arg list */
#define ARGLIST_SIZE	16

/* global variables */
extern char Dm__buffer[PATH_MAX];

/* FLH MORE - These belong in libDtI: (both Motif and non-Motif versions) */
#define ICON_PADDING	1
#define ICON_MARGIN	4	/* Margin (in points) around icons in pane */
#define INTER_ICON_PAD	3	/* Space between icons (in points) */
#define ICON_LABEL_GAP	2	/* Space between icon and label (in points) */


/* Define a value to indicate an "unspecified" position.  Some large-enough
   negative number is needed but not large enough to make it valid.
*/
#define UNSPECIFIED_POS -1000

/* MACROS */
#define DmMakePath(path, name)	Dm_MakePath(path, name, Dm__buffer)
#define Dm_ObjPath(obj, buf)	Dm_MakePath((obj)->container->path, \
					    (obj)->name, buf)
#define DmObjPath(obj)		Dm_ObjPath(obj, Dm__buffer)
#define DmMakeDir(path, name)	dirname(DmMakePath(path, name))
#define DmObjDir(obj)		dirname(DmObjPath(obj))

/* Convenience macros we all use somewhere */
#define DM_Max(x, y)		( ((x) > (y)) ? (x) : (y) )
#define DM_Min(x, y)		( ((x) < (y)) ? (x) : (y) )
#define DM_AssignMax(x, y)	if ((y) > (x)) x = (y)
#define DM_AssignMin(x, y)	if ((y) < (x)) x = (y)
     
/* function prototypes */

	/* 0: OK, 1: dlopen problem, 2: dlsym problem.
	 * DtiInitialize will call DtInitialize()
	 * if all Dt symbols are resolved!
	 *
	 * Note that only libMDtI.so has this function!
	 */

typedef char *	  (*Dts_RealPath_type)(const char *, char *);
typedef char *    (*Dts_BaseName_type)(char *);
typedef char *    (*Dts_DirName_type)(char *);

#define DtiInitialize(W)	Dti_Initialize(W,realpath,basename,dirname)
extern int	Dti_Initialize(Widget, Dts_RealPath_type,
					Dts_BaseName_type, Dts_DirName_type);

extern Pixmap	DmMaskPixmap(Widget w, DmGlyphPtr glyph);
extern int DmValueToType(char *value, DmMapping map[]);
extern char * DmTypeToValue(int type, DmMapping map[]);
extern char *	DmGetObjectName(DmObjectPtr op);
extern char *  DmGetObjectTitle(DmObjectPtr op);
extern DmGlyphPtr DmGetPixmap(Screen *screen, char *name);
extern DmGlyphPtr DmGetCursor(Screen *screen, char *name);
extern void	DmReleasePixmap(Screen *screen, DmGlyphPtr gp);
extern char *	Dm_MakePath(char * path, char * name, char * buf);
extern char *	Dm__expand_sh(char *, char * (*)(), XtPointer);
extern char *DmResolveSymLink(char *name, char **real_path,
				char **real_name);
extern char *Dm__ExpandObjProp(char *name, XtPointer client_data);

extern char	*DmGetObjProperty(DmObjectPtr op, char *name, DtAttrs *attrs);
extern void	DmSetObjProperty(DmObjectPtr op,
			      char *name,
			      char *value,
			      DtAttrs attrs);
extern DtPropPtr DmFindObjProperty(DmObjectPtr op, DtAttrs attrs);
extern DmFclassPtr DmNewFileClass(void *key);
extern int Dm__strnicmp(const char *str1, const char *str2, int len);
extern int Dm__stricmp(const char *str1, const char *str2);
extern char *CvtToRegularExpression(char *expression);
extern void DmSetIconPath(char *path);
extern DmGlyphPtr DmCreateBitmapFromData(Screen *screen,
					 char *name,
					 unsigned char *data,
					 unsigned int width,
					 unsigned int height);

extern int Dm__AddToObjList(DmContainerPtr, DmObjectPtr, DtAttrs);

extern Pixmap Dm__CreateIconMask(Screen *screen, DmGlyphPtr gp);
extern void DmRegisterReqProcs(Widget w, DmPropEventData *edp);
extern int DmDispatchRequest(Widget w, Atom selection, char *str);
extern Widget DtGetShellOfWidget(Widget w);
extern Cursor DtGetBusyCursor(Widget);
extern Cursor DtGetStandardCursor(Widget);
extern char *Dt__strndup(char *str, int len);
extern char *	Dm_DayOneName(const char * path, const char * login);


/* debugging macros */
#define CHKPT()		fprintf(stderr,"checkpoint in file %s line=%d\n", __FILE__, __LINE__)
#define MEMCHK()	{ char *__p__; CHKPT();__p__ = malloc(4); free(__p__); }
#define MEMCHECK(SIZE)	{ char *__p__; CHKPT();__p__ = malloc(SIZE); free(__p__); }

/* desktop administration routines, available for Finder, other clients */

/*	diagnostic flags for diskettes	*/

#define	DTAM_UNDIAGNOSED	0
#define	DTAM_S5_FILES		1
#define	DTAM_UFS_FILES		2
#define	DTAM_FS_TYPE		(1<<3)-1	/* up to 7 file system types */
#define	DTAM_BACKUP		1<<3
#define	DTAM_CPIO		2<<3
#define	DTAM_CPIO_BINARY	3<<3
#define	DTAM_CPIO_ODH_C		4<<3
#define	DTAM_TAR		5<<3
#define	DTAM_CUSTOM		6<<3	/* TAR with file in /etc/perms */
#define	DTAM_DOS_DISK		7<<3
#define	DTAM_UNFORMATTED	8<<3
#define	DTAM_NO_DISK		9<<3
#define	DTAM_UNREADABLE		10<<3
#define	DTAM_BAD_ARGUMENT	11<<3
#define	DTAM_BAD_DEVICE		12<<3
#define	DTAM_DEV_BUSY		13<<3
#define	DTAM_NDS		14<<3
#define	DTAM_UNKNOWN		1<<8
/*
 *	the two following values are bit-flags that may be or-ed with the above
 */
#define	DTAM_PACKAGE		1<<9	/* can be cpio or File system format */
#define	DTAM_INSTALL		1<<10	/* can be cpio or File system format */

#define	DTAM_UNMOUNTED		0
#define	DTAM_MOUNTED		1<<10
#define	DTAM_MIS_MOUNT		1<<11
#define	DTAM_CANT_MOUNT		1<<12
#define	DTAM_CANT_OPEN		1<<13
#define	DTAM_NOT_OWNER		1<<14
#define	DTAM_NO_ROOM		1<<15
#define	DTAM_READ_ONLY		1<<16
#define	DTAM_TFLOPPY		1<<17
#define	DTAM_FIRST		1
#define	DTAM_NEXT		0

extern	char	*DtamGetDev(		/* find a line in /etc/device.tab */
			char *		/* pattern in device.tab to match */,
			int		/* first or next match in table. */
		);

extern	char	*DtamDevAttr(		/* find an attribute in a devtab line */
			char *		/* table entry (one line to search) */,
			char *		/* attribute (e.g. mountpt) to find */
		);

extern	char	*DtamDevAttrTxt(	/* find internationalized devtab attr */
			char *		/* table entry (one line to search) */,
			char *		/* attribute (e.g. volume) to find */
		);

extern	char	*DtamDevAlias(		/* "internationalized alias attribute */
			char *		/* table entry (one line to search) */
		);

extern	char	*DtamMapAlias(		/* i18n "alias" -> real devtab alias */
			char *		/* table entry (one line to search) */
		);

extern	char	*DtamDevDesc(		/* "internationalized desc attribute */
			char *		/* table entry (one line to search) */
		);

extern	char	*DtamGetAlias(		/* DtamGetDev + DtamDevAlias */
			char *		/* pattern in device.tab to match */,
			int		/* first or next match in table. */
		);

extern	int	DtamCheckDevice(	/* device diagnostic function */
			char *		/* alias of device to diagnose */
		);

extern	int	DtamMountDev(
			char *		/* alias of device to mount */,
			char **		/* mount point: char * value returned */
		);

extern	int	DtamUnMount(
			char *		/* mount point */
		);


/*
 * DmRegisterMnemonic - this function enables the mnemonic capability
 *	for Motif apps. This routine assumes that the given info are
 *	correct and the routine will not perform any error checking.
 *	This routine returns a non-zero handle if the call was successful.
 *	With this handle, DmUpdateMnemonic (if needed later on) can be
 *	implemented easily.
 *
 *		w		- Any widget/gadget id within the shell.
 *				  This widget/gadget id will be used to
 *				  locate the shell widget. You shall pass
 *				  the shell widget if it is possible!
 *		mne_info	- specify mnemoic information, see
 *				  DesktopP.h:DmMnemonicInfo for more info.
 *		num_mne_info	- specify number of mne_info.
 *
 *	Constrains - This API can't handle the following situations
 *
 *		a. You must call this routine after all children are
 *		   created, otherwise this routine won't be able to
 *		   catch all children within the shell, also see b.
 *
 *		b. If an appl needs to destroy/create object(s) on demand
 *		   within the `shell', and these objects appear (destroy
 *		   case) or do not appear (create case) in the initial
 *		   lists (mne_info).
 *
 *		c. This API won't handle the mnemonic visuals, they come
 *		   from Motif/libXm (usually, XmLabel subclasses).
 *
 *		d. DM_B_MNE_ACTIVATE_BTN can't build a reasonable call_data,
 *		   applications will have to check `call_data' if the CB code
 *		   uses `call_data'.
 *
 *	Hints -
 *
 *		a. To animate OLIT caption layout, usually, you can
 *		   create a XmLabel widget/gadget as a caption (say A)
 *		   and a 2nd widget/gadget as the caption child (say B).
 *		   To enable mnemonic in this case, you can borrow
 *		   mnemonic visual from XmLabel (A) but place B in
 *		   `mne_info'.
 *
 *		b. You may want to use DM_B_MNE_ACTIVATE_CB for Help
 *		   button for avoiding `Can't find per display information'
 *		   error. This is because libXm is doing too much work
 *		   in this case...
 *
 *		c. Try to use DmRegisterMnemonicWithGrab if you hit
 *		   the Constrain a and/or b if mne_info is still same
 *		   as before. If mne_info is changed, then we will have
 *		   to enable DmUpdateMnemonic!!!
 *
 *	You shall free up mne_info after the call if necessary.
 */
extern XtPointer DmRegisterMnemonic(
	Widget,				/* w */
	DmMnemonicInfo,			/* mne_info */
	Cardinal			/* num_mne_info */
);

extern XtPointer DmRegisterMnemonicWithGrab(
	Widget,				/* w */
	DmMnemonicInfo,			/* mne_info */
	Cardinal			/* num_mne_info */
);

#endif /* __DesktopP_h_ */
