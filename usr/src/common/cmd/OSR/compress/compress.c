#ident	"@(#)OSRcmds:compress/compress.c	1.1"
#pragma comment(exestr, "@(#) compress.c 26.3 95/06/30 ")
/*
 *	Copyright (C) The Santa Cruz Operation, 1986-1995.
 *		All Rights Reserved.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation and should be treated as Confidential.
 */
/*
 *  MODIFICATION HISTORY
 *	5 Jan 1987	sco!blf S001	unnoted
 *		- Straighten out the XENIX-code:  It's currently only
 *		  for XENIX-*86 (albeit it works Ok on non Intel machines).
 *	9 Sept 1989 	sco!kenj S002
 *		- Add an option to allow filenames to be passed through
 *		  a pipe whose read end is defined by the passed parameter.
 *	15 Nov 1989	sco!kenj S003
 *		- Changed ctmp to lng_ctmp in do_Pflag(). lng_ctmp is ctmp
 *		  prepended by the directory where the file being decompressed
 *		  lives.  This allows decompression across filesystems.
 *	30 Oct 1990	scol!dipakg	L004
 *		- Allow for long filenames 
 *	4 Feb 1991	scol!anthonys	L005
 *		- Determine maximum file name length at run time.
 *	6 Feb 1991	scol!anthonys	L006
 *		- Fixed a bug in the implementation of L005
 *	20 Nov 1991	scol!ashleyb	L007
 *		- Altered usage message to reflect code and manual page changes.
 *		- hughd notes in merging S008: L007's usage message seems to
 *		  reflect neither the code nor the manual page: fix it later
 *	27 Nov 1991	sco!markd	S008
 *		- added in -H option to use LZH compression algorithm, for
 *		  use in ODT Phoenix: saves 20% more than existing algorithm
 *		- new magic number, 1F A0, recognized by uncompress to do LZH
 *	28nov91		scol!hughd	L009
 *		- lzh_compress() set exit_stat 2 if compressSize>originalSize,
 *		  as compress() does: in particular, this stops xtra/:cutos
 *		  from wasting a tar block on each empty file
 *	29nov91		scol!markhe	L010
 *		- cleaned up Usage messages.  Have not added the flags
 *		  n, C and H to the messages (they're not doc'ed so don't
 *		  confuse the user with them).  No point adding usage
 *		  flags to zcat, it accepts flags but does not use them.
 *		- but hughd added -H to Usage after all.
 *		- Report the usage message of the utility invoked,
 *		  not by the operation requested.
 *	11 May 1992	scol!ianw	L011
 *		- The array of characters into which the two byte magic
 *		  number is read must have the same type as the array
 *		  magic_header. This only worked previously because of
 *		  a bug in the 3.2v2 DevSys which failed to convert to
 *		  integer type before comparing two characters.
 *		- Fix long/short mismatch warning (when building with
 *		  the 3.2v5 DevSys) by making mode the correct type.
 *	4 Nov 92	scol!johnfa	L012
 *		For XPG4 Conformance;
 *		- if appending .Z to filename would make name
 *		  exceed NAME_MAX bytes the command will now fail
 * 		  with an error status.
 *		- converted to use getopt() to follow Utility Syntax
 *		  Guidelines.
 *		- Added message catalogue functionality.
 *	30 Nov 92	scol!harveyt	L013
 *		- Changed perror() to psyserrorl() with more verbose message
 *		  for better error output.
 *	 3 Dec 92	scol!harveyt	L014
 *	 	- Modification L012 broke handle file argument "-" meaning
 *		  (un)compress standard input.
 *		- Modification L012 also broke handling multiple file name
 *		  arguments.
 *	        - Having multiple file arguments where one is "-" has never
 *		  worked (stdin was not read).  This now works, and it is an
 *		  error to have more than one "-" specified as a file argument.
 *		  This required adding new global input and output FILE*'s and
 *		  modifying compress(), decompress(), CheckMagic() and others
 *		  to use these instead of stdin/stdout.
 *		- A new function process_stdin() handles (un)compressing
 *		  stdin and consists of a large chunk of code taken from
 *		  main().
 *		- Don't compile in the M_XENIX code, with the above changes
 *		  this improves the speed of compression by 10%, and has
 *		  little change on uncompression.
 *	28 Jan 93	scol!anthonys	L015
 *		- Make the parsing of (numerical) option arguments more
 *		  robust. This is needed for the case of tar exec()ing
 *		  compress, when the options including option argument
 *		  are all passed in one string.
 *	31 Oct 94	scol!anthonys	L016
 *		- Added missing argument in error message, which was causing
 *		  a dereference of a null pointer.
 *		- Corrected a number of compiler warnings (unmarked).
 *	14 Mar 95	scol!ianw	L017
 *		- Simplified and corrected L005, the addition of a cast in
 *		  L016 revealed a bug, compress /filename gives an error.
 *	14 Jun 95	scol!ianw	L018
 *		- Internationalised the request of confirmation if file.Z
 *		  already exists by calling the libos routine nl_confirm().
 *	30 Jun 95	scol!ianw	L019
 *		- Internationalised the display of the usage messages.
 *	30 Jun 95	scol!ianw	L020
 *		- Changes required for XPG4 conformance:
 *		  o If -f is not specified and the standard input is not a
 *		    terminal an existing file should not be overwritten and
 *		    uncompress should exit with a status greater than zero.
 *		    To conform changed the exit status to 2 and read the
 *		    response to a prompt from the standard input (instead of
 *		    the standard error). Although the standard only requires
 *		    these changes for uncompress, to keep things consistent
 *		    the behaviour of compress is also changed.
 *		- Stopped a floating point exception seen when running
 *		  compress -vH on an empty file (done by making the call to
 *		  prratio() in lzh_compress() consistent with the call in
 *		  compress() where the count of the number of bytes input
 *		  starts at 1 not 0).
 *		- Only check for the .Z suffix if the length of the filename
 *		  is greater than 1 (this stops the character before the start
 *		  of the filename string from being referenced).
 *		- Ensure the input and output files are not the same, possible
 *		  if the files are linked (hard or symbolic) or if the
 *		  filesystem was mounted with -o trunc.
 *		- Use basename() instead of rindex().
 *		- Renamed dirname() to local_dirname() to avoid confusion with
 *		  the libgen routine.
 */

/* 
 * Compress - data compression program 
 */
#define min(a,b)	((a>b) ? b : a)

/*
 * machine variants which require cc -Dmachine:	 pdp11, z8000, pcxt
 */

/*
 * Set USERMEM to the maximum amount of physical user memory available
 * in bytes.  USERMEM is used to determine the maximum BITS that can be used
 * for compression.
 *
 * SACREDMEM is the amount of physical memory saved for others; compress
 * will hog the rest.
 */
#ifndef SACREDMEM
#define SACREDMEM	0
#endif

#ifndef USERMEM
# define USERMEM	450000	/* default user memory */
#endif

#ifdef interdata		/* (Perkin-Elmer) */
#define SIGNED_COMPARE_SLOW	/* signed compare is slower than unsigned */
#endif

#ifdef pdp11
# define BITS	12	/* max bits/code for 16-bit machine */
# define NO_UCHAR	/* also if "unsigned char" functions as signed char */
# undef USERMEM 
#endif /* pdp11 */	/* don't forget to compile with -i */

#ifdef z8000
# define BITS	12
# undef vax		/* weird preprocessor */
# undef USERMEM 
#endif /* z8000 */

#ifdef pcxt
# define BITS	12
# undef USERMEM
#endif /* pcxt */

#ifdef USERMEM
# if USERMEM >= (433484+SACREDMEM)
#  define PBITS 16
# else
#  if USERMEM >= (229600+SACREDMEM)
#   define PBITS	15
#  else
#   if USERMEM >= (127536+SACREDMEM)
#    define PBITS	14
#   else
#    if USERMEM >= (73464+SACREDMEM)
#     define PBITS	13
#    else
#     define PBITS	12
#    endif
#   endif
#  endif
# endif
# undef USERMEM
#endif /* USERMEM */

#ifdef PBITS		/* Preferred BITS for this memory size */
# ifndef BITS
#  define BITS PBITS
# endif /* BITS */
#endif /* PBITS */

#if BITS == 16
# define HSIZE	69001		/* 95% occupancy */
#endif
#if BITS == 15
# define HSIZE	35023		/* 94% occupancy */
#endif
#if BITS == 14
# define HSIZE	18013		/* 91% occupancy */
#endif
#if BITS == 13
# define HSIZE	9001		/* 91% occupancy */
#endif
#if BITS <= 12
# define HSIZE	5003		/* 80% occupancy */
#endif

#if defined(M_XENIX) && !defined(M_UNIX)			/* L014 */
#ifdef M_I86			/* Stupid compiler can't handle arrays with */
# if BITS == 16			/* more than 65535 bytes - so we fake it */
#  define XENIX_16
# else
#  if BITS > 13			/* Code only handles BITS = 12, 13, or 16 */
#   define BITS 13
#  endif
# endif
#endif
#endif

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
#if BITS > 15
typedef long int	code_int;
#else
typedef int		code_int;
#endif

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
#else
typedef long int	  count_int;
#endif

