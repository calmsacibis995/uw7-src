#ident	"@(#)getif.h	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* getif.h */

#ifdef	__STDC__
extern struct ifreq *getif(int, struct in_addr *);
#else
extern struct ifreq *getif();
#endif
