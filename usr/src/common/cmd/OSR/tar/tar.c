#pragma comment(exestr, "@(#) tar.c 61.1 97/02/25 ")

/*
 *	Copyright (C) The Santa Cruz Operation, 1988-1997.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */

/*
 *	Copyright (C) Microsoft Corporation, 1983
 *	Copyright (c) 1984-1997 The Santa Cruz Operation, Inc.
 *	All rights reserved.
 *
 *	This Module contains Proprietary Information of Microsoft
 *	Corporation, The Santa Cruz Operation, Inc., and AT&T,
 *	and should be treated as Confidential.
 */
/***	tar -- tape archiving program.
 *
 *	tar [key] [file ...]
 *
 *      MODIFICATION HISTORY
 *
 * HCR: SP-1: Steve Pozgaj, February, 1981
 *	-add multivolume capability to 'rc' options
 *
 * arw	 1 Jun 1981	M000
 *	-Allowed minimum size for multivolume to be 500 for 5.25" floppies.
 *	-Added some register declarations.
 *
 * arw	11 Aug 1981	M001
 *	-Changed default tape name to avoid inadvertent destruction of
 *       mounted mag tapes.  Conditional on MSLOCAL.
 *
 * arw	 1 Sep 1981	M002
 *	-Corrected flushtape to properly handle blocked, multi-volume tapes.
 *	-Added check that blocklim is a multiple of nblock, corrected
 *	 computation of actual blocks to be used.
 *	-Allowed interruption between volumes.
 *
 * jgl  8 Sep 1981      M003
 *      - fixed bug preventing tar from handling 512 byte records via
 *        raw mag tape
 *
 * MJN	6/19/82		M004
 *	files too big to fit on one floppy are split.  Extents are
 *	supported transparently, only with 's' (mulvol) option
 *	New option 'n' meaning "not a magtape" causes seek instead
 *	of read when skipping over files.
 *
 * MJN	6/28/82		M005
 *	changed 's' option to match 'k' option in dump:  now takes
 *	a kilobyte size.  All sizes are printed in K instead of blocks.
 *	Corrected previous correction to flushtape().
 *	Changed all printf's of sizes to use %lu instead of %ld or %D.
 *
 *	***	BASIC ASSUMPTIONS
 *	Tar assumes that the system block size (currently either
 *	512 or 1024 bytes--see BSIZE in /usr/include/sys/param.h) is
 *		(1) a multiple of TBLOCK (512),
 *		(2) equal to TBLOCK, or
 *		(3) a divisor of TBLOCK.
 *	If it is a multiple, for example on a 1K block filesystem,
 *	extra code is compiled in to ensure that read/write/seek on
 *	non-magtape devices is performed only on 1K boundaries.
 *	Otherwise, no check is necessary.
 *
 * JGL	07/28/82	M006
 *	- fixed bug which left open channels when special files 
 *	  encountered.  More tha 16 special files and tar couldnt
 *	  do any more opens.
 *
 * MJN	8/7/82		M007
 *	Changed "End of file reread failure" to be a warning only.
 *
 * MK	8/10/82		M008
 *	added p switch which restores files to their original modes,
 *	ignoring present umask value.  Setuid and sticky information
 *	will be also restored to the super-user.
 *
 * JGL  09/22/82        M009
 *      - added various comments
 *      - fixed -r and -u handling for non-tape files
 *      - cleaned up backtape() handling for -r and -u, its still
 *        quite nasty
 *
 * ADP	08/15/83	M010	3.0 upgrade
 *	- used v7 source.
 *	- picked up 2..7 option.
 * ADP	09/09/83	M011
 *	- added 'F/filename' key/modifier pair to allow specifying a file
 *	  with list of pathnames in place of arglist.
 *
 * BAS	09/09/83	M012
 *	- if a non fatal error occurs (ie cause tar to exit), then
 *	  the exit code will be set to a value greater than 0
 *	- the inode of the archive file and the input file are compared
 *	  so that if they are the same then that input file is skipped.
 *	  This prevents the user from archiving the archive file with
 *	  such commands as "tar cf file.a *".
 *	- moved M006 so that a check is done on the file type before the
 *	  file is opened.  This change prevents the opening of file devices
 *	  that will cause some action to be taken when opened (for example
 *	  tape drives would rewind when the file is opened).
 *	- if the number of files actually extracted and the files listed
 *	  on the command line are different, then a warning message is given.
 * ADP	07 Jan 84	M013
 *	- parameterized some stuff, upped a limit.  WARNING: should add some
 *	  error checks for overflow of pathname buffers.  [ see S051 ]
 * ADP	16 Feb 84	M014
 *	- Fixed bug in treatment of special files, it turns out the S_IF*
 *	  masks are not disjoint.
 * ADP	01 Apr 84	M015
 *	- Get physio buffer using physalloc(3).
 * BAS	17 Apr 84	M016
 *	- After a file has been extracted a check is made to ensure that the
 *	  file permissions of the extracted file are the same as those
 *	  in the tar file.  If the permissions are not the same a warning
 *	  is issued.  This case can arise when a file with sticky permissions
 *	  is extracted.
 * BAS	17 Apr 84	M017
 *	- The access time for an extracted file was being set incorrectly.
 *	  This is because the return value for "time" was not declared to
 *	  be a long.
 * ADP	21 Apr 84	M018
 *	- Added checks to make sure don't recurse too deep and run out
 *	  of stack.  NOTE for fixed-stack machines: 'putfile' is the
 *	  routine to look at, it will be called recursively a maximum
 *	  of MAXLEV times.
 * BAS	8 May 84	M019
 *	- Added the provision to interrupt out of tar when the user
 *	  hits delete.  
 * BAS	17 May 84	M020
 *	- Added the "e" option.  If the "k" option is also specified,
 *	  then a file will not be split across volumes and an attempt
 *	  is made to fit the file entirely on the next volume.
 * ADP	13 Jun 84	M021
 *	- New switch "s".  Syntax is "s file", where file name be "-"
 *	  for stdout.  Causes /bin/sum algorithm to be performed on
 *	  the entire tar file.  Should be used in conjunction with "c",
 *	  "x", and "t".
 * BAS	20 Jul 84	M022
 *	- If the "n" switch is specified, then all messages will
 *	  reflect that the media is not a tape (ie. all block measurements
 *	  will be in K blocks instead of tape blocks).
 *	  
 * NCM  07 Dec 84	M023
 *	- Large files were not being correctly handled near the end of
 *	  volumes.  Now rechecks to see if they need splitting after a
 *	  new volume has been started, and the first check failed.
 *	- Changed 't' option to handle command line file args
 *	- Incorporated other SCO changes.
 *	- Added default initialization of device, blocksize and volume
 *	  size from /etc/default/tar.
 *	- Extracted files will now only be chowned if the 'p' flag is
 *	  specified or if the user has root privileges.  This prevents
 *	  extraction failures into newly created, unwritable directories.
 *	- Added 'A' flag which removes a leading '/' from absolute paths
 *	  on extraction.
 *
 * NCM	23 Jan 85	M024
 *	- Fixed problem where tar would issue a warning if umask was not
 *	  the same as when archive was created.
 *
 * NCM	16 May 85	M025
 *	- Corrected earlier fix to umask problem.
 *	- Linked files are now added into the count of extracted files.
 *	- Default file is not required if device is specified.
 *
 * NCM	19 Jun 85	M026
 *	- Include sys/types.h since sys/param.h no longer does.
 *
 * NCM	24 July 85	M027
 *	- Don't print blocksize/bufsize message unless verbose is on.
 *
 * JONB  2 June 86	M028
 *      - fixed r and u options for nblock > 1 (NCM & BLF).
 *	- made seek values correct for u, r options 
 * 	- added 'tape' field in /etc/default/tar.
 *	- NotTape set explicitly not implicitly via non-zero blocklim
 *	- checked for option consistency _after_ all were gotten.
 *	- skipped _multiple_ leading slashes for -A
 *	- errors in the /etc/default/tar file are not fatal.
 * 	  simply give a message and revert to the case
 *	  of the default file not existing.
 *	- set uid permissions printed with S if not executable.
 *	  same for set gid (G) and save text (T).
 *	- "tar cv" will not erase the archive but simply give a message.
 *	  one can create a null archive by "tar cv /dev/null" which will
 *	  give an error message but will null the archive.  /dev/null can
 *	  be replaced with a non-existent file.
 *	- the A flag inhibits _putting_ leading slashes in the directory
 *	  for the c, r, and u flags.
 * JONB   31 July 86	  M029
 *	- When extracting files with the -F option, 
 *	  the F file is read into memory for faster rereading of the filenames.
 *	  Had to move some code for the 'u' option after signals
 *	  were caught to avoid leaving a /tmp file.
 *	  it is not conclusive that this helps much!
 * JONB, NCM, BLF   23 Sept 86	  M030
 * 	- checking for an 'f' entry in /etc/default/tar removed.
 *	  8 and 9 can appear in default file.
 *	- the 'boolean' first changed to use TRUE, FALSE.
 *	- in readtape(), the setting of what size block to read has changed.
 *	  we used NBLOCK (20) if it was a tape or if the user did not
 *	  specify a blocksize via the b flag or via the default file.
 *	- checked that nblock was an even multiple of SYSBLOCK/TBLOCK
 *	  if the device file is a character special file and not a tape.
 * JONB, NCM 14 Oct 86	M031
 *	- for reading (keys of t or x) from stdin, make sure
 *	  that readtape() uses a blocking factor of 1.
 *	  make this check for writing to stdout too.
 * JONB Dec 10, 1986
 *	- checked for a bad return code from chdir to avoid
 *	  a funny bug where you do a tar cv ../nodir/bug
 *	  if bug existed in the current directory and nodir did
 *	  not exist or was unsearchable, then the bug file in 
 *	  current directory would be put on the tar floppy.
 * BUCKM  18 Mar 87	M033
 *	- fix the "tar | tar" bug.
 *	  the SysV shell sets up this pipeline such that the 2nd tar
 *	  is the parent of the 1st tar.  tar should not wait() for ALL
 *	  children after a fork()/exec() of "mkdir"; instead, just the
 *	  "mkdir" child process should be waited for.
 * NCM    18 Mar 87	M034
 *	- added q flag for quick extract or list of named arguments.
 *	  Stops after satisfying number of extract or list requests given.
 *	  Does not work properly (yet) with directories.
 *
 *	1 June 1987	blf	S035
 *		- Fold some long or frequently used strings together.
 *		- Make the use of system() a little more robust.
 *		- Added data checksumming under #ifdef CHKSUM.
 *		  The checksum in each header block covers the data
 *		  blocks written since the previous header block on
 *		  this volume.  Checksums do not extend across volumes
 *		  or header blocks, nor do they apply to header blocks.
 * 	11 June 87	scol!neilo	L036
 *		- Xenix Internationalisation Phase 1.
 *	    	  Use ctype(S) and conv(S) macros in response() to keep
 *        	  things clean.
 * 	11 June 87	scol!neilo	L037
 *		- Xenix Internationalisation Phase 1.
 *	  	  Change cmp()'s parameters to be register unsigned char *.
 * 	18 March 88	NCM		S038
 *		- print /etc/default/tar entries in usage message.
 * 	23 Feb 88	scol!bijan	L039
 *		- Xenix Internationalisation Phase ll.
 *		- Replace ctime	by strftime for LC_TIME dependency.
 * 	27 May 88	sco!rr		S040
 *		- Default device is now "archive=" in /etc/default/tar 
 *	  	  rather than "archive0="
 * 	28 Dec 88	sco!hoeshuen sco!jeffj		S041
 *		- allow archive number to be an ascii digit string of 1 to
 *	  	  ARC_NUM_LEN bytes.
 *		- define LPNMAX to be 128
 * 	Dec 88	  	MAF		S042
 *		- mods to allow using tar over NFS network. By MAF of Lachmann.
 * 	12 Feb 89	sco!chrisn	S043
 *		-quick hack to use the mkdir() system call instead of /bin/mkdir
 * 	30 March 89	sco!hoeshuen	S044
 *		- Redo S040 because S041 neglect the modification. 
 *	7 April 89	sco!hoeshuen	S045 
 *		- tar hangs when user tries to hit <del> because the wrong 
 *	  	  density is specified or the drive is empty. Check return 
 *	  	  status from wait() for signals before read().
 *	13 June 89	sco!hoeshuen	S046 
 *		- correct usage message: 
 *	  	  Remove '=' in front of the device name.
 *	  	  Display volume size in 1K blocks not in TBLOCK (512).
 *	6 July 89	scol!abraham scol!wooch		L047 
 * 		- Unix  Internationalisation Phase 1.
 *		- Replace i + '0' with todigit(i).
 *	9 Sept 89	sco!kenj	S048 
 *		- Added -C option which sets a new field in the tar header
 *	  	  indicating that the floppy contains compressed files. On
 *	  	  extraction this field is tested and if set all regular
 *	  	  files are decompressed.
 *	10 Sept 89	sco!kenj	S049 
 *		- Added -T flag to truncate long filenames to MAX_NAMELEN.  
 *	  	  This is a quick hack for internal use only. Note: It does 
 *	  	  not truncate directory components.
 * 	22 Sept 89	sco!brianm	S050
 *		- Now returns the errno equivalent for various exits.
 *		- fatal() now takes an exit value as first argument.
 *
 *	24 Nov 1989	scol!blf	S051
 *		- getwdir() assumed that the current working directory
 *		  is always 50 bytes or less in length; fix.
 *		- Converted fprintf();exit() sequences into fatal() or
 *		  equivalent, and make sure the errno value used as the
 *		  exit status is from the system call whose failure we
 *		  don't like (and not from, e.g., the write of the error
 *		  message).
 *		- Replaced bogus use of access(X_OK) with eaccess(F_OK),
 *		  and added #include of <unistd.h>.
 *		- NOTE 1:  I'm not convinced that S049 is correct -- if a
 *		  directory name is too long, the mkdir() in checkdir()
 *		  will fail because the do_trunc() seems to be done in
 *		  the wrong place.  Also, do_trunc() is claiming that
 *		  NAME_MAX is the same for all directories, which is
 *		  not true!  (See POSIX sections 2.9.5 and 5.7)
 *		- NOTE 2:  Mods S048 and S035 interfere with each other.
 *		  A CHKSUMming version will become confused if it reads
 *		  a compressed tar archive, and versa-visa.  This happens
 *		  because the tar header of is a different size for CHKSUM,
 *		  which moves the location of the cpressed flag, which can
 *		  look like part of the CHKSUM checksum!
 *
 * 	10 Nov 89	sco!kenj	S052
 *		- Only sets Cflag if file is not zero length. This makes
 *	  	  r floppies compatible with 3.2.0 custom.
 *	  	  Added -X flag which extracts compressed distributions without
 *	  	  de-compressing them.
 *
 * 	5 Dec 89	sco!kenj	S053
 *		- If Cflag then old file is unlinked before it is opened.
 *
 *	12 April 90	sco!kenl	S054
 *		- kcheck() was modified so that -k option could identify
 *		  non-numeric and negative archive arguments, and issue
 *		  an error message.
 * 
 * 	7 September 90 sco!alanw	S055
 * 		- tar was modified by deleting 2 lines that check
 *  		  for inode == 0.  This was a redundant check, causing
 * 		  problems with High Sierra file systems.  The semantics
 *		  of readdir changed, making this check redundant.
 *	
 *	Tue Oct 09 13:48:33 PDT 1990	sco!chapman	S056
 *		Fixed the "Missing filenames" feature.
 *
 *	10oct90		scol!hughd	L057
 *		- doing tar c twice in succession on the same set of unmodified
 *		  files would not necessarily generate exactly the same tarfile:
 *		  read into buffer on stack but didn't bother to zero it at eof
 *		  (since I don't have tape copying facilities here, I'd like to
 *		  create 3.2v2f MC tapes by tarring repeatly from the same tree:
 *		  but the checksums were coming out differently each time)
 *		- with blf's agreement, changed the "eaccess" call to a "stat":
 *		  allows tar x on Xenix 2.3.2 without Bad system call coredump
 *		- however, tar t on Xenix 2.3.2 still gives a Bad system call
 *		  on ftime (I believe: I'm inexperienced with adb and shared
 *		  libraries) if this tar is built to use shared libc_s
 *		- moved pragma comments up to the top and removed comment SID
 *
 *	4 January 1991	scol!markhe	L058
 *		- code modified to handle symbolic links.  With the new flag
 *		  '-L', symbolic links are followed.  Otherwise links are
 *		  not followed.
 *
 *	27 January 1991	scol!markhe	L059
 *		- modifications to remove lint/compiler warnings.
 *
 *	25feb91		scol!hughd	L060
 *		- replace lstat by statlstat so executable runs on versions
 *	  	  of the OS both with and without the lstat system call
 *
 *	01 November 1990 scol!dipakg	L061
 *		- truncate path name components if -T used
 *
 *	20 February 1991 scol!markhe	L062
 *		- checked long file name behaviour.
 *			- Confirmed all occurrences of NAMESIZ (100) are
 *			  correct: this so because the tar header is limited
 *			  to 100 char file names.
 *			- changed putfile() to use a working area of size
 *			  PATHSIZE, rather than size TBLOCK.
 *		- putfile() used to open a directory with open(), then later
 *		  on recognise it as a directory and use opendir().  Changed
 *		  this so that only opendir() is used on directories.
 *
 *	13 March 1991	scol!panosd	L063
 *		- tar's usage message showed only 10 entries even if
 *		  /etc/default/tar includes more than 10 entries.
 *		- Additionally, usage function expected exactly 10 sequentially
 *		  counted entries in /etc/default/tar file and if any of them
 *		  was missing then it complained.
 *		- With this modification, usage() picks up the last entry and
 *		  expects all the others to exist, from the 0th up to the last
 *
 *	11 June 1991	scol!markhe	L064
 *		- bug fixing:
 *		- stopped recursion when a non-searchable sub
 *		  directory is found.
 *		- moved chown/chmod in putfile() so when extracting
 *		  with p flag, setuid and setgid bits are preserved.
 *		- checkdir() was assigning random user and group ids
 *		  to directories created when extracting with p flag.
 *		- allow "tar cvf - | compress - > file" by sending
 *		  file list to stderr when output file is stdout.
 *	
 *	04dec91		scol!hughd	L065
 *		- tar's results have been reproducible since L057, but it
 *		  was still filling the end of a blocked tarfile with junk
 *		  left over from before: if nobody else does, I find this
 *		  confusing and undesirable (I was worried why making a
 *		  tape through tarperm came out different from direct tar):
 *		  do putempty()s if necessary before the last flushtape()
 *
 *	15dec91		scol!hughd	L066
 *		- aargh! when I checked that clearing the cpressed flag
 *		  worked on the MAN pages, I must have tried on a volume
 *		  that started with MAN pages: in general, once one file
 *		  had been extracted as cpressed, everything else thereafter
 *		  was extracted as cpressed irrespective of the flag setting:
 *		  fix this embarrassment and recut the 3.2.4n N disks!
 *
 *	22 July 1992	scol!jamesle	L067
 *		- In checkdir() stbuf had been made local and chown() 
 *		  was commented out. This meant that directories which
 *		  were created did not have their ownership set to the
 *		  ownership of the first file to be put in that
 *		  directory. This is the original functionality that
 *		  everyone has come to know and hate. I have restored
 *		  this functionality.
 *
 *	2 sep 92	scol!corinnew	L068
 *		- corrected error message for long pathnames
 *
 *	25 August 1992	scol!sohrab	L069
 *		- Usage message fixed to display "T" option. (Bug fix:
 *		  LTD-1068-2)
 *
 *	25 August 1992	scol!sohrab	L070
 *		- when appending to an archive using the absolute path
 *		  of the device, always it is assumed that the media 
 *		  is endless. This caused all the appending to the disk
 *		  media to fail. This fix however implies only 
 *		  disk archives can be appended.  
 *		  (Bug fix: LTD-21-56)
 *
 *	14 Sept 1992	scol!sohrab	L071
 *		- Option "o" sets id of the extracted files from the archive to
 *		  the user and group identifier of the user running the program
 *		  rather than those in the archive.
 *		  In SCO UNIX 3.2v4 this behaviour exists in the tar utility by
 *		  default.
 *		  Added flag '-o' to tar. The new option does not change the
 *		  functionality of the tar at all. If option "o" is passed to
 *		  tar, it will not complain. But tar will issue an error
 *		  message if this option is used incorrectly.
 *		  (Bug fix: SCO-97-92).
 *
 *	15 Sept 1992	scol!sohrab	L072
 *		- tar allows the use of '-f' option and a digit argument. The
 *		  result would be creation of a tar archive in the specified
 *		  file on the command line with the volume size of the device
 *		  in /etc/default/tar represented by digit argument.
 *		  When the size of the tar file exceeds the volume
 *		  size of the device, tar was asking for the second volume
 *		  and then creating the rest of the the archive in the 
 *		  specified file with flag '-f'. (Bug fix: SCO-58-2377).
 *
 *	20 Sept 1992	scol!sohrab	L073
 *		- when root restores an archive with or without "pflag" set
 *		  it always restores the original ownership and permission of
 *		  the files on the archive. This behaviour is incorrect.
 *		  Tar should behave exactly opposite, when it is used by
 *		  a normal user and  by the root.
 *		  The correct behaviour of tar is something historical. When
 *		  tar uses flag "p" to extract files, the id of the 
 *		  extracted files should be set to those of the root.  And 
 *		  when root does not use flag "p" during file extraction,
 *		  the user and the group identifier of the files should be
 *		  set to those of the original files on the archive.
 *	28 Jan 1993	scol!anthonys	L074
 *		- tar may have to exec() compress. If it does, split up the
 *		  the options and option arguments passed to compress
 *		  into separate strings. This makes it easier for
 *		  compress to parse its arguments.
 *	27 Jul 1993	scol!philk	L075
 *		- fix bug preventing use of archive devices of >1 digit.
 *		  Parsing of vol. parameter was broken
 *	06 Jul 1994	scol!ashleyb	L076
 *		- Avoid possible overflow in kcheck() and usage() by
 *		  changing the order of evaluation
 *		- Removed some compiler warnings (unmarked).
 *	18 Aug 1994	scol!ashleyb	L077
 *		- tar was using an uninitialised variable.
 *	20 Dec 1994	scol!ashleyb	L078
 *		- Allow `A' flag to be used with the `t' flag.
 *	21 Feb 1995	sco!calvinw	L079
 *		- Allow tar to write symlinks and directories.
 *		- Allow limited reading of ptar archives.
 *	02 Nov 1994	scol!trevorh	L080
 *	 - message catalogued.
 *	14 May 1995	sco!ncm		S081
 *		- Extracting directories should not clobber symlinks
 *		  that point to already existing directories.
 *	24 Feb 1997	johng@sco.com	S082
 *		- Added direct block mode support with. BUG ID SCO-244-262
 * 
 */

