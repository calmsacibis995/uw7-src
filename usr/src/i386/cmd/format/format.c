/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)format:i386/cmd/format/format.c	1.1.6.9"
#ident  "$Header$"

char formatcopyright[] = "Copyright 1986 Intel Corp.";

/*
 * format.c
 *	Format utility for disks.  uses iSBC214-style ioctl calls.
 *	supportted by: iSBC 214
 *	This code is very dependent on the ioctl call behaviour.
 */

#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/iobuf.h>
#include <sys/vtoc.h>
#include <sys/fd.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>

long	lseek();
long	atol();
void	dev_check();

extern int	opterr;
extern char	*optarg;
extern int	optind;

extern	errno;

int	qflag		= 0;		/* "quiet" flag   */
int	vflag		= 0;		/* "verbose" flag */
int	verifyflag	= 0;		/* sector r/w verify flag */
int	exhaustive	= 0;		/* verify is exhaustive? */
int	write_bb	= 0;		/* need to write bb info?? */
int	dot_column;			/* this value is used to keep
					track of which column we will
					print the next dot in so that 
					if we are running the -v option,
					the dots still come out aligned.
					*/

char	*device;			/* The name of the device. */
int	devfd;				/* File descriptor for above. */
struct  disk_parms dp;                  /* Device description from driver */
uchar_t	fill_pattern;			/* Format fill pattern */

daddr_t	numtrk;				/* number of cylinders */

daddr_t	first_track = 0;			/* First track to format. */
daddr_t	last_track = -1;			/* Last track to format. */
ushort	interleave = 1;

char	*use =
":3:Usage: format [-f first] [-l last] [-i interleave]\n\t\t\t\t [-q] [-v] [-V] [-E] <dev>\n\t\t\t  NOTES:  -E implies -V\n";

/* Exit Codes */
#define	RET_OK		0	/* success */
#define	RET_USAGE	1	/* usage */
#define	RET_STAT	2	/* can't stat device */
#define	RET_OPEN	3	/* can't open device */
#define	RET_CHAR	4	/* device not character device */
#define	RET_FLOPPY	5	/* device not a floppy disk */
#define	RET_FORM0	6	/* formatted 0 tracks */

giveusage()
{
	pfmt(stderr, MM_ACTION, use);
	exit(RET_USAGE);
}

main(argc, argv)
int	argc;
char	**argv;
{

	register daddr_t track,tracks;
	int	leave_loop;
	ushort	sector;
	char	c;

	int	verify_status;
	int	track_count;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxformat");
	(void)setlabel("UX:format");

	if (argc == 1)
		giveusage();

	/*
	 * Decode/decipher the arguments.
	 */

	opterr = 0;
	while ((c = getopt(argc,argv,"f:l:i:qvVE")) != EOF)
		switch(c) {

		case 'f':	/* first track to format */
			first_track = atol (optarg);
			continue;
		case 'l':	/* last track to format */
			last_track = atol (optarg);
			continue;
		case 'i':	/* interleave */
			interleave = atol (optarg);
			continue;

		case 'q':	/* quiet mode */
			++qflag;
			continue;
			/*
			 * Verify/verbosity and other aberations.
			 */
		case 'v':			/* verbose */
			++vflag;
			break;
		case 'V':			/* sector r/w verify 'on' */
			++verifyflag;
			continue;
		case 'E':			/* Exhaustive verify */
			++verifyflag;		/* assumes verify */
			++exhaustive;
			continue;
		default:
			giveusage() ;
		}

	if (opterr)
		giveusage();

	if (optind < argc)
		device = argv[optind++];
	else
		giveusage();

	if (optind < argc)
		giveusage();

	/*
	 * Ask the driver what the device looks like.
	 */
	if ((devfd = open(device, 2 )) == -1) {
		if (errno == EROFS)
			pfmt(stderr, MM_ERROR, 
				":12:Cannot open %s, write-protected floppy.\n",
				device);
		else
			pfmt(stderr, MM_ERROR, ":4:Cannot open %s.\n", device);
		exit(RET_OPEN);
	}

	if(ioctl(devfd, V_GETPARMS, &dp) == -1) {
		pfmt(stderr, MM_ERROR,
			":5:Device must be a character device.\n");
		exit(RET_CHAR);
	}

	if (dp.dp_type != DPT_FLOPPY) {
		pfmt(stderr, MM_ERROR,
			":6:%s is not a floppy disk.\n", device);
		exit(RET_FLOPPY);
	}

	/* entry point for machine dependent device checks */
	dev_check();

	numtrk = dp.dp_cyls * dp.dp_heads;
	tracks = numtrk - (dp.dp_pstartsec/(unsigned int)dp.dp_sectors);
	fill_pattern = (uchar_t)dp.dp_ptag;

	/*
	 * If the last track was not specified, compute it.
	 */
	if (last_track == -1)
		last_track = dp.dp_pnumsec/(unsigned int)dp.dp_sectors - 1;
	if (last_track > (tracks-1)) {
		pfmt(stderr, MM_WARNING,
			":7:format limited to track: %lu\n",tracks);
		last_track = tracks;
	}

	sector = 0;
	if (!qflag)
		pfmt(stdout, MM_NOSTD, ":8:formatting");
	dot_column = 10;
	verify_status = 1;
	for(track=first_track, track_count = 0; track<=last_track; track++, track_count++) {
		if (!qflag)
			printdot(track);
		if(!fmtrack(track))
			break;
		if (verifyflag) {
			if((fill_pattern == (uchar_t)0xe5) || (fill_pattern == (uchar_t)0xf6)) {
				if (!rdtrack_ok(track, &sector)) {
					verify_status = 0;
					break;
				}
			} else {
				if (!track_ok(track, &sector)) {
					verify_status = 0;
					break;
				}
			}
		}
	}

	tracks = track - first_track;
	if(!qflag)
		printf ("\n");

	if (verify_status == 0) {
		pfmt(stderr, MM_ERROR,
			":13:Formatted %d out of %d tracks:\n",
			track_count, last_track+1);

		pfmt(stderr, MM_NOSTD,
			":14:Failed in write/read/compare verification on track %d.\n",
			track);
		close(devfd);
		exit(RET_FORM0);
	}

	if (tracks == 1) {
		if(!qflag)
			pfmt(stdout, MM_NOSTD,
				":9:Formatted 1 track: %lu, interleave %d.\n",
					first_track, interleave);
	}
	else if (tracks == 0) {
		pfmt(stderr, MM_ERROR,
			":10:Formatted 0 tracks: Please check floppy density\n");
		close(devfd);
		exit(RET_FORM0);
	}
	else if(!qflag)
		pfmt(stdout, MM_NOSTD,
			":11:Formatted %lu tracks: %lu thru %lu, interleave %d.\n",
				tracks, first_track, last_track, interleave);

	close(devfd);
	exit(RET_OK);
}
/**************************************************************************
 *		main ends here.  below are subroutines			  *
 **************************************************************************
 */

