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

#ident	"@(#)pax:extract.c	1.1.1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/* 
 * extract.c - Extract files from a tar archive. 
 *
 * DESCRIPTION
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
 * Revision 1.3  89/02/12  10:29:43  mark
 * Fixed misspelling of Replstr
 * 
 * Revision 1.2  89/02/12  10:04:24  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:07  mark
 * Initial revision
 * 
 */

/* Headers */
#include <sys/types.h>
#include <sys/utime.h>
#include "config.h"
#include "pax.h"
#include "charmap.h"
#include "func.h"
#include "pax_msgids.h"




/* Defines */

/*
 * Swap bytes. 
 */
#define	SWAB(n)	((((ushort)(n) >> 8) & 0xff) | (((ushort)(n) << 8) & 0xff00))


/* Function Prototypes */


static int inbinary(char *, char *, Stat *);
static int inascii(char *, char *, Stat *);
static int inswab(char *, char *, Stat *);
static int readtar(char *, Stat *);
static int readcpio(char *, Stat *);



/* read_archive - read in an archive
 *
 * DESCRIPTION
 *
 *	Read_archive is the central entry point for reading archives.
 *	Read_archive determines the proper archive functions to call
 *	based upon the archive type being processed.
 *
 * RETURNS
 *
 */


int read_archive(void)

{
    Stat            sb, osb;
    char            name[PATH_MAX + 1];
    int             match;
    int		    pad;

    name_gather();		/* get names from command line */
    name[0] = '\0';
    while (get_header(name, &sb) == 0) {

	if (f_charmap && charmap_convert(name) < 0) 
	    continue;
/*
	match = name_match(name, ((sb.sb_mode & S_IFMT) == S_IFDIR)) ^ f_reverse_match;
*/
	if ((match = name_match(name,((sb.sb_mode & S_IFMT) == S_IFDIR))) == -1)
	    break;
	else
	    match ^= f_reverse_match;
	if (f_list) {		/* only wanted a table of contents */
	    if (match) {
		print_entry(name, &sb);
	    }
	    if (((ar_format == TAR) 
		? buf_skip(ROUNDUP((OFFSET) sb.sb_size, BLOCKSIZE)) 
		: buf_skip((OFFSET) sb.sb_size)) < 0) {
		warn(name, gettxt(EX_CORRUPT, "File data is corrupt"));
	    }
	} else if (match) {
    	    if (LSTAT(name, &osb) == 0) {
	    	if (!f_unconditional && osb.sb_mtime > sb.sb_mtime) {
		    bad_last_match = 1;
		    warn(name, gettxt(FIO_NEWER, "Newer file exists"));
		    if (((ar_format == TAR) 
			? buf_skip(ROUNDUP((OFFSET) sb.sb_size, BLOCKSIZE)) 
			: buf_skip((OFFSET) sb.sb_size)) < 0)
			warn(name, gettxt(EX_CORRUPT, "File data is corrupt"));
		    continue;
		}
	    }
	    if (rplhead != (Replstr *)NULL) {
		rpl_name(name);
		if (strlen(name) == 0) {
		    if (((ar_format == TAR) 
			? buf_skip(ROUNDUP((OFFSET) sb.sb_size, BLOCKSIZE)) 
			: buf_skip((OFFSET) sb.sb_size)) < 0) {
			warn(name, gettxt(EX_CORRUPT, "File data is corrupt"));
		    }
		    continue;
		}
	    }
	    if (get_disposition(EXTRACT, name) || 
                get_newname(name, sizeof(name))) {
		/* skip file... */
		if (((ar_format == TAR) 
		    ? buf_skip(ROUNDUP((OFFSET) sb.sb_size, BLOCKSIZE)) 
		    : buf_skip((OFFSET) sb.sb_size)) < 0) {
		    warn(name, gettxt(EX_CORRUPT, "File data is corrupt"));
		}
		continue;
	    } 
	    if (inentry(name, &sb) < 0) {
		warn(name, gettxt(EX_CORRUPT, "File data is corrupt"));
	    }
	    if (f_verbose) {
		print_entry(name, &sb);
	    }
	    if (ar_format == TAR && sb.sb_nlink > 1) {
		/*
		 * This kludge makes sure that the link table is cleared
		 * before attempting to process any other links.
		 */
		if (sb.sb_nlink > 1) {
		    linkfrom(name, &sb);
		}
	    }
	    if (ar_format == TAR && (pad = sb.sb_size % BLOCKSIZE) != 0) {
		pad = BLOCKSIZE - pad;
		buf_skip((OFFSET) pad);
	    }
	} else {
	    if (((ar_format == TAR) 
		? buf_skip(ROUNDUP((OFFSET) sb.sb_size, BLOCKSIZE)) 
		: buf_skip((OFFSET) sb.sb_size)) < 0) {
		warn(name, gettxt(EX_CORRUPT, "File data is corrupt"));
	    }
	}
    }

    close_archive();
    if (!f_list)
	reset_directories();
}



