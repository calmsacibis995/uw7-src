#ident	"@(#)OSRcmds:cpio/utility.c	1.1"
#pragma comment(exestr, "@(#) utility.c 25.10 95/02/16 ")
/***************************************************************************
 *--------------------------------------------------------------------------
 * Utility functions used by cpio.
 *
 *--------------------------------------------------------------------------
 *
 *	Portions Copyright 1983-1995 The Santa Cruz Operation, Inc
 *			All Rights Reserved
 *
 *	Copyright (c) 1984, 1986, 1987, 1988 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 *
 *--------------------------------------------------------------------------
 * Revision History:
 *
 *	L000	01 Sep 1993		scol!ashleyb
 *	  - Module created in an attempt to clean up code.
 *	L001	17jan94			scol!hughd
 *	  - no need to free buffers prior to exit(), and a bad idea
 *	    if we've changed the alignment from what malloc() gave
 *	L002	11 Feb 1994		scol!ashleyb
 *	  - Allow INTL to be undefined, to make Nfloppy version of cpio.
 *	L003	08 Jun 1994		scol!ashleyb
 *	  - Renamed truncate function to avoid clash with truncate() in
 *	    libsocket.
 *	  - Removed old Mod markers.
 *	L004	05 Jul 1994		scol!ianw
 *	  - Added #include of errno.h, necessary now the default stdio.h no
 *	    longer #includes it.
 *	L005	06 Jul 1994		scol!ashleyb
 *	  - Format usage message to 80 columns.
 *	  - Use errorl and psyserrorl to report errors (unmarked).
 *	  - Added a block calculator that tries not to overflow.
 *	L006	07 Sep 1994		scol!ashleyb
 *	  - Generate our own inode/device numbers for old archive types.
 *	  - Handle the header options here.
 *	L007	08 Nov 1994 		scol!trevorh
 *	 - message catalogued.
 *	L008	06 Feb 1995		scol!mwatts
 *	 - use libprot routine pw_idtoname rather than the slower library
 *	   function getpwnam.
 *
 *==========================================================================
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/mkdev.h>
#include <stdio.h>
#include <errno.h>						/* L004 */
#include <string.h>
#include <unistd.h>
#include <varargs.h>
/* #include <prot.h> */						/* L008 */
#include <pwd.h>

/* #include <errormsg.h> */					/* L007 Start */
#ifdef INTL
#  include <locale.h>
#  include "cpio_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str)	(str)
#  define catclose(d)		/*void*/
#endif /* INTL */						/* L007 Stop */

#define _UTILITY_C
#include "../include/osr.h"
#include "errmsg.h"
#include "cpio.h"

/* --------------- Global Variables. --------------- */
extern header Hdr;

extern char **Pattern;
extern char *command_name;

extern short	fflag,
		Mod_time,
		Aflag,
		Option,
		Verbose,
		Dir,
		Rename;

extern ushort	A_directory;

extern long	Longfile,
		Longtime;

extern FILE	*Rtty,
		*Wtty;

extern buf_info	buf;
extern mode_t	orig_umask;
extern struct stat	Xstatb;

/* --------------- Static Variables. --------------- */

static int      verbcount = 0;
static FILE     *verbout = 0;

/* --------------- Utility Functions. --------------- */

void cleanup(int exitcode)
{
								/* L001 */
	exit(exitcode);
}

void resetverbcount()
{
	if( Verbose == 2  &&  verbcount ) {
		fputc( '\n', verbout );
		fflush(verbout);
		verbcount = 0;
	}
}

ulong crcsum(int file,ulong filesize)
{
	static char		buffer[CPIOBSZ];
	register ulong		crc = 0;
	register char		*ptr, *end;

	lseek(file,0,SEEK_SET);
	while(filesize){
		int amount;

		amount = filesize > CPIOBSZ ? CPIOBSZ : filesize;
		amount = read(file,buffer,amount);

		if (amount < 0) {
			psyserrorl(errno,MSGSTR(UTIL_MSG_READ_ERR, "read error while calculating checksum"));
			return(0);
		}
		ptr = buffer;
		end = buffer + amount;
		while (	ptr < end)
		{
			crc += (*ptr & 0xff);
			ptr++;
		}
		filesize -= amount;
	}
	lseek(file,0,SEEK_SET);
	return(crc);
}

