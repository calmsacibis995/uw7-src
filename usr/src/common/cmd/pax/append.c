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

#ident	"@(#)pax:append.c	1.1"

/*
 * OSF/1 1.2
 */
#if !defined(lint) && !defined(_NOIDENT)
static char rcsid[] = "@(#)$RCSfile$ $Revision$ (OSF) $Date$";
#endif
/* 
 * append.c - append to a tape archive. 
 *
 * DESCRIPTION
 *
 *	Routines to allow appending of archives
 *
 * AUTHORS
 *
 *     	Mark H. Colburn, NAPS International (mark@jhereg.mn.org)
 *
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
 * Revision 1.2  89/02/12  10:03:58  mark
 * 1.2 release fixes
 * 
 * Revision 1.1  88/12/23  18:02:00  mark
 * Initial revision
 * 
 */


/* Headers */
#include <sys/types.h>
#include <sys/ioctl.h>
/* #include <sys/mtio.h> */
#include <locale.h>
#include "config.h"
#include "pax.h"
#include "charmap.h"
#include "func.h"
#include "pax_msgids.h"



/* append_archive - main loop for appending to a tar archive
 *
 * DESCRIPTION
 *
 *	Append_archive reads an archive until the end of the archive is
 *	reached once the archive is reached, the buffers are reset and the
 *	create_archive function is called to handle the actual writing of
 *	the appended archive data.  This is quite similar to the
 *	read_archive function, however, it does not do all the processing.
 */


void
append_archive(void)
{
    Stat            sb;
    char            name[PATH_MAX + 1];

    name[0] = '\0';
    while (get_header(name, &sb) == 0) {
	if (!f_unconditional)
	    hash_name(name, &sb);

	if (((ar_format == TAR)
	     ? buf_skip(ROUNDUP((OFFSET) sb.sb_size, BLOCKSIZE))
	     : buf_skip((OFFSET) sb.sb_size)) < 0) {
	    warn(name, gettxt(APPEND_CORRUPT, "File data is corrupt"));
	}
    }
    /* we have now gotten to the end of the archive... */

    backup();	/* adjusts the file descriptor and buffer pointers */

    create_archive();
}


/* backup - back the tape up to the end of data in the archive.
 *
 * DESCRIPTION
 *
 *	The last header we have read is either the cpio TRAILER!!! entry
 *	or the two blocks (512 bytes each) of zero's for tar archives.
 * 	adjust the file pointer and the buffer pointers to point to
 * 	the beginning of the trailer headers.
 */


void backup(void)

{
	
        if (lseek(archivefd, -(off_t)(bufend-bufstart), SEEK_CUR) == -1) {
	    warn("lseek", strerror(errno));
	    fatal(gettxt(APPEND_BACK2, "backspace error"));
        }

	bufidx = lastheader; 	/* point to beginning of trailer */
	/*
	 * if lastheader points to the very end of the buffer
	 * Then the trailer really started at the beginning of this buffer
	 */
	if (bufidx == bufstart+blocksize)
		bufidx = bufstart;
}