#include <unistd.h>		/* S051 - must be first include (POSIX) */
#include <sys/types.h>			/* M026 */
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <deflt.h>
#include <errno.h>			/* S050 */
#include <string.h>						/* L058 */
#include <fcntl.h>
#include "../include/osr.h"
#include "../include/sum.h"		/* M021 */

/* #include <errormsg.h> */						/* L080 Start */
#ifdef INTL
#  include <regex.h>
#  include <nl_types.h>
#  include <langinfo.h>
#  include <locale.h>
#  include "tar_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str)	(str)
#  define catclose(d)		/*void*/
#endif /* INTL */

#ifdef  INTL
static  nl_catd catd;
static  regex_t pregy;
#endif								/* L080 Stop */

/* Begin SCO_INTL */
#ifdef INTL
#include <time.h>			/* L039 */
#endif
/* End SCO_INTL */
#include <sys/param.h>		/* for PATHSIZE */		 /* L058 */
#ifndef MINSIZE				/* M023 ... */
#define MINSIZE 250
#endif

#ifndef LPNMAX				/* S041 ... */
#define LPNMAX PATHSIZE						/* L062 */
#endif

#define	DEF_FILE "/etc/default/tar"	/* ... M023 */

extern	char	*fgets();
#if	0	/* header file string.h makes this redundant */	/* L059 */
extern	int	strlen();
#endif								/* L059 */
extern	char	*malloc();
/* extern	char	*physalloc(); */	/* M015 */
#if 0		/* Declaration is in header file */		/* L059 */
extern	int	sprintf();
#endif								/* L059 */
extern	long	time();		/* M017 */

daddr_t	bsrch();
char	*nextarg();	/* M011 */
long	kcheck();	/* M023 */
char	*strtok();	/* M023 */

/* -DDEBUG	ONLY for debugging */
#ifdef	DEBUG
#undef	DEBUG
#define	DEBUG(a,b,c)	fprintf(stderr,"DEBUG - "),fprintf(stderr, a, b, c)
#else
#define	DEBUG(a,b,c)
#endif

#define	TBLOCK	512	/* tape block size--should be universal */
			/* Also the size used for block mode    */

#ifdef	BSIZE
#define	SYS_BLOCK BSIZE	/* from sys/param.h:  secondary block size */
#else
#define	SYS_BLOCK 512	/* default if no BSIZE in param.h */
#endif

#define NBLOCK	20
#define NAMSIZ	100	/* This is correct L062 */
#define	MODEMASK 07777	/* file creation mode mask */
#define	MAXEXT	100	/* M004 reasonable max # extents for a file */
#define	EXTMIN	50	/* M004 min blks left on floppy to split a file */

#define EQUAL	0	/* SP-1: for `strcmp' return */
#define UPDATE	2	/* SP-1: for `open' call */
#define	TBLOCKS(bytes)	(((bytes) + TBLOCK - 1)/TBLOCK)	/* useful roundup */
#define	K(tblocks)	((tblocks+1)/2)	/* tblocks to Kbytes for printing */

#define DEFAULT_YES     "y"		/* L080 */
char *command_name;			/* L080 */
char IGNORE;				/* L080 */
char ABORT;				/* L080 */

/* M018 */
#define	MAXLEV	18
#define	LEV0	1

#define TRUE	1
#define FALSE	0
#define MAX_NAMELEN 14		/* S049 Maximum name length for system file */

/* M015 Was statically allocated tbuf[NBLOCK] */
union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];	/* M004 size of this extent if file split */
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
		char extno[4];		/* M004 extent #, null if not split */
		char extotal[4];	/* M004 total extents */
		char efsize[12];	/* M004 size of entire file */
#ifdef CHKSUM			/* S035 begin... */
		char datsum[8];		/* checksum of previous file */
#endif				/* ...end S035 */
		char cpressed;		/* S048 Flag if compressed volume */
	} dbuf;
} dblock, *tbuf;

struct linkbuf {
	ino_t	inum;
	dev_t	devnum;
	int     count;
	char	pathname[NAMSIZ];
	struct	linkbuf *nextp;
} *ihead;

struct stat stbuf;
struct stat stbuf2;

int	rflag, xflag, vflag, tflag, mt, cflag, mflag, pflag;	/* M008 */
int	uflag;							/* M029 */
int	eflag;							/* M020 */
int	qflag;							/* M034 */
int	sflag, Sflag;						/* M021 */
int	bflag, kflag, Aflag;					/* M023 */
int	term, chksum, wflag, recno,  bmode,			/* S082 */
	first = TRUE, defaults_used = FALSE, linkerrok;
int	Cflag;							/* S048	*/
int	Tflag;							/* S049 */
int	oflag;							/* L071 */
int	Xflag;							/* S052	*/
int	Lflag = 0;	/* set non-zero if `L' option chosen */	/* L058 */
int	freemem = 1;
int	nblock = 1;						/* M025 */
int	Errflg = 0;						/* M012 */

dev_t	mt_dev;				/* device containing output file */
ino_t	mt_ino;				/* inode number of output file M012 */

daddr_t	low;
daddr_t	high;

FILE	*vout = stdout;		/* file for verbose messages */	/* L064 */
FILE	*tfile;
char	tname[] = "/tmp/tarXXXXXX";
/* char	archive[] = "archive0="; 			M023 */
/* Begin S041 */	/* Begin SCO_BASE */
/*
 *  The array archive will end up being "archiveX="
 *  where X is an ascii digit string of 1 to ARC_NUM_LEN
 *  bytes.  X is specified in the keywords string on
 *  the arg list, i.e. tar xv32, where 32 is the archive
 *  number.
 */

#define ARC_NAME	"archive"
#define ARC_NUM_LEN	4
#define ARC_NUM_IDX	sizeof( ARC_NAME ) - 1   /* subtract NULL */
#define ARC_END_IDX	ARC_NUM_IDX + ARC_NUM_LEN

/* add 2 to end idx to allow for '=' and '\0' */
char	archive[ARC_END_IDX + 2] = ARC_NAME; 			/* M023 */
/* end S041 */		/* End SCO_BASE */

char	deffile[] = DEF_FILE;	/* S035 */
char	*usefile;
char	*Filefile;		/* M011 */
char	*Sumfile;		/* M021 */
FILE	*Sumfp;			/* M021 */
struct suminfo	Si;		/* M021 */

#ifdef CHKSUM		/* S035 begin... */
int	zflag;
unsigned cursum;		/* current file's computed data checksum */
unsigned datsum;		/* last file's read data checksum */
int	didsum;			/* non-0 if actually computed cursum */
int	gotsum;			/* non-0 if actually read in datsum */
int	badsum;			/* number of bad data checksums */
#endif			/* ...end S035 */

int	mulvol;		/* SP-1: multi-volume option selected */
long	blocklim;	/* SP-1: number of blocks to accept per volume */
long	tapepos;	/* SP-1: current block number to be written */
long	atol();		/* SP-1: to get blocklim */
int	NotTape;	/* M004 true if tape is a disk */
int	dumping;	/* M004 true if writing a tape or other archive */
int	extno;		/* M004 number of extent:  starts at 1 */
int	extotal;	/* M004 total extents in this file */
long	efsize;		/* M004 size of entire file */
ushort	Oumask = 0;	/* M016 M025 old umask value */

/* L080 Compound message removed, Estdout used in full where called
*char	Estdout[]= "standard output archives";
*/

/* L080 Now initialised after main */
char * Enoabs;
char * Estdin;
char * Eopen;
/* L080 Stop */

char	wdir[PATHSIZE+1];	/* original working directory *//* L058 */

/* Prototypes for voids */					/* L059 { */
void onintr( int sig), onhup( int sig), onquit( int sig);
void getdir( void), passtape( void);
void putfile( char *, char *, int);
void splitfile( char *, int);
void xsfile( int), putempty( register long), getempty( register long);
void seekdisk( long), initarg( char **, char *);
void docompr( char *, int, long, char *), do_trunc( char *);	/* L059 } */

								/* L079 Begin */
int tbird_flag = 0;
int pname_flag = 0;
int ustar_flag = 0;
char posix_name[257];
#define SYMTYPE '2'
#define DIRTYPE '5'
#define TMAGIC "ustar"
								/* L079 end */

