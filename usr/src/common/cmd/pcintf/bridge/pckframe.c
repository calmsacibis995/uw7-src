#ident	"@(#)pcintf:bridge/pckframe.c	1.1.1.2"
#include	"sccs.h"
SCCSID(@(#)pckframe.c	6.6	LCC);	/* Modified: 11:04:34 11/19/91 */

/*****************************************************************************

	Copyright (c) 1984 Locus Computing Corporation.
	All rights reserved.
	This is an unpublished work containing CONFIDENTIAL INFORMATION
	that is the property of Locus Computing Corporation.
	Any unauthorized use, duplication or disclosure is prohibited.

*****************************************************************************/

#include "sysconfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#ifdef BSDGROUPS

#ifdef LOCUS
#include	<grp.h>
#define MAXGROUPS	NGROUPS
#endif

#ifdef SVR4
#include	<limits.h>
#define MAXGROUPS	NGROUPS_MAX
#endif

#ifdef SCO_ODT
/*
 * I could use the SVR4 '#ifdef', but ODT isn't SVR4 (yet).  SVR4 is probably
 * the wrong condition for that definition, but that's what happens when
 * people start relying on conditional compilation.  And I'm not about to
 * change it now.
 */
#include	<limits.h>
#define MAXGROUPS	NGROUPS_MAX
#endif

#ifndef MAXGROUPS
.error MAXGROUPS not defined
#endif

#endif /* BSDGROUPS */

#include "pci_proto.h"
#include "pci_types.h"
#include "common.h"

unsigned short	btime	PROTO((struct tm *));
unsigned short	bdate	PROTO((struct tm *));


/*		Routines Used for Packing Output Packets		*/


/*
 * btime() -		Takes the file creation time from a stat() call
 *			and converts it to MS-DOS time.
 */

unsigned short
btime(ptr)
register struct tm *ptr;
{
	register int 
	    hour,
	    min;

	    hour = ptr->tm_hour;
	    hour <<= 11;
	    min = ptr->tm_min;
	    min <<= 5;
	    return (unsigned short)(hour | min | ptr->tm_sec/2);
}


/*
 * bdate() -		Takes file creation date and converts into an DOS date.
 */

unsigned short 
bdate(ptr)
register struct tm *ptr;
{
	register int 
	    year,
	    month;

	    year = ptr->tm_year - 80;
	    year <<= 9;
	    month = ptr->tm_mon + 1;
	    month <<= 5;
	    return (unsigned short)(year | month | ptr->tm_mday);
}



/*
 * attribute() -	Sets the file attribute byte in a packet for MS-DOS.
 */

unsigned char
attribute(statBuf, filename)
struct stat	*statBuf;
char		*filename;
{
	register unsigned char dosAttr = 0;	/* MS-DOS File attributes */
	char *fp;
#ifdef BSDGROUPS
	gid_t gidset[MAXGROUPS];
	int grps, i, getgroups();
#endif

	if ((statBuf->st_mode & S_IFMT) == S_IFDIR)
		dosAttr |= SUB_DIRECTORY;
	else {
		/* Plain files always appear un-archived */
		dosAttr |= ARCHIVE;

		if (statBuf->st_uid == getuid()) {
			if (!(statBuf->st_mode & O_WRITE))
				dosAttr |= READ_ONLY;
		} else if (statBuf->st_gid == getgid()) {
			if (!(statBuf->st_mode & G_WRITE))
				dosAttr |= READ_ONLY;
		} else {
#ifdef BSDGROUPS
			/*
			 *  Check if a group you belong to has write access
			 */
			grps = getgroups(MAXGROUPS, gidset);
			for (i = 0; i < grps; i++) {
				if (statBuf->st_gid == gidset[i]) {
					if (!(statBuf->st_mode & G_WRITE))
						dosAttr |= READ_ONLY;
					goto found_group;
				}
			}
#endif /* BSDGROUPS */
	
			if (!(statBuf->st_mode & W_WRITE))
				dosAttr |= READ_ONLY;
		}
#ifdef BSDGROUPS
found_group:
#endif /* BSDGROUPS */
		if ((fp = strrchr(filename, '/')) == NULL)
			fp = filename;
		if (*fp == '.')
			dosAttr |= HIDDEN;
	}
	return dosAttr;
}


/*
 *	pckframe() -	Packs frame header data into an output buffer.
 *			NOTE:  At present (7/30/86), this routine is never
 *			called with anything but a null file descriptor (fds)
 *			field, and thus, it is OK to pack the current UNIX
 *			clock time into the header.  If this routine is ever
 *			used in the future with an actual fds, pckframe should
 *			be modified to use the virtual DOS time stamp for the
 *			given file descriptor (taken from the vfCache kept in
 *			vfile.c.  Please see routines get_dos_time() and 
 *			get_vdescriptor() and module vfile.c).
 */

#if defined(__STDC__)
void pckframe(register struct output *addr, int sel, int seq, unsigned char req,
		int stat, int res, int fds, int bcnt, int tcnt, int mode,
		int size, int off, int attr, int date,
		register struct stat *fptr)
#else
void
pckframe(addr, sel, seq, req, stat, res, fds, bcnt, tcnt, mode,
size, off, attr, date, fptr)
register struct output *addr;
int sel, seq, stat, res, fds, bcnt, tcnt, mode, size, off, attr,
date;
unsigned char req;
register struct	stat *fptr;
#endif
{
	register struct tm 
	    *tptr;

	    addr->pre.select = (char)sel;
	    addr->hdr.seq    = (char)seq;
	    addr->hdr.req    = (char)req;
	    addr->hdr.stat   = (char)stat;
	    addr->hdr.res    = (char)res;
	    addr->hdr.b_cnt  = (short)bcnt;
	    addr->hdr.t_cnt  = (short)tcnt;
	    addr->hdr.fdsc   = (short)fds;
	    addr->hdr.offset = (long)off;
	    addr->hdr.mode   = (mode) ? fptr->st_mode : 0;
	    addr->hdr.f_size = (size) ? fptr->st_size : 0;
	    addr->hdr.attr   = (attr) ? attribute(fptr, "") : 0;
	    if (date)
	    	tptr = localtime(&(fptr->st_mtime));
	    addr->hdr.date = (date) ? bdate(tptr) : 0;
	    addr->hdr.time = (date) ? btime(tptr) : 0;
}

/*
 * Name: pckRD
 * Purpose: packs a reliable delivery header with appropriate stuff
 * Arguments: addr,    Address of header.
 *            code,    Command code
 *            seq1,    Sequence number of command
 *            seq2,    Sequence number ack number
 *            options: Options flags
 *            version: Current version of RD
 */

#if defined(__STDC__)
void pckRD(register struct emhead *addr, unsigned char code,
		unsigned short seq1, unsigned short seq2, unsigned char options,
		unsigned char tcbn, unsigned char version)
#else
void
pckRD( addr, code, seq1, seq2, options, tcbn, version )
register struct emhead *addr;
unsigned char code, options, tcbn, version;
unsigned short seq1, seq2;
#endif
{
            addr->code = code;
	    addr->dnum = seq1;
            addr->anum = seq2;
	    addr->options = options;
	    addr->tcbn    = tcbn;
	    addr->version = version;
}

/* rdtest: test whether a == b, which returns 0
 *                       a < b, which returns -1
 *                       a > b, which returns 1
 * This function is used for testing protocol state variables in the
 * reliable delivery code, which are subject to wrap-around.
 */

#if defined(__STDC__)
int rdtest(unsigned short a, unsigned short b)
#else
rdtest( a, b )
unsigned short a, b;
#endif
{
	if ( a == b) return 0;

	if ( !( a == RD_SEQ_MAX || b == RD_SEQ_MAX) ) /* not wrapped */
	{
		if (a < b)  return -1;
	        if (a > b)  return 1;
	}
	else           /* one of the two is wrapped */
	{
		if ( a == RD_SEQ_MAX )       /* a is wrapped */
		{
			if (b==0) return -1; /* a<b */
			return 1;            /* a>b */
		}
		else /* b is wrapped */
		{
			if (a==0) return 1; /* a>b */
			return -1;          /* a<b */
		}
	}
}