#ifdef NO_UCHAR
 typedef char	char_type;
#else
 typedef	unsigned char	char_type;
#endif /* UCHAR */
char_type magic_header[]     = { "\037\235" };	/* 1F 9D */
char_type lzh_magic_header[] = { "\037\240" };	/* 1F A0 */	/* S008 */

/* Defines for third byte of header */
#define BIT_MASK	0x1f
#define BLOCK_MASK	0x80
/* Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
   a fourth header byte (for expansion).
*/
#define INIT_BITS 9			/* initial number of bits/code */

/*
 * compress.c - File compression ala IEEE Computer, June 1984.
 *
 * Authors:	Spencer W. Thomas	(decvax!utah-cs!thomas)
 *		Jim McKie		(decvax!mcvax!jim)
 *		Steve Davies		(decvax!vax135!petsd!peora!srd)
 *		Ken Turkowski		(decvax!decwrl!turtlevax!ken)
 *		James A. Woods		(decvax!ihnp4!ames!jaw)
 *		Joe Orost		(decvax!vax135!petsd!joe)
 *
 * $Header$
 * $Log$
 * Revision 4.0	 85/07/30  12:50:00  joe
 * Removed ferror() calls in output routine on every output except first.
 * Prepared for release to the world.
 * 
 * Revision 3.6	 85/07/04  01:22:21  joe
 * Remove much wasted storage by overlaying hash table with the tables
 * used by decompress: tab_suffix[1<<BITS], stack[8000].  Updated USERMEM
 * computations.  Fixed dump_tab() DEBUG routine.
 *
 * Revision 3.5	 85/06/30  20:47:21  jaw
 * Change hash function to use exclusive-or.  Rip out hash cache.  These
 * speedups render the megamemory version defunct, for now.  Make decoder
 * stack global.  Parts of the RCS trunks 2.7, 2.6, and 2.1 no longer apply.
 *
 * Revision 3.4	 85/06/27  12:00:00  ken
 * Get rid of all floating-point calculations by doing all compression ratio
 * calculations in fixed point.
 *
 * Revision 3.3	 85/06/24  21:53:24  joe
 * Incorporate portability suggestion for M_XENIX.  Got rid of text on #else
 * and #endif lines.  Cleaned up #ifdefs for vax and interdata.
 *
 * Revision 3.2	 85/06/06  21:53:24  jaw
 * Incorporate portability suggestions for Z8000, IBM PC/XT from mailing list.
 * Default to "quiet" output (no compression statistics).
 *
 * Revision 3.1	 85/05/12  18:56:13  jaw
 * Integrate decompress() stack speedups (from early pointer mods by McKie).
 * Repair multi-file USERMEM gaffe.  Unify 'force' flags to mimic semantics
 * of SVR2 'pack'.  Streamline block-compress table clear logic.  Increase 
 * output byte count by magic number size.
 * 
 * Revision 3.0	  84/11/27  11:50:00  petsd!joe
 * Set HSIZE depending on BITS.	 Set BITS depending on USERMEM.	 Unrolled
 * loops in clear routines.  Added "-C" flag for 2.0 compatibility.  Used
 * unsigned compares on Perkin-Elmer.  Fixed foreground check.
 *
 * Revision 2.7	  84/11/16  19:35:39  ames!jaw
 * Cache common hash codes based on input statistics; this improves
 * performance for low-density raster images.  Pass on #ifdef bundle
 * from Turkowski.
 *
 * Revision 2.6	  84/11/05  19:18:21  ames!jaw
 * Vary size of hash tables to reduce time for small files.
 * Tune PDP-11 hash function.
 *
 * Revision 2.5	  84/10/30  20:15:14  ames!jaw
 * Junk chaining; replace with the simpler (and, on the VAX, faster)
 * double hashing, discussed within.  Make block compression standard.
 *
 * Revision 2.4	  84/10/16  11:11:11  ames!jaw
 * Introduce adaptive reset for block compression, to boost the rate
 * another several percent.  (See mailing list notes.)
 *
 * Revision 2.3	  84/09/22  22:00:00  petsd!joe
 * Implemented "-B" block compress.  Implemented REVERSE sorting of tab_next.
 * Bug fix for last bits.  Changed fwrite to putchar loop everywhere.
 *
 * Revision 2.2	  84/09/18  14:12:21  ames!jaw
 * Fold in news changes, small machine typedef from thomas,
 * #ifdef interdata from joe.
 *
 * Revision 2.1	  84/09/10  12:34:56  ames!jaw
 * Configured fast table lookup for 32-bit machines.
 * This cuts user time in half for b <= FBITS, and is useful for news batching
 * from VAX to PDP sites.  Also sped up decompress() [fwrite->putc] and
 * added signal catcher [plus beef in writeerr()] to delete effluvia.
 *
 * Revision 2.0	  84/08/28  22:00:00  petsd!joe
 * Add check for foreground before prompting user.  Insert maxbits into
 * compressed file.  Force file being uncompressed to end with ".Z".
 * Added "-c" flag and "zcat".	Prepared for release.
 *
 * Revision 1.10  84/08/24  18:28:00  turtlevax!ken
 * Will only compress regular files (no directories), added a magic number
 * header (plus an undocumented -n flag to handle old files without headers),
 * added -f flag to force overwriting of possibly existing destination file,
 * otherwise the user is prompted for a response.  Will tack on a .Z to a
 * filename if it doesn't have one when decompressing.	Will only replace
 * file if it was compressed.
 *
 * Revision 1.9	 84/08/16  17:28:00  turtlevax!ken
 * Removed scanargs(), getopt(), added .Z extension and unlimited number of
 * filenames to compress.  Flags may be clustered (-Ddvb12) or separated
 * (-D -d -v -b 12), or combination thereof.  Modes and other status is
 * copied with copystat().  -O bug for 4.2 seems to have disappeared with
 * 1.8.
 *
 * Revision 1.8	 84/08/09  23:15:00  joe
 * Made it compatible with vax version, installed jim's fixes/enhancements
 *
 * Revision 1.6	 84/08/01  22:08:00  joe
 * Sped up algorithm significantly by sorting the compress chain.
 *
 * Revision 1.5	 84/07/13  13:11:00  srd
 * Added C version of vax asm routines.	 Changed structure to arrays to
 * save much memory.  Do unsigned compares where possible (faster on
 * Perkin-Elmer)
 *
 * Revision 1.4	 84/07/05  03:11:11  thomas
 * Clean up the code a little and lint it.  (Lint complains about all
 * the regs used in the asm, but I'm not going to "fix" this.)
 *
 * Revision 1.3	 84/07/05  02:06:54  thomas
 * Minor fixes.
 *
 * Revision 1.2	 84/07/05  00:27:27  thomas
 * Add variable bit length output.
 *
 */
static char rcs_ident[] = "$Header$";

#include <stdio.h>
#include <stdlib.h>						/* L020 */
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>			/* L004, for PATHSIZE */
#include <unistd.h>			/* L005, for _PC_NAME_MAX */
#include <string.h>			/* L005 */
#include <libgen.h>						/* L020 */
/* #include <errormsg.h> */			/* L012 Start */
#include <errno.h>
#include "../include/osr.h"
#ifdef INTL
#  include <locale.h>
#  include "compress_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str)	(str)
#  define catclose(d)		/*void*/
#endif /* INTL */
char *command_name = "compress";	/* L012 Stop */

#ifdef notdef
#include <sys/ioctl.h>
#endif

								/* L010 { */
#define	ZCAT_PROG	0	/* zcat prog */
#define	UNCOMP_PROG	1	/* uncompress prog */
#define	COMP_PROG	2	/* compress prog */
static short we_are_prog = COMP_PROG;	/* what prog we were called as */
								/* L010 } */

int n_bits;				/* number of bits/code */
int maxbits = BITS;			/* user settable max # bits/code */
code_int maxcode;			/* maximum code, given n_bits */
code_int maxmaxcode = 1 << BITS;	/* should NEVER generate this code */
#ifdef COMPATIBLE		/* But wrong! */
# define MAXCODE(n_bits)	(1 << (n_bits) - 1)
#else
# define MAXCODE(n_bits)	((1 << (n_bits)) - 1)
#endif /* COMPATIBLE */

#ifdef XENIX_16
count_int htab0[8192];
count_int htab1[8192];
count_int htab2[8192];
count_int htab3[8192];
count_int htab4[8192];
count_int htab5[8192];
count_int htab6[8192];
count_int htab7[8192];
count_int htab8[HSIZE-65536];
count_int * htab[9] = {
	htab0, htab1, htab2, htab3, htab4, htab5, htab6, htab7, htab8 };

