#ident	"@(#)bcollapse.c	11.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)bcollapse.c	1.1 'attmail mail(1) command'"
#include "libmail.h"
/* based on PMX library mx_collaps.c 2.8 of 11/8/90 */

/*
    NAME
	bang_collapse - collapse a bang-path mail address, removing loops

    DESCRIPTION
	Bang_collapse scans a mail address written in uucp bang format and
	collapses it, removing loops. For example,

		sysa!sysb!sysa!user

	can be collapsed down to

		sysa!user

	because sysb can only talk with one sysa. Similarly,

		sysa!sysa!user

	is collapsed down to to

		sysa!user

    RETURNS
	 0 - all okay
	-1 - bad address
	-2 - out of memory
*/

int bang_collapse(addr)
char	*addr;
{
	char	*ptr1, *ptr2;
	register int	i, start;
	int	hopcnt, lastone = -1;
	struct hop {
		char	*name;
		int	next;
	};
#define MAXHOPS 65
#define HOPINC 10
	struct hop _hoplist[MAXHOPS], *hoplist = &_hoplist[0], *h1, *h2, *h3;
	int curmaxhops = MAXHOPS;
	char *clustername;

	/* If string doesn't contain a '!', return unchanged */
	/* Also, if just one '!' and it's the first character */
	/* Return error if string ends in '!' */

	if (strchr (addr, '!') == NULL) {
		return (0);
	}
	if (*addr == '!'){
		if (strchr (addr+1, '!') == NULL) {
			return (0);
		}
	}
	if (addr[ strlen(addr)-1 ] == '!') {
		return (-1);
	}

	hoplist[0].name = mailsystem(1);
	hoplist[0].next = 1;
	hoplist[1].name = clustername = mailsystem(0);
	hoplist[1].next = hopcnt = 2;

	/*
	 * Parse address into hops.
	 */
	for (ptr1 = strtok(addr, "!"); ptr1; ptr1 = strtok((char*)0, "!")) {
		/* do we have enough room? */
		if (hopcnt == curmaxhops) {
			if (hoplist == _hoplist) {
				hoplist = (struct hop*) malloc(sizeof(struct hop) * (curmaxhops + HOPINC));
				if (!hoplist)
					return -2;
				(void) memcpy(hoplist, _hoplist, curmaxhops);
				curmaxhops += HOPINC;
			} else {
				curmaxhops += HOPINC;
				hoplist = (struct hop*) realloc(hoplist, curmaxhops);
				if (!hoplist)
					return -2;
			}
		}
		/* save the pointer */
		hoplist[hopcnt].name = ptr1;
		hopcnt++;
		hoplist[hopcnt-1].next = hopcnt;
	}
	hoplist[hopcnt-1].next = -1; /* no more entries */

	/* Don't collapse username... */
	lastone = hopcnt - 2;

	/* Collapse entries. */
	start = 0;
	while (start < hopcnt) {
		h1 = &hoplist[start];
		if ((h1->next >= 0) && (h1->next <= lastone)) {
			h2 = &hoplist[h1->next];
			if ((h2->next >= 0) && (h2->next <= lastone)) {
				h3 = &hoplist[h2->next];
			} else {
				h3 = (struct hop *)NULL;
			}
		} else {
			/* Done! */
			break;
		}

		/*
		 * First Compare 1st and 3rd Hops
		 */
		if (h3 != (struct hop *)NULL) {
			if (strcmp(h1->name,h3->name) == 0) {
				h1->next = h3->next;
				start = 0;
				continue;
			}
			/*
			 * Now compare 2d and 3rd Hops
			 */
			if (strcmp(h2->name,h3->name) == 0) {
				h2->next = h3->next;
				start = 0;
				continue;
			}
		}
		/* Lastly, compare 1st and 2d Hops */
		if (strcmp(h1->name,h2->name) == 0) {
			h1->next = h2->next;
			start = 0;
			continue;
		}
		start = hoplist[start].next;
	}

	/* Construct the new address. */
	ptr1 = addr;
	i = hoplist[0].next;	/* Don't include first hop (local system) */
	if (strcmp(hoplist[i].name, clustername) == 0 && hoplist[i].name != 0)
		i = hoplist[i].next;

	while(i >= 0) {
		for (ptr2 = hoplist[i].name; *ptr2; )
			*ptr1++ = *ptr2++;
		if ((i = hoplist[i].next) >= 0)
			*ptr1++ = '!';
	}
	*ptr1 = '\0';

	/* clean up */
	if (hoplist != _hoplist)
		free(hoplist);

	return (0);
}
