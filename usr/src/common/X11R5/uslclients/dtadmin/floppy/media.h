#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/media.h	1.40.1.2"
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Vendor.h>
#include <Xol/OpenLookP.h>
#include <Xol/BulletinBo.h>
#include <Xol/FooterPane.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>
#include <Xol/Gauge.h>
#include <Xol/FButtons.h>
#include <Xol/Caption.h>
#include <Xol/Notice.h>
#include <Xol/PopupMenu.h>
#include <Xol/PopupWindo.h>
#include <Xol/AbbrevButt.h>
#include <Xol/ScrolledWi.h>
#include <Xol/RubberTile.h>
#include <Xol/FList.h>
#include <Xol/Footer.h>
#include <Xol/Form.h>

#include <Dt/Desktop.h>
#include <DnD/OlDnDVCX.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/FileGizmo.h>
#include <Gizmo/ChoiceGizm.h>
#include <Gizmo/xpm.h>

#include <Memutil/memutil.h>

#include <libDtI/DtI.h>

#include "../dtamlib/dtamlib.h"

#include "media_msgs.h"

#define	DHELP_PATH	"dtadmin/floppy.hlp"
#define	CDHELP_PATH	"dtadmin/cdrom.hlp"
#define	THELP_PATH	"dtadmin/tape.hlp"
#define	BHELP_PATH	"dtadmin/backup.hlp"
#define	D1ICON_NAME	"Disk_A.icon"
#define	D2ICON_NAME	"Disk_B.icon"
#define D1_MNTPT        "Disk_A"
#define D2_MNTPT        "Disk_B"
#define CD_MNTPT        "CD-ROM"
#define	GENICON_NAME	"gendev.icon"
#define	TICON_NAME	"ctape.glyph"

#define	DO_OPEN		0
#define	DO_COPY		1
#define	DO_BACKUP	2
#define	DO_RESTOR	4
#define	DO_INSTAL	8
#define	DO_FORMAT	16
#define	DO_REWIND	32

#define	FooterMsg(basegizmo,txt)	SetBaseWindowMessage(&basegizmo,txt)
#define	GGT(str)			GetGizmoText(str)

typedef enum _exitValue { NoErrs, NoAttempt, FindErrs, CpioErrs, NDSErrs } ExitValue;
typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  XtArgVal	sensitive;
		  XtArgVal	selCB;
		  XtArgVal	subMenu;
} MBtnItem;

typedef struct	{
	int	opflag;
	int	diagn;
	char	*fstype;
	char	**drop_list;
} MountInfo;

#define	NUM_MBtnFields	5

#define	SET_BTN(id,n,name,cbf)	id[n].label = GetGizmoText(label##_##name);\
				id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].sensitive = (XtArgVal)TRUE;\
				id[n].selCB = (XtArgVal)cbf

typedef	struct	{ char	*	label;
		  XtArgVal	mnem;
		  XtArgVal	setting;
		  XtArgVal	sensitive;
} ExclItem;

#define	NUM_ExclFields	4

#define SET_EXCL(id,n,name,st)	id[n].label = GetGizmoText(label##_##name);\
                               	id[n].mnem = (XtArgVal)*GGT(mnemonic##_##name);\
				id[n].setting = (XtArgVal)st;\
				id[n].sensitive = (XtArgVal)TRUE;

typedef struct  { char	*	label;
		  Boolean	deflt;
} DevItem;

#define	NUM_DevFields	2
#define	N_DEVS		8	/* recommended max menu size */

#define	MOUNT_CMD	"/sbin/mount"

typedef	struct {
		char	*bk_path;	
		char	bk_type;	/* 'd' for directory */
		int	bk_blksize;
} FileObj, *FileList;

typedef struct {
    int	num_files;	/* number of src_files in file-op */
    int cur_file;	/* current file being operated on */
    char **src_files;	/* list of files to be copied, full pathnames */
    char **dest_files;	/* destination pathnames for each of above */
    char *device;	/* device where filesystem resides */
    char *destination;	/* mount point for filesystem */
    char *fstype;	/* filesystem type on device (e.g., "dosfs") */
} FileOpRec, *FileOpPtr;

struct MMOptions
{
      OlFontList      *font_list;
};

struct MMOptions MMOptions;

#define MM_FONTLIST_OFFSET      XtOffsetOf(struct MMOptions, font_list)
#define MM_FONTLIST             MMOptions.font_list

extern  XtAppContext	App_con;
extern	Widget		w_toplevel, w_note, w_panel, w_gauge, w_txt;
extern	Display		*theDisplay;
extern	Screen		*theScreen;
extern	Dimension	x3mm, y3mm;
extern	Dimension	xinch, yinch;
extern	XFontStruct	*def_font, *bld_font;
extern	XtIntervalId	gauge_id;
extern	PopupGizmo	note;
extern	Arg		arg[];
extern	int		g_value;
extern	char		*MBtnFields[];
extern	char		*ExclFields[];
extern	char		*DevFields[];

extern	int		errno;
extern	FILE		*cmdfp[];

#define	CMDIN		fileno(cmdfp[0])
#define	CMDOUT		fileno(cmdfp[1])

extern	char		*_dtam_mntpt;
extern	char		*_dtam_mntbuf;
extern	long		_dtam_flags;

extern	char		*desc;
extern	char		*curdev;
extern	char		*curalias;
extern	char		*cur_file;
extern	char		*Help_intro;

extern	char		*getenv();
extern	char		*ptsname();
extern	void		exitCB();
extern	Widget 		CreateMediaWindow();
extern	Widget		DevMenu();
extern  void		attempt_copy(FileOpPtr file_op);
extern  void		FreeFileOp(FileOpPtr);
extern	void		BaseNotice(int, Boolean, char **);
extern	void		CreateBackupWindow(Widget, char **, char *);
