#ident	"@(#)OSRcmds:dd/dd.c	1.1"
#pragma comment(exestr, "@(#) dd.c 26.2 96/01/05 ")
/*
 *	      UNIX is a registered trademark of AT&T
 *		Portions Copyright 1976-1989 AT&T
 *	Portions Copyright 1980-1989 Microsoft Corporation
 *   Portions Copyright 1983-1996 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)dd:dd.c	1.5" */

/*
 *	convert and copy
 *
 *	Modification History
 *	S000	30 May 89	scol!nitin
 *	- Unix Internationalisation
 *	used ctype(S) macros to be safe in number()
 *	S001	5 June 89	scol!nitin
 *	-using toupper() & tolower() macros instead of table
 *	entries if INTL
 *	L002	28 March 91	scol!panosd
 *	- Fixed wrong statistics message (1+0 records out)
 *	  when trying to "dd" to a write protected outputfile.
 *	L003	25 Feb 92	scol!anthonys
 *	- Added notrunc option for POSIX 1003.2 compliance
 *	- Modified the default behaviour to truncate the output
 *	  file. Old behaviour was to not truncate, which is now
 *	  provided when using the notrunc option. This change is
 *	  explicitly required by POSIX 1003.2/XPG4.
 *	L004	13 Aug 92	scol!anthonys
 *	- Added message catalogues.
 *	- Altered to use of the "blf" error reporting routines.
 *	  (unmarked)
 *	- Removed an array that isn't used.
 *	L005	9 Dec  92	scol!anthonys
 *	- Some of the error reporting code assumed that errno
 *	  had been set. Fixed.
 *	- Corrected the code reporting "records out" for the
 *	  case that a write() only writes some of the data.
 *	L006	06oct93		scol!hughd
 *	- awkwardly, added conv=immap,ommap,mmap options: to let input,
 *	  output or both be memory mapped: for demonstration and testing
 *	  (odd restrictions so I don't waste too much time on this)
 *	- includes an application for zero-length mappings: binary
 *	  chop to discover the size a block device: function fsize()
 *	- reworked L003's file truncation: if output is mapped, then it's
 *	  best to truncate first (to avoid reading in old data on faults),
 *	  then extend (to avoid SIGBUS); but otherwise (assuming reasonable
 *	  block size) it's best to leave truncation to the end (to avoid
 *	  reallocating blocks) - do it in term() to catch interrupts
 *	- go the COPY route if ibs equals obs: before, entering ibs and
 *	  obs separately was treated less efficiently than setting bs
 *	L007	22oct93		scol!hughd
 *	- my idea of how to handle SIGBUS (on unexpected EOF) was naive,
 *	  and would simply have caused dd to hang: use setjmp and longjmp
 *	- show a message if this happens: sorry, we don't know whether the
 *	  problem was with input or output, nor quite how much we've done
 *	- the COPY IMMAP not OMMAP|SWAB|SYNC case forgot to step along the
 *	  output buffer (= mapped input buffer in that case) after SIGBUS
 *	L008	17jan94		scol!hughd
 *	- for efficiency in raw disk transfers, align buffer on sector boundary
 *	L009	24mar94		scol!stephenp
 *	- modified to support "bmode" conversion option signifying file I/O
 *	  should use block mode if possible (if block size divisible by
 *	  NBPSCTR and if target file supports block mode).
 *	L010	01may94		scol!hughd
 *	- fsize()'s insistence on regular file or block device prevented
 *	  /dev/zero from being mapped: we don't know here which character
 *	  device is /dev/zero, so fsize() permit any character device if a
 *	  count has been specified - mmap() will fail if it's not /dev/zero
 *	L011	07sep95		scol!hughd
 *	- stop "Invalid reference" notice under load: up to NBPSCTR may have
 *	  been added to the malloc() address by the time it comes to check it
 *	L012	02 Jan 1996	scol!ashleyb
 *	- With conv=sync and either conv=block or conv=unblock, we should pad
 *	  with space characters rather than nulls.
 */

#include	<stdio.h>
#include	<signal.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/errno.h>				/* L006 */
#include	<sys/mman.h>				/* L006 */
#include	<unistd.h>				/* L006 */
#include	<memory.h>				/* L006 */
#include	<malloc.h>				/* L006 */
#include	<setjmp.h>				/* L007 */
#include	<fcntl.h>
#include	<sys/stat.h>				/* L004 begin */
/* #include	<errormsg.h> */
#ifdef INTL						/* S000 begin */
#include	<ctype.h>
#include	<locale.h>
#include	"dd_msg.h"
#else
#  define MSGSTR(num,str) (str)
#  define MSGSTR_SET(set,num,str) (str)
#endif /* INTL */					/* S000, L004 end */
#include	"../include/osr.h"

/* BSIZE must be defined here for the b size suffix to work correctly. */
#undef	BSIZE
#define	BSIZE	512

/* The BIG parameter is machine dependent.  It should be a long integer	*/
/* constant that can be used by the number parser to check the validity	*/
/* of numeric parameters.  On 16-bit machines, it should probably be	*/
/* the maximum unsigned integer, 0177777L.  On 32-bit machines where	*/
/* longs are the same size as ints, the maximum signed integer is more	*/
/* appropriate.  This value is 017777777777L.				*/

#define	BIG	017777777777L

/* Option parameters */

