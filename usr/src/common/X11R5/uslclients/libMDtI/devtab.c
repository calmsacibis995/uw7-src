#pragma	ident	"@(#)libMDtI:devtab.c	1.22.1.6"

/*
 * To reduce the copies of stat(), fstat(), lstat(), and mknod(),
 * mapfile.c and diagnose.c are moved into devtab.c.
 * See sys/stat.h.
 */

/*
 *	This provides a library interface to the device table similar to
 *	the commands getdev and devattr; that functionality is heavily used
 *	in device administration, so that considerable overhead would be
 *	introduced by continually spawning child processes to get this data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		/* for gettxt() */
#include <string.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <errno.h>
#include <archives.h>
#include <sys/fs/s5param.h>
#include <sys/fs/s5filsys.h>
#undef	getfs
#undef	FsOKAY
#undef	FsACTIVE
#include <sys/fs/sfs_fs.h>

#include <X11/Intrinsic.h>	/* for FREE, STRDUP, REALLOC etc. marco */
				/* why not just include memutil.h? */
#include <nws/nwbackup.h>
#include "DesktopP.h"

#include "dayone.h"		/* for ALIAS, BDEVICE, CDEVICE, DISKETTE */
#include "mapfile.h"

#define DTDEVTAB_FILENAME	"dtdevtab:"	/* message catelog file name */

#define	FS			"\000"

#define	alias_disk		 "1" FS "Disk_%c"
#define	alias_ctape1		 "2" FS "Cartridge_Tape"
#define	tag_disk		 "3" FS "A"
#define	desc_disk3		 "4" FS "3.5 inch"
#define	desc_disk5		 "5" FS "5.25 inch"
#define	desc_disk0		 "6" FS "Floppy Disk Drive %c"
#define desc_ctape1		 "7" FS "Cartridge Tape Drive"
#define alias_cdrom              "8" FS "CD-ROM"
#define desc_cdrom               "9" FS "Compact Disc-ROM Drive"
#define alias_ctape2		"10" FS "Cartridge_Tape 2"
#define desc_ctape2		"11" FS "Cartridge Tape Drive 2"
#define alias_ctape3		"12" FS "Cartridge_Tape 3"
#define desc_ctape3		"13" FS "Cartridge Tape Drive 3"
#define alias_ctape4		"14" FS "Cartridge_Tape 4"
#define desc_ctape4		"15" FS "Cartridge Tape Drive 4"
#define alias_ctape5		"16" FS "Cartridge_Tape 5"
#define desc_ctape5		"17" FS "Cartridge Tape Drive 5"
#define diskette_txt		"18" FS "diskette"

#define	DEVTAB			"/etc/device.tab"

#define	FIRST			1
#define	NEXT			0

static char *	DevtabGetTxt(const char *);
static char *	FetchDevtab(void);
static char *	SetAliasMap(char *, char *);

extern char *	DtamGetDev(char *, int);
extern char *	DtamDevAttr(char *, char *);
extern char *	DtamDevAttrTxt(char *, char *);
extern char *	DtamMapAlias(char *);
extern char *	DtamDevAlias(char *);
extern char *	DtamDevDesc(char *);
extern char *	DtamGetAlias(char *, int);

static char *
FetchDevtab(void)
{
struct	stat	st_buf;
	char	*ptr;
	int	devfd;
	char	*dev_tab = NULL;

	if (stat(DEVTAB, &st_buf) != -1 &&
	    (devfd = open(DEVTAB,O_RDONLY)) != -1) {
		dev_tab = mmap((caddr_t)0, st_buf.st_size, PROT_READ,
				MAP_SHARED, devfd, 0);
	}
	return(dev_tab);
}

char *
DtamGetDev(char *pattern, int flag)
{
static	char	*nextptr = NULL;
static	char	*devtab = NULL;

	char	*ptr, *devline;

	if (devtab == NULL)
		if (!(devtab = FetchDevtab()))
			return(NULL);

	if (flag == FIRST || nextptr == NULL)
		nextptr = devtab;
	if ((ptr = strstr(nextptr, pattern)) == NULL) {
		nextptr = devtab;
		return NULL;
	}
	nextptr = strchr(ptr,'\n');
	while ((*ptr != '\n') && (ptr != devtab))
		ptr--;
	if ((devline=(char *)MALLOC(nextptr-ptr+1)) == NULL)
		return NULL;
	ptr++;
	strncpy(devline, ptr, nextptr-ptr);
	devline[nextptr-ptr] = 0;
	return devline;
}

