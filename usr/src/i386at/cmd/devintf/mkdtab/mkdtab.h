#ident	"@(#)mkdtab.h	1.3"
#ident	"$Header$"

#define	ALIASMAX	16
#define	DESCMAX		128

/* generic devices for auto-density detect; at386 floppy disk driver only */
#define	FBDEV	"/dev/dsk/f%ldt"		  /* generic block device */
#define	FCDEV	"/dev/rdsk/f%ldt"		  /* generic char  device */

#define	FDESC1	"5.25 inch 360 Kbyte (Low Density)"	/* floppy type      */
#define	FBDEV1	"/dev/dsk/f%ldd9dt"		/* floppy block device mask */
#define	FCDEV1	"/dev/rdsk/f%ldd9dt"		/* floppy char device mask  */
#define	FDENS1	"mdens%dLOW"			/* floppy density mask/value*/
#define	FBLK1	702				/* number of blocks	    */
#define	FINO1	160				/* number of inodes	    */
#define	FBPC1	18				/* blocks per cyllinder	    */
#define	FLABEL1	"dtmedia:290^360 Kbyte"

#define	FDESC2	"5.25 inch 1.2 Mbyte (High Density)"
#define	FBDEV2	"/dev/dsk/f%ldq15dt"
#define	FCDEV2	"/dev/rdsk/f%ldq15dt"
#define	FDENS2	"mdens%dHIGH"
#define	FBLK2	2370
#define	FINO2	592
#define	FBPC2	30
#define	FLABEL2	"dtmedia:291^1.2 Mbyte"

#define	FDESC3	"3.5 inch 720 Kbyte (Low Density)"
#define	FBDEV3	"/dev/dsk/f%ld3dt"
#define	FCDEV3	"/dev/rdsk/f%ld3dt"
#define	FDENS3	"mdens%dLOW"
#define	FBLK3	1422
#define	FINO3	355
#define	FBPC3	18
#define	FLABEL3	"dtmedia:292^720 Kbyte"

#define	FDESC4	"3.5 inch 1.44 Mbyte (High Density)"
#define	FBDEV4	"/dev/dsk/f%ld3ht"
#define	FCDEV4	"/dev/rdsk/f%ld3ht"
#define	FDENS4	"mdens%dHIGH"
#define	FBLK4	2844
#define	FINO4	711
#define	FBPC4	36
#define	FLABEL4	"dtmedia:293^1.44 Mbyte"

#define	FDESC5	"5.25 inch 720 Kbyte (Medium Density)"
#define	FBDEV5	"/dev/dsk/f%ld5ht"
#define	FCDEV5	"/dev/rdsk/f%ld5ht"
#define	FDENS5	"mdens%dMED"
#define	FBLK5	1404
#define	FINO5	351
#define	FBPC5	18
#define	FLABEL5	"dtmedia:294^720 KByte"

#define	FDESC6	"3.5 inch 2.88 Mbyte (Extra Density)"
#define	FBDEV6	"/dev/dsk/f%ld3et"
#define	FCDEV6	"/dev/rdsk/f%ld3et"
#define	FDENS6	"mdens%dEXT"
#define	FBLK6	5688
#define	FINO6	1422
#define	FBPC6	72
#define	FLABEL6	"dtmedia:295^2.88 Mbyte"

/*
 values for mkdtab to use to identify 3.5" 1.2MB floppy densities added
 for Pacific market support.  Since these drives physically identify
 as "4" in the CMOS (same as 3.5" 1.44 MB floppy), we will use a
 virtual ID that falls outside the possible range of 8-bit CMOS ID
 values.
 */

#define	DRV_3M	256	/* 1.2 Mbytes (512 bytes/sector) */
#define	DRV_3N	257	/* 1.2 Mbytes (1024 bytes/sector) */


#define	FDESCM	"3.5 inch 1.2 Mbyte (512 bytes/sector)"
#define	FBDEVM	"/dev/dsk/f%ld3mt"
#define	FCDEVM	"/dev/rdsk/f%ld3mt"
#define	FDENSM	"mdens%dMEDIUM1"
#define	FBLKM	2370
#define	FINOM	592
#define	FBPCM	30
#define	FLABELM	"dtmedia:296^1.2 Mbyte (512 bytes/sector)"

#define	FDESCN	"3.5 inch 1.2 Mbyte (1024 bytes/sector)"
#define	FBDEVN	"/dev/dsk/f%ld3nt"
#define	FCDEVN	"/dev/rdsk/f%ld3nt"
#define	FDENSN	"mdens%dMEDIUM2"
#define	FBLKN	1185
#define	FINON	592
#define	FBPCN	30
#define	FLABELN	"dtmedia:297^1.2 Mbyte (1024 bytes/sector)"
