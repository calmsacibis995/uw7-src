#ident	"@(#)OSRcmds:cpio/cpio.c	1.1"
#pragma comment(exestr, "@(#) cpio.c 25.27 95/04/05 ")
/*
 *   Portions Copyright 1983-1995 The Santa Cruz Operation, Inc
 *		      All Rights Reserved
 */
/*	Copyright (c) 1984, 1986, 1987, 1988 AT&T	*/
/*	  All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)cpio:cpio.c	1.10" */

/*	MODIFICATION HISTORY
 *
 *	24/1/89	sco!kai	S000
 *	- use mkdir system call instead of fork/exec /bin/mkdir
 *	  allow retries after bad I/O just after changing volumes
 *	1/2/89 sco!kai S001
 *	- fix cpio so it can read empty archives, e.g. trailer only (since it 
 *	produces them)
 *	14/2/89		sco!hoeshuen	S002
 *	- replace gets() with fgets() and delete newline character
 *	5 June 89	sco!hoeshuen	S003
 *	- replace cftime() with localtime() and strftime().
 *	18 Jul 89	sco!cliff	S004
 *	- add -K Mediasize option; forces Bufsize to be a 1k multiple.
 *	  only relevant to media writes; K is not needed when reading.
 *	14/6/89		scol!wooch	S005
 *	- Internationalization Phase I 
 *	26 Sept 89	sco!kenl	S006
 *	- added -T flag to truncate long filenames to 14 characters.
 *	  New function: truncate(); use with [-i] option   
 *	8 Jan 1990	scol!blf	S007, SCCS not used
 *	- Check for EOF in skipln(); see S002, above.
 *	- Use skipln() whenever fgets() is used, changing all calls
 *	  to fgets() to deal with EOF and skipln()s newline deletion.
 *	- Rewrite pwd() to use getcwd(S), which has better error handling.
 *	- Changed an inappropriate access(S) to the proper eaccess(S).
 *	- Immediately flush each bit of verbose (-v or -V) output.
 *	- Make sure fperrno() uses the correct errno, not a spurious one,
 *	  and use fperrno() rather than fperr()/perror() or sys_errlist[].
 *	- Use ANSI C function prototypes in `extern' declarations, and get
 *	  rid of spurious declarations.
 *	- NOTE: There are an awful lot of strcpy()s et.al. in this mess,
 *		and so it is probably very easy to cause a core dump by
 *		feeding cpio "too long" filenames.  Also, since cpio's
 *		maximum pathname length is less than that of the SVr3.2
 *		kernel (viz POSIX_PATH_MAX), cpio is incapable of backing
 *		up sufficiently pathological filesystems.
 *	31 Aug 1990	sco!edb		S008
 *	- Driver does not handle end of media properly, we have to abort
 *	- and notify user to use -K
 *	10 July 1990	scol!declann	L009
 *	- XPG3 and POSIX require that if not privileged user, then the file
 *	  permissions on newly-created files must be modified by the current
 *	  umask value.
 *	16jan91		scol!hughd	L010
 *	- I cannot tolerate that "find . -type f -print | cpio -pdm whatever"
 *	  creates directories 0777 without regard to the user's umask,
 *	  I had imagined L009 was fixing that but not so
 *	- research shows that SID 1.1 from AT&T did not behave in that way,
 *	  but for SID 1.2 xen made Changes for SCO/Microsoft EP release,
 *	  no explanation but seems to have been connected with using /bin/mkdir
 *	20 Jan 1991	sco!hishami	S009
 *	-Fixed a spelling mistake in an error message. (passed --> past)
 *	3 January	scol!markhe	L011
 *	- Adapted to work with symbolic links:
 *	- added 'L' option to indicate that symbolic links are 
 *	  to be followed (not followed by default)
 *	- added code to allow symbolic links to be copied out/in
 *	  to/from archives, and passed across directories.
 *	- output from 't' and 'tv' enchanced to show symbolic links
 *	25feb91		scol!hughd	L012
 *	- the original -r1.1 code I reinserted for mkdir in L010,
 *	  had the return code from mkdir wrong in one instance:
 *	  as a result it sometimes failed to make directories,
 *	  then reported an error on the chmod
 *	25feb91		scol!hughd	L013
 *	- replace lstat by statlstat so executable runs on versions
 *	  of the OS both with and without the lstat system call
 *	5 Dec 1990	scol!dipakg + scol!markhe	L014
 *	- enhancement to S006, -T option changed to truncate all components
 *	  of a pathname instead of just the filename.
 *	19 June 91	scol!chrisu	L015
 *	- added -A flag with similar function to tar -A (restores files
 *	  with any leading '/' removed from the names)
 *	- removed problem where initial "./" is stripped from all filenames
 *	  resulting in `Cannot obtain information about file: ""' warnings
 *	- no longer avoid restoring permission on "." if the -u flag is given
 *	  (allows backupsh to word properly)
 *	27 August 91	scol!markhe	L016
 *	- added -D flag.  It stops cpio from reading ahead, allowing oam
 *	  packages to be read ( an oam package consists of appended cpio
 *	  archives).  Without this flag, `out of sync' errors occur.
 *	- trap long file names correctly
 *	- do not try to link symbolic links when `passing' with the 'l'
 *	  flag.  link() tries to follow symlinks, producing undesirable
 *	  results.
 *	18 Sept 91	scol!ashleyb	L017
 *	- Corrected cpio handling of read errors. Now, instead of corrupting
 *	  the archive to the extent that no files written out after the read
 *	  error occurred could be recovered, only the erroneous file is
 *	  corrupted and remaining files can be recovered normally.
 *	20 Sept 91	scol!ashleyb	L018
 *	- Corrected calculation of number of blocks copied. Now works correctly
 *	  for buffer sizes that are not multiples of 512 bytes.
 *	23 Oct 91	scol!ashleyb	L019
 *	- Reset poss_eom variable in bread() when driver returns ENXIO or
 *	  ENOSPC
 *	27 Mar 92	scol!ashleyb	L020
 *	- If driver returns ENXIO in bwrite(), then ask for a new media.
 *	- If driver buffers data, and supports the MT_LOCK and MT_UNLOCK
 *        ioctl's, then use these when we change media. Therefore buffer
 *        data will not be discarded when the next media volume is used.
 *	- Include <sys/tape.h> where new ioctl's are defined.
 *	13 Aug 92	scol!jamesle	L021
 *	- Fix bug with the -T (truncate long filenames) option, long
 *	  filenames are truncated correctly to 14 characters but filenames
 *	  shorter than 14 characters were corrupted (from scol!andyr).
 *	17 Aug 92	scol!ianw	L022
 *	- Fix 3.2v5 DevSys warnings, changed the type of orig_umask to mode_t.
 *	28 Aug 92	scol!ashleyb	L023
 *	- Increased the maximum pathsize to 1024 characters, but note
 *	  warning in S007.
 *	- Removed functionality of L020 because the code in the SCSI driver
 *	  proved unreliable at present.
 *	09 Sep 92	scol!ashleyb	L024
 *	- Put back functionality of L020.
 *	18 Nov 92	scol!ashleyb	L025
 *	- Added checksum information to TRAILER!!! record. Algorithm copied
 *	  from sum -r. Use Hdr.h_mtime to hold a special value if a checksum
 *	  has been written, and then Hdr.h_ino to hold this checksum.
 *	  Notes:
 *		We only checksum filedata, not the header records. We stuff
 *		the checksum in weird fields in the TRAILER to stop other
 *		utilities that read cpio archives worrying about it.
 *	24 Nov 92	scol!ashleyb	L026
 *	- Fixed bug with symbolic links and PASS option.
 *	- Cleaned up format of an error message.
 *	- Updated usage message. (Added -Kvolumesize to cpio -o and removed
 *	  -r flag from pass option.
 *	- Updated bintochar to use information in Hdr structure rather than
 *	  Sstatb structure. Now Trailer character records are written out
 *	  correctly. Fixes bug LTD-8-637.
 *	11 Jan 93	scol!ashleyb	L027
 *	- Reset Mediaused variable after every call to eomchgreel.
 *	15 Jan 93	scol!ashleyb	L028
 *	- I broke bintochar for NFS mounted files. The dev, ino and rdev
 *	  fields in the stat structure should all still have MK_USHORT
 *	  wrapped around them.
 *	15 Jun 93	scol!ashleyb	L029
 *	- Modified to take a list of devices for input or output.
 *	  When using SCSI devices, the media size does not need to be
 *	  given, thus allowing unattended backups on multiple
 *	  optionally compressing devices.
 *	- Added an -n key. If this is specified, cpio will generate sum -r
 *	  checksums for each file read during a cpio -itvn operation. Only
 *	  files will have valid checksums, not directories and other file
 *	  type. I had to modify the checksum routine slightly, so I have
 *	  changes the USED_CHECKSUM magic number (unmarked).
 *	28 Jul 93	scol!ashleyb	L030
 *	- Define _INO_16_T to let cpio work in a 32bit inode environment.
 *	  (Only a temporary fix until code is modified to work properly.)
 *	10 Aug 93	sco!evanh	S031
 *	- Added a -q key for quick extraction.  If this is specified,
 *	  cpio will exit immediately after as many files as were specified
 *	  on the command line have been extracted.  Note that filename
 *	  globbing WILL NOT work in this case.
 *	28 Sep 93	scol!ashleyb	L032
 *	- Restructured the code, and broke it into a number of distinct
 *	  source and header files. Try to use less global variables.
 *	  (these mods are not marked.)
 *	- Added support to read and write SVR4 extended cpio archives with
 *	  and without checksums. It should be noted that we do not handle
 *	  hard links in exactly the same way as they do. It is possible using
 *	  an SVR4 cpio to write an archive that cannot be recovered. See
 *	  comments in code.
 *	- Modified pentry() so that the size of files 1Meg and over do
 *	  not run into the username when being printed.
 *	- Removed the checksum from normal archives. Users can now use
 *	  the crc format.
 *	25 Oct 93	scol!ashleyb	L033
 *	- Added -Pinput_fd,output_fd support. This stops cpio using /dev/tty
 *	  during end-of-media operations. The input_fd and output_fd should
 *	  have been opened in cpio's parent and remained open across the
 *	  exec call.
 *	17jan94		scol!hughd	L034
 *	- for efficiency in raw disk transfers, align buffer on sector boundary
 *	01 Feb 94	scol!ashleyb	L035
 *	- Don't output "newer than" message for directories, during PASS
 *	  operation.
 *	10 Mar 94	scol!ianw	L036
 *	- Use zstat32 now we are building with 32 bit inodes. If the -L option
 *	  was specified zstat was called causing cpio to create files with
 *	  incorrect access permissions and sometimes the wrong file type.
 *	25th Apr 94	scol!ashleyb	L037
 *	- Pattern matching was not working under -itv operations -- fixed.
 *	08 June 1994	scol!ashleyb	L038
 *	- If argv[0] = "find -cpio" assume that we have been exec'd from
 *	  find, and expect a series of NULL terminated filenames, rather
 *	  than newline terminate ones.
 *	- Changed the name of the truncate() function to avoid clash with
 *	  libsocket.
 *	05 July 1994	scol!ianw	L039
 *	- Fixed a compiler warning.
 *	06 July 1994	scol!ashleyb	L040
 *	- Added S_IFNAM to special cases in cpio.
 *	- Changed the wording of an error message.
 *	- Use errorl and psyserrorl to report errors (unmarked).
 *	- Respect umask when opening output file.
 *	- Calculate block usage correctly.
 *	09 Aug 1994	scol!ashleyb	L041
 *	- L035, Don't call cktime for directories at all!
 *	07 Sep 1994	scol!ashleyb	L043
 *	- By popular demand, make cpio -o and cpio -oc produce traditional
 *	  format archives, but auto generate inode/device pairs to allow
 *	  us to store >16 bit inodes without truncating them.
 *	- Switch defaults back to traditional mode. Use same header
 *	  specifiers as GNU cpio.
 *	08 Nov 1994	scol!trevorh	L044
 *	 - message catalogued.
 *	24th Nov 1994	scol!ashleyb	L045
 *	- Fixed a null dereference, seen when cpio is run without any args.
 *	 3rd Apr 1995	scol!ashleyb	L046
 *	- Suppress the HELP_KEY facility and To Fix lines from the libgen's
 *	  zchmod, zchown etc. functions.
 *	- Corrected an error message (unmarked).
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>					/* L034 */
#include <sys/sysmacros.h>
#include <sys/mkdev.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>					/* S003 SCO_INTL */