#define COPY		0	/* file copy, preserve input block size */
#define	REBLOCK		1	/* file copy, change block size */
#define	LCREBLOCK	2	/* file copy, convert to lower case */
#define	UCREBLOCK	3	/* file copy, convert to upper case */
#define NBASCII		4	/* file copy, convert from EBCDIC to ASCII */
#define LCNBASCII	5	/* file copy, EBCDIC to lower case ASCII */
#define UCNBASCII	6	/* file copy, EBCDIC to upper case ASCII */
#define NBEBCDIC	7	/* file copy, convert from ASCII to EBCDIC */
#define LCNBEBCDIC	8	/* file copy, ASCII to lower case EBCDIC */
#define UCNBEBCDIC	9	/* file copy, ASCII to upper case EBCDIC */
#define NBIBM		10	/* file copy, convert from ASCII to IBM */
#define LCNBIBM		11	/* file copy, ASCII to lower case IBM */
#define UCNBIBM		12	/* file copy, ASCII to upper case IBM */
#define	UNBLOCK		13	/* convert blocked ASCII to ASCII */
#define	LCUNBLOCK	14	/* convert blocked ASCII to lower case ASCII */
#define	UCUNBLOCK	15	/* convert blocked ASCII to upper case ASCII */
#define	ASCII		16	/* convert blocked EBCDIC to ASCII */
#define	LCASCII		17	/* convert blocked EBCDIC to lower case ASCII */
#define	UCASCII		18	/* convert blocked EBCDIC to upper case ASCII */
#define	BLOCK		19	/* convert ASCII to blocked ASCII */
#define	LCBLOCK		20	/* convert ASCII to lower case blocked ASCII */
#define	UCBLOCK		21	/* convert ASCII to upper case blocked ASCII */
#define	EBCDIC		22	/* convert ASCII to blocked EBCDIC */
#define	LCEBCDIC	23	/* convert ASCII to lower case blocked EBCDIC */
#define	UCEBCDIC	24	/* convert ASCII to upper case blocked EBCDIC */
#define	IBM		25	/* convert ASCII to blocked IBM */
#define	LCIBM		26	/* convert ASCII to lower case blocked IBM */
#define	UCIBM		27	/* convert ASCII to upper case blocked IBM */
#define	LCASE		01	/* flag - convert to lower case */
#define	UCASE		02	/* flag - convert to upper case */
#define	SWAB		04	/* flag - swap bytes before conversion */
#define NERR		010	/* flag - proceed on input errors */
#define SYNC		020	/* flag - pad short input blocks with nulls */
#define	NOTRUNC		040	/* flag - don't truncate output file */ /*L003*/
#define IMMAP		0100	/* flag - input file memory mapped */	/*L006*/
#define OMMAP		0200	/* flag - output file memory mapped */	/*L006*/
#define IBMODE		0400	/* flag - file reads in block mode */	/*L009*/
#define OBMODE		01000	/* flag - file writes in block mode */	/*L009*/
#define BADLIMIT	5	/* give up if no progress after BADLIMIT tries*/

#define	MAP_FAILED	(caddr_t)-1

/* Global references */

extern int	errno; /* system call error code value */	/* L004 */
extern void	exit();						/* L004 */

/* Local routine declarations */

static int		match(char *);				/* L003 begin */
static void		term(int);
static unsigned int	number(long);
static unsigned char	*flsh(void);
static void		stats(void);				/* L003 end */
static void		noterm(int);				/* L006 */
static off_t		fsize(int, off_t);			/* L006 */
static void		ftrunc(int, off_t, off_t);		/* L006 */

/* Local data definitions */

static unsigned ibs;	/* input buffer size */
static unsigned obs;	/* output buffer size */
static unsigned bs;	/* buffer size, overrules ibs and obs */
static unsigned cbs;	/* conversion buffer size, used for block conversions */
static unsigned ibc;	/* number of bytes still in the input buffer */
static unsigned obc;	/* number of bytes in the output buffer */
static unsigned cbc;	/* number of bytes in the conversion buffer */

static int	ibf;	/* input file descriptor */
static int	obf;	/* output file descriptor */
static int	cflag;	/* conversion option flags */
static int	skipf;	/* if skipf == 1, skip rest of input line */
static int	nifr;	/* count of full input records */
static int	nipr;	/* count of partial input records */
static int	nofr;	/* count of full output records */
static int	nopr;	/* count of partial output records */
static int	ntrunc;	/* count of truncated input lines */
static int	nbad;	/* count of bad records since last good one */
static int	files;	/* number of input files to concatenate (tape only) */
static int	skip;	/* number of input records to skip */
static int	iseekn;	/* number of input records to seek past */
static int	oseekn;	/* number of output records to seek past */
static int	count;	/* number of input records to copy (0 = all) */

static off_t	isize, iseeksize, osize, oseeksize;		/* L006 */
static int	doneseeks;					/* L006 L007 */
static jmp_buf	jmpbuf;						/* L007 */

static char		*string;	/* command arg pointer */
static char		*ifile;		/* input file name pointer */
static char		*ofile;		/* output file name pointer */
static unsigned char	*imbuf;		/* input memory map pointer L006 */
static unsigned char	*ibuf;		/* input buffer pointer */
static unsigned char	*obuf;		/* output buffer pointer */

#ifdef INTL				/* L004 */
static nl_catd catd;			/* L004 */
#endif /* INTL */			/* L004 */

char	*command_name = "dd";		/* L004 */

/* This is an EBCDIC to ASCII conversion table	*/
/* from a proposed BTL standard April 16, 1979	*/

static unsigned char etoa [] =
{
	0000,0001,0002,0003,0234,0011,0206,0177,
	0227,0215,0216,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0235,0205,0010,0207,
	0030,0031,0222,0217,0034,0035,0036,0037,
	0200,0201,0202,0203,0204,0012,0027,0033,
	0210,0211,0212,0213,0214,0005,0006,0007,
	0220,0221,0026,0223,0224,0225,0226,0004,
	0230,0231,0232,0233,0024,0025,0236,0032,
	0040,0240,0241,0242,0243,0244,0245,0246,
	0247,0250,0325,0056,0074,0050,0053,0174,
	0046,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0041,0044,0052,0051,0073,0176,
	0055,0057,0262,0263,0264,0265,0266,0267,
	0270,0271,0313,0054,0045,0137,0076,0077,
	0272,0273,0274,0275,0276,0277,0300,0301,
	0302,0140,0072,0043,0100,0047,0075,0042,
	0303,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0304,0305,0306,0307,0310,0311,
	0312,0152,0153,0154,0155,0156,0157,0160,
	0161,0162,0136,0314,0315,0316,0317,0320,
	0321,0345,0163,0164,0165,0166,0167,0170,
	0171,0172,0322,0323,0324,0133,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0135,0346,0347,
	0173,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0350,0351,0352,0353,0354,0355,
	0175,0112,0113,0114,0115,0116,0117,0120,
	0121,0122,0356,0357,0360,0361,0362,0363,
	0134,0237,0123,0124,0125,0126,0127,0130,
	0131,0132,0364,0365,0366,0367,0370,0371,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0372,0373,0374,0375,0376,0377,
};

/* This is an ASCII to EBCDIC conversion table	*/
/* from a proposed BTL standard April 16, 1979	*/

static unsigned char atoe [] =
{
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0132,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0255,0340,0275,0232,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0117,0320,0137,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0152,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0112,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0241,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};

/* Table for ASCII to IBM (alternate EBCDIC) code conversion	*/

static unsigned char atoibm[] =
{
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0132,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0255,0340,0275,0137,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0117,0320,0241,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0232,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};

/* Table for conversion of ASCII to lower case ASCII	*/

#ifndef INTL			/* L004 */

static unsigned char utol[] =
{
	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0152,0153,0154,0155,0156,0157,
	0160,0161,0162,0163,0164,0165,0166,0167,
	0170,0171,0172,0133,0134,0135,0136,0137,
	0140,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0152,0153,0154,0155,0156,0157,
	0160,0161,0162,0163,0164,0165,0166,0167,
	0170,0171,0172,0173,0174,0175,0176,0177,
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377,
};