ushort sum(char *buffer,int size,register ushort lastsum)
{
	register char	*c	= buffer;
	register char	*end	= buffer + size;

	while (c < end)
	{
		if(lastsum & 01)
			lastsum = (lastsum >> 1) + HIGHBIT;
		else
			lastsum >>= 1;

		lastsum += (*c & 0xFF);
		c++;
	}
	return(lastsum);
}

void usage()
{								/* L005 Begin */

	fprintf(stderr,MSGSTR(UTIL_MSG_USE1, "Usage:\t%s -o[acvVBL] [-Kvolumesize] [-Cbufsize] [-Hhdr]\n\t\t\t[-Mmessage] <name-list >collection\n"), command_name);

	fprintf(stderr,MSGSTR(UTIL_MSG_USE2, "\t%s -o[acvVBL] -Ocollection [-Kvolumesize] [-Cbufsize] [-Hhdr]\n\t\t\t[-Mmessage] <name-list\n"), command_name);

	fprintf(stderr,MSGSTR(UTIL_MSG_USE3, "\t%s -i[AbcdkmnrsStTuvVfB6] [-Cbufsize] [-Mmessage]\n\t\t\t[pattern ...] <collection\n"), command_name);

	fprintf(stderr,MSGSTR(UTIL_MSG_USE4, "\t%s -i[AbcdkmnrsStTuvVfB6] -Icollection [-Cbufsize] [-Mmessage]\n\t\t\t[pattern ...]\n"), command_name);

	fprintf(stderr,MSGSTR(UTIL_MSG_USE5, "\t%s -p[adlmuvVL] directory <name-list\n"),command_name);

	exit(255);
								/* L005 End */
}

void skipln(char *p, FILE *fp)
{
	int c;

	if (p != (char *)0 && (c = strlen(p)) >= 1) {
		if (p[--c] == '\n')
			p[c] = '\0';
		else
			while ((c = getc(fp)) != '\n' && c != EOF)
				continue;
	}
}

 
/*
 * modified so that if the -A flag has been given then any initial '/'s
 * in the pattern are skipped too (just a user convenience really...)
 */
int nmatch(char *s, char **patv)
{
	register char *pat;

	if( !patv )
		return 1;
	while(pat = *patv++) {
		while (Aflag && *pat == '/' && pat[1])
			pat++;
		if((*pat == '!' && !gmatch(s, pat+1)) || gmatch(s, pat))
			return 1;
	}
	return 0;
}

void chkswfile( char *sp, char c, short option )
{
	if( !option )
	{
		errorl(MSGSTR(UTIL_MSG_OPT, "-%c must be specified before -%c option."),
			c == 'I' ? 'i' : 'o', c );
		exit(2);
	}
	if( (c == 'I'  &&  option != IN)  ||  (c == 'O'  &&  option != OUT) )
	{
		errorl(MSGSTR(UTIL_MSG_NO_OPT, "-%c option not permitted with -%c option."),c,option);
		exit(2);
	}
	if( !sp )
		return;

	errorl(MSGSTR(UTIL_MSG_OPT_IO, "No more than one -I or -O flag permitted."));
	exit(2);
}

void cannotopen( char *sp, char *mode )
{
	errorl(MSGSTR(UTIL_MSG_NO_OPEN, "cannot open <%s> for %s."), sp, mode );
	exit(2);
}

void chartobin(char *buffer, header *Hdr)	/* new ASCII header read */
{
	sscanf(buffer,"%6o%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx%8lx",
		&(Hdr->h_magic),&(Hdr->h_ino),&(Hdr->h_mode),
		&(Hdr->h_uid),&(Hdr->h_gid),&(Hdr->h_nlink),
		&(Hdr->h_mtime),&(Hdr->h_filesize),&(Hdr->h_dev_maj),
		&(Hdr->h_dev_min),&(Hdr->h_rdev_maj),&(Hdr->h_rdev_min),
		&(Hdr->h_namesize),&(Hdr->h_chksum));
}

void ochartobin(char *buffer, header *Hdr)	/* old ASCII header read */
{
	ulong	device,rdevice;
	
	sscanf(buffer, "%6ho%6ho%6ho%6ho%6ho%6ho%6ho%6ho%11lo%6ho%11lo",
		&(Hdr->h_magic), &device, &(Hdr->h_ino), &(Hdr->h_mode),
		&(Hdr->h_uid), &(Hdr->h_gid), &(Hdr->h_nlink), &rdevice,
		&(Hdr->h_mtime), &(Hdr->h_namesize), &(Hdr->h_filesize));
	Hdr->h_dev_maj = major(device);
	Hdr->h_dev_min = minor(device);
	Hdr->h_rdev_maj = major(rdevice);
	Hdr->h_rdev_min = minor(rdevice);
}