/* #include <errormsg.h> */						/* L044 Start */
#ifdef INTL
#  include <locale.h>
#  include "cpio_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str)	(str)
#  define catclose(d)		/*void*/
#endif /* INTL */						/* L044 Stop */

#include "../include/osr.h"
#define _CPIO_C
#include "cpio.h"
#include "errmsg.h"

/* Internal Function Declarations. */
static int	getname(void);
static int	gethdr(void);
static int	openout(char *namep);
static int	postml(char *namep, char *np);
static void	synch(void);
static int	cktime(char *filename, ulong mtime);

static struct	stat	Statb, Astatb;
struct stat	Xstatb;

buf_info buf;
header Hdr;

/* vvv S004 vvv */
unsigned	Mediasize = 0;		/* default: ignore media size */
unsigned	Mediaused = 0;		/* accumulated media use, kbytes */
/* ^^^ S004 ^^^ */

unsigned	Bufsize = BUFSIZE;		/* default record size */
char	*Buf;

char *command_name = "cpio";					/* L040 */

short	Cflag,
	Dir,
	Rename,
	oamflag,
	Mod_time,
	Verbose,
	Option,
	Aflag,		/* L015 */
	fflag,
	kflag,
	cpio_from_find;						/* L038 */

static
short	SymLink,	/* set under 'L' flag */		/* L011 */
	Uncond,
	PassLink,
	Toc,
	Acc_time,
	Tflag;		/* S006 */   

int	newreel = 0;
int	flushbuf = 0;	/* write buffer out */
int	fillbuf = 0;	/* fill buffer */
int	Input = 0;
int	buf_error = 0;	/* indicates an I/O occurred during buffer fill */
int	error_cnt = 0;	/* tracks number of I/O errors that have occurred */
int	Output = 1;

static
int	buf_cnt,	/* number of Bufsize blocks allocated to buffer */
	Ifile,
	Ofile;

mode_t	orig_umask;	/* umask inherited from cpio's parent process L022 */

ulong	sBlocks,						/* L040 */
	Blocks;							/* L040 */

long	Longfile,
	Longtime;

char	Fullname[PATHSIZE+1];
char	Name[PATHSIZE+1];
static int	Pathend;
char	*swfile;
char	*eommsg;						/* L044 */

FILE	*Rtty,
	*Wtty;

char	*ttydev = "/dev/tty";

char	**Pattern = 0;

/* Maximum size of a header is 6+8*13 (CHARS) + 1024 (PATHSIZE) + 1 (NULL)
 *  = 1135. But round up to next long = 1136 bytes.
 */
char	Chdr[CHARS+PATHSIZE+2];