main(argc, argv)
int	argc;
char	*argv[];
{
	char *cp;
	char *tmpi;			/* L080 */
	char *tmpa;			/* L080 */

#if 0	/* no longer needed, have prototypes above */		/* L059 */
	int onintr(), onquit(), onhup() /* , onterm() */;
#endif								/* L059 */
	struct stat statinfo;		/* M030 */
	int i;				/* S041 */
	int fDigit = 0;			/* L072 */
	int fflag = 0;			/* L072 */
	char *cp_saved;			/* L072 */

#ifdef INTL							/* L080 Start */
        setlocale(LC_ALL,"");
        catd = catopen(MF_TAR,MC_FLAGS);
#endif /* INTL */

	tmpi= MSGSTR(TAR_MSG_IGNORE, "i");
	IGNORE = tmpi[0];

	tmpa = MSGSTR(TAR_MSG_ABORT, "a");
	ABORT = tmpa[0];


/* Compile the ERE for the positive response */
#ifdef	INTL
	{
		int status;
		char *yesexpr;

		if ((yesexpr = nl_langinfo((nl_item) YESEXPR)) == NULL) {
			errorl(MSGSTR(TAR_MSG_YESEXPR, "unable to find language information for YESEXPR"));
			yesexpr = DEFAULT_YES;
		}

		if (status = regcomp(&pregy, yesexpr, REG_EXTENDED | REG_NOSUB)) {
			char errbuf[100];

			regerror(status, &pregy, errbuf, sizeof(errbuf));
			errorl(errbuf);
			exit(2);
		}
	}
#endif	/* INTL */


	Enoabs = MSGSTR(TAR_MSG_SUPP_ABS_PATH, "tar: suppressing absolute pathnames\n");
	Estdin = MSGSTR(TAR_MSG_STD_IN_ARC, "cannot read blocked standard input archives");
	Eopen = MSGSTR(TAR_MSG_NO_OPEN, "cannot open: %s");
								/* L080 Stop */
	if (argc < 2)
		usage();

	/* M015 */
	if ((tbuf = (union hblock *) malloc(sizeof(union hblock) * NBLOCK)) == (union hblock *) NULL) {
	/* if ((tbuf = (union hblock *) physalloc(sizeof(union hblock) * NBLOCK)) == (union hblock *) NULL) { */
		fprintf(stderr, MSGSTR(TAR_MSG_NO_ALLOC, "tar: cannot allocate physio buffer\n"));
		exit(ENOMEM);
	}

	tfile = NULL;
	argv[argc] = 0;
	argv++;

	cp_saved = *argv;				/* L077 */
	/*
	 * Set up default values.
	 * Search the option string looking for the first digit or an 'f'.
	 * If you find a digit, use the 'archive#' entry in DEF_FILE.
	 * If 'f' is given, bypass looking in DEF_FILE altogether. 
	 * If no digit or 'f' is given, still look in DEF_FILE but use '0'.
	 */
					/* L072 ... */
	for (cp = *argv; *cp != '\0'; ++cp) {
		if (isdigit(*cp)){
			fDigit++;
			/*
			* Only do this the first time so we can
			* parse device no. from the start - else
			* we wrap round after the first digit
			*/
			if (fDigit == 1)	/* L075	*/
				cp_saved = cp;
		}
		if (*cp == 'f'){ 
			fflag++;
			cp_saved = cp;
		}
	}

	if (fDigit && fflag) {
		fprintf(stderr,
		MSGSTR(TAR_MSG_OPT_F, "tar: Option \"f\" cannot be used with a digit argument!\n"));
		usage();
		done(EINVAL);
	}
	else
		cp = cp_saved;
			
/* 	for (cp = *argv; *cp; ++cp)
 		if (isdigit(*cp) || *cp == 'f')
 			break;
*/					/* ... L072 */
	if (*cp != 'f') {
/*
 *		if (*cp)			 begin S040 
 *			archive[7] = *cp;
 *		else {
 *			archive[7] = '=';
 *			archive[8] = '\0';
 *		}				end S040 
 */
/* begin S041 */		/* Begin SCO_BASE */
	 	if ( *cp ) 
		{   for ( i = ARC_NUM_IDX; i < ARC_END_IDX; ++i )
		    {   if ( isdigit(*cp) )
			    archive[i] = *cp++;
			else
			    break;
		    } 
		    archive[i] = '\0';
		}
/* end S041 */			/* End SCO_BASE */
	    
		if (!(defaults_used = defset(archive))) {
			usefile = NULL;		/* ... M023 M025 */
			nblock = 1;			/* M028 */
			blocklim = 0;
			NotTape = 0;
		}
	}

	for (cp = *argv++; *cp; cp++) 
		switch(*cp) {
		case 'f':
			usefile = *argv++;
			break;
		case 'F':		/* M011 && M029 */
			Filefile = *argv++;
			break;
		case 'c':
			cflag++;
			rflag++;
			break;
		case 'u':
			uflag++;     /* M029 moved code after signals caught */
			rflag++;
			break;
		case 'r':
			rflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'x':
			xflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'm':
			mflag++;
			break;
		case 'o':		/* L071 */
			oflag++;	/* L071 */
			break;		/* L071 */
		case 'p':	/* M008 */
			pflag++;
			break;
		case 'S':
			Sflag++;
			/*FALLTHRU*/
		case 's':	/* M021 */
			sflag++;
			Sumfile = *argv++;
			break;
		case '-':
		case '0':	/* numeric entries used only for defaults */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;
		case 'b':
			bflag++;				/* M023 */
			nblock = bcheck(*argv++);
			break;
		case 'k':
			kflag++;				/* M023 */
			blocklim = kcheck(*argv++);
			break;
		case 'n':	/* M004 not a magtape (instead of 'k') */
			NotTape++;	/* assume non-magtape */
			break;
		case 'l':
			linkerrok++;
			break;
		case 'e':					/* M020 */
			eflag++;				/* M020 */
			break;					/* M020 */
		case 'A':					/* M023 */
			Aflag++;
			break;
		case 'q':					/* M034 */
			qflag++;
			break;
#ifdef CHKSUM					/* S035 begin... */
		case 'z':
			zflag++;
			break;
#endif						/* ...end S035 */
		case 'C':			/* S048 begin */
			Cflag++;
			cflag++;
			rflag++;
			break;			/* S048 end   */
		case 'T':			/* S049 begin */
			Tflag++;
			break;			/* S049 end   */
		case 'X':			/* S052 */
			Xflag++;
			xflag++;
			break;
		case 'L':					/* L058 { */
			Lflag++;
			break;					/* L058 } */
								/* L079 Begin */
		case 'P':
			tbird_flag++;
			break;
								/* L079 end */
		default:
			fprintf(stderr, MSGSTR(TAR_MSG_OPT_UNKNOWN, "tar: %c: unknown option\n"), *cp);
			usage();
		}

	if (cflag && !*argv && !Filefile)	/* M028 */	/* S056 */
		fatal(EINVAL, MSGSTR(TAR_MSG_MISS_FILENAMES, "Missing filenames"));
	if (rflag && !cflag && !NotTape && nblock != 1)
		fatal(EINVAL, MSGSTR(TAR_MSG_BLOCKED, "Blocked tapes cannot be updated"));
	if (usefile == NULL)		/* M025 */
		fatal(EINVAL, MSGSTR(TAR_MSG_NEED_DEV_ARG, "device argument required"));

#if SYS_BLOCK > TBLOCK
	/* M005 if user gave blocksize for non-tape device check integrity */
	if (cflag &&			/* check only needed when writing */
	    NotTape &&
	    stat(usefile, &statinfo) >= 0 &&		/* M030 */
	    ((statinfo.st_mode & S_IFMT) == S_IFCHR) &&
	    (nblock % (SYS_BLOCK / TBLOCK)) != 0)
		fatal(EINVAL, MSGSTR(TAR_MSG_BLK_SZ_MULT, "blocksize must be multiple of %d."), SYS_BLOCK/TBLOCK);
#endif
	/* M021 begin */
	if (sflag) {
	    if (Sflag && !mulvol)
		fatal(EINVAL, MSGSTR(TAR_MSG_OPT_SK, "'S' option requires 'k' option."));
	    if ( !(cflag || xflag || tflag) || ( !cflag && (Filefile != NULL || *argv != NULL)) )
		fprintf(stderr, MSGSTR(TAR_MSG_OPT_S, "tar: warning: 's' option results are predictable only with 'c' option or 'x' or 't' option and 0 'file' arguments\n"));
	    if (strcmp(Sumfile, "-") == 0)
		Sumfp = stdout;
	    else if ((Sumfp = fopen(Sumfile, "w")) == NULL)
		fatal(errno, Eopen, Sumfile);
	    sumpro(&Si);
	}
	/* M021 end */
	/* M023 begin */
	/* M028 removed Aflag & !xflag check */
	if (pflag && oflag) {				/* L071 ... */
		fprintf(stderr, MSGSTR(TAR_MSG_OPT_OP, "tar: Option \"o\" and option \"p\" cannot be used together!\n\n "));
		usage();
		done(EINVAL);
	}					 	/* ... L071 */
		
	if (geteuid() == 0)	/* force owner and perm change if root */
		pflag = !pflag;			/* L073 */
	if (!pflag) {			/* M025 */
		Oumask = umask(0); 	/* get file creation mask */
		umask(Oumask);
	}
	/* M023 end */

#ifdef CHKSUM					/* S035 begin... */
	if (!NotTape)
		zflag++;
#endif						/* ...end S035 */

	if (rflag) {
		if (oflag) { 			/* L071 ... */
			fprintf(stderr,
				MSGSTR(TAR_MSG_OPT_O, " tar: Option \"o\" can only be used during file extraction!\n"));
			usage();
			done(EINVAL);
		} 				/* ... L071 */
		if (cflag && tfile != NULL) {
			usage();
			done(EINVAL);
		}
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			signal(SIGINT, onintr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			signal(SIGHUP, onhup);
		if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
			signal(SIGQUIT, onquit);
/*              if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
 *                      signal(SIGTERM, onterm);
 */
		if (uflag) {
			mktemp(tname);
			if ((tfile = fopen(tname, "w")) == NULL)
				fatal(errno, MSGSTR(TAR_MSG_NO_CREATE_TEMP, "cannot create temporary file (%s)"),
				      tname);
			fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");
		}
		if (strcmp(usefile, "-") == 0) {
			if (cflag == 0)
/* L080 */			fatal(EINVAL, MSGSTR(TAR_MSG_STD_OP_ARC, "can only create standard output archives"));
			mt = dup(1);
			if (nblock != 1)
/* L080 */			fatal(EINVAL, MSGSTR(TAR_MSG_BLKED_OP_ARC, "cannot create blocked standard output archives"));
			vout = stderr;				/* L064 */
			++bflag;    /* M031 */
		}
		else if ((mt = open(usefile, 2)) < 0) {
			if (cflag == 0 || (mt =  creat(usefile, 0666)) < 0)
openerr:
				fatal(errno, Eopen, usefile);
		}
		/* Get inode and device number of output file M012 */
		if ( -1 != fstat(mt,&stbuf)){
			/* if it's a block or char dev 	*/
			if ( S_ISCHR(stbuf.st_mode) || S_ISBLK(stbuf.st_mode ) ) 
				bmode = (fcntl(mt, F_SETBMODE, 1) != -1);/* S082 */
		}
		mt_ino = stbuf.st_ino;
		mt_dev = stbuf.st_dev;
		if (Aflag && vflag)		/* M028 */
			fputs(Enoabs, vout);			/* L064 */
		dorep(argv);
	}
	else if (xflag) {
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			if (nblock != 1)
				fatal(EINVAL, Estdin);
			++bflag;     /* M031 */
		}
		else if ((mt = open(usefile, 0)) < 0)
			goto openerr;

		if ( -1 != fstat(mt,&stbuf2)){				/* S082 */
			/* if it's a block or char dev  */
			if ( S_ISCHR(stbuf2.st_mode) || S_ISBLK(stbuf2.st_mode ) ) 
				bmode = (fcntl(mt, F_SETBMODE, 1) != -1);
		}

		if (Aflag && vflag)			/* M023 */
			fputs(Enoabs, stdout);
		doxtract(argv);
	}
	else if (tflag) {
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			if (nblock != 1)
				fatal(EINVAL, Estdin);
			++bflag;	/* M031 */
		}
		else if ((mt = open(usefile, 0)) < 0)
			goto openerr;

		if ( -1 != fstat(mt,&stbuf2)){				/* S082 */
			/* if it's a block or char dev */
			if ( S_ISCHR(stbuf2.st_mode) || S_ISBLK(stbuf2.st_mode ) ) 
				bmode = (fcntl(mt, F_SETBMODE, 1) != -1);		
		}
		dotable(argv);
	}
	else
		usage();
	/* M021 begin */
	if (sflag) {
		sumepi(&Si);
		sumout(Sumfp, &Si);
		fprintf(Sumfp, "\n");
	}
	/* M021 end */
	done(Errflg);		/* M012 */
}


usage()				/* SP-1 M023 S035 S038 begin... */
{
	char buf[BUFSIZ], *bp;
	int i, printit, max_devs;				/* L063 */

	fputs(MSGSTR(TAR_MSG_USE1, "Usage: tar -{txruc}[0-9vfbkelmnopwAFLTP"), stderr);/* L058 L069*/
#ifdef CHKSUM
	putc('z', stderr);
#endif
	fputs(MSGSTR(TAR_MSG_USE2, "] [tapefile] [blocksize] [tapesize] files...\n"), stderr);

	printit = 0;
	strcpy(buf, MSGSTR(TAR_MSG_HEADER, "\tKey     Device            Block   Size(K)    Tape"));
								/* S046 */

#ifdef	CHKSUM
	strcat(buf, "    Cksum");
#endif
	bp = buf + strlen(buf);
	*bp++ = '\n';
	if ((max_devs=count_entries()) < 0)			/* L063 start */
		done(EBADF);
	for (i = 0; i <= max_devs; ++i) {
		char sec_dig=' '; 
		if (i >= 10) {
#ifdef INTL
			archive[ARC_NUM_IDX] = todigit(i/10);	/* L047 */
			archive[ARC_NUM_IDX+1] = todigit(i%10);	/* L047 */
#else
			archive[ARC_NUM_IDX] = i / 10 + '0';
			archive[ARC_NUM_IDX+1] = i % 10 + '0';
#endif
			archive[ARC_NUM_IDX+2] = '=';		/* S046 */
			sec_dig = archive[ARC_NUM_IDX+1];
		} else {
#ifdef INTL
			archive[ARC_NUM_IDX] = todigit(i);	/* L047 */
#else
			archive[ARC_NUM_IDX] = i + '0';
#endif
			/* archive[ARC_NUM_IDX+1] = '='; */		/* S046 */
		}						/* L063 end */
		if (defset(archive) == FALSE)
			continue;
		blocklim = K(blocklim);				/* L076 */
		bp += sprintf(bp,"\t%c%-6c %-17.17s %-7d %-10ld %-7.7s",/*L063*/
			archive[7], sec_dig, usefile, nblock, blocklim, /*L063*/
			NotTape ? MSGSTR(TAR_MSG_NO, "No") : 
				MSGSTR(TAR_MSG_YES, "Yes"));		/* S046 */
#ifdef	CHKSUM
 		bp += sprintf(bp, " %-7.7s", zflag ? "Yes" : "No");
#endif
		*bp++ = '\n';
		printit = 1;
	}
	*bp = '\0';
	if (printit)
		fputs(buf, stderr);
	done(EINVAL);
}				/* ...end S035 S038 */

/***    dorep - do "replacements"
 *
 *      Dorep is responsible for creating ('c'),  appending ('r')
 *      and updating ('u');
 */