/* get_header - figures which type of header needs to be read.
 *
 * DESCRIPTION
 *
 *	This is merely a single entry point for the two types of archive
 *	headers which are supported.  The correct header is selected
 *	depending on the archive type.
 *
 * PARAMETERS
 *
 *	char	*name	- name of the file (passed to header routine)
 *	Stat	*asb	- Stat block for the file (passed to header routine)
 *
 * RETURNS
 *
 *	Returns the value which was returned by the proper header
 *	function.
 */


int get_header(char *name, Stat *asb)

{
    if (ar_format == TAR) {
	return(readtar(name, asb));
    } else {
	return(readcpio(name, asb));
    }
}


/* readtar - read a tar header
 *
 * DESCRIPTION
 *
 *	Tar_head read a tar format header from the archive.  The name
 *	and asb parameters are modified as appropriate for the file listed
 *	in the header.   Name is assumed to be a pointer to an array of
 *	at least PATH_MAX+1 bytes.
 *
 * PARAMETERS
 *
 *	char	*name 	- name of the file for which the header is
 *			  for.  This is modified and passed back to
 *			  the caller.
 *	Stat	*asb	- Stat block for the file for which the header
 *			  is for.  The fields of the stat structure are
 *			  extracted from the archive header.  This is
 *			  also passed back to the caller.
 *
 * RETURNS
 *
 *	Returns 0 if a valid header was found, or -1 if EOF is
 *	encountered.
 */


static int readtar(char *name, Stat *asb)

{
    int             status = 3;	/* Initial status at start of archive */
    static int      prev_status;

    for (;;) {
	prev_status = status;
	status = read_header(name, asb);
	switch (status) {
	case 1:		/* Valid header */
		return(0);
	case 0:		/* Invalid header */
	    switch (prev_status) {
	    case 3:		/* Error on first record */
		warn(ar_file, gettxt(EX_NOTAR, "This doesn't look like a tar archive"));
		/* FALLTHRU */
	    case 2:		/* Error after record of zeroes */
	    case 1:		/* Error after header rec */
		warn(ar_file, gettxt(EX_SKIP, "Skipping to next file..."));
		/* FALLTHRU */
	    default:
	    case 0:		/* Error after error */
		break;
	    }
	    break;

	case 2:			/* Record of zeroes */
	case EOF:		/* End of archive */
	default:
	    return(-1);
	}
    }
}


/* readcpio - read a CPIO header 
 *
 * DESCRIPTION
 *
 *	Read in a cpio header.  Understands how to determine and read ASCII, 
 *	binary and byte-swapped binary headers.  Quietly translates 
 *	old-fashioned binary cpio headers (and arranges to skip the possible 
 *	alignment byte). Returns zero if successful, -1 upon archive trailer. 
 *
 * PARAMETERS
 *
 *	char	*name 	- name of the file for which the header is
 *			  for.  This is modified and passed back to
 *			  the caller.
 *	Stat	*asb	- Stat block for the file for which the header
 *			  is for.  The fields of the stat structure are
 *			  extracted from the archive header.  This is
 *			  also passed back to the caller.
 *
 * RETURNS
 *
 *	Returns 0 if a valid header was found, or -1 if EOF is
 *	encountered.
 */