static short	Dev;
ushort	Uid,
	A_directory,
	A_special,
	A_symlink,	/* flag indicating file is symbolic link *//* L011 */
	Filetype = S_IFMT;

DevPtr		devicelist = (DevPtr)NULL;
DevPtr		nextdev = (DevPtr)NULL;

main(int argc, char **argv)
{
	register ct;
	register char *fullp;
	register i;
	long	filesz, maxsz;
	int ans,  rc, first;
	short nflag = 0;
	short select;			/* set when files are selected */
	short Coption = 0;		/* S004 */
	short swapopt = 0;
	short qflag = 0;		/* S031 */
	short Pflag = 0;		/* L033 */
	int in_fd, out_fd;
	int fcnt = 0, xcnt = 0;		/* S031 */
	ulong bufalign;			/* L034 */
	extern char	*optarg;
	extern int	optind;

#ifdef INTL							/* L044 Start */
        setlocale(LC_ALL,"");
        catd = catopen(MF_CPIO,MC_FLAGS);
#endif /* INTL */						/* L044 Stop */

								/* L046 Begin */
	/* errsource(command_name); */  /* Tag libgen errors with the command name. */
	/* errverb("notag,notofix"); */		/* Turn off extra help info. */
	/* errexit(2); */		      /* Set exit on error value to 2 */
								/* L046 End */

	eommsg = MSGSTR(CPIO_MSG_CHANGE, "Change to part %d and press RETURN key. [q] ");

	signal(SIGSYS, SIG_IGN);	/* S007 */

	/* Flag to see if we have been executed from find <dir> -cpio. */
	if (!strcmp(argv[0],"find -cpio"))		/* L038 */
		cpio_from_find = 1;			/* L038 */

	if ((argc < 2) || (*argv[1] != '-'))			/* L045 */ 
		usage();

	Uid = getuid();
	orig_umask = umask(0);		/* L009 L010 */

/* vvv S004 vvv */
	while( (ans = getopt( argc, argv,			/* L039 */
		"aABK:C:ifopcdH:kLlmnP:qrSsbtTuvVM:6eI:O:D")) != EOF ) {  /* L011, L015 */	/* L016 */ /* L029 */ /* S031 */ /* L033 */
/* ^^^ S004 ^^^ */

		switch( ans ) {
		case 'a':		/* reset access time */
			Acc_time++;
			break;
		case 'A':		/* strip leading '/' -- L015 */
			Aflag++;
			break;
		case 'B':		/* change record size to 5120 bytes */
			Bufsize = 5120;
			break;
/* vvv S004 vvv */
		case 'K':
			Mediasize = atoi( optarg );
			if( Mediasize == 0 )
			{
				errorl(MSGSTR(CPIO_MSG_BAD_ARG, "illegal argument to -%c, '%s'."),
					ans, optarg );
				exit(2);
			}
			break;
/* ^^^ S004 ^^^ */
		case 'C':		/* reset buffer size to arbitrary value */
			Bufsize = atoi( optarg );
			if( Bufsize == 0 )
			{
				errorl(MSGSTR(CPIO_MSG_BAD_ARG, "illegal argument to -%c, '%s'."),
					ans, optarg );
				exit(2);
			}
			Coption++;			/* S004 */
			break;
		case 'i':
			Option = IN;
			break;
		case 'f':	/* copy files not matched by patterns */
			fflag++;
			break;
		case 'o':
			Option = OUT;
			break;
		case 'p':
			Option = PASS;
			break;
		case 'c':		/* Old ASCII header */	/* L043 Begin */
			Cflag = OASCII;
			break;
		case 'H':
			if ((Cflag = set_header_format(optarg)) == -1)
				exit(2);
			break;					/* L043 End */
		case 'd':		/* create directories when needed */
			Dir++;
			break;
		case 'L':					/* L011 { */
			SymLink++;
			break;					/* L011 } */
		case 'l':		/* link files, when necessary */
			PassLink++;
			break;
		case 'm':		/* retain mod time */
			Mod_time++;
			break;
					/* L029 Begin */
		case 'n' :		/* Produce sum -r numbers for files */
			nflag++;
			break;
					/* L029 End */
		case 'P':		/* L033 Begin */
			Pflag++;
			if (check_io(optarg,&in_fd,&out_fd) == 0)
			{
				errorl(MSGSTR(CPIO_MSG_PARSE_FAIL, "failed to parse file descriptor string for -P option."));
				exit(2);
			}
			break;		/* L033 End */
		case 'q':		/* S031 { */
			qflag++;	/* quit after listed files are found */
			break;		/* S031 } */
		case 'r':		/* rename files interactively */
			Rename++;
			Rtty = fopen(ttydev, "r");
			Wtty = fopen(ttydev, "w");
			if(Rtty==NULL || Wtty==NULL) {
				errorl(MSGSTR(CPIO_MSG_RENAME_FAIL, "cannot rename (%s missing)."), ttydev );
				exit(2);
			}
			break;
		case 'S':		/* swap halfwords */
			swapopt |= HALFSWAP;
			break;
		case 's':		/* swap bytes */
			swapopt |= BYTESWAP;
			break;
		case 'b':		/* swap both bytes and halfwords */
			swapopt |= BOTHSWAP;
			break;
		case 't':		/* table of contents */
			Toc++;
			break;
		case 'T':		/* S006  truncate long file names */  
			Tflag++;  
			break;    
		case 'u':		/* copy unconditionally */
			Uncond++;
			break;
		case 'v':		/* verbose - print out file names */
			Verbose = 1;
			break;
		case 'V':		/* print a dot '.' for each file */
			Verbose = 2;
			break;
		case 'M':		/* alternate message for end-of-media */
			eommsg = optarg;
			break;
		case '6':		/* for old, sixth-edition files */
			Filetype = 060000;
			break;
		case 'I':
			chkswfile( swfile, (char)ans, Option );
			close( Input );
								/* L029 Begin */
			devicelist = blddevlist(optarg);
			nextdev = devicelist;

			if( open( nextdev->device, O_RDONLY ) != Input )
				cannotopen( optarg, MSGSTR(CPIO_MSG_INPUT, "input") );
			swfile = nextdev->device;
			nextdev->used = 1;
			nextdev = nextdev->next;
			break;
		case 'O':
			chkswfile( swfile, (char)ans, Option );
			close( Output );

			devicelist = blddevlist(optarg);
			nextdev = devicelist;

			if( open( nextdev->device, O_WRONLY | O_CREAT | O_TRUNC, 0666 & (~orig_umask)) != Output)	/* L040 */
				cannotopen( optarg, MSGSTR(CPIO_MSG_OUTPUT, "output") );
			swfile = nextdev->device;
			nextdev->used = 1;
			nextdev = nextdev->next;
								/* L029 End */
			break;
		case 'k':
			kflag++;
			break;
		case 'D':					/* L016 { */
			oamflag++;
			break;					/* L016 } */
		default:
			usage();
		}
	}

	if (Pflag)			/* L033 Begin */
	{
		if (register_io(in_fd,out_fd) == 0)
		{
			errorl(MSGSTR(CPIO_MSG_FILE_DESC_FAIL, "failed to find open file descriptors specified with -P."));
			cleanup(1);
		}
	}				/* L033 End */

/* vvv S004 vvv */
	if (Mediasize){
		if (Option == IN){
		    Mediasize = 0;
		    errorl(MSGSTR(CPIO_MSG_OPT_KI, "`K' option is irrelevant with the `i' option."));
		}
		if (Option == PASS){
		    Mediasize = 0;
		    errorl(MSGSTR(CPIO_MSG_OPT_KP, "`K' option is irrelevant with the `p' option."));
		}
		if ((Option == OUT) && (Bufsize & 0x3ff)) {
			if (Coption)
			{
				errorl(MSGSTR(CPIO_MSG_OPT_C, "illegal -C %x, must be multiple of 1k."),
					Bufsize);
				exit(2);
			}
			else
			{
				errorl(MSGSTR(CPIO_MSG_OPT_CK, "must use -C option with 1k multiple when using -K option."), Bufsize);
				exit(2);
			}
		}
	}
/* ^^^ S004 ^^^ */

	switch(Option) {
		case IN:
			if ( SymLink)				/* L011 */
				errorl( MSGSTR(CPIO_MSG_OPT_LI, "`L' option is irrelevant with the `-i' option."));							/* L011 */
			break;
		case OUT:
			(void)fstat(Output, &Astatb);
			if (kflag)
				errorl(MSGSTR(CPIO_MSG_OPT_KO, "`k' option is irrelevant with the `-o' option."));
			if (Aflag)				/* L015 */
				errorl(MSGSTR(CPIO_MSG_OPT_AO, "`A' option is irrelevant with the `-o' option."));							/* L015 */
			break;
		case PASS:
			if(Rename)
			{
				errorl(MSGSTR(CPIO_MSG_PASS_RENAME, "pass and rename cannot be used together."));
				exit(2);
			}
			if(kflag)
				errorl(MSGSTR(CPIO_MSG_OPT_KP2, "`k' option is irrelevant with the `-p' option."));
			if (Aflag)				/* L015 */
				errorl(MSGSTR(CPIO_MSG_OPT_AP, "`A' option is irrelevant with the `-p' option."));							/* L015 */
			if( Bufsize != BUFSIZE ) {
				errorl(MSGSTR(CPIO_MSG_OPT_BCP, "`B' or `C' option is irrelevant with the '-p' option."));
				Bufsize = BUFSIZE;
			}
			break;
		default:
			errorl(MSGSTR(CPIO_MSG_OPT_OIP, "options must include one: -o, -i, -p."));
			exit(2);
	}

	Buf = (char *)zmalloc(EERROR, CPIOBSZ);

	if (Option != PASS) {
		bufalign = NBPSCTR;				/* L034 */
		for (buf_cnt = MAX_BUFS; buf_cnt > 1; buf_cnt--) {
			if ((buf.b_base_p = (char *)malloc(
				bufalign + Bufsize*buf_cnt))) {	/* L034 begin */
				if (bufalign) {
					bufalign = (ulong)buf.b_base_p
							& (NBPSCTR-1);
					buf.b_base_p += NBPSCTR - bufalign;
				}				/* L034 end */
				buf.b_out_p = buf.b_base_p;
				buf.b_in_p = buf.b_base_p;
				buf.b_count = 0L;
				buf.b_size = (long)(Bufsize * buf_cnt);
				buf.b_end_p = buf.b_base_p + buf.b_size;
				break;
			}
			bufalign = 0;	/* short of memory */	/* L034 */
		}
		if (buf_cnt < 2) {
			errorl(MSGSTR(CPIO_MSG_NO_MEM, "Not enough memory for buffers."));
			cleanup(1);
		}
	}

	argc -= optind;
	argv += optind;

	switch(Option) {
	case OUT:
		if(argc != 0)
			usage();
		/* get filename, copy header and file out */
		while(getname()) {

			mode_t	ftype;
			
			ftype = Hdr.h_mode & Filetype;
			
			if(!A_special && ident(&Statb, &Astatb))
				continue;
			if(Hdr.h_filesize == 0L) {
				ans = build_header(Cflag,Chdr,Hdr);
				bwrite(Chdr,ans);
				if(Verbose)
					verbdot( stderr, Hdr.h_name);
				continue;
			}
			if(!A_symlink && (Ifile = open(Hdr.h_name, O_RDONLY)) < 0) {								/* L011 */
				psyserrorl(errno,MSGSTR(CPIO_MSG_NO_OPEN, "could not open %s for reading"), Hdr.h_name);
				continue;
			}
								/* L032 Begin */
			if (IS_CRC(Cflag) && (ftype == S_IFREG))
				Hdr.h_chksum = crcsum(Ifile,Hdr.h_filesize);

			ans = build_header(Cflag,Chdr,Hdr);	/* L032 End */

			bwrite(Chdr,ans);
								/* L011 { */
			/* If the file is a symbolic link, read it in and */
			/* then throw it out to the archive.		  */
			if ( A_symlink)
			{
				/* Read symbolic link into Buf, assume	*/
				/* Buf is large enough to hold longest	*/
				/* pathnamne.  It should be, being set	*/
				/* at 4096!				*/
				int SymSize;

				if ( ( SymSize = readlink( Hdr.h_name, Buf, CPIOBSZ-1)) < 0)
				{
					psyserrorl(errno,MSGSTR(CPIO_MSG_NO_READ_SYM, "cannot read symbolic link %s"), Hdr.h_name);
					continue;
				}/*if*/
				bwrite( Buf, SymSize);
				if ( Verbose)
					verbdot( stderr, Hdr.h_name);
				continue;
			}/*if*/					/* L011 } */
			for(filesz=Hdr.h_filesize; filesz>0; filesz-= CPIOBSZ){
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
				errno = 0;
				if(read(Ifile, Buf, (unsigned)ct) < 0) {
								/* L017 { */
				/* If a read error occurs, report it but let a correctly */
				/* sized buffer be written. This ensures that the actual */
				/* filesize is the same as that written in the header    */
				/* information.						 */
					psyserrorl(errno,MSGSTR(CPIO_MSG_NO_READ, "cannot read %s"), Hdr.h_name);
					fprintf(stderr,MSGSTR(CPIO_MSG_FILE_CORRUPT, "This file will be corrupted in archive.\n"));
								/* L017 } */
				}
				bwrite(Buf,ct);
			}
			close(Ifile);
			if(Acc_time)
				utime(Hdr.h_name, &Statb.st_atime);
			if(Verbose)
				verbdot( stderr, Hdr.h_name);
		}

	/* copy trailer, after all files have been copied */
		strcpy(Hdr.h_name, "TRAILER!!!");
		Hdr.h_nlink = 1;				/* S001 */
		Hdr.h_mtime=0;					/* L025 */
		Hdr.h_ino = 0;					/* L025 */
		Hdr.h_filesize=0L;
		Hdr.h_namesize = strlen("TRAILER!!!") + 1;
		ans=build_header(Cflag,Chdr,Hdr);
		bwrite(Chdr,ans);
		bflush();
		break;

	case IN:
		if(argc > 0 ) {	/* save patterns, if any */
			setbuf(stdout, (char *)NULL);
			Pattern = argv;
			fcnt = argc;				/* S031 */
		}
		Cflag = NONE;
		Pathend = pwd(Fullname);
		maxsz = 512 * ulimit(1, 0);			/* S007 */
		while(gethdr()) {  

			mode_t	ftype;				/* L032 Begin */
			short	num_links, name_ok, time_ok;
			ushort	filesum ;
			ulong	crc;

			ftype = Hdr.h_mode & Filetype;

			Ofile=0;
			num_links = 0;
			name_ok = 0;
			time_ok = 0;
			filesum = 0;
			crc = 0;

			select = NORMAL_OP;

			if(qflag && fcnt && (xcnt >= fcnt))	/* S031 { */
				break;				/* S031 } */

			if (Tflag)
				truncate_fn(Hdr.h_name);   	/* L038 */
			if (Aflag)
				deroot(Hdr.h_name);		/* L015 */

			/*
			 * Does this file need processing, check the name and
			 * the time.
			 */

			name_ok = ckname(Hdr.h_name);		/* L037 */
			if (!Toc)
			{

				if (name_ok)
				{
					if (Uncond || A_directory) /* L040 */
						time_ok = 1;
					else
						time_ok = cktime(Hdr.h_name,Hdr.h_mtime);
				}

				/* It is possible that this file is not wanted, but
				 * we are storing a link record that should be created
				 * so we must handle this.
				 */

				if ((ftype == S_IFREG) && (IS_NEW(Cflag)) &&
				   (Hdr.h_nlink > 1)  && (Hdr.h_filesize))
					num_links = linkcount(&Hdr);

				/* Here, we have 3 variables to consider;
				* name_ok, time_ok and num_links.
				*
				* If name_ok and time_ok, then we want to try
				* to create this file.
				*
				* We can safely ignore this record if num_links
				* = 0 and either name_ok or time_ok equal 0;
				*
				* Finally if time_ok or name_ok are 0 but
				* num_links is set, we have to try and find a
				* link that we can create, and substitute this
				* filename for our current filename. Remember
				* that in order for the link to be in our list,
				* it has already gone through the above checks.
				*/

				if (name_ok && time_ok)
					select = NORMAL_OP;
				else
				{
					if (num_links)
						select = LINK_OP;
					else
						select = SKIP_OP;
				}
			}
			else
								/* L037 */
				select = name_ok ? NORMAL_OP : SKIP_OP;

			if ((select == NORMAL_OP))
			{
				/*
				* If this is a potential link record then
				* store it.
				*/
				if ((ftype == S_IFREG) && (IS_NEW(Cflag)) &&
				(Hdr.h_nlink > 1) && (Hdr.h_filesize == 0)){
					if (addlink(&Hdr))
					{
						Ofile = 0;
						select = SKIP_OP;
					} else
						errorl(MSGSTR(CPIO_MSG_NO_MEM2, "out of memory for hard link storage."));
				}
				else
					if (!Toc)
					{
						Ofile = openout(Hdr.h_name);
						++xcnt;		/* S031 */
					}
			}

			if (select == LINK_OP)
			{
				char *try_name;

				while ((try_name = get_next_link(Hdr.h_ino,
							Hdr.h_dev_maj,Hdr.h_dev_min)) != (char *)NULL)
				{
					if ((Ofile = openout(try_name)) != 0)
					{
						++xcnt;
						break;
					}
				}
				if (Ofile)
				{
					strcpy(Hdr.h_name,try_name);
					select = NORMAL_OP;
				}
				else
					select = SKIP_OP;

			}
			filesz=Hdr.h_filesize;
			if((select == NORMAL_OP) && maxsz < filesz) {
				zclose( EERROR, Ofile );
				Ofile = 0;
				select = SKIP_OP;
				errorl(MSGSTR(CPIO_MSG_SKIPPED, "%s skipped: exceeds ulimit by %d bytes."), Hdr.h_name, filesz - maxsz);
			}
			first = 1;

			/* i is used to align ourselves after each file. The
			   next header will be alligned on shorts for the old
			   binary header, and longs for the new ascii and
			   crc headers.
			*/
			switch (Cflag) {
			case BINARY :	i = (filesz % 2);
					break;
			case CRC    :
			case ASCII  :	i = (filesz % 4);
					break;
			default     :	i = 0;
					break;
			}					/* L032 End */

								/* L011 { */
			/* If symbolic link, process it differently than */
			/* a regular file.				 */
			/* Read it into Buf, turn it into a string, and	 */
			/* then try to create it ( with any associated	 */
			/* directories).				 */
			if ( A_symlink)
			{
				errno = 0;
				if ( bread( Buf, filesz, filesz + i) == -1)
				{
					errorl(MSGSTR(CPIO_MSG_IO_ERR, "i/o error, %s is corrupt."), Hdr.h_name);
					Buf[0] = '\0';
					goto jp_sym1;
				}/*if*/
				Buf[filesz] = '\0';
				if ((select == NORMAL_OP) && !Toc)
				{
					if ( symlink( Buf, Hdr.h_name) < 0)
						if ( missdir( Hdr.h_name) == 0)
						{
							if ( symlink( Buf, Hdr.h_name) < 0)
								errorl(MSGSTR(CPIO_MSG_NO_CREATE_SYM, "cannot create symbolic link %s."), Hdr.h_name);
						}
						else
							errorl( MSGSTR(CPIO_MSG_NO_CREATE_DIR, "Cannot create directory for symbolic link <%s>."), Hdr.h_name);
				}/*if*/
				goto jp_sym1;
			}/*if*/					/* L011 } */
			for(; filesz>0; filesz-= CPIOBSZ){
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
				errno = 0;
				if(bread(Buf, ct, filesz + i) == -1) {
					errorl(MSGSTR(CPIO_MSG_IO_ERR, "i/o error, %s is corrupt."), Hdr.h_name);
					break;
				}
				if (nflag)			/* L029 */
					filesum = sum(Buf,ct,filesum);

								/* L032 Begin */
				if (IS_CRC(Cflag) && (ftype == S_IFREG))
				{
					register char *ptr = Buf;
					register char *end = Buf + ct;

					while (ptr < end)
					{
						crc += (*ptr & 0xff);
						ptr++;
					}
				}
								/* L032 End */
				if(Ofile) {
					if(swapopt)
						swap(Buf,ct,swapopt);
					errno = 0;
					if((rc = write(Ofile, Buf, ct)) < ct) {
						if(rc < 0) {
							if(first++ == 1) {
								psyserrorl(errno,
								MSGSTR(CPIO_MSG_NO_WRT, "cannot write %s"), /* L026 */
								Hdr.h_name);
							}
							if (errno == EFBIG || errno == ENOSPC)
								continue;
							else
								cleanup(2);
						}
						else {
							errorl(MSGSTR(CPIO_MSG_TRUNC, "%s truncated."),
								Hdr.h_name);
							continue;
						}
					}
				}
			}

			if( Ofile ) {
				zclose( EERROR, Ofile );
				set_inode_info(Hdr);
			}

								/* L032 Begin */
			if (IS_CRC(Cflag) && (ftype == S_IFREG) &&
			     (Hdr.h_filesize) && (crc != Hdr.h_chksum))
				errorl(MSGSTR(CPIO_MSG_WARN_CHKSUM, "WARNING checksum error in %s."),Hdr.h_name);

			/* If filesize is not zero, and nlink is > 1 
			   we will want to check our link records
			   to see if we have any links that need creating.

			   It is best to do this after we have created
			   this entry and set the permissions. Of course
			   we still may be stuck if one of the links we try
			   to create is in a read only directory, but this
			   is a problem with this archive format.

			*/

			if (IS_NEW(Cflag) && (ftype == S_IFREG) &&
			    (Hdr.h_nlink > 1) && (Hdr.h_filesize != 0) &&
			    (linkcount(&Hdr)))
			{
					/* Print out the link data, but use the filesize in this
					 * header.
					 */
					plinks(&Hdr,Toc,Verbose,filesum,nflag);

					/* Try to create the links if possible.
					 */
					if (!Toc)
						create_links(Hdr);
			}

jp_sym1:							/* L011 */

			if(select == NORMAL_OP) {
				if(Verbose)
					if(Toc)
						if ( A_symlink)	/* L011 { */
							pentry( Hdr.h_name, Buf, Hdr.h_uid, Hdr.h_mode, Hdr.h_filesize, Hdr.h_mtime, 0, nflag);
						else
							pentry(Hdr.h_name, "", Hdr.h_uid, Hdr.h_mode, Hdr.h_filesize, Hdr.h_mtime, filesum, nflag);
								/* L011 } */
					else
						verbdot( stdout, Hdr.h_name);
				else if(Toc)
					if ( A_symlink)		/* L011 { */
					{
						/* Indicate symbolic link */
						fputs( Hdr.h_name, stdout);
						fputs( " -> ", stdout);
						puts( Buf);
					}
					else
						puts(Hdr.h_name);/* L011 } */
			}
		}

		/* Finished the archive, so see if we have any real 0 length
		   files being stored in our link list. If so, then create
		   them.
		*/

		if (!Toc)
			flush_link_list();			/* L032 End */

		if (buf.b_count > Bufsize)
			Blocks -= (buf.b_count / Bufsize);
		break;

	case PASS:		/* move files around */
		if(argc != 1)
			usage();
		if(eaccess(argv[0], 2) == -1) {			/* S007 */
			psyserrorl(errno, MSGSTR(CPIO_MSG_NO_WRT2, "cannot write in <%s>"), argv[0]);
			exit(2);
		}
		strcpy(Fullname, argv[0]);	/* destination directory */
		if (stat(Fullname, &Xstatb) == -1)
		{
			psyserrorl(errno,MSGSTR(CPIO_MSG_NO_STAT, "cannot stat %s"),Fullname);
			exit(2);
		}
		if((Xstatb.st_mode&S_IFMT) != S_IFDIR)
			errorl(MSGSTR(CPIO_MSG_NOT_DIR, "<%s> not a directory."), Fullname );
		Dev = Xstatb.st_dev;
		if( Fullname[ strlen(Fullname) - 1 ] != '/' )
			strcat(Fullname, "/");
		fullp = Fullname + strlen(Fullname);

		while(getname()) {
			if (A_directory && !Dir)
				errorl(MSGSTR(CPIO_MSG_OPT_D, "Use `-d' option to copy <%s>."),
					Hdr.h_name);
			if(!ckname(Hdr.h_name))
				continue;
			i = 0;
			while(Hdr.h_name[i] == '/')
				i++;
			strcpy(fullp, &(Hdr.h_name[i]));

			if(!Uncond && !A_directory)		/* L035 */
				if(!cktime(Fullname,Hdr.h_mtime))
					continue;

			if( PassLink  && !A_symlink && !A_directory  &&  Dev ==  Statb.st_dev ) {						/* L016 */
				if(link(Hdr.h_name, Fullname) < 0) {
					switch(errno) {
						case ENOENT:
							if(missdir(Fullname) != 0) {
					/* S007... */		psyserrorl(errno,
									MSGSTR(CPIO_MSG_NO_CREATE_DIR2, "Cannot create directory for <%s>"),
									Fullname);
								goto lnkfailed;
							}
							break;
						case EEXIST:
							if(unlink(Fullname) < 0) {
					/* S007... */		psyserrorl(errno,
									MSGSTR(CPIO_MSG_NO_UNLINK, "cannot unlink <%s>"),
									Fullname);
								goto lnkfailed;
							}
							break;
						default:
					/* S007... */	psyserrorl(errno,
								MSGSTR(CPIO_MSG_NO_LINK, "cannot link <%s> to <%s>"),
								Hdr.h_name, Fullname);
							goto lnkfailed;
						}
					if(link(Hdr.h_name, Fullname) < 0) {
						psyserrorl(errno,
							MSGSTR(CPIO_MSG_NO_LINK, "cannot link <%s> to <%s>"),
							Hdr.h_name, Fullname);
						goto lnkfailed;
					}
				}

				goto ckverbose;
			}
			/* Pass a symbolic link: read it and */	/* L011 { */
			/* then re-create it.		     */
			if ( A_symlink)
			{
				int SymSize;

				if ((SymSize = readlink( Hdr.h_name, Buf, CPIOBSZ-1)) < 0) /* L026 */
					errorl( MSGSTR(CPIO_MSG_NO_READ_SYM, "cannot read symbolic link %s"), Hdr.h_name);
				else
				{				/* L026 Begin */
					Buf[SymSize] = '\0';
trysymlink:				if ( symlink( Buf, Fullname) < 0)
						switch (errno){

							case EEXIST:	if(unlink(Fullname) >= 0)
										goto trysymlink;
										break;
							case ENOENT:	if(missdir(Fullname) == 0)
										goto trysymlink;
									else
										errorl(MSGSTR(CPIO_MSG_NO_CREATE_DIR, "Cannot create directory for symbolic link <%s>."), Hdr.h_name);
									continue;

							default:	psyserrorl(errno,MSGSTR(CPIO_MSG_NO_WRT_SYM, "cannot write symbolic link %s"), Hdr.h_name);
									continue;
						}
				}/*if*/				/* L026 End */
				goto ckverbose;			/* L016 */
			}/*if*/					/* L011 } */
lnkfailed:							/* L016 */
			if(!(Ofile = openout(Fullname)))
				continue;
			if((Ifile = zopen( EWARN, Hdr.h_name, 0)) < 0) {
				close(Ofile);
				continue;
			}
			filesz = Statb.st_size;
			for(; filesz > 0; filesz -= CPIOBSZ) {
				ct = filesz>CPIOBSZ? CPIOBSZ: filesz;
				errno = 0;
				if(read(Ifile, Buf, (unsigned)ct) < 0) {
					psyserrorl(errno,MSGSTR(CPIO_MSG_NO_READ, "cannot read %s"), Hdr.h_name);
					break;
				}
				if (Ofile) {
					errno = 0;
					if (write(Ofile, Buf, ct) < 0) {
						psyserrorl(errno,MSGSTR(CPIO_MSG_NO_WRT, "cannot write %s"), Hdr.h_name);
						break;
					}
				}
					/* Removed u370 ifdef which caused cpio */
					/* to report blocks in terms of 4096 bytes. */

				Blocks += ((ct + (BUFSIZE - 1)) / BUFSIZE);
			}
			close(Ifile);
			if(Acc_time)
				utime(Hdr.h_name, &Statb.st_atime);
			if(Ofile) {
				close(Ofile);
				zchmod( EWARN, Fullname, Hdr.h_mode);
				set_time(Fullname, Statb.st_atime, Hdr.h_mtime);
ckverbose:
				if(Verbose)
					verbdot( stdout, Fullname );
			}
		}
	}
	/* print number of 512 byte blocks actually copied     L040 */
	fprintf(stderr,MSGSTR(CPIO_MSG_BLKS, "%ld blocks\n"), block_usage(Blocks, Bufsize, sBlocks));
	cleanup(error_cnt);
}

								/* L038 Begin */
