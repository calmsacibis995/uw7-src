/*		copyright	"%c%" 	*/

#ident	"@(#)eac:i386/eaccmd/dosutil/disk.c	1.1.1.6"
#ident  "$Header$"

/*
 *	@(#) disk.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1984, 1985, 1986, 1987.
 *	Copyright (C) Microsoft Corporation, 1984, 1985, 1986, 1987.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */
/*
 *	Modification History
 *
 *	M000	barrys	Jul 17/84
 *	- Modified setup so that if the media descriptor is undefined, then
 *	  an assumption is made that the floppy disk being used is
 *	  earlier than DOS 2.0.  In this case, an attempt is made to
 *	  get the media byte from the first byte of the fat.  If the
 *	  first byte of the fat is a valid media type, then we make
 *	  an assumption about the rest of the disk parameters.
 *	  NOTE - Versions of DOS earlier than 2.0 do not support 
 *	  directories.  An appropriate flag is set so utilities attempting
 *	  to perform directory operations will be disallowed.
 *	M002	barrys	Aug 2/84
 *	- An attempt is made to ensure that the correct device has been
 *	  opened.  This is done only for 48 tpi diskettes.  When the
 *	  media byte is determined (and the diskette is 48 tpi), then
 *	  the present read device is closed and the device that
 *	  corresponds to the media byte is opened.
 *	M003	ericc	Dec 7, 1984			16 fit FAT Support
 *	- Rewrote the programs; corrected specifications in dos1_format,
 *	  included support for 16 bit FATs and hard disks.  No two programs
 *	  can access the same DOS disk concurrently.
 *	M004	ericc	Feb 13, 1985
 *	- Included information for 96tpi disks (media F9) in dos1_format[].
 *	  This accomodates 96tpi disks which do not have a proper boot sector.
 *	M005	May 1, 1985	ericc			Micro Floppy Support
 *	- Added support for Macintosh style 3.5 " floppy drives, which also
 *	  have a media byte of F9.
 *	M006	March 11, 1986	ericc			Single sided Floppies
 *	- DOS [123].* had a bug where single sided disks would be formatted
 *	  with bs->sectorspercluster incorrectly set to 2.  In such a case,
 *	  this field in the BPB is ignored, and assumed to be 1 instead.
 *	M007	October 14, 1986 gregj
 *	- add media code 0xfA in dos1_format[]
 *	M008	Oct 15, 1986 gregj
 *	- setup now does setuid(getuid()) to change to real ID. 
 *	L001	Apr  6, 1987 nigelt
 *	- added support for 96ds9 and 96ds18 3.5" micro floppies.
 *	L002	Aug 18, 1987 hughd
 *	- L001 corrected and merged into source for 2.2.2:
 *	  dos1_format[] table extended and commented
 *	M009	Sep 21, 1987	buckm
 *	- make the dosutils behave gracefully as setuid programs.
 *	- re-write openagain() to handle being called with device
 *	  names such as "/dev/install".
 */



#include	<stdio.h>
#include	<errno.h>
#include	<string.h>					/* M009 */
#include	"dosutil.h"

extern int Bps;

extern int  fd;				/* file descr of current DOS disk */
extern int  errno;						/* M009 */
extern char *realloc();						/* M009 */

static unsigned short effuid;		/* effective uid */	/* M009 */
static unsigned short realuid;		/* real uid */		/* M009 */
static char sectbuf[MAX_BPS];		/* buffer for reading sectors */


/*	L002: Comments on the dos1_format[] table by hughd.
	The entries for F0, F7, F8 were added for 3.5" 135tpi disks:
	no parameters are listed because they are read from the boot
	sector, so any particular values would be misleading (for example,
	there may be 1 or 2 sectors per cluster with the same media byte).
	PS2 and Xen-i DOS use F0 for 135ds18 disks, F9 for 135ds9 disks;
	the entries for F7 and F8 may be unnecessary - they are included
	because there is a rumour that Apricot Xen-i may also use them.  The
	parameters corresponding to F9 were removed as misleading, F9 disks
	with other parameters certainly exist (the M004 log suggests that
	these parameters were useful; but in fact they were not used, perhaps
	M005 disabled them again).  The only entries for which parameters are
	necessary are those for FC, FD, FE, FF - DOS 1.0 48tpi disks which
	had a media byte at the start of the fat, but no boot block parameters;
	otherwise the table is simply a list of allowable media bytes.
	Note that dosformat.c uses a table of its own, not this one.
 */
					/* characteristics of DOS 1.0 disks */