dorep(argv)
char	*argv[];
{
	register char *cp, *cp2, *curarg;
#if 0			/* wdir has been made global */		/* L058 */
	char wdir[NAMSIZ];	/* M013 was 60; upped, parameterized */
#endif								/* L058 */

	if (!cflag) {
		getdir();                       /* read header for next file */
#ifdef CHKSUM				/* S035 begin... */
		checkdata(dblock.dbuf.name);	/* check data checksum */
#endif					/* ...end S035 */
		while (!endtape()) {	     /* M028 changed from a do while */
			passtape();             /* skip the file data */
			if (term)
				done(Errflg);   /* received signal to stop */
			getdir();
#ifdef CHKSUM				/* S035 begin... */
			checkdata(dblock.dbuf.name);	/* check data sum */
#endif					/* ...end S035 */
		}
		backtape();			/* M021 was called by endtape */
#ifdef CHKSUM				/* S035 begin... */
		cursum = datsum;		/* save last read checksum */
		didsum = gotsum;		/* also save its validity */
#endif					/* ...end S035 */
		if (tfile != NULL) {
			char buf[200];

			sprintf(buf, "PATH=/bin:/usr/bin; umask 077; sort +0 -1 +1nr %s -o %s; awk '$1 != prev {print; prev=$1}' %s >%sX; exec mv %sX %s",
				tname, tname, tname, tname, tname, tname);
			fflush(tfile);
			system(buf);
			freopen(tname, "r", tfile);
			fstat(fileno(tfile), &stbuf);
			high = stbuf.st_size;
		}
	}

#ifdef CHKSUM		/* S035 begin... */
	zflag = 1;		/* always checksum files being written */
#endif			/* ...end S035 */
	dumping = 1;	/* M004 */
	getwdir(wdir, sizeof(wdir));				/* S051 */
	if (mulvol) {	/* SP-1 */
		if (nblock && (blocklim%nblock) != 0) 		/* M002 */
			fatal(EINVAL, MSGSTR(TAR_MSG_VOL_SZ, "Volume size not a multiple of block size."));
		blocklim -= 2;			/* M002 - for trailer records */
		if (vflag)
			fprintf( vout, MSGSTR(TAR_MSG_VOL_END, "Volume ends at %luK, blocking factor = %dK\n"), K(blocklim - 1), K(nblock));				/* L064 */
	}
	initarg(argv, Filefile);
	while ((curarg = nextarg()) != NULL && ! term) {
		cp2 = curarg;
		for (cp = curarg; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != curarg) {
			*cp2 = '\0';
			if (chdir(curarg) < 0) {
				fprintf(stderr, MSGSTR(TAR_MSG_NO_CHDIR, "tar: cannot chdir to %s\n"),
						curarg);
				continue;
			}
			*cp2 = '/';
			cp2++;
		}
		putfile(curarg, cp2, LEV0);
		chdir(wdir);
	}
	closevol();	/* SP-1 */
	if (linkerrok == 1)
		for (; ihead != NULL; ihead = ihead->nextp)
			if (ihead->count != 0)
				fprintf(stderr, MSGSTR(TAR_MSG_MISS_LINKS, "tar: Missing links to %s\n"), ihead->pathname);
}



/***    endtape - check for tape at end
 *
 *      endtape checks the entry in dblock.dbuf to see if its the
 *      special EOT entry.  Endtape is usually called after getdir().
 *
 *	endtape used to call backtape; it no longer does, he who
 *	wants it backed up must call backtape himself	M021
 *      RETURNS:        0 if not EOT, tape position unaffected
 *                      1 if     EOT, tape position unaffected
 */

endtape()
{
	if (dblock.dbuf.name[0] == '\0') {	/* null header = EOT */
		/* M021
		backtape();
		*/
		return(1);
	}
	else
		return(0);
}



/***    getdir - get directory entry from tar tape
 *
 *      getdir reads the next tarblock off the tape and cracks
 *      it as a directory.  The checksum must match properly.
 *
 *      If tfile is non-null getdir writes the file name and mod date
 *      to tfile.
 */

void								/* L059 */
getdir()
{
	register struct stat *sp;
	int i;

	readtape( (char *) &dblock);

#ifdef CHKSUM					/* S035 begin... */
	if (gotsum = (dblock.dbuf.datsum[0] != '\0'))
		sscanf(dblock.dbuf.datsum, "%o", &datsum);
#endif						/* ...end S035 */
	if (dblock.dbuf.name[0] == '\0')
		return;
	sp = &stbuf;
	sscanf(dblock.dbuf.mode, "%o", &i);
	sp->st_mode = i;
	sscanf(dblock.dbuf.uid, "%o", &i);
	sp->st_uid = i;
	sscanf(dblock.dbuf.gid, "%o", &i);
	sp->st_gid = i;
	sscanf(dblock.dbuf.size, "%lo", &sp->st_size);
	sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
	sscanf(dblock.dbuf.chksum, "%o", &chksum);
	if (dblock.dbuf.extno[0] != '\0') {	/* M004 split file? */
		sscanf(dblock.dbuf.extno, "%o", &extno);
		sscanf(dblock.dbuf.extotal, "%o", &extotal);
		sscanf(dblock.dbuf.efsize, "%lo", &efsize);
	} else
		extno = 0;	/* M004 tell others file not split */
	if (chksum != checksum()) {
		fprintf(stderr, MSGSTR(TAR_MSG_ERR_CHKSUM, "tar: directory checksum error\n"));
		done(2);
	}
								/* L079 Begin */
	if (strcmp(&dblock.dummy[257], TMAGIC) == 0) {		/* posix */
		ustar_flag = 1;
		/* Check if there is a prefix and use that information */
		if(dblock.dummy[345] != '\0') {
			int len = 0;				
			/* The code place upto 257 chars into buf
				( 155 + '/' + 100 + null ) */
			strncpy(posix_name,&dblock.dummy[345], 155);	
			/* just in case it's not null terminated */
			posix_name[155] = '\0';
			strcat(posix_name,"/");		
			len = strlen(posix_name);	
			strncat(posix_name,dblock.dbuf.name, 100);
			/* just in case it's not null terminated */
			posix_name[len+100] = '\0';
			/* flag new name so we'll use it in future */
			pname_flag = 1;
		} else {
			pname_flag = 0;
		}
	} else {
		pname_flag = ustar_flag = 0;
	}
								/* L079 End */
	if (tfile != NULL)
		fprintf(tfile, "%s %s\n",
			pname_flag?posix_name:dblock.dbuf.name,
			dblock.dbuf.mtime);			/* L079 */
}



/***    passtape - skip over a file on the tape
 *
 *      passtape skips over the next data file on the tape.
 *      The tape directory entry must be in dblock.dbuf.  This
 *      routine just eats the number of blocks computed from the
 *      directory size entry; the tape must be (logically) positioned
 *      right after thee directory info.
 */

void								/* L059 */
passtape()
{
	long blocks;
	char buf[TBLOCK];

	if (dblock.dbuf.linkflag == '1' ||
				dblock.dbuf.linkflag == SYMTYPE || 
				dblock.dbuf.linkflag == DIRTYPE)  /* L079 */
		return;
	blocks = TBLOCKS(stbuf.st_size);

	/* M004 if operating on disk, seek instead of reading */
	if (NotTape && !sflag		/* M021 S035 begin... */
#ifdef CHKSUM
	    && !zflag) {
		cursum = 0;
		didsum = 0;
#else
	   ) {
#endif
		seekdisk(blocks);
	}
	else {
		while (blocks-- > 0) {
			readtape(buf);
#ifdef CHKSUM
			xsum(buf);
#endif
		}
	}						/* ...end S035 */
}

void								/* L059 */
putfile(longname, shortname, lev)
char *longname;
char *shortname;
int lev;	/* M018 */
{
	int infile;
	long blocks;
	char buf[TBLOCK];
	/* Use WorkName as work area for construction pathnames    L062 */
	char WorkName[PATHSIZE];				/* L062 */
	register char *cp, *cp2;
	char *truncname;
	struct dirent *dbuf;
	DIR	*indir;
	long	diroff;
	int i, j;
	int ssize;				/* L079 symlink name length */

	/* M018 */
	if (lev >= MAXLEV) {
		/*
		 * Notice that we have already recursed, so we have already
		 * allocated our frame, so things would in fact work for this
		 * level.  We put the check here rather than before each
		 * recursive call because it is cleaner and less error prone.
		 */
		fprintf(stderr, MSGSTR(TAR_MSG_NESTING, "tar: directory nesting too deep, %s not dumped\n"), longname);
		return;
	}
	if ( Lflag)						/* L058 { */
	{
		/* Following symbolic links ( the Lflag ) then use	*/
		/* stat().  However, this may fail if `shortname' is	*/
		/* a symbolic link that references a non-existent file	*/
		/* Therefore try lstat(), letting the code further	*/
		/* down trapping the error of a symbolic link found.	*/
		if (stat(shortname,&stbuf)<0)
			if(statlstat(shortname,&stbuf)<0)
			{
				(void) fprintf(stderr, MSGSTR(TAR_MSG_NO_STAT, "tar: could not stat %s\n"),longname); /* M025 */
				return;
			}/*if*/
	}
	else
		/* Not following symbolic links */
		if( statlstat(shortname,&stbuf)<0)
		{
			(void) fprintf( stderr, MSGSTR(TAR_MSG_NO_STAT, "tar: could not stat %s\n"), longname);
			return;
		}/*if*/						/* L058 } */

	/* M006 if (((stbuf.st_mode & S_IFMT) & (S_IFREG | S_IFDIR)) == 0) */
 								/* L079 Begin */
 	if ((stbuf.st_mode & S_IFMT) != S_IFREG &&
	   				(stbuf.st_mode & S_IFMT) != S_IFDIR) {
 		if (tbird_flag) {
			fprintf(stderr, MSGSTR(TAR_MSG_NOT_FILE_DIR, "tar: %s is not a file or a directory. Not dumped\n"), longname);
 			return;
 	    } else if ((stbuf.st_mode & S_IFMT) != S_IFLNK)
		{
			fprintf(stderr, MSGSTR(TAR_MSG_NOT_SYMLINK, "tar: %s is not a file, symlink or a directory. Not dumped\n"), longname);
 			return;
		}
 	}

	/*
	 * Check if the input file is the same as the tar file we
	 * are creating	- M012
	 */
	if((mt_ino == stbuf.st_ino) && (mt_dev == stbuf.st_dev)) {
		error(MSGSTR(TAR_MSG_SAME_ARC, "same as archive file: %s"), longname);
		return;
	}

	if (tfile != NULL && checkupdate(longname) == 0) {
		return;
	}
	if (checkw('r', longname) == 0) {
		return;
	}

	if ((stbuf.st_mode & S_IFMT) == S_IFREG) {		/* L062 { */
		if ( ( infile = open( shortname, 0)) < 0)
		{
			error( Eopen, longname);
			return;
		}/*if*/
	}
	/* L079 else */	/* else must be a directory */

	tomodes(&stbuf);

	truncname = longname;
	if (Aflag)		/* M028 */
		while (*truncname == '/')
			++truncname;
	cp2 = truncname;
	for (cp = dblock.dbuf.name, i=0; (*cp++ = *cp2++) && i < NAMSIZ; i++);
	if (i >= NAMSIZ) {
		fprintf(stderr, MSGSTR(TAR_MSG_MAX_PATH,"tar: %s: pathname too long\n"), truncname); /* L068 */
		if ((stbuf.st_mode & S_IFMT) == S_IFREG) 	/* L079 */	
			close(infile);
		return;
	}

	/* L079 start */
	if ((stbuf.st_mode & S_IFMT) == S_IFLNK && !tbird_flag) {	
		if ((ssize = readlink(shortname, WorkName,
                             			sizeof(WorkName) - 1)) < 0) {
			(void) fprintf(stderr, MSGSTR(TAR_READ_LINK,"tar: could not readlink %s\n"),longname); /* M025 */
            		return;
        	}
		WorkName[ssize] = '\0';
		if (ssize >= 100) {
			(void) fprintf(stderr, MSGSTR(TAR_MSG_MAX_PATH,"tar: %s: pathname too long\n"), WorkName);
			return;
		}
		strcpy(dblock.dbuf.linkname, WorkName);
		dblock.dbuf.linkflag = SYMTYPE;
		if (mulvol && tapepos + 1 >= blocklim)
			newvol();
#ifdef CHKSUM
		setsum(dblock.dbuf.datsum);
#endif	
		sprintf(dblock.dbuf.chksum, "%6o", checksum());
		writetape( (char *) &dblock);
		if (vflag) {
			fprintf(vout, MSGSTR(TAR_ISA_SYML2,"a %s symbolic link to %s\n"), truncname,
												dblock.dbuf.linkname);
		}
		return;
	}
	/* L079 end */

	if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {		/* L079 */
		/* L079 start */
		if (!tbird_flag) {
				if (mulvol && tapepos + 1 >= blocklim)
					newvol();
				dblock.dbuf.linkflag = DIRTYPE;
#ifdef CHKSUM
				setsum(dblock.dbuf.datsum);
#endif						/* ...end S035 */
				sprintf(dblock.dbuf.chksum, "%6o", checksum());
				writetape( (char *) &dblock);
		}
		/* L079 end */

		if ((indir = opendir(shortname)) == NULL) {
			fprintf(stderr,MSGSTR(TAR_MSG_NO_OPEN_DIR, "Unable to open directory: %s\n"),
				longname);
			return;
		}
                if ( chdir( shortname) < 0)                     /* L064 { */
                {
                        (void) fprintf( stderr, MSGSTR(TAR_MSG_NO_SEARCH, "tar: Cannot search directory %s"), longname);
                        if ( errno == EACCES)
                                (void) fprintf( stderr, MSGSTR(TAR_MSG_NO_PERMIT, ": Permission denied\n"));
                        else
                                (void) fprintf( stderr, "\n");
			errno = 0;
			closedir( indir);
			return;
                }/*if*/                                         /* L064 } */

		for (i = 0, cp = WorkName; *cp++ = longname[i++];);/* L062 */
		*--cp = '/';
		cp++;

		while (((dbuf = readdir(indir)) != NULL) && !term) {
					/* Deleted 2 lines - S055 */
			if (strcmp(".", dbuf->d_name) == 0 || 
			   strcmp("..", dbuf->d_name) == 0) {
				continue;
			}
			cp2 = cp;
			for (j=0; dbuf->d_name[j]; j++)
				*cp2++ = dbuf->d_name[j];
			*cp2 = '\0';
			diroff = telldir(indir);
			closedir(indir);
			putfile(WorkName, cp, lev + 1);		/* L062 */
			if ((indir = opendir(".")) == NULL) {
				fprintf(stderr,MSGSTR(TAR_MSG_NO_REOPEN, "Unable to reopen directory: %s\n"),
					longname);
				return;
			}
			seekdir(indir, diroff);	
		}
		closedir(indir);
								/* L058 { */
		/* Now need to move back up tree to previous directory	      */
		/* Before symbolic links this was simply chdir(".."),	      */
		/* but now that is not enough ( could have followed a	      */
		/* symbolic link to this directory ).			      */
		strcpy( buf, longname);
		if ((cp = strrchr( buf, '/')) == buf)
			chdir( "/");
		else
		{
			chdir(wdir);
			if ( cp != (char *) 0)
			{
				*cp = '\0';
				if ( chdir( buf) == -1) {
					(void) fprintf( stderr, MSGSTR(TAR_MSG_BAD_TREE, "tar: bad directory tree\n"));
					exit( 1);
				}/*if*/
			}/*if*/
		}/*if*/						/* L058 } */
		return;
	}

	if (stbuf.st_nlink > 1) {
		struct linkbuf *lp;
		int found = 0;

		for (lp = ihead; lp != NULL; lp = lp->nextp) {
			if (lp->inum == stbuf.st_ino && lp->devnum == stbuf.st_dev) {
				found++;
				break;
			}
		}
		if (found) {			/* S035 begin... */
			if (mulvol && tapepos + 1 >= blocklim)	/* M004 */
				newvol();
			strcpy(dblock.dbuf.linkname, lp->pathname);
			dblock.dbuf.linkflag = '1';
#ifdef CHKSUM
			setsum(dblock.dbuf.datsum);
#endif						/* ...end S035 */
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
			writetape( (char *) &dblock);
			if (vflag) {
				if (NotTape)	/* SP-1 M005 M022 M023 */
								/* L064 { */
					fprintf( vout, MSGSTR(TAR_MSG_SEEK, "seek = %luK\t"), K(tapepos));
			/* L080 - Combined as one message, previously 2 */
				fprintf( vout, MSGSTR(TAR_MSG_LINK, "a %s link to %s\n"), truncname, lp->pathname);
								/* L080 Stop */
								/* L064 } */
			}
			lp->count--;
			close(infile);
			return;
		}
		else {
			lp = (struct linkbuf *) malloc(sizeof(*lp));
			if (lp == NULL) {
				if (freemem) {
					fprintf(stderr, MSGSTR(TAR_MSG_OUT_MEM, "tar: Out of memory. Link information lost\n"));
					freemem = 0;
				}
			}
			else {
				lp->nextp = ihead;
				ihead = lp;
				lp->inum = stbuf.st_ino;
				lp->devnum = stbuf.st_dev;
				lp->count = stbuf.st_nlink - 1;
				strcpy(lp->pathname, longname);
			}
		}
	}

	blocks = TBLOCKS(stbuf.st_size);
	DEBUG("putfile: %s wants %lu blocks\n", longname, blocks);

	/* correctly handle end of volume 	M023 */
	while (mulvol && tapepos + blocks + 1 > blocklim) { /* file won't fit */
		if (eflag) {
			if (blocks <= blocklim) {
				newvol();
				break;
			}
			error(MSGSTR(TAR_MSG_MAX_FILE_SZ, "file too large to fit on volume: %s"), longname);
			done(ENOSPC);
		}
		/* split only if floppy has some room and file is large */
	    	if (blocklim - tapepos >= EXTMIN && blocks + 1 >= blocklim/10) {
			splitfile(longname, infile);
			return;
		}
		newvol();	/* not worth it--just get new volume */
	}

	if (vflag) {
		if (NotTape)		/* SP-1 M022 M023 */
			fprintf( vout, MSGSTR(TAR_MSG_SEEK, "seek = %luK\t"), K(tapepos)); /* SP-1 M005 */	/* L064 */
		fprintf( vout, MSGSTR(TAR_MSG_A, "a %s "), truncname);		/* L064 */
		if (NotTape)		/* M022 M023 */
			fprintf( vout, "%luK\n", K(blocks));	/* L064 */
		else
			fprintf( vout, MSGSTR(TAR_MSG_TAPE_BLKS, "%lu tape blocks\n"), blocks); /* L064 */
	}
#ifdef CHKSUM				/* S035 begin... */
	setsum(dblock.dbuf.datsum);
#endif					/* ...end S035 */
	sprintf(dblock.dbuf.chksum, "%6o", checksum());
	writetape( (char *) &dblock);

	while ((i = read(infile, buf, TBLOCK)) > 0 && blocks > 0) {
		/* Start M019 */
		if (term) {
			fprintf(stderr, 
				MSGSTR(TAR_MSG_INTERRUPT, "tar: Interrupted in the middle of a file\n"));
			done(Errflg);
		}
		/* End M019 */
		while (i < TBLOCK)				/* L057 */
			buf[i++] = '\0';			/* L057 */
		writetape(buf);
#ifdef CHKSUM				/* S035 begin... */
		xsum(buf);
#endif					/* ...end S035 */
		blocks--;
	}
	close(infile);
	if (blocks != 0 || i != 0)
		fprintf( stderr, MSGSTR(TAR_MSG_CHANGE_SZ, "%s: file changed size\n"), longname);/* L064 */
	putempty(blocks);
}