char *
DtamDevAttr(char *devline, char *pattern)
{
	char	*ptr = devline, *endstr, *attr = NULL;

	endstr = strchr(ptr, ':');
	if (strcmp(pattern, ALIAS) != 0) {
		ptr = endstr+1;
		endstr = strchr(ptr, ':');
		if (strcmp(pattern, CDEVICE)  != 0) {
			ptr = endstr+1;
			endstr = strchr(ptr, ':');
			if (strcmp(pattern, BDEVICE) != 0) {
				if ((ptr=strstr(endstr+1,pattern)) != NULL) {
					ptr += strlen(pattern);
					if (ptr[0] == '=' && ptr[1] == '"') {
						ptr += 2;
						endstr = strchr(ptr, '"');
					}
					else {
						ptr = NULL;
					}
				}
			}
		}
	}
	if (ptr == NULL || ptr == endstr)
		attr = NULL;
	else {
		if ((attr=(char *)MALLOC(endstr-ptr+2)) != NULL) {
			strncpy(attr, ptr, endstr-ptr);
			attr[endstr-ptr] = 0;
		}
	}
	return attr;
}

/*
 *	devalias is an "internationalization" of the /etc/device.tab alias
 *	In general, if a device entry has a dtalias attribute, this is read
 *	as a message catalog id, with optional following default text:
 *
 *		dtalias="<pathname>:<index>[:<text>]"
 *
 *	in the absence of a following text component, the alias attribute
 *	is taken as the default text argument to gettxt().
 *	If no dtalias attribute exists, and the device is one of the standard
 *	ones (diskette? and ctape1), it is special-cased through DevtabGetTxt.
 *	In all other cases, devalias returns the alias.
 *
 *	AliasMap retains a trace of devalias calls, with the mapping of the
 *	returned value to the actual alias in the device line.
 */
static char	**AliasMap = NULL;

static	char *
SetAliasMap(char *i18nalias, char *devline)
{
static	int	cnt = 0;

	if (cnt++)
		AliasMap = (char **)REALLOC(AliasMap,(2*cnt+1)*sizeof(char *));
	else
		AliasMap = (char **)MALLOC(3*sizeof(char *));
	if (AliasMap == NULL) {
		fprintf(stderr, "cannot allocate device alias table\n");
		return NULL;
	}
	AliasMap[2*cnt-2] = STRDUP(i18nalias);
	if (strncmp(devline, i18nalias, strlen(i18nalias))==0)
		AliasMap[2*cnt-1] = AliasMap[2*cnt-2];
	else
		AliasMap[2*cnt-1] = DtamDevAttr(devline,ALIAS);
	AliasMap[2*cnt] = NULL;
	return i18nalias;
}

char *
DtamMapAlias(char *i18nalias)	/* return /etc/device.tab alias */
{
	int	n;

	for (n = 0;  AliasMap[n]; n += 2) {
		if (strcmp(AliasMap[n], i18nalias) == 0)
			return AliasMap[n+1];
	}
	return i18nalias;
}

static char *
DevtabGetTxt(const char * label)
{
	static char msgid[9 + 10] = DTDEVTAB_FILENAME;

	strcpy(msgid + 9, label);
	return(gettxt((const char *)msgid, label + strlen(label) + 1));
}

/* This routine provides an internationalized version of the attribute
 * from /etc/device.tab that matches the pattern passed to this routine.
 * Currently only attributes that are set to "diskette" will be 
 * translated.
 */
char *
DtamDevAttrTxt(char *devline, char *pattern)
{
	char *str;

	str = DtamDevAttr(devline, pattern);

	if (strncmp(str, DISKETTE, sizeof(DISKETTE)-1)==0) 
		return (DevtabGetTxt(diskette_txt));
	else
		return (str);
}

