#ident	"@(#)lookup.h	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* lookup.h */

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern u_char *lookup_hwa P((char *hostname, int htype));
extern int lookup_ipa P((char *hostname, u_long *addr));
extern int lookup_netmask P((u_long addr, u_long *mask));

#undef P