static int readcpio(char *name, Stat *asb)

{
    OFFSET          skipped;
    char            magic[M_STRLEN];
    static int      align;

    if (align > 0) {
	buf_skip((OFFSET) align);
    }
    align = 0;
    for (;;) {
	if (f_append)
	    lastheader = bufidx;		/* remember for backup */
	buf_read(magic, M_STRLEN);
	skipped = 0;
	while ((align = inascii(magic, name, asb)) < 0
	       && (align = inbinary(magic, name, asb)) < 0
	       && (align = inswab(magic, name, asb)) < 0) {
	    if (++skipped == 1) {
		if (total - sizeof(magic) == 0) {
		    fatal(gettxt(EX_UNREG, "Unrecognizable archive"));
		}
		warnarch(gettxt(EX_BADMAG, "Bad magic number"), (OFFSET) sizeof(magic));
		if (name[0]) {
		    warn(name, gettxt(EX_MAY, "May be corrupt"));
		}
	    }
	    memcpy(magic, magic + 1, sizeof(magic) - 1);
	    buf_read(magic + sizeof(magic) - 1, 1);
	}
	if (skipped) {
	    warnarch(gettxt(EX_RESYNC, "Apparently resynchronized"), (OFFSET) sizeof(magic));
	    warn(name, gettxt(EX_CONT, "Continuing"));
	}
	if (strcmp(name, TRAILER) == 0) {
	    return (-1);
	}
	if (nameopt(name) >= 0) {
	    break;
	}
	buf_skip((OFFSET) asb->sb_size + align);
    }
#ifdef	S_IFLNK
    if ((asb->sb_mode & S_IFMT) == S_IFLNK) {
	if (buf_read(asb->sb_link, (uint) asb->sb_size) < 0) {
	    warn(name, gettxt(EX_SYM, "Corrupt symbolic link"));
	    return (readcpio(name, asb));
	}
	asb->sb_link[asb->sb_size] = '\0';
	asb->sb_size = 0;
    }
#endif				/* S_IFLNK */

    /* destroy absolute pathnames for security reasons */
    if (name[0] == '/') {
	if (name[1]) {
	    while (name[0] = name[1]) {
		++name;
	    }
	} else {
	    name[0] = '.';
	}
    }
    asb->sb_ctime = asb->sb_mtime;
    asb->sb_atime = -1;			/* the access time will be 'now' */
    if (asb->sb_nlink > 1) {
	linkto(name, asb);
    }
    return (0);
}


/* inswab - read a reversed by order binary header
 *
 * DESCRIPTIONS
 *
 *	Reads a byte-swapped CPIO binary archive header
 *
 * PARMAMETERS
 *
 *	char	*magic	- magic number to match
 *	char	*name	- name of the file which is stored in the header.
 *			  (modified and passed back to caller).
 *	Stat	*asb	- stat block for the file (modified and passed back
 *			  to the caller).
 *
 *
 * RETURNS
 *
 * 	Returns the number of trailing alignment bytes to skip; -1 if 
 *	unsuccessful. 
 *
 */


static int inswab(char *magic, char *name, Stat *asb)

