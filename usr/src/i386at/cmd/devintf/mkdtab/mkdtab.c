#ident	"@(#)mkdtab.c	1.20"
#ident	"$Header$"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<devmgmt.h>
#include	<sys/mkdev.h>
#include	<sys/cram.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<sys/vtoc.h>
#include	<sys/vfstab.h>
#include	"mkdtab.h"
#include	<sys/types.h>
#include	<errno.h>
#include	<mac.h>

/*
 * Update device.tab and dgroup.tab to reflect current configuration.
 * Designed so it can be run either once at installation time or after
 * every reboot.  The alias naming scheme used is non-intuitive but
 * is consistent with existing conventions and documentation and with
 * the device numbering scheme used by the disks command.
 * Code borrowed liberally from prtconf, disks and prtvtoc commands.
 */

static struct dpart {
	char	alias[ALIASMAX];
	char	cdevice[MAXPATHLEN];
	char	bdevice[MAXPATHLEN];
	char	cdevlist[MAXPATHLEN];
	char	bdevlist[MAXPATHLEN];
	long	capacity;
} dparttab;

static int		vfsnum;
static char		putdev_d[2048];
static char		putdevcmd[2048];
static char		cmd[MAXPATHLEN];
static struct vfstab	*vfstab;
static	int		macinstalled;
unsigned char		crambuf[2];

static void		fdisk(), initialize(), mkdgroups();
static char		*memstr();
#define SECATTRS "mode=static state=private range=SYS_RANGE_MAX-SYS_RANGE_MIN startup=no ual_enable=yes other=\">y\" "

main(argc, argv)
int	argc;
char	**argv;
{
	int			i, fd;
	level_t			level;

	strcpy(cmd, argv[0]);

	if (lvlin(SYS_PUBLIC, &level) == 0) {
		macinstalled = 1;
	}

	/* initialize vfstab in memory */

	if (argc >=2)	/* since only one argument, no need for checks/cases */
		initialize(1);
	else
		initialize(0);

	/*
	 * For AT386, we can only use CRAM to determine how many floppy
	 * disks are configured on the system and their types. The CRAM
	 * configuration information may only be altered via the "set-up"
	 * floppy.
	 *
	 * Using CRAM and the vfstab information, construct device.tab
	 * information for the floppy disks.
	 *
	 */

	if ((fd = open("/dev/cram", O_RDONLY)) < 0) {
		fprintf (stderr, "%s: Can't open /dev/cram\n", cmd);
		exit(1);
	}

	crambuf[0] = DDTB;
	ioctl(fd, CMOSREAD, crambuf);

	/* only two integral devices possible */

	fdisk(((crambuf[1] >> 4) & 0x0F),0,0);	/* Drive 0 alias diskette1 */
	fdisk((crambuf[1]        & 0x0F),1,0);	/* Drive 1 alias diskette2 */

	if (macinstalled == 0) {
		/*
		 * There's no need to do this more than once per installation.
		 * Let's assume that if we're running ES, we've
		 * already booted at least once (prior to installing ES)
		 * and put the device names in /etc/dgroup.tab.
		 */

		/*
		 * Update the dgroup.tab file.
		 */
		mkdgroups();
	}
}


/*
 * Add device table entry for the floppy drive.
 */

