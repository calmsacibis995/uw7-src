#ifndef _COPYRIGHT_H
#define _COPYRIGHT_H

#ident	"@(#)copyright.h	1.4"

/*
 * copyright for i386 machines
 */

#if 0

#define	COPYRIGHT(un) \
        (void) printf("UNIX(r) System V Release %s Version %s   (support level 7.0.0t)\n%s\n\
Copyright (c) 1976-1998 The Santa Cruz Operation, Inc. and its suppliers.\n\
All Rights Reserved.\n\ 
\n\
RESTRICTED RIGHTS LEGEND:\n\
\n\
When licensed to a U.S., State, or Local Government,\n\
all Software produced by SCO is commercial computer software\n\
as defined in FAR 12.212, and has been developed exclusively\n\
at private expense.  All technical data, or SCO commercial\n\
computer software/documentation is subject to the provisions\n\
of FAR 12.211 - \"Technical Data\", and FAR 12.212 - \"Computer\n\
Software\" respectively, or clauses providing SCO equivalent\n\
protections in DFARS or other agency specific regulations.\n\
Manufacturer: The Santa Cruz Operation, Inc., 400 Encinal\n\
Street, Santa Cruz, CA 95060.\n\n",\
	un.release, un.version, un.nodename)

#else				/* UnixWare licensee */

#define	COPYRIGHT(un) \
        (void) printf("UnixWare %s   (support level 7.0.0t)\n%s\n\
Copyright (c) 1976-1998 The Santa Cruz Operation, Inc. and its suppliers.\n\
All Rights Reserved.\n\
\n\
RESTRICTED RIGHTS LEGEND:\n\
\n\
When licensed to a U.S., State, or Local Government,\n\
all Software produced by SCO is commercial computer software\n\
as defined in FAR 12.212, and has been developed exclusively\n\
at private expense.  All technical data, or SCO commercial\n\
computer software/documentation is subject to the provisions\n\
of FAR 12.211 - \"Technical Data\", and FAR 12.212 - \"Computer\n\
Software\" respectively, or clauses providing SCO equivalent\n\
protections in DFARS or other agency specific regulations.\n\
Manufacturer: The Santa Cruz Operation, Inc., 400 Encinal\n\
Street, Santa Cruz, CA 95060.\n\n",\
	un.version, un.nodename)

#endif

#endif