/***	splitfile	dump a large file across volumes	M004
 *
 *	splitfile(longname, ifd);
 *		char *longname;		full name of file
 *		int ifd;		input file descriptor
 *
 *	NOTE:  only called by putfile() to dump a large file.
 */
void								/* L059 */
splitfile(longname, ifd)
char *longname;
int ifd;
{
	long blocks, bytes, s;
	char buf[TBLOCK];
	register i, extents;

	blocks = TBLOCKS(stbuf.st_size);	/* blocks file needs */

	/* # extents =
	 *	size of file after using up rest of this floppy
	 *		blocks - (blocklim - tapepos) + 1	(for header)
	 *	plus roundup value before divide by blocklim-1
	 *		+ (blocklim - 1) - 1
	 *	all divided by blocklim-1 (one block for each header).
	 * this gives
	 *	(blocks - blocklim + tapepos + 1 + blocklim - 2)/(blocklim-1)
	 * which reduces to the expression used.
	 * one is added to account for this first extent.
	 */
	extents = (blocks + tapepos - 1L)/(blocklim - 1L) + 1;

	if (extents < 2 || extents > MAXEXT) {	/* let's be reasonable */
		fprintf(stderr, MSGSTR(TAR_MSG_NO_VOLS, "tar: %s needs unusual number of volumes to split\ntar: %s not dumped\n"), longname, longname);
		return;
	}
	sprintf(dblock.dbuf.extotal, "%o", extents);	/* # extents */
	bytes = stbuf.st_size;
	sprintf(dblock.dbuf.efsize, "%lo", bytes);

	fprintf(stderr, MSGSTR(TAR_MSG_EXTENTS, "tar: large file %s needs %d extents.\ntar: current device seek position = %luK\n"), longname, extents, K(tapepos));

	s = (blocklim - tapepos - 1) * TBLOCK;
	for (i = 1; i <= extents; i++) {
		if (i > 1) {
			newvol();
			if (i == extents)
				s = bytes;	/* last ext. gets true bytes */
			else
				s = (blocklim - 1)*TBLOCK; /* whole volume */
		}
		bytes -= s;
		blocks = TBLOCKS(s);

		sprintf(dblock.dbuf.size, "%lo", s);
		sprintf(dblock.dbuf.extno, "%o", i);
#ifdef CHKSUM					/* S035 begin... */
		setsum(dblock.dbuf.datsum);
#endif						/* ...end S035 */
		sprintf(dblock.dbuf.chksum, "%6o", checksum());
		writetape( (char *) &dblock);

		if (vflag)
			printf(MSGSTR(TAR_MSG_EXTENT, "+++ a %s %luK [extent #%d of %d]\n"),
				longname, K(blocks), i, extents);
		while (blocks > 0 && read(ifd, buf, TBLOCK) > 0) {
			blocks--;
			writetape(buf);
#ifdef CHKSUM					/* S035 begin... */
			xsum(buf);
#endif						/* ...end S035 */
		}
		if (blocks != 0) {
			fprintf(stderr, MSGSTR(TAR_MSG_CHANGE_SZ2, "tar: %s: file changed size\n"), longname);
			fprintf(stderr, MSGSTR(TAR_MSG_ABORT_SPLIT, "tar: aborting split file %s\n"), longname);
			close(ifd);
			return;
		}
	}
	close(ifd);
	if (vflag)
		printf(MSGSTR(TAR_MSG_EXTENTS2, "a %s %luK (in %d extents)\n"),
			longname, K(TBLOCKS(stbuf.st_size)), extents);
}

doxtract(argv)
char	*argv[];
{
	struct	stat	xtractbuf;	/* stat on file after extracting */
					/* M016 */
	long blocks, bytes;
	char *curarg;
	int ofile;
	int uncompress;			/* Uncompress this file?	L066 */
	int newfile;			/* Does the file already exist  M016 */
	int xcnt;			/* count # files extracted	M012 */
	int fcnt;			/* count # files in argv list	M012 */
	char *namep, *linkp;		/* for removing absolute paths	M023 */

	dumping = 0;	/* M004 for newvol(), et al:  we are not writing */

	/*
	 * Count the number of files that are to be extracted M012
	 */
	fcnt = xcnt = 0;
	initarg(argv, Filefile);
	while (nextarg() != NULL)
		++fcnt;

	for (;;) {
		if (qflag && xcnt == fcnt)	/* M034 */
			break;
		getdir();
#ifdef CHKSUM					/* S035 begin... */
		checkdata(dblock.dbuf.name);
#endif						/* ...end S035 */
		initarg(argv, Filefile);
		if (endtape())
			break;

		namep = pname_flag?posix_name:dblock.dbuf.name;	/* M023  L079 */
		if (Aflag)				/* M028 */
			while (*namep == '/')  /* step past leading slashes */
				namep++;

		if ((curarg = nextarg()) == NULL)
			goto gotit;

		for ( ; curarg != NULL; curarg = nextarg())
			if (prefix(curarg, namep))
				goto gotit;
		passtape();
		continue;

gotit:
		if (checkw('x', namep) == 0) {
			passtape();
			continue;
		}

		if (Tflag) do_trunc(namep);			/* S049 L061 */

		checkdir(namep);

		uncompress = (dblock.dbuf.cpressed == '1' && !Xflag &&
					!ustar_flag); /* L066  L079 */
		if (uncompress) Cflag=1;		    /* S048 S052 L066 */
		if (dblock.dbuf.linkflag == '1') {
			linkp = dblock.dbuf.linkname;
			if (Tflag) do_trunc(linkp);		/* S049 */
			if (Aflag && *linkp == '/')
				linkp++;
			if(uncompress) {		/* Begin S048 L066 */
				docompr(linkp, 0, 0, namep);
				xcnt++;
				continue;
			}				/* End S048 */
			unlink(namep);
			if (link(linkp, namep) < 0) {
				fprintf(stderr, MSGSTR(TAR_MSG_NO_LINK, "tar: %s: cannot link\n"),namep);
				continue;
			}
			if (vflag)
				printf(MSGSTR(TAR_MSG_LINKED, "%s linked to %s\n"), namep, linkp);
			xcnt++;		/* increment # files extracted M025 */
			continue;
		} else if  (dblock.dbuf.linkflag == SYMTYPE) { 	/* L079 begin */
			linkp = dblock.dbuf.linkname;
			if (Tflag) do_trunc(linkp);		/* S049 */
			if (Aflag && *linkp == '/')
				linkp++;
			unlink(namep);
			if (symlink(linkp, namep) < 0) {
				fprintf(stderr, MSGSTR(TAR_NO_SYML,"tar: %s: cannot symlink\n"),namep);
				continue;
			}
			if (vflag)
				printf(MSGSTR(TAR_ISA_SYML,"x %s symbolic linked to %s\n"), namep, linkp);
			xcnt++;		/* increment # files extracted M025 */
			continue; /* Can't chown()/chmod() a symbolic link */
		} else if  (dblock.dbuf.linkflag == DIRTYPE) {
			newfile = ((stat(namep, &xtractbuf) == -1)?TRUE:FALSE); /* S081 */
			/* if not directory, try to delete it */
			if (!newfile) {
				if ((xtractbuf.st_mode & S_IFMT) != S_IFDIR) {
					/* mkdir below will catch error */
					unlink(namep);
					newfile = TRUE;
				}
			}
			if (newfile && mkdir(namep, stbuf.st_mode & MODEMASK) < 0) {
				fprintf(stderr, MSGSTR(TAR_NO_DIR,"tar: %s - cannot create directory\n"), namep);
				continue;
			}
			if (mflag == 0) {
				time_t timep[2];

				timep[0] = time((long *) 0);
				timep[1] = stbuf.st_mtime;
				utime(namep, timep);
			}
			if (pflag) {
				chmod(namep, stbuf.st_mode & MODEMASK);	   /* L064 */
				chown(namep, stbuf.st_uid, stbuf.st_gid);  /* L064 */
			}
			xcnt++;		/* increment # files extracted M025 */
			continue;
		} /* L079 end */
		newfile = ((statlstat(namep, &xtractbuf) == -1)?TRUE:FALSE);/*L058*/
		if (uncompress) unlink(namep);			/* S053 L066 */
		if ((ofile = creat(namep, stbuf.st_mode & MODEMASK)) < 0) {
			fprintf(stderr, MSGSTR(TAR_MSG_NO_CREATE, "tar: %s - cannot create\n"), namep);
			passtape();
			continue;
		}

#ifdef CHKSUM					/* S035 begin... */
		zflag++;
#endif						/* ...end S035 */
		if (extno != 0)	{	/* file is in pieces M004 */
			if (extotal < 1 || extotal > MAXEXT)
				fprintf(stderr, MSGSTR(TAR_MSG_BAD_EXTENT, "tar: ignoring bad extent info for %s\n"), namep);
			else {
				xsfile(ofile);	/* M004 extract it */
				goto filedone;
			}
		}
		extno = 0;	/* let everyone know file is not split */
		blocks = TBLOCKS(bytes = stbuf.st_size);
		if (vflag) {
			printf(MSGSTR(TAR_MSG_BYTES, "x %s, %lu bytes, "), namep, bytes);
			if (NotTape)		/* M022 M023 */
				printf("%luK\n", K(blocks));
			else
				printf(MSGSTR(TAR_MSG_TAPE_BLKS, "%lu tape blocks\n"), blocks);
		}

		xblocks(bytes, ofile);
filedone:
#ifdef CHKSUM					/* S035 begin... */
		zflag--;
#endif						/* ...end S035 */
		if (mflag == 0) {
			time_t timep[2];

			timep[0] = time((long *) 0);
			timep[1] = stbuf.st_mtime;
			utime(namep, timep);
		}
		close( ofile);					/* L064 */
		/* M023 moved this code from above */
		if (pflag) {
			chmod(namep, stbuf.st_mode & MODEMASK);	   /* L064 */
			chown(namep, stbuf.st_uid, stbuf.st_gid);  /* L064 */
		}
		if ( stat(namep, &xtractbuf) == -1)		/* L064 */
			fprintf(stderr, MSGSTR(TAR_MSG_NO_STAT2, "tar: cannot stat extracted file\n"));
		if (newfile == TRUE && (xtractbuf.st_mode & MODEMASK) 
				!= ((stbuf.st_mode & ~Oumask) & MODEMASK))
			fprintf(stderr, MSGSTR(TAR_MSG_FILE_PERMIT, "tar: warning - file permissions have changed for %s\n"), namep);
		/* M023 end */
		xcnt++;			/* increment # files extracted M012 */
		if (uncompress)					/* S048 L066 */
		  	docompr(namep, xtractbuf.st_mode, bytes, "");
	}
	if (Cflag) 			/* S048 Signal compress were done */ 
		docompr("SENDaNULL", 0, 0, "");

	if (sflag) {		/* M021 */
		getempty(1L);	/* don't forget extra EOT *//* M021 */
	}

	/*
	 * Check if the number of files extracted is different from the
	 * number of files listed on the command line M012
	 */
	if (fcnt > xcnt ) {
		fprintf(stderr, MSGSTR(TAR_MSG_NO_EXTRACT, "tar: %d file(s) not extracted\n"),fcnt-xcnt);
		Errflg = 1;
	}
}

/***	xblocks		extract file/extent from tape to output file	M004
 *
 *	xblocks(bytes, ofile);
 *		long bytes;	size of extent or file to be extracted
 *
 *	called by doxtract() and xsfile()
 */
xblocks(bytes, ofile)
long bytes;
int ofile;
{
	long blocks;
	char buf[TBLOCK];
	int nwr;				/* S051 */

	blocks = TBLOCKS(bytes);
	while (blocks-- > 0) {
		readtape(buf);
#ifdef CHKSUM					/* S035 begin... */
		xsum(buf);
#endif						/* ...end S035 */
		if (bytes > TBLOCK)		/* S051 begin... */
			nwr = write(ofile, buf, TBLOCK);
		else
			nwr = write(ofile, buf, (int) bytes);
		if (nwr < 0)
			fatal(errno,
				MSGSTR(TAR_MSG_WRT_ERR, "%s: HELP - extract write error"),
				dblock.dbuf.name
			);			/* ...end S051 */
		bytes -= TBLOCK;
	}
}



/***	xsfile	extract split file			M004
 *
 *	xsfile(ofd);	ofd = output file descriptor
 *
 *	file extracted and put in ofd via xblocks()
 *
 *	NOTE:  only called by doxtract() to extract one large file
 */

static	union	hblock	savedblock;	/* to ensure same file across volumes */