static void
fdisk(drvtyp, num, mflag)			/* at386 specific */
int	drvtyp;
int	num;
int	mflag;
{
	char	desc[DESCMAX]="";	   /* "desc=" table keyword value  */
	char	mdenslist[DESCMAX]="";	   /* floppy density keyword value */
	char	medialabel[DESCMAX]="";	   /* floppy density keyword value */
	char	tbuf[DESCMAX]="";	   /* scratch buffer		   */
	char	mdensdefault[ALIASMAX]=""; /* floppy density default value */
	char	mediatype[ALIASMAX]="";	   /* "type=" keyword value	   */
	char	display[ALIASMAX]="";	   /* "display=" keyword value	   */
	char	secalias[ALIASMAX]="";	   /* "secdev=" keyword value	   */
	char	fmtcdevice[MAXPATHLEN]=""; /* need for spcl case 2 and 4   */
	int	blocks, inodes, cyl;	   /* more keyword values	   */
	int	multiflag = mflag;	   /* is set for flops that work   */
					   /* at multiple densities	   */
	char	*cptr;

	switch (drvtyp) {
		case 0:
			return;

		case 1:
			/* 5.25 360 KB Floppy Disk */
			sprintf(dparttab.bdevice, FBDEV1, num);
			sprintf(dparttab.cdevice, FCDEV1, num);
			strcpy(fmtcdevice, dparttab.cdevice);
			blocks=FBLK1;
			inodes=FINO1;
			cyl=FBPC1;

			if (!multiflag) {
				sprintf(desc,  "Floppy Drive %d", num+1);
				strcpy(mediatype, "diskette");
				strcpy(display, "true");
				sprintf(dparttab.alias, "diskette%d", num+1);
			}
			else {
	/* if special mdens case */
				multiflag=0;
				strcpy(desc, FDESC1);
				strcpy(mediatype, "mdens");
				strcpy(display, "false");
				sprintf(dparttab.alias, FDENS1, num+1);
				sprintf(medialabel,"mdenslabel=\"%s\"",FLABEL1);
			}
			break;

		case 2:
			/* 5.25 1.2 MB Floppy Disk */
			blocks=FBLK2;
			inodes=FINO2;
			cyl=FBPC2;

			if (!multiflag) {
				sprintf(dparttab.bdevice, FBDEV, num);
				sprintf(dparttab.cdevice, FCDEV, num);
				sprintf(fmtcdevice, FCDEV2, num);
				sprintf(desc,  "Floppy Drive %d", num+1);
				strcpy(mediatype, "diskette");
				strcpy(display, "true");
				sprintf(dparttab.alias, "diskette%d", num+1);
	/*
	 * Case 2 hardware is special for the AT&T at386
	 * It can behave like types 1, 2 or 5. Type 2 is default.
	 */
				multiflag=2;
				sprintf(tbuf,"\"mdenslist=%s,%s\"", FDENS2, FDENS1);
				sprintf(mdenslist, tbuf, num+1, num+1, num+1);
				sprintf(tbuf,"\"mdensdefault=%s\"", FDENS2);
				sprintf(mdensdefault, tbuf, num+1);
			}

			else {
	/* if special mdens case */
				sprintf(dparttab.bdevice, FBDEV2, num);
				sprintf(dparttab.cdevice, FCDEV2, num);
				sprintf(fmtcdevice, FCDEV2, num);
				multiflag=0;
				strcpy(desc, FDESC2);
				strcpy(mediatype, "mdens");
				strcpy(display, "false");
				sprintf(dparttab.alias, FDENS2, num+1);
				sprintf(medialabel,"mdenslabel=\"%s\"",FLABEL2);
			}
			break;

		case 3:
			/* 3.5 720 KB Floppy Disk */
			sprintf(dparttab.bdevice, FBDEV3, num);
			sprintf(dparttab.cdevice, FCDEV3, num);
			strcpy(fmtcdevice, dparttab.cdevice);
			blocks=FBLK3;
			inodes=FINO3;
			cyl=FBPC3;

			if (!multiflag) {
				sprintf(desc,  "Floppy Drive %d", num+1);
				strcpy(mediatype, "diskette");
				strcpy(display, "true");
				sprintf(dparttab.alias, "diskette%d", num+1);
			}
			else {
		/* if special mdens case */
				multiflag=0;
				strcpy(desc, FDESC3);
				strcpy(mediatype, "mdens");
				strcpy(display, "false");
				sprintf(dparttab.alias, FDENS3, num+1);
				sprintf(medialabel,"mdenslabel=\"%s\"",FLABEL3);
			}
			break;

		case 4: /* This is CMOS ID for 3.5" 1.44 MB Drive */
			blocks=FBLK4;
			inodes=FINO4;
			cyl=FBPC4;

			if (!multiflag) {
				sprintf(dparttab.bdevice, FBDEV, num);
				sprintf(dparttab.cdevice, FCDEV, num);
				sprintf(fmtcdevice, FCDEV4, num);
				sprintf(desc,  "Floppy Drive %d", num+1);
				strcpy(mediatype, "diskette");
				strcpy(display, "true");
				sprintf(dparttab.alias, "diskette%d", num+1);
	/*
	 * Case 4 hardware is special for the AT&T at386
	 * It can behave like types 3 or 4. Type 4 is default.
	 */
				multiflag=4;
				sprintf(tbuf,"\"mdenslist=%s,%s,%s,%s\"", FDENS4, FDENS3,FDENSM,FDENSN);
				sprintf(mdenslist, tbuf, num+1, num+1, num+1,num+1,num+1);
				sprintf(tbuf,"\"mdensdefault=%s\"", FDENS4);
				sprintf(mdensdefault, tbuf, num+1);
			}

			else {
	/* if special mdens case */
				sprintf(dparttab.bdevice, FBDEV4, num);
				sprintf(dparttab.cdevice, FCDEV4, num);
				sprintf(fmtcdevice, FCDEV4, num);
				multiflag=0;
				strcpy(desc, FDESC4);
				strcpy(mediatype, "mdens");
				strcpy(display, "false");
				sprintf(dparttab.alias, FDENS4, num+1);
				sprintf(medialabel,"mdenslabel=\"%s\"",FLABEL4);
			}
			break;

		case 5:
		case 6: /* This is CMOS ID for 2.88 MB Drive */
			/* Default format for diskette1 will be 1.44 mB, though */
			if (!multiflag) {
	/*
	 * Case 6 hardware is special.
	 * It can behave like types 3, 4 or 6. Type 4 is default.
	 */
				blocks=FBLK4;
				inodes=FINO4;
				cyl=FBPC4;
				sprintf(dparttab.bdevice, FBDEV, num);
				sprintf(dparttab.cdevice, FCDEV, num);
				/* default format will be 1.44 MB */
				sprintf(fmtcdevice, FCDEV4, num);
				sprintf(desc,  "Floppy Drive %d", num+1);
				strcpy(mediatype, "diskette");
				strcpy(display, "true");
				sprintf(dparttab.alias, "diskette%d", num+1);
				sprintf(dparttab.bdevice, FBDEV, num);
				sprintf(dparttab.cdevice, FCDEV, num);
				multiflag=6;
				sprintf(tbuf,"\"mdenslist=%s,%s,%s,%s,%s\"", FDENS6, FDENS4, FDENS3,FDENSM,FDENSN);
				sprintf(mdenslist, tbuf, num+1, num+1, num+1,num+1,num+1,num+1);
				sprintf(tbuf,"\"mdensdefault=%s\"", FDENS4);
				sprintf(mdensdefault, tbuf, num+1);
			}

			else {
	/* if special mdens case */
				blocks=FBLK6;
				inodes=FINO6;
				cyl=FBPC6;
				sprintf(dparttab.bdevice, FBDEV6, num);
				sprintf(dparttab.cdevice, FCDEV6, num);
				sprintf(fmtcdevice, FCDEV6, num);
				multiflag=0;
				strcpy(desc, FDESC6);
				strcpy(mediatype, "mdens");
				strcpy(display, "false");
				sprintf(dparttab.alias, FDENS6, num+1);
				sprintf(medialabel,"mdenslabel=\"%s\"",FLABEL6);
			}
			break;

		case DRV_3M:
			if (!multiflag) /* only create "mdens" entry */
				return;
			blocks=FBLKM;
			inodes=FINOM;
			cyl=FBPCM;
			sprintf(dparttab.bdevice, FBDEVM, num);
			sprintf(dparttab.cdevice, FCDEVM, num);
			sprintf(fmtcdevice, FCDEVM, num);
			multiflag=0;
			strcpy(desc, FDESCM);
			strcpy(mediatype, "mdens");
			strcpy(display, "false");
			sprintf(dparttab.alias, FDENSM, num+1);
			sprintf(medialabel,"mdenslabel=\"%s\"",FLABELM);
			break;

		case DRV_3N:
			if (!multiflag) /* only create "mdens" entry */
				return;
			blocks=FBLKN;
			inodes=FINON;
			cyl=FBPCN;
			sprintf(dparttab.bdevice, FBDEVN, num);
			sprintf(dparttab.cdevice, FCDEVN, num);
			sprintf(fmtcdevice, FCDEVN, num);
			multiflag=0;
			strcpy(desc, FDESCN);
			strcpy(mediatype, "mdens");
			strcpy(display, "false");
			sprintf(dparttab.alias, FDENSN, num+1);
			sprintf(medialabel,"mdenslabel=\"%s\"",FLABELN);
			break;


		default:
			fprintf(stderr, "%s: Warning: type %d type from CRAM for floppy drive %d is unknown.\n", cmd, drvtyp, num);
			return;
			break;
	}

	/*
	 * set up the bdevlist and cdevlist putdev arguments to
	 * match the bdevice and cdevice arguments, without the
	 * trailing "t".
	 */
	strcpy(dparttab.cdevlist, dparttab.cdevice);
	cptr = dparttab.cdevlist + strlen(dparttab.cdevlist) - 1;
	*cptr = '\0';
	strcpy(dparttab.bdevlist, dparttab.bdevice);
	cptr = dparttab.bdevlist + strlen(dparttab.bdevlist) - 1;
	*cptr = '\0';
	if (macinstalled) {
		sprintf(secalias, "diskette%d", num+1);
		snprintf(putdevcmd, sizeof(putdevcmd),
		"/usr/bin/putdev -a %s secdev=%s %s \
cdevice=%s bdevice=%s cdevlist=%s bdevlist=%s \
desc=\"%s\" mountpt=/install volume=diskette type=%s display=%s removable=true \
capacity=%d fmtcmd=\"/usr/sbin/format -v %s\" \
erasecmd=\"/usr/sadm/sysadm/bin/floperase %s\" copy=true mkdtab=true \
mkfscmd=\"/sbin/mkfs -F s5 %s %d:%d 2 %d\" \
mkvxfscmd=\"/sbin/mkfs -F vxfs -o ninode=%d %s %d\" %s %s %s",
		dparttab.alias, secalias, (mflag == 0 ? SECATTRS : ""),
		dparttab.cdevice, dparttab.bdevice, dparttab.cdevlist,
		dparttab.bdevlist, desc, mediatype, display, blocks,
		fmtcdevice, dparttab.cdevice,
		dparttab.bdevice, blocks, inodes, cyl, /* for s5 mkfs */
		inodes, dparttab.bdevice, blocks,      /* for vxfs mkfs */
		mdenslist, mdensdefault, medialabel);
	} else {
		snprintf(putdevcmd, sizeof(putdevcmd),
		"/usr/bin/putdev -a %s \
cdevice=%s bdevice=%s cdevlist=%s bdevlist=%s \
desc=\"%s\" mountpt=/install volume=diskette type=%s display=%s removable=true \
capacity=%d fmtcmd=\"/usr/sbin/format -v %s\" \
erasecmd=\"/usr/sadm/sysadm/bin/floperase %s\" copy=true mkdtab=true \
mkfscmd=\"/sbin/mkfs -F s5 %s %d:%d 2 %d\" \
mkvxfscmd=\"/sbin/mkfs -F vxfs -o ninode=%d %s %d\" %s %s %s",
		dparttab.alias, dparttab.cdevice, dparttab.bdevice,
		dparttab.cdevlist,dparttab.bdevlist,
		desc, mediatype, display, blocks, fmtcdevice,
		dparttab.cdevice, dparttab.bdevice, 
		blocks, inodes, cyl, /* for s5 mkfs */
		inodes, dparttab.bdevice, blocks,      /* for vxfs mkfs */
		mdenslist, mdensdefault, medialabel);
	}
	snprintf(putdev_d, sizeof(putdev_d),
		"/usr/bin/putdev -d %s >/dev/null 2>&1", dparttab.alias);
	(void)system(putdev_d);
	(void)system(putdevcmd);


/*
 * If the floppy device is a multi-density device, we must now
 * add the type=mdens entries to device.tab.  We null the values
 * of mdenslist and mdensdefault as they are not appropriate as
 * subdevice values. Fdisk() is recursive (for now).
 */

	if (multiflag) {
		strcpy(mdenslist,"");
		strcpy(mdensdefault,"");

		switch(multiflag) {
			case 2:
				fdisk(2, num, multiflag);	/* HIGH	*/
				fdisk(5, num, multiflag);	/* MED	*/
				fdisk(1, num, multiflag);	/* LOW	*/
				break;

			case 4:
				fdisk(4, num, multiflag);	/* HIGH	*/
				fdisk(3, num, multiflag);	/* LOW	*/
				fdisk(DRV_3M, num, multiflag);	/* MEDIUM1 */
				fdisk(DRV_3N, num, multiflag);	/* MEDIUM2 */
				break;

			case 6:
				fdisk(6, num, multiflag);	/* EXTRA */
				fdisk(4, num, multiflag);	/* HIGH	*/
				fdisk(3, num, multiflag);	/* LOW	*/
				fdisk(DRV_3M, num, multiflag);	/* MEDIUM1 */
				fdisk(DRV_3N, num, multiflag);	/* MEDIUM2 */
				break;
		}
	}

	return;
}


