#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/nfs.h	1.20"
#endif

/*
 * Module:	dtadmin:nfs  Graphical Administration of Network File Sharing
 * File:	nfs.h        include file for all modules.
 *
 */

extern Boolean OwnerRemote, OwnerLocal;

typedef struct _applicationResources
{
    String   geometry;
    Cardinal updateRate;
} ApplicationResources;

typedef enum _ViewIndex
{
   ViewLocal, ViewRemote, ViewCurrent
} ViewIndex;

typedef enum _DataType
{
    nfsLocal, nfsRemote
} DataType;

typedef struct _ObjectList
{
    DmObjectPtr	        op;
    Cardinal		index;
    struct _ObjectList *next;

} ObjectList, *ObjectListPtr;

typedef struct _NFSWindow
{
    ViewIndex		viewMode;
    BaseWindowGizmo	*baseWindow;
    Widget 		iconbox;
    Gizmo		statusPopup;
    Gizmo		noticePopup;
    Gizmo		hostPopup;
    Gizmo		remotePropertiesPrompt;
    Gizmo		localPropertiesPrompt;
    DmItemPtr       	itp;
    DmContainerPtr  	cp;
    DmContainerPtr  	remote_cp;
    DmContainerPtr  	local_cp;
    ObjectListPtr      *selectList;
    ObjectListPtr     	remoteSelectList;
    ObjectListPtr     	localSelectList;
    DmFclassPtr   	remote_fcp;
    DmFclassPtr   	mounted_fcp;
    DmFclassPtr   	local_fcp;
    DmFclassPtr   	shared_fcp;
} NFSWindow;

typedef enum _ActionsMenuItemIndex
{
    ActionsConnect, ActionsUnconnect,
    ActionsAdvertise, ActionsUnadvertise,
    ActionsStatus,
    ActionsExit 
} ActionsMenuItemIndex;

typedef enum _EditMenuItemIndex
{
   EditAdd, EditDelete, EditProperties
} EditMenuItemIndex;

typedef enum _HelpMenuItemIndex
{
   HelpFileSharing, HelpTOC, HelpDesk
} HelpMenuItemIndex;

typedef enum _PropMenuItemIndex
{
    PropApply, PropReset, PropCancel, PropHelp
} PropMenuItemIndex;


typedef enum _DeleteFlags
{
    ReDoIt_Confirm, Silent, ReDoIt_Silent
} DeleteFlags;


typedef enum _msgTarget
{
    Base, Popup, Notice
} msgTarget;

extern void         SetMessage();

#ifdef ApplicationName
#else

extern Arg          arg[50];
extern Widget       root;

#endif
extern NFSWindow * 	FindNFSWindow();
extern Boolean DeleteLocalCB();
extern void PropertiesCB();
extern void MountCB();
extern void UnMountCB();
extern void AdvertiseCB();
extern void UnAdvertiseCB();
extern void NoMemoryExit();
#define NO_MEMORY_EXIT()	NoMemoryExit(__FILE__, __LINE__)

#define DIR_CREATE_MODE (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define WHITESPACE  " \t\n"
#define EOS '\0'
#define MINIBUFSIZE        50
#define SMALLBUFSIZE      150
#define MEDIUMBUFSIZE     250
#define OPTIONS_FIELD_SIZE 32
#define LABEL_FIELD_SIZE   14
#define PATH_FIELD_SIZE    32
#define HOST_FIELD_SIZE    20
#define DEFAULT_WIDTH       OlMMToPixel(OL_HORIZONTAL, 110)
#define INIT_X  40
#define INC_X   80
#define INIT_Y  5
#define INC_Y   80
#define SUCCESS 0
#define FAILURE 1

#define NOMEMORY 2


 /* DTVFSTAB contains a copy of those records in /etc/vfstab for */
 /* which the filesystem type is nfs.  The record format is the same */
 /* except that the "fsckpass" field is used to hold the icon label */
 /* string displayed in the base window.  This allows the */
 /* getvfsent(3C) tools to be used. */

#define DTVFSTAB	"/etc/dfs/dtvfstab"
#define VFSTAB		"/etc/vfstab" 
#define DFSTAB		"/etc/dfs/dfstab"
#define MNTTAB		"/etc/mnttab"

typedef enum _IconType
{
    LocalIcon,
    SharedIcon,
    RemoteIcon,
    MountedIcon
} IconType;
/* entries for each of the types defined above... */
extern char *IconFilenames[];

typedef enum _question { q_No, q_Yes, q_Idunno } question;

#define RETURN_IF_NULL(pointer, text) if ((pointer) == NULL) \
    { SetMessage(MainWindow, (text), Base); return; }

#define RETURN_VALUE_IF_NULL(pointer, text, value) if ((pointer) == NULL) \
    { SetMessage(MainWindow, (text), Base); return (value); }

extern char * debug_nfs;
#define DEBUG0(F1) DEBUG4( F1, 0, 0, 0, 0)
#define DEBUG1(F1, A2) DEBUG4( F1, A2, 0, 0, 0)
#define DEBUG2(F1, A2, A3) DEBUG4( F1, A2, A3, 0, 0)
#define DEBUG3(F1, A2, A3, A4) DEBUG4( F1, A2, A3, A4, 0)
#ifdef DEBUG_NFS
#define DEBUG4(format, parm1, parm2, parm3, parm4) \
    if (debug_nfs) \
    {\
	 (void)fprintf(stderr,"NFS: at %d in %s: ", __LINE__, __FILE__);\
	 (void)fprintf(stderr, format, parm1, parm2, parm3, parm4);\
     }
#else
#define DEBUG4(format, parm1, parm2, parm3, parm4)
#endif /* ifdef DEBUG_NFS */

#define HELPPATH		"dtadmin" "/" "FileShar.hlp"

extern HelpInfo RemoteAddWindowHelp;
extern HelpInfo RemotePropWindowHelp;
extern HelpInfo LocalAddWindowHelp;
extern HelpInfo LocalPropWindowHelp;
extern HelpInfo StatusWindowHelp;
extern HelpInfo HostWindowHelp;
extern HelpInfo FindWindowHelp;
extern HelpInfo FindFolderWindowHelp;
extern HelpInfo DeleteLocalNoticeHelp;
extern HelpInfo DeleteRemoteNoticeHelp;
extern HelpInfo StopNFSNoticeHelp;
extern HelpInfo CreateMountPointNoticeHelp;
extern HelpInfo CreateFolderNoticeHelp;
extern HelpInfo ApplicationHelp;
extern HelpInfo TOCHelp;
extern HelpInfo HelpDeskHelp;