/* Table for conversion of ASCII to upper case ASCII	*/

static unsigned char ltou[] =
{
	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0133,0134,0135,0136,0137,
	0140,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0173,0174,0175,0176,0177,
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377,
};

#endif	/* !INTL */

main(argc, argv)
int argc;
char **argv;
{
	register unsigned char *ip, *op;/* input and output buffer pointers */
	register int c;			/* character counter */
	register int ic;		/* input character */
	register int conv;		/* conversion option code */
	ulong bufalign;			/* L008 */
	
	/* Set option defaults */

	ibs = BSIZE;
	obs = BSIZE;
	files = 1;
	conv = COPY;

#ifdef INTL					/* L004 */
	setlocale (LC_ALL, "");
	(void) catopen(MF_DD, MC_FLAGS);
#endif /* INTL */				/* L004 */

	/* Parse command options */

	for (c = 1; c < argc; c++)
	{
		string = argv[c];
		if (match("ibs="))
		{
			ibs = number(BIG);
			continue;
		}
		if (match("obs="))
		{
			obs = number(BIG);
			continue;
		}
		if (match("cbs="))
		{
			cbs = number(BIG);
			continue;
		}
		if (match("bs="))
		{
			bs = number(BIG);
			continue;
		}
		if (match("if="))
		{
			ifile = string;
			continue;
		}
		if (match("of="))
		{
			ofile = string;
			continue;
		}

		if (match("skip="))
		{
			skip = number(BIG);
			continue;
		}
		if (match("iseek="))
		{
			iseekn = number(BIG);
			continue;
		}
		if (match("oseek="))
		{
			oseekn = number(BIG);
			continue;
		}
		if (match("seek="))		/* retained for compatibility */
		{
			oseekn = number(BIG);
			continue;
		}
		if (match("count="))
		{
			count = number(BIG);
			continue;
		}
		if (match("files="))
		{
			files = number(BIG);
			continue;
		}
		if (match("conv="))
		{
			for (;;)
			{
				if (match(","))
				{
					continue;
				}
				if (*string == '\0')
				{
					break;
				}
				if (match("block"))
				{
					conv = BLOCK;
					continue;
				}
				if (match("unblock"))
				{
					conv = UNBLOCK;
					continue;
				}

				if (match("ebcdic"))
				{
					conv = EBCDIC;
					continue;
				}
				if (match("ibm"))
				{
					conv = IBM;
					continue;
				}
				if (match("ascii"))
				{
					conv = ASCII;
					continue;
				}
				if (match("lcase"))
				{
					cflag |= LCASE;
					continue;
				}
				if (match("ucase"))
				{
					cflag |= UCASE;
					continue;
				}
				if (match("swab"))
				{
					cflag |= SWAB;
					continue;
				}
				if (match("noerror"))
				{
					cflag |= NERR;
					continue;
				}
				if (match("notrunc"))		/* L003 */
				{				/* L003 */
					cflag |= NOTRUNC;	/* L003 */
					continue;		/* L003 */
				}				/* L003 */
				if (match("mmap"))		/* L006 begin */
				{
					cflag |= IMMAP|OMMAP;
					continue;
				}
				if (match("immap"))
				{
					cflag |= IMMAP;
					continue;
				}
				if (match("ommap"))
				{
					cflag |= OMMAP;
					continue;
				}				/* L006 end */
				if (match("sync"))
				{
					cflag |= SYNC;
					continue;
				}
				if (match("bmode"))		/* L009 begin */
				{
					cflag |= IBMODE | OBMODE;
					continue;
				}				/* L009 end */
				goto badarg;
			}
			continue;
		}
		badarg:
		errorl(MSGSTR(DD_ERR_BADARG, "bad arg: \"%s\""), string);
		exit(2);
	}

	/* Perform consistency checks on options, decode strange conventions */

	if (bs)
	{
		ibs = obs = bs;
	}
	if ((ibs == 0) || (obs == 0))
	{
		errorl(MSGSTR(DD_ERR_BUFF, "buffer sizes cannot be zero"));
		exit(2);
	}
	if (ibs % NBPSCTR)					/* L009 begin */
	{
		cflag &= ~IBMODE;
	}
	if (obs % NBPSCTR)
	{
		cflag &= ~OBMODE;
	}							/* L009 end */
	if (conv == COPY)
	{
		if (cbs != 0)
		{
			errorl(MSGSTR(DD_ERR_CBS_OPT, "cbs must be zero if no block conversion requested"));
			exit(2);
		}
		if ((ibs != obs) || (cflag&(LCASE|UCASE)))	/* L006 */
		{
			conv = REBLOCK;
		}
	}
	if (cbs == 0)
	{
		switch (conv)
		{
		case BLOCK:
		case UNBLOCK:
			conv = REBLOCK;
			break;

		case ASCII:
			conv = NBASCII;
			break;

		case EBCDIC:
			conv = NBEBCDIC;
			break;

		case IBM:
			conv = NBIBM;
			break;
		}
	}
	else if (cflag & OMMAP)					/* L006 begin */
	{
		errorl(MSGSTR(DD_ERR_CBSOMMAP, "cbs must be zero if output is memory mapped"));
		exit(2);
	}							/* L006 end */

	/* Expand options into lower and upper case versions if necessary */

	switch (conv)
	{
	case REBLOCK:
		if (cflag&LCASE)
		{
			conv = LCREBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCREBLOCK;
		}
		break;

	case UNBLOCK:
		if (cflag&LCASE)
		{
			conv = LCUNBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCUNBLOCK;
		}
		break;

	case BLOCK:
		if (cflag&LCASE)
		{
			conv = LCBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCBLOCK;
		}
		break;

	case ASCII:
		if (cflag&LCASE)
		{
			conv = LCASCII;
		}
		else if (cflag&UCASE)
		{
			conv = UCASCII;
		}
		break;

	case NBASCII:
		if (cflag&LCASE)
		{
			conv = LCNBASCII;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBASCII;
		}
		break;

	case EBCDIC:
		if (cflag&LCASE)
		{
			conv = LCEBCDIC;
		}
		else if (cflag&UCASE)
		{
			conv = UCEBCDIC;
		}
		break;

	case NBEBCDIC:
		if (cflag&LCASE)
		{
			conv = LCNBEBCDIC;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBEBCDIC;
		}
		break;

	case IBM:
		if (cflag&LCASE)
		{
			conv = LCIBM;
		}
		else if (cflag&UCASE)
		{
			conv = UCIBM;
		}
		break;

	case NBIBM:
		if (cflag&LCASE)
		{
			conv = LCNBIBM;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBIBM;
		}
		break;
	}

	/* Open the input file, or duplicate standard input */

	ibf = -1;
	if (ifile)
	{
		ibf = blockopen(ifile, 0);
		iseeksize = 0;					/* L006 */
	}
#ifndef STANDALONE
	else
	{
		ifile = "";
		ibf = dup(0);
		iseeksize = blocklseek(ibf, 0, SEEK_CUR);	/* L006 */
	}
#endif
	if (ibf == -1)
	{
		psyserrorl(errno, MSGSTR(DD_ERR_OPEN, "cannot open %s"), ifile);
		exit(2);
	}

	if ((cflag & IBMODE) && blockfcntl(ibf, F_SETBMODE, 1) == -1)/* L009 begin */
	{
		cflag &= ~IBMODE;
	}							/* L009 end */

	/* Expand memory to get an input buffer */

	iseeksize += (skip + iseekn) * ibs;			/* L006 begin */

	if (cflag & (IMMAP|OMMAP))
	{
		isize = fsize(ibf, count? iseeksize + ibs * count: 0);
		if (iseeksize > isize)
			iseeksize = isize;
		isize -= iseeksize;
		iseekn += skip;
		skip = 0;
	}

	if (cflag & IMMAP)
	{
		register off_t off = iseeksize & (PAGESIZE-1);
		imbuf = mmap(NULL, isize + off, PROT_READ,
				MAP_SHARED, ibf, iseeksize - off);
		if (imbuf == (unsigned char *)MAP_FAILED)
		{
			psyserrorl(errno, MSGSTR(DD_ERR_IMMAP, "cannot map input"));
			exit(2);
		}
		imbuf += off;
		iseekn = 0;
		if (cflag & (SWAB|SYNC)) {
			ibuf = (unchar *)malloc(ibs + NBPSCTR);	/* L008 */
			bufalign = (ulong)ibuf & (NBPSCTR-1);	/* L008 */
			ibuf += NBPSCTR - bufalign;		/* L008 */
		}
		else
			ibuf = imbuf;
	}
	else {							/* L006 end */
		ibuf = (unchar *)malloc(ibs + NBPSCTR + 10);	/* L008 */
		bufalign = (ulong)ibuf & (NBPSCTR-1);		/* L008 */
		ibuf += NBPSCTR - bufalign;			/* L008 */
	}

	/* Open the output file, or duplicate standard output */

	obf = -1;
	if (ofile)
	{
		obf = blockopen(ofile, (cflag & OMMAP)?		/* L006 */
			O_RDWR|O_CREAT: O_WRONLY|O_CREAT, 0666);/* L006 */
		oseeksize = 0;					/* L006 */
	}
#ifndef STANDALONE
	else
	{
		ofile = "";
		obf = dup(1);
		oseeksize = blocklseek(obf, 0, SEEK_CUR);	/* L006 */
	}
#endif
	if (obf == -1)
	{
		psyserrorl(errno, MSGSTR(DD_ERR_CREATE, "cannot create %s"), ofile);
		exit(2);
	}

	/* This next line is to 'fix' the OpenServer auto-blocking */
	if (blockfcntl(obf, F_GETBMODE, &nopr) != -1) cflag |= SYNC;

	if ((cflag & OBMODE) && blockfcntl(obf, F_SETBMODE, 1) == -1)/* L009 begin */
	{
		cflag &= ~OBMODE;
	}							/* L009 end */

	oseeksize += oseekn * obs;				/* L006 begin */

	if (cflag & OMMAP)
	{
		register off_t off = oseeksize & (PAGESIZE-1);
		register off_t synclen = 0;

		if ((cflag & SYNC) && (isize % ibs))
			synclen = ibs - isize % ibs;
		obuf = mmap(NULL, isize + synclen + off, PROT_READ|PROT_WRITE,
				MAP_SHARED, obf, oseeksize - off);
		if (obuf == (unsigned char *)MAP_FAILED)
		{
			if (errno == ENODEV)
				errorl(MSGSTR(DD_ERR_OMMAPDEV, "cannot map output: Must be regular file or block device"));
			else
				psyserrorl(errno, MSGSTR(DD_ERR_OMMAP, "cannot map output"));
			exit(2);
		}
		obuf += off;
		oseekn = 0;
		ftrunc(obf, oseeksize, oseeksize + isize + synclen);
	}

	/* If no conversions, the input buffer is the output buffer */

	else							/* L006 end */
	if (conv == COPY)
	{
		obuf = ibuf;
	}

	/* Expand memory to get an output buffer.  Leave enough room at the */
	/* end to convert a logical record when doing block conversions.    */

	else
	{
		obuf = (unchar *)malloc(obs+cbs+NBPSCTR+10);	/* L008 */
		bufalign = (ulong)obuf & (NBPSCTR-1);		/* L008 */
		obuf += NBPSCTR - bufalign;			/* L008 */
	}

	if ((ibuf <= (unsigned char *)NBPSCTR)			/* L011 */
	||  (obuf <= (unsigned char *)NBPSCTR))			/* L011 */
	{
		errorl(MSGSTR(DD_ERR_NOMEM, "not enough memory"));
		exit(2);
	}

	/* Enable a statistics message on SIGINT */

#ifndef STANDALONE
	if ((cflag & (IMMAP|OMMAP)) == 0)			/* L006 */
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	{
		(void)signal(SIGINT, term);
	}
#endif

	/* Skip input blocks */

	while (skip)
	{
		ibc = blockread(ibf, (char *)ibuf, ibs);
		if (ibc == (unsigned)-1)
		{
			if (++nbad > BADLIMIT)
			{
				psyserrorl(errno, MSGSTR(DD_ERR_SKIPFAIL, "skip failed"));
				exit(2);
			}
			else
			{
				psyserrorl(errno, MSGSTR(DD_ERR_SKIPRFAIL, "read error during skip"));
			}
		}
		else
		{
			if (ibc == 0)
			{
				errorl(MSGSTR(DD_ERR_SKIPEFAIL,
					"cannot skip past end-of-file")); /* L005 */
				exit(3);
			}
			else
			{
				nbad = 0;
			}
		}
		skip--;
	}

	/* Seek past input blocks */

	if (iseekn)						/* L009 begin */
	{
		long bs = (cflag & IBMODE) ? ibs >> SCTRSHFT : ibs;

		if (blocklseek(ibf, (off_t)(bs * iseekn), SEEK_CUR) == -1)
		{
			psyserrorl(errno, MSGSTR(DD_ERR_ISEEK, "input seek error"));
			exit(2);
		}
	}							/* L009 end */

	/* Seek past output blocks */

	if (oseekn)						/* L009 begin */
	{
		long bs = (cflag & OBMODE) ? obs >> SCTRSHFT : obs;

		if (blocklseek(obf, (off_t)(bs * oseekn), SEEK_CUR) == -1)
		{
			psyserrorl(errno, MSGSTR(DD_ERR_OSEEK, "output seek error"));
			exit(2);
		}
	}							/* L009 end */

	doneseeks = 1;						/* L006 begin */

	if ((cflag & (IMMAP|OMMAP))
	&& !(cflag & (SWAB|SYNC))
	&&  (conv == COPY))
	{
		register int n = isize;

		/*
		 * Try to do it with minimal system calls
		 */
		if (setjmp(jmpbuf))				/* L007 */
			goto quickfailed;			/* L007 */
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			(void)signal(SIGINT, noterm);
		(void)signal(SIGBUS, noterm);

		while (isize && n > 0)
		{
			switch (cflag & (IMMAP|OMMAP))
			{
			case IMMAP|OMMAP:
				memcpy(obuf, imbuf, isize);
				break;				/* L007 */
			case IMMAP:
				n = blockwrite(obf, imbuf, isize);
				break;
			case OMMAP:
				n = blockread(ibf, obuf, isize);
				break;
			}
			if (n > 0)
			{
				isize -= n;
				osize += n;
				imbuf += n;
				obuf  += n;
			}
		}
		if (isize == 0)
		{
			nifr = nofr = osize / ibs;
			nipr = nopr = (osize % ibs)? 1: 0;
			term(0);
		}
quickfailed:							/* L007 */
		/*
		 * If that failed, reset and do it the slow way
		 */
		isize += osize;
		imbuf -= osize;
		obuf  -= osize;
		osize  = 0;

		{						/* L009 begin */
			/*
			 * Note that for block mode we do not use the
			 * original file seek position in our
			 * calculations. We can do this since it will
			 * have been reset to zero when we originally
			 * turned on block mode.
			 */
			off_t ioff, ooff;

			ioff = (cflag & IBMODE) ?
				(skip + iseekn) * (ibs >> SCTRSHFT) : iseeksize;
			ooff = (cflag & OBMODE) ?
				oseekn * (obs >> SCTRSHFT) : oseeksize;

			blocklseek(ibf, ioff, SEEK_SET);
			blocklseek(obf, ooff, SEEK_SET);
		}						/* L009 end */
	}

	if (cflag & (IMMAP|OMMAP))
	{
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			(void)signal(SIGINT, term);
		(void)signal(SIGBUS, term);
	}							/* L006 end */

	/* Initialize all buffer pointers */

	skipf = 0;	/* not skipping an input line */
	ibc = 0;	/* no input characters yet */
	obc = 0;	/* no output characters yet */
	cbc = 0;	/* the conversion buffer is empty */
	op = obuf;	/* point to the output buffer */

	/* Read and convert input blocks until end of file(s) */

	for (;;)
	{
		if ((count == 0) || (nifr+nipr < count))
		{
			if (cflag & IMMAP)			/* L006 begin */
			{
				ibc = isize < ibs? isize: ibs;
				if (cflag & (SWAB|SYNC))
					memcpy(ibuf, imbuf, ibc);
				else
					ibuf = imbuf;
				imbuf += ibc;
				isize -= ibc;
			}
			else					/* L006 end */
			{
				/* If proceed on error is enabled,
				 *  zero the input buffer
				 */
				if (cflag&NERR)
					memset(ibuf, 0, ibs);	/* L006 */

				/* Read the next input block */

				ibc = blockread(ibf, (char *)ibuf, ibs);
			}

			/* Process input errors */

			if (ibc == (unsigned)-1)
			{
				psyserrorl(errno, MSGSTR(DD_ERR_READ, "read error"));
				if (   ((cflag&NERR) == 0)
				    || (++nbad > BADLIMIT) )
				{
					while (obc)
					{
						(void)flsh();
					}
					term(2);
				}
				else
				{
					stats();
					ibc = ibs;	/* assume a full block */
				}
			}
			else
			{
				nbad = 0;
			}
			if ((cflag & (IMMAP|OMMAP)) == OMMAP)	/* L006 begin */
			{
				if (isize >= ibc)
					isize -= ibc;
				else
				{	/* file grew: ignore growth */
					ibc = isize;
					isize = 0;
				}
			}					/* L006 end */
		}

		/* Record count satisfied, simulate end of file */

		else
		{
			ibc = 0;
			files = 1;
		}

		/* Process end of file */

		if (ibc == 0)
		{
			switch (conv)
			{
			case UNBLOCK:
			case LCUNBLOCK:
			case UCUNBLOCK:
			case ASCII:
			case LCASCII:
			case UCASCII:

				/* Trim trailing blanks from the last line */

				if ((c = cbc) != 0)
				{
					do {
						if ((*--op) != ' ')
						{
							op++;
							break;
						}
					} while (--c);
					*op++ = '\n';
					obc -= cbc - c - 1;
					cbc = 0;

					/* Flush the output buffer if full */

					while (obc >= obs)
					{
						op = flsh();
					}
				}
				break;

			case BLOCK:
			case LCBLOCK:
			case UCBLOCK:
			case EBCDIC:
			case LCEBCDIC:
			case UCEBCDIC:
			case IBM:
			case LCIBM:
			case UCIBM:

				/* Pad trailing blanks if the last line is short */

				if (cbc)
				{
					obc += c = cbs - cbc;
					cbc = 0;
					if (c > 0)
					{
						/* Use the right kind of blank */

						switch (conv)
						{
						case BLOCK:
						case LCBLOCK:
						case UCBLOCK:
							ic = ' ';
							break;

						case EBCDIC:
						case LCEBCDIC:
						case UCEBCDIC:
							ic = atoe[' '];
							break;

						case IBM:
						case LCIBM:
						case UCIBM:
							ic = atoibm[' '];
							break;
						}

						/* Pad with trailing blanks */

						do {
							*op++ = ic;
						} while (--c);
					}
				}

				/* Flush the output buffer if full */

				while (obc >= obs)
				{
					op = flsh();
				}
				break;
			}

			/* If no more files to read, flush the output buffer */

			if (--files <= 0)
			{
				(void)flsh();
				term(0);	/* successful exit */
			}
			else
			{
				continue;	/* read the next file */
			}
		}

		/* Normal read, check for special cases */

		else if (ibc == ibs)
		{
			nifr++;		/* count another full input record */
		}
		else
		{
			nipr++;		/* count a partial input record */

								/* L012 Begin */
			/* If `sync' enabled, pad with nulls or spaces */

			if ((cflag&SYNC) && ((cflag&NERR) == 0))
			{
				if ((conv == BLOCK) || (conv == UNBLOCK))
					memset(ibuf + ibc, ' ', ibs - ibc);
				else
					memset(ibuf + ibc, 0, ibs - ibc);
				ibc = ibs;
			}
								/* L012 End */
		}

		/* Swap the bytes in the input buffer if necessary */

		if (cflag&SWAB)
		{
			ip = ibuf;
			if (ibc & 1)		/* if the byte count is odd, */
			{
				ip[ibc] = 0;	/* make it even, pad with zero */
			}
			c = (ibc + 1) >> 1;	/* compute the pair count */
			do {
				ic = *ip++;
				ip[-1] = *ip;
				*ip++ = ic;
			} while (--c);		/* do two bytes at a time */
		}

		/* Select the appropriate conversion loop */

		ip = ibuf;
		switch (conv)
		{

		/* Simple copy: no conversion, preserve the input block size */

		case COPY:
			if (cflag & OMMAP)			/* L006 */
				memcpy(obuf, ibuf, ibc);	/* L006 */
			obc = ibc;
			(void)flsh();
			if ((cflag & (IMMAP|OMMAP|SWAB|SYNC)) == IMMAP)	/*L007*/
				obuf += ibc;				/*L007*/
			break;

		/* Simple copy: pack all output into equal sized blocks */

		case REBLOCK:
		case LCREBLOCK:
		case UCREBLOCK:
		case NBASCII:
		case LCNBASCII:
		case UCNBASCII:
		case NBEBCDIC:
		case LCNBEBCDIC:
		case UCNBEBCDIC:
		case NBIBM:
		case LCNBIBM:
		case UCNBIBM:
			while ((c = ibc) != 0)
			{
				if (c > (obs - obc))
				{
					c = obs - obc;
				}
				ibc -= c;
				obc += c;
				switch (conv)
				{
				case REBLOCK:
					do {
						*op++ = *ip++;
					} while (--c);
					break;

				case LCREBLOCK:
					do {
#ifdef INTL						/* S001 begin */
						*op = tolower(*ip);
						ip++;
						op++;
#else
						*op++ = utol[*ip++];
#endif							/* S001 end */
					} while (--c);
					break;

				case UCREBLOCK:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = toupper(*ip);
						ip++;
#else
						*op++ = ltou[*ip++];
#endif							/* S001 end */
					} while (--c);
					break;

				case NBASCII:
					do {
						*op++ = etoa[*ip++];
					} while (--c);
					break;

				case LCNBASCII:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = tolower(etoa[*ip]);
						ip++;
#else
						*op++ = utol[etoa[*ip++]];
#endif							/* S001 end */
					} while (--c);
					break;

				case UCNBASCII:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = toupper(etoa[*ip]);
						ip++;
#else
						*op++ = ltou[etoa[*ip++]];
#endif							/* S001 end */
					} while (--c);
					break;

				case NBEBCDIC:
					do {
						*op++ = atoe[*ip++];
					} while (--c);
					break;

				case LCNBEBCDIC:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = atoe[toupper(*ip)];
						ip++;
#else
						*op++ = atoe[utol[*ip++]];
#endif							/* S001 end */
					} while (--c);
					break;

				case UCNBEBCDIC:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = atoe[toupper(*ip)];
						ip++;
#else
						*op++ = atoe[ltou[*ip++]];
#endif							/* S001 end */
					} while (--c);
					break;

				case NBIBM:
					do {
						*op++ = atoibm[*ip++];
					} while (--c);
					break;

				case LCNBIBM:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = atoibm[tolower(*ip)];
						ip++;