int ckname(char *namep)	/* check filenames with patterns given on cmd line */
{
	char	buf[sizeof Hdr.h_name];

	if(fflag ^ !nmatch(namep, Pattern)) {
		return 0;
	}
	if(Rename && !A_directory) {	/* rename interactively */
		fprintf(Wtty, MSGSTR(UTIL_MSG_RENAME, "Rename <%s>\n"), namep);
		fflush(Wtty);
		skipln(fgets(buf, sizeof buf, Rtty), Rtty);
		if(feof(Rtty))
			cleanup(2);
		if(EQ(buf, "")) {
			strcpy(namep,buf);
			printf(MSGSTR(UTIL_MSG_SKIPPED, "Skipped\n"));
			return 0;
		}
		else if(EQ(buf, "."))
			printf(MSGSTR(UTIL_MSG_SAME_NAME, "Same name\n"));
		else
			strcpy(namep,buf);
	}
	return  1;
}

/*
 * hdck() sanity checks the fixed length portion of the cpio header.
 * -1 indicates a bad header and 0 indicates a good header.
 */
int hdck(header Hdr)
{
	if (Hdr.h_nlink < 1 ||
		Hdr.h_namesize == 0 ||
		Hdr.h_namesize > PATHSIZE)
		return(-1);
	else
		return(0);
}

/* set access and modification times */
void set_time(char *namep, time_t atime, time_t mtime)
{
	static time_t timevec[2];

	if(!Mod_time)
		return;
	timevec[0] = atime;
	timevec[1] = mtime;
	utime(namep, timevec);
}

void set_inode_info(header hdr)
{
	if (geteuid() != 0)
		zchmod( EWARN, hdr.h_name, (hdr.h_mode & (~orig_umask)));
	else
		zchmod( EWARN, hdr.h_name, hdr.h_mode);
	set_time(hdr.h_name, hdr.h_mtime, hdr.h_mtime);
}

/* Implementation of -A option
 * remove all initial slashes from filename if there is one
 *
 * If there is nothing left, add a `.'.
 */
void deroot(char *path_ptr)
{
	register char	*ptr;
	char		*check;

	check = path_ptr;

	/* advance over initial slashes */
	for (ptr = path_ptr; *ptr == '/'; ptr++)
		;
	/* copy the remainder to the begining */
	if (ptr != path_ptr)
		while (*path_ptr++ = *ptr++)
			;
	if (!*check)
		strcpy(check,".");
	
}

/* Implementation of [-T] option 	         */  
/* Truncate filenames greater than 14 characters */
/* enhanced to truncate path components as well */
void truncate_fn(char *path_ptr)				/* L003 */
{
	char tbuf[PATHSIZE];
	char *pp, *ppend;
	int n, tind;

	pp = path_ptr;
	ppend = path_ptr + strlen( path_ptr);	/* last char in path */
	tind = 0;
	while ( pp < ppend) {
		n = 0;
		while ( (*pp != '/') && (n <MAX_NAMELEN) && (pp < ppend) ) {
			tbuf[tind++] = *pp++;
			++n;
		}
		if( n == MAX_NAMELEN ) { /* throw away the rest */
			while( ( *pp++ != '/') && (pp < ppend) );
			if( *(pp-1) == '/')
				tbuf[tind++] = '/';
		}
		else if ( *pp == '/' )
			tbuf[tind++] = *pp++;
	}
	tbuf[tind] = '\0';
	strcpy(path_ptr, tbuf); /* return truncated path in original buffer */
	
}

int missdir(char *namep)
{
	register char *np;
	register ct = 2;

	for(np = namep; *np; ++np)
		if(*np == '/') {
			if(np == namep)
				continue;	/* skip over 'root slash' */
			*np = '\0';
			if(stat(namep, &Xstatb) == -1) {
				if(Dir) {
					(void) umask(orig_umask);
					ct = mkdir(namep, 0777);
					(void) umask(0);
					if (ct != 0) {
						*np = '/';
						return(ct);
					}
				}else {
					errorl(MSGSTR(UTIL_MSG_OPT_D, "missing 'd' option."));
					return(-1);
				}
			}
			*np = '/';
		}
	if (ct == 2) ct = 0;		/* the file already exists */
	return ct;
}