char *
DtamDevAlias(char *devline)
{
	char	*ptr, buf[40];
	char	bufCD[BUFSIZ];
	int 	numCD;

	if (ptr = DtamDevAttr(devline, DTALIAS)) {
		char	*p1, *p2;

		if ((p1 = strchr(ptr,':')) == NULL) {
			fprintf(stderr,"invalid %s in %s\n", DTALIAS, DEVTAB);
			strcpy(buf, p2=DtamDevAttr(devline, ALIAS));
			FREE(p2);
		}
		else if ((p2 = strrchr(ptr,':')) == p1) {
			strcpy(buf, gettxt(ptr, p2=DtamDevAttr(devline,ALIAS)));
			FREE(p2);
		}
		else {
			*p2++ = '\0';
			strcpy(buf, gettxt(ptr, p2));
		}
		FREE(ptr);
		return SetAliasMap(STRDUP(buf), devline);
	}
	else if (strncmp(devline, CTAPE1, sizeof(CTAPE1)-1)==0) {
		return SetAliasMap(STRDUP(DevtabGetTxt(alias_ctape1)),devline);
	}
	else if (strncmp(devline, CTAPE2, sizeof(CTAPE2)-1)==0) {
		return SetAliasMap(STRDUP(DevtabGetTxt(alias_ctape2)),devline);
	}
	else if (strncmp(devline, CTAPE3, sizeof(CTAPE3)-1)==0) {
		return SetAliasMap(STRDUP(DevtabGetTxt(alias_ctape3)),devline);
	}
	else if (strncmp(devline, CTAPE4, sizeof(CTAPE4)-1)==0) {
		return SetAliasMap(STRDUP(DevtabGetTxt(alias_ctape4)),devline);
	}
	else if (strncmp(devline, CTAPE5, sizeof(CTAPE5)-1)==0) {
		return SetAliasMap(STRDUP(DevtabGetTxt(alias_ctape5)),devline);
	}
	/* for CDROMs the first field in devline is of the form cdrom# 
	 * where # can be any integer.  Parse out "cdrom" from the 
	 * string and translate then re-append the number.
	 */
	else if (strncmp(devline, CDROM, sizeof(CDROM)-1)==0) {
		sscanf(devline+sizeof(CDROM)-1, "%d", &numCD);
		sprintf(bufCD, "%s_%d", DevtabGetTxt(alias_cdrom), numCD);
		return SetAliasMap(STRDUP(bufCD), devline);
	}
	else if (strncmp(devline, DISKETTE, sizeof(DISKETTE)-1)==0) {
		/*
		 *	translate "diskette1" etc. to "Disk_A" etc
		 *	(where the etc. means that one sequence is
		 *	mapped to another, starting with the I18N
		 *	tag_disk character.
		 */
		char	c = devline[sizeof(DISKETTE)-1];
		char	A = *DevtabGetTxt(tag_disk);
		sprintf(buf, DevtabGetTxt(alias_disk), c + A - '1');
		return SetAliasMap(STRDUP(buf), devline);
	}
	else
		return SetAliasMap(DtamDevAttr(devline, ALIAS), devline);
}

/*
 *	devdesc is similar to devalias, keying off a dtdesc attribute instead
 */
char *
DtamDevDesc(char *devline)
{
	char	*ptr, *p1, *p2, buf[80];

	if (ptr = DtamDevAttr(devline, DTDESC)) {
		if ((p1 = strchr(ptr,':')) == NULL) {
			fprintf(stderr,"invalid %s in %s\n", DTDESC, DEVTAB);
			strcpy(buf, p2=DtamDevAttr(devline, DESC));
			FREE(p2);
		}
		else if ((p2 = strrchr(ptr,':')) == p1) {
			strcpy(buf, gettxt(ptr, p2=DtamDevAttr(devline, DESC)));
			FREE(p2);
		}
		else {
			*p2++ = '\0';
			strcpy(buf, gettxt(ptr, p2));
		}
		FREE(ptr);
	}
	else if (strncmp(devline, CTAPE1, sizeof(CTAPE1)-1)==0) {
		strcpy(buf, DevtabGetTxt(desc_ctape1));
	}
	else if (strncmp(devline, CTAPE2, sizeof(CTAPE2)-1)==0) {
		strcpy(buf, DevtabGetTxt(desc_ctape2));
	}
	else if (strncmp(devline, CTAPE3, sizeof(CTAPE3)-1)==0) {
		strcpy(buf, DevtabGetTxt(desc_ctape3));
	}
	else if (strncmp(devline, CTAPE4, sizeof(CTAPE4)-1)==0) {
		strcpy(buf, DevtabGetTxt(desc_ctape4));
	}
	else if (strncmp(devline, CTAPE5, sizeof(CTAPE5)-1)==0) {
		strcpy(buf, DevtabGetTxt(desc_ctape5));
	}
	else if (strncmp(devline, CDROM, sizeof(CDROM)-1)==0) {
		strcpy(buf, DevtabGetTxt(desc_cdrom));
	}
	else if (strncmp(devline, DISKETTE, sizeof(DISKETTE)-1)==0) {
		if ((p1 = DtamDevAttr(devline,"fmtcmd")) != NULL
		&&  (p2 = strpbrk(p1,"35")) != NULL) {
			sprintf(buf, DevtabGetTxt(*p2=='3'? desc_disk3:
							  desc_disk5));
			FREE(p1);
		}
		else
			sprintf(buf, DevtabGetTxt(desc_disk0),
					devline[sizeof(DISKETTE)]);
	}
	else
		return DtamDevAttr(devline, DESC);
	return STRDUP(buf);
}

