#ident "@(#)endpt.h	1.2"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1996 Computer Associates International, Inc.
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
 */

/*
 * This file contains declarations for the functions in endpt.c.
 */

#ifdef __STDC__
# define P(s) s
#else
# define P(s) ()
#endif

int get_endpt P((u_short port));
int set_endpt_opt P((int endpt, int level, int option, void *value, int len));
int send_packet P((int endpt, struct sockaddr_in *dest, void *buf, int len));
int recv_packet P((int endpt, void *buf, int len, struct sockaddr_in *from,
	u_long *ifindex));
int is_endpt P((int fd, struct sockaddr_in *addr));

#undef P