int pwd(char *directory)		/* get working directory */
{
	int dirlen;

	if (getcwd(directory, PATHSIZE + 1) == (char *)0) {
		errorl(MSGSTR(UTIL_MSG_WORK_DIR, "cannot determine working directory."));
		cleanup(2);
	}
	dirlen = strlen(directory);
	directory[dirlen - 1] = '/';

	return(dirlen);
}

/*
        In -V verbose mode, print out a dot for each file processed.
*/
void verbdot( FILE *fp, char *cp )
{

	if( !verbout )
		verbout = fp;
	if( Verbose == 2 ) {
		fputc( '.', fp );
		if( ++verbcount >= 50 ) {
			/* start a new line of dots */
			verbcount = 0;
			fputc( '\n', fp );
		}
	}
	else {
		fputs( cp, fp );
		fputc( '\n', fp );
	}
	fflush(fp);
}

/* ident() accepts pointers to two stat structures and	  */
/* determines if they correspond to identical files.	  */
/* ident() assumes that if the device and inode are the	  */
/* same then files are identical.  ident()'s purpose is to*/
/* prevent putting the archive name in the output archive.*/
int ident(struct stat *in, struct stat *ot)
{
	if (in->st_ino == ot->st_ino &&
	    in->st_dev == ot->st_dev)
		return(1);
	else
		return(0);
}

/*
 *	zlstat		- lstat a file, with warning
 *
 *	Result = zlstat( severity, path, buf);
 *	int severity;		- severity of _errmsg();
 *	char *path;		- pathname of file to be lstat()ed
 *	struct stat *buf;	- pointer to stat buffer for returning info.
 *	int Result;		- value returned by an lstat() on `path'.
 *
 * description	For stat() cpio uses zstat(), a function in devsys
 *		lib.ds/libgen which does a stat(), with a warning on failure.
 *		There is no equivalent zlstat() function in the library.
 *		Therefore the following was written, which is equivalent to
 *		zstat(), except that of course lstat() is used.
 */
int zlstat(int severity, char *path, struct stat *buf)
{
	int Result;

	if ( ( Result = statlstat( path, buf)) < 0)
		psyserrorl(errno,MSGSTR(UTIL_MSG_NO_FILE_INFO, "cannot get info about file:\"%s\""), path);

	return( Result);
}/*zlstat*/

void kmesg()
{
	errorl(MSGSTR(UTIL_MSG_ERR_END_MEDIA, "Error occurred during end-of-media operations."));
	errorl(MSGSTR(UTIL_MSG_OPT_K, "Please reissue the cpio command using -K with appropriate media size."));
}

/* Use the Hdr information to build an old ASCII header in buffer */
int build_oa_header(char *buffer, header Hdr)
{
	int hdr_size;

	/* Manufacture an inode/device pair for this record.
	 * If the link count is 1, then this record should be unique.
	 */

	generate_id(&Hdr);					/* L006 */

	Hdr.h_magic = M_BINARY;

	sprintf(buffer,"%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.6lo%.11lo%.6lo%.11lo%s",
		MK_USHORT(Hdr.h_magic),
		MK_USHORT(makedev(Hdr.h_dev_maj,Hdr.h_dev_min)),
		MK_USHORT(Hdr.h_ino),
		MK_USHORT(Hdr.h_mode),
		MK_USHORT(Hdr.h_uid),
		MK_USHORT(Hdr.h_gid),
		MK_USHORT(Hdr.h_nlink),
		MK_USHORT(makedev(Hdr.h_rdev_maj,Hdr.h_rdev_min)),
		Hdr.h_mtime,
		MK_USHORT(Hdr.h_namesize),
		Hdr.h_filesize,
		Hdr.h_name);

	hdr_size = strlen(buffer) + 1; /* Don't forget the NULL */

	return(hdr_size);
}

