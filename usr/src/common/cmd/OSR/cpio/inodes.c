#ident	"@(#)OSRcmds:cpio/inodes.c	1.1"
#pragma comment(exestr, "@(#) inodes.c 25.2 94/11/29 ")
/***************************************************************************
 *			inodes.c
 *--------------------------------------------------------------------------
 * Handle generatation and lookup of short inode/device numbers for
 * old formats.
 *
 * Module allows us to use the traditional unix, even when we archive
 * 32 bit inode numbers and extended device numbers. In traditional cpio
 * archives, devices where signed shorts (0->0x7fff) and inodes where
 * unsigned shorts(0->0xffff). This gives us the ability to archive
 * over 2000 Million files using the traditional formats.
 *
 *--------------------------------------------------------------------------
 *	Copyright 1993-1994 The Santa Cruz Operation, Inc
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *--------------------------------------------------------------------------
 * Revision History:
 *
 *	L000	02 Sep 1994		scol!ashleyb
 *	- Created.
 *	L001	08 Nov 1994		scol!trevorh
 *	 - message catalogued.
 *
 *==========================================================================
 */
#define _INODES_C
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdio.h>
#include "../include/osr.h"
#include "cpio.h"

/* #include <errormsg.h> */					/* L001 Start */
#ifdef INTL
#  include <locale.h>
#  include "cpio_msg.h"
   nl_catd catd;
#else
#  define MSGSTR(num,str)	(str)
#  define catclose(d)		/*void*/
#endif /* INTL */						/* L001 Stop */


#define HASH_SIZE	1009		/* Size of has table. */

static unsigned short inode;	/* Holds the next unused inode number. */
static short device;		/* Holds the next unused device number. */

typedef struct record {
	ulong		orig_inode;
	ulong		orig_device_maj;
	ulong		orig_device_min;
	ushort		new_inode;
	short		new_device;
	struct record	*next;
} Record, *RecordPtr;

/* Our lookup table is an array of pointers to linked lists of records. */
static RecordPtr	ino_dev_lookup_table[HASH_SIZE];

static RecordPtr ino_dev_lookup(header *hdr)
{
	RecordPtr tmp;

	tmp = ino_dev_lookup_table[(hdr->h_ino % HASH_SIZE)];

	while ((tmp != (RecordPtr)NULL) && (tmp->orig_inode <= hdr->h_ino))
	{
		/* Test inode first, as this is most likely
		 * to be different.
		 */
		if ((tmp->orig_inode == hdr->h_ino) &&
		       (tmp->orig_device_min == hdr->h_dev_min) &&
			  (tmp->orig_device_maj == hdr->h_dev_maj))
				return(tmp);

		tmp=tmp->next;
	}
	return(RecordPtr)NULL;
}

/* Add a new record to the appropriate hash bucket. */
static int ino_dev_add_record(
		ulong maj, ulong min, ulong orig_inode,
		short inode, short device)
{
	RecordPtr tmp,*item;
	int bucket;

	if ((tmp = (RecordPtr)malloc(sizeof(Record))) == (RecordPtr)NULL)
		return(0);

	tmp->orig_inode = orig_inode;
	tmp->orig_device_maj = maj;
	tmp->orig_device_min = min;
	tmp->new_inode = inode;
	tmp->new_device = device;
	tmp->next = (RecordPtr)NULL;

	bucket = orig_inode % HASH_SIZE;

	item = &(ino_dev_lookup_table[bucket]);

	/* Put the inodes in numerical order. */
	while ( *item != (RecordPtr)NULL)
	{
		if ((*item)->orig_inode > tmp->orig_inode)
			break;

		item = &((*item)->next);
	}

	tmp->next = *item;
	*item = tmp;

	return(1);
}

static void increment_ids(void)
{
	/* Bump the inode and device numbers. If we have run out then
	 * tell the user to use expanded headers and exit.
	 */
	if (inode == 0xffff)
	{
		if (device == 0x7fff)
		{
		   errorl(MSGSTR(INODES_MSG_NO_SPACE, "fatal error: no space left in inode table."));
		   errorl(MSGSTR(INODES_MSG_EXP_ASCII, "please use expanded ASCII format (-Hnewc)."));
		   exit(1);
		}
		device++;
	}
	inode++;
}

static void ino_dev_handle_links(header *hdr)
{
	RecordPtr id;
	static add_failed = 0;

	if ((id = ino_dev_lookup(hdr)) != (RecordPtr)NULL)
	{
		/* Found a record of this files link. Therefore set
		 * the device and inode equal the link.
		 */
		hdr->h_dev_maj = 0;
		hdr->h_dev_min = id->new_device;
		hdr->h_ino = id->new_inode;
		return;
	}

	/* New record. Store the old inode/device pair and the new set. */

	if (!add_failed)
	{
		if(!ino_dev_add_record(hdr->h_dev_maj,hdr->h_dev_min,
						hdr->h_ino,inode,device))
		{
		errorl(MSGSTR(INODES_MSG_OUT_MEM, "WARNING: out of memory for link table."));
		errorl(MSGSTR(INODES_MSG_BREAK_LINKS, "Hard links will be broken when archive is recovered."));
		add_failed = 1;
		}
	}

	hdr->h_dev_maj = 0;
	hdr->h_dev_min = device;
	hdr->h_ino = inode;

	increment_ids();
}

void generate_id(header *hdr)
{
	/*
	 * If this file has no links, then give it the next unique
	 * inode/device number pair.
	 */
	if (hdr->h_nlink == 1)
	{
		hdr->h_dev_maj = 0;
		hdr->h_dev_min = device;
		hdr->h_ino = inode;

		increment_ids();
	} else {
		ino_dev_handle_links(hdr);
	}
	return;
}
