/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:oh/oh_init.c	1.8.3.4"

#include <stdio.h>
#include "inc.types.h"		/* abs s14 */
#include <sys/stat.h>
#include "wish.h"
#include "typetab.h"
#include "sizes.h"	/* dmd s15 */

extern char     *nstrcat();
extern long	Dispmodes, Sortmodes;
extern time_t   Prefmodtime;	/* EFT abs k16 */
extern int	Vflag;

void
oh_init()
{
	void	oot_init(), init_modes();

	if (Vflag)
		init_modes();
	oot_get();
	return;
}

void
init_modes()
{
        time_t	oldpref;	/* EFT abs k16 */
	char	*value;
	struct	stat sbuf;
	extern char *Home;
	char	destination[PATHSIZ];	/* dmd s15 */
	char	*getepenv();

	/* folders need updating if the pref directory has been touched
	 * since the SORTMODES and DISPLAYMODES have been read.
	 * So, any form that wants to update all the folders on the screen
	 * need only touch $HOME/pref.
	 */
	oldpref = Prefmodtime;
	sprintf(destination,"%s/pref",Home ? Home : "");	/* dmd s15 */
	if (stat(destination, &sbuf) != FAIL) {
		Prefmodtime = sbuf.st_mtime;
		if (oldpref == Prefmodtime)
			return;	/* no need to reread variables if hasn't changed */
	}
#ifdef _DEBUG
	else
		_debug(stderr, "pref stat failed\n");
#endif

	/* get environment settings; if not set, use defaults */

	if (((value = getepenv("DISPLAYMODE")) == NULL) || (value[0] == '\0'))
		Dispmodes = OTT_DOBJ;
	else {
		switch (value[0]) {
		case 'T':	/* object Type */
			Dispmodes = OTT_DOBJ;
			break;
		case 'M':	/* Modification Time */
			Dispmodes = OTT_DMTIME;
			break;
		case 'S':
			Dispmodes = 0;
			break;
		default:
			Dispmodes = strtol(value, NULL, 16);
			break;
		}
	}

	if (((value = getepenv("SORTMODE")) == NULL) || (value[0] == '\0'))
		Sortmodes = OTT_SALPHA;
	else {
		switch (value[0]) {
		case 'A':	/* Alphabetic */
			Sortmodes = OTT_SALPHA;
			break;
		case 'M':	/* Most Recent */
			Sortmodes = OTT_SMTIME;
			break;
		case 'L':	/* Least Recent */
			Sortmodes = OTT_SMTIME|OTT_SREV;
			break;
		case 'O':
			Sortmodes = OTT_SOBJ;
			break;
		default:
			Sortmodes = strtol(value, NULL, 16);
		}
	}
}
