#ident	"@(#)ucb:i386/ucblib/libc/i386/gen/makefile	1.2"
#ident	"$Header$"
#	copyright	"%c%"




#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#
# makefile for ucblib/libc/i386/gen
#

include $(LIBRULES)

CFLAGS=-c

OBJECTS= alloca.o

SOURCES= alloca.s

.s.o:
	$(CC) $(CFLAGS) $*.s

ALL:	$(OBJECTS)

all: ALL

clean:
	rm -rf *.o

clobber:
	rm -rf *.o