/*
 *	the following is a convenience routine for the Finder
 */
char *
DtamGetAlias(char *pattern, int flag)
{
	char	*dev, *ptr;

	if (ptr=DtamGetDev(pattern,flag)) {
		dev = ptr;
		ptr = DtamDevAlias(dev);
		FREE(dev);
	}
	return ptr;
}

/* diagonse.c start here */

#define	MOUNT_TABLE	"/etc/mnttab"

char	*_dtam_mntpt = NULL;
char	*_dtam_mntbuf= NULL;	/* can be static */
char	*_dtam_fstyp = NULL;
long	_dtam_flags  = 0;

#ifdef	DEBUG
static	void	prtdiag(int n)
{
	char	*ptr;

	switch (n) {
	case DTAM_S5_FILES:		ptr = "S5_FILES";	break;
	case DTAM_UFS_FILES:		ptr = "UFS_FILES";	break;
	case DTAM_FS_TYPE:		ptr = "FS_TYPE";	break;
	case (DTAM_CPIO|DTAM_PACKAGE):	ptr = "PACKAGE";	break;
	case (DTAM_CPIO|DTAM_INSTALL):	ptr = "INSTALL";	break;
	case DTAM_BACKUP:		ptr = "BACKUP";		break;
	case DTAM_CPIO:			ptr = "CPIO";		break;
	case DTAM_CPIO_BINARY:		ptr = "CPIO_BINARY";	break;
	case DTAM_CPIO_ODH_C:		ptr = "CPIO_ODC_H";	break;
	case DTAM_NDS:			ptr = "NDS";		break;
	case DTAM_TAR:			ptr = "TAR";		break;
	case DTAM_CUSTOM:		ptr = "CUSTOM";		break;
	case DTAM_DOS_DISK:		ptr = "DOS_DISK";	break;
	case DTAM_UNFORMATTED:		ptr = "UNFORMATTED";	break;
	case DTAM_NO_DISK:		ptr = "NO_DISK";	break;
	case DTAM_UNREADABLE:		ptr = "UNREADABLE";	break;
	case DTAM_BAD_ARGUMENT:		ptr = "BAD_ARGUMENT";	break;
	case DTAM_BAD_DEVICE:		ptr = "BAD_DEVICE";	break;
	case DTAM_DEV_BUSY:		ptr = "DEV_BUSY";	break;
	case DTAM_UNKNOWN:		ptr = "UNKNOWN";	break;
	default:			ptr = "?";		break;
	}
	fprintf(stderr, "CheckMedia ->  %s\n", ptr);
}
#endif

