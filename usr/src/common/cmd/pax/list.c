/*
 * COPYRIGHT NOTICE
 * 
 * This source code is designated as Restricted Confidential Information
 * and is subject to special restrictions in a confidential disclosure
 * agreement between HP, IBM, SUN, NOVELL and OSF.  Do not distribute
 * this source code outside your company without OSF's specific written
 * approval.  This source code, and all copies and derivative works
 * thereof, must be returned or destroyed at request. You must retain
 * this notice on any copies which you make.
 * 
 * (c) Copyright 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC.
 * ALL RIGHTS RESERVED 
 */

#ident	"@(#)pax:list.c	1.1.1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/* 
 * list.c - List all files on an archive
 *
 * DESCRIPTION
 *
 *	These function are needed to support archive table of contents and
 *	verbose mode during extraction and creation of achives.
 *
 * AUTHOR
 *
 *	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
 * Sponsored by The USENIX Association for public distribution. 
 *
 * Copyright (c) 1989 Mark H. Colburn.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice is duplicated in all such 
 * forms and that any documentation, advertising materials, and other 
 * materials related to such distribution and use acknowledge that the 
 * software was developed * by Mark H. Colburn and sponsored by The 
 * USENIX Association. 
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Revision 1.2  89/02/12  10:04:43  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:14  mark
 * Initial revision
 * 
 */

/* Headers */
#include <pwd.h>
#include <grp.h>
#include <sys/mkdev.h>
#include "config.h"
#include "pax.h"
#include "charmap.h"
#include "func.h"
#include "pax_msgids.h"




/* Defines */

/*
 * isodigit returns non zero iff argument is an octal digit, zero otherwise
 */
#define	ISODIGIT(c)	(((c) >= '0') && ((c) <= '7'))


/* Function Prototypes */


static void cpio_entry(char *, Stat *);
static void tar_entry(char *, Stat *);
static void pax_entry(char *, Stat *);
static void print_mode(ushort);
static long from_oct(int digs, char *where);



/* Internal Identifiers */

static char       *monnames[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};


/* read_header - read a header record
 *
 * DESCRIPTION
 *
 * 	Read a record that's supposed to be a header record. Return its 
 *	address in "head", and if it is good, the file's size in 
 *	asb->sb_size.  Decode things from a file header record into a "Stat". 
 *	Also set "head_standard" to !=0 or ==0 depending whether header record 
 *	is "Unix Standard" tar format or regular old tar format. 
 *
 * PARAMETERS
 *
 *	char   *name		- pointer which will contain name of file
 *	Stat   *asb		- pointer which will contain stat info
 *
 * RETURNS
 *
 * 	Return 1 for success, 0 if the checksum is bad, EOF on eof, 2 for a 
 * 	record full of zeros (EOF marker). 
 */


int read_header(char *name, Stat *asb)

