#pragma ident	"@(#)dtm:dflt_fc.c	1.8.1.17"

#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>
#include "Dtm.h"
#include "dm_strings.h"
#include "sdefault.icon"
#include <MGizmo/Gizmo.h>

/*
 * The order of the first 3 entries here must not be changed.
 */
static DmFmodeKeyRec dflt_fmodekeys[] = {
{ "DIR",    DM_B_SYS, DM_FTYPE_DIR,  NULL, S_IFDIR, 0000, 0,       },
{ "EXEC",   DM_B_SYS, DM_FTYPE_EXEC, NULL, S_IFREG, 0111, 0,       },
{ "DATA",   DM_B_SYS, DM_FTYPE_DATA, NULL, S_IFREG, 0000, 0,       },
{ "FIFO",   DM_B_SYS, DM_FTYPE_FIFO, NULL, S_IFIFO, 0000, 0,       },
{ "CHRDEV", DM_B_SYS, DM_FTYPE_CHR,  NULL, S_IFCHR, 0000, 0,       },
{ "BLKDEV", DM_B_SYS, DM_FTYPE_BLK,  NULL, S_IFBLK, 0000, 0,       },
{ "SEM",    DM_B_SYS, DM_FTYPE_SEM,  NULL, S_IFNAM, 0000, S_INSEM, },
{ "SHDATA", DM_B_SYS, DM_FTYPE_SHD,  NULL, S_IFNAM, 0000, S_INSHD, },
{ "UNK",    DM_B_SYS, DM_FTYPE_UNK,  NULL, 0,       0000, 0,       },
{ NULL }
};

/*
 * This order must match that of dflt_fmodekeys.
 */
static const char *iconfiles[] = {
	"dir.icon",
	"exec.icon",
	"datafile.icon",
	"pipe.icon",
	"chrdev.icon",
	"blkdev.icon",
	"sem.icon",
	"shmem.icon",
	"unk.icon",
	"toolbox.icon"
};

/*
 * This order must match that of dflt_fmodekeys.
 */
static const char *small_iconfiles[] = {
	"sdir.icon",
	"sexec.icon",
	"sdatafile.icon",
	"spipe.icon",
	"schrdev.icon",
	"sblkdev.icon",
	"ssem.icon",
	"sshmem.icon",
	"sunk.icon",
	"stoolbox.icon"
};

/*
 * This order must match that of dflt_fmodekeys.
 */
static char *class_names[] = {
	TXT_TYPE_FOLDER,
	TXT_TYPE_EXEC,
	TXT_TYPE_DATA,
	TXT_TYPE_PIPE,
	TXT_TYPE_CHRDEV,
	TXT_TYPE_BLKDEV,
	TXT_TYPE_SEM,
	TXT_TYPE_SHMEM,
	TXT_TYPE_UNK,
	TXT_TYPE_TOOLBOX
};

/*
 * Initialize built-in file classes.
 */
void
DmInitDfltFileClasses()
{
	DmFmodeKeyPtr fmkp;
	char **str;
	char **names;

	for (fmkp = dflt_fmodekeys,
	     str  = (char **)iconfiles,
	     names= (char **)class_names;
	     fmkp->ftype;
	     fmkp++, str++, names++) {
		fmkp->fcp = DmNewFileClass((void *)fmkp);
		DtSetProperty(&(fmkp->fcp->plist), ICONFILE, *str, NULL);
		DtSetProperty(&(fmkp->fcp->plist), CLASS_NAME,
				 GetGizmoText(*names), NULL);

		/* To be initialized on demand later */
		fmkp->small_icon = NULL;
	}

	DESKTOP_FMKP(Desktop) = dflt_fmodekeys;
}

/*
 * The reason for having this as a separate routine from
 * DmInitDfltFileClasses() is so that we can cut down the dtm startup time.
 */
void
DmInitSmallIcons(w)
Widget w;
{
	if (dflt_fmodekeys[0].small_icon == NULL) {
		DmFmodeKeyPtr fmkp;
		char **sstr;

		for (fmkp = dflt_fmodekeys, sstr = (char **)small_iconfiles;
		     fmkp->ftype; fmkp++, sstr++) {
			fmkp->small_icon = DmGetPixmap(XtScreen(w), *sstr);
			if (fmkp->small_icon == NULL)
				/*
				 * The bitmap name must be something that
				 * cannot be a real file.
				 */
				fmkp->small_icon =
					DmCreateBitmapFromData(XtScreen(w),
						"\n/default small icon\n",
						sdefault_bits,
						sdefault_width,
						sdefault_height
					);
		}
	}
}

void
Dm__SetDfltFileClass(op, st_buf, lst_buf)
DmObjectPtr op;
struct stat *st_buf;
struct stat *lst_buf;
{
	register DmFmodeKeyPtr fmkp = DESKTOP_FMKP(Desktop);

	/* use unknown as the default */
	op->fcp = dflt_fmodekeys[8].fcp;
	op->ftype = DM_FTYPE_UNK;

	if (st_buf) {
		mode_t fmt  = st_buf->st_mode & S_IFMT;
		mode_t perm = st_buf->st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
		dev_t  rdev = st_buf->st_rdev;

		for (; fmkp->name; fmkp++) {
			if ((!(fmkp->fmt)  || (fmkp->fmt == fmt)) &&
			    (!(fmkp->perm) || (fmkp->perm & perm)) &&
			    (!(fmkp->rdev) || (fmkp->rdev == rdev))) {
				op->fcp = fmkp->fcp;
				op->ftype = fmkp->ftype;
		    		break;
			}
		}
	}
	else
		/* if can't stat a file, don't display the object */
		op->attrs |= DM_B_HIDDEN;

	/* set SYMLNK bit */
	if (lst_buf && ((lst_buf->st_mode & S_IFMT) == S_IFLNK))
		op->attrs |= DM_B_SYMLINK;

	return;
}

DmFmodeKeyPtr
DmFtypeToFmodeKey(ftype)
DmFileType ftype;
{
	return(&dflt_fmodekeys[(int)ftype - 1]);
}

DmFmodeKeyPtr
DmStrToFmodeKey(str)
char *str;
{
	if (str) {
		register DmFmodeKeyPtr fmkp = DESKTOP_FMKP(Desktop);

		for (; fmkp->name; fmkp++) {
			if (!strcmp(fmkp->name, str))
				return(fmkp);
		}
	}

	return(NULL);
}

