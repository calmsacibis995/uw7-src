#ident	"@(#)OSRcmds:cpio/devlist.c	1.1"
/***************************************************************************
 *			devlist.c
 *--------------------------------------------------------------------------
 * Functions that manipulate device lists, for multi-volume, multi-device
 * transfers.
 *
 *--------------------------------------------------------------------------
 *	@(#) devlist.c 25.1 93/10/19 
 *
 *	Copyright 1993 The Santa Cruz Operation, Inc
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 *
 *--------------------------------------------------------------------------
 * Revision History:
 *
 *	L000	01 Sep 1993		scol!ashleyb
 *	  - Module created in an attempt to clean up code.
 *
 *==========================================================================
 */
#include <sys/types.h>
#include <sys/stat.h>
/* #include <sys/param.h> */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define _DEVLIST_C
#include "../include/osr.h"
#include "cpio.h"
#include "errmsg.h"

int adddev(DevPtr *devlist, char *device)
{
	DevPtr new;

	if (*devlist != (DevPtr)NULL)
		return(adddev(&((*devlist)->next),device));

	if ((new = (DevPtr)zmalloc(EERROR,sizeof(DevList))) == (DevPtr)NULL)
		return(0);

	if ((new->device = (char *)zmalloc(EERROR,strlen(device)+1)) == (char *)NULL){
		free(new);
		return(0);
	}

	strcpy(new->device,device);
	new->used = 0;
	new->next = (DevPtr)NULL;
	*devlist = new;

	return(1);
}

void freedevlist(DevPtr devlist)
{
	if (devlist != (DevPtr)NULL)
	{
		free(devlist->device);
		if (devlist->next != devlist)
			freedevlist(devlist->next);
		free(devlist);
	}
}

DevPtr blddevlist(char *device_string)
{
	DevPtr	devlist=(DevPtr)NULL;
	DevPtr	tmp;
	char	*device;

	/* Move along the device list. A device is terminated
	   by a comma or a NULL. For each device found, add it
 	   to the DevList we are building.
	*/

	if ((device=strtok(device_string,",")) == NULL)
		return((DevPtr)NULL);

	do
		adddev(&devlist,device);
	while((device = strtok(NULL,",")) != NULL);

	/* Now that we have a device list, link the last device in the list
	   to itself. This means that we will never run out of devices to
	   use.
	*/

	tmp = devlist;
	while(tmp->next != (DevPtr)NULL)
		tmp=tmp->next;
	tmp->next=tmp;

	return(devlist);
}