int	DtamCheckMedia(char *alias)
{
struct	stat	st_buf;
	char	*device;
	char	*ptr;
	int	mntfd;
	int	n;
 
	_dtam_flags = 0;
	if (alias == NULL)
		return 0;
	else if ((ptr = DtamGetDev(alias,DTAM_FIRST)) == NULL) {
		n = diagnose(alias, alias); 
#ifdef	DEBUG
		prtdiag(n);
#endif
		return n;
	}
	else if ((device = DtamDevAttr(ptr,BDEVICE)) == NULL) {
		if ((device = DtamDevAttr(ptr,CDEVICE)) == NULL)
			return DTAM_BAD_DEVICE;
		else {
			n = diagnose(device, alias);
#ifdef	DEBUG
			prtdiag(n);
#endif
			FREE(ptr);
			FREE(device);
			return n;
		}
	}
	else
		FREE(ptr);
/*
 *	the following checks to see if /dev/dsk/fn[t] is mounted;
 *	if so, that is the target of the diagnostic, and only if checks
 *	on this device fail will the other option be tried.  If neither
 *	is (claimed to be) mounted, then the device checked first is the
 *	one originally specified, and 't' is deleted or added for a second
 *	diagnostic pass if the first one fails.
 */
	n = strlen(device)-1;
	if (strstr(alias,DISKETTE) && device[n] == 't') {
	/*
	 *	(temporarily) remove the final t, to match both possibilities
	 */
		device[n] = '\0';
		_dtam_flags = DTAM_TFLOPPY;
	}
	else
		_dtam_flags = 0;
	if (_dtam_mntpt) {
		FREE(_dtam_mntpt);
		_dtam_mntpt = NULL;
	}
	if (stat(MOUNT_TABLE, &st_buf) != -1
	&& (mntfd = open(MOUNT_TABLE,O_RDONLY)) != -1) {
		_dtam_mntbuf = mmap((caddr_t)0, st_buf.st_size, PROT_READ,
				MAP_SHARED, mntfd, 0);
	}
	if (_dtam_mntbuf != NULL && _dtam_mntbuf != (char *)-1) {
		if ((ptr=strstr(_dtam_mntbuf, device)) != NULL) {
				char	*ptr2;
				int	i = 0;
			for (ptr2 = strchr(ptr,'\t')+1; *ptr2 != '\t'; ptr2++)
				i++;
			_dtam_mntpt = (char *)malloc(i+1);
			_dtam_mntpt[i] = 0;
			strncpy(_dtam_mntpt, ptr2-i, i);
			if (ptr[n] == 't' && _dtam_flags)
			/*
			 *	retore the final t, as that is what is mounted
			 */
				device[n] = 't';
			else
				_dtam_flags = 0;
			_dtam_flags |= DTAM_MOUNTED;
		}
		else if (_dtam_flags & DTAM_TFLOPPY)
			device[n] = 't';
		munmap(_dtam_mntbuf, st_buf.st_size);
		close (mntfd);
		_dtam_mntbuf = NULL;
	}
/*
 *	Now run through actual checks on the disk 
 */
	n = diagnose(device, alias);
	if (n & DTAM_FS_TYPE == 0 && _dtam_flags & DTAM_MOUNTED) {
		_dtam_flags |= DTAM_MIS_MOUNT;
		DtamUnMount(_dtam_mntpt);
	}
	if (n == DTAM_UNKNOWN && (_dtam_flags & DTAM_TFLOPPY) != 0) {
	/*
	 *	try again, with the non-t version of the device
	 */
		_dtam_flags ^= DTAM_TFLOPPY;
		device[strlen(device)-1] = '\0';
		if ((n = diagnose(device, alias)) == DTAM_UNKNOWN) 
			_dtam_flags |= DTAM_TFLOPPY;
	}
#ifdef	DEBUG
	prtdiag(n);
#endif
	FREE(device);
	return n;
}

char * StoreIndexNumber(char *ascNum)
{
static char *flpIndexNumber = NULL;
	if (ascNum)
	{
		flpIndexNumber = strdup(ascNum);
		return (0);
	}
	else 
	{
		return (flpIndexNumber);
	}
}