struct format dos1_format[] = {
/* fatsect dirsect sect/clust sect/disk reserved media  device */
	0,	0,	0,	0,	0,	0xF0,	"",	/* L002 */
	0,	0,	0,	0,	0,	0xF7,	"",	/* L002 */
	0,	0,	0,	0,	0,	0xF8,	"",	/* L002 */
	0,	0,	0,	0,	0,	0xF9,	"",	/* L002 */
	0,	0,	0,	0,	0,	0xFA,	"",
	2,	4,	1,	360,	0,	0xFC,	"48ss9",
	2,	7,	2,	720,	0,	0xFD,	"48ds9",
	2,	4,	1,	1232,	0,	0xFE,	"",
	1,	7,	2,	640,	0,	0xFF,	"48ds8",
	0,	0,	0,	0,	0,	0,	""
};


/*	checkmedia()  --  return FALSE if media descriptor byte is
 *			unrecognizable
 */

static checkmedia(media)
int media;
{
	struct format *p;

	p = dos1_format;
	while (p->f_media != 0){
		if ((p->f_media & 0xff) == (media & 0xff))
			return(TRUE);
		p++;
	}
	return(FALSE);
}



/* M009 */
/*	setup_perms()  --  Get the effective uid, save it, then set
 *		the effective uid to the real uid.  This routine should
 *		be called before any file system access.  Opener() will
 *		use the saved effective uid to open the DOS device
 *		if it cannot be opened using the real uid.
 *		This relies on the System V ability to switch back and
 *		forth from the saved effective uid to the real uid,
 *		so the dosutils should not be setuid if they are not
 *		being compiled for System V.
 */

setup_perms()
{
	effuid = geteuid();
	realuid = getuid();
	setuid(realuid);
}



/* M009 */
/*	opener()  --  First try to open dev using the real uid;
 *		if that fails, try again using the effective uid.
 *		Return the fd or -1.
 *
 *		dev   : the device to be opened
 *		oflag : file status flags used by open()
 */		

static int opener(dev,oflag)
char *dev;
int oflag;
{
	int f;

	if ((f = open(dev,oflag)) < 0){
		if (errno == EACCES){
			setuid(effuid);
			f = open(dev,oflag);
			setuid(realuid);
		}
	}
	return(f);
}



/* M009 */
/*	openagain()  --  If the device is a 48 tpi floppy, the device
 *		is reopened to ensure that the correct floppy disk
 *		minor number is used.  The name of the reopened device,
 *		or NULL if this was unnecessary, is returned.
 *
 *		dev    : device where the DOS disk is mounted
 *		oflag  : file status flags used by open()
 */		