/* Use the Hdr information to build an old BINARY header in buffer */
int build_ob_header(char *buffer, header Hdr)
{
	int hdr_size;
	oheader *tmp;

	/* Manufacture an inode/device pair for this record.
	 * If the link count is 1, then this record should be unique.
	 */

	generate_id(&Hdr);					/* L006 */

	tmp = (oheader *)buffer;

	tmp->h_magic	= MK_USHORT(M_BINARY);
	tmp->h_dev	= MK_USHORT(makedev(Hdr.h_dev_maj,Hdr.h_dev_min));
	tmp->h_ino	= MK_USHORT(Hdr.h_ino);
	tmp->h_mode	= MK_USHORT(Hdr.h_mode);
	tmp->h_uid	= MK_USHORT(Hdr.h_uid);
	tmp->h_gid	= MK_USHORT(Hdr.h_gid);
	tmp->h_nlink	= MK_USHORT(Hdr.h_nlink);
	tmp->h_rdev	= MK_USHORT(makedev(Hdr.h_rdev_maj,Hdr.h_rdev_min));
	tmp->h_mtime[0]	= (Hdr.h_mtime >> 16);
	tmp->h_mtime[1]	= (Hdr.h_mtime & 0xffff);
	tmp->h_namesize	= MK_USHORT(Hdr.h_namesize);
	tmp->h_filesize[0] = (Hdr.h_filesize >> 16);
	tmp->h_filesize[1] = (Hdr.h_filesize & 0xffff);
	strcpy(tmp->h_name,Hdr.h_name);

	hdr_size = OHDRSIZE + tmp->h_namesize;

	/* Binary headers are rounded to shorts. */
	if (hdr_size % 2)
		hdr_size += (2 - (hdr_size % 2));

	return(hdr_size);
}

/* Use the Hdr information to build a new ASCII header in buffer */
int build_na_header(char *buffer, header Hdr)
{
	int hdr_size;

	Hdr.h_magic = M_BINARY - 6;
	sprintf(buffer,"%.6o%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%s",
			Hdr.h_magic,Hdr.h_ino,Hdr.h_mode,
			Hdr.h_uid,Hdr.h_gid,Hdr.h_nlink,
			Hdr.h_mtime,Hdr.h_filesize,Hdr.h_dev_maj,
			Hdr.h_dev_min, Hdr.h_rdev_maj,Hdr.h_rdev_min,
			Hdr.h_namesize,Hdr.h_chksum,Hdr.h_name);

	hdr_size = strlen(buffer) + 1; /* Don't forget the NULL */

	/* SVR4 new ASCII headers are rounded to longs. */
	if (hdr_size % 4)
		hdr_size += (4 - (hdr_size % 4));

	return(hdr_size);
}

/* Use the Hdr information to build a new CRC header in buffer */
int build_ncrc_header(char *buffer, header Hdr)
{
	int hdr_size;

	Hdr.h_magic = M_BINARY - 5;
	sprintf(buffer,"%.6o%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%.8lx%s",
			Hdr.h_magic,Hdr.h_ino,Hdr.h_mode,
			Hdr.h_uid,Hdr.h_gid,Hdr.h_nlink,
			Hdr.h_mtime,Hdr.h_filesize,Hdr.h_dev_maj,
			Hdr.h_dev_min, Hdr.h_rdev_maj,Hdr.h_rdev_min,
			Hdr.h_namesize,Hdr.h_chksum,Hdr.h_name);

	hdr_size = strlen(buffer) + 1; /* Don't forget the NULL */

	/* SVR4 new crc headers are rounded to longs. */
	if (hdr_size % 4)
		hdr_size += (4 - (hdr_size % 4));

	return(hdr_size);
}

int build_header(short Cflag, char *buffer, header Hdr)
{
	switch (Cflag) {
	case	OASCII	: return(build_oa_header(buffer, Hdr));
	case	CRC	: return(build_ncrc_header(buffer, Hdr));
	case	ASCII	: return(build_na_header(buffer, Hdr));
	case	BINARY	:
	default		: return(build_ob_header(buffer, Hdr));
	}
}

/* Fills out a new header structure based on data held in an old one. */
void expand_hdr(oheader Ohdr, header *Hdr)
{
	Hdr->h_magic	=	Ohdr.h_magic;
	Hdr->h_dev_maj	=	major(Ohdr.h_dev);
	Hdr->h_dev_min	=	minor(Ohdr.h_dev);
	Hdr->h_ino	=	Ohdr.h_ino;
	Hdr->h_mode	=	Ohdr.h_mode;
	Hdr->h_uid	=	Ohdr.h_uid;
	Hdr->h_gid	=	Ohdr.h_gid;
	Hdr->h_nlink	=	Ohdr.h_nlink;
	Hdr->h_rdev_maj	=	major(Ohdr.h_rdev);
	Hdr->h_rdev_min	=	minor(Ohdr.h_rdev);
	Hdr->h_mtime	=	Ohdr.h_mtime[0];
	Hdr->h_mtime	=	(Hdr->h_mtime << 16) + Ohdr.h_mtime[1];
	Hdr->h_namesize	=	Ohdr.h_namesize;
	Hdr->h_filesize	=	Ohdr.h_filesize[0];
	Hdr->h_filesize	=	(Hdr->h_filesize << 16) + Ohdr.h_filesize[1];
}


