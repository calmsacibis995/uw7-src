#ident	"@(#)OSRcmds:cpio/links.c	1.1"
/***************************************************************************
 *			links.c
 *--------------------------------------------------------------------------
 *  Handle defered links in the new new ASCII and CRC header formats.
 *
 *--------------------------------------------------------------------------
 *	@(#) links.c 25.3 94/11/29 
 *
 *	Copyright 1993-1994 The Santa Cruz Operation, Inc
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *--------------------------------------------------------------------------
 * Revision History:
 *
 *	L000	01 Sep 1993		scol!ashleyb
 *	  - Module created. If the list proves to grow too large, it
 *	    may be worth splitting them into an array of some prime
 *	    then using inode % prime to find start of correct list.
 *	L001	07 Jul 1994		scol!ashleyb
 *	  - Use errorl and psyserrorl for errors (unmarked).
 *	L002	08 Nov 1994		scol!trevorh
 *	 - message catalogued.
 *
 *==========================================================================
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/osr.h"
/* #include <errormsg.h> */						/* L002 Start */
#ifdef INTL
#  include <locale.h>
#  include "cpio_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str)	(str)
#  define catclose(d)		/*void*/
#endif /* INTL */						/* L002 Stop */

#define _LINK_C
#include "cpio.h"
#include "errmsg.h"

/* linkslist will always point to the head of our list. All the routines
 * in this module expect this.
 */
static LinksPtr linkslist = (LinksPtr)NULL;

int addlink(header *hdr)
{
	LinksPtr	tmp;

	if ((tmp = (LinksPtr)zmalloc(EERROR,sizeof(Links)))
			== (LinksPtr)NULL)
		return(0);

	if ((tmp->name = strdup(hdr->h_name)) == (char *)NULL) {
		free(tmp);
		return(0);
	}

	tmp->major = hdr->h_dev_maj;
	tmp->minor = hdr->h_dev_min;
	tmp->inode = hdr->h_ino;
	tmp->mode  = hdr->h_mode;
	tmp->nlink = hdr->h_nlink;
	tmp->uid   = hdr->h_uid;
	tmp->gid   = hdr->h_gid;
	tmp->mtime = hdr->h_mtime;

	tmp->next = linkslist;
	tmp->prev = (LinksPtr)NULL;

	if (linkslist != (LinksPtr)NULL)
		linkslist->prev = tmp;

	linkslist = tmp;

	return(1);
}

void freelinkslist()
{
	LinksPtr tmp = linkslist;

	while (tmp != (LinksPtr)NULL)
	{
		LinksPtr tmp2 = tmp;

		tmp = tmp->next;

		free(tmp2->name);
		free(tmp2);
	}

	linkslist = (LinksPtr)NULL;
}


/* Remove an element from the list. */
void remove_element(LinksPtr elem)
{
	LinksPtr tmp;

	if (elem == (LinksPtr)NULL)
		return;

	/* Are we first in the list? */
	if (elem->prev == (LinksPtr)NULL)
	{
		/* Move on linkslist. If we are now not at the end
		 * of the list, null the prev pointer of the new
		 * first element.
		 */
		linkslist = elem->next;
		if (linkslist != (LinksPtr)NULL)
			linkslist->prev = (LinksPtr)NULL;
	}

	/* or are we last in list ? */
	else if (elem->next == (LinksPtr)NULL)
	{
		/* Move back one, and null out the next pointer.
		 */
		tmp = elem->prev;
		tmp->next = (LinksPtr)NULL;
	}
	/* we must be in the middle of the list. */
	else
	{
		/* Move back one, patch this. */
		tmp = elem->prev;
		tmp->next = elem->next;

		/* Move forward one and patch. */
		tmp = elem->next;
		tmp->prev = elem->prev;
	}
	/* Free the removed element. */
	free(elem->name);
	free(elem);
}

/*
 * Find all the link elements that have identical major, minor and inode
 * numbers and remove these from the list.
 *
 */
void remove_link_recs(ulong major, ulong minor, ulong inode)
{
	LinksPtr	tmp = linkslist;
	LinksPtr	tmp2;

	while (tmp != (LinksPtr)NULL)
	{
		/* Test is done inode first as this is the most likely
		 * to fail, allowing us to short circuit the if statement
		 * ASAP.
		 */
		if ((tmp->inode == inode) && (tmp->minor == minor) &&
		    (tmp->major == major))
		{
			tmp2 = tmp->next;
			remove_element(tmp);
			tmp = tmp2;
		} else
			tmp = tmp->next;
	}
}