#define htabof(i)	(htab[(i) >> 13][(i) & 0x1fff])
unsigned short code0tab[16384];
unsigned short code1tab[16384];
unsigned short code2tab[16384];
unsigned short code3tab[16384];
unsigned short code4tab[16384];
unsigned short * codetab[5] = {
	code0tab, code1tab, code2tab, code3tab, code4tab };

#define codetabof(i)	(codetab[(i) >> 14][(i) & 0x3fff])

#else	/* Normal machine */

#ifdef sel	/* gould base register braindamage */
/*NOBASE*/
count_int htab [HSIZE];
unsigned short codetab [HSIZE];
/*NOBASE*/
#else
count_int htab [HSIZE];
unsigned short codetab [HSIZE];
#endif /* sel */

#define htabof(i)	htab[i]
#define codetabof(i)	codetab[i]
#endif	/* XENIX_16 */
code_int hsize = HSIZE;			/* for dynamic table sizing */
count_int fsize;

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.	 The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

#define tab_prefixof(i) codetabof(i)
#ifdef XENIX_16
# define tab_suffixof(i)	((char_type *)htab[(i)>>15])[(i) & 0x7fff]
# define de_stack		((char_type *)(htab2))
#else	/* Normal machine */
# define tab_suffixof(i)	((char_type *)(htab))[i]
# define de_stack		((char_type *)&tab_suffixof(1<<BITS))
#endif	/* XENIX_16 */

code_int free_ent = 0;			/* first unused entry */
int exit_stat = 0;			/* per-file status */
int perm_stat = 0;			/* permanent status */

code_int getcode();

int nomagic = 0;	/* Use a 3-byte magic number header, unless old file */
int zcat_flg = 0;	/* Write output on stdout, suppress messages */
int precious = 1;	/* Don't unlink output file on interrupt */
int quiet = 1;		/* don't tell me about compression */
int do_decomp = 0;

Usage() {
#ifdef DEBUG
fprintf(stderr,"Usage: compress [-dDVfc] [-b maxbits] [file ...]\n");
}
int debug = 0;
#else						/* L007 L010 L019 begin */
	if (we_are_prog == ZCAT_PROG)
		(void) fprintf(stderr, MSGSTR(COMPRESS_USAGE1,
			"Usage: zcat [file]\n"));
	else if (we_are_prog == UNCOMP_PROG)
		(void) fprintf(stderr, MSGSTR(COMPRESS_USAGE2,
			"Usage: uncompress [-cfFqv] [file | -P fd]\n"));
	else /* must be COMP_PROG */
		(void) fprintf(stderr, MSGSTR(COMPRESS_USAGE3,
		"Usage: compress [-cdfFqv] [-H | -b bits] [file | -P fd]\n"));
}						/* L007 L010 L019 end */
#endif /* DEBUG */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
int block_compress = BLOCK_MASK;
int clear_flg = 0;
long int ratio = 0;
#define CHECK_GAP 10000 /* ratio check interval */
count_int checkpoint = CHECK_GAP;
/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */ 
#define FIRST	257	/* first free entry */
#define CLEAR	256	/* table clear output code */

int force = 0;
char ofname [PATHSIZE];		/* L004 */
int rpipe, Pflag=0;		/* S002 Read end of pipe and flag.   */
char *ctmp="Comprtmp";		/* S002 Tmp file used in do_Pflag.   */
#ifdef DEBUG
int verbose = 0;
#endif /* DEBUG */

static void (*oldint)(int);
static void onintr(int);
static void oops(int);
int bgnd_flag;

FILE *finput = stdin;						/* L014 */
FILE *foutput = stdout;						/* L014 */

/*****************************************************************
 * TAG( main )
 *
 * Algorithm from "A Technique for High Performance Data Compression",
 * Terry A. Welch, IEEE Computer Vol 17, No 6 (June 1984), pp 8-19.
 *
 * Usage: compress [-dfvc] [-b bits] [file ...]
 * Inputs:
 *	-d:	    If given, decompression is done instead.
 *
 *	-c:	    Write output on stdout, don't remove original.
 *
 *	-b:	    Parameter limits the max number of bits/code.
 *
 *	-f:	    Forces output file to be generated, even if one already
 *		    exists, and even if no space is saved by compressing.
 *		    If -f is not used, the user will be prompted if stdin is
 *		    a tty, otherwise, the output file will not be overwritten.
 *
 *	-v:	    Write compression statistics
 *
 *	-P:	    Parameter is the read end of a pipe. File names are read
 *		    from pipe until a Null is read or the pipe is closed. 
 *		    Always overwrites the original file. 
 *
 *	file ...:   Files to be compressed.  If none specified, and not
 *		    -P option then stdin is used.
 * Outputs:
 *	file.Z:	    Compressed form of file with same mode, owner, and utimes
 *	or stdout   (if stdin used as input)
 *
 * Assumptions:
 *	When filenames are given, replaces with the compressed version
 *	(.Z suffix) only if the file decreases in size.
 * Algorithm:
 *	Modified Lempel-Ziv method (LZW).  Basically finds common
 * substrings and replaces them with a variable size code.  This is
 * deterministic, and can be done on the fly.  Thus, the decompression
 * procedure needs no input table, but tracks the way the table was built.
 */