{
    int             i;
    long            sum;
    long	    recsum;
    Link           *link;
    char           *p;
    char            hdrbuf[BLOCKSIZE];
    char	   *prefix;
    size_t	   k;
    int		   j;

    memset((char *)asb, 0, sizeof(Stat));

    if (f_append)
	lastheader = bufidx;		/* remember for backup */

    /* read the header from the buffer */
    if (buf_read(hdrbuf, BLOCKSIZE) != 0) {
	return (EOF);
    }

    prefix = &hdrbuf[PREOFS];
    if (*prefix != '\0') {	/* Prefix exists for this file */
	for (k = 0; k < PRESIZ && prefix[k] != '\0'; k++)
		name[k] = prefix[k];
	if (name[k-1] != '/' && hdrbuf[0] != '/')
		name[k++] = '/';
    } else  /* whole path in name field */
	k = 0;

    for (j=0; j< NAMSIZ && hdrbuf[j] != '\0';k++, j++) 
	name[k] = hdrbuf[j];

    name[k] = '\0';

    recsum = from_oct(8, &hdrbuf[148]);
    sum = 0;
    p = hdrbuf;
    for (i = 0 ; i < 500; i++) {

	/*
	 * We can't use unsigned char here because of old compilers, e.g. V7. 
	 */
	sum += 0xFF & *p++;
    }

    /* Adjust checksum to count the "chksum" field as blanks. */
    for (i = 0; i < 8; i++) {
	sum -= 0xFF & hdrbuf[148 + i];
    }
    sum += ' ' * 8;

    if (sum == 8 * ' ') {

	/*
	 * This is a zeroed record...whole record is 0's except for the 8
	 * blanks we faked for the checksum field. 
	 */
	return (2);
    }
    if (sum == recsum) {
	/*
	 * Good record.  Decode file size and return. 
	 */
	if (hdrbuf[156] != LNKTYPE) {
	    asb->sb_size = from_oct(1 + 12, &hdrbuf[124]);
	}
	asb->sb_mtime = from_oct(1 + 12, &hdrbuf[136]);
	asb->sb_mode = from_oct(8, &hdrbuf[100]);
	asb->sb_atime = -1;	/* access time will be 'now' */

	if (strcmp(&hdrbuf[257], TMAGIC) == 0) {
	    /* Unix Standard tar archive */
	    head_standard = 1;
#ifdef NONAMES
	    asb->sb_uid = from_oct(8, &hdrbuf[108]);
	    asb->sb_gid = from_oct(8, &hdrbuf[116]);
#else
	    asb->sb_uid = finduid(&hdrbuf[265]);
	    asb->sb_gid = findgid(&hdrbuf[297]);
#endif
	    switch (hdrbuf[156]) {
	    case BLKTYPE:
	    case CHRTYPE:
		asb->sb_rdev = makedev(from_oct(8, &hdrbuf[329]),
				      from_oct(8, &hdrbuf[337]));
		break;
	    default:
		/* do nothing... */
		break;
	    }
	} else {
	    /* Old fashioned tar archive */
	    head_standard = 0;
	    asb->sb_uid = from_oct(8, &hdrbuf[108]);
	    asb->sb_gid = from_oct(8, &hdrbuf[116]);
	}

	switch (hdrbuf[156]) {
	case REGTYPE:
	case AREGTYPE:
	    /*
	     * Berkeley tar stores directories as regular files with a
	     * trailing /
	     */
	    if (name[strlen(name) - 1] == '/') {
		name[strlen(name) - 1] = '\0';
		asb->sb_mode |= S_IFDIR;
	    } else {
		asb->sb_mode |= S_IFREG;
	    }
	    break;
	case LNKTYPE:
	    asb->sb_nlink = 2;
	    /*
	     * We need to save the linkname so that it is available later
	     * when we have to search the link chain for this link.
	     */
	    asb->linkname = mem_rpl_name(&hdrbuf[157]);

	    linkto(&hdrbuf[157], asb);	/* don't use linkname here */
	    linkto(name, asb);
	    asb->sb_mode |= S_IFREG;
	    break;
	case BLKTYPE:
	    asb->sb_mode |= S_IFBLK;
	    break;
	case CHRTYPE:
	    asb->sb_mode |= S_IFCHR;
	    break;
	case DIRTYPE:
	    asb->sb_mode |= S_IFDIR;
	    break;
#ifdef S_IFLNK
	case SYMTYPE:
	    asb->sb_mode |= S_IFLNK;
	    strcpy(asb->sb_link, &hdrbuf[157]);
	    break;
#endif
#ifdef S_IFIFO
	case FIFOTYPE:
	    asb->sb_mode |= S_IFIFO;
	    break;
#endif
#ifdef S_IFCTG
	case CONTTYPE:
	    asb->sb_mode |= S_IFCTG;
	    break;
#endif
	}
	return (1);
    }
    return (0);
}


/* print_entry - print a single table-of-contents entry
 *
 * DESCRIPTION
 * 
 *	Print_entry prints a single line of file information.  The format
 *	of the line is the same as that used by the LS command.  For some
 *	archive formats, various fields may not make any sense, such as
 *	the link count on tar archives.  No error checking is done for bad
 *	or invalid data.
 *
 * PARAMETERS
 *
 *	char   *name		- pointer to name to print an entry for
 *	Stat   *asb		- pointer to the stat structure for the file
 */


void print_entry(char *name, Stat *asb)

{
    switch (ar_interface) {
    case TAR:
	tar_entry(name, asb);
	break;
    case CPIO:
	cpio_entry(name, asb);
	break;
    case PAX: pax_entry(name, asb);
	break;
    }
}


/* cpio_entry - print a verbose cpio-style entry
 *
 * DESCRIPTION
 *
 *	Print_entry prints a single line of file information.  The format
 *	of the line is the same as that used by the traditional cpio 
 *	command.  No error checking is done for bad or invalid data.
 *
 * PARAMETERS
 *
 *	char   *name		- pointer to name to print an entry for
 *	Stat   *asb		- pointer to the stat structure for the file
 */