static int read_until_null(FILE *fp, char *buffer, int size)
{
	int c;

	*buffer = '\000';
	while (((c = fgetc(fp)) != EOF) && (size--))
	{
		*buffer = c;
		if (*buffer == '\000')
			break;
		buffer++;
	}
	switch (c)
	{
	case EOF	:	return(0);
	case '\000'	:	return(1);
	default		:	while (((c = fgetc(fp)) != EOF) &&
							(c != '\000'));
	}
	return(c != EOF);
}
								/* L038 End */

static int getname()		/* get file name, get info for header */
{
	register char *namep = Name;
	register ushort ftype;
	char *p;				/* SCO_BASE S002 */

	for(;;)
	{
								/* L038 Begin */
		if (cpio_from_find)
		{
			/* Read in a NULL terminated filename instead of
			 * a newline terminated one.
			 */
			if (read_until_null(stdin,namep,PATHSIZE) == 0)
				return(0);
		}
								/* L038 End */
		else
		{
			if( (p = fgets(namep, sizeof(Name), stdin)) == NULL)
				return 0;
			skipln(p, stdin);
			if ( strlen( namep) > PATHSIZE) { /* L023 */ /* L016{ */
				(void) errorl( MSGSTR(CPIO_MSG_WARN_ARC, "WARNING: Cannot archive %s,pathname is greater than %d chars."), namep, PATHSIZE);
				continue;
			}				/* L016 } */
		}


		while(*namep == '.' && namep[1] == '/') {
			namep++;
			while(*namep == '/') namep++;
		}
		if (!*namep)				/* L015 */
			namep = ".";
		strcpy(Hdr.h_name, namep);
		/* Follow any symbolic link if, and only if, */
		/* SymLink ( the `L' flag is set ).	     */
		if((SymLink?zstat( EWARN, namep, &Statb):zlstat( EWARN, namep, &Statb)) < 0) {						/* L011 L036 */
			continue;
		}
		ftype = Statb.st_mode & Filetype;
		A_directory = (ftype == S_IFDIR);
		A_special = (ftype == S_IFBLK)
			|| (ftype == S_IFCHR)
			|| (ftype == S_IFNAM)			/* L040 */
			|| (ftype == S_IFIFO);
		A_symlink = (ftype == S_IFLNK);			/* L011 */

		/* Magic gets filled in by the build_header routines. */

		Hdr.h_namesize = strlen(Hdr.h_name) + 1;
		Hdr.h_uid = Statb.st_uid;
		Hdr.h_gid = Statb.st_gid;
		Hdr.h_dev_maj = major(Statb.st_dev);
		Hdr.h_dev_min = minor(Statb.st_dev);
		Hdr.h_ino = Statb.st_ino;
		Hdr.h_mode = Statb.st_mode;
		Hdr.h_mtime = Statb.st_mtime;
		Hdr.h_nlink = Statb.st_nlink;
		Hdr.h_filesize = (((Hdr.h_mode&S_IFMT) == S_IFREG)
			|| (A_symlink)) ? Statb.st_size: 0L;	/* L011 */
		Hdr.h_chksum = 0;
		Hdr.h_rdev_maj = major(Statb.st_rdev);
		Hdr.h_rdev_min = minor(Statb.st_rdev);
		return 1;
	}
}