#else
						*op++ = atoibm[utol[*ip++]];
#endif							/* S001 end */
					} while (--c);
					break;

				case UCNBIBM:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = atoibm[toupper(*ip)];
						ip++;
#else
						*op++ = atoibm[ltou[*ip++]];
#endif							/* S001 end */
					} while (--c);
					break;
				}
				if (obc >= obs)
				{
					op = flsh();
				}
			}
			break;

		/* Convert from blocked records to lines terminated by newline */

		case UNBLOCK:
		case LCUNBLOCK:
		case UCUNBLOCK:
		case ASCII:
		case LCASCII:
		case UCASCII:
			while ((c = ibc) != 0)
			{
				if (c > (cbs - cbc))	/* if more than one record, */
				{
					c = cbs - cbc;	/* only copy one record */
				}
				ibc -= c;
				cbc += c;
				obc += c;
				switch (conv)
				{
				case UNBLOCK:
					do {
						*op++ = *ip++;
					} while (--c);
					break;

				case LCUNBLOCK:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = tolower(*ip);
						ip++;
#else
						*op++ = utol[*ip++];
#endif							/* S001 end */
					} while (--c);
					break;

				case UCUNBLOCK:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = toupper(*ip);
						ip++;
#else
						*op++ = ltou[*ip++];
#endif							/* S001 end */
					} while (--c);
					break;

				case ASCII:
					do {
						*op++ = etoa[*ip++];
					} while (--c);
					break;

				case LCASCII:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = tolower(etoa[*ip]);
						ip++;