{
    ushort          namesize;
    uint            namefull;
    Binary          binary;

    if (*((ushort *) magic) != SWAB(M_BINARY)) {
	return (-1);
    }
    memcpy((char *) &binary,
		  magic + sizeof(ushort),
		  M_STRLEN - sizeof(ushort));
    if (buf_read((char *) &binary + M_STRLEN - sizeof(ushort),
		 sizeof(binary) - (M_STRLEN - sizeof(ushort))) < 0) {
	warnarch(gettxt(EX_HDR, "Corrupt swapped header"),
		 (OFFSET) sizeof(binary) - (M_STRLEN - sizeof(ushort)));
	return (-1);
    }
    asb->sb_dev = (dev_t) SWAB(binary.b_dev);
    asb->sb_ino = (ino_t) SWAB(binary.b_ino);
    asb->sb_mode = SWAB(binary.b_mode);
    asb->sb_uid = SWAB(binary.b_uid);
    asb->sb_gid = SWAB(binary.b_gid);
    asb->sb_nlink = SWAB(binary.b_nlink);
    asb->sb_rdev = (dev_t) SWAB(binary.b_rdev);
    asb->sb_mtime = SWAB(binary.b_mtime[0]) << 16 | SWAB(binary.b_mtime[1]);
    asb->sb_size = SWAB(binary.b_size[0]) << 16 | SWAB(binary.b_size[1]);
    if ((namesize = SWAB(binary.b_name)) == 0 || namesize >= 
	(ushort)(PATH_MAX + 1)) {
	warnarch(gettxt(EX_PLEN1, "Bad swapped pathname length"),
		 (OFFSET) sizeof(binary) - (M_STRLEN - sizeof(ushort)));
	return (-1);
    }
    if (buf_read(name, namefull = namesize + namesize % 2) < 0) {
	warnarch(gettxt(EX_PATH, "Corrupt swapped pathname"),(OFFSET) namefull);
	return (-1);
    }
    if (name[namesize - 1] != '\0') {
	warnarch(gettxt(EX_BADPATH, "Bad swapped pathname"), (OFFSET) namefull);
	return (-1);
    }
    return (asb->sb_size % 2);
}


/* inascii - read in an ASCII cpio header
 *
 * DESCRIPTION
 *
 *	Reads an ASCII format cpio header
 *
 * PARAMETERS
 *
 *	char	*magic	- magic number to match
 *	char	*name	- name of the file which is stored in the header.
 *			  (modified and passed back to caller).
 *	Stat	*asb	- stat block for the file (modified and passed back
 *			  to the caller).
 *
 * RETURNS
 *
 * 	Returns zero if successful; -1 otherwise. Assumes that  the entire 
 *	magic number has been read. 
 */


static int inascii(char *magic, char *name, Stat *asb)

{
    uint            namelen;
    char            header[H_STRLEN + 1];

    if (strncmp(magic, M_ASCII, M_STRLEN) != 0) {
	return (-1);
    }
    if (buf_read(header, H_STRLEN) < 0) {
	warnarch(gettxt(EX_ASCII, "Corrupt ASCII header"), (OFFSET) H_STRLEN);
	return (-1);
    }
    header[H_STRLEN] = '\0';
    if (sscanf(header, H_SCAN, &asb->sb_dev,
	       &asb->sb_ino, &asb->sb_mode, &asb->sb_uid,
	       &asb->sb_gid, &asb->sb_nlink, &asb->sb_rdev,
	       &asb->sb_mtime, &namelen, &asb->sb_size) != H_COUNT) {
	warnarch(gettxt(EX_BADHDR, "Bad ASCII header"), (OFFSET) H_STRLEN);
	return (-1);
    }
    if (namelen == 0 || namelen >= PATH_MAX+1) {
	warnarch(gettxt(EX_PLEN2, "Bad ASCII pathname length"), (OFFSET) H_STRLEN);
	return (-1);
    }
    if (buf_read(name, namelen) < 0) {
	warnarch(gettxt(EX_APATH, "Corrupt ASCII pathname"), (OFFSET) namelen);
	return (-1);
    }
    if (name[namelen - 1] != '\0') {
	warnarch(gettxt(EX_BADAPATH, "Bad ASCII pathname"), (OFFSET) namelen);
	return (-1);
    }
    return (0);
}