main( argc, argv )
register int argc; char **argv;
{
    int overwrite = 0;	/* Do not overwrite unless given -f flag */
    char tempname[PATHSIZE];				/* L004 */
    char **filelist, **fileptr;
    char *cp;
    int lzhAlgorithm = 0;					/* S008 */
    struct stat statbuf;
    int done_stdin = 0;						/* L014 */
    int i;							/* L012 Start */
    char c;
    char *ptr;							/* L015 */
    extern char *optarg;
    extern int optind;

#ifdef INTL
    setlocale(LC_ALL,"");
    catd=catopen(MF_COMPRESS,MC_FLAGS);
#endif /* INTL */						/* L012 Stop */


    /* This bg check only works for sh. */
    if ( (oldint = signal ( SIGINT, SIG_IGN )) != SIG_IGN ) {
	signal ( SIGINT, onintr );
	signal ( SIGSEGV, oops );
    }
    bgnd_flag = oldint != SIG_DFL;
#ifdef notdef	  /* This works for csh but we don't want it. */
    { int tgrp;
    if (bgnd_flag == 0 && ioctl(2, TIOCGPGRP, (char *)&tgrp) == 0 &&
      getpgrp(0) != tgrp)
	bgnd_flag = 1;
    }
#endif
    
#ifdef COMPATIBLE
    nomagic = 1;	/* Original didn't have a magic number */
#endif /* COMPATIBLE */

    filelist = fileptr = (char **)(malloc(argc * sizeof(*argv)));
    *filelist = NULL;

    cp = basename(argv[0]);					/* L020 */

    if(strcmp(cp, "uncompress") == 0) {
	we_are_prog = UNCOMP_PROG;				/* L010 */
	command_name = "uncompress";				/* L012 */
	do_decomp = 1;
    } else if(strcmp(cp, "zcat") == 0) {
	we_are_prog = ZCAT_PROG;				/* L010 */
	command_name = "zcat";					/* L012 */
	do_decomp = 1;
	zcat_flg = 1;
    }

#ifdef BSD4_2
    /* 4.2BSD dependent - take it out if not */
    setlinebuf( stderr );
#endif /* BSD4_2 */

    /* Argument Processing
     * All flags are optional.
     * -D => debug
     * -V => print Version; debug verbose
     * -d => do_decomp
     * -v => unquiet
     * -f => force overwrite of output file
     * -n => no header: useful to uncompress old files
     * -b maxbits => maxbits.  If -b is specified, then maxbits MUST be
     *	    given also.
     * -c => cat all output to stdout
     * -C => generate output compatible with compress 2.0.
     * -P pipe_desc => Pass file names through a pipe in <stdio.h> defined
     *       BUFSIZ data chunks whose read end is defined by pipe_desc. 
     *       Always overwrites original file. 
     * if a string is left, must be an input filename.
     */
								/* L012 Start */
#ifdef DEBUG
	    while ((c = getopt(argc, argv, "DVdvFfnb:cCP:Hq"))!=-1) {	
#else
	    while ((c = getopt(argc, argv, "VdvFfnb:cCP:Hq"))!=-1) {	
#endif /* DEBUG */
					/* Process all flags in this arg */
		switch (c) {
#ifdef DEBUG
		    case 'D':
			debug = 1;
			break;
		    case 'V':
			verbose = 1;
			version();
			break;
#else
		    case 'V':
			version();
			break;
#endif /* DEBUG */
		    case 'v':
			quiet = 0;
			break;
		    case 'd':
			do_decomp = 1;
			break;
		    case 'f':
		    case 'F':
			overwrite = 1;
			force = 1;
			break;
		    case 'n':
			nomagic = 1;
			break;
		    case 'C':
			block_compress = 0;
			break;
		    case 'b':
			maxbits = 0;
			errno = 0;					/* L015 begin */
			maxbits = strtol(optarg, &ptr, 0);
			if (errno || *ptr || maxbits < 0 ){
				errorl(MSGSTR(COMPRESS_ERR_OPTARG, "Invalid argument %s given to -%c option"), optarg, c);
				exit(1);				/* L015 end */
			}
			break;
		    case 'c':
			zcat_flg = 1;
			break;
		    case 'q':
			quiet = 1;
			break;
		    case 'P':
			rpipe = 0;
			errno = 0;					/* L015 begin */
			rpipe = strtol(optarg, &ptr, 0);
			if (errno || *ptr || rpipe < 0 ){
				errorl(MSGSTR(COMPRESS_ERR_OPTARG, "Invalid argument %s given to -%c option"), optarg, c);
				exit(1);				/* L015 end */
			}
			Pflag=1;
			break;
                    case 'H':					/*  S008 */
                       lzhAlgorithm = 1;  /* Use Lempel-Ziv-Huffman S008 */
                       break;					/*  S008 */
		    default:
			Usage();
			exit(1);
		}
	    }
	    		/* Input file name */
	    argv = &argv[optind];
	    for (i = optind; i < argc; i++)			/* L014 */
		    *fileptr++ = *argv++;			/* L014 */
	    *fileptr = NULL;
								/* L012 Stop */


    if (lzhAlgorithm && do_decomp) {				/* S008 */
        errorl(MSGSTR(COMPRESS_HFLAG,
		"-H only allowed on compression")); 	 /* S008, L012 */
        exit (1);						/* S008 */
    }								/* S008 */

    if(maxbits < INIT_BITS) maxbits = INIT_BITS;
    if (maxbits > BITS) maxbits = BITS;
    maxmaxcode = 1 << maxbits;

    /* S002 Read names from a pipe and compress/decompress overwriting 	*/
    /* the original file. 						*/ 
    if (Pflag) 	do_Pflag (lzhAlgorithm);			/* S008 */

    if (*filelist != NULL) {
	for (fileptr = filelist; *fileptr; fileptr++) {
	    exit_stat = 0;

	    if (strcmp (*fileptr, "-") == 0) {			/* L014 Begin */
		    if (done_stdin++) {
			    errorl(MSGSTR(COMPRESS_DONESTDIN,
					  "stdin: already processed"));
		    } else {
			    process_stdin (lzhAlgorithm);
		    }
		    continue;
	    }							/* L014 End */

	    if (do_decomp) {			/* DECOMPRESSION */
		int filenamelen;				/* L020 */
		filenamelen = strlen(*fileptr);			/* L020 */
		/* Check for .Z suffix */
		if ((filenamelen < 2) ||			/* L020 */
		    strcmp(*fileptr + filenamelen - 2, ".Z") != 0) {
		    /* No .Z: tack one on */
		    strcpy(tempname, *fileptr);
		    strcat(tempname, ".Z");
		    *fileptr = tempname;
		}
		/* Open input file */
		if (finput != stdin) fclose (finput);		/* L014 */
		if ((finput = fopen(*fileptr, "r")) == NULL) {	/* L014 */
		    psyserrorl(errno, MSGSTR(COMPRESS_NOREAD,
			     "could not read %s"), *fileptr);	/* L013 */
		    perm_stat = 1;
		    continue;
		}
		/* Check the magic number */
		if (nomagic == 0) {
                    lzhAlgorithm = CheckMagic ();		/* S008 */
                    if (lzhAlgorithm < 0) {			/* S008 */
			errorl(MSGSTR(COMPRESS_NOTCOMP,
		  	    "%s: not in compressed format"),
			    *fileptr);				/* L012 */
                        continue;
		    }
                    if (!lzhAlgorithm) {			/* S008 */
			/* set -b from file */
                        maxbits = getc (finput);		/* L014 */
                        block_compress = maxbits & BLOCK_MASK;
                        maxbits &= BIT_MASK;
                        maxmaxcode = 1 << maxbits;
                        if(maxbits > BITS) {
                            errorl(MSGSTR(COMPRESS_MAX_BITS,
                            "%s: compressed with %d bits, can only handle %d bits"),
                            *fileptr, maxbits, BITS);		/* L012 */
                            continue;
                        }
                    }
		}
		/* Generate output filename */
		strcpy(ofname, *fileptr);
		ofname[strlen(*fileptr) - 2] = '\0';  /* Strip off .Z */
	    } else {					/* COMPRESSION */
		int filenamelen;				/* L020 */
		filenamelen = strlen(*fileptr);			/* L020 */
		if ((filenamelen > 1) &&			/* L020 */
		    strcmp(*fileptr + filenamelen - 2, ".Z") == 0) {
			errorl(MSGSTR(COMPRESS_HASUFIX,
			    "%s: already has .Z suffix -- no change"),
			    *fileptr);				/* L012 */
		    continue;
		}
		/* Open input file */
		if (finput != stdin) fclose (finput);		/* L014 */
		if ((finput = fopen(*fileptr, "r")) == NULL) {	/* L014 */
		    psyserrorl(errno, MSGSTR(COMPRESS_NOREAD,
			     "could not read %s"), *fileptr);	/* L013 */
		    perm_stat = 1;
		    continue;
		}
		stat ( *fileptr, &statbuf );
		fsize = (long) statbuf.st_size;
		/*
		 * tune hash table size for small files -- ad hoc,
		 * but the sizes match earlier #defines, which
		 * serve as upper bounds on the number of output codes. 
		 */
		hsize = HSIZE;
		if ( fsize < (1 << 12) )
		    hsize = min ( 5003, HSIZE );
		else if ( fsize < (1 << 13) )
		    hsize = min ( 9001, HSIZE );
		else if ( fsize < (1 << 14) )
		    hsize = min ( 18013, HSIZE );
		else if ( fsize < (1 << 15) )
		    hsize = min ( 35023, HSIZE );
		else if ( fsize < 47000 )
		    hsize = min ( 50021, HSIZE );

		/* Generate output filename */
		strcpy(ofname, *fileptr);
#ifndef BSD4_2		/* Short filenames */
		if ((cp=strrchr(ofname,'/')) != NULL)		/* L017 begin */
			cp++;
		else
			cp = ofname;
		if ((long)strlen(cp) > (pathconf(ofname, _PC_NAME_MAX) - 2)) {
			errorl(MSGSTR(COMPRESS_TOOLONG,
				"%s: filename too long to tack on .Z"), cp);
			perm_stat = 1;
			continue;
		}						/* L017 end */
#endif	/* BSD4_2		Long filenames allowed */
		strcat(ofname, ".Z");
	    }
	    /* Check for overwrite of existing file */
								/* L020 begin */
	    if (zcat_flg == 0 && stat(ofname, &statbuf) == 0) {
		struct stat isbuf;
		/* Ensure the input and output files are not the same */
		fstat(fileno(finput), &isbuf);
		if ((isbuf.st_dev == statbuf.st_dev) &&
		    (isbuf.st_ino == statbuf.st_ino)) {
			errorl(MSGSTR(COMPRESS_SAMEFILE,
			"input filename: %s and output filename: %s refer to the same file -- no change"), *fileptr, ofname);
			perm_stat = 1;
			continue;
		}
		if (overwrite == 0) {				/* L020 end */
#ifdef INTL							/* L018 begin */
		    int reply = 0;
		    fprintf(stderr,MSGSTR(COMPRESS_EXISTS,
			"%s: %s already exists;"), command_name, ofname);
		    if (bgnd_flag == 0 && isatty(0)) {		/* L020 */
			fprintf(stderr,MSGSTR(COMPRESS_OVERWRT,
			   " do you wish to overwrite %s (y or n)? "), ofname);
			fflush(stderr);
			reply = nl_confirm(0);			/* L020 */
		    }
		    else {					/* L020 begin */
			exit_stat = 2;
		    }						/* L020 end */
		    switch (reply) {
		    case -1:
			psyserrorl (errno,
			MSGSTR(COMPRESS_NOINPUT, "could not read from stderr"));
			/* FALLTHROUGH */
		    case 0:
			errorl(MSGSTR(COMPRESS_NOTOVER, "\tnot overwritten"));
			continue;
		    }
#else								/* L018 end */
		    char response[2];
		    response[0] = 'n';
		    fprintf(stderr,MSGSTR(COMPRESS_EXISTS,	/* L012 */
			"%s: %s already exists;"), command_name, ofname);
		    if (bgnd_flag == 0 && isatty(0)) {		/* L020 */
			fprintf(stderr,MSGSTR(COMPRESS_OVERWRT, /* L012 */
			   " do you wish to overwrite %s (y or n)? "), ofname);
			fflush(stderr);
			if (read(0, response, 2) < 0) { /*L020*//* L013 Start */
			    psyserrorl(errno, MSGSTR(COMPRESS_NOINPUT,
				      "could not read from stderr"));
			} else {
			    while (response[1] != '\n') {
				if (read(0, response+1, 1) < 0) { /* L020 */
				    psyserrorl (errno,
			MSGSTR(COMPRESS_NOINPUT, "could not read from stderr"));
				    break;
				}
			    }
			}					/* L013 End */
		    }
		    else {					/* L020 begin */
			exit_stat = 2;
		    }						/* L020 end */
		    if (response[0] != 'y') {
			errorl(MSGSTR(COMPRESS_NOTOVER, "\tnot overwritten"));
								/* L012 */
			continue;
		    }
#endif /* INTL */						/* L018 */
		}
	    }
	    if(zcat_flg == 0) {		/* Open output file */
		if (foutput != stdout) fclose (foutput);	/* L014 */
		if ((foutput = fopen(ofname, "w")) == NULL) {	/* L014 */
		    psyserrorl(errno, MSGSTR(COMPRESS_NOWRITE,
			       "could not write %s"), ofname);	/* L013 */
		    perm_stat = 1;
		    continue;
		}
		precious = 0;
		if(!quiet)
			fprintf(stderr, "%s: ", *fileptr);
	    } else {						/* L014 Begin */
		if (foutput != stdout) fclose (foutput);
		foutput = stdout;
	    }							/* L014 End */

	    /* Actually do the compression/decompression */
            if (lzhAlgorithm) {					/* S008 begin */
                if (do_decomp)
                    lzh_decompress ();
                else {
                    lzh_compress ();
                }
            } else {						/* S008 end */
	        if (do_decomp == 0) 	compress();
#ifndef DEBUG
	        else			decompress();
#else
	        else if (debug == 0)	decompress();
	        else			printcodes();
	        if (verbose)		dump_tab();
#endif /* DEBUG */
            }
	    if(zcat_flg == 0) {
		copystat(*fileptr, ofname);	/* Copy stats */
		precious = 1;
		if((exit_stat == 1) || (!quiet))
			putc('\n', stderr);
	    }
	}
    } else {		/* Standard input */
	    process_stdin (lzhAlgorithm);			/* L014 */
    }
    exit(perm_stat ? perm_stat : exit_stat);
    /*NOTREACHED*/		/* L004 */
}

process_stdin (lzhAlgorithm)					/* L014 Begin */
	int lzhAlgorithm;
{
	if (finput != stdin) fclose (finput);
	if (foutput != stdout) fclose (foutput);
	finput = stdin;
	foutput = stdout;

	if (do_decomp == 0) {
		if (lzhAlgorithm)				/* S008 */
		    lzh_compress ();				/* S008 */
		else {						/* S008 */
		    compress();
#ifdef DEBUG
		    if(verbose)		dump_tab();
#endif /* DEBUG */
                }
		if(!quiet)
			putc('\n', stderr);
	} else {
	    /* Check the magic number */
	    if (nomagic == 0) {
		lzhAlgorithm = CheckMagic ();			/* S008 */
		if (lzhAlgorithm < 0) {				/* S008 */
		    errorl(MSGSTR(COMPRESS_NOTCMPFMT, 
			"stdin: not in compressed format"));	/* L012 */
		    return;					/* L014 */
		}
		if (!lzhAlgorithm) {				/* S008 */
		    /* set -b from file */
		    maxbits = getc (finput);			/* L014 */
		    block_compress = maxbits & BLOCK_MASK;
		    maxbits &= BIT_MASK;
		    maxmaxcode = 1 << maxbits;
		    fsize = 100000;	/* assume stdin large for USERMEM */
                    if(maxbits > BITS) {
			    errorl(MSGSTR(COMPRESS_TOOMNYBITS,
		    "stdin: compressed with %d bits, can only handle %d bits"),
			    maxbits, BITS);			/* L012 */
			    return;				/* L014 */
		    }
		}
	    }
	    if (lzhAlgorithm)					/* S008 */
		    lzh_decompress ();				/* S008 */
	    else {						/* S008 */
#ifndef DEBUG
		decompress();
#else
		if (debug == 0)	decompress();
		else		printcodes();
		if (verbose)	dump_tab();
#endif /* DEBUG */
	    }
	}
}								/* L014 End */
							/* S008 begin */
/*
 * CheckMagic --
 *   Read two bytes from finput stream and validate it as a magic number.
 * Returns:
 *   o -1 - If magic number is invalid.
 *   o  0 - If standard compressed file.
 *   o  1 - If Lempel-Ziv-Huffman compressed file.
 */
int
CheckMagic ()
{
   char_type header [2];					/* L011 */
   
   header [0] = getc (finput) & 0xFF;				/* L014 */
   header [1] = getc (finput) & 0xFF;				/* L014 */

   if ((header [0] == magic_header [0]) && 
           (header [1] == magic_header [1]))
       return 0;  /* Standard compression. */

   if ((header [0] == lzh_magic_header [0]) && 
           (header [1] == lzh_magic_header [1]))
       return 1;  /* Lempel-Ziv-Huffman compression. */
   
   return -1;
}

/*
 * lzh_compress --
 *   Compress finput to foutput using Lempel-Ziv-Huffman compression.
 *   Includes writing the magic number.
 */
lzh_compress ()
{
    unsigned long  originalSize;
    unsigned long  compressSize;

    putc (lzh_magic_header[0], foutput);			/* L014 */
    putc (lzh_magic_header[1], foutput);			/* L014 */
    lzh_encode (finput, foutput, &originalSize, &compressSize);	/* L014 */

    if (zcat_flg == 0 && !quiet) {
        fprintf( stderr, MSGSTR(COMPRESS_COMP,"Compression: ")); /* L012 */
        prratio( stderr,					/* L020 begin */
		originalSize == 0 ? 1-compressSize : originalSize-compressSize,
		originalSize == 0 ? 1 : originalSize);		/* L020 end */
    }
    if(compressSize > originalSize)	/* exit(2) if no savings   L009 */
	exit_stat = 2;						/* L009 */
}

/*
 * lzh_decompress --
 *   Decompress finput to foutput using Lempel-Ziv-Huffman compression.
 */
lzh_decompress ()
{
    lzh_decode (finput, foutput);				/* L014 */
}								/* S008 end */

static int offset;
long int in_count = 1;			/* length of input */
long int bytes_out;			/* length of compressed output */
long int out_count = 0;			/* # of codes output (for debugging) */

/*
 * compress finput to foutput
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the 
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.	The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

compress() {
    register long fcode;
    register code_int i = 0;
    register int c;
    register code_int ent;
#ifdef XENIX_16
    register code_int disp;
#else	/* Normal machine */
    register int disp;
#endif
    register code_int hsize_reg;
    register int hshift;

#ifndef COMPATIBLE
    if (nomagic == 0) {
	putc (magic_header[0], foutput);			/* L014 */
	putc (magic_header[1], foutput);			/* L014 */
	putc ((char)(maxbits | block_compress), foutput);	/* L014 */
	if(ferror(foutput))					/* L014 */
		writeerr();
    }
#endif /* COMPATIBLE */

    offset = 0;
    bytes_out = 3;		/* includes 3-byte header mojo */
    out_count = 0;
    clear_flg = 0;
    ratio = 0;
    in_count = 1;
    checkpoint = CHECK_GAP;
    maxcode = MAXCODE(n_bits = INIT_BITS);
    free_ent = ((block_compress) ? FIRST : 256 );

    ent = getc (finput);					/* L014 */

    hshift = 0;
    for ( fcode = (long) hsize;	 fcode < 65536L; fcode *= 2L )
	hshift++;
    hshift = 8 - hshift;		/* set hash code range bound */

    hsize_reg = hsize;
    cl_hash( (count_int) hsize_reg);		/* clear hash table */

#ifdef SIGNED_COMPARE_SLOW
    while ( (c = getc (finput)) != (unsigned) EOF ) {		/* L014 */
#else
    while ( (c = getc (finput)) != EOF ) {			/* L014 */
#endif
	in_count++;
	fcode = (long) (((long) c << maxbits) + ent);
	i = ((c << hshift) ^ ent);	/* xor hashing */

	if ( htabof (i) == fcode ) {
	    ent = codetabof (i);
	    continue;
	} else if ( (long)htabof (i) < 0 )	/* empty slot */
	    goto nomatch;
	disp = hsize_reg - i;		/* secondary hash (after G. Knott) */
	if ( i == 0 )
	    disp = 1;
probe:
	if ( (i -= disp) < 0 )
	    i += hsize_reg;

	if ( htabof (i) == fcode ) {
	    ent = codetabof (i);
	    continue;
	}
	if ( (long)htabof (i) > 0 ) 
	    goto probe;
nomatch:
	output ( (code_int) ent );
	out_count++;
	ent = c;
#ifdef SIGNED_COMPARE_SLOW
	if ( (unsigned) free_ent < (unsigned) maxmaxcode) {
#else
	if ( free_ent < maxmaxcode ) {
#endif
	    codetabof (i) = free_ent++; /* code -> hashtable */
	    htabof (i) = fcode;
	}
	else if ( (count_int)in_count >= checkpoint && block_compress )
	    cl_block ();
    }
    /*
     * Put out the final code.
     */
    output( (code_int)ent );
    out_count++;
    output( (code_int)-1 );

    /*
     * Print out stats on stderr
     */
    if(zcat_flg == 0 && !quiet) {
#ifdef DEBUG
	fprintf( stderr,
		"%ld chars in, %ld codes (%ld bytes) out, compression factor: ",
		in_count, out_count, bytes_out );
	prratio( stderr, in_count, bytes_out );
	fprintf( stderr, "\n");
	fprintf( stderr, "\tCompression as in compact: " );
	prratio( stderr, in_count-bytes_out, in_count );
	fprintf( stderr, "\n");
	fprintf( stderr, "\tLargest code (of last block) was %d (%d bits)\n",
		free_ent - 1, n_bits );
#else /* !DEBUG */
        fprintf( stderr, MSGSTR(COMPRESS_COMP,"Compression: ")); /* L012 */
	prratio( stderr, in_count-bytes_out, in_count );
#endif /* DEBUG */
    }
    if(bytes_out > in_count)	/* exit(2) if no savings */
	exit_stat = 2;
    return;
}

/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *	code:	A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *		that n_bits =< (long)wordsize - 1.
 * Outputs:
 *	Outputs code to the file.
 * Assumptions:
 *	Chars are 8 bits long.
 * Algorithm:
 *	Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).	Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static char buf[BITS];

#ifndef vax
char_type lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
char_type rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
#endif /* vax */

output( code )
code_int  code;
{
#ifdef DEBUG
    static int col = 0;
#endif /* DEBUG */

    /*
     * On the VAX, it is important to have the register declarations
     * in exactly the order given, or the asm will break.
     */
    register int r_off = offset, bits= n_bits;
    register char * bp = buf;

#ifdef DEBUG
	if ( verbose )
	    fprintf( stderr, "%5d%c", code,
		    (col+=6) >= 74 ? (col = 0, '\n') : ' ' );
#endif /* DEBUG */
    if ( code >= 0 ) {
#ifdef vax
	/* VAX DEPENDENT!! Implementation on other machines is below.
	 *
	 * Translation: Insert BITS bits from the argument starting at
	 * offset bits from the beginning of buf.
	 */
	0;	/* Work around for pcc -O bug with asm and if stmt */
	asm( "insv	4(ap),r11,r10,(r9)" );
#else /* not a vax */
/* 
 * byte/bit numbering on the VAX is simulated by the following code
 */
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/*
	 * Since code is always >= 8 bits, only need to mask the first
	 * hunk on the left.
	 */
	*bp = (*bp & rmask[r_off]) | (code << r_off) & lmask[r_off];
	bp++;
	bits -= (8 - r_off);
	code >>= 8 - r_off;
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
	    *bp++ = code;
	    code >>= 8;
	    bits -= 8;
	}
	/* Last bits. */
	if(bits)
	    *bp = code;
#endif /* vax */
	offset += n_bits;
	if ( offset == (n_bits << 3) ) {
	    bp = buf;
	    bits = n_bits;
	    bytes_out += bits;
	    do
		putc (*bp++, foutput);				/* L014 */
	    while(--bits);
	    offset = 0;
	}

	/*
	 * If the next entry is going to be too big for the code size,
	 * then increase it, if possible.
	 */
	if ( free_ent > maxcode || (clear_flg > 0))
	{
	    /* 
	     * Write the whole buffer, because the input side won't
	     * discover the size increase until after it has read it.
	     */
	    if ( offset > 0 ) {
		if( fwrite( buf, 1, n_bits, foutput ) != n_bits) /* L014 */
			writeerr();
		bytes_out += n_bits;
	    }
	    offset = 0;

	    if ( clear_flg ) {
		maxcode = MAXCODE (n_bits = INIT_BITS);
		clear_flg = 0;
	    }
	    else {
		n_bits++;
		if ( n_bits == maxbits )
		    maxcode = maxmaxcode;
		else
		    maxcode = MAXCODE(n_bits);
	    }
#ifdef DEBUG
	    if ( debug ) {
		fprintf( stderr, "\nChange to %d bits\n", n_bits );
		col = 0;
	    }
#endif /* DEBUG */
	}
    } else {
	/*
	 * At EOF, write the rest of the buffer.
	 */
	if ( offset > 0 )
	    fwrite( buf, 1, (offset + 7) / 8, foutput );	/* L014 */
	bytes_out += (offset + 7) / 8;
	offset = 0;
	fflush( foutput );					/* L014 */
#ifdef DEBUG
	if ( verbose )
	    fprintf( stderr, "\n" );
#endif /* DEBUG */
	if( ferror( foutput ) )					/* L014 */
		writeerr();
    }
}