/* swap halfwords, bytes or both */
void swap(char *buf, int ct,short swapopt)
{
	register char c;
	register union swp {
		long	longw;
		short	shortv[2];
		char charv[4];
	} *pbuf;
	int savect, n, i;
	char *savebuf;
	short cc;

	savect = ct;	savebuf = buf;
	if((swapopt & BYTESWAP) || (swapopt & BOTHSWAP)) {
		if (ct % 2) buf[ct] = 0;
		ct = (ct + 1) / 2;
		while (ct--) {
			c = *buf;
			*buf = *(buf + 1);
			*(buf + 1) = c;
			buf += 2;
		}
		if (swapopt & BOTHSWAP) {
			ct = savect;
			pbuf = (union swp *)savebuf;
			if (n = ct % sizeof(union swp)) {
				if(n % 2)
					for(i = ct + 1; i <= ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
				else
					for (i = ct; i < ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
			}
			ct = (ct + (sizeof(union swp) -1)) / sizeof(union swp);
			while(ct--) {
				cc = pbuf->shortv[0];
				pbuf->shortv[0] = pbuf->shortv[1];
				pbuf->shortv[1] = cc;
				++pbuf;
			}
		}
	}
	else if (swapopt & HALFSWAP) {
		pbuf = (union swp *)buf;
		if (n = ct % sizeof(union swp))
			for (i = ct; i < ct + (sizeof(union swp) - n); i++) pbuf->charv[i] = 0;
		ct = (ct + (sizeof(union swp) -1)) / sizeof(union swp);
		while (ct--) {
			cc = pbuf->shortv[0];
			pbuf->shortv[0] = pbuf->shortv[1];
			pbuf->shortv[1] = cc;
			++pbuf;
		}
	}
}

/* print verbose table of contents */
void pentry(	char	*filename,
			char	*symto,
			ulong	uid,
			ulong	mode,
			ulong	size,
			ulong	mtime,
			ushort	sum,
			short 	nflag)
{

	static ulong lastid;
	static short found = 0;
	static char *pw;					/* L008 */
 	char	buffer[128];
	static struct passwd	*tpasswd;

	if (nflag)
	{
		printf("%-7.5d", sum);
		printf("%-7o", MK_USHORT(mode));
	} else
		printf("%-7o", MK_USHORT(mode));

	if ((uid == lastid) && found)
		printf("%-6s", pw);				/* L008 */
	else {
		/* pw_idtoname not in UW, so use combo with getpwuid */
		/* if(pw = pw_idtoname((int)uid)) { */		/* L008 */
		if ((tpasswd=getpwuid(uid))) {
			pw = tpasswd->pw_name;
			printf("%-6s", pw);			/* L008 */
			found = 1;
			lastid = uid;
		} else {
			printf("%-6d", uid);
			found = 0;
		}
	}

	printf(" %6ld ", size);
#ifdef INTL
 	strftime(buffer,sizeof(buffer),"%b %d %X %Y",localtime((long *)&mtime));
#else
	/* ctime(buffer,(time_t *)&mtime); */			/* L002 */
	ctime_r((time_t *)&mtime, buffer);			/* L002 */
#endif
 	printf(" %s  %s",buffer, filename);
	if ( *symto != '\0')	/* if symbolic link */
		(void) printf( " -> %s", symto);
	printf( "\n");
}

/* Return block usage, trying hard to avoid overflow. */
ulong block_usage(ulong x, ulong y, int z)			/* L005 Begin */
{
	ulong a, b, *p,*q;


	/* Want to return ((xy+z) + BUFSIZE - 1)/BUFSIZE. */
	if (x > y)
		q = &x, p = &y;
	else
		q = &y, p = &x;

	a = *q / BUFSIZE;
	b = *q % BUFSIZE;

	return(a * *p + (b * *p + z + BUFSIZE - 1) / BUFSIZE);
}								/* L005 End */

								/* L006 Begin */
int set_header_format(char *requested_format)
{
	if (!strncmp(requested_format,"odc",3))
		return(OASCII);
	if (!strncmp(requested_format,"bin",3))
		return(BINARY);
	if (!strncmp(requested_format,"newc",3))
		return(ASCII);
	if (!strncmp(requested_format,"crc",3))
		return(CRC);
	errorl(MSGSTR(UTIL_MSG_BAD_FORMAT, "illegal format type %s, use odc, bin, newc or crc only.")
				,requested_format);
	return(-1);
}
								/* L006 End */