/* inbinary - read a binary header
 *
 * DESCRIPTION
 *
 *	Reads a CPIO format binary header.
 *
 * PARAMETERS
 *
 *	char	*magic	- magic number to match
 *	char	*name	- name of the file which is stored in the header.
 *			  (modified and passed back to caller).
 *	Stat	*asb	- stat block for the file (modified and passed back
 *			  to the caller).
 *
 * RETURNS
 *
 * 	Returns the number of trailing alignment bytes to skip; -1 if 
 *	unsuccessful. 
 */


static int inbinary(char *magic, char *name, Stat *asb)

{
    uint            namefull;
    Binary          binary;

    if (*((ushort *) magic) != M_BINARY) {
	return (-1);
    }
    memcpy((char *) &binary,
		  magic + sizeof(ushort),
		  M_STRLEN - sizeof(ushort));
    if (buf_read((char *) &binary + M_STRLEN - sizeof(ushort),
		 sizeof(binary) - (M_STRLEN - sizeof(ushort))) < 0) {
	warnarch(gettxt(EX_BHDR, "Corrupt binary header"),
		 (OFFSET) sizeof(binary) - (M_STRLEN - sizeof(ushort)));
	return (-1);
    }
    asb->sb_dev = binary.b_dev;
    asb->sb_ino = binary.b_ino;
    asb->sb_mode = binary.b_mode;
    asb->sb_uid = binary.b_uid;
    asb->sb_gid = binary.b_gid;
    asb->sb_nlink = binary.b_nlink;
    asb->sb_rdev = binary.b_rdev;
    asb->sb_mtime = binary.b_mtime[0] << 16 | binary.b_mtime[1];
    asb->sb_size = binary.b_size[0] << 16 | binary.b_size[1];
    if (binary.b_name == 0 || binary.b_name >= (ushort)(PATH_MAX + 1)) {
	warnarch(gettxt(EX_PLEN3, "Bad binary pathname length"),
		 (OFFSET) sizeof(binary) - (M_STRLEN - sizeof(ushort)));
	return (-1);
    }
    if (buf_read(name, namefull = binary.b_name + binary.b_name % 2) < 0) {
	warnarch(gettxt(EX_BPATH, "Corrupt binary pathname"),(OFFSET) namefull);
	return (-1);
    }
    if (name[binary.b_name - 1] != '\0') {
	warnarch(gettxt(EX_BADP, "Bad binary pathname"), (OFFSET) namefull);
	return (-1);
    }
    return (asb->sb_size % 2);
}


/* reset_directories - reset time/mode on directories we have restored.
 *
 * DESCRIPTION
 *
 *	Walk through the list of directories we have extracted from
 *	the archive (dirhead) and reset the permissions, the
 *	modify times, the owner and group and if we have it, 
 * 	the access times.
 *
 */


void reset_directories(void)

{
    Dirlist		*dp;
    mode_t		perm;
    struct utimbuf	tstamp;
    time_t		now;
    char 		*name;

    now = time((time_t) 0);	/* cut down on time calls */

    for (dp = dirhead; dp; dp=dp->next) {
	name = dp->name;
	if (f_mode && f_owner) 		/* restore all mode bits */
	    perm = dp->perm & (S_IPERM | S_ISUID | S_ISGID);
	else if (f_mode)
	    perm = dp->perm & S_IPERM;
	else
	    perm = (dp->perm & S_IPOPN) & ~mask; 	/* use umask */
	if (chmod(name, perm) < 0)
	    warn(name, strerror(errno));

	if (f_extract_access_time || f_mtime) {
	    if ((dp->atime == -1) && f_extract_access_time)
		dp->atime = now;
	    tstamp.actime = f_extract_access_time ?  dp->atime  : now;
	    tstamp.modtime = f_mtime ? dp->mtime : now;
	    utime(name, &tstamp);
	}

	if (f_owner && (chown(name, dp->uid, dp->gid) < 0)) {
	    warn(name, gettxt(EX_OWN, "could not restore owner/group"));
	    chmod(name, (perm & S_IPERM));      /* Clear SUID/SGID bits */
	}
    }
    return;
}