/*
 * Decompress finput to foutput. This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  The tables used herein are shared
 * with those of the compress() routine.  See the definitions above.
 */

decompress() {
    register char_type *stackp;
    register int finchar;
    register code_int code, oldcode, incode;

    /*
     * As above, initialize the first 256 entries in the table.
     */
    maxcode = MAXCODE(n_bits = INIT_BITS);
    for ( code = 255; code >= 0; code-- ) {
	tab_prefixof(code) = 0;
	tab_suffixof(code) = (char_type)code;
    }
    free_ent = ((block_compress) ? FIRST : 256 );

    finchar = oldcode = getcode();
    if(oldcode == -1)	/* EOF already? */
	return;			/* Get out of here */
    /* first code must be 8 bits = char */
    putc( (char)finchar, foutput );				/* L014 */
    /* Crash if can't write */
    if(ferror(foutput))						/* L014 */
	writeerr();
    stackp = de_stack;

    while ( (code = getcode()) > -1 ) {

	if ( (code == CLEAR) && block_compress ) {
	    for ( code = 255; code >= 0; code-- )
		tab_prefixof(code) = 0;
	    clear_flg = 1;
	    free_ent = FIRST - 1;
	    if ( (code = getcode ()) == -1 )	/* O, untimely death! */
		break;
	}
	incode = code;
	/*
	 * Special case for KwKwK string.
	 */
	if ( code >= free_ent ) {
	    *stackp++ = finchar;
	    code = oldcode;
	}

	/*
	 * Generate output characters in reverse order
	 */
#ifdef SIGNED_COMPARE_SLOW
	while ( ((unsigned long)code) >= ((unsigned long)256) ) {
#else
	while ( code >= 256 ) {
#endif
	    *stackp++ = tab_suffixof(code);
	    code = tab_prefixof(code);
	}
	*stackp++ = finchar = tab_suffixof(code);

	/*
	 * And put them out in forward order
	 */
	do
	    putc ( *--stackp, foutput );			/* L014 */
	while ( stackp > de_stack );

	/*
	 * Generate the new entry.
	 */
	if ( (code=free_ent) < maxmaxcode ) {
	    tab_prefixof(code) = (unsigned short)oldcode;
	    tab_suffixof(code) = finchar;
	    free_ent = code+1;
	} 
	/*
	 * Remember previous code.
	 */
	oldcode = incode;
    }
    fflush( foutput );						/* L014 */
    if(ferror(foutput))						/* L014 */
	writeerr();
}

/*****************************************************************
 * TAG( getcode )
 *
 * Read one code from the standard input.  If EOF, return -1.
 * Inputs:
 *	finput
 * Outputs:
 *	code or -1 is returned.
 */

code_int
getcode() {
    /*
     * On the VAX, it is important to have the register declarations
     * in exactly the order given, or the asm will break.
     */
    register code_int code;
    static int offset = 0, size = 0;
    static char_type buf[BITS];
    register int r_off, bits;
    register char_type *bp = buf;

    if ( clear_flg > 0 || offset >= size || free_ent > maxcode ) {
	/*
	 * If the next entry will be too big for the current code
	 * size, then we must increase the size.  This implies reading
	 * a new buffer full, too.
	 */
	if ( free_ent > maxcode ) {
	    n_bits++;
	    if ( n_bits == maxbits )
		maxcode = maxmaxcode;	/* won't get any bigger now */
	    else
		maxcode = MAXCODE(n_bits);
	}
	if ( clear_flg > 0) {
	    maxcode = MAXCODE (n_bits = INIT_BITS);
	    clear_flg = 0;
	}
	size = fread( buf, 1, n_bits, finput );
	if ( size <= 0 )
	    return -1;			/* end of file */
	offset = 0;
	/* Round size down to integral number of codes */
	size = (size << 3) - (n_bits - 1);
    }
    r_off = offset;
    bits = n_bits;
#ifdef vax
    asm( "extzv	  r10,r9,(r8),r11" );
#else /* not a vax */
	/*
	 * Get to the first byte.
	 */
	bp += (r_off >> 3);
	r_off &= 7;
	/* Get first part (low order bits) */
#ifdef NO_UCHAR
	code = ((*bp++ >> r_off) & rmask[8 - r_off]) & 0xff;
#else
	code = (*bp++ >> r_off);
#endif /* NO_UCHAR */
	bits -= (8 - r_off);
	r_off = 8 - r_off;		/* now, offset into code word */
	/* Get any 8 bit parts in the middle (<=1 for up to 16 bits). */
	if ( bits >= 8 ) {
#ifdef NO_UCHAR
	    code |= (*bp++ & 0xff) << r_off;
#else
	    code |= *bp++ << r_off;
#endif /* NO_UCHAR */
	    r_off += 8;
	    bits -= 8;
	}
	/* high order bits. */
	code |= (*bp & rmask[bits]) << r_off;
#endif /* vax */
    offset += n_bits;

    return code;
}

#ifdef DEBUG
printcodes()
{
    /*
     * Just print out codes from input file.  For debugging.
     */
    code_int code;
    int col = 0, bits;

    bits = n_bits = INIT_BITS;
    maxcode = MAXCODE(n_bits);
    free_ent = ((block_compress) ? FIRST : 256 );
    while ( ( code = getcode() ) >= 0 ) {
	if ( (code == CLEAR) && block_compress ) {
	    free_ent = FIRST - 1;
	    clear_flg = 1;
	}
	else if ( free_ent < maxmaxcode )
	    free_ent++;
	if ( bits != n_bits ) {
	    fprintf(stderr, "\nChange to %d bits\n", n_bits );
	    bits = n_bits;
	    col = 0;
	}
	fprintf(stderr, "%5d%c", code, (col+=6) >= 74 ? (col = 0, '\n') : ' ' );
    }
    putc( '\n', stderr );
    exit( 0 );
}

code_int sorttab[1<<BITS];	/* sorted pointers into htab */

dump_tab()	/* dump string table */
{
    register int i, first;
    register ent;
#define STACK_SIZE	15000
    int stack_top = STACK_SIZE;
    register c;

    if(do_decomp == 0) {	/* compressing */
	register int flag = 1;

	for(i=0; i<hsize; i++) {	/* build sort pointers */
		if((long)htabof(i) >= 0) {
			sorttab[codetabof(i)] = i;
		}
	}
	first = block_compress ? FIRST : 256;
	for(i = first; i < free_ent; i++) {
		fprintf(stderr, "%5d: \"", i);
		de_stack[--stack_top] = '\n';
		de_stack[--stack_top] = '"';
		stack_top = in_stack((htabof(sorttab[i])>>maxbits)&0xff, 
				     stack_top);
		for(ent=htabof(sorttab[i]) & ((1<<maxbits)-1);
		    ent > 256;
		    ent=htabof(sorttab[ent]) & ((1<<maxbits)-1)) {
			stack_top = in_stack(htabof(sorttab[ent]) >> maxbits,
						stack_top);
		}
		stack_top = in_stack(ent, stack_top);
		fwrite( &de_stack[stack_top], 1, STACK_SIZE-stack_top, stderr);
		stack_top = STACK_SIZE;
	}
   } else if(!debug) {	/* decompressing */

       for ( i = 0; i < free_ent; i++ ) {
	   ent = i;
	   c = tab_suffixof(ent);
	   if ( isascii(c) && isprint(c) )
	       fprintf( stderr, "%5d: %5d/'%c'  \"",
			   ent, tab_prefixof(ent), c );
	   else
	       fprintf( stderr, "%5d: %5d/\\%03o \"",
			   ent, tab_prefixof(ent), c );
	   de_stack[--stack_top] = '\n';
	   de_stack[--stack_top] = '"';
	   for ( ; ent != NULL;
		   ent = (ent >= FIRST ? tab_prefixof(ent) : NULL) ) {
	       stack_top = in_stack(tab_suffixof(ent), stack_top);
	   }
	   fwrite( &de_stack[stack_top], 1, STACK_SIZE - stack_top, stderr );
	   stack_top = STACK_SIZE;
       }
    }
}

int
in_stack(c, stack_top)
	register c, stack_top;
{
	if ( (isascii(c) && isprint(c) && c != '\\') || c == ' ' ) {
	    de_stack[--stack_top] = c;
	} else {
	    switch( c ) {
	    case '\n': de_stack[--stack_top] = 'n'; break;
	    case '\t': de_stack[--stack_top] = 't'; break;
	    case '\b': de_stack[--stack_top] = 'b'; break;
	    case '\f': de_stack[--stack_top] = 'f'; break;
	    case '\r': de_stack[--stack_top] = 'r'; break;
	    case '\\': de_stack[--stack_top] = '\\'; break;
	    default:
		de_stack[--stack_top] = '0' + c % 8;
		de_stack[--stack_top] = '0' + (c / 8) % 8;
		de_stack[--stack_top] = '0' + c / 64;
		break;
	    }
	    de_stack[--stack_top] = '\\';
	}
	return stack_top;
}
#endif /* DEBUG */

writeerr()
{
    psyserrorl (errno, MSGSTR(COMPRESS_WRITEERR, "error writing %s"),
		ofname);					/* L013 */
    unlink ( ofname );
    exit ( 1 );
}

copystat(ifname, ofname)
char *ifname, *ofname;
{
    struct stat statbuf;
    mode_t mode;						/* L011 */
    time_t timep[2];

    if (foutput != stdout) fclose(foutput);			/* L014 */
    foutput = stdout;						/* L014 */
    if (stat(ifname, &statbuf)) {		/* Get stat on input file */
	psyserrorl(errno, MSGSTR(COMPRESS_NOSTAT, "could not stat %s"),
		   ifname);					/* L013 */
	return;
    }
    if ((statbuf.st_mode & S_IFMT/*0170000*/) != S_IFREG/*0100000*/) {
	if(quiet)
		fprintf(stderr, "%s: %s: ",command_name, ifname); /* L012 */
		fprintf(stderr, MSGSTR(COMPRESS_NOT_REG,
		" -- not a regular file: unchanged"));		/* L012 */
		exit_stat = 1;
		perm_stat = 1;
    } else if (statbuf.st_nlink > 1) {
	if(quiet)
		fprintf(stderr, "%s: %s: ",command_name, ifname); /* L012 */
		fprintf(stderr, MSGSTR(COMPRESS_HAS_LINKS,
		" -- has %d other links: unchanged"),
		statbuf.st_nlink - 1);				/* L012 */
		exit_stat = 1;
		perm_stat = 1;
    } else if (exit_stat == 2 && (!force)) { /* No compression: remove file.Z */
	if(!quiet)
		fprintf(stderr, MSGSTR(COMPRESS_FILUNCHG,
		" -- file unchanged"));				/* L012 */
    } else {			/* ***** Successful Compression ***** */
	exit_stat = 0;
	mode = statbuf.st_mode & 07777;
	if (chmod(ofname, mode))		/* Copy modes */
	    psyserrorl(errno, MSGSTR(COMPRESS_NOCHMOD, "could not chmod %s"),
		       ofname);					/* L013 */
	chown(ofname, statbuf.st_uid, statbuf.st_gid);	/* Copy ownership */
	timep[0] = statbuf.st_atime;
	timep[1] = statbuf.st_mtime;
	utime(ofname, timep);	/* Update last accessed and modified times */
	if (unlink(ifname))	/* Remove input file */
	    psyserrorl(errno, MSGSTR(COMPRESS_NOUNLINK, "could not unlink %s"),
		       ifname);					/* L013 */
	if(!quiet)
		fprintf(stderr, MSGSTR(COMPRESS_REPLACE,
		" -- replaced with %s"), ofname);		/* L012 */
	return;		/* Successful return */
    }

    /* Unsuccessful return -- one of the tests failed */
    if (unlink(ofname))
	psyserrorl(errno, MSGSTR(COMPRESS_NOUNLINK, "could not unlink %s"),
		   ofname);					/* L013 */
}

static void
onintr (int sig)
{
    if (!precious)
	unlink ( ofname );
    if (Pflag) unlink ( ctmp );		/* S002 */
    exit ( 1 );
}

static void
oops (int sig)	/* wild pointer -- assume bad input */
{
    if ( do_decomp ) 
	errorl(MSGSTR(COMPRESS_CORRUPT, "corrupt input"));	/* L012 */
    unlink ( ofname );
    if (Pflag) unlink ( ctmp );		/* S002 */
    exit ( 1 );
}


cl_block ()		/* table clear for block compress */
{
    register long int rat;

    checkpoint = in_count + CHECK_GAP;
#ifdef DEBUG
	if ( debug ) {
		fprintf ( stderr, "count: %ld, ratio: ", in_count );
		prratio ( stderr, in_count, bytes_out );
		fprintf ( stderr, "\n");
	}
#endif /* DEBUG */

    if(in_count > 0x007fffff) { /* shift will overflow */
	rat = bytes_out >> 8;
	if(rat == 0) {		/* Don't divide by zero */
	    rat = 0x7fffffff;
	} else {
	    rat = in_count / rat;
	}
    } else {
	rat = (in_count << 8) / bytes_out;	/* 8 fractional bits */
    }
    if ( rat > ratio ) {
	ratio = rat;
    } else {
	ratio = 0;
#ifdef DEBUG
	if(verbose)
		dump_tab();	/* dump string table */
#endif
	cl_hash ( (count_int) hsize );
	free_ent = FIRST;
	clear_flg = 1;
	output ( (code_int) CLEAR );
#ifdef DEBUG
	if(debug)
		fprintf ( stderr, "clear\n" );
#endif /* DEBUG */
    }
}

cl_hash(hsize)		/* reset code table */
	register count_int hsize;
{
#ifndef XENIX_16	/* Normal machine */
	register count_int *htab_p = htab+hsize;
#else
	register j;
	register long k = hsize;
	register count_int *htab_p;
#endif
	register long i;
	register long m1 = -1;

#ifdef XENIX_16
    for(j=0; j<=8 && k>=0; j++,k-=8192) {
	i = 8192;
	if(k < 8192) {
		i = k;
	}
	htab_p = &(htab[j][i]);
	i -= 16;
	if(i > 0) {
#else
	i = hsize - 16;
#endif
	do {				/* might use Sys V memset(3) here */
		*(htab_p-16) = m1;
		*(htab_p-15) = m1;
		*(htab_p-14) = m1;
		*(htab_p-13) = m1;
		*(htab_p-12) = m1;
		*(htab_p-11) = m1;
		*(htab_p-10) = m1;
		*(htab_p-9) = m1;
		*(htab_p-8) = m1;
		*(htab_p-7) = m1;
		*(htab_p-6) = m1;
		*(htab_p-5) = m1;
		*(htab_p-4) = m1;
		*(htab_p-3) = m1;
		*(htab_p-2) = m1;
		*(htab_p-1) = m1;
		htab_p -= 16;
	} while ((i -= 16) >= 0);
#ifdef XENIX_16
	}
    }
#endif
	for ( i += 16; i > 0; i-- )
		*--htab_p = m1;
}

prratio(stream, num, den)
FILE *stream;
long int num, den;
{
	register int q;			/* Doesn't need to be long */

	if(num > 214748L) {		/* 2147483647/10000 */
		q = num / (den / 10000L);
	} else {
		q = 10000L * num / den;		/* Long calculations, though */
	}
	if (q < 0) {
		putc('-', stream);
		q = -q;
	}
	fprintf(stream, "%d.%02d%%", q / 100, q % 100);
}

/* S002 Begin								*/
/*      Read file names from the pipe and compress/decompress them   	*/
/*    	overwriting the original file. 					*/
/* Note: <stdio.h> defined BUFSIZ chunks are read from the pipe.      	*/
/*       Programs using the -P option should write in BUFSIZ chunks.  	*/

do_Pflag(lzhAlgorithm)						/* S008 */
int lzhAlgorithm;						/* S008 */
{
    	char namebuf[BUFSIZ];   /* Buffer for fnames passed through pipe.  */
	time_t timep[2];
	char lng_ctmp[PATHSIZE], *local_dirname();	/* L004 S003 L020 */
    	struct stat statbuf;

			/* A NULL, EOF or broken pipe will break loop. 	*/
    	while (read(rpipe, namebuf, sizeof(namebuf)) > 0) { 
	    if(namebuf[0] == '\0') break;
	    /* Open input file */
	    if (finput != stdin) fclose (finput);		/* L014 */
	    if ((finput = fopen(namebuf, "r")) == NULL) {	/* L014 */
	        psyserrorl(errno, MSGSTR(COMPRESS_NOREAD, "could not read %s"),
			   namebuf);				/* L013 */
	        continue;
	    }
	    if (stat( namebuf, &statbuf ) < 0) {
		psyserrorl(errno, MSGSTR(COMPRESS_NOSTAT, "could not stat %s"),
			   namebuf);				/* L013 */
		continue;
	    }

	    if (do_decomp) {		/* DECOMPRESSION */
					/* Check the magic number */
		if (nomagic == 0) {
                    lzhAlgorithm = CheckMagic ();		/* S008 */
                    if (lzhAlgorithm < 0) {			/* S008 */
			/* File not in compressed format. No problem. */
		    	continue;
		    }
                    if (!lzhAlgorithm) {			/* S008 */
			/* set -b from file */
                        maxbits = getc (finput);		/* L014 */
                        block_compress = maxbits & BLOCK_MASK;
                        maxbits &= BIT_MASK;
                        maxmaxcode = 1 << maxbits;
                        if(maxbits > BITS) {
                           errorl(MSGSTR(COMPRESS_MAX_BITS,	/* L012 */
                           "%s: compressed with %d bits, can only handle %d bits"),
                           namebuf, maxbits, BITS);
                           continue;
                        }
                    }
		}
	    } 
	    else {				/* COMPRESSION */
		fsize = (long) statbuf.st_size;
		/*
		 * tune hash table size for small files -- as below 
		 */
		hsize = HSIZE;
		if ( fsize < (1 << 12) )
		    hsize = min ( 5003, HSIZE );
		else if ( fsize < (1 << 13) )
		    hsize = min ( 9001, HSIZE );
		else if ( fsize < (1 << 14) )
		    hsize = min ( 18013, HSIZE );
		else if ( fsize < (1 << 15) )
		    hsize = min ( 35023, HSIZE );
		else if ( fsize < 47000 )
		    hsize = min ( 50021, HSIZE );
	   }
					/* S003 Add full path to ctmp.  */
	   sprintf(lng_ctmp, "%s%s", local_dirname(namebuf), ctmp); /* L020 */
	   				/* Set output filename to ctmp. */
	   if (foutput != stdout) fclose (foutput);		/* L014 */
	   if ((foutput = fopen(lng_ctmp, "w")) == NULL) {	/* L014 */
	       psyserrorl(errno, MSGSTR(COMPRESS_NOWRITE, "could not write %s"),
			  ctmp);				/* L013 */
	       continue;
	   }
           /* Do actual compress/decompress. */
           if (lzhAlgorithm) {					/* S008 begin */
               if (do_decomp)
                   lzh_decompress ();
               else {
                   lzh_compress ();
               }
           } else {						/* S008 end */
               if (do_decomp) decompress();
               else           compress();
           }
	   fflush(foutput);					/* L014 */
	   if (unlink(namebuf))		/* Remove input file. */
		psyserrorl(errno, MSGSTR(COMPRESS_NOUNLINK,
			 "could not unlink %s"), namebuf);	/* L013 */
					/* Link tmp file to input filename. */
	   if( (link(lng_ctmp, namebuf)) < 0) {   
		psyserrorl(errno, MSGSTR(COMPRESS_NOLNK,
			"cannot link tmp to %s"), namebuf);	/* L012, L013 */
	   }
	   unlink(lng_ctmp);		/* Always unlink tmp file. */

	   /* Set time, mode and ownership for new file. */
	   timep[0] = time((long *) 0);
	   timep[1] = statbuf.st_mtime;
	   utime(namebuf, timep);
	   chmod(namebuf, statbuf.st_mode);
	   chown(namebuf, statbuf.st_uid, statbuf.st_gid);
	}
	unlink(lng_ctmp);			/* Make sure tmp is gone. */
	exit(0);
}					/* S002 end */


/* S003 Begin								*/
/*      Return the directory portion of lname.				*/
/*									*/
char *
local_dirname(lname)						/* L020 */
char *lname;
{
	char *cp, *end;
	static char buf[PATHSIZE];			/* L004 */

	if (lname[0] == '/' || lname[1] == '/') strcpy(buf, lname);
	else {
		strcpy(buf, "./");
		strcat(buf, lname);
	}
	end = &buf[strlen(buf)];
	for(cp = end; *cp != '/'; cp--); 
	*(cp+1)='\0';
	return(buf);
}			/* End S003 */

version()
{
	fprintf(stderr, "%s, Berkeley 5.9 5/11/86\n", rcs_ident);
	fprintf(stderr, MSGSTR(COMPRESS_OPTIONS, "Options: "));
								/* L012 */
#ifdef vax
	fprintf(stderr, "vax, ");
#endif
#ifdef NO_UCHAR
	fprintf(stderr, "NO_UCHAR, ");
#endif
#ifdef SIGNED_COMPARE_SLOW
	fprintf(stderr, "SIGNED_COMPARE_SLOW, ");
#endif
#ifdef XENIX_16
	fprintf(stderr, "XENIX_16, ");
#endif
#ifdef COMPATIBLE
	fprintf(stderr, "COMPATIBLE, ");
#endif
#ifdef DEBUG
	fprintf(stderr, "DEBUG, ");
#endif
#ifdef BSD4_2
	fprintf(stderr, "BSD4_2, ");
#endif
	fprintf(stderr, "BITS = %d\n", BITS);
}