int	diagnose( char *dev, char *alias)
{
extern	int	errno;
	char	devbuf[2*BBSIZE];
	char	cmdbuf[PATH_MAX+35];
	char	mycmdbuf[PATH_MAX+35];
	char	mnt_pt[BUFSIZ];
	char	pkginfo_path[PATH_MAX];
	char	pkgmap_path[PATH_MAX];
	char	install_path[PATH_MAX];
	int	devfd;
	int	n, result, result1, result2, len, slen;
	long	*l;
	struct	filsys		*s5_files;
	struct	fs		*sfs_files;
	struct	Exp_cpio_hdr	*cpio_hdr;
	struct	c_hdr		*char_hdr;
	struct	hdr_cpio	*bin_cpio;
	struct	tar_hdr		*tarbuf;
	char	*strstr();
	char	*tapenum;
	static	int		did_tapecntl = 0;
	NDSTapeHead	*nds_hdr;
	char	deviobuf[2*BBSIZE];
	FILE	*devioptr;


	if (access(dev,R_OK) == -1)
		return(DTAM_BAD_DEVICE);

	/*
	 * To support devices with variable length blocks mode, use 512-bytes
	 * block length for creating backup archives, as well as for 
	 * reading/restoring (the kernel will remember what block size was 
	 * used to write to cartridge tape, and driver will attempt to read 
	 * the same length).  512 is the safest length since it is supported
	 * by all devices.  This should avoid a block size mismatch when
	 * attempting to read a tape with a backup archive, if this was
	 * created via the MediaMgr.
	 */  
	if (((tapenum = strstr(alias, "tape")) != 0) && !did_tapecntl ){
		if (access("/usr/bin/tapecntl", F_OK) == 0) {
		/* 
		 * We are using ctape? device here because we always
		 * read/write from the beginning of the tape.
		 * If, in the future, we decide to support appending
		 * to existing archives on a tape, we should use
		 * the ntape? device for "no-rewind."
		 */ 
			sprintf(cmdbuf, "/usr/bin/tapecntl -f 512 /dev/rmt/c");
			strcat(cmdbuf, tapenum);
			system(cmdbuf);
			did_tapecntl = 1;
		}
	}
	if ((devfd = open(dev,
			_dtam_flags&DTAM_READ_ONLY? O_RDONLY: O_RDWR)) == -1) {
tsterr:		switch (errno) {
			case ENODEV:
			case EACCES:
			case EROFS:	if (_dtam_flags & DTAM_READ_ONLY)
						return DTAM_UNREADABLE;
					_dtam_flags |= DTAM_READ_ONLY;
					if ((devfd=open(dev,O_RDONLY)) == -1)
						goto tsterr;
					break;
			case EIO:	return DTAM_NO_DISK;
			case ENXIO:	return DTAM_UNFORMATTED;
			case EBUSY:	return DTAM_DEV_BUSY;
			default:	return DTAM_UNREADABLE;
		}
	}
/*
 *	map out 2 sfs-sized blocks (8192 each)
 */
	n = read(devfd,devbuf,2*BBSIZE);
	close (devfd);
	if (n <= 0) {
		switch(errno) {
			case EIO: 	
				/* UNFORMATTED for diskette only */
				if ((strstr(dev, "dsk") != 0) || 
					(strstr(dev, "diskette") != 0))
					return(DTAM_UNFORMATTED);
				/* else fall thru */
			default:	return(DTAM_UNREADABLE);
		}
	}
/*
 *	check for cpio formats (cf. archive.h)
 *
 *	all character formats for cpio have initial string "07070" -- they
 *	differ in the 6th byte, but for input that can safely be left to cpio
 */
	if (strncmp(devbuf,"# PaCkAgE DaTaStReAm",20) == 0) {
		return(DTAM_PACKAGE|DTAM_CPIO);
	}
	if (strncmp(devbuf,"07070",5) == 0) {
		if (devbuf[5] == '1') {
			cpio_hdr = (struct Exp_cpio_hdr *)devbuf;
			if (strncmp(cpio_hdr->E_name,"/tmp/flp_index",14)==0) {
				(void) StoreIndexNumber(cpio_hdr->E_name + 15);
				return(DTAM_BACKUP);
			}
		}
		else if (devbuf[5] == '7') {
			char_hdr = (struct c_hdr *)devbuf;
			/* If the 1st record is a ".", skip over to the 2nd
			 * record (assuming it is "Size") on the diskette.
			 */
			if (strcmp(char_hdr->c_name,".") == 0) {
				if ((slen=strtol(char_hdr->c_filesz, NULL, 8)) > 0)
					len = 2 + slen;
				char_hdr=(struct c_hdr *)&(char_hdr->c_name[len]);
			}
			if (strcmp(char_hdr->c_name,"Size") == 0) 
				return(DTAM_INSTALL|DTAM_CPIO);
		}
		return(DTAM_CPIO);
	}
	bin_cpio = (struct hdr_cpio *)devbuf;
	if (bin_cpio->h_magic == CMN_BIN) {
		return(DTAM_CPIO_BINARY);
	}


	/* Check if this is an NDS archive header */
	sprintf(mycmdbuf, "/usr/bin/devio -I %s -pb 2>&1", dev);
	if ((devioptr = popen(mycmdbuf, "r")) != NULL) {
		n = fread(deviobuf, 1, sizeof(NDSTapeHead), devioptr);
		if (n == sizeof(NDSTapeHead)) {
			nds_hdr = (NDSTapeHead *)deviobuf;
			if (strcmp(nds_hdr->idstring, IDSTRING)==0) {
				pclose(devioptr);
				return(DTAM_NDS);
			}
		}
		pclose(devioptr);
	}

/*
 *	the tar structure provides for a "magic number" but this is not
 *	unique, but rather dependent on the implementation of tar.  The
 *	following is an ad hoc and fallible attmept to recognize a tar file.
 */
	tarbuf = (struct tar_hdr *)devbuf;
	if ((n=strlen(tarbuf->t_name)) > 0 && n < TNAMLEN
	&&  strlen(tarbuf->t_mode) == TMODLEN-1
	&&  strlen(tarbuf->t_uid)  == TUIDLEN-1
	&&  strlen(tarbuf->t_gid)  == TGIDLEN-1) {
		/*
		 *	should check for syntactical validity of these
		*/
		if (strncmp(tarbuf->t_name, "/etc/perms/", 11) == 0
		|| strstr(tarbuf->t_name, "/prd=")) {
			return(DTAM_CUSTOM);
		}
		else {
			return(DTAM_TAR);
		}
	}
/*
 *	try files systems now
 */
	{
	FILE	*pipefp;
	char	devbuf[BUFSIZ];

		sprintf(devbuf, "LANG=C /sbin/fstyp %s 2>&1", dev);
		if (pipefp=popen(devbuf,"r")) {
			while (fgets(devbuf, BUFSIZ, pipefp))
				;
			pclose(pipefp);
			if (_dtam_fstyp)
				FREE(_dtam_fstyp);

			/* Only use the 1st "token" for now.  In the
			   case of a CD-ROM, for instance, additional
			   info is provided.  This is currently
			   unused so must be discarded.  (While we're
			   at it, clobber any trailing newline)
			*/
			(void)strtok(devbuf, " \t\n");

			_dtam_fstyp = STRDUP(devbuf);

			if (!strstr(_dtam_fstyp,"Unknown")) {
				/* check to see if a package exists in
				 * FS.
				 */
				if (_dtam_mntpt && (_dtam_flags & DTAM_MOUNTED)) {
					/* set up pathnames */		
					strcpy(pkginfo_path, _dtam_mntpt);
					strcat(pkginfo_path, "/*/pkginfo");
					strcpy(pkgmap_path, _dtam_mntpt);
					strcat(pkgmap_path, "/*/pkgmap");
					strcpy(install_path, _dtam_mntpt);
					strcat(install_path, "/install/INSTALL");
				}
				else {
					*mnt_pt = '/';
					if (strcmp(alias,"diskette1")==0)
						strcpy(mnt_pt+1, TXT_DISK_A);
        				else if (strcmp(alias,"diskette2")==0)
						strcpy(mnt_pt+1, TXT_DISK_B);
        				else if (strncmp(alias,"cdrom", 5)==0)
						sprintf(mnt_pt+1, "%s_%s", TXT_CDROM, alias+5);

					strcpy(pkginfo_path, mnt_pt);
					strcat(pkginfo_path, "/*/pkginfo");
					strcpy(pkgmap_path, mnt_pt);
					strcat(pkgmap_path, "/*/pkgmap");
					strcpy(install_path, mnt_pt);
					strcat(install_path, "/install/INSTALL");

					/* mount the FS */
					sprintf(mycmdbuf, "/sbin/tfadmin fmount -r -F %s %s %s 2> /dev/null", _dtam_fstyp, dev, mnt_pt);
					result = system(mycmdbuf);

					/* Can't mount? just return it is a file 
					 * system ??? 
					 */
					if (result != 0)
						return(DTAM_FS_TYPE);	
				}
				

				/* check to see if it is pkgadd format */
				sprintf(mycmdbuf, "/usr/bin/ls %s 2>/dev/null", pkginfo_path);
				result1 = system(mycmdbuf);
				sprintf(mycmdbuf, "/usr/bin/ls %s 2>/dev/null", pkgmap_path);
				result2 = system(mycmdbuf);	
				sprintf(mycmdbuf,"/sbin/tfadmin fumount %s 2> /dev/null", mnt_pt);
				if (result1 == 0 && result2 == 0) {
					if (!(_dtam_mntpt) || !(_dtam_flags & DTAM_MOUNTED)) 
						system(mycmdbuf);
					return(DTAM_FS_TYPE|DTAM_PACKAGE);
				}

				/* check to see if it is installpkg format */
				if (access(install_path, F_OK) == 0) {
					if (!(_dtam_mntpt) || !(_dtam_flags & DTAM_MOUNTED)) 
						system(mycmdbuf);
					return(DTAM_FS_TYPE|DTAM_INSTALL);
				}

				/* umount the mnt_pt */
				if (!(_dtam_mntpt) || !(_dtam_flags & DTAM_MOUNTED))
					system(mycmdbuf);

				/* if not all above, says it is file system */
				return(DTAM_FS_TYPE);
			}
		}
	}
/*
 *	if all tests fail, ...
 */
	return(DTAM_UNKNOWN);
}

