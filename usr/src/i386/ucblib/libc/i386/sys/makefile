#ident	"@(#)ucb:i386/ucblib/libc/i386/sys/makefile	1.4"
#ident	"$Header$"
#	copyright	"%c%"

#
#
# makefile for ucblib/libc/i386/sys
#

include $(LIBRULES)

CFLAGS=-c

OBJECTS= syscall.o _times.o gettimeofday.o

SOURCES= syscall.s _times.s gettimeofday.s

.s.o:
	$(CC) $(CFLAGS) $*.s

ALL:	$(OBJECTS)

all:	ALL

clean:
	rm -rf *.o

clobber:
	rm -rf *.o
