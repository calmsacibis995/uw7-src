#ident	"@(#)revision.h	15.1"

/* revision.h -- define the version number
 * Copyright (C) 1992-1993 Jean-loup Gailly.
 * This is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License, see the file COPYING.
 */

#define VERSION "1.0.6"
#define PATCHLEVEL 0
#define REVDATE "10 Mar 93"

/* This version does not support compression into old compress format: */
#ifdef LZW
#  undef LZW
#endif