void								/* L059 */
xsfile(ofd)
int ofd;
{
	register i, c;
	char name[NAMSIZ];	/* holds name for diagnostics */
	int extents, totalext;
	long bytes, totalbytes;

	strncpy(name, dblock.dbuf.name, NAMSIZ); /* so we don't lose it */
	totalbytes = 0L;	/* in case we read in half the file */
	totalext = 0;		/* these keep count */

	fprintf(stderr, MSGSTR(TAR_MSG_SPLIT, "tar: %s split across %d volumes\n"), name, extotal);

	/* make sure we do extractions in order */
	if (extno != 1) {	/* starting in middle of file? */
		printf(MSGSTR(TAR_MSG_EXTENT_READ, "tar: first extent read is not #1\nOK to read file beginning with extent #%d (y/n) ? "), extno);
		if (response() != 'y') {
canit:
			passtape();
			close(ofd);
			return;
		}
	}
	extents = extotal;
	for (i = extno; ; ) {
		bytes = stbuf.st_size;
		if (vflag)
			printf(MSGSTR(TAR_MSG_EXTENT2, "+++ x %s [extent #%d], %lu bytes, %luK\n"),
				name, extno, bytes, K(TBLOCKS(bytes)));
		xblocks(bytes, ofd);

		totalbytes += bytes;
		totalext++;
		if (++i > extents)
			break;

		/* get next volume and verify it's the right one */
		copy(&savedblock, &dblock);

		getdir();			/* S035 begin... */
#ifdef CHKSUM
		checkdata(dblock.dbuf.name);
#endif
		if (!endtape()) {
			fprintf(stderr, MSGSTR(TAR_MSG_END_VOL, "tar: misplaced end of volume\n"));
			done(EBADF);
		}				/* ...end S035 */
tryagain:
		newvol();
		getdir();
		if (endtape()) {	/* seemingly empty volume */
			fprintf(stderr, MSGSTR(TAR_MSG_NULL_REC, "tar: first record is null\n"));
asknicely:
			fprintf(stderr, MSGSTR(TAR_MSG_NEED_EXTENT, "tar: need volume with extent #%d of %s\n"), i, name);
			goto tryagain;
		}
		if (notsame()) {
			fprintf(stderr, MSGSTR(TAR_MSG_FIRST_FILE, "tar: first file on that volume is not the same file\n"));
			goto asknicely;
		}
		if (i != extno) {
			fprintf(stderr, MSGSTR(TAR_MSG_ORDER, "tar: extent #%d received out of order\ntar: should be #%d\n"), extno, i);
			fprintf(stderr, MSGSTR(TAR_MSG_ACTION, "Ignore error, Abort this file, or load New volume (i/a/n) ? "));
			c = response();
/*			if (c == 'a')	L080 Replaced below */
			if (c == ABORT)
				goto canit;
/*			if (c != 'i')	L080 Replaced below default to new volume */
			if (c != IGNORE)		/* default to new volume */
				goto asknicely;
			i = extno;		/* okay, start from there */
		}
#ifdef CHKSUM				/* S035 begin... */
		didsum = 1;
		cursum = 0;
#endif					/* ...end S035 */
	}
	bytes = stbuf.st_size;
	if (vflag)
		printf(MSGSTR(TAR_MSG_EXTENT3, "x %s (in %d extents), %lu bytes, %luK\n"),
			name, totalext, totalbytes, K(TBLOCKS(totalbytes)));
}



/***	notsame()	check if extract file extent is invalid		M004
 *
 *	returns true if anything differs between savedblock and dblock
 *	except extno (extent number), checksum, or size (extent size).
 *	Determines if this header belongs to the same file as the one we're
 *	extracting.
 *
 *	NOTE:	though rather bulky, it is only called once per file
 *		extension, and it can withstand changes in the definition
 *		of the header structure.
 *
 *	WARNING:	this routine is local to xsfile() above
 */
notsame()
{
	return(
	    strncmp(savedblock.dbuf.name, dblock.dbuf.name, NAMSIZ)
	    || strcmp(savedblock.dbuf.mode, dblock.dbuf.mode)
	    || strcmp(savedblock.dbuf.uid, dblock.dbuf.uid)
	    || strcmp(savedblock.dbuf.gid, dblock.dbuf.gid)
	    || strcmp(savedblock.dbuf.mtime, dblock.dbuf.mtime)
	    || savedblock.dbuf.linkflag != dblock.dbuf.linkflag
	    || strncmp(savedblock.dbuf.linkname, dblock.dbuf.linkname, NAMSIZ)
	    || strcmp(savedblock.dbuf.extotal, dblock.dbuf.extotal)
	    || strcmp(savedblock.dbuf.efsize, dblock.dbuf.efsize)
	);
}


dotable(argv)
char	*argv[];
{
	char *curarg;
	char *truncname;					/* L078 */
	int tcnt;			/* count # files tabled       M023 */
	int fcnt;			/* count # files in argv list M023 */

	dumping = 0;	/* M004 */

	/* M005 if not on magtape, maximize seek speed */
	if (NotTape && !bflag) {		/* M031 */
#if SYS_BLOCK > TBLOCK
		nblock = SYS_BLOCK / TBLOCK;
#else
		nblock = 1;
#endif
	}
	/*
	 * Count the number of files that are to be tabled	M023
	 */
	fcnt = tcnt = 0;
	initarg(argv, Filefile);
	while (nextarg() != NULL)
		++fcnt;

	for (;;) {
		if (qflag && tcnt == fcnt)	/* M034 */
			break;
		getdir();
#ifdef CHKSUM					/* S035 begin... */
		checkdata(dblock.dbuf.name);
#endif						/* ...end S035 */
		initarg(argv, Filefile);		/* M023 ... */
		if (endtape())
			break;
		if ((curarg = nextarg()) == NULL)
			goto tableit;
		for ( ; curarg != NULL; curarg = nextarg())
			if (prefix(curarg, pname_flag?posix_name:dblock.dbuf.name)) /* L079 */
				goto tableit;
		passtape();
		continue;
tableit:						/* ... M023 */
		++tcnt;
		/* print entries out if it's not a directory entry (same as
           ptar) */
		if (dblock.dbuf.linkflag != DIRTYPE) {
			if (vflag)
				longt(&stbuf);

			truncname = pname_flag?posix_name:dblock.dbuf.name; /* L078 Begin  L079 */
			if (Aflag)
				while (*truncname == '/')
					++truncname;

			printf("%s", dblock.dbuf.name);

			if (extno != 0) {	/* M004 */
				/* L080 Compound messages removed */
				if (vflag) 
				{
					if (dblock.dbuf.linkflag == '1')
						printf(MSGSTR(TAR_MSG_EXTENT4, "\n [extent #%d of %d] %lu bytes total linked to %s"),
							extno, extotal, efsize, dblock.dbuf.linkname);
					else
					if (dblock.dbuf.linkflag == SYMTYPE)
						printf(MSGSTR(TAR_MSG_EXTENT5, "\n [extent #%d of %d] %lu bytes total symbolic link to %s"),
							dblock.dbuf.linkname);
					else
						printf(MSGSTR(TAR_MSG_EXTENT6, "\n [extent #%d of %d] %lu bytes total"),
							extno, extotal, efsize);
				}
				else
				{
					if (dblock.dbuf.linkflag == '1')
						printf(MSGSTR(TAR_MSG_EXTENT7, " [extent #%d of %d] linked to %s"),
							extno, extotal, dblock.dbuf.linkname);
					else
					if (dblock.dbuf.linkflag == SYMTYPE)
						printf(MSGSTR(TAR_MSG_EXTENT8, "\n [extent #%d of %d] symbolic link to %s"),
							dblock.dbuf.linkname);
					else
						printf(MSGSTR(TAR_MSG_EXTENT9, " [extent #%d of %d]"),
							extno, extotal);
				}
			}
			printf("\n");
		}
		passtape();
	}
	if (sflag) {		/* M021 */
		getempty(1L);	/* don't forget extra EOT *//* M021 */
	}
	/*
	 * Check if the number of files tabled is different from the
	 * number of files listed on the command line		M023
	 */
	if (fcnt > tcnt ) {
		fprintf(stderr, MSGSTR(TAR_MSG_NOT_FOUND, "tar: %d file(s) not found\n"),fcnt-tcnt);
		Errflg = 1;
	}
}

void								/* L059 */
putempty(n)
register long n;		/* M021 new argument 'n' */
{
	char buf[TBLOCK];
	register char *cp;

	for (cp = buf; cp < &buf[TBLOCK]; )
		*cp++ = '\0';
	/* M021 begin */
	while (n-- > 0) {		/* S035 begin... */
		writetape(buf);
#ifdef CHKSUM
		xsum(buf);
#endif
	}				/* ...end S035 */
	/* M021 end */
	return;
}

/* M021 new routine */
void								/* L059 */
getempty(n)
register long n;
{
	char buf[TBLOCK];
	register char *cp;

	if (!sflag)
		return;
	for (cp = buf; cp < &buf[TBLOCK]; )
		*cp++ = '\0';
	while (n-- > 0)
		sumupd(&Si, buf, TBLOCK);
	return;
}

longt(st)
register struct stat *st;
{
#ifdef INTL
	struct tm *localtime();					/* L039 */
	char buffer[128];					/* L039 */
#else
	register char *cp;
	char *ctime();
#endif

	pmode(st);
	fprintf( vout, "%3d/%-3d", st->st_uid, st->st_gid);	/* L064 */
	fprintf( vout, "%7lu", st->st_size);			/* L064 */
#ifdef INTL					/* L039 begin */
	strftime(buffer, sizeof(buffer), "%b %d %H:%M %Y", localtime(&st->st_mtime));
	fprintf( vout, " %s ", buffer);				/* L064 */
#else						/* L039 ..end */
	cp = ctime(&st->st_mtime);
	fprintf( vout, " %-12.12s %-4.4s ", cp+4, cp+20);	/* L064 */
#endif
}
/*
 * print various r,w,x permissions
 */
pmode(st)
struct stat *st;
{
	static int m0[] = {1, S_IREAD>>0, 'r', '-' };
	static int m1[] = {1, S_IWRITE>>0, 'w', '-' };
	static int m2[] = {3, S_ISUID|S_IEXEC, 's', S_IEXEC, 'x', S_ISUID, 'S', '-' };
	static int m3[] = {1, S_IREAD>>3, 'r', '-' };
	static int m4[] = {1, S_IWRITE>>3, 'w', '-' };
	static int m5[] = {3, S_ISGID|(S_IEXEC>>3), 's', S_IEXEC>>3, 'x', S_ISGID, 'S', '-' };
	static int m6[] = {1, S_IREAD>>6, 'r', '-' };
	static int m7[] = {1, S_IWRITE>>6, 'w', '-' };
	static int m8[] = {3, S_ISVTX|(S_IEXEC>>6), 't', S_IEXEC>>6, 'x', S_ISVTX, 'T', '-' };

	static int *m[] = { m0, m1, m2, m3, m4, m5, m6, m7, m8 };
	register int **mp;

	for (mp = &m[0]; mp < &m[sizeof(m)/sizeof(m[0])];)
		oselect(*mp++, st);
}

oselect(pairp, st)
register int *pairp;
struct stat *st;
{
	register int n;

	n = *pairp++;
	while (n-->0)
		if ((st->st_mode & *pairp) == *pairp) {
			pairp++;
			break;
		} else
			pairp += 2;
	fprintf( vout, "%c", *pairp);
}

checkdir(name)
register char *name;
{
    register char *cp;
    struct stat Tstbuf;					/* L057 L067 */
    int i;

    if (*(cp = name) == '/')
	cp++;
    for (; *cp; cp++) {
	if (*cp == '/') {
	    *cp = '\0';
	    /* formerly M033, now S043 */
	    if (statlstat(name, &Tstbuf) < 0) {		/* L057 L058 L067 */
		if ((i = mkdir( name, 0777 )) != 0 )
			fatal(errno, MSGSTR(TAR_MSG_NO_MKDIR, "cannot make directory %s!"), name);
		else if (pflag)			/* M023  L064  L067 */
			chown(name, stbuf.st_uid, stbuf.st_gid); /* L064 L067 */
	    }						/* ...end S051 */
	    *cp = '/';
	}
    }
}

void								/* L059 */
onintr(int sig)
{
	signal(SIGINT, SIG_IGN);
	term++;
}

void								/* L059 */
onquit(int sig)
{
	signal(SIGQUIT, SIG_IGN);
	term++;
}

void								/* L059 */
onhup(int sig)
{
	signal(SIGHUP, SIG_IGN);
	term++;
}

/*	uncomment if you need it
onterm()
{
	signal(SIGTERM, SIG_IGN);
	term++;
}
*/

tomodes(sp)
register struct stat *sp;
{
	register char *cp;

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';
	sprintf(dblock.dbuf.mode, "%6o ", sp->st_mode & MODEMASK);
	sprintf(dblock.dbuf.uid, "%6o ", sp->st_uid);
	sprintf(dblock.dbuf.gid, "%6o ", sp->st_gid);
	if ((stbuf.st_mode & S_IFMT) != S_IFLNK &&
				(stbuf.st_mode & S_IFMT) != S_IFDIR) /* L079 */
		sprintf(dblock.dbuf.size, "%11lo ", sp->st_size);
	else
		sprintf(dblock.dbuf.size, "%11lo ", 0);
	sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
	if (Cflag && sp->st_size != 0 ) { 	/* L079 */
	if ((stbuf.st_mode & S_IFMT) != S_IFLNK &&
				(stbuf.st_mode & S_IFMT) != S_IFDIR) 
			dblock.dbuf.cpressed = '1'; /*S048-52*/
	}
}

checksum()
{
	register i;
	register char *cp;

	for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return(i);
}

#ifdef CHKSUM					/* S035 begin... */
/*
 *  More reliable checksum: V7 sum(1) algorithm.
 */
xsum(cp)
	register char *cp;
{
	register unsigned sum;
	register int n;

	if (zflag) {
		sum = cursum;
		for (n = 0; n < TBLOCK; n++) {
			if (sum & 01)
				sum = (sum >> 1) + 0x8000;
			else
				sum >>= 1;
			sum += *cp++;
			sum &= 0xFFFF;
		}
		cursum = sum;
	}
	else
		didsum = 0;
}

setsum(cp)
	register char *cp;
{
	register int i;

	if (didsum) {
		for (i = 0; i < sizeof(dblock.dbuf.datsum); i++)
			cp[i] = ' ';
		sprintf(cp, "%6o", cursum);
	}
	else {
		for (i = 0; i < sizeof(dblock.dbuf.datsum); i++)
			cp[i] = '\0';
	}
	cursum = 0;
	didsum = 1;
}

checkdata(name)
	char *name;
{
	static char lastfile[NAMSIZ+1];
	
	if (gotsum && didsum && datsum != cursum && *lastfile) {
		fprintf(stderr, MSGSTR(TAR_MSG_ERR_CHKSUM2, "tar: file checksum error: %s\n"), lastfile);
		Errflg = 5;
		badsum++;
	}
	strncpy(lastfile, name, NAMSIZ);
	cursum = 0;
	didsum = 1;
}
#endif						/* ...end S035 */

checkw(c, name)
char *name;
{
	if (wflag) {
		fprintf( vout, "%c ", c);			/* L064 */
		if (vflag)
			longt(&stbuf);
		fprintf( vout, "%s: ", name);			/* L064 */
		if (response() == 'y'){
			return(1);
		}
		return(0);
	}
	return(1);
}

response()
{
/*	register int c;					   L080 */
	char buffer[80];				/* L080 */
	int len;					/* L080 */

	fflush(stdout);
							/* L080 Start */
	if (fgets(buffer, sizeof(buffer), stdin) == NULL)
		return(0);
	len = strlen(buffer);
	if (buffer[len-1] == '\n')
		buffer[len-1] = '\0';

/*	c = getchar();
	if (c != '\n')
		while (getchar() != '\n');
	else c = 'n';				Replaced above  L080 Stop */
#ifdef INTL
	if (regexec(&pregy, buffer, 0, NULL, 0) == 0)		/* L080 Start */
		return('y');
	else
		return(isupper(buffer[0]) ? _tolower(buffer[0]) : buffer[0]);		/* L036 */
#else
	return((buffer[0] >= 'A' && buffer[0] <= 'Z') ? buffer[0] + ('a'-'A') : buffer[0]);
#endif								/* L080 Stop */
}