static char *openagain(dev,oflag)
char *dev;
int oflag;
{
	char *newdev, *base, drive;
	struct format *p;
	struct stat open_dev;
	int	minor_no;

	/*
	 * find out if dev needs to be re-opened
	 */
	for (p = dos1_format; p->f_media != 0; p++)
		if ((p->f_media & 0xff) == (frmp->f_media & 0xff))
			break;
	if ((p->f_media == 0) || (*(p->f_device) == (char) NULL))
		return(NULL);

	/*
	 * going to re-open dev; close it now
	 */
	close(fd);

	/*
	 * find last component of dev
	 */
	base = basename(dev);

	/*
	 * Determine drive number by getting the minor number. even minor
	 * numbers are for drive 0, odd are for drive 1.
	 */
	stat(dev, &open_dev);
	minor_no = minor(open_dev.st_rdev);
	if  ((minor_no % 2) == 0 )
		drive = '0';
	else	
		drive = '1';
#ifdef DEBUG
	fprintf(stderr, "DEBUG	dev=%s st_rdev=%d minor_no=%d drive=%c\n",
				dev, open_dev.st_rdev,minor_no, drive);
#endif
	/*
	 * first try re-opening in the same directory
	 */
	newdev = malloc((base - dev) + 3 + strlen(p->f_device) + 1);
	sprintf(newdev, "%.*sfd%c%s", base - dev, dev, drive, p->f_device);
#ifdef DEBUG
	fprintf(stderr,"DEBUG reopening %s as %s\n",dev,newdev);
#endif
	if ((fd = opener(newdev,oflag)) >= 0)
		return(newdev);
	/*
	 * that failed; now try in /dev
	 */
	newdev = realloc(newdev, 8 + strlen(p->f_device) + 1);
	sprintf(newdev, "/dev/fd%c%s", drive, p->f_device);
#ifdef DEBUG
	fprintf(stderr,"DEBUG reopening %s as %s\n",dev,newdev);
#endif
	if ((fd = opener(newdev,oflag)) < 0){
		sprintf(errbuf,"can't open %s",newdev);
		fatal(errbuf,0);
	}
	return(newdev);
}

/*	readclust(), readsect()  --  read routines.  The file descriptor
 *		fd must already be set.  If impossible, return FALSE.
 *
 *	NOTES:	Readclust() reads up to cluster 65535; readsect() up to
 *		sector 65535.  The first sector on a disk is sector 0 !!!
 *		In IBM documentation, a dsdd floppy has sectors 0 to 27F.
 */

readclust(clustno,buffer)
unsigned clustno;
char *buffer;
{
	long posn;
	unsigned size;

	posn = ((clustno - FIRSTCLUST) * frmp->f_sectclust + segp->s_data)
			* (long) Bps;
	size = frmp->f_sectclust * Bps;

#ifdef DEBUG
	fprintf(stderr,"DEBUG reading cluster %u\toffset %ld (%u bytes)\n",
			clustno,posn,size);
#endif

	if ((lseek(fd,posn, 0)) == -1){
/*		perror("lseek");
 */		return(FALSE);
	}
	if (read(fd,buffer,size) != size){
/*		perror("read");
 */		return(FALSE);
	}
	return(TRUE);
}


readsect(sectno,buffer)
unsigned sectno;
char *buffer;
{
	if ((lseek(fd,(long) sectno * Bps, 0)) == -1){
 		return(FALSE);
	}
	if (read(fd,buffer,(unsigned) Bps) != Bps){
 		return(FALSE);
	}
	return(TRUE);
}


/*	readfat()  --  reads the FAT off the current DOS disk.  There are two
 *		contiguous copies of the FAT.  They are verified against each
 *		other.  The extern variable bigfat is set if FAT entries are
 *		16 bits.  If problems occur, FALSE is returned.
 */

readfat(fat)
char *fat;
{
	unsigned i;
	char *fat2, *p;
	extern int bigfat;

	bigfat = isbigfat();

	for (i = 0; i < frmp->f_fatsect; i++){
		if ( !readsect(segp->s_fat + i, (char *) (fat + (i * Bps))) )
			return(FALSE);
	}
	p = fat;
	for (i = 0; i < frmp->f_fatsect; i++){
		if ( !readsect(segp->s_fat + frmp->f_fatsect + i, sectbuf) )
			return(FALSE);

		for (fat2 = sectbuf; fat2 < &sectbuf[Bps]; p++, fat2++)
			if (*p != *fat2)
				return(FALSE);
	}
	return(TRUE);
}



/*	setup()  --  open the device, setting the external variable fd.  The
 *		device is interrogated, and the details filled into *frmp and
 *		*segp.  The name of the device actually opened is returned.
 *		If anything goes wrong, NULL is returned.
 *
 *		dev   :  the device to be opened
 *		oflag :  file status flags used by open()
 */

