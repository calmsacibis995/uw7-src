#ident	"@(#)hwaddr.h	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* hwaddr.h */
#ifndef	HWADDR_H
#define HWADDR_H

/*
 * This structure holds information about a specific network type.  The
 * length of the network hardware address is stored in "hlen".
 * The string pointed to by "name" is the cononical name of the network.
 */
struct hwinfo {
    unsigned int hlen;
    char *name;
};

extern struct hwinfo hwinfolist[];
extern int hwinfocnt;

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern void setarp P((int, struct in_addr *, u_char *, int));
extern char *haddrtoa P((u_char *, int));

#undef P

/*
 * Return the length in bytes of a hardware address of the given type.
 * Return the canonical name of the network of the given type.
 */
#define haddrlength(type)	((hwinfolist[(int) (type)]).hlen)
#define netname(type)		((hwinfolist[(int) (type)]).name)

#endif	/* HWADDR_H */