checkupdate(arg)
char	*arg;
{
	char name[NAMSIZ];	/* M013 was 100; parameterized */
	long	mtime;
	daddr_t seekp;
	daddr_t	lookup();

	rewind(tfile);
	if ((seekp = lookup(arg)) < 0)
		return(1);
	fseek(tfile, seekp, 0);
	fscanf(tfile, "%s %lo", name, &mtime);
	if (stbuf.st_mtime > mtime)
		return(1);
	else
		return(0);
}

/***	newvol	get new floppy (or tape) volume			M004
 *
 *	newvol();		resets tapepos and first to TRUE, prompts for
 *				for new volume, and waits.
 *	if dumping, end-of-file is written onto the tape.
 */

newvol()
{
	register int c;

	if (dumping) {
		DEBUG("newvol called with 'dumping' set\n", 0, 0);
		closevol();
		sync();
		tapepos = 0;
	} else
		first = TRUE;
#ifdef CHKSUM				/* S035 begin... */
	cursum = 0;
	didsum = 0;
#endif					/* ...end S035 */
	close(mt);
	/* M021 begin */
	if (sflag) {
		sumepi(&Si);
		sumout(Sumfp, &Si);
		fprintf(Sumfp, "\n");

		sumpro(&Si);
	}
	/* M021 end */
	fprintf(stderr, MSGSTR(TAR_MSG_NEW_VOL, "tar: \007please insert new volume, then press RETURN."));
	fseek(stdin, 0L, 2);	/* scan over read-ahead */
	while ((c = getchar()) != '\n' && ! term)
		if (c == EOF)
			done(0);
	if (term)
		done(0);
#ifdef LISA					/* M023 ... */
	sleep(3);		/* yecch */
#endif						/* ... M023 */
	mt = strcmp(usefile, "-") == EQUAL  ?  dup(1) : open(usefile, dumping ? UPDATE : 0);
	if (mt < 0)				/* S051... */

/* L080 Compound message removed */
		if (dumping)
			fatal(errno, MSGSTR(TAR_MSG_NO_REOPEN_OUT, "cannot reopen output (%s)"), usefile);
		else
			fatal(errno, MSGSTR(TAR_MSG_NO_REOPEN_IN, "cannot reopen input (%s)"), usefile);


/* L080 Replaced by above
*		fatal(errno, "cannot reopen %s (%s)", dumping ? "output" : "input", usefile);
* L080 End */
}

/*
 * SP-1: Write a trailer portion to close out the current output volume.
 */

closevol()
{
#ifdef CHKSUM				/* S035 begin... */
	stoptape();
	putempty(1L);
#else
	putempty(2L);	/* 2 EOT marks */
#endif					/* ...end S035 */
	/* M021 begin */
	if (mulvol && Sflag) {
		/*
		 * blocklim does not count the 2 EOT marks;
		 * tapepos  does     count the 2 EOT marks;
		 * therefore we need the +2 below.
		 */
		putempty(blocklim + 2L - tapepos);
	}
	/* M021 end */
	/* L065 begin */
	while (recno > 0 && recno < nblock) {
		if (NotTape
#if SYS_BLOCK > TBLOCK
		&& (recno % (SYS_BLOCK / TBLOCK)) == 0
#endif
		) break;
		putempty(1L);
	}
	/* L065 end */
	flushtape();
}

#ifdef CHKSUM					/* S035 begin... */
/*
 *  Write an end-of-archive/volume header with the checksum for
 *  the last (possibly partial) file.
 */
stoptape()
{
	register int i;
	union hblock hdr;

	for (i = 0; i < TBLOCK; i++)
		hdr.dummy[i] = '\0';
	setsum(hdr.dbuf.datsum);
	writetape((char *)&hdr);
}
#endif						/* ...end S035 */

done(n)
{
	unlink(tname);
#ifdef CHKSUM					/* S035 begin... */
	if (badsum)
		fprintf(stderr, MSGSTR(TAR_MSG_ERR_CHKSUM3, "tar: %d checksum errors\n"), badsum);
#endif						/* ...end S035 */
	exit(n);
}

prefix(s1, s2)
register char *s1, *s2;
{
	while (*s1)
		if (*s1++ != *s2++)
			return(0);
	if (*s2)
		return(*s2 == '/');
	return(1);
}

getwdir(s, n)							/* S051 */
char *s;
unsigned n;							/* S051 */
{
	int i;
	int	pipdes[2];
	int	w, status;					/* S045 */
	char *p;						/* S051 */

	pipe(pipdes);
	if ((i = fork()) == 0) {
		int ret;
		close(1);
		dup(pipdes[1]);
		execl("/bin/pwd", "pwd", 0);
		execl("/usr/bin/pwd", "pwd", 0);
		ret = errno;
		fprintf(stderr, MSGSTR(TAR_MSG_PWD_FAIL, "tar: pwd failed!\n"));
		printf("/\n");
		exit(ret);
	}
	if (i == -1)					/* S051 begin... */
		fatal(errno, MSGSTR(TAR_MSG_NO_PROCESS, "No process to get directory name!"));
	while ( (w = wait(&status)) != i && w != -1 )		/* M033 */
		;
	/* S045
	 * If <del> signal is issued by user, the pipe will
	 * be broken. Closing the write end of the pipe will cause 
	 * subsequent read from the pipe to get an EOF so that it will not 
	 * hang indefinitely trying to read something.
	 */
	close(pipdes[1]);					/* S045 */
	if ( w == -1 ) 
		done(EINTR);
	if ( read(pipdes[0], s, n-1) > 0 ) {	/* S045 can read something */
		s[n-1] = '\0';
		p = s;	/* before `p' was not initialised before possible call to fatal below */						/* L059 */
		while(*s != '\0' && *s != '\n')
			s++;
		if (*s != '\n')
			fatal(ENAMETOOLONG, MSGSTR(TAR_MSG_DIR_TOO_LONG, "Directory too long: %s ..."), p);
	}						/* ...end S051 */
	*s = '\0';
	close(pipdes[0]);
}

#define	N	200
int	njab;
daddr_t
lookup(s)
char *s;
{
	register i;
	daddr_t a;

	for(i=0; s[i]; i++)
		if(s[i] == ' ')
			break;
	a = bsrch(s, i, low, high);
	return(a);
}

daddr_t
bsrch(s, n, l, h)
daddr_t l, h;
char *s;
{
	register i, j;
	char b[N];
	daddr_t m, m1;

	njab = 0;

loop:
	if(l >= h)
		return(-1L);
	m = l + (h-l)/2 - N/2;
	if(m < l)
		m = l;
	fseek(tfile, m, 0);
	fread(b, 1, N, tfile);
	njab++;
	for(i=0; i<N; i++) {
		if(b[i] == '\n')
			break;
		m++;
	}
	if(m >= h)
		return(-1L);
	m1 = m;
	j = i;
	for(i++; i<N; i++) {
		m1++;
		if(b[i] == '\n')
			break;
	}
	i = cmp(b+j, s, n);
	if(i < 0) {
		h = m;
		goto loop;
	}
	if(i > 0) {
		l = m1;
		goto loop;
	}
	return(m);
}

cmp(b, s, n)
#ifdef INTL
  register unsigned char *b, *s;		/* M000 */ /* L037 */
#else
  register char *b, *s;		/* M000 */
#endif
{
	register i;

	if(b[0] != '\n')
		exit(EINVAL);
	for(i=0; i<n; i++) {
		if(b[i+1] > s[i])
			return(-1);
		if(b[i+1] < s[i])
			return(1);
	}
	return(b[i+1] == ' '? 0 : -1);
}

/***	seekdisk	seek to next file on archive		M004
 *
 *	called by passtape() only
 *
 *	WARNING: expects "nblock" to be set, that is, readtape() to have
 *		already been called.  Since passtape() is only called
 *		after a file header block has been read (why else would
 *		we skip to next file?), this is currently safe.
 *
 *	M005 changed to guarantee SYS_BLOCK boundary
 */
void								/* L059 */
seekdisk(blocks)
long blocks;
{
	long seekval;
#if SYS_BLOCK > TBLOCK
	/* handle non-multiple of SYS_BLOCK */
	register nxb;	/* # extra blocks */
#endif

	tapepos += blocks;		/* M028 */
	DEBUG("seekdisk(%lu) called\n", blocks, 0);
	if (recno + blocks <= nblock) {
		recno += blocks;
		return;
	}
	if (recno > nblock)
		recno = nblock;
	seekval = blocks - (nblock - recno);
	recno = nblock;	/* so readtape() reads next time through */
#if SYS_BLOCK > TBLOCK
	nxb = (int) (seekval % (long)(SYS_BLOCK / TBLOCK));
	DEBUG("xtrablks=%d seekval=%lu blks\n", nxb, seekval);
	if (nxb && nxb > seekval) /* don't seek--we'll read */
		goto noseek;
	seekval -= (long) nxb;	/* don't seek quite so far */
#endif
	if ( 0 == bmode ) { /* block mode not set  */		/* S082 */
		if (lseek(mt,(long) TBLOCK * seekval,1) == -1L)	/* S051 ... */
			fatal(errno, MSGSTR(TAR_MSG_DEV_SEEK_ERR, "device seek error"));
	} else {
		if (lseek(mt,(long)  seekval,1) == -1L)	/* S051 ... */
			fatal(errno, MSGSTR(TAR_MSG_DEV_SEEK_ERR, "device seek error"));
	}

#if SYS_BLOCK > TBLOCK
	/* read those extra blocks */
noseek:
	if (nxb) {
		DEBUG("reading extra blocks\n",0,0);
	        if(read(mt,(char *)tbuf,TBLOCK*nblock)<0)/* S051 ... */ /*L059*/
			fatal(errno, MSGSTR(TAR_MSG_READ_ERR, "read error while skipping file"));
	    	recno = nxb;	/* so we don't read in next readtape() */
	}
#endif
}

readtape(buffer)
char *buffer;
{
	register int i, j;		/* M000 */

	++tapepos;			/* M028 */
	if (recno >= nblock || first) {
		if (first) {		/* M030 */
			/*
			 * set the number of blocks to
			 * read initially.
			 * very confusing!
			 */
			if (bflag)
				j = nblock;
			else if (!NotTape)
				j = NBLOCK;
			else if (defaults_used)
				j = nblock;
			else
				j = NBLOCK;
		} else
			j = nblock;
		{ /* Begin read block - MAF S042 */	/* Begin SCO_BASE */

			int sz = TBLOCK * j;

			int rt;

			i = 0;
			while ((rt = read(mt,((char *)tbuf)+i,sz)) != sz) {
				if (rt < 0) {
					int ret = errno;   /* S051 begin... */
					error(MSGSTR(TAR_MSG_TAPE_READ_ERR, "tape read error"));
					errno = ret;
					perror(MSGSTR(TAR_MSG_READ, "read"));
					done(ret);	   /* ...end S051 */
				} else if (rt == 0) {
					break;
				}
				i += rt;
				sz -= rt;
			}
			if (rt == (TBLOCK*j))
				i = rt;
			else
				i += rt;
		} /* End read block - MAF S042 */	/* End SCO_BASE */
		if (first) {
			if ((i % TBLOCK) != 0) {
				fprintf(stderr, MSGSTR(TAR_MSG_BLK_SZ_ERR, "tar: tape blocksize error\n"));
				done(EINVAL);
			}
			i /= TBLOCK;

			if ((rflag && !NotTape) && (usefile != (char *)NULL)) {/* L070 ... */
				fprintf(stderr, MSGSTR(TAR_MSG_NO_UPDATE_DEV, "tar: cannot update tape devices. Try option \"n\".\n"));
				done(EINVAL);
			}					 /* ... L070 */
			if (rflag && i != 1 && !NotTape) { 
				fprintf(stderr, MSGSTR(TAR_MSG_BLOCKED2, "tar: Cannot update blocked tapes (yet)\n"));
				done(EINVAL);
			}
			if (vflag && i != nblock && i != 1) {	/* M027 */
				if (NotTape)
					fprintf(stderr, MSGSTR(TAR_MSG_BUF_SZ, "tar: buffer size = %dK\n"), K(i));
				else
					fprintf(stderr, MSGSTR(TAR_MSG_BLK_SZ, "tar: blocksize = %d\n"), i);
			}
			nblock = i;                     /* M003 */
		}
		recno = 0;
	}

	first = FALSE;
	copy(buffer, &tbuf[recno++]);
	if (sflag)				/* M021 */
		sumupd(&Si, buffer, TBLOCK);	/* M021 */
	return(TBLOCK);
}

writetape(buffer)
char *buffer;
{
	tapepos++;	/* M004 output block count */

	first = FALSE;	/* removed setting of nblock if file arg given M023 */
	if (recno >= nblock)
		flushtape();
	copy(&tbuf[recno++], buffer);
	if (recno >= nblock)
		flushtape();
}



/***    backtape - reposition tape after reading soft "EOF" record
 *
 *      Backtape tries to reposition the tape back over the EOF       M009
 *      record.  This is for the -u and -r options so that the        M009
 *      tape can be extended.  This code is not well designed, but    M009
 *      I'm confident that the only callers who care about the        M009
 *      backspace-over-EOF feature are those involved in -u and -r.   M009
 *      Thus, we don't handle it correctly when there is              M009
 *      a block factor, but the -u and -r options refuse to work      M009
 *      on block tapes, anyway.                                       M009
 *                                                                    M009
 *      Note that except for -u and -r, backtape is called as a       M009
 *      (apparently) unwanted side effect of endtape().  Thus,        M009
 *      we don't bitch when the seeks fail on raw devices because     M009
 *      when not using -u and -r tar can be used on raw devices.      M009
 */

backtape()
{
	DEBUG("backtape() called, recno=%d nblock=%d\n", recno, nblock);
	if (nblock > 1 && !NotTape)                             /* M009 */
		return;                                         /* M009 */
	/*
	 * The first seek positions the tape to before the eof;
	 * this currently fails on raw devices.
	 * Thus, we ignore the return value from lseek().
	 * The second seek does the same.
	 */
	if ( 0 ==  bmode ) /* block mode not set  */		/* S082 */
		lseek(mt,(long) -(TBLOCK*nblock), 1);/* back one large tape block */
	else
		lseek(mt,(long) -nblock, 1);/* back one large tape block */

	recno--;				/* reposition over soft eof */
	tapepos--;				/* back up tape position */
	if (read(mt,(char *)tbuf, TBLOCK*nblock) <= 0)	/* S051 ... *//*L059*/
	   	fatal(errno, MSGSTR(TAR_MSG_TAPE_READ_ERR2, "tape read error after seek"));

	if ( 0 ==  bmode ) /* block mode not set  */		/* S082 */
		lseek(mt, (long) -(TBLOCK*nblock), 1);	/* back large block again */
	else
		lseek(mt, (long) -nblock, 1);	/* back large block again */
}



/***    flushtape  write buffered block(s) onto tape       M005
 *
 *      recno points to next free block in tbuf.  If nonzero, a write is done.
 *      Care is taken to write in multiples of SYS_BLOCK when device is
 *      non-magtape in case raw i/o is used.
 *
 *      NOTE:   this is called by writetape() to do the actual writing
 */
flushtape()
{
	
	DEBUG("flushtape() called, recno=%d\n", recno, 0);
	if (recno > 0) {	/* anything buffered? */
		if (NotTape) {
			/* L065: rounding up to a SYS_BLOCK moved outside */
			if (recno > nblock)
				recno = nblock;
		}
		if (write(mt, (char *)tbuf, (NotTape ? recno : nblock) * TBLOCK) < 0)								/* L059 */
			fatal(errno, MSGSTR(TAR_MSG_TAPE_WRT_ERR, "tape write error"));	/* S051 */
		if (sflag)		/* M021 */
			sumupd(&Si, tbuf, (NotTape ? recno : nblock) * TBLOCK);	/* M021 */
		recno = 0;
	}
}

