#ident "@(#)proto.h	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 *
 */
#ifdef __STDC__
# define	P(s) s
#else
# define P(s) ()
#endif


/* tcpcfg.c */
int conv_addr P((char *str , struct in_addr *addr ));
int conv_inetaddr P((char *str , struct in_addr *addr ));
int conv_netmask P((char *str , struct in_addr *mask ));
void usage P((void ));

#undef P