static void
initialize(flag)
int	flag;			/* if true, re-initialize the table */
{
	FILE		*fp;
	int		i;
	struct vfstab	vfsent;
	char		**olddevlist;
	char		**lastdevlist;

	char	*criteria[] = {
			"type=diskette",
			"type=mdens",
			(char *)NULL
		};
	char	*mkdtabcriteria[] = {
			"mkdtab=true",
			(char *)NULL
		};
	char	*mdenscriteria[] = {
			"mkdtab=true",
			"type=mdens",
			(char *)NULL
		};

	/*
	 * Build a copy of vfstab in memory for later use.
	 */
	if ((fp = fopen("/etc/vfstab", "r")) == NULL) {
		fprintf(stderr,
		    "%s: can't update device tables:Can't open /etc/vfstab\n",
		     cmd);
		exit(1);
	}

	/*
	 * Go through the vfstab file once to get the number of entries so
	 * we can allocate the right amount of contiguous memory.
	 */
	vfsnum = 0;
	while (getvfsent(fp, &vfsent) == 0)
		vfsnum++;
	rewind(fp);

	if ((vfstab = (struct vfstab *)malloc(vfsnum * sizeof(struct vfstab)))
	    == NULL) {
		fprintf(stderr,"%s: can't update device tables:Out of memory\n",
		    cmd);
		exit(1);
	}

	/*
	 * Go through the vfstab file one more time to populate our copy in
	 * memory.  We only populate the fields we'll need.
	 */
	i = 0;
	while (getvfsent(fp, &vfsent) == 0 && i < vfsnum) {
		if (vfsent.vfs_special == NULL)
			vfstab[i].vfs_special = NULL;
		else
			vfstab[i].vfs_special = memstr(vfsent.vfs_special);
		if (vfsent.vfs_mountp == NULL)
			vfstab[i].vfs_mountp = NULL;
		else
			vfstab[i].vfs_mountp = memstr(vfsent.vfs_mountp);
		if (vfsent.vfs_fstype == NULL)
			vfstab[i].vfs_fstype = NULL;
		else
			vfstab[i].vfs_fstype = memstr(vfsent.vfs_fstype);
		i++;
	}
	(void)fclose(fp);

	/*
	 * If the "-f" flag is passed to mkdtab, remove all current entries
	 * of type disk, dpart, ctape, diskette, and mdens from the device
	 * and device group tables. Otherwise, we will only remove the
	 * entries populated by the mkdtab command. This will preserve
	 * entries created by the user or add-ons like SCSI on each boot.
	 *
	 * Any changes made manually since the last time this command
	 * was run will be lost.  Note that after this we are committed
	 * to try our best to rebuild the tables (i.e. the command
	 * should try not to fail completely after this point).
	 */

	if (flag)
		olddevlist = getdev((char **)NULL, criteria, 0);
	else
		olddevlist = getdev((char **)NULL, mdenscriteria,
					DTAB_ANDCRITERIA);

	_enddevtab();	/* getdev() should do this but doesn't */

	/*
	 * First delete the "mdens" entries", then the "diskette1"
	 * entry.  You can't delete the diskette1 entry until all
	 * other entries utilizing diskette1 as the secdev have first
	 * been deleted.
	 */
	for (i = 0; olddevlist[i] != (char *)NULL; i++) {
		(void)sprintf(putdevcmd,"/usr/bin/putdev -d %s 2>/dev/null",
			olddevlist[i]);
		(void)system(putdevcmd);
	}
	if (!flag) {
		lastdevlist = getdev((char **)NULL, mkdtabcriteria, 0);
		for (i = 0; lastdevlist[i] != (char *)NULL; i++) {
			(void)sprintf(putdevcmd,
				"/usr/bin/putdev -d %s 2>/dev/null",
				lastdevlist[i]);
			(void)system(putdevcmd);
		}
	}

	if (macinstalled == 0) {
		/*
		 * There's no need to do this more than once per installation.
		 * Let's assume that if we're running ES, we've
		 * already booted at least once (prior to installing ES)
		 * and put the device names in /etc/dgroup.tab.
		 */

		for (i = 0; olddevlist[i] != (char *)NULL; i++) {
			(void)sprintf(putdevcmd,
				"/usr/bin/putdgrp -d %s 2>/dev/null",
				olddevlist[i]);
			(void)system(putdevcmd);
		}
		for (i = 0; lastdevlist[i] != (char *)NULL; i++) {
			(void)sprintf(putdevcmd,
				"/usr/bin/putdgrp -d %s 2>/dev/null",
				lastdevlist[i]);
			(void)system(putdevcmd);
		}
	}
}

/*
 * Update the dgroup.tab file with information from the updated device.tab.
 */
static void
mkdgroups()
{
	int	i;
	char	*criteria[2];
	char	**devlist;

	criteria[1] = (char *)NULL;

	criteria[0] = "type=disk";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp disk");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);

	criteria[0] = "type=dpart";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp dpart");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);

	criteria[0] = "type=ctape";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp ctape");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);

	criteria[0] = "type=diskette";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp diskette");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);

	criteria[0] = "type=mdens";
	devlist = getdev((char **)NULL, criteria, DTAB_ANDCRITERIA);
	sprintf(putdevcmd, "/usr/bin/putdgrp mdens");
	for (i = 0; devlist[i] != (char *)NULL; i++) {
		strcat(putdevcmd, " ");
		strcat(putdevcmd, devlist[i]);
	}
	if (i != 0)
		(void)system(putdevcmd);
}

static char *
memstr(str)
register char	*str;
{
	register char	*mem;

	if ((mem = (char *)malloc((uint_t)strlen(str) + 1)) == NULL) {
		fprintf(stderr,"%s: can't update device tables:Out of memory\n",
		    cmd);
		exit(1);
	}
	return(strcpy(mem, str));
}