static int gethdr()		/* get file headers */
{
	register ushort ftype;

	synch();
	if(EQ(Hdr.h_name, "TRAILER!!!"))
		return 0;
	ftype = Hdr.h_mode & Filetype;
	A_directory = (ftype == S_IFDIR);
	A_special = (ftype == S_IFBLK)
		||  (ftype == S_IFCHR)
		||  (ftype == S_IFNAM)				/* L040 */
		||  (ftype == S_IFIFO);
	A_symlink = (ftype == S_IFLNK);				/* L011 */
	return 1;
}

/* open files for writing, set all necessary info */
static int openout(char *namep)
{
	register f;
	register char *np;
	int ans;

	if(!strncmp(namep, "./", 2))
		namep += 2;
	np = namep;
	if(A_directory) {
		if( !Dir  ||  Rename 
		|| (EQ(namep, ".") && !Uncond)	/* allow changes to "." when -u flag is given L015 */
		||  EQ(namep, "..") )
			/* do not consider . or .. files */
			return 0;
		if(statlstat(namep, &Xstatb) == -1) {		/* L011 */

/* try creating (only twice) */
			(void) umask(orig_umask);			/*L010*/
			ans = 0;
			do {
				if (mkdir(namep, 0777) != 0) {		/*L012*/
					ans += 1;
				}else {
					ans = 0;
					break;
				}
			}while(ans < 2 && missdir(namep) == 0);
			(void) umask(0);				/*L010*/
			if(ans == 1) {
				psyserrorl(errno,MSGSTR(CPIO_MSG_NO_CREATE_DIR2, "Cannot create directory for <%s>"),
					namep);
				return(0);
			}else if(ans == 2) {
				psyserrorl(errno,MSGSTR(CPIO_MSG_NO_CREATE_DIR3, "Cannot create directory <%s>"), namep);
				return(0);
			}
		}

ret:
		zchmod( EWARN, namep, Hdr.h_mode);
		if(Uid == 0)
			zchown( EWARN, namep, Hdr.h_uid, Hdr.h_gid);
		set_time(namep, Hdr.h_mtime, Hdr.h_mtime);
		return 0;
	}

	if(Hdr.h_nlink > 1)
		if(!postml(namep, np))
			return 0;


	if(statlstat(namep, &Xstatb) == 0) {			/* L011 */
		if(Uncond && !((!(Xstatb.st_mode & S_IWRITE) || A_special) && (Uid != 0))) {
			if(unlink(namep) < 0) {
				psyserrorl(errno,MSGSTR(CPIO_MSG_NO_UNLINK2, "cannot unlink current <%s>"), namep);
			}

		}
	}

	if( Option == PASS
		&& Hdr.h_ino == Xstatb.st_ino
		&& Hdr.h_dev_maj == major(Xstatb.st_dev)
		&& Hdr.h_dev_min == minor(Xstatb.st_dev)) {
		errorl(MSGSTR(CPIO_MSG_PASS_ERR, "attempt to pass file to self!"));
		exit(2);
	}
	if(A_special) {
		if((Hdr.h_mode & Filetype) == S_IFIFO)
			Hdr.h_rdev_maj = Hdr.h_rdev_min = 0;

/* try creating (only twice) */
		ans = 0;
		do {
			if(mknod(namep, Hdr.h_mode,
				makedev(Hdr.h_rdev_maj,Hdr.h_rdev_min)) < 0) {
				ans += 1;
			}else {
				ans = 0;
				break;
			}
		}while(ans < 2 && missdir(np) == 0);
		if(ans == 1) {
			psyserrorl(errno,MSGSTR(CPIO_MSG_NO_CREATE_DIR2, "Cannot create directory for <%s>"), namep);
			return(0);
		}else if(ans == 2) {
			psyserrorl(errno,MSGSTR(CPIO_MSG_NO_MKNOD, "cannot mknod <%s>"), namep);
			return(0);
		}

		goto ret;
	}
	if ( A_symlink)	/* no point creat()ing a symbolic link here *//* L011 */
		return( 1);					      /* L011 */

/* try creating (only twice) */
	ans = 0;
	do {
		if((f = creat(namep, (mode_t)Hdr.h_mode)) < 0) {
			ans += 1;
		}else {
			ans = 0;
			break;
		}
	}while(ans < 2 && missdir(np) == 0);
	if(ans == 1) {
		psyserrorl(errno,MSGSTR(CPIO_MSG_NO_CREATE_DIR2, "Cannot create directory for <%s>"), namep);
		return(0);
	}else if(ans == 2) {
		psyserrorl(errno,MSGSTR(CPIO_MSG_NO_CREATE, "Cannot create <%s>"), namep);
		return(0);
	}

	if(Uid == 0)
		zchown( EWARN, namep, Hdr.h_uid, Hdr.h_gid);
	return f;
}

	/*
	 * synch searches for and verifies all headers.  Any user specified 
	 * Cflag type is ignored.  Cflag is set appropriately after a valid
	 * header is found.  Unless the -k option is set a corrupted header 
	 * causes an exit with an error.  I/O errors during examination of any 
	 * part of the header causes synch to throw away any current data and 
	 * start over.  Other errors during examination of any part of the 
	 * header cause synch to advance one byte and continue the examination.
	 */