char *setup(dev,oflag)
char *dev;
int oflag;
{
	int media;
	char *newdev;

	if ((fd = opener(dev,oflag)) < 0){			/* M009 */
		sprintf(errbuf,"can't open %s",dev);
		fatal(errbuf,0);
		return(NULL);			/* read the boot sector in a */
	}					/* compiler-independent way  */
	if (!readsect(0,sectbuf)){
 		sprintf(errbuf,"can't read DOS boot sector off %s",dev);
		fatal(errbuf,0);
		return(NULL);
	}
	if (!whatdisk( (struct bootsector *) sectbuf)){
		sprintf(errbuf,"bad media byte on %s",dev);
		fatal(errbuf,0);
		return(NULL);
	}					/* try reopening the device */
	if (newdev = openagain(dev,oflag)){
		if (fd < 0)
			return(NULL);
		else
			dev = newdev;
	}
	if ((((media = frmp->f_media & 0xff) == 0xFC) || (media == 0xFE)) &&
	     (frmp->f_sectclust == 2)) {
		frmp->f_sectclust = 1;
	}
	if( media == 0xFE ) {
		frmp->f_dirsect = 6;
	}
#ifdef DEBUG
	fprintf(stderr,"\tsectors per FAT\t\t= %u\n",frmp->f_fatsect);
	fprintf(stderr,"\troot directory sectors\t= %u\n",frmp->f_dirsect);
	fprintf(stderr,"\tsectors per cluster\t= %u\n",frmp->f_sectclust);
	fprintf(stderr,"\tsectors per disk\t= %u\n",frmp->f_sectors);
	fprintf(stderr,"\tmedia descriptor\t= %2x\n",media);
#endif
					/* two FATs, starting at 2nd sector */
	segp->s_fat      = 1;
	segp->s_dir      = segp->s_fat + (2 * frmp->f_fatsect);
	segp->s_data     = segp->s_dir + frmp->f_dirsect;
	frmp->f_clusters = ((frmp->f_sectors - segp->s_data)
					/ frmp->f_sectclust) + FIRSTCLUST - 1;
	return(dev);
}


/*	whatdisk()  --  fills *frmp with some details of the DOS disk.  First,
 *		the BPB in the boot sector is examined.  If the media descriptor
 *		is unrecognizable, the first byte of the FAT is scrutinized.
 *		Pre-DOS 2.0 disks do not have a media descriptor in the BPB.
 *		Hence, a guess of the DOS disk format is made based on this
 *		byte.  If both are unrecognizble, FALSE is returned.
 */

static whatdisk(bs)
struct  bootsector *bs;
{
	extern int dirflag;			/* DOS directories supported? */
	struct format *p;

	if ( checkmedia(frmp->f_media = (int) bs->mediadescriptor) ){

		frmp->f_fatsect   = word(bs->sectorsperFAT);
		frmp->f_dirsect   = (word(bs->rootdirmax) * DIRBYTES) / Bps;
		frmp->f_sectclust = (unsigned) bs->sectorspercluster;
		frmp->f_sectors   = word(bs->totalsectors);
		if((word(bs->totalsectors)) != 0){
			frmp->f_sectors   = word(bs->totalsectors);
		} else {
			frmp->f_sectors   = bs->newtotalsectors;
		}
		dirflag           = TRUE;
		return(TRUE);
	}
	if (!readsect(1,sectbuf))		/* pre-DOS 2.0; examine FAT */
		return(FALSE);

	for (p = dos1_format; p->f_media != 0; p++)

		if (((p->f_media & 0xff) == (sectbuf[0] & 0xff)) &&
		    (*(p->f_device) != (char) NULL)){

			frmp->f_fatsect   = p->f_fatsect;
			frmp->f_dirsect   = p->f_dirsect;
			frmp->f_sectclust = p->f_sectclust;
			frmp->f_sectors   = p->f_sectors;
			frmp->f_media     = p->f_media;
			dirflag           = FALSE;
			return(TRUE);
		}
	return(FALSE);
}