/* diagonse.c end here */

/* mapfile.c start here */

DmMapfilePtr
Dm__mapfile(char *filename, int prot, int flags)
{
    int			fd;
    char *		fp;
    struct stat		st;
    DmMapfilePtr	mp;
    int			oflag;

    if (!filename)
	return(NULL);

    oflag = (prot == PROT_READ) ?  O_RDONLY : O_RDWR;

    errno = 0;
    if ((fd = open(filename, oflag)) == -1)
    {
	return(NULL);
    }

    /* get file size */
    if (fstat(fd, &st)) {
    err1:
	if (close(fd))
	    fprintf(stderr, "close() error (%d)", errno);
	return(NULL);
    }
	
    if (st.st_size == 0) {
	/* Not really an error, just nothing to map. */
	errno = 0;
	goto err1;
    }

    if ((fp = mmap((caddr_t)0, st.st_size, prot, flags, fd, 0)) == (char *)-1)
    {
	fprintf(stderr, "mmap error (%d)", errno);
	(void)close(fd);
	return(NULL);
    }
    if (close(fd))
	fprintf(stderr, "close() error (%d)", errno);

    if ((mp = (DmMapfilePtr)malloc(sizeof(DmMapfileRec))) == NULL)
    {
	fprintf(stderr, "Out of memory");
	return(NULL);
    }

    /* initialize header */
    mp->filesize = st.st_size;
    mp->endptr = fp + st.st_size;
    mp->curptr = mp->mapptr  = fp;
    mp->line = 1;
    return(mp);
}