static void synch()
{
	register int hit = NONE, cnt = 0;
	int hsize = 0, offset, align = 0;
	static int min = CHARS;
	static oheader Ohdr;

	union {
		char bite[2];
		ushort temp;
	} mag;

	if (Cflag == NONE) {
		fillbuf = 1;
		if (bread(Chdr, 0, oamflag?512:0) == -1)	/* L016 */
			errorl(MSGSTR(CPIO_MSG_IO_ERR2, "I/O error, searching to next header."));
	}
	do {
		while (buf.b_count < min)
			rstbuf();

		/* Is this a new ASCII format archive. */
		if (IS_ASCII(Cflag) || Cflag == NONE) {
			if (!strncmp(buf.b_out_p, M_ASCII, M_STRLEN)) {
				memcpy(Chdr, buf.b_out_p, CHARS);
				chartobin(Chdr,&Hdr);
				hit = ASCII;
				hsize = CHARS + Hdr.h_namesize;
				align = (4 - (hsize % 4)) % 4;
			}
		}

		/* Is this a new ASCII CRC format archive. */
		if (IS_CRC(Cflag) || Cflag == NONE) {
			if (!strncmp(buf.b_out_p, M_CRC, M_STRLEN)) {
				memcpy(Chdr, buf.b_out_p, CHARS);
				chartobin(Chdr,&Hdr);
				hit = CRC;
				hsize = CHARS + Hdr.h_namesize;
				align = (4 - (hsize % 4)) % 4;
			}
		}

		/* Is this an old ASCII format archive. */
		if (IS_OASCII(Cflag) || Cflag == NONE) {
			if (!strncmp(buf.b_out_p, M_OASCII, M_STRLEN)) {
				memcpy(Chdr, buf.b_out_p, OCHARS);
				ochartobin(Chdr,&Hdr);
				hit = OASCII;
				hsize = OCHARS + Hdr.h_namesize;
				min = OCHARS;
			}
		}

		if (IS_BINARY(Cflag) || Cflag == NONE) {
			mag.bite[0] = buf.b_out_p[0];
			mag.bite[1] = buf.b_out_p[1];
			if (mag.temp == M_BINARY) {
				memcpy(&Ohdr, buf.b_out_p, OHDRSIZE);
				expand_hdr(Ohdr,&Hdr);
				hit = BINARY;
				hsize = OHDRSIZE + Hdr.h_namesize;
				align =	(Hdr.h_namesize % 2) ? 1 : 0;
				min = OHDRSIZE;
			}
		}

		if (hit == NONE) {
			buf.b_out_p++;
			buf.b_count--;
		} else {
			if (hdck(Hdr) == -1) {
				buf.b_out_p++;
				buf.b_count--;
				hit = NONE;
				if (kflag)
					errorl(MSGSTR(CPIO_MSG_HEAD_CORRUPT, "Header corrupted, file(s) may be lost."));
			} else {			/* consider possible alignment byte */
				while (buf.b_count < hsize + align)
					rstbuf();
				if (hit == NONE)
					continue;
				if (*(buf.b_out_p + hsize - 1) != '\0') {
					buf.b_out_p++;
					buf.b_count--;
					hit = NONE;
					continue;
				}
			}
		}
		if (cnt++ == 2)
			errorl(MSGSTR(CPIO_MSG_OUT_SYNC, "Out of sync, searching for magic number/header."));
	} while (hit == NONE && kflag && (buf.b_count > 0));
	if ((cnt > 2) && buf.b_count == 0)
		errorl(MSGSTR(CPIO_MSG_RESYNC, "Re-synchronized on magic number/header."));
	if (hit == NONE) {
		if (Cflag == NONE)
		{
			errorl(MSGSTR(CPIO_MSG_NOT_CPIO_FILE, "this is not a cpio file, bad header."));
			exit(2);
		}
		else
		{
			errorl(MSGSTR(CPIO_MSG_OUT_SYNC2, "out of sync, bad magic number/header."));
			exit(2);
		}
	}	
	switch (hit) {
	case CRC    :
	case ASCII  :	memcpy(Hdr.h_name, buf.b_out_p + CHARS, Hdr.h_namesize);
			break;
	case OASCII :	memcpy(Hdr.h_name, buf.b_out_p+OCHARS, Hdr.h_namesize);
			break;
	default     :	memcpy(Hdr.h_name, buf.b_out_p+OHDRSIZE, Hdr.h_namesize);
			break;
	}
	Cflag = hit;
	offset = hsize + align;
	buf.b_out_p += offset;
	buf.b_count -= offset;
}

