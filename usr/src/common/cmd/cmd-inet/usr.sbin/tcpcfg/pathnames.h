#ident "@(#)pathnames.h	1.2"
/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1993 Lachman Technology, Inc.
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
#include <paths.h>
#ifdef DEBUG
#define _PATH_OUTPUT "/tmp/file"
#else
#define _PATH_OUTPUT "/usr/lib/netcfg/tmp/sco_tcp.conf"
#endif