static void cpio_entry(char *name, Stat *asb)

{
    struct tm	       *atm;
    Link	       *from;
    struct passwd      *pwp;
    struct group       *grp;
    char	       mon[4];		/* abreviated month name */

    if (f_list && f_verbose) {
	pfmt(msgfile, MM_NOSTD, ":120:%-7o", asb->sb_mode);
	atm = localtime(&asb->sb_mtime);
	if (pwp = getpwuid((int) USH(asb->sb_uid))) {
	    pfmt(msgfile, MM_NOSTD, ":121:%-6s", pwp->pw_name);
	} else {
	    pfmt(msgfile, MM_NOSTD, ":122:%-6u", USH(asb->sb_uid));
	}
	(void)strftime(mon, sizeof(mon), "%b", atm);
	pfmt(msgfile, MM_NOSTD, ":123:%7ld  %3s %2d %02d:%02d:%02d %4d  ",
	               asb->sb_size, mon, 
		       atm->tm_mday, atm->tm_hour, atm->tm_min, 
		       atm->tm_sec, atm->tm_year + 1900);
    }
    fprintf(msgfile, "%s", name);
    if ((asb->sb_nlink > 1) && (from = islink(name, asb))) {
	pfmt(msgfile, MM_NOSTD, LS_LINK": linked to %s", from->l_name);
    }
#ifdef	S_IFLNK
    if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	pfmt(msgfile, MM_NOSTD, LS_SYMLINK": symbolic link to %s",
		asb->sb_link);
    }
#endif	/* S_IFLNK */
    putc('\n', msgfile);
}


/* tar_entry - print a tar verbose mode entry
 *
 * DESCRIPTION
 *
 *	Print_entry prints a single line of tar file information.  The format
 *	of the line is the same as that produced by the traditional tar 
 *	command.  No error checking is done for bad or invalid data.
 *
 * PARAMETERS
 *
 *	char   *name		- pointer to name to print an entry for
 *	Stat   *asb		- pointer to the stat structure for the file
 */


static void tar_entry(char *name, Stat *asb)

{
    struct tm  	       *atm;
    int			i;
    int			mode;
    char               *symnam = "NULL";
    Link               *link;
    char		mon[4];		/* abbreviated month name */

    if ((mode = asb->sb_mode & S_IFMT) == S_IFDIR) {
	return;			/* don't print directories */
    }
    if (f_extract) {
	switch (mode) {
#ifdef S_IFLNK
	case S_IFLNK: 	/* This file is a symbolic link */
	    i = readlink(name, symnam, PATH_MAX);
	    if (i < 0) {		/* Could not find symbolic link */
		warn(gettxt(LS_SYM, "can't read symbolic link"),
			strerror(errno));
	    } else { 		/* Found symbolic link filename */
		symnam[i] = '\0';
		pfmt(msgfile, MM_NOSTD, LS_XSYM":x %s symbolic link to %s\n",
			name, symnam);
	    }
	    break;
#endif
	case S_IFREG: 	/* It is a link or a file */
	    if ((asb->sb_nlink > 1) && (link = islink(name, asb))) {
		pfmt(msgfile, MM_NOSTD, LS_LINK2":%s linked to %s\n",
			name, link->l_name); 
	    } else {
		pfmt(msgfile, MM_NOSTD,
			LS_SUM":x %s, %ld bytes, %d tape blocks\n", 
			name, asb->sb_size, ROUNDUP(asb->sb_size, 
			BLOCKSIZE) / BLOCKSIZE);
	    }
	}
    } else if (f_append || f_create) {
	switch (mode) {
#ifdef S_IFLNK
	case S_IFLNK: 	/* This file is a symbolic link */
	    i = readlink(name, symnam, PATH_MAX);
	    if (i < 0) {		/* Could not find symbolic link */
		warn(gettxt(LS_READ, "can't read symbolic link"),
			strerror(errno));
	    } else { 		/* Found symbolic link filename */
		symnam[i] = '\0';
		pfmt(msgfile, MM_NOSTD, LS_ASYM":a %s symbolic link to %s\n",
			name, symnam);
	    }
	    break;
#endif
	case S_IFREG: 	/* It is a link or a file */
	    pfmt(msgfile, MM_NOSTD, ":124:a %s ", name);
	    if ((asb->sb_nlink > 1) && (link = islink(name, asb))) {
		pfmt(msgfile, MM_NOSTD, LS_LINK3":link to %s\n", link->l_name); 
	    } else {
		pfmt(msgfile, MM_NOSTD, BLOCKS":%ld Blocks\n", 
			ROUNDUP(asb->sb_size, BLOCKSIZE) / BLOCKSIZE);
	    }
	    break;
	}
    } else if (f_list) {
	if (f_verbose) {
	    atm = localtime(&asb->sb_mtime);
	    (void)strftime(mon, sizeof(mon), "%b", atm);
	    print_mode(asb->sb_mode);
	    pfmt(msgfile, MM_NOSTD, ":125: %d/%d %6d %3s %2d %02d:%02d %4d %s",
		    asb->sb_uid, asb->sb_gid, asb->sb_size,
		    mon, atm->tm_mday, atm->tm_hour, 
		    atm->tm_min, atm->tm_year + 1900, name);
	} else {
	    fprintf(msgfile, "%s", name);
	}
	switch (mode) {
#ifdef S_IFLNK
	case S_IFLNK: 	/* This file is a symbolic link */
	    i = readlink(name, symnam, PATH_MAX);
	    if (i < 0) {		/* Could not find symbolic link */
		warn(gettxt(LS_READ, "can't read symbolic link"),
			strerror(errno));
	    } else { 		/* Found symbolic link filename */
		symnam[i] = '\0';
		pfmt(msgfile, MM_NOSTD, LS_SYMLINK": symbolic link to %s",
			symnam);
	    }
	    break;
#endif
	case S_IFREG: 	/* It is a link or a file */
	    if ((asb->sb_nlink > 1) && (link = islink(name, asb))) {
		pfmt(msgfile, MM_NOSTD, LS_LINK": linked to %s", link->l_name);
	    }
	    break;		/* Do not print out directories */
	}
	fputc('\n', msgfile);
    } else {
	pfmt(msgfile, MM_NOSTD, LS_SUM2":? %s %ld blocks\n", name,
		ROUNDUP(asb->sb_size, BLOCKSIZE) / BLOCKSIZE);
    }
}