/* linking function:  Postml() is called after namep is created.	*/
/* Postml() checks to see if namep should be linked to np.  If so,	*/
/* postml() removes the independent instance of namep and links		*/
/* namep to np.								*/

static int postml(char *namep, char *np)		
{

	register i;
	static struct ml {
		ulong	m_dev_maj;
		ulong	m_dev_min;
		ulong	m_ino;
		char	m_name[2];
	} **ml = 0;

	register struct ml	*mlp;
	static unsigned	mlsize = 0;
	static unsigned	mlinks = 0;
	char		*lnamep;
	int		ans;

	if( !ml ) {
		mlsize = LINKS;
		ml = (struct ml **) zmalloc( EERROR, mlsize * sizeof(struct ml));
	}
	else if( mlinks == mlsize ) {
		mlsize += LINKS;
		ml = (struct ml **) zrealloc( EERROR, ml, mlsize * sizeof(struct ml));
	}
	for(i = 0; i < mlinks; ++i) {
		mlp = ml[i];
		if(mlp->m_ino == Hdr.h_ino  &&
			mlp->m_dev_maj == Hdr.h_dev_maj &&
				mlp->m_dev_min == Hdr.h_dev_min) {
			if(Verbose == 1)
				errorl(MSGSTR(CPIO_MSG_LINKED, "%s linked to %s."), mlp->m_name, np);
			if(Verbose && Option == PASS)
				verbdot(stdout, np);
			unlink(namep);
			if(Option == IN && *(mlp->m_name) != '/') {
				Fullname[Pathend] = '\0';
				strcat(Fullname, mlp->m_name);
				lnamep = Fullname;
			}
			lnamep = mlp->m_name;

/* try linking (only twice) */
			ans = 0;
			do {
				if(link(lnamep, namep) < 0) {
					ans += 1;
				}else {
					ans = 0;
					break;
				}
			}while(ans < 2 && missdir(np) == 0);
			if(ans == 1) {
				psyserrorl(errno,MSGSTR(CPIO_MSG_NO_CREATE_DIR2, "Cannot create directory for <%s>"), np);
				return(0);
			}else if(ans == 2) {
				psyserrorl(errno,MSGSTR(CPIO_MSG_NO_LINK2, "Cannot link <%s> & <%s>"), lnamep, np);
				return(0);
			}

			set_time(namep, Hdr.h_mtime, Hdr.h_mtime);
			return 0;
		}
	}
	if( !(ml[mlinks] = (struct ml *)zmalloc( EWARN, strlen(np) + 2 + sizeof(struct ml)))) {
		static int first=1;

		if(first)
			errorl(MSGSTR(CPIO_MSG_NO_MEM3, "No memory for links (%d)."), mlinks);
		first = 0;
		return 1;
	}
	ml[mlinks]->m_dev_maj = Hdr.h_dev_maj;
	ml[mlinks]->m_dev_min = Hdr.h_dev_min;
	ml[mlinks]->m_ino = Hdr.h_ino;
	strcpy(ml[mlinks]->m_name, np);
	++mlinks;
	return 1;
}

int cktime(char *filename, ulong mtime)
{
	static struct stat	buf;

	if ((statlstat(filename, &buf) == 0) && (mtime <= buf.st_mtime))
	{
		errorl(MSGSTR(CPIO_MSG_NEWER, "current <%s> newer or same age."), filename);
		return(0);
	}

	return(1);
}
