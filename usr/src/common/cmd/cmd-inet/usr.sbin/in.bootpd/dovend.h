#ident	"@(#)dovend.h	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* dovend.h */

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern int dovend_rfc1497 P((struct host *hp, byte *buf, int len));
extern int insert_ip P((int, struct in_addr_list *, byte **, int *));
extern void insert_u_long P((u_long, byte **));

#undef P