/* pax_entry - print a verbose cpio-style entry
 *
 * DESCRIPTION
 *
 *	Print_entry prints a single line of file information.  The format
 *	of the line is the same as that used by the LS command.  
 *	No error checking is done for bad or invalid data.
 *
 * PARAMETERS
 *
 *	char   *name		- pointer to name to print an entry for
 *	Stat   *asb		- pointer to the stat structure for the file
 */


static void pax_entry(char *name, Stat *asb)

{
    struct tm	       *atm;
    Link	       *from;
    struct passwd      *pwp;
    struct group       *grp;
    char		mon[4];		/* abbreviated month name */
    time_t		six_months_ago;

    if (f_list && f_verbose) {
	print_mode(asb->sb_mode);
	pfmt(msgfile, MM_NOSTD, ":126: %2d", asb->sb_nlink);
	atm = localtime(&asb->sb_mtime);
	six_months_ago = now - 6L*30L*24L*60L*60L; /* 6 months ago */
	(void) strftime(mon, sizeof(mon), "%b", atm);
	if (pwp = getpwuid((int) USH(asb->sb_uid))) {
	    pfmt(msgfile, MM_NOSTD, ":127: %-8s", pwp->pw_name);
	} else {
	    pfmt(msgfile, MM_NOSTD, ":128: %-8u", USH(asb->sb_uid));
	}
	if (grp = getgrgid((int) USH(asb->sb_gid))) {
	    pfmt(msgfile, MM_NOSTD, ":127: %-8s", grp->gr_name);
	} else {
	    pfmt(msgfile, MM_NOSTD, ":128: %-8u", USH(asb->sb_gid));
	}
	switch (asb->sb_mode & S_IFMT) {
	case S_IFBLK:
	case S_IFCHR:
	    pfmt(msgfile, MM_NOSTD, ":129:\t%3d, %3d",
		           major(asb->sb_rdev), minor(asb->sb_rdev));
	    break;
	case S_IFREG:
	    pfmt(msgfile, MM_NOSTD, ":130:\t%8ld", asb->sb_size);
	    break;
	default:
	    pfmt(msgfile, MM_NOSTD, ":131:\t        ");
	}
	if ((asb->sb_mtime < six_months_ago) || (asb->sb_mtime > now)) {
	    pfmt(msgfile, MM_NOSTD,":132: %3s %2d  %4d ",
	            mon, atm->tm_mday, 
		    atm->tm_year + 1900);
	} else {
	    pfmt(msgfile, MM_NOSTD,":133: %3s %2d %02d:%02d ",
	            mon, atm->tm_mday, 
		    atm->tm_hour, atm->tm_min);
	}
	fprintf(msgfile, "%s", name);
	if ((asb->sb_nlink > 1) && (from = islink(name, asb))) {
	    pfmt(msgfile, MM_NOSTD, ":134: == %s", from->l_name);
	}
#ifdef	S_IFLNK
	if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	    pfmt(msgfile, MM_NOSTD, ":135: -> %s", asb->sb_link);
	}
#endif	/* S_IFLNK */
	putc('\n', msgfile);
    } else
	fprintf(msgfile, "%s\n", name);
}


