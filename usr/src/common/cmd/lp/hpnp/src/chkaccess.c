/*		copyright	"%c%"	*/

#ident	"@(#)chkaccess.c	1.3"

/*
 * (c)Copyright Hewlett-Packard Company 1991.  All Rights Reserved.
 * (c)Copyright 1983 Regents of the University of California
 * (c)Copyright 1988, 1989 by Carnegie Mellon University
 * 
 *                          RESTRICTED RIGHTS LEGEND
 * Use, duplication, or disclosure by the U.S. Government is subject to
 * restrictions as set forth in sub-paragraph (c)(1)(ii) of the Rights in
 * Technical Data and Computer Software clause in DFARS 252.227-7013.
 *
 *                          Hewlett-Packard Company
 *                          3000 Hanover Street
 *                          Palo Alto, CA 94304 U.S.A.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "snmp.h"

#include <locale.h>
#include "hpnp_msg.h"

extern nl_catd catd;

extern	char	*ProgName;
extern	int	 errno;
extern	char	*community;

int
check_access(char *netperiph)
{
	char	line[BUFSZ], *val;
	FILE	*pfd;
	int	ret = 0;		/* default, no access */

	if (access(GETONE, X_OK) == 0) {
		sprintf(line, "%s %s %s %s\n", GETONE, netperiph, community, SNMP_ACCESS);

		pfd = popen(line, "r");
		if (!pfd)
		{
			fprintf
			  (
			  stderr,
			  MSGSTR
			    (
			    HPNP_CHKA_CNCPI,
			    "%s: Cannot create pipe to %s - %s\n"
			    ),
			  ProgName, GETONE, strerror(errno)
			  );
			goto finished;
		}

		while (fgets(line, BUFSZ, pfd) != NULL) {
			if (*line != 'V')
				continue;
			val = strchr(line, ' ');

			if (*(++val) == '1')
				ret++;		/* access */
			break;
		}
		pclose(pfd);
	}

finished:
	return ret;
}
