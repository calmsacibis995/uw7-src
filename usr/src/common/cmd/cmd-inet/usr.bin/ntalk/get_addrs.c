#ident	"@(#)get_addrs.c	1.2"
#ident	"$Header$"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)get_addrs.c	5.5 (Berkeley)	10/11/88
 */

/*	Convergent Technologies - System V - Aug 1987	*/
#ident	"@(#)get_addrs.c	1.3 :/source/net/cmd/talk/s.get_addrs.c 5/27/88 16:59:37"


#include "talk_ctl.h"
#include <netdb.h>

#ifdef SYSV
#define bcmp(a,b,c)     memcmp((a),(b),(c))
#define bcopy(a,b,c)    memcpy((b),(a),(c))
#endif /* SYSV */


get_addrs(my_machine_name, his_machine_name)
	char *my_machine_name, *his_machine_name;
{
	struct hostent *hp;
	struct servent *sp;

	msg.pid = htonl(getpid());
	/* look up the address of the local host */
	hp = gethostbyname(my_machine_name);
	if (hp == NULL) {
		fprintf(stderr, "talk: %s: ", my_machine_name);
		herror((char *)NULL);
		exit(-1);
	}
	bcopy(hp->h_addr, (char *)&my_machine_addr, hp->h_length);
	/*
	 * If the callee is on-machine, just copy the
	 * network address, otherwise do a lookup...
	 */
	if (strcmp(his_machine_name, my_machine_name)) {
		hp = gethostbyname(his_machine_name);
		if (hp == NULL) {
			fprintf(stderr, "talk: %s: ", his_machine_name);
			herror((char *)NULL);
			exit(-1);
		}
		bcopy(hp->h_addr, (char *) &his_machine_addr, hp->h_length);
	} else
		his_machine_addr = my_machine_addr;
	/* find the server's port */
	sp = getservbyname("ntalk", "udp");
	if (sp == 0) {
		fprintf(stderr, "talk: %s/%s: service is not registered.\n",
		     "ntalk", "udp");
		exit(-1);
	}
	daemon_port = sp->s_port;
}