void
Dm__unmapfile(mp)
DmMapfilePtr mp;
{
	if (mp) {
		munmap(mp->mapptr, mp->filesize);
		free(mp);
	}
}

/*
 * Look for a specific character, starting from the current position.
 */
char *
Dm__findchar(mp, ch)
DmMapfilePtr mp;
int ch;
{
	while (MF_NOT_EOF(mp) && (MF_PEEKC(mp) != ch))
		MF_NEXTC(mp);
	return(MF_GETPTR(mp));
}

/*
 * Look for any of many possible characters, starting from the current position.
 */
char *
Dm__strchr(mp, str)
DmMapfilePtr mp;
char *str;
{
	while (MF_NOT_EOF(mp) && !strchr(str, MF_PEEKC(mp)))
		MF_NEXTC(mp);
	return(MF_GETPTR(mp));
}

/*
 * Look for a substring.
 */
char *
Dm__strstr(mp, str)
DmMapfilePtr mp;
char *str;
{
	char *p;
	char *q;

	while (MF_NOT_EOF(mp)) {
		p = str;
		q = MF_GETPTR(mp);
	 	while (*p && *q == *p) {
			p++;
			if (q != MF_EOFPTR(mp))
				q++;
			else
				break;
		}

		if (!*p)
			/* found it */
			return(MF_GETPTR(mp));

		MF_NEXTC(mp);
	}
	return(NULL);
}

/* mapfile.c end here */
