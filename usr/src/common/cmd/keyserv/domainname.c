/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)domainname.c	1.3"
#ident	"$Header$"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/*
 * domainname -- get (or set domainname)
 */
#include <stdio.h>
#include <errno.h>
#include <locale.h>
#include <pfmt.h>
#include "msg.h"

char domainname[256];
char *msg_label = "UX:domainname";

main(argc,argv)
	int	argc;
        char	*argv[];
{
	int	myerrno = 0;
	int	c;
	
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdomainname");
	(void)setlabel(msg_label);

	while ((c = getopt(argc, argv, "")) != EOF) {
		switch(c) {
		default:
			pfmt (stderr, MM_ERROR, MSG15, argv[0], argv[0]);
			exit(255);
		}
	}

	switch (argc) {
	case 1:
		if (getdomainname(domainname, sizeof (domainname))) {
			myerrno = errno;
			perror("getdomainname");
		} else {
			printf("%s\n", domainname);
		}
		break;
	case 2:
		if (setdomainname(argv[1], strlen(argv[1]))) {
			myerrno = errno;
			perror("setdomainname");
		}
		break;
	default:
		pfmt (stderr, MM_ERROR, MSG15, argv[0], argv[0]);
		myerrno = 255;
		break;
	}
		
	exit(myerrno);
	/* NOTREACHED */
}