int linkcount(header *hdr)
{
	int		count = 0;
	LinksPtr	tmp = linkslist;

	while (tmp != (LinksPtr)NULL)
	{
		if ((tmp->inode == hdr->h_ino) &&
		    (tmp->minor == hdr->h_dev_min) &&
		    (tmp->major == hdr->h_dev_maj))
			count++;
		tmp=tmp->next;
	}
	return(count);
}

/* Link rec->name to hdr->h_name which has already been created in the filesystem. */
void do_link(header hdr, LinksPtr rec)
{
	/* Need to make any missing directories. */
	if (missdir(rec->name) != 0){
		psyserrorl(errno,MSGSTR(LINKS_MSG_NO_CREATE_DIR, "cannot create directory for <%s>"),rec->name);
		return;
	}

	/* Don't worry if the file does not already exist. */
	if ((unlink(rec->name) == -1) && (errno != ENOENT))
		psyserrorl(errno,MSGSTR(LINKS_MSG_NO_REM, "could not remove %s"),rec->name);

	if (link(hdr.h_name,rec->name) == -1)
		psyserrorl(errno,MSGSTR(LINKS_MSG_NO_LINK, "could not link %s to %s"),rec->name, hdr.h_name);
}

/* This routine takes care of any real 0 length files that are not links to
 * anything else in the archive. Things are not as neat as they could be are
 * this code is duplicated in utiltiy.c
 */
void do_node(LinksPtr node)
{
        int	ans = 0;

	do {
		if(creat(node->name, (mode_t)node->mode) < 0) {
			ans += 1;
		}else {
			ans = 0;
			break;
		}
	}while(ans < 2 && missdir(node->name) == 0);
	if(ans == 1) {
		psyserrorl(errno,MSGSTR(LINKS_MSG_NO_CREATE_DIR, "cannot create directory for <%s>"), node->name);
		return;
	}else if(ans == 2) {
		psyserrorl(errno,MSGSTR(LINKS_MSG_NO_CREATE, "cannot create <%s>"), node->name);
		return;
	}

	zchmod( EWARN, node->name, node->mode);
	/* If we are root then set ownership. */
	if(getuid() == 0)
		zchown( EWARN, node->name, node->uid, node->gid);

	set_time(node->name,node->mtime,node->mtime);

	return;
}

/* We have just restored hdr.h_name, so scan our list for any links that
 * should be built.
 * Build each link then remove the records from the list.
 */
void create_links(header hdr)
{
	LinksPtr	next_rec = linkslist;

	while(next_rec != (LinksPtr)NULL)
	{
		if ((next_rec->inode == hdr.h_ino) &&
		    (next_rec->minor == hdr.h_dev_min) &&
		    (next_rec->major == hdr.h_dev_maj))
		{
			do_link(hdr,next_rec);
		}
		next_rec = next_rec->next;
	}
	remove_link_recs(hdr.h_dev_maj, hdr.h_dev_min, hdr.h_ino);

}

/* Create any real 0 length files that will still be lurking in our list
 * then free up the list.
 */
void flush_link_list()
{
	LinksPtr	record = linkslist;

	while(record != (LinksPtr)NULL)
	{
		do_node(record);
		record = record->next;
	}

	freelinkslist();
}

/* Get next link goes through the link list looking for a link to this
 * file. If one is found the name is returned, and the record deleted.
 */
char *get_next_link(ulong inode, ulong major, ulong minor)
{
	static char	name[PATHSIZE];
	LinksPtr tmp = linkslist;

	while (tmp != (LinksPtr)NULL)
	{
		if ((inode == tmp->inode) && (minor == tmp->minor) &&
			(major == tmp->major))
		{
			/* Found a match, so store the name and delete
			 * the record.
			 */
			strncpy(name,tmp->name,PATHSIZE);
			remove_element(tmp);
			return(name);
		}
		tmp = tmp->next;
	}
	return((char *)NULL);
}


/* Print out the link data in the correct format. Use the filesize
 * from hdr rather than the 0 bytes in our store.
 */
void plinks(header *hdr, short toc, short verbose, ushort filesum, short nflag)
{
	LinksPtr tmp = linkslist;

	while (tmp != (LinksPtr)NULL)
	{
		if ((tmp->inode == hdr->h_ino) &&
		    (tmp->minor == hdr->h_dev_min) &&
		    (tmp->major == hdr->h_dev_maj))
		{
			if (verbose)
				if (toc)
					pentry(tmp->name, "", tmp->uid, tmp->mode,hdr->h_filesize, tmp->mtime, filesum, nflag);
				else
					verbdot(stdout,tmp->name);
			else if (toc)
				puts(tmp->name);
		}
		tmp = tmp->next;
	}
}