/* print_mode - fancy file mode display
 *
 * DESCRIPTION
 *
 *	Print_mode displays a numeric file mode in the standard unix
 *	representation, ala ls (-rwxrwxrwx).  No error checking is done
 *	for bad mode combinations.  FIFOS, sybmbolic links, sticky bits,
 *	block- and character-special devices are supported if supported
 *	by the hosting implementation.
 *
 * PARAMETERS
 *
 *	ushort	mode	- The integer representation of the mode to print.
 */


static void print_mode(ushort mode)

{
    /* Tar does not print the leading identifier... */
    if (ar_interface != TAR) {
	switch (mode & S_IFMT) {
	case S_IFDIR: 
	    putc('d', msgfile); 
	    break;
#ifdef	S_IFLNK
	case S_IFLNK: 
	    putc('l', msgfile); 
	    break;
#endif	/* S_IFLNK */
	case S_IFBLK: 
	    putc('b', msgfile); 
	    break;
	case S_IFCHR: 
	    putc('c', msgfile); 
	    break;
#ifdef	S_IFIFO
	case S_IFIFO: 
	    putc('p', msgfile); 
	    break; 
#endif	/* S_IFIFO */ 
	case S_IFREG: 
	default:
	    putc('-', msgfile); 
	    break;
	}
    }
    putc(mode & 0400 ? 'r' : '-', msgfile);
    putc(mode & 0200 ? 'w' : '-', msgfile);
    putc(mode & 0100
	 ? mode & 04000 ? 's' : 'x'
	 : mode & 04000 ? 'S' : '-', msgfile);
    putc(mode & 0040 ? 'r' : '-', msgfile);
    putc(mode & 0020 ? 'w' : '-', msgfile);
    putc(mode & 0010
	 ? mode & 02000 ? 's' : 'x'
	 : mode & 02000 ? 'S' : '-', msgfile);
    putc(mode & 0004 ? 'r' : '-', msgfile);
    putc(mode & 0002 ? 'w' : '-', msgfile);
    putc(mode & 0001
	 ? mode & 01000 ? 't' : 'x'
	 : mode & 01000 ? 'T' : '-', msgfile);
}


/* from_oct - quick and dirty octal conversion
 *
 * DESCRIPTION
 *
 *	From_oct will convert an ASCII representation of an octal number
 *	to the numeric representation.  The number of characters to convert
 *	is given by the parameter "digs".  If there are less numbers than
 *	specified by "digs", then the routine returns -1.
 *
 * PARAMETERS
 *
 *	int digs	- Number to of digits to convert 
 *	char *where	- Character representation of octal number
 *
 * RETURNS
 *
 *	The value of the octal number represented by the first digs
 *	characters of the string where.  Result is -1 if the field 
 *	is invalid (all blank, or nonoctal). 
 *
 * ERRORS
 *
 *	If the field is all blank, then the value returned is -1.
 *
 */


static long from_oct(int digs, char *where)

{
    long            value;

    while (isspace(*where)) {	/* Skip spaces */
	where++;
	if (--digs <= 0) {
	    return(-1);		/* All blank field */
	}
    }
    value = 0;
    while (digs > 0 && ISODIGIT(*where)) {	/* Scan til nonoctal */
	value = (value << 3) | (*where++ - '0');
	--digs;
    }

    if (digs > 0 && *where && !isspace(*where)) {
	return(-1);		/* Ended on non-space/nul */
    }
    return(value);
}
