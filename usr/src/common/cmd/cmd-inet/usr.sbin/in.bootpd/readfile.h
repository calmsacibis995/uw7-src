#ident	"@(#)readfile.h	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/* readfile.h */

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
 */

#include "hash.h"

#ifdef	__STDC__
#define P(args) args
#else
#define P(args) ()
#endif

extern boolean hwlookcmp P((hash_datum *, hash_datum *));
extern boolean iplookcmp P((hash_datum *, hash_datum *));
extern boolean nmcmp P((hash_datum *, hash_datum *));
extern void readtab P((int));
extern void rdtab_init P((void));
extern struct host *lookup_subnet(ulong);

#undef P

