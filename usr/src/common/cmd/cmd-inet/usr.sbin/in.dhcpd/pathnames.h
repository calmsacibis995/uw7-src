#ident "@(#)pathnames.h	1.4"
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

#define _PATH_CONFIG_FILE	"/etc/inet/dhcpd.conf"
#define _PATH_PID_FILE		"/etc/inet/dhcpd.pid"

#ifdef _USE_PATHS
#include <paths.h>
#else
#define _PATH_IP	"/dev/ip"
#define _PATH_UDP	"/dev/udp"
#define _PATH_ARP	"/dev/arp"
#define _PATH_CONSOLE	"/dev/console"
#endif