#else
						*op++ = utol[etoa[*ip++]];
#endif							/* S001 end */
					} while (--c);
					break;

				case UCASCII:
					do {
#ifdef INTL						/* S001 begin */
						*op++ = toupper(etoa[*ip]);
						ip++;
#else
						*op++ = ltou[etoa[*ip++]];
#endif							/* S001 end */
					} while (--c);
					break;
				}

				/* Trim trailing blanks if the line is full */

				if (cbc == cbs)
				{
					c = cbs;	/* `do - while' is usually */
					do {		/* faster than `for' */
						if ((*--op) != ' ')
						{
							op++;
							break;
						}
					} while (--c);
					*op++ = '\n';
					obc -= cbs - c - 1;
					cbc = 0;

					/* Flush the output buffer if full */

					while (obc >= obs)
					{
						op = flsh();
					}
				}
			}
			break;

		/* Convert to blocked records */

		case BLOCK:
		case LCBLOCK:
		case UCBLOCK:
		case EBCDIC:
		case LCEBCDIC:
		case UCEBCDIC:
		case IBM:
		case LCIBM:
		case UCIBM:
			while ((c = ibc) != 0)
			{
				int nlflag = 0;

				/* We may have to skip to the end of a long line */

				if (skipf)
				{
					do {
						if ((ic = *ip++) == '\n')
						{
							skipf = 0;
							c--;
							break;
						}
					} while (--c);
					if ((ibc = c) == 0)
					{
						continue;	/* read another block */
					}
				}

				/* If anything left, copy until newline */

				if (c > (cbs - cbc + 1))
				{
					c = cbs - cbc + 1;
				}
				ibc -= c;
				cbc += c;
				obc += c;

				switch (conv)
				{
				case BLOCK:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = ic;
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCBLOCK:
					do {
						if ((ic = *ip++) != '\n')
						{
#ifdef INTL						/* S001 begin */
							*op = tolower(ic);
							op++;
#else
							*op++ = utol[ic];
#endif							/* S001 end */
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case UCBLOCK:
					do {
						if ((ic = *ip++) != '\n')
						{
#ifdef INTL						/* S001 begin */
							*op = toupper(ic);
							op++;
#else
							*op++ = ltou[ic];
#endif							/* S001 end */
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case EBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoe[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCEBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
#ifdef INTL						/* S001 begin */
							*op++ = atoe[tolower(ic)];
#else
							*op++ = atoe[utol[ic]];
#endif							/* S001 end */
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case UCEBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
#ifdef INTL						/* S001 begin */
							*op++ = atoe[toupper(ic)];
#else
							*op++ = atoe[ltou[ic]];
#endif							/* S001 end */
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case IBM:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoibm[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCIBM:
					do {
						if ((ic = *ip++) != '\n')
						{
#ifdef INTL						/* S001 begin */
							*op++ = atoibm[tolower(ic)];
#else
							*op++ = atoibm[utol[ic]];
#endif							/* S001 end */
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case UCIBM:
					do {
						if ((ic = *ip++) != '\n')
						{
#ifdef INTL						/* S001 begin */
							*op++ = atoibm[toupper(ic)];
#else
							*op++ = atoibm[ltou[ic]];
#endif							/* S001 end */
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;
				}

				/* If newline found, update all the counters and */
 				/* pointers, pad with trailing blanks if necessary */

				if (nlflag)
				{
					ibc += c - 1;
					obc += cbs - cbc;
					c += cbs - cbc;
					cbc = 0;
					if (c > 0)
					{
						/* Use the right kind of blank */

						switch (conv)
						{
						case BLOCK:
						case LCBLOCK:
						case UCBLOCK:
							ic = ' ';
							break;

						case EBCDIC:
						case LCEBCDIC:
						case UCEBCDIC:
							ic = atoe[' '];
							break;

						case IBM:
						case LCIBM:
						case UCIBM:
							ic = atoibm[' '];
							break;
						}

						/* Pad with trailing blanks */

						do {
							*op++ = ic;
						} while (--c);
					}
				}

				/* If not end of line, this line may be too long */

				else if (cbc > cbs)
				{
					skipf = 1;	/* note skip in progress */
					obc--;
					op--;
					cbc = 0;
					ntrunc++;	/* count another long line */
				}

				/* Flush the output buffer if full */

				while (obc >= obs)
				{
					op = flsh();
				}
			}
			break;
		}
	}
}

/* match ************************************************************** */
/*									*/
/* Compare two text strings for equality				*/
/*									*/
/* Arg:		s - pointer to string to match with a command arg	*/
/* Global arg:	string - pointer to command arg				*/
/*									*/
/* Return:	1 if match, 0 if no match				*/
/*		If match, also reset `string' to point to the text	*/
/*		that follows the matching text.				*/
/*									*/
/* ********************************************************************	*/

static int
match(s)
char *s;
{
	register char *cs;

	cs = string;
	while (*cs++ == *s)
	{
		if (*s++ == '\0')
		{
			goto true;
		}
	}
	if (*s != '\0')
	{
		return(0);
	}

true:
	cs--;
	string = cs;
	return(1);
}

/* number ************************************************************* */
/*									*/
/* Convert a numeric arg to binary					*/
/*									*/
/* Arg:		big - maximum valid input number			*/
/* Global arg:	string - pointer to command arg				*/
/*									*/
/* Valid forms:	123 | 123k | 123w | 123b | 123*123 | 123x123		*/
/*		plus combinations such as 2b*3kw*4w			*/
/*									*/
/* Return:	converted number					*/
/*									*/
/* ********************************************************************	*/

static
unsigned int number(long big)
{
	register char *cs;
	long n;
	long cut = BIG / 10;	/* limit to avoid overflow */

	cs = string;
	n = 0;
#ifdef INTL						/* S000 begin */
	while (isdigit(*cs) && (n <= cut))
	{
		n = n*10 + toint(*cs);
		cs++;
	}
#else
	while ((*cs >= '0') && (*cs <= '9') && (n <= cut))
	{
		n = n*10 + *cs++ - '0';
	}
#endif							/* S000 end */
	for (;;)
	{
		switch (*cs++)
		{

		case 'k':
			n *= 1024;
			continue;

		case 'w':
			n *= 2;
			continue;

		case 'b':
			n *= BSIZE;
			continue;

		case '*':
		case 'x':
			string = cs;
			n *= number(BIG);

			/*FALLTHROUGH*/

		/* End of string, check for a valid number */

		case '\0':
			if ((n > big) || (n < 0))
			{
				errorl(MSGSTR(DD_ERR_ARGERR, "argument out of range: \"%lu\""), n);
				exit(2);
			}
			return(n);

		default:
			errorl(MSGSTR(DD_ERR_BAD_N_ARG, "bad numeric arg: \"%s\""), string);
			exit(2);
		}
	} /* never gets here */
}

/* flsh *************************************************************** */
/*									*/
/* Flush the output buffer, move any excess bytes down to the beginning	*/
/*									*/
/* Arg:		none							*/
/* Global args:	obuf, obc, obs, nofr, nopr				*/
/*									*/
/* Return:	Pointer to the first free byte in the output buffer.	*/
/*		Also reset `obc' to account for moved bytes.		*/
/*									*/
/* ********************************************************************	*/

static unsigned char *
flsh()
{
	register unsigned char *op, *cp;
	register unsigned int bc;
	register unsigned int oc;

	if (obc)			/* don't flush if the buffer is empty */
	{
		if (obc >= obs)
			oc = obs;
		else
			oc = obc;
		if (cflag & OMMAP)				/* L006 begin */
		{
			obuf += oc;
			bc = oc;
		}
		else						/* L006 end */
			bc = blockwrite(obf, (char *)obuf, oc);
		if (bc != oc)
		{
			if (bc == -1){				/* L005 begin */
				psyserrorl(errno, MSGSTR(DD_ERR_WRITE, "write error"));
			}
			else{
				errorl(MSGSTR(DD_ERR_WRITE, "write error"));
				nopr++;				/* L007 */
				osize += bc;			/* L006 */
			}					/* L005 end */
			term(2);
		}
		if (obc >= obs)					/* L007 */
			nofr++;		/* count a full output buffer */
		else						/* L007 */
			nopr++;		/* count a partial output buffer */
		osize += oc;					/* L006 */
		obc -= oc;
		op = obuf;

		/* If any data in the conversion buffer, move it into the output buffer */

		if (obc)
		{
			cp = obuf + obs;
			bc = obc;
			do {
				*op++ = *cp++;
			} while (--bc);
		}
		return(op);
	}
	return(obuf);
}

/* term *************************************************************** */
/*									*/
/* Write record statistics, then exit					*/
/*									*/
/* Arg:		c - exit status code					*/
/*									*/
/* Return:	no return, calls exit					*/
/*									*/
/* ********************************************************************	*/

static void
term(int c)
{
	(void)signal(SIGINT, SIG_IGN);				/* L006 */
	if (doneseeks && !(cflag & NOTRUNC))			/* L006 */
		ftrunc(obf, oseeksize + osize, 0);		/* L006 */
	if (c == SIGBUS)					/* L007 */
		errorl(MSGSTR(DD_ERR_SIGBUS, "unexpected end of file"));
	stats();
	exit(c);
}

static void							/* L006 begin */
noterm(int c)
{
	longjmp(jmpbuf, 1);					/* L007 */
}								/* L006 end */

/* stats ************************************************************** */
/*									*/
/* Write record statistics onto standard error				*/
/*									*/
/* Args:	none							*/
/* Global args:	nifr, nipr, nofr, nopr, ntrunc				*/
/*									*/
/* Return:	void							*/
/*									*/
/* ********************************************************************	*/

static void
stats()
{
	(void) fprintf(stderr, MSGSTR(DD_MSG_RECIN, "%u+%u records in\n"), nifr, nipr);
	(void) fprintf(stderr, MSGSTR(DD_MSG_RECOUT, "%u+%u records out\n"), nofr, nopr);
	if (ntrunc)
	{
		(void) fprintf(stderr, MSGSTR(DD_MSG_RECTRUN, "%u truncated record%s\n"), ntrunc, ntrunc != 1 ? "s" : "");
	}
}

static off_t							/* L006 begin */
fsize(int fd, off_t maxsize)
{
	struct stat stbuf;
	register size_t last, base, len;
	register off_t off;
	char *addr;

	errno = 0;
	if (fstat(fd, &stbuf) < 0) {
		psyserrorl(errno, MSGSTR(DD_ERR_ISTAT, "input stat error"));
		exit(2);
	}
	if (S_ISREG(stbuf.st_mode)) {
		if (maxsize <= 0 || stbuf.st_size < maxsize)
			maxsize = stbuf.st_size;
		return maxsize;
	}
	if (!S_ISBLK(stbuf.st_mode)) {
		/* Allow /dev/zero to be mapped if count is given  L010 */
		if (S_ISCHR(stbuf.st_mode) && maxsize > 0)	/* L010 */
			return maxsize;				/* L010 */
		errorl(MSGSTR(DD_ERR_IMMAPDEV, "cannot size input: Must be regular file or block device"));
		exit(2);
	}
	if (maxsize <= 0)
		maxsize = (off_t)0x80000000;
	else {
		len = (size_t)maxsize & (PAGESIZE-1);
		maxsize -= (off_t)len;
		addr = mmap(NULL, len, PROT_NONE, MAP_SHARED, fd, maxsize);
		switch (errno) {
		case ENXIO:
			errno = 0;
			break;
		case 0:
			(void)munmap(addr, len);
			/* FALLTHROUGH */
		default:
			errno = 0;
			return maxsize + (off_t)len;
		}
	}
/*
 * Binary search algorithm, generalized from Knuth (6.2.1) Algorithm B.
 */
	base = PAGESIZE;
	last = (size_t)maxsize - PAGESIZE;
	while (last >= base) {
		off = (off_t)(base+PAGESIZE*((last-base)/(PAGESIZE+PAGESIZE)));
		(void)mmap(NULL, 0UL, PROT_NONE, MAP_SHARED, fd, off);
		switch (errno) {
		case ENXIO:
			last = off - PAGESIZE;
			break;
		default:
			base = off + PAGESIZE;
			break;
		}
		errno = 0;
	}
/*
 * To decide the last odd blocks, we must use non-zero-length mappings,
 * since offset is constrained to be a multiple of PAGESIZE, 8*NBPSCTR.
 */
	base = last + NBPSCTR;
	last = base + PAGESIZE - NBPSCTR - NBPSCTR;
	while (last >= base) {
		off = (off_t)(base+NBPSCTR*((last-base)/(NBPSCTR+NBPSCTR)));
		len = (size_t)off & (PAGESIZE-1);
		addr = mmap(NULL, len, PROT_NONE, MAP_SHARED, fd, off-len);
		switch (errno) {
		case ENXIO:
			last = off - NBPSCTR;
			break;
		case 0:
			(void)munmap(addr, len);
			/* FALLTHROUGH */
		default:
			base = off + NBPSCTR;
			break;
		}
		errno = 0;
	}
	if (base == 0x80000000) {
		addr = mmap(NULL, PAGESIZE, PROT_NONE, MAP_SHARED, fd, base-PAGESIZE);
		switch (errno) {
		case ENXIO:
			break;
		case 0:
			(void)munmap(addr, PAGESIZE);
			/* FALLTHROUGH */
		default:
			last = base - 1; /* miss last byte as write() would */
			break;
		}
	}
	return (off_t)last;
}

static void
ftrunc(int fd, off_t minsize, off_t maxsize)
{
	struct stat stbuf;
	register size_t last, base, len;
	register off_t off;
	char *addr;

	if (fstat(fd, &stbuf) < 0) {
		psyserrorl(errno, MSGSTR(DD_ERR_OSTAT, "output stat error"));
		exit(2);
	}
	if (!S_ISREG(stbuf.st_mode))
		return;
	if (minsize < stbuf.st_size && !(cflag & NOTRUNC)) {
		if (ftruncate(fd, minsize) < 0) {
			psyserrorl(errno, MSGSTR(DD_ERR_OTRUNC, "output truncate error"));
			exit(2);
		}
		stbuf.st_size = minsize;
	}
	if (maxsize > stbuf.st_size) {
		if (ftruncate(fd, maxsize) < 0) {
			psyserrorl(errno, MSGSTR(DD_ERR_EXTEND, "output extend error"));
			exit(2);
		}
	}
}								/* L006 end */