copy(to, from)
register char *to, *from;
{
	register i;

	i = TBLOCK;
	do {
		*to++ = *from++;
	} while (--i);
}

/* M011 new routine */
/***	initarg -- initialize things for nextarg.
 *
 *	argv		filename list, a la argv.
 *	filefile	name of file containing filenames.  Unless doing
 *		a create, seeks must be allowable (e.g. no named pipes).
 *
 *	- if filefile is non-NULL, it will be used first, and argv will
 *	be used when the data in filefile are exhausted.
 *	- otherwise argv will be used.
 */
static char **Cmdargv = NULL;
static FILE *FILEFile = NULL;
static long seekFile = -1;
static char *ptrtoFile, *begofFile, *endofFile; 

void								/* L059 */
initarg(argv, filefile)
char *argv[];
char *filefile;
{
	struct stat statbuf;
	register char *p;
	int nbytes;

	Cmdargv = argv;
	if (filefile == NULL)
		return;		/* no -F file */
	if (FILEFile != NULL) {
		/*
		 * need to REinitialize
		 */
		if (seekFile != -1)
			fseek(FILEFile, seekFile, 0);
		ptrtoFile = begofFile;
		return;
	}
	/*
	 * first time initialization 
	 */
	if ((FILEFile = fopen(filefile, "r")) == NULL)
		fatal(errno, Eopen, filefile);
	fstat(fileno(FILEFile), &statbuf);
	if ((statbuf.st_mode & S_IFMT) != S_IFREG)
		fatal(EINVAL, MSGSTR(TAR_MSG_NOT_REG_FILE, "not a regular file: %s"), filefile);
	ptrtoFile = begofFile = endofFile;
	seekFile = 0;
	if (!xflag)
		return;		/* the file will be read only once anyway */
	nbytes = statbuf.st_size;
	while ((begofFile = malloc(nbytes)) == NULL)
		nbytes -= 20;
	if (nbytes < 50) {
		free(begofFile);
		begofFile = endofFile;
		return;		/* no room so just do plain reads */
	}
	if (fread(begofFile, 1, nbytes, FILEFile) != nbytes)
		fatal(errno, MSGSTR(TAR_MSG_ERR_READ, "error reading %s"), filefile);
	ptrtoFile = begofFile;
	endofFile = begofFile + nbytes;
	for (p = begofFile; p < endofFile; ++p)
		if (*p == '\n')
			*p = '\0';
	if (nbytes != statbuf.st_size)
		seekFile = nbytes + 1;
	else
		fclose(FILEFile);
}

/***	nextarg -- get next argument of arglist.
 *
 *	The argument is taken from wherever is appropriate.
 *
 *	If the 'F file' option has been specified, the argument will be
 *	taken from the file, unless EOF has been reached.
 *	Otherwise the argument will be taken from argv.
 *
 *	WARNING:
 *	  Return value may point to static data, whose contents are over-
 *	  written on each call.
 */
char  *
nextarg()
{
	static char nameFile[LPNMAX];
	int n;
	char *p;

	if (FILEFile) {
		if (ptrtoFile < endofFile) {
			p = ptrtoFile;
			while (*ptrtoFile)
				++ptrtoFile;
			++ptrtoFile;
			return(p);
		}
		if (fgets(nameFile, LPNMAX, FILEFile) != NULL) {
			n = strlen(nameFile);
			if (n > 0 && nameFile[n-1] == '\n')
				nameFile[n-1] = '\0';
			return(nameFile);
		}
	}
	return(*Cmdargv++);
}

/* begin M023 */
/*
 * kcheck()
 *	- checks the validity of size values for non-tape devices
 *	- if size is zero, mulvol tar is disabled and size is
 *	  assumed to be infinite.
 *	- returns volume size in TBLOCKS
 */
long
kcheck(kstr)
char	*kstr;
{
	long kval;

	if ((*kstr < 0x30) || (*kstr > 0x39)) {             /* start S054 */ 
		fprintf(stderr, MSGSTR(TAR_MSG_BAD_ARC_SZ, "tar: invalid archive size.\n"));
		done(EINVAL);
 	} 						    /* end   S054 */ 

	kval = atol(kstr);

	if (kval == 0L) {	/* no multi-volume; size is infinity.  */
		mulvol = 0;	/* definitely not mulvol, but we must  */
		return(0);	/* M028 took out setting of NotTape */
	}

	if (kval < (long) MINSIZE) {
		fprintf(stderr, MSGSTR(TAR_MSG_MIN_SZ, "tar: sizes below %luK not supported (%lu).\n"),
				(long) MINSIZE, kval);
		if (!kflag)
			fprintf(stderr, MSGSTR(TAR_MSG_BAD_SZ, "bad size entry for %s in %s.\n"),
				archive, deffile);
		done(EINVAL);
	}
	mulvol++;
 	NotTape++;				/* implies non-tape */
	return(kval * (1024L / TBLOCK));	/* convert to TBLOCKS  L076 */
}

/*
 * bcheck()
 *	- checks the validity of blocking factors
 *	- returns blocking factor
 */
bcheck(bstr)
char	*bstr;
{
	int bval;

	bval = atoi(bstr);
	if (bval > NBLOCK || bval <= 0) {
		fprintf(stderr, MSGSTR(TAR_MSG_BAD_BLK_SZ, "tar: invalid blocksize. (Max %d)\n"), NBLOCK);
		if (!bflag)
			fprintf(stderr, MSGSTR(TAR_MSG_BAD_BLK_SZ2, "bad blocksize entry for '%s' in %s.\n"),
				archive, deffile);
		done(EINVAL);
	}
	return(bval);
}

/*
 * defset()
 *	- reads DEF_FILE for the set of default values specified.
 *	- initializes 'usefile', 'nblock', and 'blocklim', 'NotTape',
 *		and 'zflag'.					S035
 *	- 'usefile' points to static data, so will be overwritten
 *	  if this routine is called a second time.
 *	- the pattern specified by 'arch' must be followed by four or five
 *	  blank-separated fields (1) device (2) blocking,
 *    	                         (3) size(K), (4) tape,		S035
 *				 and (5) data checksum		S035
 *	  for example: archive0=/dev/fd 1 400 n y		S035
 */
defset(arch)
char	*arch;
{
	char *bp;
	FILE *deffp;

	if ((deffp = defopen(deffile)) == 0)
		return(FALSE);			/* M025 */
	if (defcntl(DC_SETFLAGS, (DC_STD & ~(DC_CASE))) == -1) {
		fprintf(stderr, MSGSTR(TAR_MSG_ERR_SET_PARAM, "tar: error setting parameters for %s.\n"),
				deffile);
		return(FALSE);			/* M028 & following ones too */
	}
	if ((bp = defread(deffp, arch)) == NULL) {
		fprintf(stderr, MSGSTR(TAR_MSG_MISS_ENTRY, "tar: missing or invalid '%s' entry in %s.\n"),
				arch, deffile);
		return(FALSE);
	}
	if ((usefile = strtok(bp, " \t")) == NULL) {
		fprintf(stderr, MSGSTR(TAR_MSG_EMPTY_ENTRY, "tar: '%s' entry in %s is empty!\n"),
				arch, deffile);
		return(FALSE);
	}
	if ((bp = strtok(NULL, " \t")) == NULL) {
		fprintf(stderr, 
			MSGSTR(TAR_MSG_MISS_BLK_COMP, "tar: block component missing in '%s' entry in %s.\n"),
		       	arch, deffile);
		return(FALSE);
	}
	nblock = bcheck(bp);
	if ((bp = strtok(NULL, " \t")) == NULL) {
		fprintf(stderr,
			MSGSTR(TAR_MSG_MISS_SZ_COMP, "tar: size component missing in '%s' entry in %s.\n"),
		       	arch, deffile);
		return(FALSE);
	}
	blocklim = kcheck(bp);
	if ((bp = strtok(NULL, " \t")) != NULL)
		NotTape = (*bp == 'n' || *bp == 'N');
	else 
		NotTape = (blocklim > 0);
#ifdef CHKSUM					/* S035 begin... */
	if ((bp = strtok(NULL, " \t")) != NULL)	
		zflag = (*bp == 'y' || *bp == 'Y');
	else
		zflag = ! NotTape;
#endif						/* ...end S035 */
	defopen(NULL);
	DEBUG("defset: archive='%s'; usefile='%s'\n", arch, usefile);
	DEBUG("defset: nblock='%d'; blocklim='%ld'\n",
	      nblock, blocklim);
	DEBUG("defset: not tape = %d\n", NotTape, 0);
	return(TRUE);				/* M025 */
}

/*  						     S048 begin */
/* docompr()							*/
/* Determine if file needs to be decompressed and send its path */
/* to the child for decompression.				*/

void								/* L059 */
docompr(fname, ftype, fsize ,lname) 
char *fname;	/* Name of file to be uncompressed. */
int ftype;   	/* xstractbuf.st_mode 		    */
long fsize;	/* Size of entire file. 	    */
char *lname;	/* Name of link to be made. 	    */
{
	static int child_pid;		/* Child process id.  		*/
	int status, w; 			/* Status of child process. 	*/
	int i;
	int e;				/* S051 */
	static int apipe[2];		/* Pipe file descriptors. 	*/
	char pipebuf[BUFSIZ];		/* Buffer used to send filename */
		/* Note: Tar sends in <stdio.h> defined BUFSIZ chunks   */
		/* and compress reads in BUFSIZ chunks. 		*/
	static int firstime=1;

	static struct Clinkbuf {	/* Keep track of linked files. 	*/
		char	filename[NAMSIZ];
		char	linkname[NAMSIZ];
		struct	Clinkbuf *nextp;
	} *Clp;
	static struct Clinkbuf *Clphead=NULL;

	/* FIRSTIME: This section of code creates a pipe and	*/
	/*	     forks a child. 				*/
	if (firstime) {
		/* Create pipe */
		if(pipe(apipe) == -1) {
			e = errno; perror(MSGSTR(TAR_MSG_PIPE, "pipe")); exit(e);	/* S051 */
		}
		switch (child_pid = fork()) { 
		case -1:
			/* Error fork failed */
			e = errno; perror(MSGSTR(TAR_MSG_FORK, "fork")); exit(e);	/* S051 */
		case 0:
			/* child */
			close(apipe[1]);	/* Close write end of pipe. */
			dochild(apipe[0]);	/* Pass read end of pipe.   */
			exit(0);
		}
		/* Parent */
		close(apipe[0]);		/* Close read end of pipe.  */
		firstime=0;
	}	

	/* DONE: This section of code signals our child were finished	*/
	/*       sending filenames across the pipe via a NULL and	*/
	/* 	 processes the list of links we have yet to make.	*/

	if ((strcmp(fname, "SENDaNULL") == 0) && ftype == 0) {
		for(i=0; i<(sizeof(pipebuf)); i++) pipebuf[i]=' ';
		pipebuf[0]='\0';
		write(apipe[1], pipebuf, sizeof(pipebuf));
				/* Wait for child to complete decompression. */
		while ((w = wait(&status)) != child_pid && w != -1);
				/* Close pipe. */
		close(apipe[1]);
				/* Process list of links to be made. */
		for (Clp = Clphead; Clp != NULL; Clp = Clp->nextp) {
			unlink(Clp->linkname);
			if (link(Clp->filename, Clp->linkname) < 0) {
			      	error(MSGSTR(TAR_MSG_NO_CREATE_LINK, "%s: cannot create link"), Clp->linkname);
				continue;
			}
			if (vflag) printf(MSGSTR(TAR_MSG_LINKED, "%s linked to %s\n"), 
					Clp->filename, Clp->linkname);
		}
		return;
	}

	/* LINKED FILE: This section of code adds filenames to our list */
	/*		of links to be made after decompression. This	*/
	/* 		is flagged by a non-zero length lname.		*/
	if (strlen(lname) != 0) {
		Clp = (struct Clinkbuf *) malloc(sizeof(*Clp));
		if (Clp == NULL) 
		     	error(MSGSTR(TAR_MSG_FAIL_MALLOC, "malloc failed. Link information lost for %s"),
									lname);
		else {
			Clp->nextp = Clphead;
			Clphead = Clp;
			strcpy(Clp->filename, fname);
			strcpy(Clp->linkname, lname);
		}
		return;
	}

	/* SEND FILE: This section of code sends appropriate files 	*/
	/*	      through the pipe to compress.			*/

	if ((ftype & S_IFMT) == S_IFREG && fsize != 0) {
		/* Initialize pipe buffer */
		for(i=0; i<(sizeof(pipebuf)); i++) pipebuf[i]=' ';
		strcpy(pipebuf, fname);
		write(apipe[1], pipebuf, sizeof(pipebuf));
	}
	return;
}

/*  						     		*/
/* dochild()							*/
/* Exec compress with -dP <rpipe> telling it to decompress      */
/* and overwrite files read from the pipe descriptor. Compress  */
/* will continue reading names from the pipe until a NULL is    */
/* or the pipe is closed. 				        */

dochild(rpipe)
int rpipe;	/* Read end of pipe */
{
	char param[10];
	int e;							/* S051 */

	sprintf(param, "%d", rpipe); 				/* L074 */
	execl("/usr/bin/compress", "compress", "-d", "-P", param, (char*)0);	/* L074 */
	e = errno;						/* S051 */
	perror(MSGSTR(TAR_MSG_EXECL, "execl"));
	exit(e);						/* S051 */
} 						/* S048 end */

						/* S049 begin */
/* Truncate the name to MAX_NAMELEN characters */
/* L061: enhanced to truncate path components as well */
void								/* L059 */
do_trunc(path_ptr)
char *path_ptr;
{
	char tbuf[257]; /* tmp buf to hold truncated path */	/* L061  L079 */
	char *pp, *ppend;
	int n, tind;

	pp = path_ptr;
	ppend = path_ptr + strlen( path_ptr); /* the last char in path */
	tind = 0;
	while ( pp < ppend) {
		n = 0;
		while ( (*pp != '/') && (n<MAX_NAMELEN) && (pp < ppend) ) {
			tbuf[tind++] = *pp++;
			++n;
		}
		if ( n == MAX_NAMELEN ) { /* throw away rest */
			while ( (*pp++ !='/') && (pp < ppend) );
			if ( *(pp-1) == '/')
				tbuf[tind++] = '/';
		}
		else if ( *pp == '/')
			tbuf[tind++] = *pp++;
	}
	tbuf[tind] = '\0';
	strcpy( path_ptr, tbuf); /* return truncated path in original buffer */ 
} 						/* S049 end */

/*VARARGS2*/
fatal(n, s1, s2, s3, s4, s5)
{
	error(s1, s2, s3, s4, s5);		/* S035 begin... */
	done(n);
}
/* end M023 */

/*VARARGS1*/
error(s1, s2, s3, s4, s5)
char *s1;
{
	fprintf(stderr, "tar: ");
	fprintf(stderr, s1, s2, s3, s4, s5);
	fprintf(stderr, "\n");
	Errflg = 1;
}						/* ...end S035 */

count_entries()					/* L063 start */
{
	int i, ret=0;
	char tmp_str[sizeof(archive)];
	struct _FILE_	*fp;

	if ((fp = defopen(deffile)) == 0)
		return(-1);			/* M025 */
	if (defcntl(DC_SETFLAGS, (DC_STD & ~(DC_CASE))) == -1)
		return(-1);
	for (i=0; i<99 ; i++) {
		sprintf(tmp_str,"%s%d",ARC_NAME, i);
		if (defread(fp, tmp_str) == NULL)
			continue;
		if (ret < i)
			ret = i;
	}
	defclose(fp);

	return ret;
}						/* L063 end */