/*
* fmtrack
*       Format a track.
*
*/

fmtrack(track)
daddr_t	track;					/* partition relative */
{
	union io_arg  fmt;
	int fmtval;

	fmt.ia_fmt.start_trk = track;
	fmt.ia_fmt.num_trks = 1;
	fmt.ia_fmt.intlv = interleave;
	fmtval = ioctl(devfd, V_FORMAT, &fmt);
	return(fmtval == 0);
}


/*
 *	track_ok writes to a sector, reads from it, compares the values,
 *		if values mis-compare, return 0;
 *	Does single, random-sector testing when
 *		exhaustive is set to 0.
 */
track_ok (track_no, sector_p)
daddr_t	track_no;
ushort	*sector_p;
{

#define	BSIZE	BUFSIZ

	char	buffer[BSIZE];
	long	present;
	daddr_t	start, end;
	int	i;
	long	junk;
	register ushort secsiz;

	secsiz = dp.dp_secsiz;
	start = (daddr_t)track_no * dp.dp_sectors;

	if (!exhaustive) {
		start = start + (int) (rand() % (unsigned int)dp.dp_sectors);
		end = start + 1;
	}
	else {
		end = start + dp.dp_sectors;
	}

	for (present = start; present < end; present++) {
		for (i=0; i < (unsigned int)secsiz; i++) {
			buffer[i] = (char)0xa5;
		}
		junk = lseek (devfd, (long) (present * secsiz), 0);
		if ((junk = write (devfd, buffer, secsiz)) != secsiz) {
			*sector_p = present - start;
			return(0);
		}

		for (i=0; i < (unsigned int)secsiz; i++)
			buffer[i] = (char)0x5a;
		junk = lseek (devfd, (long) (present * secsiz), 0);
		if (read (devfd, buffer, secsiz) != secsiz) {
			*sector_p = present - start;
			return(0);
		}
		for (i=0; i < (unsigned int)secsiz; i++) {
			if (buffer[i] != (char)0xa5) {
				*sector_p = present - start;
				return(0);
			}
		}
	}
	return (1);
}


/*
 * printdot
 *	Put a dot on the screen
 */
printdot(track)
daddr_t	track;
{
	if (((track % (((unsigned int)dp.dp_heads * 5) * 40)) == 0) && (track != first_track)) {
		printf ("\n          ");
		dot_column = 10;
	}
	if ((track % ((unsigned int)dp.dp_heads * 5)) == 0) {
		printf (".");
		dot_column++;
		fflush(stdout);
	}
}

/*
 *	rdtrack_ok reads a sector, compares the values,
 *		if values mis-compare, return 0;
 *	Does single, random-sector testing when
 *		exhaustive is set to 0.
 */
rdtrack_ok (track_no, sector_p)
daddr_t	track_no;
ushort	*sector_p;
{

#define	BSIZE	BUFSIZ

	char	buffer[BSIZE];
	long	present;
	daddr_t	start, end;
	int	i;
	long	junk;
	register ushort secsiz;


	secsiz = dp.dp_secsiz;
	start = (daddr_t)track_no * dp.dp_sectors;

	if (!exhaustive) {
		start = start + (int) (rand() % (unsigned int)dp.dp_sectors);
		end = start + 1;
	}
	else {
		end = start + dp.dp_sectors;
	}

	for (present = start; present < end; present++) {
		for (i=0; i < (unsigned int)secsiz; i++) {
			buffer[i] = (char)fill_pattern;
		}
		junk = lseek (devfd, (long) (present * secsiz), 0);

		for (i=0; i < (unsigned int)secsiz; i++)
			buffer[i] = (char)0x5a;
		junk = lseek (devfd, (long) (present * secsiz), 0);
		if (read (devfd, buffer, secsiz) != secsiz) {
			*sector_p = present - start;
			return(0);
		}
		for (i=0; i < (unsigned int)secsiz; i++) {
			if (buffer[i] != (char)fill_pattern) {
				*sector_p = present - start;
				return(0);
			}
		}
	}
	return (1);
}
