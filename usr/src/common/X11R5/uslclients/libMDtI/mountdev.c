#ifndef NOIDENT
#pragma	ident	"@(#)libMDtI:mountdev.c	1.16"
#endif

#include <X11/Intrinsic.h>
#include "DesktopP.h"

#include "dayone.h"	/* for BDEVICE and CDEVICE */

extern	char	*_dtam_mntpt;
extern	char	*_dtam_fstyp;
extern	long	_dtam_flags;

#define	TFADMIN	"/sbin/tfadmin "

int	DtamMountDev(char *alias, char **where)
{
	char	*ptr, cmdbuf[1024];
	char	*fstype;
	char	*mntpt;
	char	*rdonly;
	char	*devline, *device, *dalias;
	int	uid;
	int	diagn;

	dalias = DtamMapAlias(alias);
	diagn = DtamCheckMedia(dalias);
	if (_dtam_flags & DTAM_MOUNTED) {
		*where = _dtam_mntpt;
		return DTAM_MOUNTED;
	}
	switch(diagn&DTAM_FS_TYPE) {
		case 0:			return diagn;
		case DTAM_S5_FILES:	fstype = "s5";		break;
		case DTAM_UFS_FILES:	fstype = "ufs";		break;
		default:		fstype = _dtam_fstyp;	break;
	}
	if ((mntpt = (char *)MALLOC(strlen(alias)+2)) == (char *)0)
		return DTAM_CANT_MOUNT;
	*mntpt = '/';
	strcpy(mntpt+1,alias);
	*where = mntpt;
	if ((devline=DtamGetDev(dalias,DTAM_FIRST))) {
		device = DtamDevAttr(devline,BDEVICE);
		FREE(devline);
	}
	else
		return DTAM_BAD_DEVICE;
	rdonly = (_dtam_flags & DTAM_READ_ONLY)? " -r " : " ";
	strcat(strcpy(cmdbuf, TFADMIN), "-t f");
	sprintf(cmdbuf+strlen(cmdbuf),"mount %s-F %s %s %s",
					rdonly, fstype, device, mntpt);
	FREE(device);
	if (system(cmdbuf) != 0) {
		return DTAM_NOT_OWNER;
	}
	else {
		cmdbuf[strlen(TFADMIN)]   = ' ';
		cmdbuf[strlen(TFADMIN)+1] = ' ';
	} 
	if (system(cmdbuf) != 0)
		return DTAM_CANT_MOUNT;
	else
		return 0;
}

static	int
do_unmount(Widget w, XtPointer client_data, XtPointer call_data)
{
	char	*where = (char *)client_data;
	char	cmdbuf[BUFSIZ];
	int	n, result;

	strcat(strcpy(cmdbuf, TFADMIN), "f");
	strcat(strcat(cmdbuf, "umount "), where);
	result = system(cmdbuf);
	if (result != 0) {
		for (n=2; n>0 && (result == 0x2100 || result == 0xf00); n--) {
			sleep(2);	/* try again: may take a while*/
			result = system(cmdbuf);
		}
#ifdef DEBUG
		if (n == 0 || result != 0)
			fprintf(stderr, "can't unmount: %x\n", result);
#endif
	}
	return result/256;
}

int	DtamUnMount(char *where)
{
	FILE	*fp;
	char	*ptr, *dev;
	int	result;

	ptr = DtamMapAlias(where+1);
	if ((dev = DtamGetDev(ptr,DTAM_FIRST)) == NULL) {
		return do_unmount(NULL, where, NULL);
	}
	else {
		ptr = DtamDevAttr(dev,CDEVICE);
		if (fp=fopen(ptr,"r")) {
			fclose(fp);
			result = do_unmount(NULL, where, NULL);
		}
		else {
			/*
			 *	popup a gizmo for this
			 */
#ifdef DEBUG
			fprintf(stderr,"reinsert diskette and try again\n");
#endif
			result = -1;
		}
		FREE(ptr);
		FREE(dev);
	}
	return result;
}
